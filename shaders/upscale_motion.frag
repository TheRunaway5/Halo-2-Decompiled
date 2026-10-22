#version 450
layout(location=0) in vec4 previousClip;
layout(location=1) in vec4 currentClip;
layout(location=2) in float valid;
layout(binding=0) uniform sampler2D rawDepth;
layout(binding=6,std140) uniform View {
    mat4 inverseCurrent;mat4 previous;mat4 previousForeground;vec4 rectangle;vec4 planes;vec4 jitter;vec4 size;
} view;
layout(push_constant) uniform Motion { mat4 current;float depthScale;uint foreground; } pc;
layout(location=0) out vec2 motion;
layout(location=1) out float depth;
layout(location=2) out float reactive;
void main() {
    float actual=texelFetch(rawDepth,ivec2(gl_FragCoord.xy),0).r;
    float expected=view.planes.z!=0.0?currentClip.w*pc.depthScale/1024.0:gl_FragCoord.z;
    // The auxiliary geometry must only replace samples belonging to its
    // visible surface, including depth retained through foreground drawing.
    if(abs(actual-expected)>max(0.000002,abs(expected)*0.0002))discard;
    motion=vec2(0);reactive=1.0;
    if(valid>.99 && view.planes.w==0.0 && previousClip.w>0.0) {
        motion=(previousClip.xy/previousClip.w-currentClip.xy/currentClip.w)*0.5*view.rectangle.zw;
        reactive=0.0;
    }
    depth=clamp(currentClip.z/currentClip.w,0.0,1.0);
}
