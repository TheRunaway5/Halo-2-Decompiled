#version 450
#extension GL_GOOGLE_include_directive : require
layout(push_constant) uniform Push {
    uint width,base,inputMode,expression;
    uint sourceControl,wMode,caseCount,unused;
} pc;
layout(set=0,binding=0,std430) readonly buffer Cases { uint values[]; } cases;
#include "depth_expressions.glsl"
#ifdef DEPTH_COLOR_CONTROL
layout(location=0) out vec4 pixel;
#else
layout(location=0) out uint pixel;
#endif
void main() {
    uint index=uint(gl_FragCoord.y)*pc.width+uint(gl_FragCoord.x);
    if(pc.inputMode!=0u&&index>=pc.caseCount)discard;
    float raw=pc.inputMode==0u?float(pc.base+index):uintBitsToFloat(cases.values[index]);
    // Explicit post-admission diagnostic input; no full original clip claim.
    float w=clamp(raw,0.0,16777215.0);
    float encoded=pc.expression==0u?currentEncoding(w):sourceEncoding(w);
    gl_FragDepth=encoded;
#ifdef DEPTH_COLOR_CONTROL
    pixel=vec4(51.0/255.0,102.0/255.0,153.0/255.0,1.0);
#else
    // Observe the emitted float independently from fixed-point attachment copy.
    pixel=floatBitsToUint(encoded);
#endif
}
