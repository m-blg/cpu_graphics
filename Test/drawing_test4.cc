#include "../SDL_main.cc"
#include <SDL2/SDL_image.h>

#include "cp_lib/basic.cc"
#include "cp_lib/array.cc"
#include "cp_lib/vector.cc"
#include "cp_lib/matrix.cc"
#include "cp_lib/memory.cc"
#include <stdlib.h>
#include <unistd.h>

#include "../draw.cc"
#include "wireframe_test.cc"

using namespace cp;

sbuff<Tuple<vec3f, vec2f, vec4f>, 4> cube_vertices = {{
    { { -1, -1, -1 }, { {0, 0}, {{1, 1, 0, 0}} } },
	{ { 1, -1, -1 }, { {1, 0}, {{1, 0, 1, 0}} } },
	{ { 1, 1, -1 }, { {1, 1}, {{1, 0, 0, 1}} } },
	{ { -1, 1, -1 }, { {0, 1}, {{1, 1, 0, 0}} } }
}};
sbuff<u32[3], 10> cube_triangles = {{
    {0, 2, 1}, //face front
    {0, 3, 2},
}};

// sbuff<u32[2], 12> cube_edges = {{
//     {0, 1}, {0, 3}, {1, 2}, {2, 3},
//     {4, 5}, {4, 7}, {6, 5}, {6, 7},
//     {0, 7}, {1, 6}, {2, 5}, {3, 4},
// }};

Mesh cube_mesh = {{(u8*)cube_vertices.buffer, cap(&cube_vertices) * (u32)sizeof(Tuple<vec3f, vec2f, vec4f>)}, {cube_triangles.buffer, cap(&cube_triangles)}};

vec3f cube_position = { 0, 0, 3};
quat cube_rotation = {1, 0, 0, 0};

bool is_ortho = true;


dbuff<vec3f> proj_buffer;

dbuff<u8> proc_buffer;

float line_color[2][4] = {{1, 0.9, 0, 0.9}, {1, 0, 0.9, 0.9}};
dbuff2f line_color_buffer = {(f32*)line_color, 2, 4};

// float triangle_color[3][4] = {{1, 0.9, 0, 0.9}, {1, 0, 0.9, 0.9}, {1, 0.9, 0.9, 0}};
float triangle_color[3][4] = {{1, 1, 0, 0}, {1, 0, 0, 1}, {1, 0, 1, 0}};
dbuff2f triangle_color_buffer = {(f32*)triangle_color, 3, 4};


dbuff2f z_buffer;

static quat rot_x;
static quat rot_y;
static quat rot_z;

dbuff2u test_texture;


void test_color_itpl_vsh(void* handle_p) {
    auto handle = (Vertex_Shader_Handle*)handle_p;

    auto vertex = (Tuple<vec3f, vec2f, vec4f>*)handle->vertex;
    
    auto t_args = (Tuple<mat4f*, vec2f(*)(vec3f)>*)handle->args;
    mat4f* affine_m = t_args->get<0>();
    vec2f(*project_lmd)(vec3f) = t_args->get<1>();

    vec4f vertex_pos = { vertex->get<0>().x, vertex->get<0>().y, vertex->get<0>().z, 1 };
    vec4f p = *affine_m * vertex_pos;
    vec2f pr = project_lmd({p.x, p.y, p.z});

    handle->out_vertex_itpl_vector[0] = p.z;
    *(vec2f*)&handle->out_vertex_itpl_vector[1] = vertex->get<1>();
    *(vec4f*)&handle->out_vertex_itpl_vector[3] = vertex->get<2>();
    *handle->out_vertex_position = space_to_screen_coord(pr, window_size, {100, 100});
}


void test_color_itpl_fsh(void* args) {
    auto handle = (Fragment_Shader_Handle*)args;

    float z = handle->itpl_vector[0];
    dbuff2f *z_buffer = (dbuff2f*)handle->args;
    f32* prev_z;
    if (!z_buffer->sget(&prev_z, handle->point.y, handle->point.x) || z > *prev_z)
        return;
    

    vec4f *fcolor = (vec4f*)&handle->itpl_vector[1];
    Color color;
    if (abs(z) != 0) { 
        color = to_color( *fcolor / z);
    } else 
        color = {0xffffffff};
    handle->set_pixel_color_lmd(handle->out_frame_buffer, handle->point, color);
    *prev_z = z;
}

