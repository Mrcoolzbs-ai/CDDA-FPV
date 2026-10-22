#include "../../src/first_person.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace first_person;
static int checks=0;
void check(bool condition,const char *message) { ++checks; if(!condition) throw std::runtime_error(message); }
void tests() {
    for(int f=0;f<8;++f) check(heading(f*pi/4)==f,"8-way camera heading");
    check(heading(-pi/4)==7,"negative yaw wrap");
    facing=2; yaw=0; pitch=0;
    mouse_motion(100,-100,.003,false);
    check(facing==2,"free look must not change movement heading");
    check(std::abs(yaw-.3)<1e-9 && std::abs(pitch-.3)<1e-9,"mouse yaw/pitch");
    mouse_motion(100000,-100000,.003,true);
    check(facing==heading(yaw),"optional linked movement heading");
    check(pitch==1.35,"pitch clamp");
    gameplay_input=false;
    { input_scope outer(true); check(gameplay_input,"game input capture");
      { input_scope modal(false); check(!gameplay_input,"modal must release mouse"); }
      check(gameplay_input,"modal restores previous input context"); }
    check(!gameplay_input,"input scope exits safely");
    const camera view{{0,0,1.6},0,0};
    const vec3 front=view.transform({0,-3,1.6});
    check(front.z==3 && front.x==0 && front.y==0,"camera coordinate transform");
    check(over(0xffff0000,0xff0000ff)==0xffff0000,"opaque sprite compositing");
    check(over(0,0xff0000ff)==0xff0000ff,"transparent sprite compositing");
    rasterizer r;
    auto panel=[&](double distance,std::uint32_t color,int id) {
        r.quad({{{-1,-distance,0},{1,-distance,0},{1,-distance,3},{-1,-distance,3}}},nullptr,color,1,false,id);
    };
    r.begin(160,120,view); panel(5,0xffff0000,1); panel(2,0xff00ff00,2);
    check(r.center_object()==2,"wall occludes farther objects");
    r.begin(160,120,view); panel(2,0xff00ff00,2); panel(5,0xffff0000,1);
    check(r.center_object()==2,"depth buffer independent of draw order");
    image transparent{2,2,{0,0,0,0}};
    r.begin(160,120,view); panel(5,0xffff0000,1);
    r.billboard({0,-2,0},1,3,&transparent,0,1,2);
    check(r.center_object()==1,"transparent billboard pixels do not occlude");
    image opaque{2,2,{0xff00ff00,0xff00ff00,0xff00ff00,0xff00ff00}};
    r.billboard({0,-2,0},1,3,&opaque,0,1,2);
    check(r.center_object()==2,"opaque billboard pixels are depth tested");
    // Look down: a floor must hide all geometry on the lower z-level.
    camera down{{.5,.5,4.5},0,-pi/2};
    auto floor=[&](double x,double y,double X,double Y,double z,int id) {
        r.quad({{{x,y,z},{X,y,z},{X,Y,z},{x,Y,z}}},nullptr,0xff777777,1,false,id);
    };
    r.begin(160,120,down); floor(-2,-2,3,3,0,10); floor(0,0,1,1,3,20);
    check(r.center_object()==20,"upper floor hides lower z-level");
    r.begin(160,120,down); floor(-2,-2,3,3,0,10);
    floor(0,0,1,.1,3,20); floor(0,.9,1,1,3,20); floor(0,.1,.1,.9,3,20); floor(.9,.1,1,.9,3,20);
    check(r.center_object()==10,"open shaft reveals lower z-level");
    // A ceiling hides geometry above even when that geometry was submitted first.
    camera up{{.5,.5,1.5},0,pi/2};
    r.begin(160,120,up); floor(0,0,1,1,6,10); floor(0,0,1,1,3,20);
    check(r.center_object()==20,"ceiling hides upper z-level");
    r.begin(160,120,view); panel(4,0xffffffff,1);
    r.box({-1,-2.5,0},{1,-2,3},nullptr,0xff000000,1,-1);
    check(r.center_object()==-1,"unknown-cell fog occludes readable targets");
    r.begin(160,120,view);
    r.quad({{{-1,-2,0},{1,1,0},{1,1,3},{-1,-2,3}}},nullptr,0xffffffff);
    for(double z:r.depth) check(!std::isnan(z),"near-plane clipping must not create NaNs");
    r.begin(160,120,view); panel(5,0xffff0000,1);
    const auto opaque_pixel=r.pixels[60*160+80];
    r.transparent_quad({{{-1,-2,0},{1,-2,0},{1,-2,3},{-1,-2,3}}},0xff90b9c9,2);
    r.finish_transparency();
    check(r.center_object()==2,"glass is selectable in front of the creature");
    check(r.pixels[60*160+80]!=opaque_pixel,"glass blends with creature behind");
    check(r.depth[60*160+80]>4.9,"glass does not replace opaque depth");
    r.begin(160,120,view); panel(1,0xff00ff00,1);
    const auto before_glass=r.pixels[60*160+80];
    r.transparent_quad({{{-1,-2,0},{1,-2,0},{1,-2,3},{-1,-2,3}}},0xff90b9c9,2);
    r.finish_transparency();
    check(r.pixels[60*160+80]==before_glass && r.center_object()==1,"opaque foreground occludes glass");
    viewport_x=20;viewport_y=40;viewport_w=320;viewport_h=240;pick_w=160;pick_h=120;
    pick_pixels.assign(160*120,-1);pick_pixels[60*160+80]=3;
    check(pick(180,160)==3,"screen coordinate maps to rendered pixel");
    check(pick(19,40)==-1 && pick(340,40)==-1,"picking rejects outside viewport");
    visible_targets={{"test",1,2,3,true}};
    check(select_target(0) && selected_target.creature && selected_target.z==3,"target selection preserves level");
    check(!select_target(-1) && !select_target(1),"invalid target rejected");
    std::cout<<checks<<" renderer/camera assertions passed\n";
}
image read_image(const std::string &file) {
    image im; std::ifstream f(file,std::ios::binary);
    f.read(reinterpret_cast<char *>(&im.w),4); f.read(reinterpret_cast<char *>(&im.h),4);
    if(im.w<=0||im.h<=0||im.w>4096||im.h>4096) throw std::runtime_error("bad fixture image");
    im.pixels.resize(im.w*im.h); f.read(reinterpret_cast<char *>(im.pixels.data()),im.w*im.h*4);
    if(!f) throw std::runtime_error("missing fixture image: "+file);
    return im;
}
void save(const rasterizer &r,const std::string &file) {
    std::ofstream f(file,std::ios::binary); f<<"P6\n"<<r.w<<" "<<r.h<<"\n255\n";
    for(auto pixel:r.pixels) { char rgb[3]={static_cast<char>(pixel>>16),static_cast<char>(pixel>>8),static_cast<char>(pixel)}; f.write(rgb,3); }
}
void fixture(const std::string &dir,const std::string &prefix) {
    const image wall=wall_material(read_image(dir+"/t_wall.bin")),ground=read_image(dir+"/t_floor.bin"),
                zombie=read_image(dir+"/mon_zombie.bin"),npc=read_image(dir+"/npc_male.bin"),
                table=read_image(dir+"/f_table.bin"),bottle=read_image(dir+"/bottle_plastic.bin");
    rasterizer r;
    const auto scene=[&](camera c) {
        r.begin(960,600,c);
        for(int x=0;x<9;++x) for(int y=0;y<9;++y) {
            r.quad({{{double(x),double(y),0},{x+1.,double(y),0},{x+1.,y+1.,0},{double(x),y+1.,0}}},&ground,0xff605848);
            if(x==0||x==8||y==0||y==8) r.box({double(x),double(y),0},{x+1.,y+1.,6},&wall,0xff797369);
            if(x<=3 && y<=4 && x>=1 && y>=1)
                r.box({double(x),double(y),2.87},{x+1.,y+1.,3},&ground,0xff605848);
        }
        r.box({1,4.85,3},{4,4.98,3.13},&wall,0xff776655);
        r.billboard({3.25,2.7,3},.9,1.8,&npc,0xff775533,1,2);
        r.billboard({4.7,2.5,0},.9,1.8,&zombie,0xff996644,1,1);
        r.box({5.3,4.2,.05},{6.2,5.1,.82},&table,0xff685640,1,3);
        r.billboard({5.75,4.6,.845},.38,.38,&bottle,0xff6688aa,1,4);
    };
    scene({{4.5,7.0,1.65},0,-.10}); save(r,prefix+"-room.ppm");
    scene({{5.2,6.8,4.65},-.15,-.35}); save(r,prefix+"-balcony.ppm");
}
int main(int argc,char **argv) {
    try { tests(); if(argc==3) fixture(argv[1],argv[2]); }
    catch(const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<"\n"; return 1; }
}
