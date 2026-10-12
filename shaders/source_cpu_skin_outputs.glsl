// Exact local/skinned transparent_generic_offset lowering. The selected
// compiled family/class sets bit6 on its POSITION selector. Geometry keeps
// posed position and raw normal; only the six noise coordinates use the
// separately retained decompressed local position. Original o9/o10.w survive.
void applySourceCPUSkinTextureCoordinates() {
    if((material.vertexDeclaration.x&255u)!=192u)return;
    for(uint stage=0u;stage<2u;++stage)for(uint axis=0u;axis<3u;++axis) {
        vec4 row=vpConstant(18u+stage*3u+axis);
        precise vec4 products=vpProduct(vec4(sourceLocalPosition,1),row);
        precise float xy=products.x+products.y;
        precise float xyz=xy+products.z;
        vpOut[9u+stage][axis]=xyz+row.w;
    }
}
