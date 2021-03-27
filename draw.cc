#pragma once

#include "cp_lib/basic.cc"
#include "cp_lib/buffer.cc"
#include "cp_lib/vector.cc"
#include "cp_lib/math.cc"
#include <math.h>
#include <SDL2/SDL_image.h>

using namespace cp;

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

union Color {
    u32 value;
    struct { u8 b, g, r, a; };

    void init_vf(vec4f _v) {
        b = (u8)round(255 * _v.b); 
        g = (u8)round(255 * _v.g); 
        r = (u8)round(255 * _v.r); 
        a = (u8)round(255 * _v.a);
    }

    operator u32() {
        return value;
    }
    operator vec4<u8>() {
        return {a, r, g, b};
    }
};


Color to_color(vec4f _v) {
    Color c; c.init_vf(_v);
    return c;
}

vec4f to_vf(Color color) {
    return {(f32)color.a / 255.0f, (f32)color.r / 255.0f, (f32)color.g / 255.0f, (f32)color.b / 255.0f };
}

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

struct Vertex_Buffer {
    dbuff<u8> buffer;
    u32 stride;
};

struct Shader_Pack {
    void (*vertex_shader)(void*);
    void *vertex_shader_args;
    void (*fragment_shader)(void*);
    void *fragment_shader_args;

    u32 itpl_vector_len; 
};

struct Vertex_Shader_Handle {
    u8* vertex;
    void* args;

    vec2i *out_vertex_position;
    dbuff<f32> out_vertex_itpl_vector;
};

struct Fragment_Shader_Handle {
    vec2i point;
    dbuff<f32> itpl_vector;
    void* args;

    dbuff2u *out_frame_buffer;
    void (*set_pixel_color_lmd)(dbuff2u*, vec2i, Color);
};



vec2i space_to_screen_coord(vec2i v, vec2i window_size, vec2i pixels_per_unit) {
    return {window_size.x / 2 + v.x * pixels_per_unit.x, window_size.y / 2 - v.y * pixels_per_unit.y};
}
vec2i space_to_screen_coord(vec2f v, vec2i window_size, vec2i pixels_per_unit) {
    return {window_size.x / 2 + iround(v.x * pixels_per_unit.x), window_size.y / 2 - iround(v.y * pixels_per_unit.y)};
}


static void raw_set_pixel_color(dbuff2u *frame_buffer, vec2i indexes, Color color) {
    frame_buffer->get(indexes.y, indexes.x) = color;
}


static void set_pixel_color(dbuff2u *frame_buffer, vec2i indexes, Color color) {
    if (rect_is_contained<u32, u32>({{0, 0}, {frame_buffer->x_cap - 1, frame_buffer->y_cap - 1}}, indexes))
        frame_buffer->get(indexes.y, indexes.x) = color;
}

void rasterize_line(dbuff2<u32> *frame_buffer, vec2i p1, vec2i p2, Color color, 
                      void (*set_pixel_color_lmd)(dbuff2u*, vec2i, Color)) {
    
    if (p1 == p2) {
        set_pixel_color_lmd(frame_buffer, p2, color);
        return;
    }
    
    i32 delta_x = p2.x - p1.x;
    i32 delta_y = p2.y - p1.y;

    if (abs(delta_x) > abs(delta_y)) {

        f32 k = (f32)delta_y / delta_x;

        for (i32 x1 = 0; x1 != delta_x; x1 += sgn(delta_x)) {
            i32 y1 = round(k * x1);
            //frame_buffer.get(p1.y + y1, p1.x + x1) = color;
            set_pixel_color_lmd(frame_buffer, {p1.x + x1, p1.y + y1}, color);
        }

    } else {

        f32 k = (f32)delta_x / delta_y;

        for (i32 y1 = 0; y1 != delta_y; y1 += sgn(delta_y)) {
            i32 x1 = round(k * y1);
            //frame_buffer.get(p1.y + y1, p1.x + x1) = color;
            set_pixel_color_lmd(frame_buffer, {p1.x + x1, p1.y + y1}, color);
        }
    }

    //frame_buffer.get(p2.y, p2.x) = color;
    set_pixel_color_lmd(frame_buffer, p2, color);
}


