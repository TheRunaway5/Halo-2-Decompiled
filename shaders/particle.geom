#version 450
// Captured original QUADS lowering. Input order is the original four corners;
// each output triangle keeps its own three perspective-depth positions.
layout(lines_adjacency) in;
layout(triangle_strip,max_vertices=6) out;
layout(location=0) in vec4 v_vtxD0[];
layout(location=1) in vec4 v_vtxD1[];
layout(location=5) in vec4 v_vtxT0[];
layout(location=6) in vec4 v_vtxT1[];
layout(location=9) flat in vec4 v_vtxPos0[];
layout(location=0) out vec4 vtxD0;
layout(location=1) out vec4 vtxD1;
layout(location=5) out vec4 vtxT0;
layout(location=6) out vec4 vtxT1;
layout(location=9) flat out vec4 vtxPos0;
layout(location=10) flat out vec4 vtxPos1;
layout(location=11) flat out vec4 vtxPos2;
layout(location=12) flat out float triMZ;
float determinant(float a,float b,float c,float d) {
    float cd=c*d;float error=fma(-c,d,cd);return fma(a,b,-cd)+error;
}
mat4 triangleDepth(int i0,int i1,int i2) {
    vec4 p0=v_vtxPos0[i0],p1=v_vtxPos0[i1],p2=v_vtxPos0[i2];
    mat2 m=mat2(p1.xy-p0.xy,p2.xy-p0.xy);
    vec2 b=vec2(p0.w-p1.w,p0.w-p2.w)/(vec2(p1.w,p2.w)*p0.w);
    float det=determinant(m[0].x,m[1].y,m[1].x,m[0].y);
    float dx=determinant(b.x,m[1].y,b.y,m[0].y)/det;
    float dy=determinant(b.y,m[0].x,b.x,m[1].x)/det;
    float slope=max(abs(dx),abs(dy));
    return mat4(p0,p1,p2,vec4(slope,0,0,0));
}
void emit(int i,mat4 depth) {
    gl_Position=gl_in[i].gl_Position;gl_PointSize=gl_in[i].gl_PointSize;
    vtxD0=v_vtxD0[i];vtxD1=v_vtxD1[i];
    vtxT0=v_vtxT0[i];vtxT1=v_vtxT1[i];
    vtxPos0=depth[0];vtxPos1=depth[1];vtxPos2=depth[2];
    triMZ=isnan(depth[3].x)||isinf(depth[3].x)?0.0:depth[3].x;
    EmitVertex();
}
void main() {
    mat4 a=triangleDepth(0,1,2),b=triangleDepth(0,2,3);
    emit(1,a);emit(2,a);emit(0,a);EndPrimitive();
    emit(2,b);emit(3,b);emit(0,b);EndPrimitive();
}
