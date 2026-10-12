// Match the captured title PshUniform.consts upload. Keep the source program
// words for operation decoding, but do not expand their packed colors on GPU.
vec4 titlePatchyConstant(uint index) {
    uint slot=index<18u?(index-10u)*2u:index<26u?(index-18u)*2u+1u:16u+index-43u;
    return material.combinerConstants[slot];
}
#define H2_COMBINER_CONSTANT(index) titlePatchyConstant(index)
