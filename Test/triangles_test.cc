#include "../SDL_main.cc"


#include <stdlib.h>
#include <unistd.h>

#include "../draw.cc"
#include "wireframe_test.cc"

using namespace cp;

vec2i triangles[4][3] = {
    {{10, 50}, {50, 10}, {70, 70}}
};

Color triangle_colors[4] = {
    {0xff00ff00},
    {0xffff0000},
    {0xff00ffff},
    {0xff5500f5}
};

bool is_drawing_vertices = false;

float line_color[2][4] = {{1, 0.9, 0, 0.9}, {1, 0, 0.9, 0.9}};
// float line_color[2][4] = {{1, 0.9, 0, 0.9}, {1, 0.9, 0, 0.9}};
dbuff2f line_color_buffer = {(f32*)line_color, 2, 4};

float triangle_color[3][4] = {{1, 1, 0, 0}, {1, 0, 0, 1}, {1, 0, 1, 0}};
// float triangle_color[3][4] = {{1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}};
dbuff2f triangle_color_buffer = {(f32*)triangle_color, 3, 4};

i32 tr_vertex_index = 0;

dbuff2f z_buffer;

dbuff<u8> proc_buffer;



void game_init() {
    input_init();

    window_max_size = {1920, 1080};
    window_size = {1000, 1000};

    // create buffer

    frame_buffer.init(100, 100);
    z_buffer.init(100, 100);

    proc_buffer.init(80000);

    triangles[1][0] = triangles[0][0];triangles[1][1] = triangles[0][1]; triangles[1][2] = {5, 25};
    triangles[2][0] = triangles[0][1];triangles[2][1] = triangles[0][2]; triangles[2][2] = {80, 35};
    triangles[3][0] = triangles[0][2];triangles[3][1] = triangles[0][0]; triangles[3][2] = {30, 90};
}

void game_shut() {
    input_shut();

    frame_buffer.shut();
    z_buffer.shut();
}



void game_update() {


    if (get_bit(Input::keys_down, 'q')) {
        is_running = false;
    }
    if (get_bit(Input::keys_down, 'v')) {
        is_drawing_vertices = !is_drawing_vertices;
    }
    if (get_bit(Input::keys_down, '1')) {
        tr_vertex_index = 0;
    }
    if (get_bit(Input::keys_down, '2')) {
        tr_vertex_index = 1;
    }
    if (get_bit(Input::keys_down, '3')) {
        tr_vertex_index = 2;
    }

    // write to buffer

    for (auto p = begin(&frame_buffer); p < end(&frame_buffer); p++) {
        *p = 0xff223344;
    }
    for (auto p = begin(&z_buffer); p < end(&z_buffer); p++) {
        *p = INT_MAX;
    }

    // Color c = {0xff5533ff};
    vec2i pointer_local_pos = Input::mouse_pos;
    rasterize_line(&frame_buffer, {25, 25}, pointer_local_pos/10 - vec2i(2, 2), 
    line_color_buffer, color_itpl_frag_shader, null, set_pixel_color);

    triangles[0][tr_vertex_index] = pointer_local_pos/10;
    triangles[1][0] = triangles[0][0];triangles[1][1] = triangles[0][1]; triangles[1][2] = {5, 25};
    triangles[2][0] = triangles[0][1];triangles[2][1] = triangles[0][2]; triangles[2][2] = {80, 35};
    triangles[3][0] = triangles[0][2];triangles[3][1] = triangles[0][0]; triangles[3][2] = {30, 90};
    
    for (u32 i = 0; i < 1; i++) {
        // rasterize_triangle_scanline(frame_buffer, triangles[i][0], triangles[i][1], triangles[i][2], triangle_colors[i], proc_buffer, set_pixel_color);
    }
 
    if (pointer_local_pos.x < window_size.x && pointer_local_pos.y < window_size.y)
        rasterize_triangle_scanline(&frame_buffer, {50, 50}, {30, 50}, pointer_local_pos/10, 
            triangle_color_buffer, color_itpl_frag_shader, null, proc_buffer, set_pixel_color);

    //  rasterize_triangle_scanline(frame_buffer, {5, 5}, {25, 15}, {10, 20}, 
    //          triangle_color_buffer, color_itpl_frag_shader, null, proc_buffer, set_pixel_color);

    // for (i32 i = 1; i < 200; i++) {
    //     rasterize_line(frame_buffer, {1800, 100 + i}, {300, 900 + i}, {0xffff55ff + i}, set_pixel_color);
    // }

    // if (is_ortho) {
    //     render_wireframe({&cube_mesh, &cube_position, &cube_rotation}, project_xy_orthogonal, _proj_buffer, 
    //                 frame_buffer, {0xffffffff}, window_size, {100, 100});
    // } else {
    //     render_wireframe({&cube_mesh, &cube_position, &cube_rotation}, project_xy_perspective, _proj_buffer, 
    //                 frame_buffer, {0xffffffff}, window_size, {100, 100});
    // }
    // if (is_ortho) {
    //     render_wireframe({&cube_mesh, &cube_position, &cube_rotation}, project_xy_orthogonal, _proj_buffer, 
    //                 frame_buffer, z_buffer, window_size, {100, 100});
    // } else {
    //     render_wireframe({&cube_mesh, &cube_position, &cube_rotation}, project_xy_perspective, _proj_buffer, 
    //                 frame_buffer, z_buffer, window_size, {100, 100});
    // }
  
    if (is_drawing_vertices) {
        for (u32 i = 0; i < 3; i++) {
            vec2i& v = triangles[0][i];
            frame_buffer.get(v.y, v.x) = 0xffff0000;
        }

        frame_buffer.get(5, 5) = 0xffff0000;
        frame_buffer.get(15, 25) = 0xffff0000;
        frame_buffer.get(20, 10) = 0xffff0000;


        frame_buffer.get(50, 50) = 0xffff0000;
        frame_buffer.get(50, 30) = 0xffff0000;
        frame_buffer.get(pointer_local_pos.y/10, pointer_local_pos.x/10) = 0xffff0000;
    }
}

