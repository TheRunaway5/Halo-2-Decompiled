#version 450
#extension GL_GOOGLE_include_directive : require
#include "world_depth_geometry.glsl"
// Candidate for a geometry-stage-only original-capture replacement. This
// keeps the original ordinary GS interface, input order and emit0/1/2 policy.
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
layout(location = 0) in vec4 v_vtxD0[];
layout(location = 1) in vec4 v_vtxD1[];
layout(location = 2) in vec4 v_vtxB0[];
layout(location = 3) in vec4 v_vtxB1[];
layout(location = 4) in float v_vtxFog[];
layout(location = 5) in vec4 v_vtxT0[];
layout(location = 6) in vec4 v_vtxT1[];
layout(location = 7) in vec4 v_vtxT2[];
layout(location = 8) in vec4 v_vtxT3[];
layout(location = 9) flat in vec4 v_vtxPos0[];
layout(location = 10) flat in vec4 v_vtxPos1[];
layout(location = 11) flat in vec4 v_vtxPos2[];
layout(location = 12) flat in float v_triMZ[];
layout(location = 0) out vec4 vtxD0;
layout(location = 1) out vec4 vtxD1;
layout(location = 2) out vec4 vtxB0;
layout(location = 3) out vec4 vtxB1;
layout(location = 4) out float vtxFog;
layout(location = 5) out vec4 vtxT0;
layout(location = 6) out vec4 vtxT1;
layout(location = 7) out vec4 vtxT2;
layout(location = 8) out vec4 vtxT3;
layout(location = 9) flat out vec4 vtxPos0;
layout(location = 10) flat out vec4 vtxPos1;
layout(location = 11) flat out vec4 vtxPos2;
layout(location = 12) flat out float triMZ;
void emit_original_vertex(int index, mat4 triangle) {
    gl_Position = gl_in[index].gl_Position;
    gl_PointSize = gl_in[index].gl_PointSize;
    vtxD0 = v_vtxD0[index];
    vtxD1 = v_vtxD1[index];
    vtxB0 = v_vtxB0[index];
    vtxB1 = v_vtxB1[index];
    vtxFog = v_vtxFog[index];
    vtxT0 = v_vtxT0[index];
    vtxT1 = v_vtxT1[index];
    vtxT2 = v_vtxT2[index];
    vtxT3 = v_vtxT3[index];
    vtxPos0 = triangle[0];
    vtxPos1 = triangle[1];
    vtxPos2 = triangle[2];
    triMZ = world_depth_finite_slope(triangle);
    EmitVertex();
}
void main() {
#ifdef HALO2_WORLD_DEPTH_WRONG_INPUT_ORDER
    // Negative control: cyclically rotate only the depth inputs. Preserve
    // raster positions/emission, so a difference diagnoses ordered GS data.
    mat4 triangle = world_depth_triangle(v_vtxPos0[1], v_vtxPos0[2], v_vtxPos0[0]);
#else
    mat4 triangle = world_depth_triangle(v_vtxPos0[0], v_vtxPos0[1], v_vtxPos0[2]);
#endif
    emit_original_vertex(0, triangle);
    emit_original_vertex(1, triangle);
    emit_original_vertex(2, triangle);
    EndPrimitive();
}
