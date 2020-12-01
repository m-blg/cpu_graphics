
#include "../cp_lib/basic.cc"
#include "../cp_lib/array.cc"
#include "../cp_lib/vector.cc"
#include <math.h>

using namespace cp;

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

void draw_line(dbuff2<u32> buffer, vec2i p1, vec2i p2, u32 color) {
    i32 delta_x = p2.x - p1.x;
    i32 delta_y = p2.y - p1.y;

    if (abs(delta_x) < abs(delta_y)) {
        i32 div = delta_y / abs(delta_x);
        i32 rest = delta_y % delta_x;

        for (u32 x = p1.x, y = p1.y; x != p2.x; x += sgn(delta_x)) {
            i32 dy = div;
            if (rest > 0 && x % 2 == 1) {
                dy += sgn(dy);
                rest--;
            }
            for (u32 j = 0; j < abs(dy); j++) {
                buffer.get(y, x) = color;
                y += sgn(dy);
            }
        }

        buffer.get(p2.y, p2.x) = color;
    } else {
        i32 div = delta_x / abs(delta_y);
        i32 rest = delta_x % delta_y;

        for (u32 y = p1.y, x = p1.y; y != p2.y; y += sgn(delta_y)) {
            i32 dx = div;
            if (rest > 0 && y % 2 == 1) {
                dx += sgn(dx);
                rest--;
            }
            for (u32 j = 0; j < abs(dx); j++) {
                buffer.get(y, x) = color;
                x += sgn(dx);
            }
        }

        buffer.get(p2.y, p2.x) = color;
    }
}
