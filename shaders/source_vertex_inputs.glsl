// Original1C330 register ordinals select literal vrtx semantic IDs. The
// already-decoded world streams preserve the matching source format defaults.
// Unlisted registers remain the original constant vertex attributes.
bool loadSourceWorldVertexInputs() {
    if((material.vertexDeclaration.x&255u)==255u)return false;
    for(uint reg=0u;reg<16u;++reg) {
        uint semantic=(material.vertexDeclaration[reg/4u]>>((reg&3u)*8u))&255u;
        if(semantic==255u)continue;
        if(reg==0u)semantic&=127u;
        if(semantic==0u || semantic==64u)vpInput[reg]=vec4(position,1);
        else if(semantic==3u)vpInput[reg]=vec4(texcoord,0,1);
        else if(semantic==4u)vpInput[reg]=vec4(normal,1);
        else if(semantic==68u) {
            // Explicit compiled skinned-normal operation, absent from the
            // matching world program. Preserve source DP3/RSQ/MUL order.
            precise vec3 squared=normal*normal;
            precise float xy=squared.x+squared.y;
            precise float lengthSquared=xy+squared.z;
            float inverse=inversesqrt(abs(lengthSquared));
            vec3 transformed=vec3(normal.x==0?0:normal.x*inverse,
                normal.y==0?0:normal.y*inverse,normal.z==0?0:normal.z*inverse);
            vpInput[reg]=vec4(transformed,1);
        }
        else if(semantic==5u)vpInput[reg]=vec4(tangent,1);
        else if(semantic==6u)vpInput[reg]=vec4(binormal,1);
        else if(semantic==8u)vpInput[reg]=vec4(incident,1);
        else if(semantic==9u)vpInput[reg]=vec4(lightmapCoord,0,1);
#ifdef RETAIL_VERTEX_RADIANCE
        else if(semantic==14u)vpInput[reg]=vec4(RETAIL_VERTEX_RADIANCE,1);
#endif
#ifdef H2_EXTENDED_VERTEX_BANK
        else if(semantic==19u)vpInput[reg]=vec4(prtCluster,0,0,1);
        else if(semantic==20u)vpInput[reg]=prtWeights;
#endif
    }
    return true;
}
