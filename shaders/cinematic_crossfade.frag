#version 450
#extension GL_GOOGLE_include_directive : require
layout(set=0,binding=0) uniform sampler2D capturedScene;
layout(set=0,binding=1,std140) uniform Crossfade {uvec4 program[15];} material;
layout(push_constant) uniform Frame {ivec2 extent;} frame;
layout(location=0) out vec4 pixel;
#include "combiner.glsl"
void main() {
    // Original point/clamp fullscreen sampling. Integer pixel centers keep
    // odd native window sizes from selecting a neighbor due to interpolated
    // UV roundoff at an exact texel boundary. History retains its capture grid.
    ivec2 at=((2*ivec2(gl_FragCoord.xy)+1)*textureSize(capturedScene,0))/(2*frame.extent);
    vec4 samples[4];samples[0]=texelFetch(capturedScene,at,0);
    for(int i=1;i<4;++i)samples[i]=vec4(0);
    pixel=runCombiners(samples,vec4(0,0,0,1),vec4(0,0,0,1));
}
