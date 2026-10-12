#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) in vec4 corner;
layout(set=0,binding=2,std140) uniform ParticleVertexState {
    vec4 attributes[16];vec4 constants[192];vec4 clipRange;
    vec4 surfaceFog; // surface width/height, original fog parameters.
} vertexState;
#define particleAttributes vertexState.attributes
#define particleConstants vertexState.constants
#define particleCorner corner
#define particleSurfaceSize vertexState.surfaceFog.xy
#define particleFogParam vertexState.surfaceFog.zw
#define particleClipRange vertexState.clipRange
layout(location=0) out vec4 v_vtxD0;
layout(location=1) out vec4 v_vtxD1;
vec4 v_vtxB0;
vec4 v_vtxB1;
float v_vtxFog;
layout(location=5) out vec4 v_vtxT0;
layout(location=6) out vec4 v_vtxT1;
vec4 v_vtxT2;
vec4 v_vtxT3;
layout(location=9) flat out vec4 v_vtxPos0;
vec4 v_vtxPos1;
vec4 v_vtxPos2;
float v_triMZ;
#ifdef PARTICLE_DIRECTIONAL
#include "particle_directional_program.glsl"
#else
#include "particle_program.glsl"
#endif
#include "particle_vertex_output.glsl"
void main(){particleProgram();particleVertexOutput();}
