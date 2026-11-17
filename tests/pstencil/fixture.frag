#version 450
#extension GL_GOOGLE_include_directive : require
#include "world_depth_fragment.glsl"
layout(push_constant) uniform Camera {mat4 matrix;vec4 color;vec4 viewport;vec4 rasterOffset;} camera;
layout(location=0) out vec4 pixel;
layout(location=0) in float sourceDepth;
layout(location=1) flat in vec4 window0;
layout(location=2) flat in vec4 window1;
layout(location=3) flat in vec4 window2;
void main(){
    pixel=vec4(camera.color.rgb,1);
    float depth=camera.color.w< -1.5?
        worldWDepth(gl_FragCoord.xy-camera.rasterOffset.xy,vec2(1),window0,window1,window2,0,0,0):sourceDepth;
    gl_FragDepth=camera.color.w<0?worldD24Depth(clamp(depth,0.0,16777215.0)):gl_FragCoord.z;
}
