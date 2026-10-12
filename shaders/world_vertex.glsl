layout(location=0) in vec3 position;
layout(location=1) in vec3 color;
layout(location=2) in vec2 texcoord;
layout(location=3) in vec2 lightmapCoord;
layout(location=4) in vec3 normal;
layout(location=5) in vec3 tangent;
layout(location=6) in vec3 binormal;
layout(location=7) in vec3 incident;
layout(location=8) in uint lightmapIndex;
layout(location=11) in uint sourceStippleLevel;
layout(location=12) in vec3 sourceLocalPosition;
layout(location=13) in float sourceColorAlpha;
#ifdef H2_EXTENDED_VERTEX_BANK
layout(location=9) in float prtCluster;
layout(location=10) in vec4 prtWeights;
#endif
layout(location=0) out vec3 world;
layout(location=1) out vec3 tint;
layout(location=2) out vec2 uv;
layout(location=3) out vec2 lightmapUV;
layout(location=4) out vec4 lightingDirection;
layout(location=5) out vec4 eyeDirection;
layout(location=6) flat out uint lightmapped;
layout(location=7) out vec4 forwardColor;
layout(location=8) out vec4 stageCoordinates[4];
layout(location=12) out vec2 titleDepth;
layout(location=13) flat out uint stippleLevel;
layout(location=14) flat out vec4 sourceEmblemWindow;
layout(push_constant) uniform Camera { mat4 viewProjection; vec4 eyeForward; vec4 eyePosition; vec4 viewport; } camera;
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
    uvec4 vertexDeclaration;
} material;
#ifdef H2_EXTENDED_VERTEX_BANK
layout(set=0,binding=7,std140) uniform VertexBank {
    uvec4 range; // First source constant, count, PRT stream, actual c0..11.
    vec4 constants[158];
} vertexBank;
#endif
#define RETAIL_VERTEX_RADIANCE color
#define H2_SOURCE_VIEW_SHADER_CONSTANTS
#define H2_SOURCE_VERTEX_DECLARATION
#define H2_SOURCE_IMMEDIATE_COLOR11
#include "vertex_program.glsl"
#include "source_cpu_skin_outputs.glsl"
void main() {
    gl_Position=camera.viewProjection*vec4(position,1);
    world=position;tint=color;uv=texcoord;lightmapUV=lightmapCoord;
    stippleLevel=sourceStippleLevel;
    sourceEmblemWindow=vec4(0);
    titleDepth=vec2(dot(position-camera.eyePosition.xyz,camera.eyeForward.xyz)*camera.eyeForward.w/1024.0,camera.eyeForward.w);
    if(material.sourceCameraControl.x!=0u && camera.eyeForward.w!=0.0) {
        // NV2A W-depth consumes the submitted camera's scaled c3 row.
        // Keep raw 24-bit W through perspective-correct interpolation.
        titleDepth=vec2(dot(vec4(position,1),vpConstant(3u)),camera.eyeForward.w);
    }
    lightmapped=lightmapIndex;
    // Recovered lightmap_perpixel_tex[_detail] vertex program: project the
    // incident vector into the authored tangent basis, normalize it and retain
    // its magnitude in oD1.a. oD0 is the eye vector in the same basis.
    float magnitude=length(incident);
    vec3 direction=magnitude>0.0?incident/magnitude:vec3(0);
    lightingDirection=vec4(vec3(dot(direction,binormal),dot(direction,tangent),dot(direction,normal))*0.5+0.5,magnitude);
    eyeDirection=vec4(vec3(dot(camera.eyeForward.xyz,binormal),dot(camera.eyeForward.xyz,tangent),dot(camera.eyeForward.xyz,normal))*0.5+0.5,1);
    // transparent_generic_reflection oD0 uses constant attributes v10/v11
    // and the surface's angle to the eye, rather than diffuse/specular v3/v4.
    vec3 toEye=camera.eyePosition.xyz-position;
    float eyeDistance=length(toEye);
    float facing=eyeDistance>0.0?max(dot(normal,toEye/eyeDistance),0.0):0.0;
    forwardColor=clamp(material.diffuseColor*facing+material.specularColor*(1.0-facing),0.0,1.0);
    for(uint i=0u;i<4u;++i)stageCoordinates[i]=vec4(texcoord,0,1);
    if(material.flags.w!=0u) {
        runVertexProgram();
        applySourceCPUSkinTextureCoordinates();
        // Authored oPos is already in target pixels after perspective divide.
        // NV2A keeps 1/16-pixel XY precision. Undo that viewport transform
        // for Vulkan, including the original c11 half-pixel position bias.
        vec4 screen=vpOut[0];
        screen.xy=trunc(screen.xy*16.0)/16.0;
        screen.w=(screen.w<0.0?-1.0:1.0)*clamp(abs(screen.w),5.421010862427522e-20,1.8446744073709552e19);
        sourceEmblemWindow=screen;
        vec2 ndc=(screen.xy-camera.viewport.xy)*2.0/camera.viewport.zw-1.0;
        gl_Position=vec4(ndc*screen.w,(screen.z/16777215.0)*screen.w,screen.w);
        eyeDirection=clamp(vpOut[3],0.0,1.0);
        lightingDirection=clamp(vpOut[4],0.0,1.0);
        forwardColor=eyeDirection;
        for(uint i=0u;i<4u;++i)stageCoordinates[i]=vpOut[9u+i];
    }
}
