#version 450
layout(push_constant) uniform Color { vec4 rgba; } color;
layout(location=0) out vec4 pixel;
void main() { pixel=color.rgba; }
