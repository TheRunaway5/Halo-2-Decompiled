#version 450
// Test adapter: exact captured original post-transform armor triangles.
layout(set=0,binding=7,std140) uniform CapturedVertices {
    vec4 positions[256];
    vec4 coordinates[256];
    vec4 windows[256];
} captured;
// Match the production world vertex interface and execute its geometry stage.
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
void main() {
    gl_Position=captured.positions[gl_VertexIndex];
    world=tint=vec3(0);uv=lightmapUV=vec2(0);
    lightingDirection=eyeDirection=forwardColor=vec4(0,0,0,1);
    lightmapped=0u;stippleLevel=16u;
    stageCoordinates[0]=captured.coordinates[gl_VertexIndex];
    for(uint i=1u;i<4u;++i)stageCoordinates[i]=vec4(0,0,0,1);
    titleDepth=vec2(gl_Position.w,1);
    sourceEmblemWindow=captured.windows[gl_VertexIndex];
}
