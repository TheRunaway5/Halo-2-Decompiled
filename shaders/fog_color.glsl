// Retail 14600 aliases integer W depth as A8R8G8B8. The stencil byte is
// irrelevant here: mode 1 reads R/G/A; 25E50's HILO mapping multiplies LO by 0.
bool titleFogVisible(float depth) {
    // 26520 draws a quad at maximum W with LEQUAL and no depth writes.
    // It changes only clear-depth pixels, before the BSP planar-fog layer.
    return word(54u)!=0u || uint(round(clamp(depth,0.0,1.0)*16777215.0))==16777215u;
}
vec4 titleFogColor(float depth) {
    uint encoded=uint(round(clamp(depth,0.0,1.0)*16777215.0));
    vec4 samples[4];
    samples[0]=vec4((encoded>>8u)&255u,encoded&255u,0,encoded>>16u)/255.0;
    samples[1]=samples[2]=samples[3]=vec4(0);
    if(word(54u)==0x2621u) {
        // DOTMAPPING_HILO_1, followed by DOTPRODUCT/DOT_ST. Use the
        // authored gradient lookup rather than replacing it with linear fog.
        vec3 hilo=vec3(float(encoded>>8u)/65535.0,float((encoded&255u)<<8u)/65535.0,1);
        samples[2]=fogGradient(vec2(dot(material.lookup[0].xyz,hilo),dot(material.lookup[1].xyz,hilo)));
    }
    return runCombiners(samples,vec4(0,0,0,1),vec4(0,0,0,1));
}