void test_texture_fsh(void* args) {
    auto handle = (Fragment_Shader_Handle*)args;

    float z = handle->itpl_vector[0];
    auto args_tuple = (Tuple<dbuff2f*, dbuff2u*>*)handle->args;
    dbuff2f *z_buffer = args_tuple->get<0>(); 
    f32* prev_z;
    if (!z_buffer->sget(&prev_z, handle->point.y, handle->point.x) || z > *prev_z)
        return;
    
    dbuff2u *texture = args_tuple->get<1>();
    vec2f *itpl_uv = (vec2f*)&handle->itpl_vector[1];
    vec2i uv = { (i32)(texture->x_cap * itpl_uv->u), (i32)(texture->y_cap * itpl_uv->v) };
    Color color = { texture->get(uv.y, uv.x) }; 
    if (color.a < 1) return;
    // if (abs(z) != 0) { 
    //     color = to_color( *fcolor / z);
    // } else 
    //     color = {0xffffffff};
    handle->set_pixel_color_lmd(handle->out_frame_buffer, handle->point, color);
    *prev_z = z;
}


void game_init() {
    input_init();

    window_max_size = {1920, 1080};
    window_size = {1280, 720};

    proj_buffer.init(8);
    proc_buffer.init(80000);
    // create buffer

    frame_buffer.init(window_size.y/4, window_size.x/4);
    z_buffer.init(window_size.y/4, window_size.x/4);

    frame_buffer.init(window_size.y, window_size.x);
    z_buffer.init(window_size.y, window_size.x);

    rot_x.init(normalized(vec3f{1, 0, 0}), M_PI/10);
    rot_y.init(normalized(vec3f{0, 1, 0}), M_PI/10);
    rot_z.init(normalized(vec3f{0, 0, 1}), M_PI/10);

    SDL_Init(SDL_INIT_VIDEO);

    int flags = IMG_INIT_PNG;
    if ( !( IMG_Init( flags ) & flags ) ) {
        printf("Can't init image: %i\n", IMG_GetError());
        assert(false);
    }

    TTF_Init();

    SDL_Surface *test_texture_sur = IMG_Load("Test/Assets/TestTexture.png");
    test_texture = { (u32*)test_texture_sur->pixels, test_texture_sur->h, test_texture_sur->w };
}

void game_shut() {
    input_shut();
    proj_buffer.shut();
    proc_buffer.shut();
    frame_buffer.shut();
    z_buffer.shut();
}



