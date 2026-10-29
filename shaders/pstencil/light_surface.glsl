// Current authored sphere vertex interface, shared by the two recovered passes.
layout(location=0) in vec3 world;
layout(location=1) in vec3 tint;
layout(location=2) in vec2 uv;
layout(location=3) in vec2 lightmapUV;
layout(location=4) in vec4 lightingDirection;
layout(location=5) in vec4 eyeDirection;
layout(location=6) flat in uint lightmapped;
layout(location=7) in vec4 forwardColor;
layout(location=8) in vec4 currentCoordinates[4];
layout(location=12) in vec2 titleDepth;
layout(location=13) flat in uint stippleLevel;
#ifdef H2_SOURCE_WORLD_DEPTH
layout(location=14) flat in vec4 sourceEmblemWindow0;
layout(location=15) flat in vec4 sourceEmblemWindow1;
layout(location=16) flat in vec4 sourceEmblemWindow2;
layout(push_constant) uniform SourceDepthRaster {layout(offset=112) vec4 clipAndOffset;} sourceDepthRaster;
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
layout(set=0,binding=7,std140) uniform VertexBank {
    uvec4 range;
    vec4 constants[158];
} vertexBank;
layout(location=0) out vec4 pixel;
#ifndef H2_COMBINER_WORD
#include "world_combiner_specialization.glsl"
#endif
#include "combiner.glsl"
#include "source_stipple.glsl"
#include "world_depth_fragment.glsl"
vec4 lightConstant(uint index) {
    if(index>=18u && index<34u)return material.vertexConstants[index-18u];
    if(index>=vertexBank.range.x && index-vertexBank.range.x<vertexBank.range.y)
        return vertexBank.constants[index-vertexBank.range.x];
    return vec4(0);
}
void lightSurfaceDepth() {
    if(material.sourceCameraControl.y!=0u && stippleLevel<16u &&
       !sourceStipplePass(stippleLevel,material.sourceCameraControl.z,uvec2(gl_FragCoord.xy)))discard;
#ifdef H2_SOURCE_WORLD_DEPTH
    float depth=worldWDepth(gl_FragCoord.xy-sourceDepthRaster.clipAndOffset.zw,vec2(1),
        sourceEmblemWindow0,sourceEmblemWindow1,sourceEmblemWindow2,0,0,0);
    if(depth<sourceDepthRaster.clipAndOffset.x || depth>sourceDepthRaster.clipAndOffset.y)discard;
    gl_FragDepth=worldD24Depth(depth);
#else
    gl_FragDepth=titleDepth.y!=0.0 ?
        worldD24Depth(clamp(material.sourceCameraControl.x!=0u ? titleDepth.x : titleDepth.x*16777215.0,
            0.0,16777215.0)) : gl_FragCoord.z;
#endif
}
