// Original 1C1B0/1C290 pattern with the NV2A hardware byte-bit convention.
// Use global physical raster coordinates. View/geometry origins do not shift
// the pattern. Explicit row origin31 preserves the observed source480p phase.
bool sourceStipplePass(uint level,uint rowOrigin,uvec2 pixelPosition) {
    const uint rank[16]=uint[16](0u,12u,3u,10u,8u,4u,15u,6u,
        2u,11u,1u,13u,14u,7u,9u,5u);
    uint row=(rowOrigin-pixelPosition.y)&31u;
    uint bit=(pixelPosition.x&24u)|(7u-(pixelPosition.x&7u));
    return rank[(row&3u)*4u+(bit&3u)]<level;
}
