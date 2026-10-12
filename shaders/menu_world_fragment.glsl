#ifndef H2_MENU_WORLD_FRAGMENT_GLSL
#define H2_MENU_WORLD_FRAGMENT_GLSL
// Independently written interface adapter around the project's MIT kernels.
// The caller owns program words, completed constants, texture bindings and
// depth/clip/alpha state. No owner, pass, fog color or camera is selected here.
// Raw native callers MUST provide H2_COMBINER_CONSTANT from the explicitly
// bound completed float bank. Original-layout diagnostics already supply the
// actual original consts18 bank through that same hook.
// Required hooks: MENU_WORLD_PROGRAM_WORD, CLIP_RANGE, CLIP_REGION,
// DEPTH_FACTOR, DEPTH_OFFSET, SURFACE_SCALE, ALPHA_STATE, TEXTURE_EFFECTS and
// BUMP_MATRIX (all prefixed MENU_WORLD_). The caller supplies unchanged
// stageCoordinates[4] plus fetchStage2D/Cube/Projected2D. The projected hook
// preserves original textureProj and rectangle normalization BEFORE division.
#ifndef H2_COMBINER_CONSTANT
#error Raw_world_requires_an_explicit_completed_float_constant_bank
#endif
struct MenuWorldPixelProgramView {uvec4 program[15];};
MenuWorldPixelProgramView menuWorldPixelProgram;
#define material menuWorldPixelProgram
#include "combiner.glsl"
#undef material
#include "source_alpha_test.glsl"
#include "world_depth_fragment.glsl"

#ifndef MENU_WORLD_ALPHA_VALUE
#define MENU_WORLD_ALPHA_VALUE(pixel) ((pixel).a)
#endif
#ifndef MENU_WORLD_ALPHA_COMPARE
#define MENU_WORLD_ALPHA_COMPARE(alpha,comparison,reference) sourceAlphaTestPass(alpha,comparison,reference)
#endif

vec4 menuWorldTexture2D(uint stage,vec2 coordinates);
#define fetchStage2D menuWorldTexture2D
#define TEXTURE_REJECT() demote
#define TEXTURE_BUMP_MATRIX(i) MENU_WORLD_BUMP_MATRIX(i)
#define TEXTURE_COLOR_SIGN(i) (MENU_WORLD_TEXTURE_EFFECTS(i).y>>1u)
#ifdef MENU_WORLD_TEXTURE_VOLUME
#define TEXTURE_VOLUME(i) MENU_WORLD_TEXTURE_VOLUME(i)
#endif
#include "texture_program.glsl"
#undef fetchStage2D
#undef TEXTURE_REJECT
#undef TEXTURE_BUMP_MATRIX
#undef TEXTURE_COLOR_SIGN
#undef TEXTURE_VOLUME
vec4 menuWorldTexture2D(uint stage,vec2 coordinates) {
    if(textureMode(stage)==1u || textureMode(stage)==2u)
        return fetchStageProjected2D(stage,stageCoordinates[stage]);
    return fetchStage2D(stage,coordinates);
}
vec4 menuWorldFragment(vec4 front0,vec4 front1,vec4 p0,vec4 p1,vec4 p2,float slope) {
    H2_FIXED_STAGE_UNROLL for(uint i=0u;i<15u;++i)
        menuWorldPixelProgram.program[i]=uvec4(
            MENU_WORLD_PROGRAM_WORD(i*4u),MENU_WORLD_PROGRAM_WORD(i*4u+1u),
            MENU_WORLD_PROGRAM_WORD(i*4u+2u),MENU_WORLD_PROGRAM_WORD(i*4u+3u));
    vec2 position=gl_FragCoord.xy-vec2(0.5);
    bool contained=false;
    H2_FIXED_STAGE_UNROLL for(uint i=0u;i<8u;++i) {
        ivec4 region=MENU_WORLD_CLIP_REGION(i);
        if(all(greaterThanEqual(position,vec2(region.xy))) &&
           all(lessThan(position,vec2(region.zw)))) {contained=true;break;}
    }
    if(!contained)demote;
    float depth=worldWDepth(gl_FragCoord.xy,vec2(MENU_WORLD_SURFACE_SCALE()),
        p0,p1,p2,slope,MENU_WORLD_DEPTH_FACTOR(),MENU_WORLD_DEPTH_OFFSET());
    vec4 clip=MENU_WORLD_CLIP_RANGE();
    if(depth<clip.z || depth>clip.w)demote;
    gl_FragDepth=worldD24Depth(depth);
    vec4 samples[4];runTextureStages(samples);
    H2_FIXED_STAGE_UNROLL for(uint i=0u;i<4u;++i) {
        uint mode=textureMode(i);
        // Original NONE contributes RGB0/A1, independent of r0's separate
        // no-stage0 initialization. CLIPPLANE contributes all-zero lanes.
        if(mode==0u)samples[i]=vec4(0.0,0.0,0.0,1.0);
        if((MENU_WORLD_TEXTURE_EFFECTS(i).x&4u)!=0u &&
           (mode==1u || mode==2u || mode==3u || mode==9u || mode==11u || mode==12u ||
            mode==14u || mode==15u || mode==16u) && samples[i].a==0.0)demote;
    }
    vec4 result=runCombiners(samples,front0,front1);
    uvec3 alpha=MENU_WORLD_ALPHA_STATE();
    if(alpha.x!=0u && !MENU_WORLD_ALPHA_COMPARE(MENU_WORLD_ALPHA_VALUE(result),alpha.y,alpha.z))demote;
    return result;
}
#endif
