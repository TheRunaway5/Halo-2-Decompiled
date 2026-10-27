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
void traceLightingStore(uint record,vec3 diffuseLight,vec3 specularLight,
                        vec3 position,vec3 normal,vec3 incoming,uint material,float roughness,
                        float pixelFootprint) {
    diffuseLight=traceFiniteLighting(diffuseLight);specularLight=traceFiniteLighting(specularLight);
    uint scale=max(traceVisibility.extent.w,1u);
    uint history=traceHistoryRecord(uvec2(gl_FragCoord.xy)/scale);
    uint previousCount=traceVisibility.words[history+12u];
    uint previousSerial=traceVisibility.words[history+13u],serial=traceVisibility.temporal.x;
    bool reuse=traceVisibility.words[history+7u]==material && previousCount>0u &&
        previousCount<=traceHistoryLimit && previousSerial+1u==serial;
    if(reuse) {
        vec3 previousNormal=traceNormalize(unpackSnorm4x8(traceVisibility.words[history+3u]).xyz,normal);
        vec3 previousIncoming=traceNormalize(unpackSnorm4x8(traceVisibility.words[history+14u]).xyz,incoming);
        vec3 delta=traceLightingLoad3(history+8u)-position;
        float tolerance=max(traceEpsilon*4.0,pixelFootprint*.05);
        // Fixed-grid history is intentionally conservative: moving surfaces,
        // changed view directions and silhouettes restart their own estimate.
        reuse=dot(previousNormal,normal)>.995 && dot(previousIncoming,incoming)>.9995 &&
            dot(delta,delta)<=tolerance*tolerance &&
            abs(uintBitsToFloat(traceVisibility.words[history+11u])-roughness)<.01;
    }
    uint count=reuse?min(previousCount+1u,traceHistoryLimit):1u;
    if(reuse) {
        float previousWeight=1.0-1.0/float(count);
        diffuseLight+=(traceLightingLoad3(history)-diffuseLight)*previousWeight;
        specularLight+=(traceLightingLoad3(history+4u)-specularLight)*previousWeight;
    }
    for(uint lane=0u;lane<3u;++lane) {
        uint diffuseWord=floatBitsToUint(diffuseLight[lane]),specularWord=floatBitsToUint(specularLight[lane]);
        uint positionWord=floatBitsToUint(position[lane]);
        traceVisibility.words[record+lane]=diffuseWord;traceVisibility.words[history+lane]=diffuseWord;
        traceVisibility.words[record+4u+lane]=specularWord;traceVisibility.words[history+4u+lane]=specularWord;
        traceVisibility.words[record+8u+lane]=positionWord;traceVisibility.words[history+8u+lane]=positionWord;
    }
    uint normalWord=packSnorm4x8(vec4(normal,0)),roughnessWord=floatBitsToUint(roughness);
    traceVisibility.words[record+3u]=normalWord;traceVisibility.words[history+3u]=normalWord;
    traceVisibility.words[record+11u]=roughnessWord;traceVisibility.words[history+11u]=roughnessWord;
    traceVisibility.words[history+7u]=material;traceVisibility.words[history+12u]=count;
    traceVisibility.words[history+13u]=serial;traceVisibility.words[history+14u]=packSnorm4x8(vec4(incoming,0));
    traceVisibility.words[history+15u]=0u;
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
