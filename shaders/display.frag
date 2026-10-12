#version 450
// Native display adapter using the PC663D0E/663E28 scalar settings.
layout(set=0,binding=0) uniform sampler2D Image;
layout(push_constant) uniform Adjustment { vec4 value; };
layout(location=0) in vec2 UV;
layout(location=0) out vec4 pixel;
void main() {
    vec4 source=texture(Image,UV);
    pixel=vec4(pow(clamp(source.rgb+value.x,0.0,1.0),vec3(value.y)),source.a);
}
