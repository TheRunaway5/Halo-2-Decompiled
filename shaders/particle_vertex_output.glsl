// Output conversion in the captured original classification-7 vertex shader.
// The disc program already supplies screen XYZ/W and particle fog in oD1.
vec4 particleColor(vec4 value) {
    return clamp(mix(value,vec4(1),isnan(value)),0.0,1.0);
}
void particleVertexOutput() {
    vec4 screen=outv[0];
    screen.xy=trunc(screen.xy*16.0)/16.0;
    screen.w=clampAwayZeroInf(screen.w);
    vec4 clip=screen;
    clip.xy=(clip.xy*2.0-particleSurfaceSize)/particleSurfaceSize;
    clip.z/=particleClipRange.y;
    clip.xyz*=clip.w;
    v_vtxD0=particleColor(outv[3]);v_vtxD1=particleColor(outv[4]);
    v_vtxB0=particleColor(outv[7]);v_vtxB1=particleColor(outv[8]);
    float fogDistance=outv[5].x;
    float fogFactor=(particleFogParam.x+fogDistance*particleFogParam.y)-1.0;
    v_vtxFog=isinf(fogDistance)?1.0:isnan(fogFactor)?1.0:
        clamp(fogFactor,-uintBitsToFloat(0x7f7fffffu),uintBitsToFloat(0x7f7fffffu));
    v_vtxT0=outv[9];v_vtxT1=outv[10];v_vtxT2=outv[11];v_vtxT3=outv[12];
    v_vtxPos0=screen;v_vtxPos1=screen;v_vtxPos2=screen;v_triMZ=0.0;
    gl_Position=clip;gl_PointSize=1.0;
}
