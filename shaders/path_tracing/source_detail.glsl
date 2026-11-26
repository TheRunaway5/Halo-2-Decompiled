#ifndef H2_PATH_TRACING_SOURCE_DETAIL_GLSL
#define H2_PATH_TRACING_SOURCE_DETAIL_GLSL
// Authored surface detail for the full-resolution resolve. Path tracing
// replaces Halo 2's baked lighting, not its surfaces: a pixel keeps the detail
// map, the forward texture pass's own combiner and the bump map's relief.
//
// Requires (world.frag): traceSurfaceTexture0..3 and traceSurfaceSource
// (the draw's own material descriptor), the vertex basis varyings, and the
// GpuMaterial flags written by material_response (scene.cpp).
const uint traceDetailStage1Flag=8192u,traceBumpStage2Flag=16384u,
    traceBumpStage1Flag=32768u;
const uint traceVisorSceneReflectionFlag=33554432u;
// Unlit albedo of the surface, from the source base colour `base` that
// tracePrimarySurface already sampled.
vec3 traceSourceAlbedo(TraceMaterial material,vec2 rawUV,vec3 base) {
    uint flags=material.texture.w;
    if((flags&traceDetailStage1Flag)!=0u) {
        // lightmap_tex_detail modulates the base by its detail map. Apply the
        // detail relative to its own average (the last mip), so the authored
        // fine variation is kept whatever the map's detail function and
        // centre, while the surface's overall albedo stays the base map's.
        vec2 uv=rawUV*traceSurfaceSource.ambientBumpDetail.xy+traceSurfaceSource.ambientBumpDetail.zw;
        vec3 detail=texture(traceSurfaceTexture1,uv).rgb;
        vec3 average=textureLod(traceSurfaceTexture1,uv,16.0).rgb;
        return clamp(base*detail/max(average,vec3(1.0/64.0)),0.0,1.0);
    }
    return base;
}
// One bilinear tap of level `level` of a normal map record (scene.cpp
// append_normal_map), wrapping like the base texture.
vec3 traceNormalMapLevel(uint record,uint level,vec2 uv) {
    uint header=record+1u+level*3u;
    uint start=record+tracePixels[header];uvec2 extent=uvec2(tracePixels[header+1u],tracePixels[header+2u]);
    vec2 point=uv*vec2(extent)-.5;ivec2 at=ivec2(floor(point));vec2 weight=point-vec2(at);
    ivec2 size=ivec2(extent);
    ivec2 a=ivec2((at.x%size.x+size.x)%size.x,(at.y%size.y+size.y)%size.y);
    ivec2 b=ivec2((a.x+1)%size.x,(a.y+1)%size.y);
    vec3 t00=unpackUnorm4x8(tracePixels[start+uint(a.y*size.x+a.x)]).rgb,t10=unpackUnorm4x8(tracePixels[start+uint(a.y*size.x+b.x)]).rgb;
    vec3 t01=unpackUnorm4x8(tracePixels[start+uint(b.y*size.x+a.x)]).rgb,t11=unpackUnorm4x8(tracePixels[start+uint(b.y*size.x+b.x)]).rgb;
    return mix(mix(t00,t10,weight.x),mix(t01,t11,weight.x),weight.y);
}
// The surface's own normal map from the scene atlas (models: their lighting
// pass binds it, not the pass the path tracer draws), in the base texture's
// coordinates, at the mip level its screen footprint selects.
vec3 traceAtlasNormalMap(TraceMaterial material,vec2 rawUV) {
    uint record=material.detail.x,levels=tracePixels[record];
    vec2 uv=rawUV*material.transform.xy+material.transform.zw;
    vec2 size=vec2(tracePixels[record+2u],tracePixels[record+3u]);
    vec2 dx=dFdx(uv)*size,dy=dFdy(uv)*size;
    float footprint=max(dot(dx,dx),dot(dy,dy));
    float level=clamp(.5*log2(max(footprint,1.e-8)),0.0,float(levels-1u));
    uint lower=uint(level);uint upper=min(lower+1u,levels-1u);
    return mix(traceNormalMapLevel(record,lower,uv),traceNormalMapLevel(record,upper,uv),fract(level));
}
// The authored reflection cube map (scene.cpp append_environment_map) along
// a WORLD direction, bilinear within the face it selects (Vulkan's cube face
// convention, which the retail draws sample it with).
vec3 traceEnvironmentFace(uint record,uint face,vec2 uv) {
    uint size=tracePixels[record];uint start=record+1u+face*size*size;
    vec2 point=clamp(uv,0.0,1.0)*float(size)-.5;ivec2 at=ivec2(floor(point));vec2 weight=point-vec2(at);
    ivec2 a=clamp(at,ivec2(0),ivec2(int(size)-1)),b=clamp(at+1,ivec2(0),ivec2(int(size)-1));
    vec3 t00=unpackUnorm4x8(tracePixels[start+uint(a.y)*size+uint(a.x)]).rgb,t10=unpackUnorm4x8(tracePixels[start+uint(a.y)*size+uint(b.x)]).rgb;
    vec3 t01=unpackUnorm4x8(tracePixels[start+uint(b.y)*size+uint(a.x)]).rgb,t11=unpackUnorm4x8(tracePixels[start+uint(b.y)*size+uint(b.x)]).rgb;
    return mix(mix(t00,t10,weight.x),mix(t01,t11,weight.x),weight.y);
}
vec3 traceAtlasEnvironment(TraceMaterial material,vec3 direction) {
    vec3 a=abs(direction);uint face;vec2 st;float major;
    if(a.x>=a.y && a.x>=a.z) {major=a.x;face=direction.x>0.0?0u:1u;st=vec2(direction.x>0.0?-direction.z:direction.z,-direction.y);}
    else if(a.y>=a.z) {major=a.y;face=direction.y>0.0?2u:3u;st=vec2(direction.x,direction.y>0.0?direction.z:-direction.z);}
    else {major=a.z;face=direction.z>0.0?4u:5u;st=vec2(direction.z>0.0?direction.x:-direction.x,-direction.y);}
    vec2 uv=st/max(major,1.e-6)*.5+.5;
    uint record=material.detail.y;
    vec3 sharp=traceEnvironmentFace(record,face,uv);
    // Explicit material finishes can use the authored cube's next mip
    // without changing the reflection color, strength or mask.
    const uint environmentMipFlag=2097152u;
    if(material.detail.w==0u || (material.texture.w&environmentMipFlag)==0u)return sharp;
    float blend=clamp((material.response.x*material.response.x-.04)*5.0,0.0,1.0);
    if((material.texture.w&traceVisorSceneReflectionFlag)!=0u)
        blend=max(blend,0.5);
    if(blend==0.0)return sharp;
    uint size=tracePixels[record];
    return mix(sharp,traceEnvironmentFace(record+1u+6u*size*size,face,uv),blend);
}
vec3 traceAuthoredEnvironment(TraceMaterial material,vec3 position,vec3 normal,vec3 smoothNormal,vec3 geometric,
                             vec3 incoming,float sourceAlpha,vec3 sourceBaked,bool localReflections) {
    if(material.detail.y==0u)return vec3(0);
    // Independent environment passes use the texture pass's destination
    // alpha. The chamber's combined cube also uses that alpha and weights
    // it by its baked light. The original light-angle attenuation is
    // approximated by the path tracer's available RGB irradiance guide.
    float mask=traceSurfaceReflectionMask(material,sourceAlpha);
    const uint chamberBakedReflectionFlag=8388608u;
    if((material.texture.w&chamberBakedReflectionFlag)!=0u)
        mask=clamp(2.0*mask*clamp(material.glancingTint.a*dot(sourceBaked,vec3(1)),0.0,1.0),0.0,1.0);
    bool visor=(material.texture.w&traceVisorSceneReflectionFlag)!=0u;
    // The curved visor keeps a little of the authored relief in the cube.
    // Its scene ray follows the lens curvature instead of the texture grain.
    vec3 reflectionNormal=visor?traceNormalize(mix(smoothNormal,normal,0.35),smoothNormal):normal;
    float strength=traceReflection(material,reflectionNormal,incoming)*mask;
    if(strength<=0.0)return vec3(0);
    vec3 radiance=traceAtlasEnvironment(material,reflect(incoming,reflectionNormal));
    const uint nearbySceneReflectionFlag=4194304u;
    if(localReflections && (material.texture.w&nearbySceneReflectionFlag)!=0u) {
        const float reach=12.0;
        vec3 direction=traceMirrorDirection(visor?smoothNormal:normal,geometric,incoming);
        TraceHit hit;
        if(traceClosest(position+geometric*(traceEpsilon*2.0),direction,reach,hit)) {
            // The cube supplies the room's distant response. Substitute some
            // of it with the nearby hit, inside the same authored mask.
            float gloss=1.0-material.response.x;
            float weight=gloss*(1.0-smoothstep(.4*reach,reach,hit.distance));
            vec3 nearby=traceFiniteMirror(traceReflectedHitRadiance(hit,direction));
            // This one sharp ray is a local accent. Broad finishes keep most
            // of the filtered cube and bright hit radiance keeps its hue.
            float peak=max(nearby.r,max(nearby.g,nearby.b));
            nearby/=max(peak,1.0);
            if(visor) {
                // Keep the source reflection dominant. Unlit hits have no
                // secondary field to substitute, and close hits can be the
                // adjacent helmet shell.
                weight*=peak>0.0?0.25*smoothstep(.05,.4,hit.distance):0.0;
            } else weight*=gloss*gloss;
            radiance=mix(radiance,nearby,weight);
        }
    }
    return radiance*traceReflectionTint(material,reflectionNormal,incoming)*strength;
}
// Samples the authored bump map (expanded to a tangent-space normal).
// Sample before any divergent discard: mip selection uses derivatives.
bool traceSampleBump(TraceMaterial material,vec2 rawUV,TraceObjectSurface objectSurface,out vec3 local) {
    uint flags=material.texture.w;
    vec3 texel;
    if(objectSurface.hasBump)
        texel=objectSurface.bump;
    else if((flags&traceBumpStage2Flag)!=0u)
        texel=texture(traceSurfaceTexture2,rawUV*traceSurfaceSource.bumpTransform.xy+traceSurfaceSource.bumpTransform.zw).rgb;
    else if((flags&traceBumpStage1Flag)!=0u)
        texel=texture(traceSurfaceTexture1,rawUV*traceSurfaceSource.ambientBumpDetail.xy+traceSurfaceSource.ambientBumpDetail.zw).rgb;
    else if(material.detail.x!=0u)texel=traceAtlasNormalMap(material,rawUV);
    else {local=vec3(0,0,1);return false;}
    local=texel*2.0-1.0;
    // Only a tangent-space normal map (unit length, facing out of the surface)
    // is a usable bump; other encodings keep the smooth normal.
    float length2=dot(local,local);
    if(local.z<.2 || length2<.5 || length2>1.7){local=vec3(0,0,1);return false;}
    local*=inversesqrt(length2);
    return true;
}
// Shading normal from that sample. The retail program dots the expanded map
// with the light vector expressed as (binormal, tangent, normal).
vec3 traceBumpedNormal(vec3 local,vec3 normal,vec3 tangent,vec3 binormal) {
    // Orthonormalize the interpolated basis against the (oriented) shading
    // normal, keeping the authored handedness, so the bump always perturbs
    // around the visible surface.
    vec3 t=tangent-normal*dot(normal,tangent);
    float tangentLength=dot(t,t);
    if(!(tangentLength>1.e-10) || !(dot(binormal,binormal)>1.e-10))return normal;
    t*=inversesqrt(tangentLength);
    vec3 b=cross(normal,t);
    if(dot(b,binormal)<0.0)b=-b;
    return traceNormalize(local.x*b+local.y*t+local.z*normal,normal);
}
// Relief of indirect light, as Halo 2 shades its lightmaps: the bumped normal
// against the bake's dominant incident direction, relative to the flat
// normal, weighted by how directional that light was.
float traceIndirectRelief(vec3 normal,vec3 bumped,vec3 incident) {
    float magnitude=length(incident);
    if(!(magnitude>1.e-4))return 1.0;
    vec3 direction=incident/magnitude;
    float relief=max(dot(bumped,direction),0.0)/max(dot(normal,direction),.1);
    return mix(1.0,clamp(relief,0.0,2.0),clamp(magnitude,0.0,1.0));
}
#endif
