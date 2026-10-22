#version 450
#extension GL_GOOGLE_include_directive : require
layout(constant_id=0) const uint sourceFormat=0u;
#ifdef H2_INTEGER_INPUT
layout(location=0) in ivec4 nativeInput;
#else
layout(location=0) in vec4 nativeInput;
#endif
layout(set=0,binding=0,std430) readonly buffer Raw {uint words[];} raw;
layout(push_constant) uniform Input {uint prefix;uint stride;} inputData;
uint sourceWorldVertexWord(uint index){return raw.words[index];}
#include "source_world_vertex_fetch.glsl"
layout(location=0) flat out uvec4 nativeBits;
layout(location=1) flat out uvec4 fetchedBits;
void main() {
    uint index=uint(gl_VertexIndex);
    gl_Position=vec4((vec2(index&255u,index>>8u)+0.5)/128.0-1.0,0,1);
    gl_PointSize=1.0;
#ifdef H2_INTEGER_INPUT
    nativeBits=uvec4(nativeInput);
#else
    nativeBits=floatBitsToUint(nativeInput);
#endif
    fetchedBits=sourceWorldVertexFetch(inputData.prefix+index*inputData.stride,sourceFormat);
}
