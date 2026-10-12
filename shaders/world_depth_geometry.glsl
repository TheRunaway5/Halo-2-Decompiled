#ifndef HALO2_WORLD_DEPTH_GEOMETRY_GLSL
#define HALO2_WORLD_DEPTH_GEOMETRY_GLSL
// Ordinary original GS arithmetic, retained pass1335-3.spv:
// SHA25620ac4499716ad849300a2230c91faed205ad0c609bc0779e22006ae88aa45e22.
// Compensated determinant and ordered reciprocal-W slope; input order is
// supplied by the caller, rather than reconstructed from screen coordinates.
float world_depth_determinant(float a, float b, float c, float d) {
    precise float cd = c * d;
    precise float error = fma(-c, d, cd);
    precise float result = fma(a, b, -cd) + error;
    return result;
}
mat4 world_depth_triangle(vec4 p0, vec4 p1, vec4 p2) {
    mat2 m = mat2(p1.xy - p0.xy, p2.xy - p0.xy);
    precise vec2 b = vec2(p0.w - p1.w, p0.w - p2.w);
    b /= vec2(p1.w, p2.w) * p0.w;
    float determinant = world_depth_determinant(m[0].x, m[1].y, m[1].x, m[0].y);
    float dx = world_depth_determinant(b.x, m[1].y, b.y, m[0].y) / determinant;
    float dy = world_depth_determinant(b.y, m[0].x, b.x, m[1].x) / determinant;
    float slope = max(abs(dx), abs(dy));
    return mat4(p0, p1, p2, slope, vec3(0.0));
}
float world_depth_finite_slope(mat4 triangle) {
    return (isnan(triangle[3].x) || isinf(triangle[3].x)) ? 0.0 : triangle[3].x;
}
#endif
