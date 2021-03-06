#pragma once

#include "cp_lib/basic.cc"
#include "cp_lib/array.cc"
#include "cp_lib/vector.cc"
#include "cp_lib/bitfield.cc"
#include "cp_lib/memory.cc"

using namespace cp;

bool is_running = true;

vec2i window_max_size;
vec2i window_size;

dbuff2u frame_buffer;

// input
#define KEY_COUNT 500
#define MOUSE_BUTTON_COUNT 5

namespace Input {
    dbitfld keys_down;
    dbitfld keys_hold;
    dbitfld keys_up;

    dbitfld mouse_button_down;
    dbitfld mouse_button_hold;
    dbitfld mouse_button_up;
    vec2i mouse_pos;
};


void input_init() {
    Input::keys_down.init_bits(KEY_COUNT);
    Input::keys_hold.init_bits(KEY_COUNT);
    Input::keys_up.init_bits(KEY_COUNT);

    Input::mouse_button_down.init_bits(MOUSE_BUTTON_COUNT);
    Input::mouse_button_hold.init_bits(MOUSE_BUTTON_COUNT);
    Input::mouse_button_up.init_bits(MOUSE_BUTTON_COUNT);
}


void input_shut() {
    Input::keys_down.shut();
    Input::keys_hold.shut();
    Input::keys_up.shut();

    Input::mouse_button_down.shut();
    Input::mouse_button_hold.shut();
    Input::mouse_button_up.shut();
}



void game_init();
void game_shut();

void game_update();
