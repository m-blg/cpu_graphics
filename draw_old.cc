#pragma once

#include "draw.cc"

//slow
void rasterize_line_parametric(dbuff2<u32> buffer, vec2i p1, vec2i p2, Color color, 
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

//slow
void rasterize_triangle_scanline_parametric(dbuff2<u32> buffer, vec2i p0, vec2i p1, vec2i p2, Color color, 
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

void write_slope_x_bound_dda(dbuff<i32> out_bounds, vec2i p1, vec2i p2) {
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

void write_slope_x_left_bound(dbuff2u gr_buffer, dbuffi out_bounds, vec2i p1, vec2i p2, 
    Color color, void (*set_pixel_color_lmd)(dbuff2u, vec2i, Color)) {

    if (p1 == p2) {
        out_bounds[0] = p1.x;
        set_pixel_color_lmd(gr_buffer, p1, color);
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

        // write left pixel in the pixel row
        // if goint to the right save first pixel in the row
        if (delta_x > 0) {
            i32 prev_y = INT_MIN;
            for (i32 x = p1.x; x != p2.x; x+=sgn_delta_x) {
                i32 local_y = (y - p1.y) * sgn_delta_y;

                if (y != prev_y) {
                    out_bounds[local_y] = x;
                    prev_y = y;
                }
                set_pixel_color_lmd(gr_buffer, {x ,y}, color);

                if (2 * (e + delta_y) * sgn_delta_y < adelta_x) {
                    e = e + delta_y;
                } else {
                    e = e + delta_y - sgn_delta_y * adelta_x;
                    y += sgn_delta_y;
                }
            }
        } else { // if goint to the left save last pixel in the row
            for (i32 x = p1.x; x != p2.x; x+=sgn_delta_x) {
                i32 local_y = (y - p1.y) * sgn_delta_y;

                out_bounds[local_y] = x;
                set_pixel_color_lmd(gr_buffer, {x ,y}, color);

                if (2 * (e + delta_y) * sgn_delta_y < adelta_x) {
                    e = e + delta_y;
                } else {
                    e = e + delta_y - sgn_delta_y * adelta_x;
                    y += sgn_delta_y;
                }
            }
        }

    } else {

        i32 e = 0;
        i32 x = p1.x;

        for (i32 y = p1.y; y != p2.y; y+=sgn_delta_y) {
            i32 local_y = (y - p1.y) * sgn_delta_y;

            out_bounds[local_y] = x;
            set_pixel_color_lmd(gr_buffer, {x ,y}, color);

            if (2 * (e + delta_x) * sgn_delta_x < adelta_y) {
                e = e + delta_x;
            } else {
                e = e + delta_x - sgn_delta_x * adelta_y;
                x += sgn_delta_x;
            }

        }
    }

    out_bounds[adelta_y] = p2.x;
    set_pixel_color_lmd(gr_buffer, p2, color);

}



void write_slope_x_right_bound(dbuff2u gr_buffer, dbuffi out_bounds, vec2i p1, vec2i p2, 
    Color color, void (*set_pixel_color_lmd)(dbuff2u, vec2i, Color)) {

    if (p1 == p2) {
        out_bounds[0] = p1.x;
        set_pixel_color_lmd(gr_buffer, p1, color);
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

        // write right pixel in the pixel row
        // if goint to the right save last pixel in the row
        if (delta_x < 0) {
            i32 prev_y = INT_MIN;
            for (i32 x = p1.x; x != p2.x; x+=sgn_delta_x) {
                i32 local_y = (y - p1.y) * sgn_delta_y;

                if (y != prev_y) {
                    out_bounds[local_y] = x;
                    prev_y = y;
                }
                set_pixel_color_lmd(gr_buffer, {x ,y}, color);


                if (2 * (e + delta_y) * sgn_delta_y < adelta_x) {
                    e = e + delta_y;
                } else {
                    e = e + delta_y - sgn_delta_y * adelta_x;
                    y += sgn_delta_y;
                }
            }
        } else { // if goint to the left save first pixel in the row
            for (i32 x = p1.x; x != p2.x; x+=sgn_delta_x) {
                i32 local_y = (y - p1.y) * sgn_delta_y;

                out_bounds[local_y] = x;
                set_pixel_color_lmd(gr_buffer, {x ,y}, color);

                if (2 * (e + delta_y) * sgn_delta_y < adelta_x) {
                    e = e + delta_y;
                } else {
                    e = e + delta_y - sgn_delta_y * adelta_x;
                    y += sgn_delta_y;
                }
            }
        }

    } else {

        i32 e = 0;
        i32 x = p1.x;

        for (i32 y = p1.y; y != p2.y; y+=sgn_delta_y) {
            i32 local_y = (y - p1.y) * sgn_delta_y;

            out_bounds[local_y] = x;
            set_pixel_color_lmd(gr_buffer, {x ,y}, color);

            if (2 * (e + delta_x) * sgn_delta_x < adelta_y) {
                e = e + delta_x;
            } else {
                e = e + delta_x - sgn_delta_x * adelta_y;
                x += sgn_delta_x;
            }
        }
    }

    out_bounds[adelta_y] = p2.x;
    set_pixel_color_lmd(gr_buffer, p2, color);
}



void write_slope_x_bound_dda(dbuffi out_bounds, dbuff2f out_slope_itpl_buffer, 
        vec2i p1, vec2i p2, dbufff itpl_buffer_from, dbufff itpl_buffer_to) {
    if (p1 == p2) {
        out_bounds[0] = p1.x;

        // write interpolation data
        for (u32 i = 0; i < cap(&itpl_buffer_from); i++) {
            out_slope_itpl_buffer.get(0, i) = itpl_buffer_from[i];
        }
        return;
    }
    
    i32 delta_x = p2.x - p1.x;
    i32 delta_y = p2.y - p1.y;

    dbufff cur_itpl_buffer = {(f32*)alloca(sizeof(f32) * itpl_buffer_from.cap), itpl_buffer_from.cap};
    dbufff cur_itpl_deltas = {(f32*)alloca(sizeof(f32) * itpl_buffer_from.cap), itpl_buffer_from.cap};

    memcpy(cur_itpl_buffer.buffer, itpl_buffer_from.buffer, sizeof(f32) * cap(&cur_itpl_buffer));

    if (abs(delta_x) > abs(delta_y)) {
        // calculate delta vector
        for (u32 i = 0; i < cap(&cur_itpl_deltas); i++) {
            cur_itpl_deltas[i] = (itpl_buffer_to[i] - itpl_buffer_from[i]) / abs(delta_x);
        }

        f32 k = (f32)delta_y / delta_x;

        i32 prev_y1 = INT_MIN;
        for (i32 x1 = 0; x1 != delta_x; x1 += sgn(delta_x)) {
            // update current interpolation buffer
            for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                cur_itpl_buffer[i] += cur_itpl_deltas[i]; 
            }

            i32 y1 = round(k * x1);

            if (y1 != prev_y1) {
                out_bounds[y1] = p1.x + x1;
                // write interpolation data
                for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                    out_slope_itpl_buffer.get(y1, i) = cur_itpl_buffer[i];
                }
                prev_y1 = y1;
            }
        }

    } else {
        // calculate delta vector
        for (u32 i = 0; i < cap(&cur_itpl_deltas); i++) {
            cur_itpl_deltas[i] = (itpl_buffer_to[i] - itpl_buffer_from[i]) / abs(delta_y);
        }

        f32 k = (f32)delta_x / delta_y;

        for (i32 y1 = 0; y1 != delta_y; y1 += sgn(delta_y)) {
            // update current interpolation buffer
            for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                cur_itpl_buffer[i] += cur_itpl_deltas[i]; 
            }

            i32 x1 = round(k * y1);
            
            out_bounds[y1] = p1.x + x1;
            // write interpolation data
            for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                out_slope_itpl_buffer.get(y1, i) = cur_itpl_buffer[i];
            }
        }
    }

    out_bounds[delta_y] = p2.x;

    // write interpolation data
    for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
        out_slope_itpl_buffer.get(delta_y, i) = cur_itpl_buffer[i];
    }

}


