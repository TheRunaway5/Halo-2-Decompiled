// A qualified source lightmap is a cached diffuse irradiance field, not an
// emissive material or a primary lighting term. CPU admission proves the
// authored diffuse combiner's 2*lightmapRGB transfer independently of specular.
// The existing closest-hit segment must reach this surface before it applies.
// A map can retain this authenticated field without any sky/outdoor ambient.
bool traceSourceIndirectAvailable(TraceMaterial material,vec4 guide,uint bounce) {
    return bounce>0u && material.colorGuide.w==2u && guide.a>0.0;
}
vec3 traceSourceIndirect(vec4 guide) {return 2.0*guide.rgb;}
