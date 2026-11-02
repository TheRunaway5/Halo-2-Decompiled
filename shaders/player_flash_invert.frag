#version 450
// TU1.5 40880 case5: literal RGB inputs01411120/output00000C00,
// final0C/1C00, C0=134960's quantized premultiplied ARGB. The native
// ONE_MINUS_DST_COLOR/CONSTANT_COLOR blend consumes this completed output.
layout(push_constant) uniform Effects { vec4 constant0; vec4 frame; } effects;
layout(location=0) out vec4 color;
void main() {
    vec3 c=effects.constant0.rgb;
    color=vec4(clamp(c*(2.0*c-1.0)+effects.constant0.a,0.0,1.0),0.0);
}
