#version 450

layout(location=0) in vec3 currentPosition;
layout(location=1) in vec3 previousPosition;
layout(location=0) out vec4 currentClip;
layout(location=1) out vec4 previousClip;
layout(push_constant) uniform MotionBlur {
    mat4 matrix;
    ivec4 bounds;
    vec2 extent;
    float shutterFraction;
    uint wDepth;
} blur;

void main() {
    // Use this frame's camera twice so camera movement contributes no blur.
    currentClip = blur.matrix * vec4(currentPosition, 1.0);
    previousClip = blur.matrix * vec4(previousPosition, 1.0);
    gl_Position = currentClip;
}
