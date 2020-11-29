

#include <SDL2/SDL.h> 
#include <SDL2/SDL_timer.h> 
#include "../cp_lib/basic.cc"
#include "../cp_lib/vector.cc"

using namespace cp;

void set_pixel(u32* pixels, vec2i pixel_pos, u32 W) {
    u32* c_pixel = pixels + pixel_pos.y * W + pixel_pos.x;
    *c_pixel = 0x555555;
}

void foo(u32* pixels, u32 W) {
    for (i32 i = 0; i < 100; i++) {
        for (i32 j = 0; j < 200; j++) {
            set_pixel(pixels, {j, i}, W);
        }
    }
}

int main()
{
    const int W = 424, H = 240;
    // Create a screen.
    SDL_Window* window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W*4,H*4, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, W,H);

    // Create a mesh of triangles covering the entire screen

    for(bool interrupted=false; !interrupted; )
    {

        Uint32 pixels[W*H]={};
        unsigned color = 0x3B0103A5, blank = 0xFFFFFF, duplicate = 0xFFAA55;
        for(auto& p: pixels) p = blank;
        foo(pixels, W);

        // Process events.
        SDL_Event ev;
        while(SDL_PollEvent(&ev))
            switch(ev.type)
            {
                case SDL_QUIT: interrupted = true; break;
                case SDL_KEYDOWN: foo(pixels, W); break;
            }

        SDL_UpdateTexture(texture, nullptr, pixels, 4*W);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        SDL_Delay(1000/60);
    }
}
