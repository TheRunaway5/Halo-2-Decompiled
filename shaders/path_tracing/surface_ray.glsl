#ifndef H2_PATH_TRACING_SURFACE_RAY_GLSL
#define H2_PATH_TRACING_SURFACE_RAY_GLSL
vec3 traceSurfaceCoordinateUlps(vec3 value) {
    vec3 magnitude=abs(value);
    return uintBitsToFloat(floatBitsToUint(magnitude)+uvec3(1u))-magnitude;
}
float traceSurfacePointTolerance(vec3 vertex,vec3 position,vec3 normal,vec3 positionUlps) {
    // An anchored plane test avoids cancellation between two large WORLD
    // plane constants. The only allowance is the existing ray epsilon plus
    // four coordinate ULPs for subtraction and the three-term plane product.
    vec3 error=(traceSurfaceCoordinateUlps(vertex)+positionUlps)*4.0;
    precise float roundoff=(abs(normal.x)*error.x+abs(normal.y)*error.y)+abs(normal.z)*error.z;
    return traceEpsilon+roundoff;
}
bool traceSurfaceEdgeContains(vec3 a,vec3 b,vec3 position,vec3 normal,vec3 positionUlps) {
    vec3 inward=traceNormalize(cross(normal,b-a),vec3(0));
    vec3 delta=position-a;
    precise float distance=(delta.x*inward.x+delta.y*inward.y)+delta.z*inward.z;
    return distance>=-traceSurfacePointTolerance(a,position,inward,positionUlps);
}
bool traceSkipStartingSurface(uint index,vec3 position,vec3 geometric,vec3 direction,uint sourceTriangle) {
    TraceTriangle triangle=traceTriangles[index];
    // Form the plane from the candidate's original vertices. Screen-space
    // derivatives of large WORLD coordinates cannot reliably identify that
    // plane at native resolution. The source shading normal supplies its
    // hemisphere orientation instead.
    vec3 normal=traceNormalize(cross(triangle.p1.xyz-triangle.p0.xyz,triangle.p2.xyz-triangle.p0.xyz),vec3(0));
    if(dot(normal,normal)==0.0)return false;
    vec3 oriented=dot(normal,geometric)<0.0?-normal:normal;
    if(dot(oriented,direction)>0.0)return false;
    if(index==sourceTriangle)return true;
    vec3 positionUlps=traceSurfaceCoordinateUlps(position),delta=position-triangle.p0.xyz;
    precise float distance=(delta.x*normal.x+delta.y*normal.y)+delta.z*normal.z;
    if(abs(distance)>traceSurfacePointTolerance(triangle.p0.xyz,position,normal,positionUlps))return false;
    // The starting point must belong to this exact triangle, not merely to
    // its infinite plane. Displaced and separate tilted blockers remain in
    // the same existing BVH walk. No ray displacement or angular bias is used.
    return traceSurfaceEdgeContains(triangle.p0.xyz,triangle.p1.xyz,position,normal,positionUlps) &&
        traceSurfaceEdgeContains(triangle.p1.xyz,triangle.p2.xyz,position,normal,positionUlps) &&
        traceSurfaceEdgeContains(triangle.p2.xyz,triangle.p0.xyz,position,normal,positionUlps);
}
#endif
