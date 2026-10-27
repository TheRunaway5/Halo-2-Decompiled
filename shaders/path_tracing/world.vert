#version 450
layout(location=0) in vec3 position;
layout(location=1) in vec3 color;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec3 normal;
layout(location=4) in uint material;
layout(location=5) in uint sourceStippleLevel;
layout(location=6) in uint lightmapIndex;
layout(location=7) in int bspCluster;
// Reuse the completed per-view/per-owner material bank. These explicit offsets
// match WorldMaterialUniform; no source program or lighting state is replaced.
layout(set=0,binding=4,std140) uniform SourceCamera {
    layout(offset=3392) vec4 world[7];
    layout(offset=3504) vec4 reflection[7];
    layout(offset=3616) vec4 foreground[4];
    layout(offset=3680) uvec4 control;
} sourceCamera;
layout(push_constant) uniform Camera {mat4 viewProjection;vec4 eyeForward;vec4 eyePosition;vec4 viewport;} camera;
layout(location=0) out vec3 traceWorld;
layout(location=1) out vec2 traceTexcoord;
layout(location=2) out vec3 traceWorldNormal;
layout(location=3) flat out uint traceMaterialIndex;
layout(location=4) out vec2 titleDepth;
layout(location=5) flat out uint stippleLevel;
layout(location=6) out vec3 traceTint;
layout(location=8) flat out uint sourceDepthRaw;
void main() {
    gl_Position=camera.viewProjection*vec4(position,1);
    traceWorld=position;traceTexcoord=texcoord;traceWorldNormal=normal;
    uint drawBinding=floatBitsToUint(camera.viewport.w);
    traceMaterialIndex=(drawBinding&0x80000000u)!=0u?drawBinding&0x7fffffffu:material;
    stippleLevel=sourceStippleLevel;
    // In source BSP streams v3 is baked radiance, rather than albedo tint.
    traceTint=lightmapIndex!=0u || bspCluster!=-1?vec3(1):color;
    titleDepth=vec2(dot(position-camera.eyePosition.xyz,camera.eyeForward.xyz)*camera.eyeForward.w/1024.0,camera.eyeForward.w);
    sourceDepthRaw=0u;
    if(sourceCamera.control.x!=0u && camera.eyeForward.w!=0.0) {
        vec4 depthRow=camera.eyePosition.w==3.0?sourceCamera.reflection[3]:
            camera.eyePosition.w==2.0?sourceCamera.foreground[3]:sourceCamera.world[3];
        titleDepth.x=dot(vec4(position,1),depthRow);
        sourceDepthRaw=1u;
    }
}
