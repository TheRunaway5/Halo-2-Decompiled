#version 450
layout(location=0) in vec3 currentPosition;
layout(location=1) in vec3 previousPosition;
layout(location=2) in float historyValid;
layout(binding=6,std140) uniform View {
    mat4 inverseCurrent;mat4 previous;mat4 previousForeground;vec4 rectangle;vec4 planes;vec4 jitter;vec4 size;
} view;
layout(push_constant) uniform Motion { mat4 current;float depthScale;uint foreground; } pc;
layout(location=0) out vec4 previousClip;
layout(location=1) out vec4 currentClip;
layout(location=2) out float valid;
void main() {
    currentClip=pc.current*vec4(currentPosition,1);
    previousClip=(pc.foreground!=0u?view.previousForeground:view.previous)*vec4(previousPosition,1);
    valid=historyValid;
    gl_Position=currentClip;
    gl_Position.xy+=view.jitter.xy*2.0/view.rectangle.zw*gl_Position.w;
}
