#version 450
#extension GL_GOOGLE_include_directive : require
#include "world_depth_fragment.glsl"
#include "source_stipple.glsl"
layout(push_constant) uniform Camera {
    mat4 matrix;vec4 depthRow;vec4 light;vec4 control;vec4 viewport;
} camera;
layout(location=0) in float sourceDepth;
layout(location=1) flat in uint sourceStipple;
layout(location=2) flat in vec4 sourceWindow0;
layout(location=3) flat in vec4 sourceWindow1;
layout(location=4) flat in vec4 sourceWindow2;
void main() {
    // May2003 partial shadow opacity is NV097 polygon stipple, not blending.
    if(!sourceStipplePass(sourceStipple,31u,uvec2(gl_FragCoord.xy)))discard;
    // control.w is zero for volume caps. Only the world-composite owner
    // exclusion uses an explicit bias. Coincident caps use the native
    // surface's ordered W reconstruction so LESS consistently fails there.
    float depth=sourceDepth;
    if(camera.control.x!=0.0 && camera.control.z!=0.0)
        depth=worldWDepth(gl_FragCoord.xy-camera.depthRow.xy,vec2(1),
            sourceWindow0,sourceWindow1,sourceWindow2,0,0,0);
    gl_FragDepth=camera.control.x!=0.0?
        worldD24Depth(clamp(depth*camera.control.y+camera.control.w,0.0,16777215.0)):
        clamp(gl_FragCoord.z+camera.control.w/16777215.0,0.0,1.0);
}