void write_slope_x_left_bound(dbuffi out_bounds, dbuff2f out_slope_itpl_buffer, 
        vec2i p1, vec2i p2, dbufff itpl_buffer_from, dbufff itpl_buffer_to) {

    if (p1 == p2) {
        out_bounds[0] = p1.x;

        // write interpolation data
        for (u32 i = 0; i < cap(&itpl_buffer_from); i++) {
            out_slope_itpl_buffer.get(0, i) = itpl_buffer_from[i];
        }
        return;
    }

    
    i32 adelta_x = abs(p2.x - p1.x);
    i32 adelta_y = abs(p2.y - p1.y);

    dbufff cur_itpl_buffer = {(f32*)alloca(sizeof(f32) * itpl_buffer_from.cap), itpl_buffer_from.cap};
    dbufff cur_itpl_deltas = {(f32*)alloca(sizeof(f32) * itpl_buffer_from.cap), itpl_buffer_from.cap};


    sbuff<f32*, 2> itpl_buffer_p = {&itpl_buffer_from[0], &itpl_buffer_to[0]};


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

        // write left pixel in the pixel row
        // if goint to the right save first pixel in the row
        if (delta_x > 0) {
            i32 prev_y = INT_MIN;
            for (i32 x = p1.x; x != p2.x; x+=sgn_delta_x) {
                i32 local_y = (y - p1.y) * sgn_delta_y;

                if (y != prev_y) {
                    out_bounds[local_y] = x;
                    // write interpolation data
                    for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                        out_slope_itpl_buffer.get(local_y, i) = cur_itpl_buffer[i];
                    }
                    prev_y = y;
                }

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
        } else { // if goint to the left save last pixel in the row
            for (i32 x = p1.x; x != p2.x; x+=sgn_delta_x) {
                i32 local_y = (y - p1.y) * sgn_delta_y;

                out_bounds[local_y] = x;
                // write interpolation data
                for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                    out_slope_itpl_buffer.get(local_y, i) = cur_itpl_buffer[i];
                }

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
}



void write_slope_x_right_bound(dbuffi out_bounds, dbuff2f out_slope_itpl_buffer, 
        vec2i p1, vec2i p2, dbufff itpl_buffer_from, dbufff itpl_buffer_to) {

    if (p1 == p2) {
        out_bounds[0] = p1.x;

        // write interpolation data
        for (u32 i = 0; i < cap(&itpl_buffer_from); i++) {
            out_slope_itpl_buffer.get(0, i) = itpl_buffer_from[i];
        }
        return;
    }

    
    i32 adelta_x = abs(p2.x - p1.x);
    i32 adelta_y = abs(p2.y - p1.y);

    dbufff cur_itpl_buffer = {(f32*)alloca(sizeof(f32) * itpl_buffer_from.cap), itpl_buffer_from.cap};
    dbufff cur_itpl_deltas = {(f32*)alloca(sizeof(f32) * itpl_buffer_from.cap), itpl_buffer_from.cap};


    sbuff<f32*, 2> itpl_buffer_p = {&itpl_buffer_from[0], &itpl_buffer_to[0]};


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

        // write right pixel in the pixel row
        // if goint to the right save last pixel in the row
        if (delta_x < 0) {
            i32 prev_y = INT_MIN;
            for (i32 x = p1.x; x != p2.x; x+=sgn_delta_x) {
                i32 local_y = (y - p1.y) * sgn_delta_y;

                if (y != prev_y) {
                    out_bounds[local_y] = x;
                    // write interpolation data
                    for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                        out_slope_itpl_buffer.get(local_y, i) = cur_itpl_buffer[i];
                    }
                    prev_y = y;
                }

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
        } else { // if goint to the left save first pixel in the row
            for (i32 x = p1.x; x != p2.x; x+=sgn_delta_x) {
                i32 local_y = (y - p1.y) * sgn_delta_y;

                out_bounds[local_y] = x;
                // write interpolation data
                for (u32 i = 0; i < cap(&cur_itpl_buffer); i++) {
                    out_slope_itpl_buffer.get(local_y, i) = cur_itpl_buffer[i];
                }

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
}