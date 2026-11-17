#version 450
// Preserve the submitted triangle and its quantized source window coordinates.
// Native surfaces reconstruct integer W depth from this same ordered triangle;
// hardware varying interpolation can cross a D24 boundary on a coincident cap.
layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;
layout(location=0) in float inDepth[];
layout(location=1) flat in uint inStipple[];
layout(location=2) flat in vec4 inWindow[];
layout(location=0) out float sourceDepth;
layout(location=1) flat out uint sourceStipple;
layout(location=2) flat out vec4 sourceWindow0;
layout(location=3) flat out vec4 sourceWindow1;
layout(location=4) flat out vec4 sourceWindow2;
void main() {
    for(int i=0;i<3;++i) {
        gl_Position=gl_in[i].gl_Position;
        sourceDepth=inDepth[i];
        sourceStipple=inStipple[i];
        sourceWindow0=inWindow[0];
        sourceWindow1=inWindow[1];
        sourceWindow2=inWindow[2];
        EmitVertex();
    }
    EndPrimitive();
}
