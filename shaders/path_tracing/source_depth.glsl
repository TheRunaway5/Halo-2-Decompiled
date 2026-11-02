// This projection is available only to a native catalog vertex stage and its
// ordered triangle transport. Same clip/jitter/W reconstruction as world.frag.
layout(location=12) flat in vec4 traceSourceWindow0;
layout(location=13) flat in vec4 traceSourceWindow1;
layout(location=14) flat in vec4 traceSourceWindow2;
layout(set=1,binding=10,std140) uniform TraceSourceProjection {
    vec4 viewport;
    vec4 clipRange;
} traceSourceProjection;
float traceSourceDepth() {
    float sourceW=worldWDepth(gl_FragCoord.xy-traceSourceProjection.clipRange.zw,vec2(1),
        traceSourceWindow0,traceSourceWindow1,traceSourceWindow2,0,0,0);
    if(sourceW<traceSourceProjection.clipRange.x || traceSourceProjection.clipRange.y<sourceW)discard;
    return worldD24Depth(sourceW);
}
