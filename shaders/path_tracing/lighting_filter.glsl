#ifndef H2_PATH_TRACING_LIGHTING_FILTER_GLSL
#define H2_PATH_TRACING_LIGHTING_FILTER_GLSL
// Binding6 contains the full-resolution source-W depth prefix followed by
// twelve-word HDR lighting records on a bounded grid and two sixteen-word
// persistent histories (world/foreground). Its32-byte header contains extent
// {fullWidth,fullHeight,depthWordCount,lightingScale} and temporal{frameSerial}.
// Material word7 is an atomic first-writer claim, so overlapping primitives
// never tear a record or update the same history cell concurrently.
const uint traceLightingWords=12u,traceLightingInvalid=0xffffffffu;
const uint traceHistoryWords=16u,traceHistoryLimit=16u;
// An immutable snapshot makes reprojection independent of fragment execution
// order. The current frame writes binding6; every history lookup reads9.
layout(set=1,binding=9,std430) readonly restrict buffer TracePreviousHistory {
    mat4 previousMVP[2];
    vec4 previousViewport[2];
    uint words[];
} tracePrevious;
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
                         vec3 incoming,uint material,float roughness,float pixelFootprint,
                         bool viewDependent,out uint history,out uint count) {
    uint role=camera.eyePosition.w==2.0?1u:0u;
    history=0u;count=0u;
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
        uint previousCount=tracePrevious.words[candidate+12u];
        if(tracePrevious.words[candidate+7u]!=material || previousCount==0u ||
           previousCount>traceHistoryLimit || tracePrevious.words[candidate+13u]+1u!=traceVisibility.temporal.x ||
           abs(uintBitsToFloat(tracePrevious.words[candidate+11u])-roughness)>=.01)continue;
        vec3 previousNormal=traceNormalize(unpackSnorm4x8(tracePrevious.words[candidate+3u]).xyz,normal);
        vec3 previousGeometric=traceNormalize(unpackSnorm4x8(tracePrevious.words[candidate+15u]).xyz,geometric);
        if(dot(previousNormal,normal)<=.995 || dot(previousGeometric,geometric)<=.95)continue;
        if(viewDependent) {
            vec3 previousIncoming=traceNormalize(unpackSnorm4x8(tracePrevious.words[candidate+14u]).xyz,incoming);
            if(dot(previousIncoming,incoming)<=.9995)continue;
        }
        vec3 delta=tracePreviousLoad3(candidate+8u)-position;
        float distanceSquared=dot(delta,delta);
        float plane=max(abs(dot(delta,geometric)),abs(dot(delta,previousGeometric)));
        if(distanceSquared>positionTolerance*positionTolerance || plane>planeTolerance)continue;
        float score=distanceSquared/(positionTolerance*positionTolerance)+plane/planeTolerance;
        if(score<best) {best=score;history=candidate;count=previousCount;}
    }
    return count>0u;
}
void traceLightingStore(uint record,vec3 diffuseLight,vec3 specularLight,vec3 currentDirect,
                        vec3 position,vec3 normal,vec3 geometric,vec3 incoming,uint material,float roughness,
                        float pixelFootprint,bool viewDependent) {
    diffuseLight=traceFiniteLighting(diffuseLight);specularLight=traceFiniteLighting(specularLight);
    currentDirect=traceFiniteLighting(currentDirect);
    uint scale=max(traceVisibility.extent.w,1u);
    uint history=traceHistoryRecord(uvec2(gl_FragCoord.xy)/scale);
    vec3 historyPosition=position,historyNormal=normal,historyGeometric=geometric,historyIncoming=incoming;
    if(camera.eyePosition.w==2.0) {
        mat3 basis=traceHistoryViewBasis();
        historyPosition=basis*(position-camera.eyePosition.xyz);
        historyNormal=basis*normal;historyGeometric=basis*geometric;historyIncoming=basis*incoming;
    }
    uint previous,previousCount,serial=traceVisibility.temporal.x;
    bool reuse=tracePreviousRecord(position,historyPosition,historyNormal,historyGeometric,historyIncoming,
        material,roughness,pixelFootprint,viewDependent,previous,previousCount);
    uint count=reuse?min(previousCount+1u,traceHistoryLimit):1u;
    if(reuse) {
        float previousWeight=1.0-1.0/float(count);
        diffuseLight+=(tracePreviousLoad3(previous)-diffuseLight)*previousWeight;
        specularLight+=(tracePreviousLoad3(previous+4u)-specularLight)*previousWeight;
    }
    for(uint lane=0u;lane<3u;++lane) {
        uint diffuseWord=floatBitsToUint(diffuseLight[lane]),specularWord=floatBitsToUint(specularLight[lane]);
        uint positionWord=floatBitsToUint(position[lane]);
        // Persistent diffuse history contains stochastic transport only.
        // The current resolve record also carries this frame's deterministic
        // direct visibility, so an animated caster cannot leave its old shadow.
        traceVisibility.words[record+lane]=floatBitsToUint(diffuseLight[lane]+currentDirect[lane]);
        traceVisibility.words[history+lane]=diffuseWord;
        traceVisibility.words[record+4u+lane]=specularWord;traceVisibility.words[history+4u+lane]=specularWord;
        traceVisibility.words[record+8u+lane]=positionWord;
        traceVisibility.words[history+8u+lane]=floatBitsToUint(historyPosition[lane]);
    }
    uint normalWord=packSnorm4x8(vec4(normal,0)),roughnessWord=floatBitsToUint(roughness);
    traceVisibility.words[record+3u]=normalWord;
    traceVisibility.words[history+3u]=packSnorm4x8(vec4(historyNormal,0));
    traceVisibility.words[record+11u]=roughnessWord;traceVisibility.words[history+11u]=roughnessWord;
    traceVisibility.words[history+7u]=material;traceVisibility.words[history+12u]=count;
    traceVisibility.words[history+13u]=serial;
    traceVisibility.words[history+14u]=packSnorm4x8(vec4(historyIncoming,0));
    traceVisibility.words[history+15u]=packSnorm4x8(vec4(historyGeometric,0));
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
        float specular=geometry*pow(max(cosine,0.0),256.0)*
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
