// 26CA0 noise geometry; 27520 depth-gradient and half-resolution noise lookup.
vec2 titlePatchyCoordinates(int layer,vec2 uv) {
    vec2 p=uv*2.0-1.0;
    vec4 r=material.rows[layer*2],s=material.rows[layer*2+1];
    return vec2(p.x*r.x-p.y*r.y*material.controls.x+r.w,
                p.x*s.x-p.y*s.y*material.controls.x+s.w);
}
vec4 titlePatchyNoise(vec2 uv) {
    vec4 samples[4];
    for(int i=0;i<4;++i)samples[i]=patchyNoiseLookup(i,titlePatchyCoordinates(i,uv));
    return runCombiners(samples,vec4(0,0,0,1),vec4(0,0,0,1));
}
vec4 titlePatchyComposite(float depth,vec2 uv) {
    uint encoded=uint(round(clamp(depth,0.0,1.0)*16777215.0));
    vec3 hilo=vec3(float(encoded>>8u)/65535.0,float((encoded&255u)<<8u)/65535.0,1);
    vec4 samples[4];
    samples[0]=vec4((encoded>>8u)&255u,encoded&255u,0,encoded>>16u)/255.0;
    samples[1]=vec4(0);
    float coordinate=dot(material.lookup.xyz,hilo);
    samples[2]=patchyGradientLookup(vec2(coordinate));
    samples[3]=patchyTargetLookup(uv);
    return runCombiners(samples,vec4(0,0,0,1),vec4(0,0,0,1));
}