void rasterize_line_dda(dbuff2<u32> *frame_buffer, vec2i p1, vec2i p2, dbuff2f itpl_buffer,
                      void (*fragment_shader_lmd)(void*), 
                      void* frag_shader_args, void (*set_pixel_color_lmd)(dbuff2u*, vec2i, Color)) {
    if (p1 == p2) {
        // fragment_shader_lmd(frame_buffer, p2, {itpl_buffer.frame_buffer, itpl_buffer.x_cap}, frag_shader_args, set_pixel_color_lmd);
        return;
    }
    
    i32 delta_x = p2.x - p1.x;
    i32 delta_y = p2.y - p1.y;

    dbufff cur_itpl_buffer = {(f32*)alloca(sizeof(f32) * itpl_buffer.x_cap), itpl_buffer.x_cap};
    dbufff cur_itpl_deltas = {(f32*)alloca(sizeof(f32) * itpl_buffer.x_cap), itpl_buffer.x_cap};

    memcpy(cur_itpl_buffer.buffer, itpl_buffer.buffer, sizeof(f32) * cap(&cur_itpl_buffer));

    if (abs(delta_x) > abs(delta_y)) {
        for (u32 i = 0; i < cap(&cur_itpl_deltas); i++) {
            cur_itpl_deltas[i] = (itpl_buffer.get(1, i) - itpl_buffer.get(0, i)) / abs(delta_x);
        }

        f32 k = (f32)delta_y / delta_x;

        for (i32 x1 = 0; x1 != delta_x; x1 += sgn(delta_x)) {
            i32 y1 = round(k * x1);
            // fragment_shader_lmd(frame_buffer, {p1.x + x1, p1.y + y1}, cur_itpl_buffer, frag_shader_args, set_pixel_color_lmd);

            for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                cur_itpl_buffer[i] += cur_itpl_deltas[i]; 
            }
        }

    } else {
        for (u32 i = 0; i < cap(&cur_itpl_deltas); i++) {
            cur_itpl_deltas[i] = (itpl_buffer.get(1, i) - itpl_buffer.get(0, i)) / abs(delta_y);
        }

        f32 k = (f32)delta_x / delta_y;

        for (i32 y1 = 0; y1 != delta_y; y1 += sgn(delta_y)) {
            i32 x1 = round(k * y1);
            // fragment_shader_lmd(frame_buffer, {p1.x + x1, p1.y + y1}, cur_itpl_buffer, frag_shader_args, set_pixel_color_lmd);
           
            for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                cur_itpl_buffer[i] += cur_itpl_deltas[i]; 
            }
        }
    }

    // fragment_shader_lmd(frame_buffer, p2, {itpl_buffer.buffer + itpl_buffer.x_cap, itpl_buffer.x_cap}, frag_shader_args, set_pixel_color_lmd);
}

void rasterize_line(dbuff2<u32> *out_frame_buffer, vec2i p0, vec2i p1,
                    f32** itpl_buffer, u32 itpl_vector_len,
                    void (*fragment_shader_lmd)(void*), 
                    void* frag_shader_args,
                    void (*set_pixel_color_lmd)(dbuff2u*, vec2i, Color)) {

    Fragment_Shader_Handle fsh_handle = {};
    fsh_handle.out_frame_buffer = out_frame_buffer;
    fsh_handle.set_pixel_color_lmd = set_pixel_color_lmd;
    fsh_handle.args = frag_shader_args;


    sbuff<f32*, 2> itpl_buffer_p = {itpl_buffer[0], itpl_buffer[1]};

    if (p0 == p1) {
        fsh_handle.point = p0;
        fsh_handle.itpl_vector = {itpl_buffer_p[0], itpl_vector_len};

        fragment_shader_lmd(&fsh_handle);
        //set_pixel_color_lmd(out_frame_buffer, p0, {0xff0000ff});
        return;
    }

    
    i32 adelta_x = abs(p1.x - p0.x);
    i32 adelta_y = abs(p1.y - p0.y);

    dbufff cur_itpl_buffer = {(f32*)alloca(sizeof(f32) * itpl_vector_len), itpl_vector_len};
    dbufff cur_itpl_deltas = {(f32*)alloca(sizeof(f32) * itpl_vector_len), itpl_vector_len};

    fsh_handle.itpl_vector = cur_itpl_buffer;

    i32 delta_x = p1.x - p0.x;
    i32 delta_y = p1.y - p0.y;
    i32 sgn_delta_x = sgn(p1.x - p0.x);
    i32 sgn_delta_y = sgn(p1.y - p0.y);


    if (adelta_x > adelta_y) {

        // init interpolation vector
        memcpy(cur_itpl_buffer.buffer, itpl_buffer_p[0], sizeof(f32) * cap(&cur_itpl_buffer));

        // init delta vector
        for (u32 i = 0; i < cap(&cur_itpl_deltas); i++) {
            cur_itpl_deltas[i] = (itpl_buffer_p[1][i] - itpl_buffer_p[0][i]) / adelta_x;
        }

        i32 e = 0;
        i32 y = p0.y;

        for (i32 x = p0.x; x != p1.x; x+=sgn_delta_x) {
            fsh_handle.point = {x, y};
            fragment_shader_lmd(&fsh_handle);
            //set_pixel_color_lmd(out_frame_buffer, {x, y}, {0xff0000ff});


            if (2 * (e + delta_y) * sgn_delta_y < adelta_x) {
                e = e + delta_y;
            } else {
                e = e + delta_y - sgn_delta_y * adelta_x;
                y += sgn_delta_y;
            }

            for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                cur_itpl_buffer[i] += cur_itpl_deltas[i]; 
            }
        }

    } else {

        // init interpolation vector
        memcpy(cur_itpl_buffer.buffer, itpl_buffer_p[0], sizeof(f32) * cap(&cur_itpl_buffer));

        // init delta vector
        for (u32 i = 0; i < cap(&cur_itpl_deltas); i++) {
            cur_itpl_deltas[i] = (itpl_buffer_p[1][i] - itpl_buffer_p[0][i]) / adelta_y;
        }

        i32 e = 0;
        i32 x = p0.x;

        for (i32 y = p0.y; y != p1.y; y+=sgn_delta_y) {

            fsh_handle.point = {x, y};
            fragment_shader_lmd(&fsh_handle);
            //set_pixel_color_lmd(out_frame_buffer, {x, y}, {0xff0000ff});


            if (2 * (e + delta_x) * sgn_delta_x < adelta_y) {
                e = e + delta_x;
            } else {
                e = e + delta_x - sgn_delta_x * adelta_y;
                x += sgn_delta_x;
            }

            for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                cur_itpl_buffer[i] += cur_itpl_deltas[i]; 
            }
        }
    }

    fsh_handle.point = p1;
    fragment_shader_lmd(&fsh_handle);
    //set_pixel_color_lmd(out_frame_buffer, p1, {0xff0000ff});

}


