#version 450
layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;
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
void main(){for(int i=0;i<3;++i){gl_Position=gl_in[i].gl_Position;color0=inColor0[i];color1=inColor1[i];texcoord0=inTexcoord0[i];screen0=inScreen0[0];screen1=inScreen0[1];screen2=inScreen0[2];EmitVertex();}EndPrimitive();}
