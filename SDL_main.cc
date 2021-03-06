

#include <SDL2/SDL.h> 
#include <SDL2/SDL_timer.h> 
#include "cp_lib/basic.cc"
#include "cp_lib/vector.cc"

#include "game.cc"

using namespace cp;

int main()
{
    game_init();

    SDL_Window* window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_size.x, window_size.y, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, frame_buffer.x_cap, frame_buffer.y_cap);


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

        game_update();

        SDL_UpdateTexture(texture, nullptr, frame_buffer.buffer, frame_buffer.x_cap * sizeof(u32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        SDL_Delay(1000/30);
    }

    game_shut();
}
