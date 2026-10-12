#version 450
layout(location=0) in vec4 retainedClip;
layout(location=1) in vec4 retainedWindow;
layout(location=9) flat out vec4 v_vtxPos0;
void main() {gl_Position=retainedClip;v_vtxPos0=retainedWindow;}
