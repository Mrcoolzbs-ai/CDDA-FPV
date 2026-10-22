// Standalone SDL extraction test; helper copied verbatim from patched cata_tiles.cpp.
#include "../../src/first_person.h"
#include <SDL.h>
#include <memory>
#include <iostream>
#include <stdexcept>
struct renderer_deleter {void operator()(SDL_Renderer *p)const{SDL_DestroyRenderer(p);}};
struct texture_deleter {void operator()(SDL_Texture *p)const{SDL_DestroyTexture(p);}};
using SDL_Renderer_Ptr=std::unique_ptr<SDL_Renderer,renderer_deleter>;
using SDL_Texture_Ptr=std::unique_ptr<SDL_Texture,texture_deleter>;
class texture {
    SDL_Texture *ptr; SDL_Rect rect;
    public: texture(SDL_Texture *p,SDL_Rect r):ptr(p),rect(r){}
    SDL_Texture *native()const{return ptr;}
    SDL_Rect source_rect()const{return rect;}
};
first_person::image fpv_read_sprite( const SDL_Renderer_Ptr &renderer, const texture &source )
{
    first_person::image result;
    const SDL_Rect src = source.source_rect();
    if( src.w <= 0 || src.h <= 0 || !SDL_RenderTargetSupported( renderer.get() ) ) {
        return result;
    }
    SDL_Texture_Ptr target( SDL_CreateTexture( renderer.get(), SDL_PIXELFORMAT_ARGB8888,
                           SDL_TEXTUREACCESS_TARGET, src.w, src.h ) );
    if( !target ) { return result; }
    SDL_Texture *old_target = SDL_GetRenderTarget( renderer.get() );
    SDL_Rect viewport, clip;
    SDL_RenderGetViewport( renderer.get(), &viewport );
    SDL_RenderGetClipRect( renderer.get(), &clip );
    const SDL_bool clipped = SDL_RenderIsClipEnabled( renderer.get() );
    float old_scale_x, old_scale_y;
    SDL_RenderGetScale(renderer.get(),&old_scale_x,&old_scale_y);
    Uint8 r, g, b, a, tr, tg, tb, ta;
    SDL_GetRenderDrawColor( renderer.get(), &r, &g, &b, &a );
    SDL_GetTextureColorMod( source.native(), &tr, &tg, &tb );
    SDL_GetTextureAlphaMod( source.native(), &ta );
    SDL_BlendMode blend;
    SDL_GetTextureBlendMode( source.native(), &blend );
    const bool switched = SDL_SetRenderTarget( renderer.get(), target.get() ) == 0;
    if( switched ) {
        SDL_RenderSetScale(renderer.get(),1.0f,1.0f);
        SDL_RenderSetViewport( renderer.get(), nullptr );
        SDL_RenderSetClipRect( renderer.get(), nullptr );
        SDL_SetRenderDrawColor( renderer.get(), 0, 0, 0, 0 );
        SDL_RenderClear( renderer.get() );
        SDL_SetTextureColorMod( source.native(), 255, 255, 255 );
        SDL_SetTextureAlphaMod( source.native(), 255 );
        SDL_SetTextureBlendMode( source.native(), SDL_BLENDMODE_NONE );
        result.w = src.w; result.h = src.h;
        result.pixels.resize( src.w * src.h );
        if( SDL_RenderCopy( renderer.get(), source.native(), &src, nullptr ) != 0 ||
            SDL_RenderReadPixels( renderer.get(), nullptr, SDL_PIXELFORMAT_ARGB8888,
                                  result.pixels.data(), src.w * 4 ) != 0 ) {
            result = {};
        }
        SDL_SetTextureColorMod( source.native(), tr, tg, tb );
        SDL_SetTextureAlphaMod( source.native(), ta );
        SDL_SetTextureBlendMode( source.native(), blend );
    }
    SDL_SetRenderTarget( renderer.get(), old_target );
    SDL_RenderSetScale(renderer.get(),old_scale_x,old_scale_y);
    SDL_RenderSetViewport( renderer.get(), &viewport );
    SDL_RenderSetClipRect( renderer.get(), clipped ? &clip : nullptr );
    SDL_SetRenderDrawColor( renderer.get(), r, g, b, a );
    return result;
}
int main() {
    SDL_Init(0);
    SDL_Surface *canvas=SDL_CreateRGBSurfaceWithFormat(0,96,96,32,SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer_Ptr renderer(SDL_CreateSoftwareRenderer(canvas));
    SDL_Texture_Ptr display(SDL_CreateTexture(renderer.get(),SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,96,96));
    if(!renderer||!display) return 2;
    SDL_SetRenderTarget(renderer.get(),display.get());
    SDL_Surface *src=SDL_CreateRGBSurfaceWithFormat(0,2,2,32,SDL_PIXELFORMAT_ARGB8888);
    const std::uint32_t original[4]={0xffff0000,0xff00ff00,0xff0000ff,0x80405060};
    for(int y=0;y<2;++y)for(int x=0;x<2;++x)
      reinterpret_cast<std::uint32_t *>(static_cast<char *>(src->pixels)+src->pitch*y)[x]=original[y*2+x];
    SDL_Texture_Ptr atlas(SDL_CreateTextureFromSurface(renderer.get(),src));
    SDL_FreeSurface(src);
    SDL_SetTextureColorMod(atlas.get(),101,102,103);SDL_SetTextureAlphaMod(atlas.get(),80);
    SDL_SetTextureBlendMode(atlas.get(),SDL_BLENDMODE_BLEND);
    SDL_Rect viewport{4,5,70,75},clip{3,4,20,21};
    SDL_RenderSetScale(renderer.get(),1.25f,.75f);
    SDL_RenderSetViewport(renderer.get(),&viewport);SDL_RenderSetClipRect(renderer.get(),&clip);
    const auto image=fpv_read_sprite(renderer,texture(atlas.get(),SDL_Rect{0,0,2,2}));
    int checks=0;
    const auto check=[&](bool pass){++checks;if(!pass)throw std::runtime_error("SDL state or color mismatch");};
    check(image.valid());
    for(int i=0;i<4;++i)check(image.pixels[i]==original[i]);
    check(SDL_GetRenderTarget(renderer.get())==display.get());
    SDL_Rect got;SDL_RenderGetViewport(renderer.get(),&got);
    check(got.x==viewport.x&&got.y==viewport.y&&got.w==viewport.w&&got.h==viewport.h);
    SDL_RenderGetClipRect(renderer.get(),&got);
    check(SDL_RenderIsClipEnabled(renderer.get())&&got.x==clip.x&&got.y==clip.y&&got.w==clip.w&&got.h==clip.h);
    float x,y;SDL_RenderGetScale(renderer.get(),&x,&y);check(x==1.25f&&y==.75f);
    Uint8 r,g,b,a;SDL_GetTextureColorMod(atlas.get(),&r,&g,&b);SDL_GetTextureAlphaMod(atlas.get(),&a);
    check(r==101&&g==102&&b==103&&a==80);
    SDL_BlendMode blend;SDL_GetTextureBlendMode(atlas.get(),&blend);check(blend==SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer.get(),nullptr);atlas.reset();display.reset();renderer.reset();
    SDL_FreeSurface(canvas);SDL_Quit();
    std::cout<<checks<<" SDL sprite-capture assertions passed\n";
}
