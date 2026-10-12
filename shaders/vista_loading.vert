#version 450
layout(location = 0) in vec3 position; // Original float2 xy; packed color alpha in z.
layout(location = 1) in vec3 tint;
layout(location = 2) in vec2 texcoord;
layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 color;
layout(push_constant) uniform Loading {
    vec4 constant0;
    vec2 clip_offset;
    uint program;
    uint reserved;
} loading;
void main() {
    // Embedded VS0 7E6B10, c177..181. Original D3D viewport inverts Y;
    // the positive Vulkan viewport requires the equivalent sign change.
    // Retain separate multiply/add instructions, as in the original VS.
    precise float x = position.x * 2.0;
    precise float y = position.y * 2.0;
    x = x + -1.0;
    y = y + -1.0;
    // D3D9 pixel-center compensation belongs to the viewport. Adding a
    // divided pixel offset here changes the interpolated texture plane.
    gl_Position = vec4(x, y, 0.5, 1.0);
    uv = texcoord;
    color = vec4(tint, position.z);
}
