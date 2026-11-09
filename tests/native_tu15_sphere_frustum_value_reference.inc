// Test-only native-native regression reference: exact pre-extraction public value algorithm.
SourceSphereVisibility source_sphere_frustum_visibility_reference(
    const SourceSphereFrustum& f,Vec3 center,float radius) {
    if(center.x-radius>f.bounds[1] || center.y-radius>f.bounds[3] ||
       center.z-radius>f.bounds[5] || f.bounds[0]>center.x+radius ||
       f.bounds[2]>radius+center.y || f.bounds[4]>radius+center.z)
        return SourceSphereVisibility::outside;
    std::array<float,6> distances;
    for(unsigned i=0;i<6;++i) {
        const auto& p=f.planes[i];
        //181882 onward: z product, y product, ADDSS, x product, ADDSS, SUBSS.
        distances[i]=(p[2]*center.z+p[1]*center.y)+p[0]*center.x-p[3];
        if(distances[i]>radius)return SourceSphereVisibility::outside;
    }
    const float negative_radius=0.f-radius;
    for(unsigned i:{0u,1u,2u,3u,5u})
        if(!(negative_radius>distances[i]))return SourceSphereVisibility::intersecting;
    return SourceSphereVisibility::contained;
}
