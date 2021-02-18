#pragma once

#include "../cp_lib/basic.cc"
#include "../cp_lib/buffer.cc"
#include "../cp_lib/vector.cc"
#include "../cp_lib/math.cc"
#include <math.h>
#include <libpng16/png.h>

using namespace cp;

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

union Color {
    u32 value;
    vec4<u8> v;

    operator u32() {
        return value;
    }
    operator vec4<u8>() {
        return v;
    }
};

template <typename T>
struct Rect {
    vec2<T> lb;
    vec2<T> rt;
};

template <typename T1, typename T2>
bool rect_is_contained(Rect<T1> rect, vec2<T2> p) {
    return ( rect.lb.x <= p.x && p.x <= rect.rt.x && rect.lb.y <= p.y && p.y <= rect.rt.y);
}


template <typename T>
i32 iround(T value) {
    return value + 0.5;
}

vec2i space_to_screen_coord(vec2i v, vec2i window_size, vec2i pixels_per_unit) {
    return {window_size.x / 2 + v.x * pixels_per_unit.x, window_size.y / 2 - v.y * pixels_per_unit.y};
}
vec2i space_to_screen_coord(vec2f v, vec2i window_size, vec2i pixels_per_unit) {
    return {window_size.x / 2 + iround(v.x * pixels_per_unit.x), window_size.y / 2 - iround(v.y * pixels_per_unit.y)};
}


static void raw_set_pixel_color(dbuff2u buffer, vec2i indexes, Color color) {
    buffer.get(indexes.y, indexes.x) = color;
}


static void set_pixel_color(dbuff2u buffer, vec2i indexes, Color color) {
    if (rect_is_contained<u32, u32>({{0, 0}, {buffer.x_cap - 1, buffer.y_cap - 1}}, indexes))
        buffer.get(indexes.y, indexes.x) = color;
}

void draw_line_screen_(dbuff2<u32> buffer, vec2i p1, vec2i p2, Color color, 
                      void (*set_pixel_color_lmd)(dbuff2u, vec2i, Color)) {
    
    if (p1 == p2) {
        set_pixel_color_lmd(buffer, p2, color);
        return;
    }
    
    i32 delta_x = p2.x - p1.x;
    i32 delta_y = p2.y - p1.y;

    if (abs(delta_x) > abs(delta_y)) {

        f32 k = (f32)delta_y / delta_x;

        for (i32 x1 = 0; x1 != delta_x; x1 += sgn(delta_x)) {
            i32 y1 = round(k * x1);
            //buffer.get(p1.y + y1, p1.x + x1) = color;
            set_pixel_color_lmd(buffer, {p1.x + x1, p1.y + y1}, color);
        }

    } else {

        f32 k = (f32)delta_x / delta_y;

        for (i32 y1 = 0; y1 != delta_y; y1 += sgn(delta_y)) {
            i32 x1 = round(k * y1);
            //buffer.get(p1.y + y1, p1.x + x1) = color;
            set_pixel_color_lmd(buffer, {p1.x + x1, p1.y + y1}, color);
        }
    }

    //buffer.get(p2.y, p2.x) = color;
    set_pixel_color_lmd(buffer, p2, color);
}


void rasterize_line(dbuff2<u32> buffer, vec2i p1, vec2i p2, Color color, 
                      void (*set_pixel_color_lmd)(dbuff2u, vec2i, Color)) {
    if (p1 == p2) {
        set_pixel_color_lmd(buffer, p2, color);
        return;
    }
    
    i32 delta_x = p2.x - p1.x;
    i32 delta_y = p2.y - p1.y;

    f32 delta_t = 1.0f / max(abs(delta_x), abs(delta_y));
    
    for (f32 t = 0; t < 1; t += delta_t) {
        i32 x1 = round(delta_x * t);
        i32 y1 = round(delta_y * t);
        set_pixel_color_lmd(buffer, {p1.x + x1, p1.y + y1}, color);
    }

    set_pixel_color_lmd(buffer, p2, color);
}

template <typename T>
void draw_line(dbuff2<u32> buffer, vec2<T> p1, vec2<T> p2, Color color, vec2i window_size, vec2i pixels_per_unit) { 
    vec2i p1_screen = space_to_screen_coord(p1, window_size, pixels_per_unit);
    vec2i p2_screen = space_to_screen_coord(p2, window_size, pixels_per_unit);

    if (!rect_is_contained<i32, i32>({ {0, 0}, window_size - vec2i::one() }, p1_screen) || 
        !rect_is_contained<i32, i32>({ {0, 0}, window_size - vec2i::one() }, p2_screen)) {
            rasterize_line(buffer, (vec2u)p1_screen, (vec2u)p2_screen, color, set_pixel_color);
        } else {
            rasterize_line(buffer, (vec2u)p1_screen, (vec2u)p2_screen, color, raw_set_pixel_color);
        }
}

