#version 450
// Controlled raster/target fixture only; original transformed VP outputs are
// independently checked by source_shadow_vertex_tests through vertex_program.
layout(push_constant) uniform Position {vec4 rectangle;vec4 depth;} position;
layout(location=0) out vec2 uv;
void main() {
    const vec2 corners[6]=vec2[6](vec2(-1,-1),vec2(1,-1),vec2(1,1),vec2(-1,-1),vec2(1,1),vec2(-1,1));
    vec2 p=corners[gl_VertexIndex];
    uv=(p+1.0)*0.5;
    gl_Position=vec4(mix(position.rectangle.xy,position.rectangle.zw,uv),position.depth.x,1.0);
}