void draw_line(dbuff2<u32> *buffer, vec2f p1, vec2f p2, Color color, vec2i window_size, vec2i pixels_per_unit) { 
    vec2i p1_screen = space_to_screen_coord(p1, window_size, pixels_per_unit);
    vec2i p2_screen = space_to_screen_coord(p2, window_size, pixels_per_unit);

    if (!rect_is_contained<i32, i32>({ {0, 0}, window_size - vec2i::one() }, p1_screen) || 
        !rect_is_contained<i32, i32>({ {0, 0}, window_size - vec2i::one() }, p2_screen)) {
            rasterize_line(buffer, (vec2u)p1_screen, (vec2u)p2_screen, color, set_pixel_color);
        } else {
            rasterize_line(buffer, (vec2u)p1_screen, (vec2u)p2_screen, color, raw_set_pixel_color);
        }
}

void draw_lines(dbuff2u *out_frame_buffer, Vertex_Buffer *vb, dbuff<u32[2]> *ib, 
        Shader_Pack *shaders, dbuff<u8> *pr_buffer) {

    Vertex_Buffer pr_vertices = {{pr_buffer->buffer, 0}, (u32)(sizeof(vec2i) + sizeof(f32) * shaders->itpl_vector_len)};
    pr_vertices.buffer.cap = cap(&vb->buffer) / vb->stride * pr_vertices.stride;

    Vertex_Shader_Handle vsh_handle = {};
    vsh_handle.args = shaders->vertex_shader_args;
    vsh_handle.out_vertex_itpl_vector.cap = shaders->itpl_vector_len;

         
    auto pr_vertex_p = begin(&pr_vertices.buffer);
    vsh_handle.vertex = begin(&vb->buffer);
    for (; vsh_handle.vertex <= end(&vb->buffer) - vb->stride; vsh_handle.vertex += vb->stride, pr_vertex_p += pr_vertices.stride) {
        vsh_handle.out_vertex_position = (vec2i*)pr_vertex_p;
        vsh_handle.out_vertex_itpl_vector.buffer = (f32*)(pr_vertex_p + sizeof(vec2i));
        shaders->vertex_shader(&vsh_handle);
    }

    for (auto ib_p = begin(ib); ib_p != end(ib); ib_p++) {
        auto pr_vertex_p0_p = &pr_vertices.buffer[pr_vertices.stride * (*ib_p)[0]];
        auto pr_vertex_p1_p = &pr_vertices.buffer[pr_vertices.stride * (*ib_p)[1]];

        vec2i *p0 = (vec2i*)pr_vertex_p0_p;
        vec2i *p1 = (vec2i*)pr_vertex_p1_p;

        f32* p0_itpl_vec = (f32*)(pr_vertex_p0_p + sizeof(vec2i));
        f32* p1_itpl_vec = (f32*)(pr_vertex_p1_p + sizeof(vec2i));

        sbuff<f32*, 2> itpl_buffer = {p0_itpl_vec, p1_itpl_vec};

        rasterize_line(out_frame_buffer, *p0, *p1, (f32**)&itpl_buffer.buffer, shaders->itpl_vector_len,
                shaders->fragment_shader, shaders->fragment_shader_args, set_pixel_color);
        
    }
}


