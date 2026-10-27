#version 450
// Test-only capture after the unchanged production native vertex module.
// Rasterizer discard is enabled; these stores are component observations.
layout(points) in;
layout(points,max_vertices=1) out;
layout(location=4) in vec4 lightingDirection[];
layout(location=5) in vec4 eyeDirection[];
layout(location=8) in vec4 stageCoordinates[][4];
layout(location=14) flat in vec4 sourceEmblemWindow[];
layout(set=1,binding=0,std430) buffer Captured {vec4 values[];} captured;
void main() {
    uint base=uint(gl_PrimitiveIDIn)*8u;
    captured.values[base]=gl_in[0].gl_Position;
    captured.values[base+1u]=eyeDirection[0];
    captured.values[base+2u]=lightingDirection[0];
    for(uint stage=0u;stage<4u;++stage)
        captured.values[base+3u+stage]=stageCoordinates[0][stage];
    captured.values[base+7u]=sourceEmblemWindow[0];
}
