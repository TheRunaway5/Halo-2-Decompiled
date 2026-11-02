#version 450
#extension GL_GOOGLE_include_directive : require
#define H2_PATH_TRACING_SOURCE_DEPTH 1
layout(set=1,binding=10,std140) uniform TraceSourceProjection {
    vec4 viewport;
    vec4 clipRange;
} traceSourceProjection;
// The catalog generator retains the original vertex program and turns its
// ordinary output declarations into private globals, preserving every op.
#define main sourceDepthMain
#include "world_vertex.glsl"
#undef main
layout(location=14) in int sourceTraceCluster;
layout(location=0) out vec3 traceWorld;
layout(location=1) out vec2 traceTexcoord;
layout(location=2) out vec3 traceWorldNormal;
layout(location=3) flat out uint traceMaterialIndex;
layout(location=4) out vec2 traceTitleDepth;
layout(location=5) flat out uint traceStippleLevel;
layout(location=6) out vec3 traceTint;
layout(location=8) flat out uint sourceDepthRaw;
layout(location=9) out vec2 traceLightmapUV;
layout(location=10) flat out uint traceLightmapIndex;
layout(location=11) out vec3 traceVertexRadiance;
layout(location=12) flat out vec4 traceSourceWindow;
void main() {
    sourceDepthMain();
    traceWorld=position;traceTexcoord=texcoord;traceWorldNormal=normal;
    traceMaterialIndex=floatBitsToUint(camera.viewport.w)&0x7fffffffu;
    traceTitleDepth=titleDepth;sourceDepthRaw=2u;
    traceStippleLevel=material.sourceCameraControl.y!=0u?sourceStippleLevel:16u;
    traceTint=lightmapIndex!=0u || sourceTraceCluster!=-1?vec3(1):color;
    traceLightmapUV=lightmapCoord;traceLightmapIndex=lightmapIndex;traceVertexRadiance=color;
    traceSourceWindow=sourceEmblemWindow;
}