void write_slope_x_bound(dbuff2u *out_frame_buffer, dbuffi out_bounds, vec2i p1, vec2i p2, 
    Color color, void (*set_pixel_color_lmd)(dbuff2u*, vec2i, Color)) {

    if (p1 == p2) {
        out_bounds[0] = p1.x;
        set_pixel_color_lmd(out_frame_buffer, p1, color);
        return;
    }

    
    i32 adelta_x = abs(p2.x - p1.x);
    i32 adelta_y = abs(p2.y - p1.y);

    i32 delta_x = p2.x - p1.x;
    i32 delta_y = p2.y - p1.y;
    i32 sgn_delta_x = sgn(p2.x - p1.x);
    i32 sgn_delta_y = sgn(p2.y - p1.y);


    if (adelta_x > adelta_y) {

        i32 e = 0;
        i32 y = p1.y;

        for (i32 x = p1.x; x != p2.x; x+=sgn_delta_x) {
            i32 local_y = (y - p1.y) * sgn_delta_y;

            out_bounds[local_y] = x;
            set_pixel_color_lmd(out_frame_buffer, {x ,y}, color);

            if (2 * (e + delta_y) * sgn_delta_y < adelta_x) {
                e = e + delta_y;
            } else {
                e = e + delta_y - sgn_delta_y * adelta_x;
                y += sgn_delta_y;
            }
        }

    } else {

        i32 e = 0;
        i32 x = p1.x;

        for (i32 y = p1.y; y != p2.y; y+=sgn_delta_y) {
            i32 local_y = (y - p1.y) * sgn_delta_y;

            out_bounds[local_y] = x;
            set_pixel_color_lmd(out_frame_buffer, {x ,y}, color);

            if (2 * (e + delta_x) * sgn_delta_x < adelta_y) {
                e = e + delta_x;
            } else {
                e = e + delta_x - sgn_delta_x * adelta_y;
                x += sgn_delta_x;
            }
        }
    }

    out_bounds[adelta_y] = p2.x;
    set_pixel_color_lmd(out_frame_buffer, p2, color);
}

void rasterize_triangle_scanline(dbuff2<u32> *buffer, vec2i p0, vec2i p1, vec2i p2, Color color, 
        dbuff<u8> proc_buffer, void (*set_pixel_color_lmd)(dbuff2u*, vec2i, Color)) {
    // sort points by y coordinate p0 - top one, p1 - middle, p2 - bottom
    if (p0.y > p1.y) {swap(&p0, &p1);} else if (p0.y == p1.y && p0.x > p1.x) {swap(&p0, &p1);}
    if (p0.y > p2.y) {swap(&p0, &p2);} else if (p0.y == p2.y && p0.x > p2.x) {swap(&p0, &p2);}
    if (p1.y > p2.y) {swap(&p1, &p2);} else if (p1.y == p2.y && p1.x > p2.x) {swap(&p1, &p2);}

    // find on which side is the bend is
    bool is_right_bended = (cross(p2 - p0, p1 - p0) < 0);

    i32 long_edge_hight = p2.y - p0.y + 1;
    i32 short_top_edge_hight = p1.y - p0.y + 1;
    i32 short_bottom_edge_hight = p2.y - p1.y + 1;

    dbuffi bound_buffer = {(i32*)proc_buffer.buffer, 2 * (u32)long_edge_hight};


    if (is_right_bended) {
        write_slope_x_bound(buffer, bound_buffer, p0, p2, color, set_pixel_color_lmd); 
        write_slope_x_bound(buffer, {bound_buffer.buffer + long_edge_hight, bound_buffer.cap}, p0, p1, color, set_pixel_color_lmd); 
        write_slope_x_bound(buffer, {bound_buffer.buffer + long_edge_hight + short_top_edge_hight - 1, bound_buffer.cap}, p1, p2, color, set_pixel_color_lmd); 

    } else {
        write_slope_x_bound(buffer, bound_buffer, p0, p1, color, set_pixel_color_lmd); 
        write_slope_x_bound(buffer, {bound_buffer.buffer + short_top_edge_hight, bound_buffer.cap}, p1, p2, color, set_pixel_color_lmd); 
        write_slope_x_bound(buffer, {bound_buffer.buffer + long_edge_hight, bound_buffer.cap}, p0, p2, color, set_pixel_color_lmd); 
    }

    if (p0.y < p1.y) {
        for (i32 y = 0; y < short_top_edge_hight; y++) {
            for (i32 x = bound_buffer[y]; x <= bound_buffer[long_edge_hight + y]; x++) {
                set_pixel_color_lmd(buffer, {x, p0.y + y}, color);
            }
        }
    }

    if (p1.y < p2.y) {
        for (i32 y = short_top_edge_hight; y < long_edge_hight; y++) {
            for (i32 x = bound_buffer[y]; x <= bound_buffer[long_edge_hight + y]; x++) {
                set_pixel_color_lmd(buffer, {x, p0.y + y}, color);
            }
        }
    }
}



