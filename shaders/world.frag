#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
// Expand the bounded NV2A stages before driver compilation. This preserves
// authored lookups/filtering and prevents the native multi-view texture loss.
#define H2_FIXED_STAGE_UNROLL [[unroll]]
layout(location=0) in vec3 world;
layout(location=1) in vec3 tint;
layout(location=2) in vec2 uv;
layout(location=3) in vec2 lightmapUV;
layout(location=4) in vec4 lightingDirection;
layout(location=5) in vec4 eyeDirection;
layout(location=6) flat in uint lightmapped;
layout(location=7) in vec4 forwardColor;
layout(location=8) in vec4 stageCoordinates[4];
layout(location=12) in vec2 titleDepth;
layout(location=13) flat in uint stippleLevel;
#ifdef H2_EMBLEM_W_DEPTH
layout(location=14) flat in vec4 sourceEmblemWindow0;
layout(location=15) flat in vec4 sourceEmblemWindow1;
layout(location=16) flat in vec4 sourceEmblemWindow2;
#endif
layout(set=0,binding=0) uniform sampler2D diffuse;
layout(set=0,binding=1) uniform sampler2D detail;
layout(set=0,binding=2) uniform sampler2D bump;
layout(set=0,binding=3) uniform sampler2D lightmap;
layout(set=0,binding=5) uniform samplerCube cubeTextures[4];
layout(set=0,binding=6) uniform sampler3D volumeTextures[4];
layout(set=0,binding=4,std140) uniform Material {
    uvec4 program[15]; vec4 transforms[3]; uvec4 flags;
    vec4 forwardTransforms[8]; vec4 diffuseColor; vec4 specularColor;
    uvec4 vertexInstructions[136]; vec4 vertexConstants[16]; vec4 vertexAttributes[16];
    uvec4 alphaTest;
    uvec4 textureEffects[4];
    vec4 fogConstants[6];
    vec4 bumpMatrices[4];
    vec4 sourceCameraWorld[7];
    vec4 sourceCameraReflection[7];
    vec4 sourceCameraForeground[4];
    uvec4 sourceCameraControl;
} material;
layout(location=0) out vec4 pixel;
#include "world_combiner_specialization.glsl"
#include "combiner.glsl"
#include "pstencil/specular_mask.glsl"
#include "source_alpha_test.glsl"
#include "source_stipple.glsl"
#include "world_depth_fragment.glsl"
vec4 fetchStageCube(uint stage,vec3 p) {
    if(stage==0u)return texture(cubeTextures[0],p);
    if(stage==1u)return texture(cubeTextures[1],p);
    if(stage==2u)return texture(cubeTextures[2],p);
    return texture(cubeTextures[3],p);
}
vec4 fetchStage3D(uint stage,vec3 p) {
    if(stage==0u)return texture(volumeTextures[0],p);
    if(stage==1u)return texture(volumeTextures[1],p);
    if(stage==2u)return texture(volumeTextures[2],p);
    return texture(volumeTextures[3],p);
}
vec4 fetchStage2D(uint stage,vec2 p) {
    // Retail texture extern 3 is a linear render target: oT0 is in pixels.
    // Vulkan normalized sampling uses the dimensions of the actual target.
    if(stage==0u && (material.textureEffects[0].y&1u)!=0u)p/=vec2(textureSize(diffuse,0));
    if(stage==0u)return texture(diffuse,p);
    if(stage==1u)return texture(detail,p);
    if(stage==2u)return texture(bump,p);
    return texture(lightmap,p);
}
#define TEXTURE_REJECT() discard
#define TEXTURE_BUMP_MATRIX(i) material.bumpMatrices[i]
#define TEXTURE_COLOR_SIGN(i) (material.textureEffects[i].y>>1u)
#define TEXTURE_VOLUME(i) ((material.textureEffects[i].z&1u)!=0u)
// Outputs that the original program never writes retain initialized W=1.
// Keep that constant through projected sampling instead of interpolating it.
#define TEXTURE_COORDINATE_W(i) ((material.alphaTest.w&(1u<<(i)))!=0u ? 1.0 : stageCoordinates[i].w)
vec4 fetchStageProjected2D(uint stage) {
    vec3 p=vec3(stageCoordinates[stage].xy,TEXTURE_COORDINATE_W(stage));
    // The retained TU1.5 shader normalizes linear-image S/T before
    // textureProj divides by Q. Reversing those operations changes pixels.
    if(stage==0u) {
        if((material.textureEffects[0].y&1u)!=0u)p.xy/=vec2(textureSize(diffuse,0));
        return textureProj(diffuse,p);
    }
    if(stage==1u)return textureProj(detail,p);
    if(stage==2u)return textureProj(bump,p);
    return textureProj(lightmap,p);
}
#define TEXTURE_PROJECTED_2D(i) fetchStageProjected2D(i)
#include "texture_program.glsl"
void textureAlphaKill(vec4 samples[4]) {
    // NV_PGRAPH_TEXCTL0_ALPHAKILLEN is bit 2 of the authored nibble. It
    // tests each fetched texture before combiners, independent of ALPHATEST.
    for(uint i=0u;i<4u;++i) {
        uint mode=(word(54u)>>(5u*i))&31u;
        if((material.textureEffects[i].x&4u)!=0u &&
           (mode==1u || mode==2u || mode==3u || mode==9u || mode==11u || mode==12u ||
            mode==14u || mode==15u || mode==16u) && samples[i].a==0.0)discard;
    }
}
void main() {
    if(material.sourceCameraControl.y!=0u && stippleLevel<16u &&
       !sourceStipplePass(stippleLevel,material.sourceCameraControl.z,uvec2(gl_FragCoord.xy)))discard;
    // The title uses integer W depth (00467018 = 1024). Keep the aliased
    // D24 bytes available to the original fog composite. Division byFFFFFF
    // can lower to a reciprocal multiply and store N+1; retain the source
    // power-of-two conversion and next representable float instead.
    gl_FragDepth=titleDepth.y!=0.0 ?
        worldD24Depth(clamp(material.sourceCameraControl.x!=0u ? titleDepth.x : titleDepth.x*16777215.0,
            0.0,16777215.0)) : gl_FragCoord.z;
#ifdef H2_EMBLEM_W_DEPTH
    // Late armor emblems compare against integer W depth. Hardware varying
    // interpolation can cross an integer boundary and drop a valid edge pixel.
    // Retain the original ordered triangle reconstruction before quantization.
    if(material.flags.w!=0u && titleDepth.y!=0.0)
        gl_FragDepth=worldD24Depth(worldWDepth(gl_FragCoord.xy,vec2(1),
            sourceEmblemWindow0,sourceEmblemWindow1,sourceEmblemWindow2,0,0,0));
#endif
    if(material.flags.y!=0u) {
        vec4 samples[4];
        vec2 coordinates[4];
        if(material.flags.z==11u) {
            for(uint i=0u;i<4u;++i)coordinates[i]=uv;
        } else if(material.flags.z==1u) {
            // Original transparent program: c20..25 are paired affine rows
            // for oT1..3. The authored program below supplies oT0 reflection.
            coordinates[0]=vec2(0);
            for(uint i=1u;i<4u;++i) coordinates[i]=vec2(
                dot(vec3(uv,1),material.forwardTransforms[i*2u].xyw),
                dot(vec3(uv,1),material.forwardTransforms[i*2u+1u].xyw));
        } else if(material.flags.z==3u) {
            // Original stencil_texture: c18 transforms the base once; c19
            // transforms detail. c20.xy / c20.zw scale stages 2 / 3.
            vec2 base=uv*material.forwardTransforms[0].xy+material.forwardTransforms[0].zw;
            coordinates[0]=base;
            coordinates[1]=base*material.forwardTransforms[1].xy+material.forwardTransforms[1].zw;
            coordinates[2]=base*material.forwardTransforms[2].xy;
            coordinates[3]=base*material.forwardTransforms[2].zw;
        } else {
            // stencil_emissive first applies c18, then c19..22 per stage.
            vec2 base=uv*material.forwardTransforms[0].xy+material.forwardTransforms[0].zw;
            for(uint i=0u;i<4u;++i)coordinates[i]=base*material.forwardTransforms[i+1u].xy+material.forwardTransforms[i+1u].zw;
        }
        samples[0]=texture(diffuse,coordinates[0]);
        samples[1]=texture(detail,coordinates[1]);
        samples[2]=texture(bump,coordinates[2]);
        samples[3]=texture(lightmap,coordinates[3]);
        // The fixed fallback handles PROJECT2D. Authored transform programs
        // supply projected coordinates and cubemap directions below.
        for(uint i=0u;i<4u;++i)if(((word(54u)>>(5u*i))&31u)!=1u)samples[i]=vec4(0);
        if(material.flags.w!=0u)runTextureStages(samples);
        textureAlphaKill(samples);
        pixel=runCombiners(samples,material.flags.z==3u?material.diffuseColor:forwardColor,
            material.flags.w!=0u?lightingDirection:vec4(0,0,0,1));
    } else if(material.flags.x!=0u && lightmapped!=0u) {
        vec4 samples[4];
        samples[0]=texture(diffuse,uv*material.transforms[0].xy+material.transforms[0].zw);
        samples[1]=texture(detail,uv*material.transforms[1].xy+material.transforms[1].zw);
        samples[2]=texture(bump,uv*material.transforms[2].xy+material.transforms[2].zw);
        samples[3]=texture(lightmap,lightmapUV);
        // The supported passes use PROJECT2D texture modes. Disabled stages
        // read zero, as on NV2A, rather than sampling a substituted bitmap.
        for(uint i=0u;i<4u;++i)if(((word(54u)>>(5u*i))&31u)==0u)samples[i]=vec4(0);
        if(material.flags.w!=0u)runTextureStages(samples);
        textureAlphaKill(samples);
        pixel=runCombiners(samples,eyeDirection,lightingDirection);
    } else {
        // Explicit fallback until the remaining retail passes are scheduled.
        // Authored baked lighting replaces the former invented directional sun.
        vec3 baked=lightmapped!=0u?texture(lightmap,lightmapUV).rgb:vec3(1);
        pixel=vec4(texture(diffuse,uv).rgb*tint*baked,1);
    }
    if((material.textureEffects[0].y&32u)!=0u)
        pixel=pstencilSpecularMask(pixel,texture(diffuse,gl_FragCoord.xy/vec2(textureSize(diffuse,0))).a);
    if(material.alphaTest.x!=0u) {
        if(!sourceAlphaTestPass(pixel.a,material.alphaTest.y,material.alphaTest.z&255u))discard;
    }
}
