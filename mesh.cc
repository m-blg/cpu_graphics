#pragma once


#include "cp_lib/basic.cc"
#include "cp_lib/buffer.cc"
#include "cp_lib/vector.cc"
#include "cp_lib/math.cc"
#include <math.h>


struct Mesh {
    cp::dbuff<cp::vec3f> vertices;
    cp::dbuff<u32[3]> triangles;

};

// cap(&vertices) <= cap(&out_points)
void project_xy_orthogonal(cp::dbuff<cp::vec3f> vertices, cp::dbuff<cp::vec2f> out_points) {
    for (u32 i = 0; i < cap(&vertices); i++) {
        cp::vec3f *vertex = &vertices[i];
        out_points[i] = { vertex->x, vertex->y };
    }
}

cp::vec2f project_xy_orthogonal(cp::vec3f vertex) {
    return { vertex.x, vertex.y };
}


// cap(&vertices) <= cap(&out_points)
void project_xy_perspective(cp::dbuff<cp::vec3f> vertices, cp::dbuff<cp::vec2f> out_points) {
    for (u32 i = 0; i < cap(&vertices); i++) {
        cp::vec3f *vertex = &vertices[i];
        out_points[i] = { vertex->x / vertex->z, vertex->y / vertex->z };
    }
}

cp::vec2f project_xy_perspective(cp::vec3f vertex) {
    return { vertex.x / vertex.z, vertex.y / vertex.z };
}

