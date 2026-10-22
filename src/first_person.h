#pragma once
#ifndef CATA_SRC_FIRST_PERSON_H
#define CATA_SRC_FIRST_PERSON_H

// Renderer-only experimental camera. Does not change CDDA sight or save formats.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace first_person
{
constexpr double pi = 3.14159265358979323846;
constexpr double story_height = 3.0;
inline bool enabled = false;
inline bool gameplay_input = false;
inline bool mouse_locked = false;
inline int facing = 0; // movement heading, N NE E SE S SW W NW
inline double yaw = 0.0;
inline double pitch = 0.0;
inline std::string pending_action;
inline constexpr int dx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
inline constexpr int dy[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
inline int heading( double angle ) {
    const int n = static_cast<int>( std::lround( angle / ( pi / 4 ) ) );
    return ( n % 8 + 8 ) % 8;
}
inline void turn( int steps ) {
    facing = ( facing + steps + 8 ) % 8;
    yaw += steps * pi / 4;
}
inline void mouse_motion( int x, int y, double sensitivity, bool link_heading ) {
    yaw = std::remainder( yaw + x * sensitivity, 2 * pi );
    pitch = std::clamp( pitch - y * sensitivity, -1.35, 1.35 );
    if( link_heading ) {
        facing = heading( yaw );
    }
}
struct input_scope {
    bool previous;
    explicit input_scope( bool active ) : previous( gameplay_input ) { gameplay_input = active; }
    ~input_scope() { gameplay_input = previous; }
};
struct hit_button {
    int x, y, w, h;
    std::string action;
};
inline std::vector<hit_button> buttons;
struct vec3 {
    double x = 0, y = 0, z = 0;
    vec3 operator+( const vec3 &b ) const { return { x + b.x, y + b.y, z + b.z }; }
    vec3 operator-( const vec3 &b ) const { return { x - b.x, y - b.y, z - b.z }; }
    vec3 operator*( double s ) const { return { x * s, y * s, z * s }; }
};
struct image {
    int w = 0, h = 0;
    std::vector<std::uint32_t> pixels; // packed 0xAARRGGBB
    bool valid() const { return w > 0 && h > 0 && pixels.size() == static_cast<size_t>( w * h ); }
    std::uint32_t sample( double u, double v ) const {
        const int x = std::clamp( static_cast<int>( u * w ), 0, w - 1 );
        const int y = std::clamp( static_cast<int>( v * h ), 0, h - 1 );
        return pixels[y * w + x];
    }
};
inline std::uint32_t over( std::uint32_t fg, std::uint32_t bg ) {
    const unsigned a = fg >> 24;
    const unsigned ba = bg >> 24;
    const unsigned oa = a + ba * ( 255 - a ) / 255;
    if( oa == 0 ) { return 0; }
    std::uint32_t result = oa << 24;
    for( int shift : { 0, 8, 16 } ) {
        const unsigned c = ( ( ( fg >> shift ) & 255 ) * a +
                             ( ( bg >> shift ) & 255 ) * ba * ( 255 - a ) / 255 ) / oa;
        result |= c << shift;
    }
    return result;
}
// Top-down wall icons are not ready-made 3D materials. Reuse a side strip,
// avoiding the black centerline/studs of common connected wall tiles.
inline image wall_material( const image &source ) {
    image result;
    if( !source.valid() ) { return result; }
    result.w=32; result.h=96; result.pixels.resize(result.w*result.h);
    for( int y=0; y<result.h; ++y ) {
        for( int x=0; x<result.w; ++x ) {
            result.pixels[y*result.w+x]=over(source.sample(.12+.26*(x+.5)/result.w,
                .25+.5*((y%32)+.5)/32),0xff77736a);
        }
    }
    return result;
}
struct vertex {
    vec3 p;
    double u = 0, v = 0;
};
struct camera {
    vec3 eye;
    double yaw = 0, pitch = 0, hfov = 75 * pi / 180;
    vec3 transform( vec3 p ) const {
        p = p - eye;
        const double f = p.x * std::sin( yaw ) - p.y * std::cos( yaw );
        return { p.x * std::cos( yaw ) + p.y * std::sin( yaw ),
                 p.z * std::cos( pitch ) - f * std::sin( pitch ),
                 f * std::cos( pitch ) + p.z * std::sin( pitch ) };
    }
};
struct target {
    std::string label;
    int x = 0, y = 0, z = 0;
    bool creature = false;
};
// CPU picking uses the same depth-tested pixels as the rendered scene.
inline std::vector<target> visible_targets;
inline std::vector<int> pick_pixels;
inline int pick_w=0, pick_h=0, viewport_x=0, viewport_y=0, viewport_w=0, viewport_h=0;
inline int gaze_id=-1;
inline target selected_target;
inline bool has_selected_target=false, inventory_open=false;
inline std::string ui_command;
inline std::vector<std::string> inventory_labels, cell_labels;
inline int selected_row=-1, selected_side=0, inventory_page=0, cell_page=0;
inline bool cell_reachable=false;
inline std::string inventory_notice;
inline int pick(int x,int y) {
    if(viewport_w<=0 || viewport_h<=0 || x<viewport_x || y<viewport_y ||
       x>=viewport_x+viewport_w || y>=viewport_y+viewport_h) { return -1; }
    const int px=(x-viewport_x)*pick_w/viewport_w, py=(y-viewport_y)*pick_h/viewport_h;
    const size_t i=static_cast<size_t>(py*pick_w+px);
    return i<pick_pixels.size() ? pick_pixels[i] : -1;
}
inline bool select_target(int id) {
    if(id<0 || id>=static_cast<int>(visible_targets.size())) { return false; }
    selected_target=visible_targets[id]; has_selected_target=true; return true;
}
class rasterizer
{
    public:
        int w = 0, h = 0;
        std::vector<std::uint32_t> pixels;
        std::vector<double> depth;
        std::vector<int> object;
        camera view;
        struct glass_face { std::array<vec3,4> p; std::uint32_t color; int id; double distance; };
        std::vector<glass_face> glass;
        std::vector<double> pick_depth;
        double alpha=1;
        void transparent_quad(const std::array<vec3,4> &p,std::uint32_t color,int id) {
            double distance=0; for(const auto &v:p) { distance+=view.transform(v).z; }
            glass.push_back({p,color,id,distance/4});
        }
        void finish_transparency() {
            pick_depth=depth;
            std::stable_sort(glass.begin(),glass.end(),[](const glass_face &a,const glass_face &b) {
                return a.distance>b.distance;
            });
            for(const auto &f:glass) { alpha=.24; quad(f.p,nullptr,f.color,1,false,f.id); }
            alpha=1; glass.clear();
        }
        void begin( int width, int height, camera c ) {
            w = width; h = height; view = c; glass.clear(); alpha=1;
            pixels.assign( w * h, 0xff080b10 );
            depth.assign( w * h, std::numeric_limits<double>::infinity() );
            object.assign( w * h, -1 );
        }
        void quad( const std::array<vec3, 4> &p, const image *tex,
                   std::uint32_t fallback, double light = 1, bool cutout = false, int id = -1 ) {
            const vertex a{ view.transform( p[0] ), 0, 1 };
            const vertex b{ view.transform( p[1] ), 1, 1 };
            const vertex c{ view.transform( p[2] ), 1, 0 };
            const vertex d{ view.transform( p[3] ), 0, 0 };
            triangle( { a, b, c }, tex, fallback, light, cutout, id );
            triangle( { a, c, d }, tex, fallback, light, cutout, id );
        }
        void box( vec3 low, vec3 high, const image *tex, std::uint32_t color,
                  double light = 1, int id = -1 ) {
            const double x = low.x, y = low.y, z = low.z;
            const double X = high.x, Y = high.y, Z = high.z;
            quad( {{{x,y,z},{X,y,z},{X,y,Z},{x,y,Z}}}, tex, color, light * .85, false, id );
            quad( {{{X,Y,z},{x,Y,z},{x,Y,Z},{X,Y,Z}}}, tex, color, light * .85, false, id );
            quad( {{{x,Y,z},{x,y,z},{x,y,Z},{x,Y,Z}}}, tex, color, light * .7, false, id );
            quad( {{{X,y,z},{X,Y,z},{X,Y,Z},{X,y,Z}}}, tex, color, light * .7, false, id );
            quad( {{{x,y,Z},{X,y,Z},{X,Y,Z},{x,Y,Z}}}, tex, color, light, false, id );
            quad( {{{x,Y,z},{X,Y,z},{X,y,z},{x,y,z}}}, tex, color, light * .55, false, id );
        }
        void billboard( vec3 feet, double width, double height, const image *tex,
                        std::uint32_t fallback, double light = 1, int id = -1 ) {
            const vec3 right{ std::cos( view.yaw ) * width / 2, std::sin( view.yaw ) * width / 2, 0 };
            const vec3 up{ 0, 0, height };
            quad( {{ feet - right, feet + right, feet + right + up, feet - right + up }},
                  tex, fallback, light, true, id );
        }
        int center_object() const { return object.empty() ? -1 : object[(h/2)*w+w/2]; }
    private:
        void triangle( std::array<vertex, 3> tri, const image *tex, std::uint32_t fallback,
                       double light, bool cutout, int id ) {
            // Clip to the near_plane plane before perspective divide (including surfaces around the eye).
            constexpr double near_plane = .06;
            if( tri[0].p.z >= near_plane && tri[1].p.z >= near_plane && tri[2].p.z >= near_plane ) {
                raster(tri,tex,fallback,light,cutout,id); return;
            }
            if( tri[0].p.z < near_plane && tri[1].p.z < near_plane && tri[2].p.z < near_plane ) { return; }
            std::vector<vertex> input( tri.begin(), tri.end() ), clipped;
            for( size_t i = 0; i < input.size(); ++i ) {
                const vertex &a = input[i], &b = input[(i+1)%input.size()];
                const bool ain = a.p.z >= near_plane, bin = b.p.z >= near_plane;
                if( ain ) { clipped.push_back( a ); }
                if( ain != bin ) {
                    const double t = ( near_plane - a.p.z ) / ( b.p.z - a.p.z );
                    clipped.push_back( { a.p + (b.p-a.p)*t, a.u+(b.u-a.u)*t, a.v+(b.v-a.v)*t } );
                }
            }
            for( size_t i = 1; i + 1 < clipped.size(); ++i ) {
                raster( {{clipped[0], clipped[i], clipped[i+1]}}, tex, fallback, light, cutout, id );
            }
        }
        void raster( const std::array<vertex, 3> &tri, const image *tex, std::uint32_t fallback,
                     double light, bool cutout, int id ) {
            const double focal = w / (2 * std::tan(view.hfov/2));
            std::array<double, 3> x{}, y{}, invz{};
            for( int i = 0; i < 3; ++i ) {
                invz[i] = 1 / tri[i].p.z;
                x[i] = w*.5 + tri[i].p.x*focal*invz[i];
                y[i] = h*.5 - tri[i].p.y*focal*invz[i];
            }
            const double area = (x[1]-x[0])*(y[2]-y[0]) - (y[1]-y[0])*(x[2]-x[0]);
            if( std::abs(area) < 1e-9 ) { return; }
            const double minx = std::max(0.0, std::floor(std::min({x[0],x[1],x[2]})));
            const double maxx = std::min(w-1.0, std::ceil(std::max({x[0],x[1],x[2]})));
            const double miny = std::max(0.0, std::floor(std::min({y[0],y[1],y[2]})));
            const double maxy = std::min(h-1.0, std::ceil(std::max({y[0],y[1],y[2]})));
            for( int py = static_cast<int>(miny); py <= maxy; ++py ) {
                for( int px = static_cast<int>(minx); px <= maxx; ++px ) {
                    const double X=px+.5, Y=py+.5;
                    const double a = ((x[1]-X)*(y[2]-Y)-(y[1]-Y)*(x[2]-X))/area;
                    const double b = ((x[2]-X)*(y[0]-Y)-(y[2]-Y)*(x[0]-X))/area;
                    const double c = 1-a-b;
                    if( a < -1e-9 || b < -1e-9 || c < -1e-9 ) { continue; }
                    const double iz = a*invz[0]+b*invz[1]+c*invz[2];
                    const double z = 1/iz;
                    const size_t index = static_cast<size_t>(py*w+px);
                    if( z >= depth[index] ) { continue; }
                    std::uint32_t color = fallback;
                    if( tex && tex->valid() ) {
                        const double u = (a*tri[0].u*invz[0]+b*tri[1].u*invz[1]+c*tri[2].u*invz[2])/iz;
                        const double v = (a*tri[0].v*invz[0]+b*tri[1].v*invz[1]+c*tri[2].v*invz[2])/iz;
                        color = tex->sample(u,v);
                        if( cutout && (color>>24) < 100 ) { continue; }
                        if( !cutout ) { color = over(color, fallback); }
                    }
                    const double shade = std::clamp(light/(1+z*.035),0.06,1.0);
                    std::uint32_t shaded = 0xff000000;
                    for( int shift : {0,8,16} ) {
                        shaded |= static_cast<std::uint32_t>(((color>>shift)&255)*shade)<<shift;
                    }
                    if(alpha<1) {
                        pixels[index]=over((static_cast<std::uint32_t>(alpha*255)<<24)|(shaded&0xffffff),pixels[index]);
                        if(z<pick_depth[index]) { pick_depth[index]=z; object[index]=id; }
                    } else { pixels[index]=shaded; depth[index]=z; object[index]=id; }
                }
            }
        }
};
} // namespace first_person
#endif
