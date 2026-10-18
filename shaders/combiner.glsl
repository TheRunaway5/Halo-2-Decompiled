// NV2A register-combiner arithmetic. Shader inputs, outputs and constants are
// decoded directly from the retail D3DPIXELSHADERDEF words. Texture addressing
// and vertex programs are separate stages; this file does not invent lighting.
#ifndef H2_FIXED_STAGE_UNROLL
#define H2_FIXED_STAGE_UNROLL
#endif
#ifndef H2_COMBINER_CONSTANT
#define H2_COMBINER_CONSTANT(index) argb(word(index))
#endif
vec4 registers[16];
uint word(uint index) {
#ifdef H2_COMBINER_WORD
    return H2_COMBINER_WORD(index);
#else
    return material.program[index / 4u][index % 4u];
#endif
}
vec4 argb(uint color) {
    return vec4((color>>16u)&255u,(color>>8u)&255u,color&255u,color>>24u)/255.0;
}
vec3 inputValue(uint operand, bool alpha) {
    vec4 value=registers[operand&15u];
    vec3 selected=alpha ? vec3((operand&16u)!=0u ? value.a : value.b)
                        : ((operand&16u)!=0u ? value.aaa : value.rgb);
    switch((operand>>5u)&7u) {
        case 0u:return max(selected,0.0);
        case 1u:return 1.0-clamp(selected,0.0,1.0);
        case 2u:return 2.0*max(selected,0.0)-1.0;
        case 3u:return 1.0-2.0*max(selected,0.0);
        case 4u:return max(selected,0.0)-0.5;
        case 5u:return 0.5-max(selected,0.0);
        case 6u:return selected;
        default:return -selected;
    }
}
vec3 outputValue(vec3 value,uint operation) {
    switch((operation>>15u)&7u) {
        case 1u:value-=0.5;break;
        case 2u:value*=2.0;break;
        case 3u:value=(value-0.5)*2.0;break;
        case 4u:value*=4.0;break;
        case 6u:value*=0.5;break;
    }
    return clamp(value,-1.0,1.0);
}
void storeOutput(uint destination,vec3 value,bool alpha,bool blue) {
    if(destination==0u)return;
    if(alpha)registers[destination].a=value.x;
    else {
        registers[destination].rgb=value;
        if(blue)registers[destination].a=value.b;
    }
}
// Both RGB and alpha read the register state before either writes its results.
void combine(uint inputs,uint operation,bool alpha,out vec3 ab,out vec3 cd,out vec3 sum) {
    vec3 a=inputValue(inputs>>24u,alpha), b=inputValue(inputs>>16u,alpha);
    vec3 c=inputValue(inputs>>8u,alpha), d=inputValue(inputs,alpha);
    ab=(!alpha && (operation&0x2000u)!=0u)?vec3(dot(a,b)):a*b;
    cd=(!alpha && (operation&0x1000u)!=0u)?vec3(dot(c,d)):c*d;
    bool mux=(word(53u)&0x100u)!=0u ? registers[12].a>=0.5 : (uint(registers[12].a*255.0)&1u)!=0u;
    sum=(operation&0x4000u)!=0u?(mux?cd:ab):ab+cd;
    ab=outputValue(ab,operation); cd=outputValue(cd,operation); sum=outputValue(sum,operation);
}
void commit(uint operation,bool alpha,vec3 ab,vec3 cd,vec3 sum) {
    storeOutput((operation>>4u)&15u,ab,alpha,(operation&0x80000u)!=0u);
    storeOutput(operation&15u,cd,alpha,(operation&0x40000u)!=0u);
    storeOutput((operation>>8u)&15u,sum,alpha,false);
}
vec4 runCombiners(vec4 textures[4],vec4 v0,vec4 v1) {
    H2_FIXED_STAGE_UNROLL for(int i=0;i<16;++i)registers[i]=vec4(0);
#ifdef H2_COMBINER_FOG
    registers[3]=H2_COMBINER_FOG();
#endif
    registers[4]=v0;registers[5]=v1;
    H2_FIXED_STAGE_UNROLL for(int i=0;i<4;++i)registers[8+i]=textures[i];
    registers[12].a=(word(54u)&31u)!=0u?textures[0].a:1.0;
    H2_FIXED_STAGE_UNROLL for(uint stage=0u;stage<8u;++stage) {
        if(stage>=min(word(53u)&255u,8u))break;
        registers[1]=H2_COMBINER_CONSTANT(10u+((word(53u)&0x1000u)!=0u?stage:0u));
        registers[2]=H2_COMBINER_CONSTANT(18u+((word(53u)&0x10000u)!=0u?stage:0u));
        vec3 rab,rcd,rsum,aab,acd,asum;
        combine(word(34u+stage),word(45u+stage),false,rab,rcd,rsum);
        combine(word(stage),word(26u+stage),true,aab,acd,asum);
        commit(word(45u+stage),false,rab,rcd,rsum);
        commit(word(26u+stage),true,aab,acd,asum);
    }
    uint abcd=word(8u),efg=word(9u);
    if(abcd==0u && efg==0u)return clamp(registers[12],0.0,1.0);
    registers[1]=H2_COMBINER_CONSTANT(43u);registers[2]=H2_COMBINER_CONSTANT(44u);
    vec3 sum=((efg&64u)!=0u?1.0-registers[5].rgb:registers[5].rgb)
             +((efg&32u)!=0u?1.0-registers[12].rgb:registers[12].rgb);
    registers[14]=vec4((efg&128u)!=0u?clamp(sum,0.0,1.0):sum,0);
    registers[15]=vec4(inputValue(efg>>24u,false)*inputValue(efg>>16u,false),0);
    vec3 a=inputValue(abcd>>24u,false),b=inputValue(abcd>>16u,false);
    vec3 c=inputValue(abcd>>8u,false),d=inputValue(abcd,false);
    return clamp(vec4(a*b+(1.0-a)*c+d,inputValue(efg>>8u,true).x),0.0,1.0);
}
