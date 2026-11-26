#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#ifdef H2_PATH_TRACING_HARDWARE_RAYS
#extension GL_EXT_ray_query : require
#endif
// This target uses only the secondary-hit material atlas.
#define H2_PATH_TRACING_COMPUTE 1
layout(set=1,binding=6,std430) restrict buffer TraceVisibility {
    uvec4 extent;
    uvec4 temporal;
    uint words[];
} traceVisibility;
layout(push_constant) uniform WaterReflectionView {
    vec4 clipX,clipY,clipW;
    vec4 eyePlane;
    vec4 viewport;
} camera;
layout(location=0) out vec4 reflection;
#include "source_alpha_test.glsl"
#include "path_tracing/scene.glsl"

void main() {
    vec2 ndc=2.0*gl_FragCoord.xy/camera.viewport.xy-1.0;
    // Each NDC component specifies a plane through the camera. Their
    // intersection points along the ray regardless of the clip Z mapping.
    vec3 direction=normalize(cross(camera.clipX.xyz-ndc.x*camera.clipW.xyz,
                                  camera.clipY.xyz-ndc.y*camera.clipW.xyz));
    if(dot(direction,camera.clipW.xyz)<0.0)direction=-direction;
    float height=camera.eyePlane.w,eyeHeight=camera.eyePlane.z-height;
    if(abs(eyeHeight)<=traceEpsilon || direction.z*eyeHeight<=0.0)discard;
    float toPlane=eyeHeight/direction.z;
    float remaining=camera.viewport.z-toPlane;
    if(!(remaining>traceEpsilon))discard;
    // The legacy target's translation is authored for sky. Use the actual
    // reflected submitting eye and start at the water plane so geometry on
    // the other side cannot overwrite the reflected sky or an above-plane hit.
    vec3 origin=vec3(camera.eyePlane.xy+toPlane*direction.xy,height)+traceEpsilon*direction;
    TraceHit hit;
    if(!traceClosest(origin,direction,remaining,hit))discard;
    reflection=vec4(clamp(traceReflectedHitRadiance(hit,direction),0.0,1.0),0.0);
}
