#version 450
layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;
layout(location=9) flat in vec4 v_vtxPos0[];
layout(location=9) flat out vec4 vtxPos0;
layout(location=10) flat out vec4 vtxPos1;
layout(location=11) flat out vec4 vtxPos2;
layout(location=12) flat out float triMZ;
layout(push_constant) uniform SourceRotations {uvec2 rotations;};
// Literal pinned xemu geom.c perspective triangle plane calculation.
float kahan_det(float a,float b,float c,float d) {
    precise float cd=c*d;
    precise float err=fma(-c,d,cd);
    precise float res=fma(a,b,-cd)+err;
    return res;
}
mat4 calc_triz(int i0,int i1,int i2) {
    mat2 m=mat2(v_vtxPos0[i1].xy-v_vtxPos0[i0].xy,
                v_vtxPos0[i2].xy-v_vtxPos0[i0].xy);
    precise vec2 b=vec2(v_vtxPos0[i0].w-v_vtxPos0[i1].w,
                        v_vtxPos0[i0].w-v_vtxPos0[i2].w);
    b/=vec2(v_vtxPos0[i1].w,v_vtxPos0[i2].w)*v_vtxPos0[i0].w;
    float det=kahan_det(m[0].x,m[1].y,m[1].x,m[0].y);
    float dzx=kahan_det(b.x,m[1].y,b.y,m[0].y)/det;
    float dzy=kahan_det(b.y,m[0].x,b.x,m[1].x)/det;
    float dz=max(abs(dzx),abs(dzy));
    return mat4(v_vtxPos0[i0],v_vtxPos0[i1],v_vtxPos0[i2],dz,vec3(0.0));
}
void main() {
    uint rotation=rotations[gl_PrimitiveIDIn&1];
    ivec3 v=rotation==0u?ivec3(0,1,2):rotation==1u?ivec3(2,0,1):ivec3(1,2,0);
    mat4 pz=calc_triz(v[0],v[1],v[2]);
    for(int i=0;i<3;++i) {
        gl_Position=gl_in[v[i]].gl_Position;
        vtxPos0=pz[0];vtxPos1=pz[1];vtxPos2=pz[2];
        triMZ=(isnan(pz[3].x)||isinf(pz[3].x))?0.0:pz[3].x;
        EmitVertex();
    }
    EndPrimitive();
}
