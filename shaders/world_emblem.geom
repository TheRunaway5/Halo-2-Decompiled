#version 450
// Preserve the submitted triangle order for the original integer W-depth
// comparison. All armor shading and texture varyings pass through unchanged.
layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;
layout(location=0) in vec3 inWorld[];
layout(location=1) in vec3 inTint[];
layout(location=2) in vec2 inUV[];
layout(location=3) in vec2 inLightmapUV[];
layout(location=4) in vec4 inLightingDirection[];
layout(location=5) in vec4 inEyeDirection[];
layout(location=6) flat in uint inLightmapped[];
layout(location=7) in vec4 inForwardColor[];
layout(location=8) in vec4 inStageCoordinates[][4];
layout(location=12) in vec2 inTitleDepth[];
layout(location=13) flat in uint inStippleLevel[];
layout(location=14) flat in vec4 inSourceWindow[];
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
layout(location=14) flat out vec4 sourceEmblemWindow0;
layout(location=15) flat out vec4 sourceEmblemWindow1;
layout(location=16) flat out vec4 sourceEmblemWindow2;
void main() {
    for(int i=0;i<3;++i) {
        gl_Position=gl_in[i].gl_Position;
        world=inWorld[i];tint=inTint[i];uv=inUV[i];lightmapUV=inLightmapUV[i];
        lightingDirection=inLightingDirection[i];eyeDirection=inEyeDirection[i];
        lightmapped=inLightmapped[i];forwardColor=inForwardColor[i];
        for(int stage=0;stage<4;++stage)stageCoordinates[stage]=inStageCoordinates[i][stage];
        titleDepth=inTitleDepth[i];stippleLevel=inStippleLevel[i];
        sourceEmblemWindow0=inSourceWindow[0];
        sourceEmblemWindow1=inSourceWindow[1];
        sourceEmblemWindow2=inSourceWindow[2];
        EmitVertex();
    }
    EndPrimitive();
}