void rasterize_triangle_scanline_(dbuff2<u32> buffer, vec2i p0, vec2i p1, vec2i p2, Color color, 
                      void (*set_pixel_color_lmd)(dbuff2u, vec2i, Color)) {
    // sort points by y coordinate p0 - top one, p1 - middle, p2 - bottom
    if (p0.y > p1.y) {swap(&p0, &p1);} else if (p0.y == p1.y && p0.x > p1.x) {swap(&p0, &p1);}
    if (p0.y > p2.y) {swap(&p0, &p2);} else if (p0.y == p2.y && p0.x > p2.x) {swap(&p0, &p2);}
    if (p1.y > p2.y) {swap(&p1, &p2);} else if (p1.y == p2.y && p1.x > p2.x) {swap(&p1, &p2);}

    // find on which side is the bend is
    bool is_right_bended = (cross(p2 - p0, p1 - p0) < 0);

    // find the intersection point of long edge and horizontal line passing through p1
    vec2i p3 = { iround((float)(p2.x - p0.x) / (p2.y - p0.y) * (p1.y - p0.y)) + p0.x, p1.y};

    vec2i dir01 = p1 - p0; 
    vec2i dir03 = p3 - p0; 

    f32 delta_t = 1.0f / max(max(abs(dir01.x), abs(dir03.x)), max(abs(dir01.y), abs(dir03.y)));

    i32 prev_y0 = INT_MIN;
    for (f32 t = 0; t < 1; t += delta_t) {
        i32 x0, y0, x1;
        if (is_right_bended) {
            x0 = round(dir03.x * t);
            y0 = round(dir03.y * t);
            x1 = round(dir01.x * t);
        } else {
            x0 = round(dir01.x * t);
            y0 = round(dir01.y * t);
            x1 = round(dir03.x * t);
        }
        if (prev_y0 != y0) {
            for (i32 x = x0; x <= x1; x++) {
                set_pixel_color_lmd(buffer, {p0.x + x, p0.y + y0}, color);
            }
            prev_y0 = y0;
        }
    }

    if (is_right_bended) {
        for (i32 x = p3.x; x <= p1.x; x++) {
            set_pixel_color_lmd(buffer, {x, p1.y}, color);
        }
    } else {
        for (i32 x = p1.x; x <= p3.x; x++) {
            set_pixel_color_lmd(buffer, {x, p1.y}, color);
        }
    }


    vec2i dir21 = p1 - p2; 
    vec2i dir23 = p3 - p2; 

    delta_t = 1.0f / max(max(abs(dir21.x), abs(dir23.x)), max(abs(dir21.y), abs(dir23.y)));

    prev_y0 = INT_MIN;
    for (f32 t = 0; t < 1; t += delta_t) {
        i32 x0, y0, x1;
        if (is_right_bended) {
            x0 = round(dir23.x * t);
            y0 = round(dir23.y * t);
            x1 = round(dir21.x * t);
        } else {
            x0 = round(dir21.x * t);
            y0 = round(dir21.y * t);
            x1 = round(dir23.x * t);
        }
        if (prev_y0 != y0) {
            for (i32 x = x0; x <= x1; x++) {
                set_pixel_color_lmd(buffer, {p2.x + x, p2.y + y0}, color);
            }
            prev_y0 = y0;
        }
    }

    set_pixel_color_lmd(buffer, p2, color);
}

void write_slope_x_bound(dbuff<i32> out_bounds, vec2i p1, vec2i p2) {
    if (p1 == p2) {
        out_bounds[0] = p1.x;
        return;
    }
    
    i32 delta_x = p2.x - p1.x;
    i32 delta_y = p2.y - p1.y;

    if (abs(delta_x) > abs(delta_y)) {

        f32 k = (f32)delta_y / delta_x;

        i32 prev_y1 = INT_MIN;
        for (i32 x1 = 0; x1 != delta_x; x1 += sgn(delta_x)) {
            i32 y1 = round(k * x1);
            if (y1 != prev_y1) {
                out_bounds[y1] = p1.x + x1;
                prev_y1 = y1;
            }
        }

    } else {

        f32 k = (f32)delta_x / delta_y;

        for (i32 y1 = 0; y1 != delta_y; y1 += sgn(delta_y)) {
            i32 x1 = round(k * y1);
            out_bounds[y1] = p1.x + x1;
        }
    }

    out_bounds[delta_y] = p2.x;
}


