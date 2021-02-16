#include "../draw.cc"
#include "../mesh.cc"
#include "../../cp_lib/quaternion.cc"

using namespace cp;

struct Render_Object {
    Mesh *mesh;
    vec3f *position;
    quat *rotation;
};

// cap(_proj_vertex_buffer) >= cap(obj.mesh->vertices)
void render_wireframe(Render_Object obj, vec2f(*project_lmd)(vec3f), dbuff<vec2f> _proj_vertex_buffer, 
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


