#ifndef H2_PATH_TRACING_LIGHTING_FILTER_GLSL
#define H2_PATH_TRACING_LIGHTING_FILTER_GLSL
// Binding6 contains the full-resolution source-W depth prefix followed by
// sixteen-word HDR lighting records on a bounded grid, two twenty-two-word
// persistent histories (world/foreground) and the spatial denoiser's two
// four-word ping-pong buffers. Its32-byte header contains extent
// {fullWidth,fullHeight,depthWordCount,lightingScale} and temporal{frameSerial}.
// Material word7 is an atomic first-writer claim, so overlapping primitives
// never tear a record or update the same history cell concurrently.
const uint traceLightingWords=20u,traceLightingInvalid=0xffffffffu;
// Diffuse and direct histories accumulate up to 31 frames (5-bit counts);
// reflections, which change with the view, at most 12.
const uint traceHistoryWords=24u,traceHistoryLimit=31u,traceSpecularHistoryLimit=12u;
// The high bit of the packed geometric-normal/direct-count word identifies
// a direct estimator that includes live area lights.
const uint traceDirectAreaBit=0x80000000u;
// An immutable snapshot makes reprojection independent of fragment execution
// order. The current frame writes binding6; every history lookup reads9.
layout(set=1,binding=9,std430) readonly restrict buffer TracePreviousHistory {
    mat4 previousMVP[2];
    vec4 previousViewport[2];
    vec4 changedOccluderMin,changedOccluderMax; // Union of changes; max.w: local box count.
    vec4 changedLightMin,changedLightMax;
    // Local changed-geometry boxes: min (w: change flags 1 moved, 2 shaded), max.
    vec4 changedBoxes[64];
    uint words[];
} tracePrevious;
uint traceChangeBoxCount() {return min(uint(tracePrevious.changedOccluderMax.w),32u);}
// Slab test of the segment origin + t*direction, t in [0, maximum], with a box.
bool traceSegmentHitsBox(vec3 origin,vec3 direction,float maximum,vec3 minimum,vec3 maximumCorner) {
    float nearDistance=0.0,farDistance=maximum;
    for(uint axis=0u;axis<3u;++axis) {
        if(abs(direction[axis])<1.e-8) {
            if(origin[axis]<minimum[axis] || origin[axis]>maximumCorner[axis])return false;
        }else {
            float a=(minimum[axis]-origin[axis])/direction[axis];
            float b=(maximumCorner[axis]-origin[axis])/direction[axis];
            nearDistance=max(nearDistance,min(a,b));farDistance=min(farDistance,max(a,b));
        }
    }
    return farDistance>=nearDistance;
}
bool traceChangedAreaShadow(vec3 position,vec3 normal,uint boxes) {
    uint count=min(traceUnbakedEmitterCount(),8u);
    if(count==0u)return false;
    uint header=floatBitsToUint(traceScene.environment.w)+2u*traceScene.counts.w;
    uint first=header+4u+tracePixels[header+1u]*traceEmitterGroupWords;
    if(first>uint(tracePixels.length()) || uint(tracePixels.length())-first<6u*count)return true;
    const vec3 padding=vec3(traceEpsilon*8.0);
    for(uint box=0u;box<boxes;++box) {
        vec4 changedMinimum=tracePrevious.changedBoxes[2u*box];
        if((uint(changedMinimum.w)&1u)==0u)continue;
        vec3 changedMaximum=tracePrevious.changedBoxes[2u*box+1u].xyz;
        vec3 center=changedMinimum.xyz*.5+changedMaximum*.5,halfExtent=changedMaximum*.5-changedMinimum.xyz*.5;
        if(dot(center-position,normal)+dot(halfExtent,abs(normal))<-traceEpsilon*8.0)continue;
        for(uint bound=0u;bound<count;++bound) {
            uint record=first+6u*bound;
            vec3 minimum=uintBitsToFloat(uvec3(tracePixels[record],tracePixels[record+1u],tracePixels[record+2u]));
            vec3 maximum=uintBitsToFloat(uvec3(tracePixels[record+3u],tracePixels[record+4u],tracePixels[record+5u]));
            vec3 lightCenter=minimum*.5+maximum*.5,lightExtent=maximum*.5-minimum*.5;
            vec3 direction=lightCenter-position;
            if(dot(direction,normal)+dot(lightExtent,abs(normal))<=0.0)continue;
            // Every segment to the emitter box stays within its half-extents
            // of the segment to its center. Include the offset at ray origin.
            if(traceSegmentHitsBox(position,direction,1.0,
                changedMinimum.xyz-lightExtent-padding,changedMaximum+lightExtent+padding))return true;
        }
    }
    return false;
}
bool traceChangedShadow(vec3 position,vec3 normal,bool areaDirect) {
    if((traceVisibility.temporal.w&(areaDirect?3u:1u))!=0u)return true;
    if(tracePrevious.changedLightMin.w!=0.0 &&
       all(greaterThanEqual(position,tracePrevious.changedLightMin.xyz)) &&
       all(lessThanEqual(position,tracePrevious.changedLightMax.xyz)))return true;
    uint changes=uint(tracePrevious.changedOccluderMin.w),boxes=traceChangeBoxCount();
    if((changes&1u)==0u)return false;
    if(areaDirect && traceChangedAreaShadow(position,normal,boxes))return true;
    // Bound validation cost independently of unusually large source banks.
    // These are conservative slab tests only, never additional BVH queries.
    if(traceScene.counts.z>64u)return true;
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
        for(uint box=0u;box<boxes;++box) {
            vec4 minimum=tracePrevious.changedBoxes[2u*box],maximum=tracePrevious.changedBoxes[2u*box+1u];
            if((uint(minimum.w)&1u)==0u)continue;
            if(traceSegmentHitsBox(position,direction,distant?1.e10:distance,
                minimum.xyz-vec3(traceEpsilon*8.0),maximum.xyz+vec3(traceEpsilon*8.0)))return true;
        }
    }
    return false;
}
bool traceChangedReflection(vec3 position,vec3 normal,vec3 incoming,float roughness,float pixelFootprint) {
    if(tracePrevious.changedOccluderMin.w==0.0)return false;
    // The receiver can stay still while a reflected object leaves or enters
    // its lobe. Test each changed box (old/new bounds), expanded for the pixel
    // footprint and GGX core width; slab tests only, no scene traversal.
    vec3 direction=reflect(incoming,normal);
    uint boxes=traceChangeBoxCount();
    for(uint box=0u;box<boxes;++box) {
        vec3 minimum=tracePrevious.changedBoxes[2u*box].xyz,maximum=tracePrevious.changedBoxes[2u*box+1u].xyz;
        vec3 center=minimum*.5+maximum*.5,halfExtent=maximum*.5-minimum*.5;
        float reach=dot(center-position,direction)+dot(halfExtent,abs(direction));
        if(reach<0.0)continue;
        float padding=max(pixelFootprint*float(max(traceVisibility.extent.w,1u)),traceEpsilon*8.0)+
            min(reach,1.e10)*roughness*roughness*4.0;
        if(traceSegmentHitsBox(position,direction,1.e10,minimum-vec3(padding),maximum+vec3(padding)))return true;
    }
    return false;
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
    // reflection. Reject only receivers whose diffuse hemisphere reaches a
    // changed box that also subtends a meaningful solid angle (half-angle
    // above ~6 degrees): a small change far away barely alters indirect light,
    // and its history adapts within the accumulation window.
    float padding=max(pixelFootprint*float(max(traceVisibility.extent.w,1u)),traceEpsilon*8.0);
    uint boxes=traceChangeBoxCount();
    for(uint box=0u;box<boxes;++box) {
        vec4 minimum=tracePrevious.changedBoxes[2u*box],maximum=tracePrevious.changedBoxes[2u*box+1u];
        if((uint(minimum.w)&2u)==0u)continue;
        vec3 center=(minimum.xyz+maximum.xyz)*.5,halfExtent=(maximum.xyz-minimum.xyz)*.5;
        if(dot(center-position,normal)+dot(halfExtent,abs(normal))<-padding)continue;
        vec3 offset=center-position;
        float radius2=dot(halfExtent,halfExtent),distance2=dot(offset,offset);
        if(radius2>=distance2*.01)return true;
    }
    return false;
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
// Record lanes 12-13 carry the denoiser's guides: pixel footprint in WORLD
// units and the accumulated diffuse sample count. Lanes 16-17 (history
// 22-23) hold the accumulated luminance-weighted light direction (half3);
// 18-19 pad the record to whole 16-byte lanes.
const uint traceRecordFootprint=12u,traceRecordCount=13u,traceRecordIncident=16u,traceHistoryIncident=22u;
vec3 traceLoadIncident(uint first,bool previous) {
    uint xy=previous?tracePrevious.words[first]:traceVisibility.words[first];
    uint z=previous?tracePrevious.words[first+1u]:traceVisibility.words[first+1u];
    vec3 value=vec3(unpackHalf2x16(xy),unpackHalf2x16(z).x);
    return any(isnan(value)) || any(isinf(value))?vec3(0):value;
}
void traceStoreIncident(uint first,vec3 value) {
    value=clamp(value,vec3(-60000.0),vec3(60000.0));
    traceVisibility.words[first]=packHalf2x16(value.xy);traceVisibility.words[first+1u]=packHalf2x16(vec2(value.z,0));
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
// History reused by one cell: each channel blended from up to four previous
// representatives, weighted bilinearly by where this surface point projects
// between them, and only among those that pass every surface test. Blending
// (rather than choosing the single best cell) keeps reprojected lighting
// continuous while the camera moves by sub-cell amounts.
struct TraceHistory {
    vec3 diffuse,specular,direct,directSpecular,incident;
    float count,specularCount,directCount,directSpecularCount;
    float weight,specularWeight,directWeight,directSpecularWeight;
};
bool tracePreviousHistory(vec3 worldPosition,vec3 position,vec3 normal,vec3 geometric,
                          vec3 reflected,uint material,float roughness,float pixelFootprint,
                          bool viewDependent,bool areaDirect,out TraceHistory result) {
    uint role=camera.eyePosition.w==2.0?1u:0u;
    result.diffuse=result.specular=result.direct=result.directSpecular=result.incident=vec3(0);
    result.count=result.specularCount=result.directCount=result.directSpecularCount=0.0;
    result.weight=result.specularWeight=result.directWeight=result.directSpecularWeight=0.0;
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
    vec2 gridPosition=(pixel-(float(scale/2u)+.5))/float(scale);
    ivec2 base=ivec2(floor(gridPosition));vec2 fraction=gridPosition-vec2(base);
    float positionTolerance=max(traceEpsilon*4.0,pixelFootprint*float(scale)*1.5);
    float planeTolerance=max(traceEpsilon*4.0,pixelFootprint*float(scale)*.25);
    uint fingerprint=traceReflectionFingerprint(material);
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
        if(dot(previousNormal,normal)<=.99 || dot(previousGeometric,geometric)<=.95)continue;
        vec3 delta=tracePreviousLoad3(candidate+8u)-position;
        float plane=max(abs(dot(delta,geometric)),abs(dot(delta,previousGeometric)));
        if(dot(delta,delta)>positionTolerance*positionTolerance || plane>planeTolerance)continue;
        float bilinear=(x==0?1.0-fraction.x:fraction.x)*(y==0?1.0-fraction.y:fraction.y);
        float weight=max(bilinear,1.e-3);
        result.weight+=weight;
        result.diffuse+=tracePreviousLoad3(candidate)*weight;
        result.incident+=traceLoadIncident(candidate+traceHistoryIncident,true)*weight;
        result.count+=float(previousCount)*weight;
        uint directHistory=tracePrevious.words[candidate+15u];
        bool directCompatible=((directHistory&traceDirectAreaBit)!=0u)==areaDirect;
        if(directCompatible) {
            result.direct+=tracePreviousLoad3(candidate+16u)*weight;
            result.directCount+=float((directHistory>>24u)&traceHistoryLimit)*weight;
            result.directWeight+=weight;
        }
        vec3 previousReflected=traceNormalize(unpackSnorm4x8(tracePrevious.words[candidate+14u]).xyz,reflected);
        bool specularCompatible=(!viewDependent || traceReflectionCompatible(previousReflected,reflected,roughness)) &&
            (tracePrevious.words[candidate+12u]>>5u)==fingerprint;
        if(specularCompatible) {
            result.specularWeight+=weight;
            result.specular+=tracePreviousLoad3(candidate+4u)*weight;
            result.specularCount+=float(tracePrevious.words[candidate+3u]>>24u)*weight;
            if(directCompatible) {
                result.directSpecular+=tracePreviousLoad3(candidate+19u)*weight;
                result.directSpecularCount+=float(tracePrevious.words[candidate+14u]>>24u)*weight;
                result.directSpecularWeight+=weight;
            }
        }
    }
    if(result.weight<=0.0)return false;
    float inverse=1.0/result.weight;
    result.diffuse*=inverse;result.count*=inverse;
    result.incident*=inverse;
    if(result.specularWeight>0.0) {
        float specular=1.0/result.specularWeight;
        result.specular*=specular;result.specularCount*=specular;
    }
    if(result.directWeight>0.0) {
        result.direct/=result.directWeight;result.directCount/=result.directWeight;
    }
    if(result.directSpecularWeight>0.0) {
        result.directSpecular/=result.directSpecularWeight;result.directSpecularCount/=result.directSpecularWeight;
    }
    return true;
}
// The raster lighting pass publishes one surface request per grid cell; the
// compute trace reads it, replaces its free lanes with lighting and stores the
// temporal history. Lanes: 0 geometric normal, 1-2 colour guide (half4),
// 3 shading normal, 4 pixel footprint, 5 flags, 6 representative pixel,
// 7 material (published last), 8-10 WORLD position, 11 roughness.
const uint traceRequestViewDependent=1u;
void traceLightingRequest(uint record,uvec2 pixel,vec3 position,vec3 normal,vec3 geometric,vec4 colorGuide,
                          uint material,float roughness,float pixelFootprint,bool viewDependent) {
    traceVisibility.words[record]=packSnorm4x8(vec4(geometric,0));
    traceVisibility.words[record+1u]=packHalf2x16(colorGuide.rg);
    traceVisibility.words[record+2u]=packHalf2x16(colorGuide.ba);
    traceVisibility.words[record+3u]=packSnorm4x8(vec4(normal,0));
    traceVisibility.words[record+4u]=floatBitsToUint(pixelFootprint);
    traceVisibility.words[record+5u]=viewDependent?traceRequestViewDependent:0u;
    traceVisibility.words[record+6u]=(pixel.x&0xffffu)|(pixel.y<<16u);
    for(uint lane=0u;lane<3u;++lane)traceVisibility.words[record+8u+lane]=floatBitsToUint(position[lane]);
    traceVisibility.words[record+11u]=floatBitsToUint(roughness);
    memoryBarrierBuffer();
    atomicExchange(traceVisibility.words[record+7u],material);
}
void traceLightingStore(uvec2 cell,uint record,vec3 diffuseLight,vec3 specularLight,vec3 currentDirect,vec3 currentDirectSpecular,
                        vec3 incident,vec3 position,vec3 normal,vec3 geometric,vec3 incoming,uint material,float roughness,
                        float pixelFootprint,bool viewDependent,bool areaDirect) {
    if(any(isnan(incident)) || any(isinf(incident)))incident=vec3(0);
    diffuseLight=traceFiniteLighting(diffuseLight);specularLight=traceFiniteLighting(specularLight);
    currentDirect=traceFiniteLighting(currentDirect);
    currentDirectSpecular=traceFiniteLighting(currentDirectSpecular);
    uint history=traceHistoryRecord(cell);
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
    uint serial=traceVisibility.temporal.x;
    TraceHistory previous;
    bool reuse=tracePreviousHistory(position,historyPosition,historyNormal,historyGeometric,reflected,
        material,roughness,pixelFootprint,viewDependent,areaDirect,previous);
    bool diffuseReuse=reuse && !traceChangedDiffuse(position,normal,incoming,material,pixelFootprint);
    uint count=diffuseReuse?min(uint(previous.count+.5)+1u,traceHistoryLimit):1u;
    uint specularCount=1u,directCount=1u,directSpecularCount=1u;
    if(reuse) {
        float previousWeight=1.0-1.0/float(count);
        if(diffuseReuse) {
            // One bright sample (an emitter found by chance) would swing the
            // whole direction: bound each frame's pull by the cell's light.
            float bound=2.0*max(dot(previous.diffuse+previous.direct,vec3(.2126,.7152,.0722)),1.e-3);
            float pull=length(incident);
            if(pull>bound)incident*=bound/pull;
            diffuseLight+=(previous.diffuse-diffuseLight)*previousWeight;
            incident+=(previous.incident-incident)*previousWeight;
        }
        bool reflectionCompatible=previous.specularWeight>0.0;
        if(reflectionCompatible && (!viewDependent || !traceChangedReflection(position,normal,incoming,roughness,pixelFootprint))) {
            // Reflections move with the view, so they keep a shorter history.
            specularCount=min(uint(previous.specularCount+.5)+1u,traceSpecularHistoryLimit);
            specularLight+=(previous.specular-specularLight)*(1.0-1.0/float(specularCount));
        }
        // One punctual light alone is deterministic. Area samples and
        // competing light sources benefit from separate direct histories.
        if((traceScene.counts.z>1u || areaDirect) && previous.directWeight>0.0 &&
           !traceChangedShadow(position,normal,areaDirect)) {
            directCount=min(uint(previous.directCount+.5)+1u,traceHistoryLimit);
            currentDirect+=(previous.direct-currentDirect)*(1.0-1.0/float(directCount));
            if(previous.directSpecularWeight>0.0) {
                directSpecularCount=min(uint(previous.directSpecularCount+.5)+1u,traceSpecularHistoryLimit);
                currentDirectSpecular+=(previous.directSpecular-currentDirectSpecular)*(1.0-1.0/float(directSpecularCount));
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
    traceVisibility.words[history+15u]=(packSnorm4x8(vec4(historyGeometric,0))&0x00ffffffu)|(directCount<<24u)|
        (areaDirect?traceDirectAreaBit:0u);
    traceVisibility.words[record+traceRecordFootprint]=floatBitsToUint(pixelFootprint);
    traceVisibility.words[record+traceRecordCount]=count;
    // The history keeps the luminance-weighted sum; the record (what the
    // resolve reads) holds it over this cell's own diffuse luminance: a
    // direction whose length is how directional the light is. A young
    // history's few random samples would look fully directional, so its
    // length grows with the history.
    traceStoreIncident(history+traceHistoryIncident,incident);
    float luminance=dot(diffuseLight+currentDirect,vec3(.2126,.7152,.0722));
    vec3 direction=luminance>1.e-5?incident/luminance:vec3(0);
    float directional=length(direction);
    if(directional>1.0)direction/=directional;
    traceStoreIncident(record+traceRecordIncident,direction*min(float(count)/8.0,1.0));
    traceVisibility.words[record+7u]=material;
}
// Upsamples the denoised lighting grid (denoise.comp) to one pixel, in up to
// three passes that stop at the first with any accepted cell:
//  0. the 2x2 cells whose representatives surround the pixel (its bilinear
//     footprint), of the same material on a compatible surface;
//  1. 5x5 cells of any material on a compatible surface. Lighting is
//     demodulated, so a decal or thin detail object whose own material owns
//     no grid cell borrows the lighting of the surface it lies on;
//  2. 5x5 cells of any surface, weighted only by distance (very thin
//     geometry such as foliage edges).
// Average linear HDR lighting before the display clamp; the primary texture
// is applied at full resolution afterwards. No ray is traced here.
// The light direction the resolve shades bump maps against: the stored
// per-cell directions (traceLightingStore) averaged over the 5x5 cells around
// the pixel that lie on the same surface, so a single noisy cell cannot flip
// the relief of the pixels beside it.
vec3 traceFilteredIncident(uvec2 pixel,vec3 position,vec3 normal,float pixelFootprint) {
    uint scale=max(traceVisibility.extent.w,1u);uvec2 grid=traceLightingGrid();
    ivec2 center=ivec2(pixel/scale);
    float footprint=max(pixelFootprint*float(scale),traceEpsilon*4.0);
    float planeTolerance=max(footprint*.75,traceEpsilon*8.0);
    vec3 sum=vec3(0);float weight=0.0;
    [[dont_unroll]] for(int y=-2;y<=2;++y) [[dont_unroll]] for(int x=-2;x<=2;++x) {
        ivec2 cell=center+ivec2(x,y);
        if(any(lessThan(cell,ivec2(0))) || any(greaterThanEqual(cell,ivec2(grid))))continue;
        uint record=traceLightingRecord(uvec2(cell));
        if(traceVisibility.words[record+7u]>=0x80000000u)continue;
        vec3 sampleNormal=traceNormalize(unpackSnorm4x8(traceVisibility.words[record+3u]).xyz,normal);
        float plane=abs(dot(traceLightingLoad3(record+8u)-position,normal));
        if(dot(normal,sampleNormal)<.7 || plane>planeTolerance*4.0)continue;
        float w=exp(-float(x*x+y*y)/8.0);
        sum+=traceLoadIncident(record+traceRecordIncident,false)*w;weight+=w;
    }
    return weight>0.0?sum/weight:vec3(0);
}
bool traceLightingFilter(uvec2 pixel,vec3 position,vec3 normal,uint material,float roughness,
                         float pixelFootprint,out vec3 diffuseLight,out vec3 specularLight,out vec3 incident) {
    uint scale=max(traceVisibility.extent.w,1u);uvec2 grid=traceLightingGrid();
    ivec2 center=ivec2(pixel/scale);
    // Lower-left cell of the bilinear footprint: representatives sit at
    // cell*scale+scale/2.
    ivec2 footprintBase=ivec2(floor((vec2(pixel)-float(scale/2u))/float(scale)));
    float footprint=max(pixelFootprint*float(scale),traceEpsilon*4.0);
    float planeTolerance=max(footprint*.75,traceEpsilon*8.0);
    float reflectionSigma=max(.4,roughness*2.0);
    float angularSigma=max(.025,roughness*roughness);
    vec3 incoming=traceNormalize(position-camera.eyePosition.xyz,-normal);
    vec3 reflected=reflect(incoming,normal);
    [[dont_unroll]] for(uint pass=0u;pass<3u;++pass) {
        ivec2 first=pass==0u?footprintBase:center-2,last=pass==0u?footprintBase+1:center+2;
        float diffuseWeight=0.0,specularWeight=0.0;
        diffuseLight=vec3(0);specularLight=vec3(0);incident=vec3(0);
        [[dont_unroll]] for(int y=first.y;y<=last.y;++y) [[dont_unroll]] for(int x=first.x;x<=last.x;++x) {
            ivec2 cell=ivec2(x,y);
            if(any(lessThan(cell,ivec2(0))) || any(greaterThanEqual(cell,ivec2(grid))))continue;
            uint record=traceLightingRecord(uvec2(cell));
            uint sampleMaterial=traceVisibility.words[record+7u];
            if(sampleMaterial>=0x80000000u || (pass==0u && sampleMaterial!=material))continue;
            vec3 sampleNormal=traceNormalize(unpackSnorm4x8(traceVisibility.words[record+3u]).xyz,normal);
            float cosine=dot(normal,sampleNormal);
            vec3 delta=traceLightingLoad3(record+8u)-position;
            float plane=abs(dot(delta,normal));
            vec2 representative=vec2(min(uvec2(cell)*scale+scale/2u,traceVisibility.extent.xy-1u))+.5;
            vec2 offset=(representative-(vec2(pixel)+.5))/float(scale);
            float squared=dot(offset,offset);
            float diffuse,geometry;
            if(pass<2u) {
                if(cosine<.85 || plane>planeTolerance || dot(delta,delta)>footprint*footprint*64.0)continue;
                geometry=exp(-plane*plane/(planeTolerance*planeTolerance))*exp(-dot(delta,delta)/(footprint*footprint*32.0));
                diffuse=geometry*pow(max(cosine,0.0),32.0)*exp(-squared/8.0);
            } else {
                geometry=1.0/(1.0+squared);diffuse=geometry;
            }
            float sampleRoughness=uintBitsToFloat(traceVisibility.words[record+11u]);
            float specular=0.0;
            // Reflections depend on the material and view: only a cell of the
            // same material lends its specular. Diffuse is demodulated and is
            // shared across surfaces in passes 1-2.
            if(sampleMaterial==material && abs(sampleRoughness-roughness)<=.05) {
                vec3 sampleIncoming=traceNormalize(position+delta-camera.eyePosition.xyz,-normal);
                vec3 reflectedDelta=reflect(sampleIncoming,sampleNormal)-reflected;
                specular=geometry*exp(-dot(reflectedDelta,reflectedDelta)/(2.0*angularSigma*angularSigma))*
                    exp(-squared/(2.0*reflectionSigma*reflectionSigma));
            }
            // Incremental means avoid overflow from summing several individually
            // finite HDR samples. There is no radiance-dependent rejection bias.
            float nextDiffuse=diffuseWeight+diffuse,nextSpecular=specularWeight+specular;
            if(diffuse>0.0) {
                diffuseLight+=(traceLightingLoad3(record)-diffuseLight)*(diffuse/nextDiffuse);
                incident+=(traceLoadIncident(record+traceRecordIncident,false)-incident)*(diffuse/nextDiffuse);
            }
            if(specular>0.0)specularLight+=(traceLightingLoad3(record+4u)-specularLight)*(specular/nextSpecular);
            diffuseWeight=nextDiffuse;specularWeight=nextSpecular;
        }
        if(diffuseWeight>0.0)return true;
    }
    diffuseLight=vec3(0);specularLight=vec3(0);incident=vec3(0);
    return false;
}
#endif
