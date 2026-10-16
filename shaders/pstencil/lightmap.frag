#version 450
layout(push_constant) uniform Composite {vec4 color;} composite;
layout(location=0) out vec4 color;
void main(){color=composite.color;}
