#ifndef H2_PATH_TRACING_SCENE_GLSL
#define H2_PATH_TRACING_SCENE_GLSL
struct TraceNode {vec4 minimum,maximum;uvec4 children;};
struct TraceTriangle {vec4 p0,p1,p2,n0,n1,n2,uv01,uv2;uvec4 metadata;vec4 colorGuide;};
struct TraceMaterial {vec4 base,emission,response;uvec4 texture;vec4 transform;uvec4 colorGuide;vec4 glancingTint;uvec4 detail;};
struct TraceLight {vec4 position_radius,color_intensity;vec4 planes[6];};
layout(set=1,binding=0,std430) readonly restrict buffer TraceNodes {TraceNode traceNodes[];};
layout(set=1,binding=1,std430) readonly restrict buffer TraceTriangles {TraceTriangle traceTriangles[];};
layout(set=1,binding=2,std430) readonly restrict buffer TraceMaterials {TraceMaterial traceMaterials[];};
layout(set=1,binding=3,std430) readonly restrict buffer TraceLights {TraceLight traceLights[];};
layout(set=1,binding=4,std430) readonly restrict buffer TracePixels {uint tracePixels[];};
layout(set=1,binding=5,std140) uniform TraceScene {uvec4 counts;vec4 environment;vec4 calibration;} traceScene;
// Captured through the ordinary source sky renderer, in the WORLD cube basis.
// Its valid fallback is the current authored environment, including black.
layout(set=1,binding=7) uniform samplerCube traceSky;
// The sky cube carries a mip chain (64 texels per face at mip 0). Diffuse
// transport integrates the sky over a hemisphere, so it reads a blurred mip:
// single bright texels (stars, the sun disc, cloud edges) otherwise become
// rare, very bright samples that no denoiser can average away. The mean
// radiance is unchanged; only its angular detail is removed. Reflections read
// a mip that matches their GGX lobe width.
const float traceSkyDiffuseLod=4.0;
vec3 traceSkyDiffuse(vec3 direction) {return max(textureLod(traceSky,direction,traceSkyDiffuseLod).rgb,vec3(0));}
vec3 traceSkySpecular(vec3 direction,float roughness) {
    return max(textureLod(traceSky,direction,clamp(roughness*8.0,0.0,traceSkyDiffuseLod)).rgb,vec3(0));
}
const float tracePi=3.14159265358979323846;
// Light-only emitters (glows: teleporter fields, lifts) never occlude.
const uint traceLightOnlyFlag=131072u,traceUniformEmissionFlag=262144u;
// Emitter table entry (Scene::pixels): the triangle's light is in the bake.
const uint traceEmitterBakedBit=0x80000000u;
const uint traceEmitterGroupMagic=0x45525447u;
const uint traceStaticOccluderMask=0x01u,traceLightMask=0x02u,traceDynamicOccluderMask=0x04u;
const uint traceOccluderMask=traceStaticOccluderMask|traceDynamicOccluderMask;
const float traceEpsilon=0.0002;
uint traceHash(uint v) {v^=v>>16u;v*=0x7feb352du;v^=v>>15u;v*=0x846ca68bu;return v^(v>>16u);}
float traceRandom(inout uint state) {state=traceHash(state+0x9e3779b9u);return float(state>>8u)*(1.0/16777216.0);}
vec3 traceNormalize(vec3 value,vec3 fallback) {
    float squared=dot(value,value);return squared>1.e-18 && !isinf(squared)?value*inversesqrt(squared):fallback;
}
vec2 traceUV(TraceTriangle triangle,vec3 barycentric) {
    return triangle.uv01.xy*barycentric.x+triangle.uv01.zw*barycentric.y+triangle.uv2.xy*barycentric.z;
}
vec4 traceTexture(TraceMaterial material,vec2 uv) {
    uv=uv*material.transform.xy+material.transform.zw;
    uv.x=(material.texture.w&2u)!=0u?clamp(uv.x,0.0,1.0):fract(uv.x);
    uv.y=(material.texture.w&4u)!=0u?clamp(uv.y,0.0,1.0):fract(uv.y);
    uvec2 extent=max(material.texture.yz,uvec2(1));
    uvec2 at=min(uvec2(uv*vec2(extent)),extent-1u);
    return unpackUnorm4x8(tracePixels[material.texture.x+at.y*extent.x+at.x]);
}
vec3 traceVertexColor(TraceTriangle triangle,vec3 barycentric) {
    return unpackUnorm4x8(triangle.metadata.y).rgb*barycentric.x+
        unpackUnorm4x8(triangle.metadata.z).rgb*barycentric.y+
        unpackUnorm4x8(triangle.metadata.w).rgb*barycentric.z;
}
bool traceAlphaPass(TraceMaterial material,float alpha) {
    // Qualified surfaces already applied every raw stage alpha-kill before
    // evaluating native output alpha (which can legitimately be zero).
    if((material.texture.w&64u)==0u && (material.texture.w&8u)!=0u && alpha==0.0)return false;
    if((material.texture.w&1u)==0u)return true;
    return sourceAlphaTestPass(alpha,0x200u+((material.texture.w>>8u)&7u),uint(round(material.response.w*255.0)));
}
#include "path_tracing/surface.glsl"
#include "path_tracing/emission.glsl"
#include "path_tracing/color_guide.glsl"
#include "path_tracing/source_ambient.glsl"
struct TraceHit {float distance;uint triangle;vec3 barycentric;};
vec3 traceInverseDirection(vec3 direction) {
    return vec3(abs(direction.x)<1.e-20?0.0:1.0/direction.x,
                abs(direction.y)<1.e-20?0.0:1.0/direction.y,
                abs(direction.z)<1.e-20?0.0:1.0/direction.z);
}
float traceBoxNear(TraceNode node,vec3 origin,vec3 direction,vec3 inverseDirection,float maximum) {
    float near=0.0,far=maximum;
    // Reciprocals are shared by the whole ray. Widen only the broad phase by
    // a few coordinate ULPs so its multiplication cannot reject a grazing
    // triangle accepted by the unchanged narrow-phase intersection below.
    vec3 padding=max(vec3(1.e-6),max(abs(node.minimum.xyz),abs(node.maximum.xyz))*2.e-7);
    for(uint axis=0u;axis<3u;++axis) {
        float minimum=node.minimum[axis]-padding[axis],maximumBound=node.maximum[axis]+padding[axis];
        if(abs(direction[axis])<1.e-20) {
            if(origin[axis]<minimum || origin[axis]>maximumBound)return -1.0;
        }else {
            float a=(minimum-origin[axis])*inverseDirection[axis];
            float b=(maximumBound-origin[axis])*inverseDirection[axis];
            near=max(near,min(a,b));far=min(far,max(a,b));if(far<near)return -1.0;
        }
    }
    return near;
}
bool traceTriangleHit(uint index,vec3 origin,vec3 direction,float maximum,bool occlusion,out TraceHit hit) {
    // Normal/color/UV lanes are not needed for opaque visibility. Do not load
    // the full nine-lane record for every candidate traversed by every ray.
    // Ray-aligned edge tests (Woop/Benthin/Wald, JCGT2013) avoid subtracting
    // nearly equal long edge products on native needle-shaped BSP triangles.
    // Keep the same float32 operation order as the CPU validation path.
    uint kz=abs(direction.x)>abs(direction.y)?0u:1u;
    if(abs(direction.z)>abs(direction[kz]))kz=2u;
    if(direction[kz]==0.0)return false;
    uint kx=(kz+1u)%3u,ky=(kx+1u)%3u;
    if(direction[kz]<0.0){uint temporary=kx;kx=ky;ky=temporary;}
    precise float inverseZ=1.0/direction[kz];
    precise float sx=direction[kx]*inverseZ,sy=direction[ky]*inverseZ;
    precise vec3 a=traceTriangles[index].p0.xyz-origin;
    precise vec3 b=traceTriangles[index].p1.xyz-origin;
    precise vec3 c=traceTriangles[index].p2.xyz-origin;
    precise float ax=a[kx]-sx*a[kz],ay=a[ky]-sy*a[kz];
    precise float bx=b[kx]-sx*b[kz],by=b[ky]-sy*b[kz];
    precise float cx=c[kx]-sx*c[kz],cy=c[ky]-sy*c[kz];
    precise float u=cx*by-cy*bx,v=ax*cy-ay*cx,w=bx*ay-by*ax;
    if(min(u,min(v,w))<0.0 && max(u,max(v,w))>0.0)return false;
    precise float determinant=(u+v)+w;
    if(determinant==0.0)return false;
    precise float az=a[kz]*inverseZ,bz=b[kz]*inverseZ,cz=c[kz]*inverseZ;
    // Anchor depth interpolation so common-depth faces retain that exact
    // depth instead of rounding a weighted sum of the same plane distance.
    precise float distance=az+(v*(bz-az)+w*(cz-az))/determinant;
    if(!(distance>traceEpsilon*.5 && distance<maximum))return false;
    precise vec3 barycentric=vec3(u,v,w)/determinant;
    uint materialIndex=traceTriangles[index].metadata.x;
    uint materialFlags=traceMaterials[materialIndex].texture.w;
    if(occlusion && (materialFlags&traceLightOnlyFlag)!=0u)return false;
    // Function arguments are evaluated before traceAlphaPass. Guard the
    // lookup here: ordinary opaque triangles require no alpha texel at all.
    if((materialFlags&9u)!=0u) {
        TraceMaterial material=traceMaterials[materialIndex];
        vec4 uv01=traceTriangles[index].uv01;vec2 uv2=traceTriangles[index].uv2.xy;
        vec2 uv=uv01.xy*barycentric.x+uv01.zw*barycentric.y+uv2*barycentric.z;
        float alpha;
        if(!traceAtlasCoverage(material,uv,alpha) || !traceAlphaPass(material,alpha))return false;
    }
    hit=TraceHit(distance,index,barycentric);return true;
}
#include "path_tracing/surface_ray.glsl"
#ifdef H2_PATH_TRACING_HARDWARE_RAYS
// Hardware traversal through Vulkan acceleration structures. The TLAS holds at
// most four instances: static/dynamic geometry, each split into opaque and
// alpha-tested triangles. Opaque instances never return to the shader; alpha
// candidates run the same authored coverage test as the software traversal.
// An instance's custom index is its first entry in the ray index list, whose
// three vertex indices per triangle address the triangle bank's position lanes
// (ten 16-byte lanes per GpuTriangle), so index/10 recovers the triangle.
layout(set=1,binding=11) uniform accelerationStructureEXT traceAcceleration;
layout(set=1,binding=12,std430) readonly restrict buffer TraceRayIndices {uint traceRayIndices[];};
bool traceCandidateCoverage(uint index,vec2 hitBarycentric) {
    vec3 barycentric=vec3(1.0-hitBarycentric.x-hitBarycentric.y,hitBarycentric);
    TraceMaterial material=traceMaterials[traceTriangles[index].metadata.x];
    vec4 uv01=traceTriangles[index].uv01;vec2 uv2=traceTriangles[index].uv2.xy;
    vec2 uv=uv01.xy*barycentric.x+uv01.zw*barycentric.y+uv2*barycentric.z;
    float alpha;
    return traceAtlasCoverage(material,uv,alpha) && traceAlphaPass(material,alpha);
}
bool traceClosest(vec3 origin,vec3 direction,float maximum,out TraceHit hit,
                  vec3 surfacePosition,vec3 geometricNormal,bool smoothTerminator,uint sourceTriangle) {
    hit.distance=maximum;hit.triangle=0u;hit.barycentric=vec3(0);
    if(traceScene.counts.y==0u)return false;
    rayQueryEXT query;
    rayQueryInitializeEXT(query,traceAcceleration,gl_RayFlagsNoneEXT,0xffu,origin,traceEpsilon*.5,direction,maximum);
    while(rayQueryProceedEXT(query)) {
        if(rayQueryGetIntersectionTypeEXT(query,false)!=gl_RayQueryCandidateIntersectionTriangleEXT)continue;
        uint entry=uint(rayQueryGetIntersectionInstanceCustomIndexEXT(query,false))+
            uint(rayQueryGetIntersectionPrimitiveIndexEXT(query,false));
        uint index=traceRayIndices[entry*3u]/10u;
        if(index==sourceTriangle && smoothTerminator)continue;
        if(traceCandidateCoverage(index,rayQueryGetIntersectionBarycentricsEXT(query,false)))
            rayQueryConfirmIntersectionEXT(query);
    }
    if(rayQueryGetIntersectionTypeEXT(query,true)!=gl_RayQueryCommittedIntersectionTriangleEXT)return false;
    uint entry=uint(rayQueryGetIntersectionInstanceCustomIndexEXT(query,true))+
        uint(rayQueryGetIntersectionPrimitiveIndexEXT(query,true));
    vec2 barycentric=rayQueryGetIntersectionBarycentricsEXT(query,true);
    hit.distance=rayQueryGetIntersectionTEXT(query,true);
    hit.triangle=traceRayIndices[entry*3u]/10u;
    hit.barycentric=vec3(1.0-barycentric.x-barycentric.y,barycentric);
    return true;
}
bool traceOccludedBy(uint mask,vec3 origin,vec3 direction,float maximum,bool smoothTerminator,uint sourceTriangle) {
    if(traceScene.counts.y==0u)return false;
    rayQueryEXT query;
    rayQueryInitializeEXT(query,traceAcceleration,gl_RayFlagsTerminateOnFirstHitEXT,mask,
        origin,traceEpsilon*.5,direction,maximum);
    while(rayQueryProceedEXT(query)) {
        if(rayQueryGetIntersectionTypeEXT(query,false)!=gl_RayQueryCandidateIntersectionTriangleEXT)continue;
        uint entry=uint(rayQueryGetIntersectionInstanceCustomIndexEXT(query,false))+
            uint(rayQueryGetIntersectionPrimitiveIndexEXT(query,false));
        uint index=traceRayIndices[entry*3u]/10u;
        if(index==sourceTriangle && smoothTerminator)continue;
        if(traceCandidateCoverage(index,rayQueryGetIntersectionBarycentricsEXT(query,false)))
            rayQueryConfirmIntersectionEXT(query);
    }
    return rayQueryGetIntersectionTypeEXT(query,true)!=gl_RayQueryCommittedIntersectionNoneEXT;
}
bool traceOccluded(vec3 origin,vec3 direction,float maximum,
                   vec3 surfacePosition,vec3 geometricNormal,bool smoothTerminator,uint sourceTriangle) {
    return traceOccludedBy(traceOccluderMask,origin,direction,maximum,smoothTerminator,sourceTriangle);
}
#else
bool traceClosest(vec3 origin,vec3 direction,float maximum,out TraceHit hit,
                  vec3 surfacePosition,vec3 geometricNormal,bool smoothTerminator,uint sourceTriangle) {
    if(traceScene.counts.x==0u)return false;
    bool excludeStartingSurface=smoothTerminator;
    vec3 inverseDirection=traceInverseDirection(direction);
    float rootNear=traceBoxNear(traceNodes[0],origin,direction,inverseDirection,maximum);
    if(rootNear<0.0)return false;
    // CPU construction stops at depth30, so at most31 deferred nodes exist.
    // Entry distances let a nearer accepted hit prune deferred far children
    // without retesting their boxes or loading their triangle records.
    uint stack[32];float entries[32];uint size=1u;stack[0]=0u;entries[0]=rootNear;
    bool found=false;hit.distance=maximum;
    while(size>0u) {
        --size;if(entries[size]>hit.distance)continue;
        TraceNode node=traceNodes[stack[size]];
        if(node.children.z==0u) {
            uint left=node.children.x,right=node.children.y;
            float a=traceBoxNear(traceNodes[left],origin,direction,inverseDirection,hit.distance);
            float b=traceBoxNear(traceNodes[right],origin,direction,inverseDirection,hit.distance);
            if(a>=0.0 && b>=0.0) {
                if(a>b) {uint index=left;left=right;right=index;float distance=a;a=b;b=distance;}
                stack[size]=right;entries[size++]=b;
                stack[size]=left;entries[size++]=a;
            }else if(a>=0.0) {stack[size]=left;entries[size++]=a;}
            else if(b>=0.0) {stack[size]=right;entries[size++]=b;}
            continue;
        }
        for(uint n=0u;n<node.children.y;++n) {
            TraceHit candidate;
            uint index=node.children.x+n;
            if(traceTriangleHit(index,origin,direction,hit.distance,false,candidate)) {
                if(excludeStartingSurface && traceSkipStartingSurface(index,surfacePosition,geometricNormal,direction,sourceTriangle))continue;
                hit=candidate;found=true;
            }
        }
    }
    return found;
}
bool traceOccluded(vec3 origin,vec3 direction,float maximum,
                   vec3 surfacePosition,vec3 geometricNormal,bool smoothTerminator,uint sourceTriangle) {
    if(traceScene.counts.x==0u)return false;
    bool excludeStartingSurface=smoothTerminator;
    vec3 inverseDirection=traceInverseDirection(direction);
    uint stack[32];uint size=1u;stack[0]=0u;
    while(size>0u) {
        TraceNode node=traceNodes[stack[--size]];
        if(traceBoxNear(node,origin,direction,inverseDirection,maximum)<0.0)continue;
        if(node.children.z==0u) {stack[size++]=node.children.x;stack[size++]=node.children.y;continue;}
        for(uint n=0u;n<node.children.y;++n) {
            TraceHit hit;
            uint index=node.children.x+n;
            if(traceTriangleHit(index,origin,direction,maximum,true,hit)) {
                if(excludeStartingSurface && traceSkipStartingSurface(index,surfacePosition,geometricNormal,direction,sourceTriangle))continue;
                return true;
            }
        }
    }
    return false;
}
#endif
bool traceClosest(vec3 origin,vec3 direction,float maximum,out TraceHit hit) {
    return traceClosest(origin,direction,maximum,hit,vec3(0),vec3(0),false,0xffffffffu);
}
bool traceOccluded(vec3 origin,vec3 direction,float maximum) {
    return traceOccluded(origin,direction,maximum,vec3(0),vec3(0),false,0xffffffffu);
}
vec3 traceNormal(TraceTriangle triangle,vec3 barycentric,vec3 incoming) {
    vec3 geometric=traceNormalize(cross(triangle.p1.xyz-triangle.p0.xyz,triangle.p2.xyz-triangle.p0.xyz),vec3(0,0,1));
    vec3 normal=traceNormalize(triangle.n0.xyz*barycentric.x+triangle.n1.xyz*barycentric.y+triangle.n2.xyz*barycentric.z,geometric);
    // Secondary hits retain the same geometric-facing convention as primary
    // surfaces, including smooth normals just beyond the view horizon.
    if(dot(geometric,incoming)>0.0)geometric=-geometric;
    return dot(normal,geometric)<0.0?-normal:normal;
}
vec3 traceHemisphere(vec3 normal,inout uint state) {
    float a=traceRandom(state),b=traceRandom(state),r=sqrt(a),angle=2.0*tracePi*b;
    vec3 tangent=traceNormalize(cross(abs(normal.z)<.999?vec3(0,0,1):vec3(0,1,0),normal),vec3(1,0,0));
    return traceNormalize(tangent*(r*cos(angle))+cross(normal,tangent)*(r*sin(angle))+normal*sqrt(1.0-a),normal);
}
float traceSurfaceReflectionMask(TraceMaterial material,float sourceAlpha) {
    // Both complete destination-alpha programs and the base-alpha fallback
    // describe where the authored coat reflects.
    const uint exactAlpha=2048u,baseAlpha=1048576u;
    return (material.texture.w&(exactAlpha|baseAlpha))!=0u?clamp(sourceAlpha,0.0,1.0):1.0;
}
float traceReflection(TraceMaterial material,vec3 normal,vec3 incoming) {
    return clamp(mix(material.response.y,material.response.z,1.0-abs(dot(normal,incoming))),0.0,1.0);
}
vec3 traceReflectionTint(TraceMaterial material,vec3 normal,vec3 incoming) {
    return mix(material.base.rgb,material.glancingTint.rgb,clamp(1.0-abs(dot(normal,incoming)),0.0,1.0));
}
vec3 traceDirectSpecular(TraceMaterial material,vec3 normal,vec3 incoming,vec3 direction) {
    float reflection=traceReflection(material,normal,incoming);
    float nl=dot(normal,direction),nv=dot(normal,-incoming);
    if(reflection<=0.0 || material.response.x<=0.0 || nl<=0.0 || nv<=0.0)return vec3(0);
    vec3 halfVector=traceNormalize(direction-incoming,normal);
    float nh=max(dot(normal,halfVector),0.0);
    float alpha=material.response.x*material.response.x,a2=alpha*alpha;
    float denominator=nh*nh*(a2-1.0)+1.0;
    float distribution=a2/(tracePi*denominator*denominator);
    float maskingV=2.0*nv/(nv+sqrt(a2+(1.0-a2)*nv*nv));
    float maskingL=2.0*nl/(nl+sqrt(a2+(1.0-a2)*nl*nl));
    // Native light colors and environment strengths are bounded artistic
    // inputs, not calibrated delta-emitter radiance. Preserve the GGX angular
    // shape but bound this direct-only adapter to the authored F/tint envelope:
    // area-normalizing a sharp lobe otherwise turns unit source light into a
    // white cap. N.L, source color, masking and traced visibility still apply.
    // Indirect GGX sampling/roughness retains its original transport weight.
    float directLobe=min(tracePi*distribution*maskingV*maskingL/max(4.0*nv*nl,1.e-8),1.0);
    return traceReflectionTint(material,normal,incoming)*(reflection*directLobe);
}
uint traceBounceLimit() {return uint(clamp(camera.viewport.y,1.0,2.0));}
bool traceEnvironmentNee() {return (traceVisibility.temporal.y&1u)!=0u;}
#include "path_tracing/source_indirect.glsl"
// The compute grid traces only the stochastic part of primary lighting.
// Distant (sun) lights are deterministic and evaluated per pixel at full
// resolution by traceResolveDistantLights, which keeps their shadows sharp.
#ifdef H2_PATH_TRACING_COMPUTE
const bool tracePrimaryDistantAtResolve=true;
#else
const bool tracePrimaryDistantAtResolve=false;
#endif
float tracePointImportance(TraceLight light,vec3 position,vec3 normal,vec3 incoming,
                           TraceMaterial material,bool cachedStatic,bool primary) {
    for(uint plane=0u;plane<6u;++plane)
        if(dot(light.planes[plane].xyz,position)>light.planes[plane].w)return 0.0;
    bool distant=light.position_radius.w<0.0;
    if(distant && primary && tracePrimaryDistantAtResolve)return 0.0;
    vec3 delta=distant?light.position_radius.xyz:light.position_radius.xyz-position;
    float distance=length(delta),radius=light.position_radius.w;
    if(distance<=traceEpsilon || (!distant && distance>=radius))return 0.0;
    vec3 direction=delta/distance;float cosine=max(dot(normal,direction),0.0);
    if(cosine<=0.0)return 0.0;
    float fade=distant?1.0:max(1.0-distance/radius,0.0);fade*=fade;
    vec3 energy=light.color_intensity.rgb*(light.color_intensity.w*fade*cosine);
    vec3 response=vec3(cachedStatic && distant?0.0:1.0)+traceDirectSpecular(material,normal,incoming,direction);
    float weight=traceSourceBrightness(energy*response);
    return isnan(weight) || isinf(weight)?0.0:max(weight,0.0);
}
uint traceUnbakedEmitterCount() {
    uint count=traceScene.counts.w;
    if(count==0u)return 0u;
    uint header=floatBitsToUint(traceScene.environment.w)+2u*count;
    uint length=uint(tracePixels.length());
    if(header>=length || length-header<4u || tracePixels[header]!=traceEmitterGroupMagic)return 0u;
    return min(count,tracePixels[header+3u]);
}
// One point on an emitter chosen in proportion to its power (the running sum
// after the emitter index list; refresh_emitters): the light it sends this
// point over the probability of choosing it, before visibility. `baked` marks
// a source the lightmap bake already holds. False when it sends nothing.
bool traceSampleEmitter(vec3 position,vec3 normal,uint count,float choice,inout uint state,
                        out vec3 direction,out float maximum,out vec3 light,out bool baked) {
    direction=normal;maximum=0.0;light=vec3(0);baked=false;
    uint table=floatBitsToUint(traceScene.environment.w),powers=table+traceScene.counts.w;
    if(count==0u)return false;
    float total=uintBitsToFloat(tracePixels[powers+count-1u]),chosen=choice*total;
    uint low=0u,high=count-1u;
    [[dont_unroll]] for(uint step=0u;step<32u && low<high;++step) {
        uint middle=low+(high-low)/2u;
        if(uintBitsToFloat(tracePixels[powers+middle])<=chosen)low=middle+1u;else high=middle;
    }
    uint entry=tracePixels[table+low];baked=(entry&traceEmitterBakedBit)!=0u;
    float below=low>0u?uintBitsToFloat(tracePixels[powers+low-1u]):0.0;
    float share=(uintBitsToFloat(tracePixels[powers+low])-below)/max(total,1.e-30);
    TraceTriangle triangle=traceTriangles[entry&~traceEmitterBakedBit];
    float a=sqrt(traceRandom(state)),b=traceRandom(state);
    vec3 barycentric=vec3(1.0-a,a*(1.0-b),a*b);
    vec3 target=triangle.p0.xyz*barycentric.x+triangle.p1.xyz*barycentric.y+triangle.p2.xyz*barycentric.z;
    vec3 delta=target-position;float squared=dot(delta,delta);
    if(!(share>0.0) || squared<=traceEpsilon*traceEpsilon)return false;
    float distance=sqrt(squared);direction=delta/distance;
    vec3 areaNormal=cross(triangle.p1.xyz-triangle.p0.xyz,triangle.p2.xyz-triangle.p0.xyz);
    float area=length(areaNormal)*.5,cosine=max(dot(normal,direction),0.0);
    float emitterCosine=abs(dot(traceNormalize(areaNormal,normal),-direction));
    if(cosine<=0.0)return false;
    TraceMaterial material=traceMaterials[triangle.metadata.x];vec2 uv=traceUV(triangle,barycentric);
    float alpha;
    if(!traceAtlasCoverage(material,uv,alpha) || !traceAlphaPass(material,alpha))return false;
    light=traceEmission(material,uv,traceVertexColor(triangle,barycentric))*
        (cosine*emitterCosine*area/(share*tracePi*squared));
    maximum=max(traceEpsilon,distance-traceEpsilon*2.0);
    return true;
}
// Diffuse lighting (luminance) that the emitters the bake holds give this
// point, before their calibrated scale (calibration.w): what calibrate.comp
// compares with the lightmap to find that scale. Two shadowed samples.
float traceBakedEmitterLuminance(vec3 position,vec3 normal,inout uint state) {
    float sum=0.0;
    [[dont_unroll]] for(uint n=0u;n<2u;++n) {
        vec3 direction,light;float maximum;bool baked;
        if(!traceSampleEmitter(position,normal,traceScene.counts.w,traceRandom(state),state,direction,maximum,light,baked) || !baked)continue;
        if(traceOccluded(position+normal*traceEpsilon,direction,maximum,position,normal,true,0xffffffffu))continue;
        sum+=dot(light,vec3(.2126,.7152,.0722))/max(traceScene.calibration.w,1.e-3);
    }
    return sum*.5;
}
vec3 traceDirect(vec3 position,vec3 normal,vec3 geometric,vec4 colorGuide,uint sourceTriangle,inout uint state,bool primary,
                 bool cachedStatic,TraceMaterial surface,vec3 incoming,out vec3 specular,out vec3 lightDirection) {
    specular=vec3(0);lightDirection=normal;
    // The primary continuation already samples diffuse sky misses and mesh
    // emission hits. Reserve its direct query for point/directional lights:
    // choosing between a sun, sky and black emitter needlessly gives every
    // receiver a high-variance direct shadow. Secondary surfaces retain NEE
    // for all three banks, including the final allowed transport vertex.
    // A primary surface that keeps its baked field stops at its first hit,
    // so it samples emitters here. Only the unbaked prefix adds light beyond
    // the retained diffuse field. An empty prefix leaves the sample for the
    // other banks instead of randomly returning zero.
    uint emitterCount=cachedStatic?traceUnbakedEmitterCount():traceScene.counts.w;
    bool pointBank=traceScene.counts.z>0u,meshBank=(!primary || cachedStatic) && emitterCount>0u;
    bool skyBank=!primary && traceEnvironmentNee() && !cachedStatic;
    uint banks=uint(pointBank)+uint(meshBank)+uint(skyBank);
    if(banks==0u)return vec3(0);
    // Choose one bank and form its complete radiance estimate first. A single
    // shared visibility call below keeps the full BVH out of bank branches.
    float choice=traceRandom(state)*float(banks),bankWeight=float(banks);
    uint bank=min(uint(choice),banks-1u);float localChoice=fract(choice);
    bool point=pointBank && bank==0u,mesh=meshBank && bank==uint(pointBank);
    vec3 direction=normal,contribution=vec3(0),specularContribution=vec3(0);float maximum=1.e10;bool valid=false;
    if(point) {
        uint selected=min(uint(localChoice*float(traceScene.counts.z)),traceScene.counts.z-1u);
        float inverseProbability=float(traceScene.counts.z);
        if(traceScene.counts.z<=64u) {
            // Receiver-local source-power sampling removes variance from
            // out-of-range lights and unequal intensities without another
            // shadow ray. Both direct lobes use the exact conditional PDF.
            float weights[64];float total=0.0;
            [[dont_unroll]] for(uint n=0u;n<traceScene.counts.z;++n) {
                weights[n]=tracePointImportance(traceLights[n],position,normal,incoming,surface,cachedStatic,primary);
                total+=weights[n];
            }
            if(total<=0.0)return vec3(0);
            float target=localChoice*total,cumulative=0.0;selected=0u;
            [[dont_unroll]] for(uint n=0u;n<traceScene.counts.z;++n) {
                if(weights[n]<=0.0)continue;
                selected=n;cumulative+=weights[n];
                if(target<cumulative)break;
            }
            inverseProbability=total/weights[selected];
        }
        TraceLight light=traceLights[selected];bool distant=light.position_radius.w<0.0;
        vec3 delta=distant?light.position_radius.xyz:light.position_radius.xyz-position;
        // Large banks select uniformly; a distant light is still resolved per pixel.
        bool inside=!(distant && primary && tracePrimaryDistantAtResolve);
        for(uint plane=0u;plane<6u;++plane)
            if(dot(light.planes[plane].xyz,position)>light.planes[plane].w)inside=false;
        float distance=length(delta),radius=light.position_radius.w;
        if(inside && distance>traceEpsilon && (distant || distance<radius)) {
            direction=delta/distance;float cosine=max(dot(normal,direction),0.0);
            maximum=distant?1.e10:max(traceEpsilon,distance-traceEpsilon*2.0);
            if(cosine>0.0) {
                float fade=distant?1.0:max(1.0-distance/radius,0.0);fade*=fade;
                contribution=light.color_intensity.rgb*(light.color_intensity.w*fade*cosine*inverseProbability*bankWeight);
                specularContribution=contribution*traceDirectSpecular(surface,normal,incoming,direction);
                if(distant)contribution=cachedStatic?vec3(0):traceStaticColor(contribution,colorGuide);
                valid=any(greaterThan(contribution,vec3(0))) || any(greaterThan(specularContribution,vec3(0)));
            }
        }
    } else if(mesh) {
        bool baked;vec3 light;
        if(traceSampleEmitter(position,normal,emitterCount,localChoice,state,direction,maximum,light,baked)) {
            contribution=light*bankWeight;valid=true;
        }
    } else {
        // Cosine importance sampling cancels the Lambertian cosine/pi PDF.
        // Terminal diffuse hits can therefore receive actual sky radiance at
        // depth1, including colored canyon-to-ground transport.
        direction=traceHemisphere(normal,state);
        contribution=traceStaticColor(traceSkyDiffuse(direction)*bankWeight,colorGuide);valid=true;
    }
    if(valid && !traceOccluded(position+normal*traceEpsilon,direction,maximum,position,normal,
        dot(normal,direction)>0.0,sourceTriangle)) {specular=specularContribution;lightDirection=direction;return contribution;}
    return vec3(0);
}
// Full-resolution direct lighting from distant (sun) lights: one shadow ray
// per light and pixel, no noise, no temporal history. Same radiance as the
// stochastic direct estimate, including the authored lightmap hue.
// `normal` shades diffuse light (it may be bump mapped); `specularNormal` is
// the smooth normal the legacy reflection response was authored for.
void traceResolveDistantLights(vec3 position,vec3 normal,vec3 specularNormal,vec3 geometric,vec3 incoming,
                               TraceMaterial material,vec4 colorGuide,bool cachedStatic,out vec3 diffuse,out vec3 specular) {
    diffuse=vec3(0);specular=vec3(0);
    [[dont_unroll]] for(uint n=0u;n<traceScene.counts.z;++n) {
        TraceLight light=traceLights[n];
        if(light.position_radius.w>=0.0)continue;
        bool inside=true;
        for(uint plane=0u;plane<6u;++plane)
            if(dot(light.planes[plane].xyz,position)>light.planes[plane].w)inside=false;
        vec3 direction=traceNormalize(light.position_radius.xyz,vec3(0,0,1));
        float cosine=dot(normal,direction),specularCosine=max(dot(specularNormal,direction),0.0);
        if(!inside || (cosine<=0.0 && specularCosine<=0.0) || dot(geometric,direction)<=0.0)continue;
        vec3 energy=light.color_intensity.rgb*(light.color_intensity.w*max(cosine,0.0));
        vec3 lightSpecular=light.color_intensity.rgb*(light.color_intensity.w*specularCosine)*
            traceDirectSpecular(material,specularNormal,incoming,direction);
        vec3 lightDiffuse=cachedStatic?vec3(0):traceStaticColor(energy,colorGuide);
        if(!any(greaterThan(lightDiffuse+lightSpecular,vec3(0))))continue;
        if(traceOccluded(position+geometric*(traceEpsilon*2.0),direction,1.e10,position,normal,true,0xffffffffu))continue;
        diffuse+=lightDiffuse;specular+=lightSpecular;
    }
}
// Baked bounce light where a secondary ray lands on a lightmapped surface.
// The bake already holds the static world's sun, sky and fixture light; with
// hardware rays, sun light that a dynamic object now blocks at that point is
// removed, so players and vehicles also shadow the light they would bounce.
// Static occluders are tested first: the bake already lacks that light.
// calibration.x relates traced sun irradiance to the bake (calibrate.comp).
vec3 traceBakedBounce(vec3 position,vec3 geometric,vec3 normal,vec4 colorGuide) {
    vec3 baked=traceSourceIndirect(colorGuide);
#ifdef H2_PATH_TRACING_HARDWARE_RAYS
    // calibration.z: the scene separates static world triangles from objects.
    if(traceScene.calibration.z==0.0)return baked;
    vec3 blocked=vec3(0);
    [[dont_unroll]] for(uint n=0u;n<traceScene.counts.z;++n) {
        TraceLight light=traceLights[n];
        if(light.position_radius.w>=0.0)continue;
        bool inside=true;
        for(uint plane=0u;plane<6u;++plane)
            if(dot(light.planes[plane].xyz,position)>light.planes[plane].w)inside=false;
        vec3 direction=traceNormalize(light.position_radius.xyz,vec3(0,0,1));
        float cosine=dot(normal,direction);
        if(!inside || cosine<=0.0 || dot(geometric,direction)<=0.0)continue;
        vec3 origin=position+geometric*(traceEpsilon*2.0);
        if(traceOccludedBy(traceStaticOccluderMask,origin,direction,1.e10,false,0xffffffffu))continue;
        if(traceOccludedBy(traceDynamicOccluderMask,origin,direction,1.e10,false,0xffffffffu))
            blocked+=traceStaticColor(light.color_intensity.rgb*(light.color_intensity.w*cosine),colorGuide);
    }
    baked=max(baked-blocked*traceScene.calibration.x,vec3(0));
#endif
    return baked;
}
#include "path_tracing/emitter_sampling.glsl"
bool traceGGXDirection(vec3 normal,vec3 incoming,TraceMaterial material,inout uint state,
                       out vec3 direction,out float sampleWeight) {
    // GGX half-vector sampling: original strengths stay independent of the
    // deliberately conservative, optional legacy-to-roughness adaptation.
    float roughness=material.response.x,alpha=roughness*roughness;
    float u=traceRandom(state),phi=2.0*tracePi*traceRandom(state);
    float cosine=sqrt((1.0-u)/(1.0+(alpha*alpha-1.0)*u));
    float sine=sqrt(max(0.0,1.0-cosine*cosine));
    vec3 tangent=traceNormalize(cross(abs(normal.z)<.999?vec3(0,0,1):vec3(0,1,0),normal),vec3(1,0,0));
    vec3 halfVector=traceNormalize(tangent*(cos(phi)*sine)+cross(normal,tangent)*(sin(phi)*sine)+normal*cosine,normal);
    direction=reflect(incoming,halfVector);
    float nl=dot(direction,normal),nv=max(dot(-incoming,normal),1.e-6);
    if(nl<=0.0)return false;
    float nh=max(dot(normal,halfVector),1.e-6),vh=max(dot(-incoming,halfVector),0.0);
    float ggxV=2.0*nv/(nv+sqrt(alpha*alpha+(1.0-alpha*alpha)*nv*nv));
    float ggxL=2.0*nl/(nl+sqrt(alpha*alpha+(1.0-alpha*alpha)*nl*nl));
    // Half-vector sampling's weight G*VH/(NV*NH) grows without bound as the
    // view grazes the surface (NV -> 0). Masking-shadowing can only remove
    // energy, so bound it by one: grazing rough faces otherwise reflect the
    // whole sky as a saturated rim.
    sampleWeight=min(ggxV*ggxL*vh/(nv*nh),1.0);return true;
}
// `incident`: the directions the primary surface's diffuse light arrived
// from, each weighted by that light's luminance (its length over the diffuse
// luminance is how directional the light is). The resolve shades bump maps
// against it (traceIndirectRelief).
void traceLighting(vec3 position,vec3 normal,vec3 geometric,vec3 incoming,TraceMaterial material,
                   vec3 albedo,vec4 colorGuide,bool primaryCachedStatic,inout uint state,out vec3 diffuseLight,out vec3 specularLight,
                   out vec3 currentDirect,out vec3 currentDirectSpecular,out vec3 incident) {
    diffuseLight=vec3(0);specularLight=vec3(0);currentDirect=vec3(0);currentDirectSpecular=vec3(0);vec3 throughput=vec3(1);
    incident=vec3(0);vec3 continuation=normal;
    bool primarySpecular=false;vec3 sourceAmbient=vec3(0);
    uint sourceTriangle=0xffffffffu;
    uint maximumBounces=traceBounceLimit();
    // A single transport loop owns every secondary segment. Keeping the BVH,
    // direct-light and authored-emission call sites here avoids inlining a
    // separate complete ray tracer for each diffuse/reflection continuation.
    [[dont_unroll]] for(uint bounce=0u;bounce<=maximumBounces;++bounce) {
        float reflection=traceReflection(material,normal,incoming);
        bool diffuseSurface=bounce==0u?(material.texture.w&32u)==0u:any(greaterThan(albedo,vec3(0)));
        bool cachedStatic=bounce==0u?primaryCachedStatic:traceSourceIndirectAvailable(material,colorGuide,bounce);
        if(bounce>0u) {
            // Native model ambient is additive to its environment response.
            // Apply the same exact baseline as the visible primary, after
            // reaching this owner with an actual secondary intersection.
            vec3 baseline=throughput*albedo*sourceAmbient;
            if(primarySpecular)specularLight+=baseline;else diffuseLight+=baseline;
        }
        if(diffuseSurface || reflection>0.0) {
            // Primary cached interiors add their source field at full
            // resolution after filtering. Compatible secondary hits retain
            // their own field here. Neither repeats static sky/sun transport.
            vec3 directSpecular,lightDirection;
            vec3 direct=traceDirect(position,normal,geometric,colorGuide,sourceTriangle,state,bounce==0u,
                cachedStatic,material,incoming,directSpecular,lightDirection);
            // As in Halo 2, the reflection adds to the whole diffuse response.
            direct=diffuseSurface?
                direct+(cachedStatic && bounce>0u?traceBakedBounce(position,geometric,normal,colorGuide):vec3(0)):vec3(0);
            // Keep the primary authored albedo outside the lighting estimate.
            // Secondary surfaces still contribute their own colored response.
            // Direct visibility has its own history validity. Keep both
            // diffuse shadows and highlights separate from indirect history.
            if(bounce==0u) {
                currentDirect=direct;currentDirectSpecular=directSpecular;
                incident=lightDirection*dot(direct,vec3(.2126,.7152,.0722));
            }
            else if(primarySpecular)specularLight+=throughput*(albedo*direct+directSpecular);
            else diffuseLight+=throughput*(albedo*direct+directSpecular);
        }
        if(bounce==maximumBounces)break;
        // Choose one lobe per segment. Halo 2 adds the reflection to the
        // whole diffuse response, so each lobe carries its full strength over
        // the probability of choosing it, and the diffuse lobe always stays
        // reachable: a shiny material is not a mirror without a surface.
        // Primary diffuse is demodulated before spatial/temporal filtering;
        // its full-resolution reflection mask is applied at resolve.
        float probability=diffuseSurface?min(reflection,.5):reflection;
        // A cached diffuse field has already integrated that lobe. Spend
        // the remaining segment on GGX, weighting it by its actual strength
        // instead of rarely tracing an amplified low-reflectance sample.
        if(cachedStatic && reflection>0.0)probability=1.0;
        // A primary surface with an authored environment map shows that map
        // (world.frag); its traced reflection would go unused.
        if(bounce==0u && material.detail.y!=0u)probability=0.0;
        bool reflected=probability>=1.0 || (probability>0.0 && traceRandom(state)<probability);
        if(bounce==0u)primarySpecular=reflected;
        vec3 direction;
        if(reflected) {
            float weight;
            if(!traceGGXDirection(normal,incoming,material,state,direction,weight))break;
            throughput*=traceReflectionTint(material,normal,incoming)*weight*(reflection/probability);
        }else {
            // This diffuse field already integrates the remaining static
            // paths. Stop at its first compatible hit for either bounce
            // budget; a selected GGX lobe above still follows its normal ray.
            if(cachedStatic)break;
            // The diffuse lobe keeps its full weight whichever lobe is chosen
            // (the reflection is additive, as above).
            if(bounce!=0u)throughput*=albedo/(1.0-probability);
            else if((material.texture.w&32u)!=0u)break;
            else throughput/=1.0-probability;
            if(!any(greaterThan(throughput,vec3(0))))break;
            if(bounce==0u) {
                float backgroundHint=material.colorGuide.w==2u && colorGuide.a>0.0?
                    2.0*traceSourceBrightness(colorGuide.rgb):0.0;
                float weight;direction=traceDiffuseContinuation(position,normal,state,weight,backgroundHint);throughput*=weight;
                continuation=direction;
            }else direction=traceHemisphere(normal,state);
        }
        if(!any(greaterThan(throughput,vec3(0))))break;
        vec3 origin=position+normal*traceEpsilon;TraceHit hit;
        if(!traceClosest(origin,direction,1.e10,hit,position,normal,
            !reflected && dot(normal,direction)>0.0,sourceTriangle)) {
            // The primary surface uses this existing continuation for sky
            // lighting. Later diffuse misses are already covered by their
            // previous vertex's environment NEE; specular misses are not.
            if(reflected || bounce==0u || !traceEnvironmentNee()) {
                vec3 skyColor=reflected?traceSkySpecular(direction,material.response.x):traceSkyDiffuse(direction);
                if(!reflected)skyColor=traceStaticColor(skyColor,colorGuide);
                vec3 sky=throughput*skyColor;
                if(primarySpecular)specularLight+=sky;else diffuseLight+=sky;
            }
            break;
        }
        TraceTriangle triangle=traceTriangles[hit.triangle];
        position=origin+direction*hit.distance;normal=traceNormal(triangle,hit.barycentric,direction);
        geometric=traceNormalize(cross(triangle.p1.xyz-triangle.p0.xyz,triangle.p2.xyz-triangle.p0.xyz),normal);
        if(dot(geometric,normal)<0.0)geometric=-geometric;
        sourceTriangle=hit.triangle;
        material=traceMaterials[triangle.metadata.x];incoming=direction;
        colorGuide=traceTriangleColorGuide(triangle,hit.barycentric,material);
        vec2 uv=traceUV(triangle,hit.barycentric);vec3 tint=traceVertexColor(triangle,hit.barycentric);
        vec4 surface;float terrainBlend;
        if(!traceAtlasSurface(material,uv,surface,terrainBlend))break;
        material.response.yz*=traceSurfaceReflectionMask(material,surface.a);
        albedo=(material.texture.w&32u)!=0u?vec3(0):clamp(surface.rgb*tint,0.0,1.0)*
            traceDiffuseRetention(material,terrainBlend,normal,incoming);
        sourceAmbient=traceSecondarySourceAmbient(material,uv);
        // Primary diffuse emission uses the existing BSDF continuation.
        // Later diffuse hits are covered by mesh NEE at their previous
        // vertex. Reflected hits always retain their authored emission.
        if(reflected || bounce==0u) {
            vec3 emission=throughput*traceEmission(material,uv,tint);
            if(primarySpecular)specularLight+=emission;else diffuseLight+=emission;
        }
    }
    // Every diffuse path left the primary surface along its continuation.
    if(!primarySpecular)incident+=continuation*dot(diffuseLight,vec3(.2126,.7152,.0722));
}
// Halo 2's environment pass reflects each pixel along its own (bump-mapped)
// mirror direction; the grid's reflection is a half-resolution, denoised
// average of GGX samples. How much of a glossy surface's reflection the
// resolve traces per pixel instead: all of it for polished surfaces, none for
// dull ones, which keep the grid's blurred reflection.
float traceMirrorSharpness(TraceMaterial material) {
    // The same finish shapes GGX and decides whether a sharp resolve ray
    // contributes. Artistic reflection gain alone cannot turn a broad lobe
    // into a mirror.
    return clamp((.48-material.response.x)/.30,0.0,1.0);
}
vec3 traceFiniteMirror(vec3 value) {return any(isnan(value)) || any(isinf(value))?vec3(0):min(max(value,vec3(0)),vec3(16.0));}
vec3 traceMirrorDirection(vec3 normal,vec3 geometric,vec3 incoming) {
    vec3 direction=reflect(incoming,normal);
    // A bumped normal can send the mirror ray into the surface. Fold it back.
    float facing=dot(direction,geometric);
    return facing>0.0?direction:traceNormalize(direction-2.0*facing*geometric,geometric);
}
vec3 traceReflectedHitRadiance(TraceHit hit,vec3 direction) {
    TraceTriangle triangle=traceTriangles[hit.triangle];
    TraceMaterial surface=traceMaterials[triangle.metadata.x];
    vec2 uv=traceUV(triangle,hit.barycentric);vec3 tint=traceVertexColor(triangle,hit.barycentric);
    vec4 color;float terrainBlend;
    if(!traceAtlasSurface(surface,uv,color,terrainBlend))return vec3(0);
    vec3 normal=traceNormal(triangle,hit.barycentric,direction);
    vec3 albedo=(surface.texture.w&32u)!=0u?vec3(0):clamp(color.rgb*tint,0.0,1.0)*
        traceDiffuseRetention(surface,terrainBlend,normal,direction);
    vec4 guide=traceTriangleColorGuide(triangle,hit.barycentric,surface);
    vec3 field=traceBakedGuide(surface,guide)?traceSourceIndirect(guide):traceSecondarySourceAmbient(surface,uv);
    return albedo*field+traceEmission(surface,uv,tint);
}
// One ray along the mirror direction of `normal`, times the surface's
// reflection response. The hit is lit by what is already known there: its
// emission, its baked lighting field or owner ambient; a miss sees the sky.
vec3 traceMirrorReflection(vec3 position,vec3 normal,vec3 geometric,vec3 incoming,TraceMaterial material) {
    float strength=traceReflection(material,normal,incoming);
    if(!(strength>0.0))return vec3(0);
    vec3 direction=traceMirrorDirection(normal,geometric,incoming);
    vec3 light;TraceHit hit;
    if(!traceClosest(position+geometric*(traceEpsilon*2.0),direction,1.e10,hit,position,geometric,false,0xffffffffu))
        light=traceSkySpecular(direction,material.response.x);
    else light=traceReflectedHitRadiance(hit,direction);
    return traceFiniteMirror(light)*traceReflectionTint(material,normal,incoming)*strength;
}
#endif
