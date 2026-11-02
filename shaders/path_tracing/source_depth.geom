#version 450
// Retain the original ordered source window vertices for integer W depth.
layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;
layout(location=0) in vec3 inWorld[];
layout(location=1) in vec2 inUV[];
layout(location=2) in vec3 inNormal[];
layout(location=3) flat in uint inMaterial[];
layout(location=4) in vec2 inDepth[];
layout(location=5) flat in uint inStipple[];
layout(location=6) in vec3 inTint[];
layout(location=8) flat in uint inRaw[];
layout(location=9) in vec2 inLightmapUV[];
layout(location=10) flat in uint inLightmap[];
layout(location=11) in vec3 inRadiance[];
layout(location=12) flat in vec4 inWindow[];
layout(location=0) out vec3 traceWorld;
layout(location=1) out vec2 traceTexcoord;
layout(location=2) out vec3 traceWorldNormal;
layout(location=3) flat out uint traceMaterialIndex;
layout(location=4) out vec2 titleDepth;
layout(location=5) flat out uint stippleLevel;
layout(location=6) out vec3 traceTint;
layout(location=8) flat out uint sourceDepthRaw;
layout(location=9) out vec2 traceLightmapUV;
layout(location=10) flat out uint traceLightmapIndex;
layout(location=11) out vec3 traceVertexRadiance;
layout(location=12) flat out vec4 traceSourceWindow0;
layout(location=13) flat out vec4 traceSourceWindow1;
layout(location=14) flat out vec4 traceSourceWindow2;
void main() {
    for(int i=0;i<3;++i) {
        gl_Position=gl_in[i].gl_Position;
        traceWorld=inWorld[i];traceTexcoord=inUV[i];traceWorldNormal=inNormal[i];
        traceMaterialIndex=inMaterial[i];titleDepth=inDepth[i];stippleLevel=inStipple[i];
        traceTint=inTint[i];sourceDepthRaw=inRaw[i];traceLightmapUV=inLightmapUV[i];
        traceLightmapIndex=inLightmap[i];traceVertexRadiance=inRadiance[i];
        traceSourceWindow0=inWindow[0];traceSourceWindow1=inWindow[1];traceSourceWindow2=inWindow[2];
        EmitVertex();
    }
    EndPrimitive();
}
