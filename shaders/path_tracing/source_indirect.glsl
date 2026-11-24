// A qualified source lightmap is a cached diffuse irradiance field, not an
// emissive material. CPU admission proves the
// authored diffuse combiner's 2*lightmapRGB transfer independently of specular.
// Secondary use requires a real closest hit; qualified interior primaries
// retain it at their visible pixel without inventing a sky/ambient light.
// Every baked guide (lightmap image or static vertex radiance) counts, not
// only the families whose combiner was authenticated: Halo 2's standard
// world shaders transfer 2*RGB, and the alternative - tracing only lights that
// exist at run time - leaves interiors lit by baked fixtures nearly black.
bool traceBakedGuide(TraceMaterial material,vec4 guide) {
    return (material.colorGuide.x==1u || material.colorGuide.x==2u) && guide.a>0.0;
}
bool traceSourceIndirectAvailable(TraceMaterial material,vec4 guide,uint bounce) {
    return bounce>0u && traceBakedGuide(material,guide);
}
vec3 traceSourceIndirect(vec4 guide) {return 2.0*guide.rgb;}
// Preserve authenticated interior lighting at the visible pixel. Authored
// distant lights retain their traced outdoor visibility/shadow route instead.
bool tracePrimarySourceAvailable(TraceMaterial material,vec4 guide) {
    if(!traceBakedGuide(material,guide))return false;
    [[dont_unroll]] for(uint n=0u;n<traceScene.counts.z;++n)
        if(traceLights[n].position_radius.w<0.0)return false;
    return true;
}
