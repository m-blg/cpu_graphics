
#include <SDL2/SDL.h> 
#include <SDL2/SDL_timer.h> 
#include "../cp_lib/basic.cc"
#include "../cp_lib/vector.cc"

using namespace cp;

void set_pixel(SDL_Surface* surface, vec2i pixel_pos) {
    u32* pixels = (u32*)surface->pixels;
    u32* c_pixel = pixels + pixel_pos.y * surface->pitch + pixel_pos.x;
    *c_pixel = SDL_MapRGB(surface->format, 0xff, 0xff, 0xff);
}

void foo(SDL_Surface *surface) {
    for (i32 i = 0; i < 100; i++) {
        for (i32 j = 0; j < 200; j++) {
            set_pixel(surface, {j, i});
        }
    }
}


  
int main() 
{ 
  
    // retutns zero on success else non-zero 
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { 
        printf("error initializing SDL: %s\n", SDL_GetError()); 
    } 
    SDL_Window* win = SDL_CreateWindow("GAME", // creates a window 
                                       SDL_WINDOWPOS_CENTERED, 
                                       SDL_WINDOWPOS_CENTERED, 
                                       1000, 1000, 0); 
  
    // triggers the program that controls 
    // your graphics hardware and sets flags 
    Uint32 render_flags = SDL_RENDERER_ACCELERATED; 
  
    // creates a renderer to render our images 
    //SDL_Renderer* rend = SDL_CreateRenderer(win, -1, render_flags); 
  
    // creates a surface to load an image into the main memory 
    SDL_Surface* win_surface = SDL_GetWindowSurface(win); 
  
    bool close;

    while (!close) { 
        SDL_Event event; 
  
        // Events mangement 
        while (SDL_PollEvent(&event)) { 
            switch (event.type) { 
  
            case SDL_QUIT: 
                // handling of close button 
                close = 1; 
                break; 
  
            case SDL_KEYDOWN: 
                // keyboard API for key pressed 
                switch (event.key.keysym.scancode) { 
                case SDL_SCANCODE_W: 
                    foo(win_surface);
                    SDL_UpdateWindowSurface(win);
                    break;
                }
            } 
        } 
  
        //// clears the screen 
        //SDL_RenderClear(rend); 
  
        //// triggers the double buffers 
        //// for multiple rendering 
        //SDL_RenderPresent(rend); 
  
        //// calculates to 60 fps 
        SDL_Delay(1000 / 60); 
    } 
  
    // destroy renderer 
    //SDL_DestroyRenderer(rend); 
  
    // destroy window 
    SDL_DestroyWindow(win); 
    return 0; 
}
