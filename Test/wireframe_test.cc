#include "../draw.cc"
#include "../mesh.cc"
#include "../../cp_lib/quaternion.cc"

using namespace cp;

struct Render_Object {
    Mesh *mesh;
    vec3f *position;
    quat *rotation;
};

void wireframe_frag_shader(dbuff2u out_buffer, vec2i p, dbufff itpl_buffer, 
                    void* args, void (*set_pixel_color_lmd)(dbuff2u, vec2i, Color)) {
    
    float z = itpl_buffer[0];
    dbuff2f *z_buffer = (dbuff2f*)args;
    f32* prev_z;
    if (!z_buffer->sget(&prev_z, p.y, p.x) || z > *prev_z)
        return;
    

    Color color;
    if (abs(z) > 1) { 
        color = to_color(vec4f(1, 1, 1, 1) / z);
    } else 
        color = {0xffffffff};
    set_pixel_color_lmd(out_buffer, p, color);
    *prev_z = z;
}

// cap(_proj_vertex_buffer) >= cap(obj.mesh->vertices)
void render_wireframe_(Render_Object obj, vec2f(*project_lmd)(vec3f), dbuff<vec2f> _proj_vertex_buffer, 
                        dbuff2u buffer, Color color, vec2i window_size, vec2i pixels_per_unit) {
    for (u32 i = 0; i < cap(&obj.mesh->vertices); i++) {
        _proj_vertex_buffer[i] = project_lmd(*obj.rotation * obj.mesh->vertices[i] + *obj.position);
    }

    for (u32 i = 0; i < cap(&obj.mesh->triangles); i++) {
        draw_line(buffer, _proj_vertex_buffer[obj.mesh->triangles[i][0]], 
                  _proj_vertex_buffer[obj.mesh->triangles[i][1]], 
                  color, window_size, pixels_per_unit);
        draw_line(buffer, _proj_vertex_buffer[obj.mesh->triangles[i][1]], 
                  _proj_vertex_buffer[obj.mesh->triangles[i][2]], 
                  color, window_size, pixels_per_unit);
        draw_line(buffer, _proj_vertex_buffer[obj.mesh->triangles[i][2]], 
                  _proj_vertex_buffer[obj.mesh->triangles[i][0]], 
                  color, window_size, pixels_per_unit);
    }
}

// cap(_proj_vertex_buffer) >= cap(obj.mesh->vertices)
void render_wireframe(Render_Object obj, vec2f(*project_lmd)(vec3f), dbuff<vec3f> _proj_vertex_buffer, 
                    dbuff2u buffer, dbuff2f z_buffer,
                    vec2i window_size, vec2i pixels_per_unit) {
    for (u32 i = 0; i < cap(&obj.mesh->vertices); i++) {
        vec3f p = *obj.rotation * obj.mesh->vertices[i] + *obj.position;
        vec2f pr = project_lmd(p);
        _proj_vertex_buffer[i] = {pr.x, pr.y, p.z};
    }

    for (u32 i = 0; i < cap(&obj.mesh->triangles); i++) {
        vec3f& p0 = _proj_vertex_buffer[obj.mesh->triangles[i][0]];
        vec3f& p1 = _proj_vertex_buffer[obj.mesh->triangles[i][1]];
        vec3f& p2 = _proj_vertex_buffer[obj.mesh->triangles[i][2]];

        float raw_itpl_buffer[2] = {p0.z, p1.z};
        dbuff2f itpl_buffer = {raw_itpl_buffer, 2, 1};
        draw_line(buffer, {p0.x, p0.y}, 
                  {p1.x, p1.y}, 
                  itpl_buffer, wireframe_frag_shader, &z_buffer, window_size, pixels_per_unit);
        
        raw_itpl_buffer[0] = p1.z; raw_itpl_buffer[1] = p2.z;
        draw_line(buffer, {p1.x, p1.y}, 
                  {p2.x, p2.y}, 
                  itpl_buffer, wireframe_frag_shader, &z_buffer, window_size, pixels_per_unit);

        raw_itpl_buffer[0] = p2.z; raw_itpl_buffer[1] = p0.z;
        draw_line(buffer, {p2.x, p2.y}, 
                  {p0.x, p0.y}, 
                  itpl_buffer, wireframe_frag_shader, &z_buffer, window_size, pixels_per_unit);
    }
}