void write_slope_x_bound(dbuff2u *out_frame_buffer, dbuffi out_bounds, dbuff2f out_slope_itpl_buffer, 
        vec2i p1, vec2i p2, f32* itpl_buffer_from, f32* itpl_buffer_to, 
        u32 itpl_vector_len, void (*fragment_shader_lmd)(void*), 
        void* frag_shader_args, void (*set_pixel_color_lmd)(dbuff2u*, vec2i, Color)) {

    Fragment_Shader_Handle fsh_handle = {};
    fsh_handle.out_frame_buffer = out_frame_buffer;
    fsh_handle.set_pixel_color_lmd = set_pixel_color_lmd;
    fsh_handle.args = frag_shader_args;

    if (p1 == p2) {
        out_bounds[0] = p1.x;

        // write interpolation data
        for (u32 i = 0; i < itpl_vector_len; i++) {
            out_slope_itpl_buffer.get(0, i) = itpl_buffer_from[i];
        }

        fsh_handle.point = p1;
        fsh_handle.itpl_vector = { itpl_buffer_from, itpl_vector_len };

        fragment_shader_lmd(&fsh_handle);
        //set_pixel_color_lmd(out_frame_buffer, p1, {0xff0000ff});
        return;
    }

    
    i32 adelta_x = abs(p2.x - p1.x);
    i32 adelta_y = abs(p2.y - p1.y);

    dbufff cur_itpl_buffer = {(f32*)alloca(sizeof(f32) * itpl_vector_len), itpl_vector_len};
    dbufff cur_itpl_deltas = {(f32*)alloca(sizeof(f32) * itpl_vector_len), itpl_vector_len};
    fsh_handle.itpl_vector = cur_itpl_buffer;

    sbuff<f32*, 2> itpl_buffer_p = {itpl_buffer_from, &itpl_buffer_to[0]};


    i32 delta_x = p2.x - p1.x;
    i32 delta_y = p2.y - p1.y;
    i32 sgn_delta_x = sgn(p2.x - p1.x);
    i32 sgn_delta_y = sgn(p2.y - p1.y);


    if (adelta_x > adelta_y) {

        // init interpolation vector
        memcpy(cur_itpl_buffer.buffer, itpl_buffer_p[0], sizeof(f32) * cap(&cur_itpl_buffer));

        // init delta vector
        for (u32 i = 0; i < cap(&cur_itpl_deltas); i++) {
            cur_itpl_deltas[i] = (itpl_buffer_p[1][i] - itpl_buffer_p[0][i]) / adelta_x;
        }


        i32 e = 0;
        i32 y = p1.y;

        for (i32 x = p1.x; x != p2.x; x+=sgn_delta_x) {
            i32 local_y = (y - p1.y) * sgn_delta_y;

            out_bounds[local_y] = x;
            // write interpolation data
            for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                out_slope_itpl_buffer.get(local_y, i) = cur_itpl_buffer[i];
            }

            fsh_handle.point = {x, y};
            fragment_shader_lmd(&fsh_handle);
            //set_pixel_color_lmd(out_frame_buffer, {x, y}, {0xff0000ff});


            if (2 * (e + delta_y) * sgn_delta_y < adelta_x) {
                e = e + delta_y;
            } else {
                e = e + delta_y - sgn_delta_y * adelta_x;
                y += sgn_delta_y;
            }

            for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                cur_itpl_buffer[i] += cur_itpl_deltas[i]; 
            }
        }

    } else {

        // init interpolation vector
        memcpy(cur_itpl_buffer.buffer, itpl_buffer_p[0], sizeof(f32) * cap(&cur_itpl_buffer));

        // init delta vector
        for (u32 i = 0; i < cap(&cur_itpl_deltas); i++) {
            cur_itpl_deltas[i] = (itpl_buffer_p[1][i] - itpl_buffer_p[0][i]) / adelta_y;
        }

        i32 e = 0;
        i32 x = p1.x;

        for (i32 y = p1.y; y != p2.y; y+=sgn_delta_y) {
            i32 local_y = (y - p1.y) * sgn_delta_y;

            out_bounds[local_y] = x;
            // write interpolation data
            for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                out_slope_itpl_buffer.get(local_y, i) = cur_itpl_buffer[i];
            }

            fsh_handle.point = {x, y};
            fragment_shader_lmd(&fsh_handle);
            //set_pixel_color_lmd(out_frame_buffer, {x, y}, {0xff0000ff});


            if (2 * (e + delta_x) * sgn_delta_x < adelta_y) {
                e = e + delta_x;
            } else {
                e = e + delta_x - sgn_delta_x * adelta_y;
                x += sgn_delta_x;
            }

            for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                cur_itpl_buffer[i] += cur_itpl_deltas[i]; 
            }
        }
    }

    out_bounds[adelta_y] = p2.x;

    // write interpolation data
    for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
        out_slope_itpl_buffer.get(adelta_y, i) = cur_itpl_buffer[i];
    }

    fsh_handle.point = p2;
    fragment_shader_lmd(&fsh_handle);
    //set_pixel_color_lmd(out_frame_buffer, p2, {0xff0000ff});

}



