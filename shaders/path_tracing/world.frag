#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
layout(location=0) in vec3 traceWorld;
layout(location=1) in vec2 traceTexcoord;
layout(location=2) in vec3 traceWorldNormal;
layout(location=3) flat in uint traceMaterialIndex;
layout(location=4) in vec2 titleDepth;
layout(location=5) flat in uint stippleLevel;
layout(location=6) in vec3 traceTint;
layout(location=8) flat in uint sourceDepthRaw;
layout(location=9) in vec2 traceLightmapUV;
layout(location=10) flat in uint traceLightmapIndex;
layout(location=11) in vec3 traceVertexRadiance;
layout(set=1,binding=6,std430) restrict buffer TraceVisibility {
    uvec4 extent;
    uvec4 temporal;
    uint words[];
} traceVisibility;
layout(push_constant) uniform Camera {mat4 viewProjection;vec4 eyeForward;vec4 eyePosition;vec4 viewport;} camera;
#ifdef H2_PATH_TRACING_LIGHTING_WRITE
// The intermediate lighting role writes only its HDR storage records.
vec4 pixel;
#else
layout(location=0) out vec4 pixel;
#endif
#include "source_alpha_test.glsl"
#include "source_stipple.glsl"
#include "world_depth_fragment.glsl"
#include "path_tracing/scene.glsl"
#include "path_tracing/lighting_filter.glsl"
void main() {
    if(stippleLevel<16u && !sourceStipplePass(stippleLevel,31u,uvec2(gl_FragCoord.xy)))discard;
    // Source c3 already carries integer 24-bit W with the actual world/FP
    // distance scale. Its depth must agree with the retained fog/effect passes.
    gl_FragDepth=titleDepth.y!=0.0?worldD24Depth(clamp(sourceDepthRaw!=0u?
        titleDepth.x:titleDepth.x*16777215.0,0.0,16777215.0)):gl_FragCoord.z;
    TraceMaterial material=traceMaterials[traceMaterialIndex];
    vec4 sourceColor;if(!tracePrimarySurface(material,traceTexcoord,sourceColor))discard;
    if(!traceAlphaPass(material,sourceColor.a))discard;
    vec3 dx=dFdx(traceWorld),dy=dFdy(traceWorld);
    vec3 geometric=traceNormalize(cross(dx,dy),vec3(0,0,1));
    vec3 normal=traceNormalize(traceWorldNormal,geometric);
    // Source W differs from hardware projection Z. Compare against the cheap
    // exact-W prepass instead of enabling incorrect hardware early-Z tests.
    // Keep implicit texture/normal derivatives above this divergent discard.
    if(camera.viewport.z!=0.0) {
        uvec2 at=uvec2(gl_FragCoord.xy);
        if(any(greaterThanEqual(at,traceVisibility.extent.xy)))discard;
        if(gl_FragDepth>uintBitsToFloat(traceVisibility.words[at.y*traceVisibility.extent.x+at.x]))discard;
    }
    vec3 incoming=traceNormalize(traceWorld-camera.eyePosition.xyz,-normal);
    // Orient the surface from its actual face. A smooth authored normal can
    // cross the view horizon at grazing angles without becoming a back face;
    // flipping it toward the eye sends its whole light hemisphere underground.
    if(dot(geometric,incoming)>0.0)geometric=-geometric;
    if(dot(normal,geometric)<0.0)normal=-normal;
    vec3 sourceTint=(material.texture.w&128u)!=0u?vec3(1):traceTint;
    vec3 albedo=(material.texture.w&32u)!=0u?vec3(0):clamp(sourceColor.rgb*sourceTint,0.0,1.0);
    uvec2 at=uvec2(gl_FragCoord.xy);uint phase=uint(camera.viewport.z);
    vec3 diffuseLight,specularLight;
    if(phase==3u && traceLightingFilter(at,traceWorld,normal,traceMaterialIndex,material.response.x,
                                      max(length(dx),length(dy)),diffuseLight,specularLight)) {
        vec3 radiance=albedo*diffuseLight+specularLight;
        if((material.texture.w&32u)!=0u)radiance+=traceEmission(material,traceTexcoord,sourceTint);
        pixel=vec4(clamp(traceFiniteLighting(radiance),vec3(0),vec3(1)),sourceColor.a);return;
    }
    uint record=0u;
    if(phase==2u) {
        if(!traceLightingRepresentative(at))discard;
        record=traceLightingRecord(at/max(traceVisibility.extent.w,1u));
        if(!traceLightingClaim(record,traceMaterialIndex))discard;
    }
    vec4 colorGuide=tracePrimaryColorGuide(material,traceLightmapIndex,traceLightmapUV,traceVertexRadiance);
    uint frameSeed=phase>=2u?traceHash(traceVisibility.temporal.x+0x93d765ddu):0u;
    uint state=traceHash(uint(gl_FragCoord.x)+uint(gl_FragCoord.y)*65537u+traceMaterialIndex*8191u+frameSeed);
    diffuseLight=vec3(0);specularLight=vec3(0);vec3 currentDirect=vec3(0);
    const uint samples=uint(clamp(camera.viewport.x,1.0,2.0));
    [[dont_unroll]] for(uint sampleIndex=0u;sampleIndex<samples;++sampleIndex) {
        vec3 diffuseSample,specularSample,directSample;
        traceLighting(traceWorld,normal,geometric,incoming,material,albedo,colorGuide,state,diffuseSample,specularSample,directSample);
        diffuseLight+=diffuseSample;specularLight+=specularSample;
        currentDirect+=directSample;
    }
    diffuseLight/=float(samples);specularLight/=float(samples);currentDirect/=float(samples);
    if(phase==2u) {
        traceLightingStore(record,diffuseLight,specularLight,currentDirect,traceWorld,normal,geometric,incoming,
                           traceMaterialIndex,material.response.x,max(length(dx),length(dy)),
                           max(material.response.y,material.response.z)>0.0);
        return;
    }
    // Thin silhouettes with no compatible grid sample retain a single normal
    // bounded trace. The ordinary resolve path performs no ray queries.
    vec3 radiance=albedo*(diffuseLight+currentDirect)+specularLight;
    if((material.texture.w&32u)!=0u)radiance+=traceEmission(material,traceTexcoord,sourceTint);
    if(any(isnan(radiance)) || any(isinf(radiance)))radiance=vec3(0);
    // Same scene transfer as Original: no automatic white clear-coat or global
    // exposure/tonemapping change accompanies the optional lighting mode.
    pixel=vec4(clamp(radiance,vec3(0),vec3(1)),sourceColor.a);
}
