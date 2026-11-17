#version 450
// Match the ordinary world's submitted triangle order and retained native
// screen/W values. Hardware interpolation of a separately computed dot
// product cannot serve as the reference for the source W-depth attachment.
layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;
layout(location=0) in float inDepth[];
layout(location=1) flat in vec4 inWindow[];
layout(location=0) out float sourceDepth;
layout(location=1) flat out vec4 window0;
layout(location=2) flat out vec4 window1;
layout(location=3) flat out vec4 window2;
void main() {
    for(int i=0;i<3;++i) {
        gl_Position=gl_in[i].gl_Position;
        sourceDepth=inDepth[i];
        window0=inWindow[0];window1=inWindow[1];window2=inWindow[2];
        EmitVertex();
    }
    EndPrimitive();
}
