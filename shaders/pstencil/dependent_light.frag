#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#define H2_FIXED_STAGE_UNROLL [[unroll]]
#include "pstencil/light_surface.glsl"
void main() {
    lightSurfaceDepth();
    vec4 samples[4];
    // Targets11 and10 share the current camera, geometry and pixel grid.
    samples[0]=texture(diffuse,gl_FragCoord.xy/vec2(textureSize(diffuse,0)));
    samples[1]=texture(detail,gl_FragCoord.xy/vec2(textureSize(detail,0)));
    samples[2]=vec4(0);
    vec3 toLight=lightConstant(31u).xyz-world;
    vec3 gel=vec3(dot(toLight,lightConstant(35u).xyz),dot(toLight,lightConstant(34u).xyz),
                  dot(toLight,lightConstant(33u).xyz));
    if(((word(54u)>>15u)&31u)==1u) {
        // Original25 with static V9.xy=(0,0), supplied by original180810.
        // Original35E81..35E94 initializes V15=(0,.5,1,42).
        vec3 offset=lightConstant(32u).xyz-world;
        float q=dot(offset,lightConstant(35u).xyz)*lightConstant(31u).w+(1.-lightConstant(31u).w);
        vec2 p=vec2(dot(offset,lightConstant(33u).xyz)+lightConstant(33u).w,
                    dot(offset,lightConstant(34u).xyz)+lightConstant(34u).w);
        samples[3]=texture(lightmap,p/q);
    }else samples[3]=texture(cubeTextures[3],gel);
    // The current authored V10/V11 facing/grazing colors remain separate
    // from the completed source light colors packed in the pixel program.
    pixel=runCombiners(samples,eyeDirection,lightingDirection);
}
