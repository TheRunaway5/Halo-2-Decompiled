#version 450
#extension GL_GOOGLE_include_directive : require
// Test-only literal original declarations, palettes and compression banks.
layout(location=0) in vec4 sourceV0;
layout(location=1) in vec4 sourceV1;
layout(location=2) in vec4 sourceV2;
layout(location=3) in vec4 sourceV3;
layout(location=0) out vec4 sourceCoordinates[4];
layout(set=0,binding=3,std140) uniform SourceRegisters {
    uvec4 instructions[136];vec4 attributes[16];uvec4 flags;
    vec4 constants[192];vec4 conversion;
} original;
struct MaterialData {uvec4 vertexInstructions[136];vec4 vertexAttributes[16];uvec4 flags;};
MaterialData material;
#define H2_VERTEX_REFERENCE_CONSTANTS
#define H2_VERTEX_REFERENCE_INPUTS
#define H2_EXTENDED_VERTEX_BANK
vec4 vpConstant(uint index){return original.constants[index];}
#include "vertex_program.glsl"
void main() {
    for(uint i=0u;i<136u;++i)material.vertexInstructions[i]=original.instructions[i];
    for(uint i=0u;i<16u;++i)material.vertexAttributes[i]=original.attributes[i];
    material.vertexAttributes[0]=sourceV0;material.vertexAttributes[1]=sourceV1;
    material.vertexAttributes[2]=sourceV2;material.vertexAttributes[3]=sourceV3;
    material.flags=original.flags;runVertexProgram();
    vec4 screen=vpOut[0];screen.xy=trunc(screen.xy*16.0)/16.0;
    screen.w=(screen.w<0.0?-1.0:1.0)*clamp(abs(screen.w),5.421010862427522e-20,1.8446744073709552e19);
    vec2 ndc=(screen.xy*2.0-original.conversion.xy)/original.conversion.xy;
    gl_Position=vec4(ndc*screen.w,(screen.z/original.conversion.z)*screen.w,screen.w);
    for(uint i=0u;i<4u;++i)sourceCoordinates[i]=vpOut[9u+i];
}
