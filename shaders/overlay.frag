#version 450
layout(push_constant) uniform Effects { vec4 fade; vec4 frame; } effects;
layout(location = 0) out vec4 color;
void main() {
    float y = gl_FragCoord.y / effects.frame.y;
    float bar = effects.frame.z;
    color = y < bar || y > 1.0 - bar ? vec4(0, 0, 0, effects.frame.w) : effects.fade;
}