void rasterize_triangle_scanline(dbuff2<u32> *out_frame_buffer, vec2i p0, vec2i p1, vec2i p2, 
                    f32** itpl_buffer, u32 itpl_vector_len,
                    void (*fragment_shader_lmd)(void*), 
                    void* frag_shader_args, dbuff<u8> *pr_buffer, 
                    void (*set_pixel_color_lmd)(dbuff2u*, vec2i, Color)) {

    sbuff<f32*, 3> itpl_buffer_p = {&itpl_buffer[0][0], &itpl_buffer[1][0], &itpl_buffer[2][0]};
    // sort points by y coordinate p0 - top one, p1 - middle, p2 - bottom
    if (p0.y > p1.y) {swap(&p0, &p1); swap(&itpl_buffer_p[0], &itpl_buffer_p[1]);} 
    else if (p0.y == p1.y && p0.x > p1.x) {swap(&p0, &p1); swap(&itpl_buffer_p[0], &itpl_buffer_p[1]);}
    if (p0.y > p2.y) {swap(&p0, &p2); swap(&itpl_buffer_p[0], &itpl_buffer_p[2]);} 
    else if (p0.y == p2.y && p0.x > p2.x) {swap(&p0, &p2); swap(&itpl_buffer_p[0], &itpl_buffer_p[2]);}
    if (p1.y > p2.y) {swap(&p1, &p2); swap(&itpl_buffer_p[1], &itpl_buffer_p[2]);} 
    else if (p1.y == p2.y && p1.x > p2.x) {swap(&p1, &p2); swap(&itpl_buffer_p[1], &itpl_buffer_p[2]);}

    // find on which side is the bend is
    bool is_right_bended = (cross(p2 - p0, p1 - p0) < 0);

    i32 long_edge_hight = p2.y - p0.y + 1;
    i32 short_top_edge_hight = p1.y - p0.y + 1;
    i32 short_bottom_edge_hight = p2.y - p1.y + 1;

    // if (long_edge_hight == 1) return;

    dbuffi bound_buffer = {(i32*)pr_buffer->buffer, 2 * (u32)long_edge_hight};
    dbuff2f slope_itpl_buffer = {(f32*)(pr_buffer->buffer + 2 * (u32)long_edge_hight * sizeof(i32)), 2 * (u32)long_edge_hight, itpl_vector_len };
    assert(( "Not enough memory", pr_buffer->cap >= bound_buffer.cap * sizeof(i32) + total_cap(&slope_itpl_buffer) * sizeof(f32) ));

    if (is_right_bended) {
        write_slope_x_bound(out_frame_buffer, bound_buffer, slope_itpl_buffer, p0, p2, 
                itpl_buffer_p[0], itpl_buffer_p[2], itpl_vector_len, 
                fragment_shader_lmd, frag_shader_args, set_pixel_color_lmd); 

        write_slope_x_bound(out_frame_buffer, {bound_buffer.buffer + long_edge_hight, bound_buffer.cap}, 
                {&slope_itpl_buffer.get(long_edge_hight, 0), slope_itpl_buffer.y_cap, slope_itpl_buffer.x_cap}, 
                p0, p1, itpl_buffer_p[0], itpl_buffer_p[1], itpl_vector_len, 
                fragment_shader_lmd, frag_shader_args, set_pixel_color_lmd); 

        // long_edge_height + short_top_edge_height - 1 cause heights have 1 pixel overlap
        write_slope_x_bound(out_frame_buffer, {bound_buffer.buffer + long_edge_hight + short_top_edge_hight - 1, bound_buffer.cap}, 
                {&slope_itpl_buffer.get(long_edge_hight + short_top_edge_hight - 1, 0), slope_itpl_buffer.y_cap, slope_itpl_buffer.x_cap}, 
                p1, p2, itpl_buffer_p[1], itpl_buffer_p[2], itpl_vector_len, 
                fragment_shader_lmd, frag_shader_args, set_pixel_color_lmd); 
    } else {
        write_slope_x_bound(out_frame_buffer, bound_buffer, slope_itpl_buffer, p0, p1, 
                itpl_buffer_p[0], itpl_buffer_p[1], itpl_vector_len, 
                fragment_shader_lmd, frag_shader_args, set_pixel_color_lmd); 


        write_slope_x_bound(out_frame_buffer, {bound_buffer.buffer + short_top_edge_hight, bound_buffer.cap}, 
                {&slope_itpl_buffer.get(short_top_edge_hight, 0), slope_itpl_buffer.y_cap, slope_itpl_buffer.x_cap}, 
                p1, p2, itpl_buffer_p[1], itpl_buffer_p[2], itpl_vector_len, 
                fragment_shader_lmd, frag_shader_args, set_pixel_color_lmd); 

        write_slope_x_bound(out_frame_buffer, {bound_buffer.buffer + long_edge_hight, bound_buffer.cap}, 
                {&slope_itpl_buffer.get(long_edge_hight, 0), slope_itpl_buffer.y_cap, slope_itpl_buffer.x_cap}, 
                p0, p2, itpl_buffer_p[0], itpl_buffer_p[2], itpl_vector_len, 
                fragment_shader_lmd, frag_shader_args, set_pixel_color_lmd); 
    }


    dbufff cur_itpl_buffer = {(f32*)alloca(sizeof(f32) * itpl_vector_len), itpl_vector_len};
    dbufff cur_itpl_deltas = {(f32*)alloca(sizeof(f32) * itpl_vector_len), itpl_vector_len};

    Fragment_Shader_Handle fsh_handle = {};
    fsh_handle.out_frame_buffer = out_frame_buffer;
    fsh_handle.set_pixel_color_lmd = set_pixel_color_lmd;
    fsh_handle.args = frag_shader_args;
    fsh_handle.itpl_vector = cur_itpl_buffer;

    if (p0.y < p1.y) {
        for (i32 y = 0; y < short_top_edge_hight; y++) {
            memcpy(cur_itpl_buffer.buffer, &slope_itpl_buffer.get(y, 0), sizeof(f32) * cap(&cur_itpl_buffer));
            f32 delta_x = bound_buffer[long_edge_hight + y] - bound_buffer[y];
            for (u32 i = 0; i < cap(&cur_itpl_deltas); i++) {
                cur_itpl_deltas[i] = (slope_itpl_buffer.get(long_edge_hight + y, i) - slope_itpl_buffer.get(y, i)) / abs(delta_x);
            }

            for (i32 x = bound_buffer[y]; x <= bound_buffer[long_edge_hight + y]; x++) {
                //set_pixel_color_lmd(buffer, {x, p0.y + y}, color);
                fsh_handle.point = {x, p0.y + y};
                fragment_shader_lmd(&fsh_handle);

                // update current interpolation vector
                for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                    cur_itpl_buffer[i] += cur_itpl_deltas[i]; 
                }
            }
        }
    }

    if (p1.y < p2.y) {
        for (i32 y = short_top_edge_hight; y < long_edge_hight; y++) {
            // write start interpolation vector to current interpolation vector
            memcpy(cur_itpl_buffer.buffer, &slope_itpl_buffer.get(y, 0), sizeof(f32) * cap(&cur_itpl_buffer));

            f32 delta_x = bound_buffer[long_edge_hight + y] - bound_buffer[y];
            // calculate delta vector
            for (u32 i = 0; i < cap(&cur_itpl_deltas); i++) {
                cur_itpl_deltas[i] = (slope_itpl_buffer.get(long_edge_hight + y, i) - slope_itpl_buffer.get(y, i)) / abs(delta_x);
            }

            for (i32 x = bound_buffer[y]; x <= bound_buffer[long_edge_hight + y]; x++) {
                //set_pixel_color_lmd(buffer, {x, p0.y + y}, color);
                fsh_handle.point = {x, p0.y + y};
                fragment_shader_lmd(&fsh_handle);

                // update current interpolation vector
                for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                    cur_itpl_buffer[i] += cur_itpl_deltas[i]; 
                }
            }

        }
    }
}

