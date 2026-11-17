// A qualified source lightmap is a cached diffuse irradiance field, not an
// emissive material. CPU admission proves the
// authored diffuse combiner's 2*lightmapRGB transfer independently of specular.
// Secondary use requires a real closest hit; qualified interior primaries
// retain it at their visible pixel without inventing a sky/ambient light.
bool traceSourceIndirectAvailable(TraceMaterial material,vec4 guide,uint bounce) {
    return bounce>0u && material.colorGuide.x<3u && material.colorGuide.w==2u && guide.a>0.0;
}
vec3 traceSourceIndirect(vec4 guide) {return 2.0*guide.rgb;}
// Preserve authenticated interior lighting at the visible pixel. Authored
// distant lights retain their traced outdoor visibility/shadow route instead.
bool tracePrimarySourceAvailable(TraceMaterial material,vec4 guide) {
    if(material.colorGuide.x>=3u || material.colorGuide.w!=2u || guide.a<=0.0)return false;
    [[dont_unroll]] for(uint n=0u;n<traceScene.counts.z;++n)
        if(traceLights[n].position_radius.w<0.0)return false;
    return true;
}
