#ifndef H2_PATH_TRACING_SCENE_GLSL
#define H2_PATH_TRACING_SCENE_GLSL
struct TraceNode {vec4 minimum,maximum;uvec4 children;};
struct TraceTriangle {vec4 p0,p1,p2,n0,n1,n2,uv01,uv2;uvec4 metadata;};
struct TraceMaterial {vec4 base,emission,response;uvec4 texture;vec4 transform;};
struct TraceLight {vec4 position_radius,color_intensity;vec4 planes[6];};
layout(set=1,binding=0,std430) readonly restrict buffer TraceNodes {TraceNode traceNodes[];};
layout(set=1,binding=1,std430) readonly restrict buffer TraceTriangles {TraceTriangle traceTriangles[];};
layout(set=1,binding=2,std430) readonly restrict buffer TraceMaterials {TraceMaterial traceMaterials[];};
layout(set=1,binding=3,std430) readonly restrict buffer TraceLights {TraceLight traceLights[];};
layout(set=1,binding=4,std430) readonly restrict buffer TracePixels {uint tracePixels[];};
layout(set=1,binding=5,std140) uniform TraceScene {uvec4 counts;vec4 environment;} traceScene;
// Captured through the ordinary source sky renderer, in the WORLD cube basis.
// Its valid fallback is the current authored environment, including black.
layout(set=1,binding=7) uniform samplerCube traceSky;
const float tracePi=3.14159265358979323846;
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
bool traceTriangleHit(uint index,vec3 origin,vec3 direction,float maximum,out TraceHit hit) {
    // Normal/color/UV lanes are not needed for opaque visibility. Do not load
    // the full nine-lane record for every candidate traversed by every ray.
    vec3 p0=traceTriangles[index].p0.xyz;
    vec3 e1=traceTriangles[index].p1.xyz-p0,e2=traceTriangles[index].p2.xyz-p0;
    vec3 p=cross(direction,e2);float determinant=dot(e1,p);
    if(abs(determinant)<1.e-9)return false;
    vec3 t=origin-p0;float u=dot(t,p)/determinant;
    if(u<0.0 || u>1.0)return false;
    vec3 q=cross(t,e1);float v=dot(direction,q)/determinant;
    if(v<0.0 || u+v>1.0)return false;
    float distance=dot(e2,q)/determinant;
    if(distance<=traceEpsilon*.5 || distance>=maximum)return false;
    vec3 barycentric=vec3(1.0-u-v,u,v);
    uint materialIndex=traceTriangles[index].metadata.x;
    // Function arguments are evaluated before traceAlphaPass. Guard the
    // lookup here: ordinary opaque triangles require no alpha texel at all.
    if((traceMaterials[materialIndex].texture.w&9u)!=0u) {
        TraceMaterial material=traceMaterials[materialIndex];
        vec4 uv01=traceTriangles[index].uv01;vec2 uv2=traceTriangles[index].uv2.xy;
        vec2 uv=uv01.xy*barycentric.x+uv01.zw*barycentric.y+uv2*barycentric.z;
        float alpha;
        if(!traceAtlasCoverage(material,uv,alpha) || !traceAlphaPass(material,alpha))return false;
    }
    hit=TraceHit(distance,index,barycentric);return true;
}
bool traceClosest(vec3 origin,vec3 direction,float maximum,out TraceHit hit) {
    if(traceScene.counts.x==0u)return false;
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
            if(traceTriangleHit(node.children.x+n,origin,direction,hit.distance,candidate)) {hit=candidate;found=true;}
        }
    }
    return found;
}
bool traceOccluded(vec3 origin,vec3 direction,float maximum) {
    if(traceScene.counts.x==0u)return false;
    vec3 inverseDirection=traceInverseDirection(direction);
    uint stack[32];uint size=1u;stack[0]=0u;
    while(size>0u) {
        TraceNode node=traceNodes[stack[--size]];
        if(traceBoxNear(node,origin,direction,inverseDirection,maximum)<0.0)continue;
        if(node.children.z==0u) {stack[size++]=node.children.x;stack[size++]=node.children.y;continue;}
        for(uint n=0u;n<node.children.y;++n) {
            TraceHit hit;
            if(traceTriangleHit(node.children.x+n,origin,direction,maximum,hit))return true;
        }
    }
    return false;
}
vec3 traceNormal(TraceTriangle triangle,vec3 barycentric,vec3 incoming) {
    vec3 geometric=traceNormalize(cross(triangle.p1.xyz-triangle.p0.xyz,triangle.p2.xyz-triangle.p0.xyz),vec3(0,0,1));
    vec3 normal=traceNormalize(triangle.n0.xyz*barycentric.x+triangle.n1.xyz*barycentric.y+triangle.n2.xyz*barycentric.z,geometric);
    return dot(normal,incoming)>0.0?-normal:normal;
}
vec3 traceHemisphere(vec3 normal,inout uint state) {
    float a=traceRandom(state),b=traceRandom(state),r=sqrt(a),angle=2.0*tracePi*b;
    vec3 tangent=traceNormalize(cross(abs(normal.z)<.999?vec3(0,0,1):vec3(0,1,0),normal),vec3(1,0,0));
    return traceNormalize(tangent*(r*cos(angle))+cross(normal,tangent)*(r*sin(angle))+normal*sqrt(1.0-a),normal);
}
float traceReflection(TraceMaterial material,vec3 normal,vec3 incoming) {
    return clamp(mix(material.response.y,material.response.z,1.0-abs(dot(normal,incoming))),0.0,1.0);
}
uint traceBounceLimit() {return uint(clamp(camera.viewport.y,1.0,2.0));}
bool traceEnvironmentNee() {return (traceVisibility.temporal.y&1u)!=0u;}
vec3 traceDirect(vec3 position,vec3 normal,inout uint state) {
    bool pointBank=traceScene.counts.z>0u,meshBank=traceScene.counts.w>0u,skyBank=traceEnvironmentNee();
    uint banks=uint(pointBank)+uint(meshBank)+uint(skyBank);
    if(banks==0u)return vec3(0);
    // Choose one bank and form its complete radiance estimate first. A single
    // shared visibility call below keeps the full BVH out of bank branches.
    float choice=traceRandom(state)*float(banks),bankWeight=float(banks);
    uint bank=min(uint(choice),banks-1u);float localChoice=fract(choice);
    bool point=pointBank && bank==0u,mesh=meshBank && bank==uint(pointBank);
    vec3 direction=normal,contribution=vec3(0);float maximum=1.e10;bool valid=false;
    if(point) {
        uint selected=min(uint(localChoice*float(traceScene.counts.z)),traceScene.counts.z-1u);
        TraceLight light=traceLights[selected];bool distant=light.position_radius.w<0.0;
        vec3 delta=distant?light.position_radius.xyz:light.position_radius.xyz-position;
        bool inside=true;
        for(uint plane=0u;plane<6u;++plane)
            if(dot(light.planes[plane].xyz,position)>light.planes[plane].w)inside=false;
        float distance=length(delta),radius=light.position_radius.w;
        if(inside && distance>traceEpsilon && (distant || distance<radius)) {
            direction=delta/distance;float cosine=max(dot(normal,direction),0.0);
            maximum=distant?1.e10:max(traceEpsilon,distance-traceEpsilon*2.0);
            if(cosine>0.0) {
                float fade=distant?1.0:max(1.0-distance/radius,0.0);fade*=fade;
                contribution=light.color_intensity.rgb*(light.color_intensity.w*fade*cosine*float(traceScene.counts.z)*bankWeight);
                valid=true;
            }
        }
    } else if(mesh) {
        uint emitter=min(uint(localChoice*float(traceScene.counts.w)),traceScene.counts.w-1u);
        uint index=tracePixels[floatBitsToUint(traceScene.environment.w)+emitter];
        TraceTriangle triangle=traceTriangles[index];float a=sqrt(traceRandom(state)),b=traceRandom(state);
        vec3 barycentric=vec3(1.0-a,a*(1.0-b),a*b);
        vec3 target=triangle.p0.xyz*barycentric.x+triangle.p1.xyz*barycentric.y+triangle.p2.xyz*barycentric.z;
        vec3 delta=target-position;float squared=dot(delta,delta);
        if(squared>traceEpsilon*traceEpsilon) {
            float distance=sqrt(squared);direction=delta/distance;
            vec3 areaNormal=cross(triangle.p1.xyz-triangle.p0.xyz,triangle.p2.xyz-triangle.p0.xyz);
            float area=length(areaNormal)*.5,cosine=max(dot(normal,direction),0.0);
            float emitterCosine=abs(dot(traceNormalize(areaNormal,normal),-direction));
            if(cosine>0.0) {
                TraceMaterial material=traceMaterials[triangle.metadata.x];vec2 uv=traceUV(triangle,barycentric);
                float alpha;
                if(traceAtlasCoverage(material,uv,alpha) && traceAlphaPass(material,alpha)) {
                    contribution=traceEmission(material,uv,traceVertexColor(triangle,barycentric))*
                        (cosine*emitterCosine*area*float(traceScene.counts.w)*bankWeight/(tracePi*squared));
                    maximum=max(traceEpsilon,distance-traceEpsilon*2.0);valid=true;
                }
            }
        }
    } else {
        // Cosine importance sampling cancels the Lambertian cosine/pi PDF.
        // Terminal diffuse hits can therefore receive actual sky radiance at
        // depth1, including colored canyon-to-ground transport.
        direction=traceHemisphere(normal,state);
        contribution=max(textureLod(traceSky,direction,0.0).rgb,vec3(0))*bankWeight;valid=true;
    }
    if(valid && !traceOccluded(position+normal*traceEpsilon,direction,maximum))return contribution;
    return vec3(0);
}
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
    sampleWeight=ggxV*ggxL*vh/(nv*nh);return true;
}
void traceLighting(vec3 position,vec3 normal,vec3 incoming,TraceMaterial material,
                   vec3 albedo,inout uint state,out vec3 diffuseLight,out vec3 specularLight) {
    diffuseLight=vec3(0);specularLight=vec3(0);vec3 throughput=vec3(1);
    bool primarySpecular=false;
    uint maximumBounces=traceBounceLimit();
    // A single transport loop owns every secondary segment. Keeping the BVH,
    // direct-light and authored-emission call sites here avoids inlining a
    // separate complete ray tracer for each diffuse/reflection continuation.
    [[dont_unroll]] for(uint bounce=0u;bounce<=maximumBounces;++bounce) {
        float reflection=traceReflection(material,normal,incoming);
        bool diffuseSurface=bounce==0u?(material.texture.w&32u)==0u:any(greaterThan(albedo,vec3(0)));
        if(diffuseSurface && reflection<1.0) {
            vec3 direct=(1.0-reflection)*traceDirect(position,normal,state);
            // Keep the primary authored albedo outside the lighting estimate.
            // Secondary surfaces still contribute their own colored response.
            if(bounce==0u)diffuseLight+=direct;
            else if(primarySpecular)specularLight+=throughput*albedo*direct;
            else diffuseLight+=throughput*albedo*direct;
        }
        if(bounce==maximumBounces)break;
        // Choose one source lobe per segment. Its scalar probability cancels
        // the same factor in the BRDF, retaining authored matte/visor energy.
        bool reflected=reflection>=1.0 || (reflection>0.0 && traceRandom(state)<reflection);
        if(bounce==0u)primarySpecular=reflected;
        vec3 direction;
        if(reflected) {
            float weight;
            if(!traceGGXDirection(normal,incoming,material,state,direction,weight))break;
            throughput*=material.base.rgb*weight;
        }else {
            if(bounce!=0u)throughput*=albedo;
            else if((material.texture.w&32u)!=0u)break;
            if(!any(greaterThan(throughput,vec3(0))))break;
            direction=traceHemisphere(normal,state);
        }
        if(!any(greaterThan(throughput,vec3(0))))break;
        vec3 origin=position+normal*traceEpsilon;TraceHit hit;
        if(!traceClosest(origin,direction,1.e10,hit)) {
            // Diffuse sky paths are already estimated by the environment
            // bank at the previous surface. Specular misses retain their
            // own cube radiance, including reflection→diffuse paths later.
            if(reflected || !traceEnvironmentNee()) {
                vec3 sky=throughput*textureLod(traceSky,direction,0.0).rgb;
                if(primarySpecular)specularLight+=sky;else diffuseLight+=sky;
            }
            break;
        }
        TraceTriangle triangle=traceTriangles[hit.triangle];
        position=origin+direction*hit.distance;normal=traceNormal(triangle,hit.barycentric,direction);
        material=traceMaterials[triangle.metadata.x];incoming=direction;
        vec2 uv=traceUV(triangle,hit.barycentric);vec3 tint=traceVertexColor(triangle,hit.barycentric);
        vec4 surface;if(!traceAtlasSurface(material,uv,surface))break;
        albedo=(material.texture.w&32u)!=0u?vec3(0):clamp(surface.rgb*tint,0.0,1.0);
        // Diffuse emission is sampled by next-event estimation. A reflected
        // hit contributes its authored radiance because that specular lobe
        // was not included in the diffuse next-event estimator above.
        if(reflected) {
            vec3 emission=throughput*traceEmission(material,uv,tint);
            if(primarySpecular)specularLight+=emission;else diffuseLight+=emission;
        }
    }
}
#endif
