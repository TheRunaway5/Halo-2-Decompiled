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
        // Match the surface program's DPH, MUL c10, RCC and MAD c11
        // ordering, including its Z round trip. A matrix multiply or XY/W
        // division can change W after cancellation at campaign coordinates;
        // a one-D24 near-cap difference then becomes a false stencil shadow.
        mat4 rows=transpose(camera.matrix);
        precise vec4 clip;
        for(int axis=0;axis<4;++axis) {
            precise vec3 product=p.xyz*rows[axis].xyz;
            clip[axis]=((product.x+product.y)+product.z)+rows[axis].w;
        }
        precise float reciprocal=sign(clip.w)*clamp(abs(1.0/clip.w),
            5.421010862427522e-20,1.8446744073709552e19);
        precise vec3 scaled=clip.xyz*vec3(camera.viewport.zw*vec2(.5,-.5),16777215.0);
        precise vec2 bias=(camera.viewport.zw*.5+camera.viewport.xy)+vec2(.5);
        precise vec3 screen=scaled*reciprocal+vec3(bias,0);
        screen.xy=trunc(screen.xy*16.0)/16.0;
        clip.w=(clip.w<0.0?-1.0:1.0)*clamp(abs(clip.w),
            5.421010862427522e-20,1.8446744073709552e19);
        vec2 ndc=(screen.xy-camera.viewport.xy)*2.0/camera.viewport.zw-1.0;
        gl_Position=vec4(ndc*clip.w,(screen.z/16777215.0)*clip.w,clip.w);
    }
}
