// Texture addressing precedes the register combiners. The caller supplies
// stageCoordinates and the bound image lookup functions.
#ifndef H2_FIXED_STAGE_UNROLL
#define H2_FIXED_STAGE_UNROLL
#endif
#ifndef TEXTURE_COORDINATE_W
#define TEXTURE_COORDINATE_W(i) stageCoordinates[i].w
#endif
uint textureMode(uint stage) {return (word(54u)>>(stage*5u))&31u;}
uint previousTexture(uint stage) {
    // Stage 1 always reads stage 0. PSInputTexture names the earlier inputs
    // for stages 2/3 in bits 16/20 (retail D3DPIXELSHADERDEF word 56).
    return stage==1u?0u:(word(56u)>>((stage+2u)*4u))&3u;
}
vec3 textureDotInput(uint stage,vec4 previous) {
    uint mapping=(word(55u)>>((stage-1u)*4u))&7u;
    if(mapping==0u)return previous.rgb;
    // Xbox D3D normal encoding puts zero at byte 128 and +1 at byte 255.
    // It differs from the OpenGL expansion that centers between 127/128.
    if(mapping==1u)return (previous.rgb*255.0-128.0)/127.0;
    if(mapping==2u)return previous.rgb*2.0-1.0;
    // Other mappings require HILO/signed texture formats, not RGBA decoding.
    return vec3(0);
}
float textureDot(uint stage,vec4 samples[4]) {
    return dot(stageCoordinates[stage].xyz,
               textureDotInput(stage,samples[previousTexture(stage)]));
}
float textureSignedByte(float value) {
    // NV2A BUMPENVMAP interprets the original B/G bytes as signed values;
    // it does not use the normal-map expansion centered at byte 128.
    float byteValue=value*255.0;
    return (byteValue>=128.0?byteValue-256.0:byteValue)/127.0;
}
void runTextureStages(out vec4 samples[4]) {
    float products[4];bool valid[4];
    H2_FIXED_STAGE_UNROLL for(uint i=0u;i<4u;++i){samples[i]=vec4(0);products[i]=0.0;valid[i]=false;}
    H2_FIXED_STAGE_UNROLL for(uint i=0u;i<4u;++i) {
        uint mode=textureMode(i);
        if(mode==1u) {
#ifdef TEXTURE_VOLUME
            // The authored mode remains PROJECT2D even for a 3D image.
            // Retail 3FA3D0/3FFA80 only rewrite mode1 when PS word59 has
            // bit0x100; Elite's plasma definition has that bit clear.
            // NV2A PROJECT2D projects S/T by Q and supplies volume R=0.
            if(TEXTURE_VOLUME(i))
                samples[i]=fetchStage3D(i,vec3(stageCoordinates[i].xy/TEXTURE_COORDINATE_W(i),0.0));
            else
#endif
            samples[i]=fetchStage2D(i,stageCoordinates[i].xy/TEXTURE_COORDINATE_W(i));
            valid[i]=true;continue;
        }
        if(mode==3u) {
            samples[i]=fetchStageCube(i,stageCoordinates[i].xyz);
            valid[i]=true;continue;
        }
        if(mode==2u) {
            // NV2A PROJECT3D projects S/T/R by Q. xemu psh.c emits
            // textureProj(sampler3D,pT.xyzw); a 2D binding consumes S/T/Q.
#ifdef TEXTURE_VOLUME
            if(TEXTURE_VOLUME(i))
                samples[i]=fetchStage3D(i,stageCoordinates[i].xyz/TEXTURE_COORDINATE_W(i));
            else
#endif
                samples[i]=fetchStage2D(i,stageCoordinates[i].xy/TEXTURE_COORDINATE_W(i));
            valid[i]=true;continue;
        }
        if(mode==4u) {samples[i]=clamp(stageCoordinates[i],0.0,1.0);valid[i]=true;continue;}
        if(mode==5u) {
            uint comparisons=word(42u)>>(4u*i);
            H2_FIXED_STAGE_UNROLL for(uint axis=0u;axis<4u;++axis)
                if(((comparisons>>axis)&1u)!=0u ? stageCoordinates[i][axis]>=0.0 : stageCoordinates[i][axis]<0.0)
                    {TEXTURE_REJECT();}
            continue;
        }
        if(i==0u || mode==0u)continue;
        uint previous=previousTexture(i);
        if(previous>=i || !valid[previous] || textureMode(previous)==17u)continue;
        if(mode==6u) {
            uint sign=TEXTURE_COLOR_SIGN(previous);
            vec2 delta=vec2((sign&2u)!=0u?samples[previous].b:textureSignedByte(samples[previous].b),
                            (sign&4u)!=0u?samples[previous].g:textureSignedByte(samples[previous].g));
            vec4 matrix=TEXTURE_BUMP_MATRIX(i);
            // NV_PGRAPH_BUMPMAT00,01,10,11 are uploaded in column order.
            delta=mat2(matrix.xy,matrix.zw)*delta;
            samples[i]=fetchStage2D(i,stageCoordinates[i].xy+delta);
            valid[i]=true;continue;
        }
        if(mode==15u || mode==16u) {
            samples[i]=fetchStage2D(i,mode==15u?samples[previous].ar:samples[previous].gb);
            valid[i]=true;continue;
        }
        products[i]=textureDot(i,samples);
        if(mode==17u){valid[i]=true;continue;}
        if(i<2u || !valid[i-1u])continue;
        if(mode==9u && textureMode(i-1u)==17u) {
            samples[i]=fetchStage2D(i,vec2(products[i-1u],products[i]));valid[i]=true;
        }
        if(mode==11u && i==2u && textureMode(i-1u)==17u && textureMode(i+1u)==12u) {
            // The diffuse lookup uses the NEXT row's dot product, while the
            // following specular lookup retains this row's scalar product.
            uint nextInput=previousTexture(i+1u);
            if(nextInput>=i-1u || !valid[nextInput])continue;
            samples[i]=fetchStageCube(i,vec3(products[i-1u],products[i],textureDot(i+1u,samples)));
            valid[i]=true;
        }
        if(i==3u && valid[i-2u] && textureMode(i-2u)==17u) {
            vec3 n=vec3(products[i-2u],products[i-1u],products[i]);
            if(mode==14u && textureMode(i-1u)==17u) {
                samples[i]=fetchStageCube(i,n);valid[i]=true;
            }
            if(mode==12u && (textureMode(i-1u)==17u || textureMode(i-1u)==11u)) {
                vec3 e=vec3(stageCoordinates[i-2u].w,stageCoordinates[i-1u].w,stageCoordinates[i].w);
                // NV_texture_shader 3.8.13.1.18. E comes from q, not from a
                // guessed material camera vector or a normalized normal map.
                samples[i]=fetchStageCube(i,2.0*dot(n,e)/dot(n,n)*n-e);valid[i]=true;
            }
        }
    }
}
