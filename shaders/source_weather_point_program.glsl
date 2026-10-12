// Actual immediate391F0 path: v0.xyzw and the logical packed-color v1.
// The reference-input switch prevents the unrelated ordinary MODEL decoder;
// these inputs are supplied by the captured immediate command producer.
struct MaterialData {uvec4 vertexInstructions[136];vec4 vertexAttributes[16];uvec4 flags;};
MaterialData material;
#define H2_VERTEX_REFERENCE_INPUTS
#define H2_VERTEX_REFERENCE_CONSTANTS
vec4 vpConstant(uint index){return index<12u?weather.camera[index]:vec4(0);}
#include "vertex_program.glsl"
void sourceWeatherPointProgram(vec4 position,uint color) {
    for(uint i=0u;i<136u;++i)material.vertexInstructions[i]=weather.instructions[i];
    for(uint i=0u;i<16u;++i)material.vertexAttributes[i]=vec4(0);
    material.vertexAttributes[0]=position;
    material.vertexAttributes[1]=vec4((color>>16u)&255u,(color>>8u)&255u,color&255u,color>>24u)/255.0;
    material.flags=weather.flags;
    runVertexProgram();
}
