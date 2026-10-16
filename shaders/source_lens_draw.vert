#version 450
// Authored lens_flare slot10: four MOVs copy v0, v3.xy, v9, v8.x.
layout(location=0) in vec4 sourcePosition;
layout(location=1) in vec2 sourceUV;
layout(location=2) in vec4 sourceColor;
layout(location=3) in float sourceSecondaryAlpha;
layout(push_constant) uniform LensView {vec2 surface;} view;
layout(location=0) out vec2 uv;
layout(location=1) out vec4 primary;
layout(location=2) out float secondaryAlpha;
layout(location=3) flat out float sourceW;
void main() {
    vec4 p=sourcePosition;
    p.xy=trunc(p.xy*16.0)/16.0;
    sourceW=p.w;
    p.xy=(2.0*p.xy-view.surface)/view.surface;
    p.z/=16777215.0;p.xyz*=p.w;gl_Position=p;
    uv=sourceUV;primary=sourceColor;secondaryAlpha=sourceSecondaryAlpha;
}
