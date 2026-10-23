#!/usr/bin/env python3
"""Compile the actual FPV HUD function with UTF-8/graphics stubs and test RU layout.
Needs Python 3, msgfmt and a C++17 compiler. Does not launch the game or validate fonts.
"""
from pathlib import Path
import gettext, json, os, re, subprocess, tempfile
root = Path(__file__).resolve().parents[2]
source = (root / 'src/sdltiles.cpp').read_text(encoding='utf-8')
a = source.index('void draw_first_person_hud(')
hud = source[a:source.index('#if defined(__ANDROID__)', a)]
with tempfile.TemporaryDirectory(prefix='cdda-fpv-hud-') as tmp:
    tmp = Path(tmp)
    mo = tmp / 'ru.mo'
    subprocess.run(['msgfmt', '-f', '-o', str(mo), str(root / 'lang/po/ru.po')], check=True)
    with mo.open('rb') as f:
        catalog = gettext.GNUTranslations(f)
    entries = {}
    for english in re.findall(r'pgettext\("fpv",\s*"([^"]*)"\)', hud):
        translated = catalog.pgettext('fpv', english)
        assert translated != english, 'Missing translation: ' + english
        entries[english] = translated
    prefix = r'''
#include HEADER_PATH
#include <codecvt>
#include <locale>
#include <map>
#include <set>
#include <memory>
#include <stdexcept>
#include <iostream>
struct point { int x,y; point(int X,int Y):x(X),y(Y){} };
struct SDL_Color {unsigned char r,g,b,a;};
struct SDL_Rect {int x,y,w,h;};
int renderer=0;
struct Geometry {void rect(int,const SDL_Rect &,const SDL_Color &) {}};
std::unique_ptr<Geometry> geometry=std::make_unique<Geometry>();
std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> codec;
std::u32string utf8_view(const std::string &s) {return codec.from_bytes(s);}
int utf8_width(const std::string &s) {return static_cast<int>(utf8_view(s).size());}
int mk_wcwidth(char32_t) {return 1;}
std::string utf32_to_utf8(char32_t c) {return codec.to_bytes(c);}
std::string remove_color_tags(const std::string &s) {return s;}
int cyrillic_glyphs=0;
struct Font {int width=8,height=16;
 void OutputChar(int,const std::unique_ptr<Geometry> &,const std::string &s,point,unsigned char) {
  if(utf8_view(s).at(0)>127)++cyrillic_glyphs;
 }
};
std::unique_ptr<Font> font=std::make_unique<Font>();
enum action_id {ACTION_PAUSE,ACTION_PICKUP,ACTION_INVENTORY,ACTION_EXAMINE,ACTION_FIRE,ACTION_RELOAD_WIELDED,ACTION_OPEN,ACTION_CLOSE,ACTION_SMASH,ACTION_MOVE_UP,ACTION_MOVE_DOWN,ACTION_CONTROL_VEHICLE,ACTION_BIONICS,ACTION_MUTATIONS};
std::string action_ident(action_id a) {const char *ids[]={"pause","pickup","inventory","examine","fire","reload","open","close","smash","up","down","vehicle","bionics","mutations"};return ids[a];}
struct input_event {std::string short_description()const{return "Key";}};
struct input_context {std::vector<input_event> keys_bound_to(const std::string &)const{return {{}};}};
input_context get_default_mode_input_context(){return {};}
const char *pgettext(const char *,const char *s) {
 static const std::map<std::string,std::string> translations=TRANSLATIONS;
 return translations.at(s).c_str();
}
'''
    header_path = str(root / 'src/first_person.h')
    if Path('/usr/bin/cygpath.exe').exists():
        header_path = subprocess.check_output(['cygpath', '-m', header_path], text=True).strip()
    prefix = prefix.replace('HEADER_PATH', json.dumps(header_path))
    prefix = prefix.replace('TRANSLATIONS', '{' + ','.join('{' + json.dumps(k, ensure_ascii=False) + ',' + json.dumps(v, ensure_ascii=False) + '}' for k,v in entries.items()) + '}')
    main = r'''
int main() {
 using namespace first_person;
 int checks=0;
 for(int fw:{8,12,16}) for(int width:{390,600,960}) for(bool inventory:{false,true}) for(int tab:{0,1,2}) {
  font->width=fw;font->height=16;
  inventory_open=inventory;inventory_tab=tab;pointer_x=pointer_y=0;
  body_statuses={{"Голова",70,80,{"Шлем"},"head"},{"Торс",85,100,{"Футболка","Куртка","Рюкзак"},"torso"},{"Левая рука",60,80,{"Куртка"},"arm_l"},{"Правая рука",80,80,{"Куртка"},"arm_r"},{"Левая нога",50,80,{"Джинсы"},"leg_l"},{"Правая нога",80,80,{"Джинсы"},"leg_r"}};
  morale_level=-4;morale_entries={{"Вкусная еда",12},{"Мокрая одежда",-16}};
  selected_target={"стол",4,5,0,false};cell_reachable=true;
  inventory_labels={"Мачете","[-] Рюкзак [надето]","  Карман 1: 250 / 5000 ml","    [+] Аптечка","    Вода (чистая)","Джинсы"};
  cell_labels={"Пластиковая бутылка","Яблоко","Складной нож"};
  draw_first_person_hud(point(0,0),width,600,"ВЗГЛЯД С | ХОД С | F7: обзор мышью","Зомби | z=0","Без оружия",.72f);
  std::set<std::string> commands;
  for(size_t i=0;i<buttons.size();++i) {
   const auto &a=buttons[i];commands.insert(a.action);
   if(a.x<0||a.y<0||a.x+a.w>width||a.y+a.h>600) throw std::runtime_error("Button outside viewport");++checks;
   for(size_t j=0;j<i;++j) {const auto &b=buttons[j];
    if(a.x<b.x+b.w&&a.x+a.w>b.x&&a.y<b.y+b.h&&a.y+a.h>b.y)throw std::runtime_error("Overlapping buttons: "+a.action+" / "+b.action+" width="+std::to_string(width)+" font="+std::to_string(fw));++checks;
   }
  }
  if(!inventory && buttons.size()!=14)throw std::runtime_error("Missing HUD action");
  if(inventory) {
   for(const char *world:{"pause","pickup","examine","open","close","smash","fire","reload","up","down"})
    if(commands.count(world))throw std::runtime_error("World toolbar should not cover inventory");
   for(const char *old:{"interact","attack","open","close_door","up","down"})
    if(commands.count(std::string("fpv:")+old))throw std::runtime_error("World action leaked into item controls");
  }
  if(inventory)for(const char *op:{"close","expand","item_menu","take","put","wield","wear","use"}) {
   if(!commands.count(std::string("fpv:")+op))throw std::runtime_error(std::string("Missing inventory action: ")+op);++checks;
  }
  if(inventory && (width-16>=780 || tab==0)) {
   if(!commands.count("fpv:body:0") || !commands.count("fpv:body:5"))throw std::runtime_error("Missing anatomy parts");
   for(const auto &b:buttons)if(b.action=="fpv:morale") {pointer_x=b.x+1;pointer_y=b.y+1;break;}
   draw_first_person_hud(point(0,0),width,600,"ВЗГЛЯД С | ЛКМ: Атаковать","Зомби","Мачете",.72f);
   if(!morale_hover)throw std::runtime_error("Morale tooltip did not open");++checks;
  }
 }
 if(!cyrillic_glyphs)throw std::runtime_error("No Cyrillic reached font API");
 std::cout<<"RU HUD: "<<checks<<" bounds/overlap/presence checks; 54 layouts; anatomy, nested labels, morale hover; Cyrillic reached font API\n";
}
'''
    cpp = tmp / 'test.cpp'
    cpp.write_text(prefix + hud + main, encoding='utf-8')
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++17', '-O1', str(cpp), '-o', str(tmp/'test')], check=True)
    subprocess.run([str(tmp/'test')], check=True)