void draw_triangle(dbuff2<u32> *frame_buffer, vec2f p0, vec2f p1, vec2f p2, dbuff2f itpl_buffer,
                      void (*fragment_shader_lmd)(void*), 
                      void* frag_shader_args, dbuff<u8> *pr_buffer, vec2i window_size, vec2i pixels_per_unit) { 

    vec2i p0_screen = space_to_screen_coord(p0, window_size, pixels_per_unit);
    vec2i p1_screen = space_to_screen_coord(p1, window_size, pixels_per_unit);
    vec2i p2_screen = space_to_screen_coord(p2, window_size, pixels_per_unit);

    f32* itpl_buffer_p[3] = {&itpl_buffer.get(0, 0), &itpl_buffer.get(1, 0), &itpl_buffer.get(2, 0)};

    if (!rect_is_contained<i32, i32>({ {0, 0}, window_size - vec2i::one() }, p1_screen) || 
        !rect_is_contained<i32, i32>({ {0, 0}, window_size - vec2i::one() }, p2_screen)) {
            rasterize_triangle_scanline(frame_buffer, p0_screen, p1_screen, p2_screen, 
                (f32**)&itpl_buffer_p, itpl_buffer.x_cap, fragment_shader_lmd, frag_shader_args, pr_buffer, set_pixel_color);
        } else {
            rasterize_triangle_scanline(frame_buffer, p0_screen, p1_screen, p2_screen, 
                (f32**)&itpl_buffer_p, itpl_buffer.x_cap, fragment_shader_lmd, frag_shader_args, pr_buffer, raw_set_pixel_color);
        }
}

