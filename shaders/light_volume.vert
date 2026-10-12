#version 450
layout(location=0) in vec4 screenInput;
layout(location=1) in vec4 colorInput;
layout(location=2) in vec2 uvInput;
layout(location=0) out vec4 color;
layout(location=1) out vec2 uv;
layout(location=2) flat out float wDepth;
layout(push_constant) uniform Camera {
    mat4 viewProjection; vec4 eyeForward; vec4 eyePosition; vec4 viewport;
} camera;
void main() {
    vec4 screen=screenInput;
    screen.xy=trunc(screen.xy*16.0)/16.0;
    screen.w=(screen.w<0.0?-1.0:1.0)*clamp(abs(screen.w),5.421010862427522e-20,1.8446744073709552e19);
    // Xemu's original passthrough vertex program uses full surface dimensions.
    // The original captured SPIR-V does not mark these operations
    // NoContraction. Retain its shader arithmetic contract here as well.
    vec2 ndc=(screen.xy*2.0-camera.viewport.zw)/camera.viewport.zw;
    gl_Position=vec4(ndc*screen.w,(screen.z/16777215.0)*screen.w,screen.w);
    color=colorInput;uv=uvInput;wDepth=screen.w;
}
