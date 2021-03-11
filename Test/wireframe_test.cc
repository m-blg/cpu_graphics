#include "../draw.cc"
#include "../mesh.cc"
#include "cp_lib/quaternion.cc"

using namespace cp;

struct Render_Object {
    Mesh *mesh;
    vec3f *position;
    quat *rotation;
};

void wireframe_frag_shader(void* args) {
    Fragment_Shader_Handle* handle = (Fragment_Shader_Handle*)args;

    float z = handle->itpl_vector[0];
    dbuff2f *z_buffer = (dbuff2f*)handle->args;
    f32* prev_z;
    if (!z_buffer->sget(&prev_z, handle->point.y, handle->point.x) || z > *prev_z)
        return;
    

    Color color;
    if (abs(z) > 1) { 
        color = to_color(vec4f(1, 1, 1, 1) / z);
    } else 
        color = {0xffffffff};
    handle->set_pixel_color_lmd(handle->out_frame_buffer, handle->point, color);
    *prev_z = z;
}

void test_vertex_shader(void* args) {
    Vertex_Shader_Handle* handle = (Vertex_Shader_Handle*)args;

    auto t_args = (Tuple<Render_Object*, vec2f(*)(vec3f)>*)handle->args;
    Render_Object* obj = t_args->get<0>();
    vec2f(*project_lmd)(vec3f) = t_args->get<1>();

    vec3f p = *obj->rotation * *(vec3f*)handle->vertex + *obj->position;
    vec2f pr = project_lmd(p);

    handle->out_vertex_itpl_vector[0] = p.z;
    *handle->out_vertex_position = space_to_screen_coord(pr, window_size/4, {25, 25});
}

// cap(_proj_vertex_buffer) >= cap(obj.mesh->vertex_buffer)
// void render_wireframe_(Render_Object obj, vec2f(*project_lmd)(vec3f), dbuff<vec2f> _proj_vertex_buffer, 
//                         dbuff2u *buffer, Color color, vec2i window_size, vec2i pixels_per_unit) {
//     for (u32 i = 0; i < cap(&obj.mesh->vertex_buffer); i++) {
//         _proj_vertex_buffer[i] = project_lmd(*obj.rotation * obj.mesh->vertex_buffer[i] + *obj.position);
//     }

//     for (u32 i = 0; i < cap(&obj.mesh->index_buffer); i++) {
//         draw_line(buffer, _proj_vertex_buffer[obj.mesh->index_buffer[i][0]], 
//                   _proj_vertex_buffer[obj.mesh->index_buffer[i][1]], 
//                   color, window_size, pixels_per_unit);
//         draw_line(buffer, _proj_vertex_buffer[obj.mesh->index_buffer[i][1]], 
//                   _proj_vertex_buffer[obj.mesh->index_buffer[i][2]], 
//                   color, window_size, pixels_per_unit);
//         draw_line(buffer, _proj_vertex_buffer[obj.mesh->index_buffer[i][2]], 
//                   _proj_vertex_buffer[obj.mesh->index_buffer[i][0]], 
//                   color, window_size, pixels_per_unit);
//     }
// }

// cap(_proj_vertex_buffer) >= cap(obj.mesh->vertex_buffer)
// void render_wireframe(Render_Object obj, vec2f(*project_lmd)(vec3f), dbuff<vec3f> _proj_vertex_buffer, 
//                     dbuff2u *buffer, dbuff2f z_buffer,
//                     vec2i window_size, vec2i pixels_per_unit) {
//     for (u32 i = 0; i < cap(&obj.mesh->vertex_buffer); i++) {
//         vec3f p = *obj.rotation * obj.mesh->vertex_buffer[i] + *obj.position;
//         vec2f pr = project_lmd(p);
//         _proj_vertex_buffer[i] = {pr.x, pr.y, p.z};
//     }

//     for (u32 i = 0; i < cap(&obj.mesh->index_buffer); i++) {
//         vec3f& p0 = _proj_vertex_buffer[obj.mesh->index_buffer[i][0]];
//         vec3f& p1 = _proj_vertex_buffer[obj.mesh->index_buffer[i][1]];
//         vec3f& p2 = _proj_vertex_buffer[obj.mesh->index_buffer[i][2]];

//         float raw_itpl_buffer[2] = {p0.z, p1.z};
//         dbuff2f itpl_buffer = {raw_itpl_buffer, 2, 1};
//         draw_line(buffer, {p0.x, p0.y}, 
//                   {p1.x, p1.y}, 
//                   itpl_buffer, wireframe_frag_shader, &z_buffer, window_size, pixels_per_unit);
        
//         raw_itpl_buffer[0] = p1.z; raw_itpl_buffer[1] = p2.z;
//         draw_line(buffer, {p1.x, p1.y}, 
//                   {p2.x, p2.y}, 
//                   itpl_buffer, wireframe_frag_shader, &z_buffer, window_size, pixels_per_unit);

//         raw_itpl_buffer[0] = p2.z; raw_itpl_buffer[1] = p0.z;
//         draw_line(buffer, {p2.x, p2.y}, 
//                   {p0.x, p0.y}, 
//                   itpl_buffer, wireframe_frag_shader, &z_buffer, window_size, pixels_per_unit);
//     }
// }


// cap(_proj_vertex_buffer) >= cap(obj.mesh->vertex_buffer)
// void render_mesh(Render_Object obj, vec2f(*project_lmd)(vec3f), dbuff<u8> proc_buffer, 
//                     dbuff2u *buffer, dbuff2f z_buffer,
//                     vec2i window_size, vec2i pixels_per_unit) {
    
//     dbuff<vec3f> proj_vertex_buffer = {(vec3f*)proc_buffer.buffer, cap(&obj.mesh->vertex_buffer)};
//     dbuff<u8> tr_proc_buffer = {proc_buffer.buffer + sizeof(vec3f) * cap(&obj.mesh->vertex_buffer),
//         cap(&proc_buffer) - sizeof(vec3f) * cap(&obj.mesh->vertex_buffer)};

//     for (u32 i = 0; i < cap(&obj.mesh->vertex_buffer); i++) {
//         vec3f p = *obj.rotation * obj.mesh->vertex_buffer[i] + *obj.position;
//         vec2f pr = project_lmd(p);
//         proj_vertex_buffer[i] = {pr.x, pr.y, p.z};
//     }

//     for (u32 i = 0; i < cap(&obj.mesh->index_buffer); i++) {
//         vec3f& p0 = proj_vertex_buffer[obj.mesh->index_buffer[i][0]];
//         vec3f& p1 = proj_vertex_buffer[obj.mesh->index_buffer[i][1]];
//         vec3f& p2 = proj_vertex_buffer[obj.mesh->index_buffer[i][2]];

//         float raw_itpl_buffer[3] = {p0.z, p1.z, p2.z};
//         dbuff2f itpl_buffer = {raw_itpl_buffer, 3, 1};
//         draw_triangle(buffer, {p0.x, p0.y}, {p1.x, p1.y}, {p2.x, p2.y}, 
//                   itpl_buffer, wireframe_frag_shader, &z_buffer, &tr_proc_buffer, window_size, pixels_per_unit);
//     }
// }


