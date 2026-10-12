#version 450
layout(set = 0, binding = 0) uniform sampler2D image;
layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 0) out vec4 output_color;
// Native horizontal extension only. Keep the authored center coordinates;
// exclude transparent padding from the boundary's linear filter footprint.
layout(push_constant) uniform FrameBoundary {
    vec4 sampling; // left U, right U, left enabled, right enabled.
} frame;
void main() {
    vec2 coordinate = uv;
    if (frame.sampling.z != 0.0) coordinate.x = max(coordinate.x, frame.sampling.x);
    if (frame.sampling.w != 0.0) coordinate.x = min(coordinate.x, frame.sampling.y);
    output_color = texture(image, coordinate) * color;
}
