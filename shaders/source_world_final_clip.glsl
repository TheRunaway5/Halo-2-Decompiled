vec4 sourceFinalWorldClip(vec4 screen,out vec4 window) {
    // Shared with every authored AOT program: native procedural reflection
    // must retain the same half-pixel output,1/16 raster phase and depth/W.
    screen.xy=trunc(screen.xy*16.0)/16.0;
    screen.w=(screen.w<0.0?-1.0:1.0)*clamp(abs(screen.w),5.421010862427522e-20,1.8446744073709552e19);
    window=screen;
#ifdef H2_SOURCE_WORLD_DEPTH
    vec2 ndc=(2.0*screen.xy-camera.viewport.zw)/camera.viewport.zw;
#else
    vec2 ndc=(screen.xy-camera.viewport.xy)*2.0/camera.viewport.zw-1.0;
#endif
    return vec4(ndc*screen.w,(screen.z/16777215.0)*screen.w,screen.w);
}
