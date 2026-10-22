#version 450
layout(push_constant) uniform Input {uint width;uint base;} inputData;
layout(location=0) out vec4 color;
void main() {
    uint n=(inputData.base+uint(gl_FragCoord.y)*inputData.width+uint(gl_FragCoord.x))^0x00ffffffu;
    uint s=(n^(n>>8u))&255u;
    color=vec4(uvec4((n>>8u)&255u,n&255u,s,n>>16u))/255.0;
}
