

#include <SDL2/SDL.h> 
#include <SDL2/SDL_timer.h> 
#include <SDL2/SDL_ttf.h> 
#include "cp_lib/basic.cc"
#include "cp_lib/vector.cc"
#include <time.h>

#include "game.cc"

using namespace cp;

int main()
{
    game_init();

    SDL_Window* window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_size.x, window_size.y, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, frame_buffer.x_cap, frame_buffer.y_cap);

    TTF_Font* ui_font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 24);
    SDL_Color ui_font_color = {200, 200, 200}; 

    u32 pre_clock = SDL_GetTicks();
    while (is_running) {
        for (u32 i = 0; i < Input::keys_down.cap; i++) {
            Input::keys_down.buffer[i] = 0;
            Input::keys_up.buffer[i] = 0;
        }

        // Process events.
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            switch(event.type)
            {
                case SDL_QUIT: is_running = false; break;
                case SDL_KEYDOWN: {
                    if (event.key.keysym.sym < KEY_COUNT) {
                        set_bit_high(Input::keys_down, event.key.keysym.sym);
                        set_bit_high(Input::keys_hold, event.key.keysym.sym);
                    }
                } break;
                case SDL_KEYUP: {
                    if (event.key.keysym.sym < KEY_COUNT) {
                        set_bit_high(Input::keys_up, event.key.keysym.sym);
                        set_bit_low(Input::keys_hold, event.key.keysym.sym);
                    }
                } break;
                // case SDL_MOUSEBUTTONDOWN: {
                //     if (event.key.keysym.sym < MOUSE_BUTTON_COUNT) {
                //         set_bit_high(Input::mouse_button_down, event.key.keysym.sym);
                //         set_bit_low(Input::mouse_button_hold, event.key.keysym.sym);
                //     }
                // } break;
                // case SDL_MOUSEBUTTONUP: {
                //     if (event.key.keysym.sym < MOUSE_BUTTON_COUNT) {
                //         set_bit_high(Input::mouse_button_up, event.key.keysym.sym);
                //         set_bit_low(Input::mouse_button_hold, event.key.keysym.sym);
                //     }
                // } break;
                case SDL_MOUSEMOTION: {
                    Input::mouse_pos = { event.motion.x, event.motion.y };
                } break;
            }
        }

        // wall time
        u32 new_clock = SDL_GetTicks();
        f32 dt = (f32)(new_clock - pre_clock) / 1000;
        pre_clock = new_clock;


        game_update();

        SDL_UpdateTexture(texture, nullptr, frame_buffer.buffer, frame_buffer.x_cap * sizeof(u32));


        char fps_str_buff[20];
        printf("%f\n", 1/dt);

        // SDL_Surface* fps_text_sur = TTF_RenderText_Solid(ui_font, fps_str_buff, ui_font_color);

        // SDL_Texture * fps_text_texture = SDL_CreateTextureFromSurface(renderer, fps_text_sur);
        // SDL_Rect fps_text_sur_blit_rect = { 0, 0, fps_text_sur->w, fps_text_sur->h };
        // SDL_LockSurface(fps_text_sur);
        // SDL_UpdateTexture(texture, &fps_text_sur_blit_rect, fps_text_sur->pixels, fps_text_sur->pitch);
        // SDL_UnlockSurface(fps_text_sur);


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        // SDL_RenderCopy(renderer, fps_text_texture, nullptr, &fps_text_sur_blit_rect);
        SDL_RenderPresent(renderer);

        SDL_Delay(1000/60);
    }

    game_shut();
}
