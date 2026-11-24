#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#ifdef H2_PATH_TRACING_HARDWARE_RAYS
#extension GL_EXT_ray_query : require
#endif
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
layout(location=15) in vec3 traceTangent;
layout(location=16) in vec3 traceBinormal;
layout(location=17) in vec3 traceIncident;
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
#ifdef H2_PATH_TRACING_SOURCE_DEPTH
#include "path_tracing/source_depth.glsl"
#endif
#include "path_tracing/scene.glsl"
#include "path_tracing/lighting_filter.glsl"
#ifndef H2_PATH_TRACING_LIGHTING_WRITE
#include "path_tracing/source_detail.glsl"
#endif
void main() {
    if(stippleLevel<16u && !sourceStipplePass(stippleLevel,31u,uvec2(gl_FragCoord.xy)))discard;
    // Source c3 already carries integer 24-bit W with the actual world/FP
    // distance scale. Its depth must agree with the retained fog/effect passes.
    gl_FragDepth=titleDepth.y!=0.0?worldD24Depth(clamp(sourceDepthRaw!=0u?
        titleDepth.x:titleDepth.x*16777215.0,0.0,16777215.0)):gl_FragCoord.z;
#ifdef H2_PATH_TRACING_SOURCE_DEPTH
    gl_FragDepth=traceSourceDepth();
#endif
    TraceMaterial material=traceMaterials[traceMaterialIndex];
    vec4 sourceColor;if(!tracePrimarySurface(material,traceTexcoord,sourceColor))discard;
    vec3 ownerAmbient=tracePrimarySourceAmbient(material,traceTexcoord);
    if(!traceAlphaPass(material,sourceColor.a))discard;
#ifndef H2_PATH_TRACING_LIGHTING_WRITE
    // Authored detail map / texture-pass combiner and bump map, sampled
    // before the divergent visibility discard below.
    sourceColor.rgb=traceSourceAlbedo(material,traceTexcoord,sourceColor.rgb);
    vec3 bumpLocal;bool bumped=traceSampleBump(material,traceTexcoord,bumpLocal);
#endif
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
    // The retail independent environment pass is masked by the preceding
    // texture pass's destination alpha. Apply it after reconstruction so its
    // full-resolution authored texture detail is never spatially blurred.
    // Halo 2 adds that reflection on top of the whole lit surface (the pass
    // blends additively); it never darkens the surface's colour to make room,
    // so neither does this: shiny painted metal keeps its paint.
    float reflectionMask=(material.texture.w&2048u)!=0u?clamp(sourceColor.a,0.0,1.0):1.0;
    vec4 colorGuide=tracePrimaryColorGuide(material,traceLightmapIndex,traceLightmapUV,traceVertexRadiance);
    bool primaryCachedStatic=tracePrimarySourceAvailable(material,colorGuide);
    vec3 sourceBaseline=primaryCachedStatic?traceSourceIndirect(colorGuide):vec3(0);
    sourceBaseline+=ownerAmbient;
    uvec2 at=uvec2(gl_FragCoord.xy);uint phase=uint(camera.viewport.z);
    vec3 diffuseLight,specularLight;
#ifndef H2_PATH_TRACING_LIGHTING_WRITE
    // Resolve: upsample the denoised grid. Only a pixel with no surface cell
    // anywhere in its 5x5 neighbourhood (an isolated sliver) traces its own
    // path below instead.
    vec3 tracedIncident;
    if(phase==3u && traceLightingFilter(at,traceWorld,normal,traceMaterialIndex,material.response.x,
                                      max(length(dx),length(dy)),diffuseLight,specularLight,tracedIncident)) {
        // Sun light is deterministic: evaluate it per pixel, with its own
        // shadow ray and the bump-mapped normal, instead of reading it from
        // the denoised grid. Indirect light keeps Halo 2's baked relief.
        vec3 shading=bumped?traceBumpedNormal(bumpLocal,normal,traceTangent,traceBinormal):normal;
        // Bumps answer to the bake's dominant light direction where Halo 2
        // stored one (lightmapped world geometry; a model's vertex stream may
        // hold other data in that slot), otherwise to the direction the
        // traced light arrived from (its length is how directional it is).
        vec3 lightIncident=traceLightmapIndex!=0u && dot(traceIncident,traceIncident)>1.e-8?traceIncident:
            bumped?traceFilteredIncident(at,traceWorld,normal,max(length(dx),length(dy))):vec3(0);
        float relief=bumped?traceIndirectRelief(normal,shading,lightIncident):1.0;
        // The legacy reflection response is a smooth-surface adapter: keep its
        // highlight on the interpolated normal (a per-texel normal at grazing
        // view angles turns it into a saturated rim).
        vec3 sunDiffuse,sunSpecular;
        traceResolveDistantLights(traceWorld,shading,normal,geometric,incoming,material,colorGuide,primaryCachedStatic,
                                  sunDiffuse,sunSpecular);
        // Calibrated against the baked lightmaps (calibrate.comp): x scales
        // the sun, y the denoised grid diffuse.
        diffuseLight=diffuseLight*(traceScene.calibration.y*relief)+sunDiffuse*traceScene.calibration.x;
        float sharpness=material.detail.y!=0u?0.0:traceMirrorSharpness(material);
        if(material.detail.y!=0u) {
            // Halo 2's environment pass: its authored cube map along the
            // bump-mapped reflection, tinted and weighted as that pass is.
            // The grid then holds only this surface's direct highlights
            // (traceLighting does not trace its reflection lobe).
            float mask=(material.texture.w&1048576u)!=0u?clamp(sourceColor.a,0.0,1.0):1.0;
            specularLight+=traceAtlasEnvironment(material.detail.y,reflect(incoming,shading))*
                traceReflectionTint(material,shading,incoming)*traceReflection(material,shading,incoming)*mask;
        }
#ifdef H2_PATH_TRACING_HARDWARE_RAYS
        // Polished surfaces without an authored environment reflect along
        // their own bump-mapped direction at every pixel instead of the
        // grid's half-resolution average.
        else if(sharpness>0.0)
            specularLight=mix(specularLight,traceMirrorReflection(traceWorld,shading,geometric,incoming,material),sharpness);
#endif
        specularLight+=sunSpecular*traceScene.calibration.x;
        vec3 radiance=albedo*(sourceBaseline+diffuseLight)+specularLight*reflectionMask;
        if((material.texture.w&32u)!=0u)radiance+=traceEmission(material,traceTexcoord,sourceTint);
        pixel=vec4(clamp(traceFiniteLighting(radiance),vec3(0),vec3(1)),sourceColor.a);return;
    }
#endif
#ifdef H2_PATH_TRACING_LIGHTING_WRITE
    // The lighting pass only publishes the visible surface of each grid cell.
    // trace.comp traces it with full SIMD occupancy and stores its history.
    if(phase!=2u || !traceLightingRepresentative(at))discard;
    uint record=traceLightingRecord(at/max(traceVisibility.extent.w,1u));
    if(!traceLightingClaim(record,traceMaterialIndex))discard;
    traceLightingRequest(record,at,traceWorld,normal,geometric,colorGuide,traceMaterialIndex,material.response.x,
        max(length(dx),length(dy)),max(material.response.y,material.response.z)>0.0);
#else
    uint frameSeed=phase>=2u?traceHash(traceVisibility.temporal.x+0x93d765ddu):0u;
    uint state=traceHash(uint(gl_FragCoord.x)+uint(gl_FragCoord.y)*65537u+traceMaterialIndex*8191u+frameSeed);
    diffuseLight=vec3(0);specularLight=vec3(0);vec3 currentDirect=vec3(0),currentDirectSpecular=vec3(0);
    const uint samples=uint(clamp(camera.viewport.x,1.0,2.0));
    [[dont_unroll]] for(uint sampleIndex=0u;sampleIndex<samples;++sampleIndex) {
        vec3 diffuseSample,specularSample,directSample,directSpecularSample,incidentSample;
        traceLighting(traceWorld,normal,geometric,incoming,material,albedo,colorGuide,primaryCachedStatic,state,diffuseSample,specularSample,directSample,directSpecularSample,incidentSample);
        diffuseLight+=diffuseSample;specularLight+=specularSample;
        currentDirect+=directSample;
        currentDirectSpecular+=directSpecularSample;
    }
    diffuseLight/=float(samples);specularLight/=float(samples);currentDirect/=float(samples);
    currentDirectSpecular/=float(samples);
    // Thin silhouettes with no compatible grid sample retain a single normal
    // bounded trace. The ordinary resolve path performs no ray queries.
    vec3 radiance=albedo*(sourceBaseline+diffuseLight*traceScene.calibration.y+currentDirect*traceScene.calibration.x)+
        (specularLight+currentDirectSpecular*traceScene.calibration.x)*reflectionMask;
    if((material.texture.w&32u)!=0u)radiance+=traceEmission(material,traceTexcoord,sourceTint);
    if(any(isnan(radiance)) || any(isinf(radiance)))radiance=vec3(0);
    // Same scene transfer as Original: no automatic white clear-coat or global
    // exposure/tonemapping change accompanies the optional lighting mode.
    pixel=vec4(clamp(radiance,vec3(0),vec3(1)),sourceColor.a);
#endif
}
