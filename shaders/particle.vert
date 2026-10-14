#version 450
#extension GL_GOOGLE_include_directive : require
#ifdef PARTICLE_MODEL
layout(location=0) in vec4 modelPosition;
layout(location=1) in vec4 modelUV;
vec4 modelAttributes[16];
#else
layout(location=0) in vec4 corner;
#endif
layout(set=0,binding=2,std140) uniform ParticleVertexState {
    vec4 attributes[16];vec4 constants[192];vec4 clipRange;
    vec4 surfaceFog; // surface width/height, original fog parameters.
} vertexState;
#ifdef PARTICLE_MODEL
#define particleAttributes modelAttributes
#else
#define particleAttributes vertexState.attributes
#endif
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
#if defined(PARTICLE_MODEL)
#include "particle_model_program.glsl"
#elif defined(PARTICLE_PERPENDICULAR)
#include "particle_perpendicular_program.glsl"
#elif defined(PARTICLE_DIRECTIONAL)
#include "particle_directional_program.glsl"
#elif defined(PARTICLE_DISTORTION)
#include "particle_distortion_program.glsl"
#else
#include "particle_program.glsl"
#endif
#include "particle_vertex_output.glsl"
void main(){
#ifdef PARTICLE_MODEL
  for(int i=0;i<16;++i)modelAttributes[i]=vertexState.attributes[i];
  modelAttributes[0]=modelPosition;modelAttributes[1]=modelUV;
#endif
  particleProgram();particleVertexOutput();
}
