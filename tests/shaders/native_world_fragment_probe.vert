#version 450
// Test-only source varyings. Every six vertices cover one framebuffer pixel.
// Production fragment modules and their material/texture ABI stay unchanged.
struct Probe {vec4 coordinates[4];vec4 forwardColor;vec4 lightingDirection;};
layout(set=0,binding=8,std140) uniform Probes {uvec4 count;Probe probes[32];} inputs;
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
void main() {
    const vec2 corners[6]=vec2[](vec2(0,0),vec2(1,0),vec2(0,1),vec2(0,1),vec2(1,0),vec2(1,1));
    uint index=uint(gl_VertexIndex)/6u;
    vec2 p=corners[uint(gl_VertexIndex)%6u];
    p.x=(p.x+float(index))/float(inputs.count.x);
    gl_Position=vec4(p*2.0-1.0,0.5,1);
    world=vec3(0);tint=vec3(1);uv=lightmapUV=vec2(0);
    lightingDirection=inputs.probes[index].lightingDirection;
    forwardColor=inputs.probes[index].forwardColor;eyeDirection=vec4(0,0,1,1);
    for(uint stage=0u;stage<4u;++stage)stageCoordinates[stage]=inputs.probes[index].coordinates[stage];
    lightmapped=0u;titleDepth=vec2(0);stippleLevel=16u;
}