void rasterize_triangle_scanline(dbuff2<u32> buffer, vec2i p0, vec2i p1, vec2i p2, Color color, 
        dbuffi proc_buffer, void (*set_pixel_color_lmd)(dbuff2u, vec2i, Color)) {
    // sort points by y coordinate p0 - top one, p1 - middle, p2 - bottom
    if (p0.y > p1.y) {swap(&p0, &p1);} else if (p0.y == p1.y && p0.x > p1.x) {swap(&p0, &p1);}
    if (p0.y > p2.y) {swap(&p0, &p2);} else if (p0.y == p2.y && p0.x > p2.x) {swap(&p0, &p2);}
    if (p1.y > p2.y) {swap(&p1, &p2);} else if (p1.y == p2.y && p1.x > p2.x) {swap(&p1, &p2);}

    // find on which side is the bend is
    bool is_right_bended = (cross(p2 - p0, p1 - p0) < 0);

    i32 long_edge_hight = p2.y - p0.y;
    i32 short_top_edge_hight = p1.y - p0.y;
    i32 short_bottom_edge_hight = p2.y - p1.y;


    if (is_right_bended) {
        write_slope_x_bound(proc_buffer, p0, p2); 
        write_slope_x_bound({proc_buffer.buffer + long_edge_hight, proc_buffer.cap}, p0, p1); 
        write_slope_x_bound({proc_buffer.buffer + long_edge_hight + short_top_edge_hight, proc_buffer.cap}, p1, p2); 

    } else {
        write_slope_x_bound(proc_buffer, p0, p1); 
        write_slope_x_bound({proc_buffer.buffer + short_top_edge_hight, proc_buffer.cap}, p1, p2); 
        write_slope_x_bound({proc_buffer.buffer + long_edge_hight, proc_buffer.cap}, p0, p2); 
    }

    if (p0.y < p1.y) {
        for (i32 y = 0; y < short_top_edge_hight; y++) {
            for (i32 x = proc_buffer[y]; x <= proc_buffer[long_edge_hight + y]; x++) {
                set_pixel_color_lmd(buffer, {x, p0.y + y}, color);
            }
        }
    }

    if (p1.y < p2.y) {
        for (i32 y = short_top_edge_hight; y < long_edge_hight; y++) {
            for (i32 x = proc_buffer[y]; x <= proc_buffer[long_edge_hight + y]; x++) {
                set_pixel_color_lmd(buffer, {x, p0.y + y}, color);
            }
        }
    }
}



template <typename T>
void plot(dbuff2<u32> buffer, dbuff<T> domain, dbuff<T> range, 
        vec2<T> start_p, vec2<T> scale, Color color, 
        vec2i window_size, vec2i pixels_per_unit) 
{
    Rect<i32> window_rect = {{0, 0}, window_size};
    vec2<T> pre_p = {domain[0] * scale.x, range[0] * scale.y};
    for (u32 i = 1u; i < domain.capacity; i++) {
        vec2<T> p = {domain[i] * scale.x, range[i] * scale.y};
        draw_line(buffer, start_p + pre_p, start_p + p, color, window_size, pixels_per_unit);
        pre_p = p;
    }
}

template <typename t_domain_type, typename  t_range_type>
void plot(dbuff<t_range_type> domain, t_domain_type(*func)(t_range_type)) {

}

template <typename T>
void plot_axes(dbuff2<u32> buffer, vec2<T> origin, Range<T> range_ox, Range<T> range_oy, vec2<T> scale, f64 mark_width, Color color, 
        vec2i window_size, vec2i pixels_per_unit) 
{
    draw_line(buffer, origin + vec2d(1, 0) * range_ox.first * scale.x, origin + vec2d(1, 0) * range_ox.last * scale.x, color, window_size, pixels_per_unit);
    draw_line(buffer, origin + vec2d(0, 1) * range_oy.first * scale.y, origin + vec2d(0, 1) * range_oy.last * scale.y, color, window_size, pixels_per_unit);

    for (f64 x = range_ox.first; x < range_ox.last; x += range_ox.step) {
        vec2d c_pos = origin + vec2d(1, 0) * x * scale.x;
        draw_line(buffer, c_pos + vec2d(0, -1) * (mark_width / 2), c_pos + vec2d(0, 1) * (mark_width / 2), color, window_size, pixels_per_unit);
    }
}

// plot_axes void (buffer dbuffd) {

// }

// plot_axes :: (buffer:: dbuffd) -> void {

// }


