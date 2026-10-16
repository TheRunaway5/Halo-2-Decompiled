#version 450
layout(location=0) in vec4 point;
layout(location=1) in uint stippleLevel;
layout(push_constant) uniform Camera {
    mat4 matrix;vec4 depthRow;vec4 light;vec4 control;vec4 viewport;
} camera;
layout(location=0) out float sourceDepth;
layout(location=1) flat out uint sourceStipple;
void main() {
    sourceStipple=stippleLevel;
    // May2003 program89/8F: normalize(light-position), then subtract
    // c[-76].w times that direction for each odd/extruded point.
    vec3 direction=camera.light.xyz-point.xyz;
    float squared=dot(direction,direction);
    vec3 position=point.xyz;
    if(point.w!=0.0 && squared>0.0)
        position-=direction*inversesqrt(squared)*camera.light.w;
    vec4 p=vec4(position,1);
    gl_Position=camera.matrix*p;
    sourceDepth=dot(p,camera.depthRow);
    if(camera.control.z!=0.0) {
        // Use the SAME source c0..3, physical12FA0 half-pixel bias and
        // 1/16-pixel snapping as the currently drawn armor. A GLM camera
        // without that bias moves near caps relative to their own surfaces,
        // producing striped false self-shadowing at integer W boundaries.
        vec4 clip=gl_Position;
        vec2 window=(clip.xy/clip.w)*camera.viewport.zw*vec2(.5,-.5)+
            camera.viewport.xy+camera.viewport.zw*.5+vec2(.5);
        window=trunc(window*16.0)/16.0;
        vec2 ndc=(window-camera.viewport.xy)*2.0/camera.viewport.zw-1.0;
        gl_Position=vec4(ndc*clip.w,clip.z,clip.w);
    }
}
