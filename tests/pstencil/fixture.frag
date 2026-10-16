#version 450
#extension GL_GOOGLE_include_directive : require
#include "world_depth_fragment.glsl"
layout(push_constant) uniform Camera {mat4 matrix;vec4 color;} camera;
layout(location=0) out vec4 pixel;
layout(location=0) in float sourceDepth;
void main(){pixel=vec4(camera.color.rgb,1);gl_FragDepth=camera.color.w<0?worldD24Depth(clamp(sourceDepth,0.0,16777215.0)):gl_FragCoord.z;}
