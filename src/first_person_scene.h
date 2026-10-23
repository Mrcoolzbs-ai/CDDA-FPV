#pragma once
#ifndef CATA_SRC_FIRST_PERSON_SCENE_H
#define CATA_SRC_FIRST_PERSON_SCENE_H
#include "first_person.h"
namespace first_person {
// Repeating physical material, never a top-down object icon.
inline const image &wood_texture() {
    static const image tex=[] {image im;im.w=32;im.h=32;im.pixels.resize(1024);
        for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
            const unsigned grain=(x*17+y*3+(x*y)%7)%23;
            im.pixels[y*32+x]=0xff000000|((111+grain)<<16)|((85+grain)<<8)|(55+grain);
        }return im;}();return tex;
}
inline image vegetation_sprite(bool bush,unsigned seed) {
    image im;im.w=48;im.h=64;im.pixels.assign(im.w*im.h,0);
    for(int y=0;y<im.h;++y)for(int x=0;x<im.w;++x) {
        const double u=(x+.5)/im.w*2-1,v=(y+.5)/im.h;
        const unsigned noise=(x*57u+y*137u+seed+(x*y*31u))%101;
        const bool leaf=bush ? (u*u+(v-.48)*(v-.48)*3.3<.85 && noise>17):
            (v>.12+((x*13u+seed)%37)/100. && std::abs(std::sin(x*.9+v*3))<.12+v*.36);
        if(leaf)im.pixels[y*im.w+x]=0xff000000|((44+noise/3)<<16)|((68+noise/2)<<8)|(25+noise/5);
    }return im;
}
inline const image &water_texture() {
    static const image tex=[] {image im;im.w=32;im.h=32;im.pixels.resize(1024);
        for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
            const unsigned n=static_cast<unsigned>((std::sin(x*.7+y*.33)+1)*15);
            im.pixels[y*32+x]=0xff000000|((52+n)<<16)|((93+n)<<8)|(104+n);
        }return im;}();return tex;
}
inline const image &curtain_texture() {
    static const image tex=[] {image im;im.w=32;im.h=32;im.pixels.resize(1024);
        for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
            const unsigned fold=static_cast<unsigned>(18+17*std::cos(x*pi/4));
            im.pixels[y*32+x]=0xff000000|((143+fold)<<16)|((131+fold)<<8)|(102+fold);
        }return im;}();return tex;
}
// Procedural meshes use distinct horizontal, vertical and front materials.
// Existing top-down tiles are not stretched over all six faces.
struct model {
    rasterizer &r; vec3 origin; double angle=0, light=1; int id=-1;
    vec3 at(vec3 p) const {
        return {origin.x+p.x*std::cos(angle)-p.y*std::sin(angle),
                origin.y+p.x*std::sin(angle)+p.y*std::cos(angle),origin.z+p.z};
    }
    void box(vec3 a,vec3 b,std::uint32_t color,const image *top=nullptr,
             const image *side=nullptr,const image *front=nullptr) {
        const double x=a.x,y=a.y,z=a.z,X=b.x,Y=b.y,Z=b.z;
        const auto q=[&](std::array<vec3,4> p,const image *tex,double shade) {
            for(auto &v:p) { v=at(v); }
            r.quad(p,tex,color,light*shade,false,id);
        };
        q({{{x,y,z},{X,y,z},{X,y,Z},{x,y,Z}}},front?front:side,.9);
        q({{{X,Y,z},{x,Y,z},{x,Y,Z},{X,Y,Z}}},side,.8);
        q({{{x,Y,z},{x,y,z},{x,y,Z},{x,Y,Z}}},side,.7);
        q({{{X,y,z},{X,Y,z},{X,Y,Z},{X,y,Z}}},side,.75);
        q({{{x,y,Z},{X,y,Z},{X,Y,Z},{x,Y,Z}}},top,1);
        q({{{x,Y,z},{X,Y,z},{X,y,z},{x,y,z}}},nullptr,.6);
    }
    void wheel(vec3 c,double radius=.33,double breadth=.20) {
        for(int i=0;i<12;++i) {
            const double a=2*pi*i/12,b=2*pi*(i+1)/12;
            const vec3 A{c.x+radius*std::cos(a),c.y-breadth/2,c.z+radius*std::sin(a)},
                       B{c.x+radius*std::cos(b),c.y-breadth/2,c.z+radius*std::sin(b)},
                       C{B.x,c.y+breadth/2,B.z},D{A.x,c.y+breadth/2,A.z};
            r.quad({{at(A),at(B),at(C),at(D)}},nullptr,0xff25292b,light*(.6+.3*std::abs(std::sin(a))),false,id);
            for(double y:{c.y-breadth/2,c.y+breadth/2}) {
                r.quad({{at({c.x,y,c.z}),at({A.x,y,A.z}),at({B.x,y,B.z}),at({c.x,y,c.z})}},nullptr,0xff303438,light,false,id);
            }
        }
        box({c.x-.11,c.y-breadth/2-.006,c.z-.11},{c.x+.11,c.y+breadth/2+.006,c.z+.11},0xff9d9e98);
    }
    // Returns the support height for dropped items.
    double furniture(const std::string &name,const image *top,bool /* surface */) {
        const auto has=[&](const char *s){return name.find(s)!=std::string::npos;};
        const auto legs=[&](double z){for(double x:{-.34,.28})for(double y:{-.34,.28})
            box({x,y,.02},{x+.06,y+.06,z},0xff65503c);};
        if(has("firering") || has("fire_ring") || has("fireplace") || has("brazier")) {
            box({-.3,-.3,.01},{.3,.3,.025},0xff34302c);
            for(int i=0;i<10;++i) {
                const double a=i*2*pi/10,cx=std::cos(a)*.35,cy=std::sin(a)*.35;
                box({cx-.09,cy-.09,.02},{cx+.09,cy+.09,.17},0xff77736b);
            }
            return .025;
        }
        if(has("canvas") || has("groundsheet") || has("skin_wall") || has("skin_door") || has("skin_floor") || has("tent")) {
            const bool door=has("door"),wall=has("wall")||door;
            if(wall) {
                if(door && has("_o")) {
                    box({-.5,-.035,0},{-.39,.035,1.9},0xff72745a);
                    box({.39,-.035,0},{.5,.035,1.9},0xff72745a);
                    box({-.5,-.035,1.75},{.5,.035,1.9},0xff72745a);
                } else box({-.5,-.035,0},{.5,.035,1.9},0xff8d9074);
            } else {
                box({-.5,-.5,.006},{.5,.5,.018},0xff686d54);
                // Cloth canopy, not a cube bearing a tiny tent icon.
                r.quad({{at({-.55,-.55,1.85}),at({0,-.55,2.25}),at({0,.55,2.25}),at({-.55,.55,1.85})}},nullptr,0xff92957a,light*.85,false,id);
                r.quad({{at({0,-.55,2.25}),at({.55,-.55,1.85}),at({.55,.55,1.85}),at({0,.55,2.25})}},nullptr,0xffa0a286,light,false,id);
            }
            return .018;
        }
        if(has("bush") || has("shrub") || has("grass") || has("fern")) {
            r.billboard(origin,.85,.8,top,0xff476235,light,id);return 0;
        }
        if(has("locker")||has("fridge")||has("cabinet")||has("dresser")) {
            const double h=has("dresser")?.95:1.9;
            box({-.43,-.35,.04},{.43,.38,h},0xff727b7d);
            box({-.39,-.37,.12},{.39,-.35,h-.08},0xff9a9b91);
            box({.25,-.40,h*.45},{.28,-.36,h*.65},0xff24292a);
            return h;
        }
        if(has("shelf")||has("bookcase")||has("rack")) {
            for(double x:{-.43,.39})box({x,-.4,.02},{x+.04,.4,1.9},0xff655442);
            for(double z:{.15,.65,1.15,1.7})box({-.43,-.4,z},{.43,.4,z+.05},0xff81705b,&wood_texture());
            return 1.75;
        }
        if(has("bed") || has("cot") || has("mattress")) {
            const bool mattress=has("mattress"),bunk=has("bunk");
            const auto layer=[&](double z) {
                if(!mattress) {
                    for(double x:{-.39,.33})for(double y:{-.43,.37})
                        box({x,y,.02},{x+.06,y+.06,z+.12},0xff67513c);
                    box({-.43,-.49,z+.08},{.43,.49,z+.18},0xff80654b,&wood_texture());
                    box({-.43,.44,z+.12},{.43,.49,z+.72},0xff70553e,&wood_texture());
                }
                // Cloth mattress, folded blanket and a separate pillow at the head.
                box({-.40,-.46,z+.18},{.40,.43,z+.34},0xffd4cdb8);
                box({-.405,-.46,z+.34},{.405,.15,z+.365},0xff637d8c);
                box({-.405,.09,z+.36},{.405,.15,z+.40},0xff889ba6);
                box({-.31,.20,z+.34},{.31,.40,z+.43},0xffe8e0c9);
            };
            layer(mattress?-.16:.1);
            if(bunk) {layer(1.35);for(double x:{-.44,.40})box({x,-.48,.02},{x+.04,.48,2.15},0xff626a6c);}
            return mattress?.2:.47;
        }
        if(has("sofa") || has("couch")) {
            legs(.25);box({-.48,-.42,.20},{.48,.44,.42},0xff4f5954);
            for(double x:{-.41,.02})box({x,-.36,.42},{x+.39,.32,.57},0xff82918a);
            box({-.47,.31,.48},{.47,.46,1.02},0xff6e8077);
            for(double x:{-.48,.36})box({x,-.4,.38},{x+.12,.42,.76},0xff65786f);
            return .57;
        }
        if(has("chair")||has("bench")||has("stool")) {
            legs(.47);box({-.39,-.39,.47},{.39,.39,.54},0xff8e7251,&wood_texture());
            if(!has("stool"))box({-.39,.32,.53},{.39,.4,1.05},0xff786044);
            return .54;
        }
        if(has("table")||has("desk")||has("counter")) {
            legs(.78);box({-.45,-.45,.78},{.45,.45,.87},0xff8c7656,&wood_texture());
            return .87;
        }
        // Preserve the original detailed sprite for unidentified props rather than invent a cube.
        r.billboard(origin,.9,1.25,top,0xff756854,light,id);return 0;
    }
};
inline double sky_hash(int x,int y) {
    std::uint32_t h=static_cast<std::uint32_t>(x)*374761393u+static_cast<std::uint32_t>(y)*668265263u;
    h=(h^(h>>13))*1274126177u;return ((h^(h>>16))&65535)/65535.0;
}
inline double sky_noise(double x,double y) {
    const int X=static_cast<int>(std::floor(x)),Y=static_cast<int>(std::floor(y));
    double a=x-X,b=y-Y;a=a*a*(3-2*a);b=b*b*(3-2*b);
    return (sky_hash(X,Y)*(1-a)+sky_hash(X+1,Y)*a)*(1-b)+
           (sky_hash(X,Y+1)*(1-a)+sky_hash(X+1,Y+1)*a)*b;
}
inline void draw_sky(rasterizer &r,double sun_phase,double daylight,double clouds,double clock) {
    const double focal=r.w/(2*std::tan(r.view.hfov/2));
    const double az=pi/2+sun_phase*pi,el=std::sin(sun_phase*pi)*1.05;
    const vec3 sun{std::sin(az)*std::cos(el),-std::cos(az)*std::cos(el),std::sin(el)};
    for(int y=0;y<r.h;++y)for(int x=0;x<r.w;++x) {
        const double rx=(x+.5-r.w*.5)/focal,ry=(r.h*.5-y-.5)/focal;
        const double f=std::cos(r.view.pitch)-ry*std::sin(r.view.pitch);
        vec3 d{rx*std::cos(r.view.yaw)+f*std::sin(r.view.yaw),
               rx*std::sin(r.view.yaw)-f*std::cos(r.view.yaw),
               std::sin(r.view.pitch)+ry*std::cos(r.view.pitch)};
        const double norm=std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);d=d*(1/norm);
        const double horizon=std::clamp(d.z,0.0,1.0);
        double R=7+daylight*(130-85*horizon),G=10+daylight*(159-74*horizon),B=19+daylight*(173-29*horizon);
        if(d.z<0) {R=12+daylight*71;G=15+daylight*76;B=19+daylight*78;}
        if(d.z>0) {
            const double nx=d.x/(d.z+.12)*1.4+clock*.004,ny=d.y/(d.z+.12)*1.4;
            const double n=sky_noise(nx,ny)*.7+sky_noise(nx*2.2,ny*2.2)*.3;
            const double cover=std::clamp((n-(.77-clouds*.4))*4,0.0,.96);
            const double sun_dot=d.x*sun.x+d.y*sun.y+d.z*sun.z;
            if(sun_phase>=0 && sun_phase<=1 && sun_dot>.9993) {R=255;G=238;B=168;}
            const double grey=21+daylight*(197-clouds*48);
            R=R*(1-cover)+grey*cover;G=G*(1-cover)+(grey+3)*cover;B=B*(1-cover)+(grey+7)*cover;
            if(daylight<.15 && cover<.1 && sky_hash(static_cast<int>(d.x*800),static_cast<int>(d.y*800))>.9995) {R=150;G=160;B=185;}
        }
        r.pixels[y*r.w+x]=0xff000000|(static_cast<unsigned>(std::clamp(R,0.,255.))<<16)|
            (static_cast<unsigned>(std::clamp(G,0.,255.))<<8)|static_cast<unsigned>(std::clamp(B,0.,255.));
    }
}
inline image field_sprite(bool fire,double clock) {
    image im;im.w=32;im.h=48;im.pixels.resize(32*48);
    for(int y=0;y<im.h;++y)for(int x=0;x<im.w;++x) {
        const double u=(x+.5)/im.w*2-1,v=(y+.5)/im.h;
        const double n=sky_noise(x*.3+clock*(fire?7:.4),y*.25-clock*(fire?9:.6));
        if(fire) {
            const double width=.12+v*.65+.15*std::sin(v*17+clock*8);
            const double a=std::clamp((width-std::abs(u)+n*.25)*5,0.,1.)*v;
            const unsigned R=255,G=static_cast<unsigned>(80+v*155),B=static_cast<unsigned>(v*v*75);
            im.pixels[y*32+x]=(static_cast<unsigned>(a*255)<<24)|(R<<16)|(G<<8)|B;
        } else {
            const double a=std::clamp(1-u*u-(v-.5)*(v-.5)*3,0.,1.)*(.4+n*.6);
            im.pixels[y*32+x]=(static_cast<unsigned>(a*180)<<24)|0x8f9395;
        }
    }
    return im;
}
} // namespace first_person
#endif
