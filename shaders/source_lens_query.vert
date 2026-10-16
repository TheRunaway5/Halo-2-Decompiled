#version 450
layout(location=0) in vec4 sourcePosition;
layout(push_constant) uniform QueryView { vec2 surface; } view;
layout(location=0) flat out float sourceW;
void main() {
    // Same source pretransformed screen coordinates as the world weather pass.
    vec4 p=sourcePosition;
    sourceW=p.w;
    p.xy=(2.0*p.xy-view.surface)/view.surface;
    p.z/=16777215.0;
    p.xyz*=p.w;
    gl_Position=p;
}
