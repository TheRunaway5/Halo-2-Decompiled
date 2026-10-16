// Keep the authored combiner's scratch registers and highlight response intact.
vec4 pstencilSpecularMask(vec4 authored,float materialAlpha) {
    return vec4(authored.rgb*materialAlpha,authored.a);
}
