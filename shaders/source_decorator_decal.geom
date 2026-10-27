#version 450
layout(lines_adjacency) in;
layout(triangle_strip,max_vertices=6) out;
layout(location=0) in vec4 inColor0[];
layout(location=1) in vec4 inColor1[];
layout(location=5) in vec4 inTexcoord0[];
layout(location=9) flat in vec4 inScreen0[];
layout(location=0) out vec4 color0;
layout(location=1) out vec4 color1;
layout(location=5) out vec4 texcoord0;
layout(location=9) flat out vec4 screen0;
layout(location=10) flat out vec4 screen1;
layout(location=11) flat out vec4 screen2;
void triangle(int a,int b,int c) {
    int vertices[3]=int[3](a,b,c);
    for(int n=0;n<3;++n) {
        int i=vertices[n];gl_Position=gl_in[i].gl_Position;
        color0=inColor0[i];color1=inColor1[i];texcoord0=inTexcoord0[i];
        screen0=inScreen0[a];screen1=inScreen0[b];screen2=inScreen0[c];EmitVertex();
    }
    EndPrimitive();
}
void main() {
    // Actual nonindexed QUADS expansion. The authored decorator index
    // resource is not consumed by original4EBC0's floating branch.
    triangle(1,2,0);triangle(2,3,0);
}