void game_update() {


    if (get_bit(Input::keys_down, 'q')) {
        is_running = false;
    }
    if (get_bit(Input::keys_down, 'o')) {
        is_ortho = !is_ortho;
    }

    if (get_bit(Input::keys_hold, 'w')) {
        cube_position += vec3f(0, 0, 0.1);
    }
    if (get_bit(Input::keys_hold, 's')) {
        cube_position += vec3f(0, 0, -0.1);
    }
    if (get_bit(Input::keys_hold, 'a')) {
        cube_position += vec3f(-0.1, 0, 0);
    }
    if (get_bit(Input::keys_hold, 'd')) {
        cube_position += vec3f(0.1, 0, 0);
    }
    if (get_bit(Input::keys_hold, 't')) {
        cube_rotation = rot_x * cube_rotation;
    }    
    if (get_bit(Input::keys_hold, 'g')) {
        cube_rotation = inverse(rot_x) * cube_rotation;
    }
    if (get_bit(Input::keys_hold, 'f')) {
        cube_rotation = rot_y * cube_rotation;
    }
    if (get_bit(Input::keys_hold, 'h')) {
        cube_rotation = inverse(rot_y) * cube_rotation;
    }
    if (get_bit(Input::keys_hold, 'r')) {
        cube_rotation = rot_z * cube_rotation;
    }
    if (get_bit(Input::keys_hold, 'y')) {
        cube_rotation = inverse(rot_z) * cube_rotation;
    }

    // write to buffer

    for (auto p = begin(&frame_buffer); p < end(&frame_buffer); p++) {
        *p = 0xff223344;
    }
    for (auto p = begin(&z_buffer); p < end(&z_buffer); p++) {
        *p = INT_MAX;
    }

    // Color c = {0xff5533ff};


    // if (pointer_local_pos.x < window_size.x && pointer_local_pos.y < window_size.y)
    //     rasterize_line(frame_buffer, {500, 500}, pointer_local_pos - vec2i(100, 100), 
    //     line_color_buffer, color_itpl_frag_shader, null, set_pixel_color);
    
    // if (pointer_local_pos.x < window_size.x && pointer_local_pos.y < window_size.y)
    //     rasterize_triangle_scanline(frame_buffer, {500, 500}, {300, 200}, pointer_local_pos, {0xff00ff00}, proc_buffer, set_pixel_color);

    // if (pointer_local_pos.x < window_size.x && pointer_local_pos.y < window_size.y)
    //     rasterize_triangle_scanline(frame_buffer, {500, 500}, {300, 200}, pointer_local_pos, 
    //         triangle_color_buffer, color_itpl_frag_shader, null, proc_buffer, set_pixel_color);

    // rasterize_triangle_scanline(frame_buffer, {500, 500}, {300, 200}, {700, 600}, 
    //         triangle_color_buffer, color_itpl_frag_shader, null, proc_buffer, set_pixel_color);

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
    // if (is_ortho) {
    //     render_mesh({&cube_mesh, &cube_position, &cube_rotation}, project_xy_orthogonal, proc_buffer, 
    //                 &frame_buffer, z_buffer, window_size/4, {25, 25});
    // } else {
    //     render_mesh({&cube_mesh, &cube_position, &cube_rotation}, project_xy_perspective, proc_buffer, 
    //                 &frame_buffer, z_buffer, window_size/4, {100, 100});
    // }


    Render_Object robj = {&cube_mesh, &cube_position, &cube_rotation};
    desbuff vb = {{(u8*)robj.mesh->vertex_buffer.buffer, robj.mesh->vertex_buffer.cap}, sizeof(Tuple<vec3f, vec2f, vec4f>)};

    Tuple<mat4f*, vec2f(*)(vec3f)> vsh_args;
    mat4f scale_m = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    mat4f rotation_m = vec_rot_mat4(cube_rotation);
    mat4f translation_m = {
        1, 0, 0, cube_position.x,
        0, 1, 0, cube_position.y,
        0, 0, 1, cube_position.z,
        0, 0, 0, 1
    };
    mat4f affine_m = translation_m * rotation_m * scale_m;
    vsh_args.get<0>() = &affine_m;
    if (is_ortho) {
        vsh_args.get<1>() = project_xy_orthogonal;
    } else 
        vsh_args.get<1>() = project_xy_perspective;


    Tuple<dbuff2f*, dbuff2u*> fsh_arg_tupple = { &z_buffer, {&test_texture} };
    Shader_Pack shaders = {test_color_itpl_vsh, &vsh_args, test_texture_fsh, &fsh_arg_tupple, 7};
    draw_triangles(&frame_buffer, &vb, &robj.mesh->index_buffer, &shaders, &proc_buffer);
    // dbuff<u32[2]> edges_ib = {cube_edges.buffer, cap(&cube_edges)};
    // draw_lines(&frame_buffer, &vb, &edges_ib, &shaders, &proc_buffer);


    vec2i pointer_local_pos = Input::mouse_pos/4;
    if (0 < pointer_local_pos.x - 5 && pointer_local_pos.x - 5 < window_size.x/4 && 0 < pointer_local_pos.y - 5 && pointer_local_pos.y - 5 < window_size.y/4) {
        frame_buffer.get(pointer_local_pos.y-5, pointer_local_pos.x-5) = 0xffff0000;
        // printf("%i %i\n", pointer_localq_pos.x-5, pointer_local_pos.y-5);
    }
    



}

