// 34A90 scales 34100's pixel coordinates by source size / target size.
// After normalizing the rectangular texture, offsets use the target size.
vec2 titleBloomCoordinates(int stage,vec2 uv) {
    vec2 delta=vec2(material.controls.x*.5)/material.controls.yz;
    vec2 sign=vec2((stage&1)==0?-1:1,(stage&2)==0?-1:1);
    return uv+sign*delta;
}
vec4 titleBloomColor(vec2 uv) {
    vec4 samples[4];
    for(int i=0;i<4;++i) {
        samples[i]=bloomLookup(i,titleBloomCoordinates(i,uv));
    }
    return runCombiners(samples,vec4(0,0,0,1),vec4(0,0,0,1));
}