inline void color_itpl_frag_shader(void* args) {
    Fragment_Shader_Handle* handle = (Fragment_Shader_Handle*)args;
    handle->set_pixel_color_lmd(handle->out_frame_buffer, handle->point, 
        to_color({handle->itpl_vector[0], handle->itpl_vector[1], handle->itpl_vector[2], handle->itpl_vector[3]}));
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
void plot_axes(dbuff2<u32> *buffer, vec2<T> origin, Range<T> range_ox, Range<T> range_oy, vec2<T> scale, f64 mark_width, Color color, 
        vec2i window_size, vec2i pixels_per_unit) 
{
    draw_line(buffer, origin + vec2d(1, 0) * range_ox.first * scale.x, origin + vec2d(1, 0) * range_ox.last * scale.x, color, window_size, pixels_per_unit);
    draw_line(buffer, origin + vec2d(0, 1) * range_oy.first * scale.y, origin + vec2d(0, 1) * range_oy.last * scale.y, color, window_size, pixels_per_unit);

    for (f64 x = range_ox.first; x < range_ox.last; x += range_ox.step) {
        vec2d c_pos = origin + vec2d(1, 0) * x * scale.x;
        draw_line(buffer, c_pos + vec2d(0, -1) * (mark_width / 2), c_pos + vec2d(0, 1) * (mark_width / 2), color, window_size, pixels_per_unit);
    }
}





constexpr u32 c_drawing_thread_count = 4;

void clear_buffer(dbuff2u *out_frame_buffer, Color clear_color) {
    for (auto p = begin(out_frame_buffer); p != end(out_frame_buffer); p++) {
        *p = clear_color.value;
    }
}


void draw_triangles(dbuff2u *out_frame_buffer, desbuff *vb, dbuff<u32[3]> *ib, 
        Shader_Pack *shaders, dbuff<u8> *pr_buffer) {

    desbuff pr_vertices = {{pr_buffer->buffer, 0}, (u32)(sizeof(vec2i) + sizeof(f32) * shaders->itpl_vector_len)};
    pr_vertices.buffer.cap = cap(&vb->buffer) / vb->stride * pr_vertices.stride;
    dbuff<u8> tr_rast_pr_buffer = { pr_buffer->buffer + cap(&pr_vertices.buffer), cap(pr_buffer) - cap(&pr_vertices.buffer) };

    Vertex_Shader_Handle vsh_handle = {};
    vsh_handle.args = shaders->vertex_shader_args;
    vsh_handle.out_vertex_itpl_vector.cap = shaders->itpl_vector_len;

         
    auto pr_vertex_p = begin(&pr_vertices.buffer);
    vsh_handle.vertex = begin(&vb->buffer);
    for (; vsh_handle.vertex <= end(&vb->buffer) - vb->stride; vsh_handle.vertex += vb->stride, pr_vertex_p += pr_vertices.stride) {
        vsh_handle.out_vertex_position = (vec2i*)pr_vertex_p;
        vsh_handle.out_vertex_itpl_vector.buffer = (f32*)(pr_vertex_p + sizeof(vec2i));
        shaders->vertex_shader(&vsh_handle);
    }

    for (auto ib_p = begin(ib); ib_p != end(ib); ib_p++) {
        auto pr_vertex_p0_p = &pr_vertices.buffer[pr_vertices.stride * (*ib_p)[0]];
        auto pr_vertex_p1_p = &pr_vertices.buffer[pr_vertices.stride * (*ib_p)[1]];
        auto pr_vertex_p2_p = &pr_vertices.buffer[pr_vertices.stride * (*ib_p)[2]];

        vec2i *p0 = (vec2i*)pr_vertex_p0_p;
        vec2i *p1 = (vec2i*)pr_vertex_p1_p;
        vec2i *p2 = (vec2i*)pr_vertex_p2_p;

        f32* p0_itpl_vec = (f32*)(pr_vertex_p0_p + sizeof(vec2i));
        f32* p1_itpl_vec = (f32*)(pr_vertex_p1_p + sizeof(vec2i));
        f32* p2_itpl_vec = (f32*)(pr_vertex_p2_p + sizeof(vec2i));

        sbuff<f32*, 3> itpl_buffer = {p0_itpl_vec, p1_itpl_vec, p2_itpl_vec};

        rasterize_triangle_scanline(out_frame_buffer, *p0, *p1, *p2, (f32**)&itpl_buffer.buffer, shaders->itpl_vector_len,
                shaders->fragment_shader, shaders->fragment_shader_args, &tr_rast_pr_buffer, set_pixel_color);
        
    }
}
