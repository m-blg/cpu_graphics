

#include <SDL2/SDL.h> 
#include <SDL2/SDL_timer.h> 
#include "../../cp_lib/basic.cc"
#include "../../cp_lib/vector.cc"

using namespace cp;

void set_pixel(dbuff2u buffer, vec2i pixel_pos) {
    buffer.get(pixel_pos.y, pixel_pos.x) = 0xFF000000;
}

void foo(dbuff2u buffer) {
    for (i32 i = 0; i < 100; i++) {
        for (i32 j = 0; j < 200; j++) {
            set_pixel(buffer, {j, i});
        }
    }
}

int main()
{
    vec2i window_size = {1280, 720};
    // Create a screen.
    SDL_Window* window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_size.x, window_size.y, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_size.x, window_size.y);

    // Create a mesh of triangles covering the entire screen

    for(bool interrupted=false; !interrupted; )
    {

        dbuff2u pixels;
        pixels.init(window_size.y, window_size.x);
        u32 color = 0xFF000000, blank = 0xFFFF0000;
        for (u32 *it = begin(&pixels); it != end(&pixels); it++) *it = blank;
        foo(pixels);

        u32 p[window_size.x * window_size.y];
        for (auto& it : p) it = 0xff005500;

        for (u32 i = 0; i < 100; i++) {
            p[i] = 0xffff0000;
        }



        // Process events.
        SDL_Event ev;
        while(SDL_PollEvent(&ev))
            switch(ev.type)
            {
                case SDL_QUIT: interrupted = true; break;
            }

        SDL_UpdateTexture(texture, nullptr, pixels.buffer, pixels.x_cap * sizeof(u32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        SDL_Delay(1000/60);
    }
}
