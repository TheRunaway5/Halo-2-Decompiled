#version 450
layout(lines) in;
layout(line_strip,max_vertices=2) out;
layout(location=0) in vec4 inputColor0[];
layout(location=1) in vec4 inputColor1[];
layout(location=2) flat in vec4 inputPosition[];
layout(location=0) out vec4 color0;
layout(location=1) out vec4 color1;
layout(location=2) flat out vec4 position0;
layout(location=3) flat out vec4 position1;
layout(location=4) flat out vec4 position2;
void main() {
    // Retained xemu glsl/geom.c emit_line: its perpendicular third vertex
    // lets the shared perspective W-buffer reconstruction handle a line.
    vec2 delta=inputPosition[1].xy-inputPosition[0].xy;
    vec2 third=vec2(-delta.y,delta.x)+inputPosition[0].xy;
    for(int i=0;i<2;++i) {
        gl_Position=gl_in[i].gl_Position;
        color0=inputColor0[i];color1=inputColor1[i];
        position0=inputPosition[0];position1=inputPosition[1];
        position2=vec4(third,inputPosition[0].zw);
        EmitVertex();
    }
    EndPrimitive();
}
