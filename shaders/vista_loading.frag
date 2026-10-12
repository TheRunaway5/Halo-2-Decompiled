#version 450
layout(set = 0, binding = 0) uniform sampler2D image;
layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 0) out vec4 output_color;
layout(push_constant) uniform Loading {
    vec4 constant0;
    vec2 clip_offset;
    uint program;
    uint reserved;
} loading;
void main() {
    // Original Microsoft D3DX-disassembled ps_2_0 instructions. Their _pp
    // modifiers mean partial precision, not saturation. Pixel equivalence
    // is checked against the original D3D9 replay before any precision policy.
    if (loading.program == 3u) {
        output_color = color;
        return;
    }
    vec4 sampled = texture(image, uv);
    if (loading.program == 2u) {
        precise float bias = 1.0 - loading.constant0.x;
        precise vec3 rgb = sampled.rgb + bias;
        precise float strength = 1.0 - loading.constant0.y;
        strength = sampled.a * strength;
        rgb = rgb * strength;
        rgb = rgb * color.rgb;
        output_color = vec4(rgb, sampled.a);
    } else {
        precise vec3 rgb = sampled.rgb * color.rgb;
        vec4 result = vec4(rgb, sampled.a);
        if (loading.program == 1u) result = result * loading.constant0.x;
        output_color = result;
    }
}
