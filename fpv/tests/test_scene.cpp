#include "../../src/first_person_scene.h"
#include <iostream>
#include <stdexcept>
#include <fstream>
using namespace first_person;
int main(int argc,char **argv) {
 int checks=0;
 auto check=[&](bool v,const char *s){++checks;if(!v)throw std::runtime_error(s);};
 rasterizer r;camera c;c.eye={.5,2,1.6};c.yaw=0;c.pitch=0;
 r.begin(320,200,c);draw_sky(r,.5,1,.7,30);
 const auto day=r.pixels;
 check(std::all_of(r.depth.begin(),r.depth.end(),[](double z){return std::isinf(z);}),"sky cannot occlude scene");
 draw_sky(r,.5,0,.7,30);check(r.pixels!=day,"day and night differ");
 draw_sky(r,.5,1,1,30);const auto cloud=r.pixels;
 draw_sky(r,.5,1,0,30);check(r.pixels!=cloud,"cloud coverage changes sky");
 for(const char *id:{"f_table","f_chair","f_locker","f_bed","f_bookcase","unidentified"}) {
  r.begin(320,200,c); model m{r,{.5,-1,0},0,1,4};const double support=m.furniture(id,nullptr,true);
  check(support>=0 && support<=2,"plausible furniture support");
  check(std::count(r.object.begin(),r.object.end(),4)>0,"furniture emits pickable geometry");
 }
 r.begin(320,200,c);model car{r,{.5,-1,0},0,1,9};car.wheel({0,0,.4});
 check(std::count(r.object.begin(),r.object.end(),9)>0,"wheel geometry rendered");
 const image smoke=field_sprite(false,1), fire=field_sprite(true,1),fire2=field_sprite(true,2);
 check(fire.pixels!=fire2.pixels,"procedural fire animates");
 check(std::any_of(smoke.pixels.begin(),smoke.pixels.end(),[](unsigned p){return (p>>24)>0&&(p>>24)<255;}),"smoke has soft alpha");
 r.begin(320,200,c);
 r.quad({{{-2,-3,0},{3,-3,0},{3,-3,3},{-2,-3,3}}},nullptr,0xffbbaa99,1,false,7);
 const auto opaque=r.pixels;
 r.transparent_quad({{{0,-1,.1},{1,-1,.1},{1,-1,2.8},{0,-1,2.8}}},0,20,.4,&smoke,1,false);
 r.finish_transparency();check(r.center_object()==7,"smoke never steals underlying target");
 check(r.pixels!=opaque,"smoke blends over scene");
 for(const char *id:{"f_firering","f_fireplace","f_brazier"}) {
  r.begin(320,200,c);model m{r,{.5,-1,0},0,1,4};
  check(m.furniture(id,nullptr,true)<.18,"fire ring must not become table");
  check(std::count(r.object.begin(),r.object.end(),4)>0,"fire ring geometry rendered");
 }
 for(bool bush:{false,true}) {
  const auto plant=vegetation_sprite(bush,42);
  check(std::any_of(plant.pixels.begin(),plant.pixels.end(),[](unsigned p){return p==0;}),"plant has transparent background");
  check(std::any_of(plant.pixels.begin(),plant.pixels.end(),[](unsigned p){return p>>24==255;}),"plant has opaque leaves");
 }
 check(std::all_of(water_texture().pixels.begin(),water_texture().pixels.end(),[](unsigned p){return p>>24==255;}),"water material covers entire pool");
 repeated_actions.clear();
 check(repeat_action("monster:1")=="interact","new target defaults to interaction");
 remember_action("monster:1","attack");
 check(repeat_action("monster:1")=="attack","same target repeats attack");
 check(repeat_action("monster:2")=="interact","action never transfers to another monster");
 remember_action("monster:1","interact");
 check(repeat_action("monster:1")=="interact","new selection replaces last action");
 remember_action("","attack");check(repeat_action("")=="interact","empty gaze never remembers attack");
 // Highlight modifies visible colors only, never depth, IDs or unrelated surfaces.
 r.begin(160,100,c);
 r.quad({{{-2,-3,0},{3,-3,0},{3,-3,3},{-2,-3,3}}},nullptr,0xffbbaa99,1,false,7);
 r.box({0,-1,.2},{.5,-.5,1.2},nullptr,0xff556677,1,9);
 const auto before=r.pixels;const auto ids=r.object;const auto depths=r.depth;
 highlight_objects(r,{9});
 check(r.object==ids,"highlight never changes target IDs");
 check(r.depth==depths,"highlight never changes depth");
 check(r.pixels!=before,"target tint is visible");
 bool unrelated=true;for(size_t i=0;i<ids.size();++i)if(ids[i]!=9 && r.pixels[i]!=before[i])unrelated=false;
 check(unrelated,"occluded/other cells are not highlighted");
 const auto highlighted=r.pixels;highlight_objects(r,{});
 check(r.pixels==highlighted,"empty highlight does nothing");
 check(std::all_of(curtain_texture().pixels.begin(),curtain_texture().pixels.end(),[](unsigned p){return p>>24==255;}),"closed curtains are opaque cloth");
 for(const char *id:{"f_bed","f_chair","f_sofa"}) {
  r.begin(320,200,c);model a{r,{.5,-1,0},0,1,4};a.furniture(id,nullptr,true);const auto first=r.pixels;
  r.begin(320,200,c);model b{r,{.5,-1,0},pi/2,1,4};b.furniture(id,nullptr,true);
  check(r.pixels!=first,"asymmetric furniture responds to orientation");
 }
 if(argc>1) {
  r.begin(640,400,c);draw_sky(r,.4,.9,.7,12);
  r.quad({{{-10,-20,0},{10,-20,0},{10,5,0},{-10,5,0}}},nullptr,0xff5c6655,.8,false,1);
  model table{r,{-1,-1,0},0,1,2};table.furniture("f_table",nullptr,true);
  model chair{r,{-2,-2,0},.5,1,3};chair.furniture("f_chair",nullptr,true);
  model truck{r,{2,-4,0},.4,1,4};truck.box({-.7,-1,.4},{.7,1,.7},0xff526677);
  for(double x:{-.7,.7})for(double y:{-.7,.7}){model w{r,truck.at({x,y,.4}),first_person::pi/2+.4,1,4};w.wheel({0,0,0});}
  truck.box({-.7,-1,.7},{.7,-.2,1.8},0xff596b76);truck.box({-.7,-1,1.8},{.7,1,1.9},0xff9aa7b0);
  r.transparent_quad({{{0,-2,0},{.9,-2,0},{.9,-2,1.7},{0,-2,1.7}}},0,2,.96,&fire,1,false);r.finish_transparency();
  std::ofstream f(argv[1],std::ios::binary);f<<"P6\n"<<r.w<<" "<<r.h<<"\n255\n";for(auto p:r.pixels){char b[]={char(p>>16),char(p>>8),char(p)};f.write(b,3);}
 }
 std::cout<<checks<<" scene assertions passed (synthetic geometry, not a game integration test)\n";
}
