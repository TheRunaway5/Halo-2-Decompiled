#ifndef H2_PATH_TRACING_LIGHTING_FILTER_GLSL
#define H2_PATH_TRACING_LIGHTING_FILTER_GLSL
// Binding6 contains the full-resolution source-W depth prefix followed by
// twelve-word HDR lighting records on a bounded grid and two twenty-two-word
// persistent histories (world/foreground). Its32-byte header contains extent
// {fullWidth,fullHeight,depthWordCount,lightingScale} and temporal{frameSerial}.
// Material word7 is an atomic first-writer claim, so overlapping primitives
// never tear a record or update the same history cell concurrently.
const uint traceLightingWords=12u,traceLightingInvalid=0xffffffffu;
const uint traceHistoryWords=22u,traceHistoryLimit=16u;
// An immutable snapshot makes reprojection independent of fragment execution
// order. The current frame writes binding6; every history lookup reads9.
layout(set=1,binding=9,std430) readonly restrict buffer TracePreviousHistory {
    mat4 previousMVP[2];
    vec4 previousViewport[2];
    vec4 changedOccluderMin,changedOccluderMax;
    vec4 changedLightMin,changedLightMax;
    uint words[];
} tracePrevious;
bool traceChangedShadow(vec3 position,vec3 normal) {
    if(traceVisibility.temporal.w!=0u)return true;
    if(tracePrevious.changedLightMin.w!=0.0 &&
       all(greaterThanEqual(position,tracePrevious.changedLightMin.xyz)) &&
       all(lessThanEqual(position,tracePrevious.changedLightMax.xyz)))return true;
    if((uint(tracePrevious.changedOccluderMin.w)&1u)==0u)return false;
    // Bound validation cost independently of unusually large source banks.
    // These are conservative slab tests only, never additional BVH queries.
    if(traceScene.counts.z>64u)return true;
    vec3 minimum=tracePrevious.changedOccluderMin.xyz-vec3(traceEpsilon*8.0);
    vec3 maximum=tracePrevious.changedOccluderMax.xyz+vec3(traceEpsilon*8.0);
    for(uint lightIndex=0u;lightIndex<traceScene.counts.z;++lightIndex) {
        TraceLight light=traceLights[lightIndex];bool distant=light.position_radius.w<0.0;
        vec3 delta=distant?light.position_radius.xyz:light.position_radius.xyz-position;
        float distance=length(delta);
        if(distance<=traceEpsilon || (!distant && distance>=light.position_radius.w))continue;
        vec3 direction=delta/distance;
        if(dot(direction,normal)<=0.0)continue;
        bool inside=true;
        for(uint plane=0u;plane<6u;++plane)
            if(dot(light.planes[plane].xyz,position)>light.planes[plane].w)inside=false;
        if(!inside)continue;
        float nearDistance=0.0,farDistance=distant?1.e10:distance;
        for(uint axis=0u;axis<3u;++axis) {
            if(abs(direction[axis])<1.e-8) {
                if(position[axis]<minimum[axis] || position[axis]>maximum[axis])farDistance=-1.0;
            }else {
                float a=(minimum[axis]-position[axis])/direction[axis];
                float b=(maximum[axis]-position[axis])/direction[axis];
                nearDistance=max(nearDistance,min(a,b));farDistance=min(farDistance,max(a,b));
            }
        }
        if(farDistance>=nearDistance)return true;
    }
    return false;
}
bool traceChangedReflection(vec3 position,vec3 normal,vec3 incoming,float roughness,float pixelFootprint) {
    if(tracePrevious.changedOccluderMin.w==0.0)return false;
    // The receiver can stay still while a reflected object leaves or enters
    // its lobe. Test the union of that geometry's old/new bounds, not merely
    // the receiver guides. Expand for the pixel footprint and GGX core width;
    // this is one conservative slab test, with no extra scene traversal.
    vec3 minimum=tracePrevious.changedOccluderMin.xyz;
    vec3 maximum=tracePrevious.changedOccluderMax.xyz;
    vec3 direction=reflect(incoming,normal);
    vec3 center=minimum*.5+maximum*.5,halfExtent=maximum*.5-minimum*.5;
    float reach=dot(center-position,direction)+dot(halfExtent,abs(direction));
    if(reach<0.0)return false;
    float padding=max(pixelFootprint*float(max(traceVisibility.extent.w,1u)),traceEpsilon*8.0)+
        min(reach,1.e10)*roughness*roughness*4.0;
    minimum-=vec3(padding);maximum+=vec3(padding);
    float nearDistance=0.0,farDistance=1.e10;
    for(uint axis=0u;axis<3u;++axis) {
        if(abs(direction[axis])<1.e-8) {
            if(position[axis]<minimum[axis] || position[axis]>maximum[axis])return false;
        }else {
            float a=(minimum[axis]-position[axis])/direction[axis];
            float b=(maximum[axis]-position[axis])/direction[axis];
            nearDistance=max(nearDistance,min(a,b));farDistance=min(farDistance,max(a,b));
        }
    }
    return farDistance>=nearDistance;
}
bool traceReflectionCompatible(vec3 oldReflected,vec3 reflected,float roughness) {
    // A small smooth-normal change rotates a mirror direction twice as far.
    // Compare that direction with its actual GGX lobe width; a fixed normal
    // cosine admits visibly stale sharp highlights on animated characters.
    vec3 delta=oldReflected-reflected;
    float tolerance=max(.025,roughness*roughness*2.0);
    return dot(delta,delta)<tolerance*tolerance;
}
bool traceChangedDiffuse(vec3 position,vec3 normal,vec3 incoming,uint material,float pixelFootprint) {
    if((uint(tracePrevious.changedOccluderMin.w)&2u)==0u)return false;
    TraceMaterial value=traceMaterials[material];
    if((value.texture.w&32u)!=0u || ((value.texture.w&2048u)==0u && traceReflection(value,normal,incoming)>=1.0))return false;
    // A changed secondary material can alter diffuse return as well as a
    // reflection. Reject only receivers whose diffuse hemisphere reaches its
    // bounds; surfaces entirely behind that hemisphere retain their history.
    vec3 center=(tracePrevious.changedOccluderMin.xyz+tracePrevious.changedOccluderMax.xyz)*.5;
    vec3 halfExtent=(tracePrevious.changedOccluderMax.xyz-tracePrevious.changedOccluderMin.xyz)*.5;
    float padding=max(pixelFootprint*float(max(traceVisibility.extent.w,1u)),traceEpsilon*8.0);
    return dot(center-position,normal)+dot(halfExtent,abs(normal))>=-padding;
}
uint traceReflectionFingerprint(uint material) {
    TraceMaterial value=traceMaterials[material];uint key=0x9e3779b9u;
    for(uint axis=0u;axis<3u;++axis) {
        key=traceHash(key^floatBitsToUint(value.base[axis]));
        key=traceHash(key^floatBitsToUint(value.glancingTint[axis]));
    }
    key=traceHash(key^floatBitsToUint(value.response.y));
    key=traceHash(key^floatBitsToUint(value.response.z));
    return key&0x07ffffffu;
}
uvec2 traceLightingGrid() {
    uint scale=max(traceVisibility.extent.w,1u);
    return (traceVisibility.extent.xy+scale-1u)/scale;
}
uint traceLightingRecord(uvec2 cell) {
    return traceVisibility.extent.z+(cell.y*traceLightingGrid().x+cell.x)*traceLightingWords;
}
uint traceHistoryRecord(uvec2 cell) {
    uvec2 grid=traceLightingGrid();uint cells=grid.x*grid.y;
    uint role=camera.eyePosition.w==2.0?1u:0u;
    return traceVisibility.extent.z+cells*(traceLightingWords+role*traceHistoryWords)+
        (cell.y*grid.x+cell.x)*traceHistoryWords;
}
bool traceLightingRepresentative(uvec2 pixel) {
    uint scale=max(traceVisibility.extent.w,1u);
    return all(equal(pixel,min((pixel/scale)*scale+scale/2u,traceVisibility.extent.xy-1u)));
}
bool traceLightingClaim(uint record,uint material) {
    return atomicCompSwap(traceVisibility.words[record+7u],traceLightingInvalid,
                          material|0x80000000u)==traceLightingInvalid;
}
vec3 traceFiniteLighting(vec3 value) {
    return any(isnan(value)) || any(isinf(value))?vec3(0):max(value,vec3(0));
}
vec3 traceLightingLoad3(uint record) {
    return vec3(uintBitsToFloat(traceVisibility.words[record]),
                uintBitsToFloat(traceVisibility.words[record+1u]),
                uintBitsToFloat(traceVisibility.words[record+2u]));
}
vec3 tracePreviousLoad3(uint record) {
    return vec3(uintBitsToFloat(tracePrevious.words[record]),
                uintBitsToFloat(tracePrevious.words[record+1u]),
                uintBitsToFloat(tracePrevious.words[record+2u]));
}
mat3 traceHistoryViewBasis() {
    vec3 forward=traceNormalize(camera.eyeForward.xyz,vec3(0,0,1));
    vec3 right=vec3(camera.viewProjection[0][0],camera.viewProjection[1][0],camera.viewProjection[2][0]);
    vec3 up=vec3(camera.viewProjection[0][1],camera.viewProjection[1][1],camera.viewProjection[2][1]);
    // Authored asymmetric lenses add a forward component to projection rows.
    // Remove it before using those rows as a length-preserving camera basis.
    right=traceNormalize(right-forward*dot(right,forward),vec3(1,0,0));
    up=traceNormalize(up-forward*dot(up,forward)-right*dot(up,right),vec3(0,1,0));
    return transpose(mat3(right,up,forward));
}
bool tracePreviousRecord(vec3 worldPosition,vec3 position,vec3 normal,vec3 geometric,
                         vec3 reflected,uint material,float roughness,float pixelFootprint,
                         bool viewDependent,out uint history,out uint count,out bool specularCompatible) {
    uint role=camera.eyePosition.w==2.0?1u:0u;
    history=0u;count=0u;specularCompatible=false;
    if((traceVisibility.temporal.z&(1u<<role))==0u)return false;
    // A weapon follows the camera. Its history is in camera space, whereas
    // the world uses its previous projection and world-space surface guides.
    mat4 projection=role==1u?camera.viewProjection:tracePrevious.previousMVP[role];
    if(role==1u) {
        for(uint row=0u;row<2u;++row) {
            float currentScale=length(vec3(camera.viewProjection[0][row],camera.viewProjection[1][row],
                                          camera.viewProjection[2][row]));
            float previousScale=length(vec3(tracePrevious.previousMVP[role][0][row],
                tracePrevious.previousMVP[role][1][row],tracePrevious.previousMVP[role][2][row]));
            if(abs(currentScale-previousScale)>max(currentScale,previousScale)*.001)return false;
        }
    }
    vec4 clip=projection*vec4(worldPosition,1);
    if(any(isnan(clip)) || any(isinf(clip)) || clip.w<=0.0)return false;
    vec4 viewport=tracePrevious.previousViewport[role];
    vec2 normalized=clip.xy/clip.w*.5+.5;
    if(any(lessThan(normalized,vec2(0))) || any(greaterThanEqual(normalized,vec2(1))))return false;
    vec2 pixel=normalized*viewport.zw+viewport.xy;
    if(any(lessThan(pixel,vec2(0))) || any(greaterThanEqual(pixel,vec2(traceVisibility.extent.xy))))return false;
    uint scale=max(traceVisibility.extent.w,1u);uvec2 grid=traceLightingGrid();
    ivec2 base=ivec2(floor((pixel-(float(scale/2u)+.5))/float(scale)));
    float positionTolerance=max(traceEpsilon*4.0,pixelFootprint*float(scale)*1.5);
    float planeTolerance=max(traceEpsilon*4.0,pixelFootprint*float(scale)*.25);
    float best=1e30;
    // Four previous representatives cover the projected grid cell without
    // blending across a material, silhouette, or disoccluded surface.
    for(int y=0;y<2;++y)for(int x=0;x<2;++x) {
        ivec2 cell=base+ivec2(x,y);
        if(any(lessThan(cell,ivec2(0))) || any(greaterThanEqual(cell,ivec2(grid))))continue;
        uint candidate=(role*grid.x*grid.y+uint(cell.y)*grid.x+uint(cell.x))*traceHistoryWords;
        uint previousCount=tracePrevious.words[candidate+12u]&31u;
        if(tracePrevious.words[candidate+7u]!=material || previousCount==0u ||
           previousCount>traceHistoryLimit || tracePrevious.words[candidate+13u]+1u!=traceVisibility.temporal.x ||
           abs(uintBitsToFloat(tracePrevious.words[candidate+11u])-roughness)>=.01)continue;
        vec3 previousNormal=traceNormalize(unpackSnorm4x8(tracePrevious.words[candidate+3u]).xyz,normal);
        vec3 previousGeometric=traceNormalize(unpackSnorm4x8(tracePrevious.words[candidate+15u]).xyz,geometric);
        if(dot(previousNormal,normal)<=.995 || dot(previousGeometric,geometric)<=.95)continue;
        vec3 delta=tracePreviousLoad3(candidate+8u)-position;
        float distanceSquared=dot(delta,delta);
        float plane=max(abs(dot(delta,geometric)),abs(dot(delta,previousGeometric)));
        if(distanceSquared>positionTolerance*positionTolerance || plane>planeTolerance)continue;
        float score=distanceSquared/(positionTolerance*positionTolerance)+plane/planeTolerance;
        if(score<best) {
            best=score;history=candidate;count=previousCount;
            vec3 previousReflected=traceNormalize(unpackSnorm4x8(tracePrevious.words[candidate+14u]).xyz,reflected);
            specularCompatible=!viewDependent || traceReflectionCompatible(previousReflected,reflected,roughness);
            specularCompatible=specularCompatible &&
                (tracePrevious.words[candidate+12u]>>5u)==traceReflectionFingerprint(material);
        }
    }
    return count>0u;
}
void traceLightingStore(uint record,vec3 diffuseLight,vec3 specularLight,vec3 currentDirect,vec3 currentDirectSpecular,
                        vec3 position,vec3 normal,vec3 geometric,vec3 incoming,uint material,float roughness,
                        float pixelFootprint,bool viewDependent) {
    diffuseLight=traceFiniteLighting(diffuseLight);specularLight=traceFiniteLighting(specularLight);
    currentDirect=traceFiniteLighting(currentDirect);
    currentDirectSpecular=traceFiniteLighting(currentDirectSpecular);
    uint scale=max(traceVisibility.extent.w,1u);
    uint history=traceHistoryRecord(uvec2(gl_FragCoord.xy)/scale);
    // Foreground surface guides follow the camera, but a reflected ray samples
    // WORLD radiance. Keep its direction in WORLD even during a rigid camera
    // turn, reusing the former incoming-direction word and its count bits.
    vec3 reflected=reflect(incoming,normal);
    vec3 historyPosition=position,historyNormal=normal,historyGeometric=geometric;
    if(camera.eyePosition.w==2.0) {
        mat3 basis=traceHistoryViewBasis();
        historyPosition=basis*(position-camera.eyePosition.xyz);
        historyNormal=basis*normal;historyGeometric=basis*geometric;
    }
    uint previous,previousCount,serial=traceVisibility.temporal.x;
    bool reflectionCompatible;
    bool reuse=tracePreviousRecord(position,historyPosition,historyNormal,historyGeometric,reflected,
        material,roughness,pixelFootprint,viewDependent,previous,previousCount,reflectionCompatible);
    bool diffuseReuse=reuse && !traceChangedDiffuse(position,normal,incoming,material,pixelFootprint);
    uint count=diffuseReuse?min(previousCount+1u,traceHistoryLimit):1u;
    uint specularCount=1u,directCount=1u,directSpecularCount=1u;
    if(reuse) {
        float previousWeight=1.0-1.0/float(count);
        if(diffuseReuse)diffuseLight+=(tracePreviousLoad3(previous)-diffuseLight)*previousWeight;
        if(reflectionCompatible && (!viewDependent || !traceChangedReflection(position,normal,incoming,roughness,pixelFootprint))) {
            specularCount=min((tracePrevious.words[previous+3u]>>24u)+1u,traceHistoryLimit);
            specularLight+=(tracePreviousLoad3(previous+4u)-specularLight)*(1.0-1.0/float(specularCount));
        }
        // One light is already deterministic. Keeping it current also avoids
        // reprojecting a nearby representative across a sharp shadow edge.
        if(traceScene.counts.z>1u && !traceChangedShadow(position,normal)) {
            directCount=min((tracePrevious.words[previous+15u]>>24u)+1u,traceHistoryLimit);
            currentDirect+=(tracePreviousLoad3(previous+16u)-currentDirect)*(1.0-1.0/float(directCount));
            if(reflectionCompatible) {
                directSpecularCount=min((tracePrevious.words[previous+14u]>>24u)+1u,traceHistoryLimit);
                currentDirectSpecular+=(tracePreviousLoad3(previous+19u)-currentDirectSpecular)*(1.0-1.0/float(directSpecularCount));
            }
        }
    }
    for(uint lane=0u;lane<3u;++lane) {
        uint diffuseWord=floatBitsToUint(diffuseLight[lane]),specularWord=floatBitsToUint(specularLight[lane]);
        uint positionWord=floatBitsToUint(position[lane]);
        // Direct visibility has its own history. A changed occluder invalidates
        // only receivers whose shadow segments cross its old/new bounds.
        traceVisibility.words[record+lane]=floatBitsToUint(diffuseLight[lane]+currentDirect[lane]);
        traceVisibility.words[history+lane]=diffuseWord;
        traceVisibility.words[record+4u+lane]=floatBitsToUint(specularLight[lane]+currentDirectSpecular[lane]);
        traceVisibility.words[history+4u+lane]=specularWord;
        traceVisibility.words[history+16u+lane]=floatBitsToUint(currentDirect[lane]);
        traceVisibility.words[history+19u+lane]=floatBitsToUint(currentDirectSpecular[lane]);
        traceVisibility.words[record+8u+lane]=positionWord;
        traceVisibility.words[history+8u+lane]=floatBitsToUint(historyPosition[lane]);
    }
    uint normalWord=packSnorm4x8(vec4(normal,0)),roughnessWord=floatBitsToUint(roughness);
    traceVisibility.words[record+3u]=normalWord;
    traceVisibility.words[history+3u]=(packSnorm4x8(vec4(historyNormal,0))&0x00ffffffu)|(specularCount<<24u);
    traceVisibility.words[record+11u]=roughnessWord;traceVisibility.words[history+11u]=roughnessWord;
    traceVisibility.words[history+7u]=material;
    traceVisibility.words[history+12u]=count|(traceReflectionFingerprint(material)<<5u);
    traceVisibility.words[history+13u]=serial;
    traceVisibility.words[history+14u]=(packSnorm4x8(vec4(reflected,0))&0x00ffffffu)|(directSpecularCount<<24u);
    traceVisibility.words[history+15u]=(packSnorm4x8(vec4(historyGeometric,0))&0x00ffffffu)|(directCount<<24u);
    memoryBarrierBuffer();
    atomicExchange(traceVisibility.words[record+7u],material);
}
bool traceLightingFilter(uvec2 pixel,vec3 position,vec3 normal,uint material,float roughness,
                         float pixelFootprint,out vec3 diffuseLight,out vec3 specularLight) {
    uint scale=max(traceVisibility.extent.w,1u);uvec2 grid=traceLightingGrid();
    ivec2 center=ivec2(pixel/scale);float diffuseWeight=0.0,specularWeight=0.0;
    diffuseLight=vec3(0);specularLight=vec3(0);
    float footprint=max(pixelFootprint*float(scale),traceEpsilon*4.0);
    float planeTolerance=max(footprint*.75,traceEpsilon*8.0);
    float reflectionSigma=max(.4,roughness*2.0);
    // Average linear HDR lighting before the display clamp. Material, normal
    // and world-plane guides preserve silhouettes and separate nearby walls;
    // the primary texture is sampled at full resolution after this filter.
    [[dont_unroll]] for(int y=-2;y<=2;++y) [[dont_unroll]] for(int x=-2;x<=2;++x) {
        ivec2 cell=center+ivec2(x,y);
        if(any(lessThan(cell,ivec2(0))) || any(greaterThanEqual(cell,ivec2(grid))))continue;
        uint record=traceLightingRecord(uvec2(cell));
        if(traceVisibility.words[record+7u]!=material)continue;
        vec3 sampleNormal=unpackSnorm4x8(traceVisibility.words[record+3u]).xyz;
        float cosine=dot(normal,traceNormalize(sampleNormal,normal));
        if(cosine<.85)continue;
        vec3 delta=traceLightingLoad3(record+8u)-position;
        float plane=abs(dot(delta,normal));
        if(plane>planeTolerance || dot(delta,delta)>footprint*footprint*64.0)continue;
        float sampleRoughness=uintBitsToFloat(traceVisibility.words[record+11u]);
        if(abs(sampleRoughness-roughness)>.05)continue;
        vec2 representative=vec2(min(uvec2(cell)*scale+scale/2u,traceVisibility.extent.xy-1u))+.5;
        vec2 offset=(representative-(vec2(pixel)+.5))/float(scale);
        float squared=dot(offset,offset);
        float geometry=exp(-plane*plane/(planeTolerance*planeTolerance));
        geometry*=exp(-dot(delta,delta)/(footprint*footprint*32.0));
        float diffuse=geometry*pow(max(cosine,0.0),32.0)*exp(-squared/8.0);
        vec3 incoming=traceNormalize(position-camera.eyePosition.xyz,-normal);
        vec3 sampleIncoming=traceNormalize(position+delta-camera.eyePosition.xyz,-normal);
        vec3 reflectedDelta=reflect(sampleIncoming,traceNormalize(sampleNormal,normal))-reflect(incoming,normal);
        float angularSigma=max(.025,roughness*roughness);
        float specular=geometry*exp(-dot(reflectedDelta,reflectedDelta)/(2.0*angularSigma*angularSigma))*
            exp(-squared/(2.0*reflectionSigma*reflectionSigma));
        // Incremental means avoid overflow from summing several individually
        // finite HDR samples. There is no radiance-dependent rejection bias.
        float nextDiffuse=diffuseWeight+diffuse,nextSpecular=specularWeight+specular;
        if(diffuse>0.0)diffuseLight+=(traceLightingLoad3(record)-diffuseLight)*(diffuse/nextDiffuse);
        if(specular>0.0)specularLight+=(traceLightingLoad3(record+4u)-specularLight)*(specular/nextSpecular);
        diffuseWeight=nextDiffuse;specularWeight=nextSpecular;
    }
    return diffuseWeight>0.0 && specularWeight>0.0;
}
#endif
