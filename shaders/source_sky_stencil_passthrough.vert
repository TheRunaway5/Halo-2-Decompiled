#version 450
layout(location=0) in vec4 retainedClip;
layout(location=1) in vec4 retainedWindow0;
layout(location=2) in vec4 retainedWindow1;
layout(location=3) in vec4 retainedWindow2;
layout(location=4) in float retainedTriMZ;
layout(location=9) flat out vec4 vtxPos0;
layout(location=10) flat out vec4 vtxPos1;
layout(location=11) flat out vec4 vtxPos2;
layout(location=12) flat out float triMZ;
void main() {
    gl_Position=retainedClip;vtxPos0=retainedWindow0;
    vtxPos1=retainedWindow1;vtxPos2=retainedWindow2;triMZ=retainedTriMZ;
}
