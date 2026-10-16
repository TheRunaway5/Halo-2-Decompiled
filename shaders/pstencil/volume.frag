#version 450
#extension GL_GOOGLE_include_directive : require
#include "world_depth_fragment.glsl"
#include "source_stipple.glsl"
layout(push_constant) uniform Camera {
    mat4 matrix;vec4 depthRow;vec4 light;vec4 control;vec4 viewport;
} camera;
layout(location=0) in float sourceDepth;
layout(location=1) flat in uint sourceStipple;
void main() {
    // May2003 partial shadow opacity is NV097 polygon stipple, not blending.
    if(!sourceStipplePass(sourceStipple,31u,uvec2(gl_FragCoord.xy)))discard;
    gl_FragDepth=camera.control.x!=0.0?
        worldD24Depth(clamp(sourceDepth*camera.control.y,0.0,16777215.0)):gl_FragCoord.z;
}
