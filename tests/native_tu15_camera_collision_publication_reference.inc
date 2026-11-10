void native_tu15_publish_camera_collision(SourcePhysicsContactMemory&m,std::uint32_t out,
    std::uint32_t transform,std::uint32_t collision,NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&o){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp);store(m,o,0x16b9a5u,out+4u,word(m,collision));
    if(transform){
        const auto plane=word(m,collision+4u);const auto y=scalar(word(m,plane+4u)),z=scalar(word(m,plane+8u));
        auto nx=scalar(word(m,transform+28u)),middle=scalar(word(m,transform+16u));const auto x=scalar(word(m,plane));nx=mul(nx,z);middle=mul(middle,y);nx=add(nx,middle);middle=scalar(word(m,transform+4u));middle=mul(middle,x);nx=add(nx,middle);store(m,o,0x16b9e2u,out+40u,bits(nx));
        auto ny=scalar(word(m,transform+32u));middle=scalar(word(m,transform+20u));ny=mul(ny,z);middle=mul(middle,y);ny=add(ny,middle);middle=scalar(word(m,transform+8u));middle=mul(middle,x);ny=add(ny,middle);store(m,o,0x16ba0au,out+44u,bits(ny));
        auto nz=scalar(word(m,transform+36u));nz=mul(nz,z);auto term=scalar(word(m,transform+24u));term=mul(term,y);middle=scalar(word(m,transform+12u));nz=add(nz,term);middle=mul(middle,x);nz=add(nz,middle);store(m,o,0x16ba32u,out+48u,bits(nz));
        auto distance=scalar(word(m,transform+48u));distance=mul(distance,nz);auto value=scalar(word(m,plane+12u));value=mul(value,scalar(word(m,transform)));distance=add(distance,value);value=scalar(word(m,transform+44u));value=mul(value,scalar(word(m,out+44u)));distance=add(distance,value);value=scalar(word(m,out+40u));value=mul(value,scalar(word(m,transform+40u)));distance=add(distance,value);store(m,o,0x16ba69u,out+52u,bits(distance));
        const auto index=word(m,collision+16u);if(index&0x8000u){const auto zero=F{};for(unsigned lane=0;lane<4;++lane){constexpr std::array pc{0x16ba84u,0x16ba91u,0x16ba9eu,0x16baa8u};const auto n=sub(zero,scalar(word(m,out+40u+lane*4u)));store(m,o,pc[lane],out+40u+lane*4u,bits(n));}}
    }else{
        const auto index=word(m,collision+16u);
        if(index&0x8000u){const auto plane=word(m,collision+4u);const auto zero=F{};for(unsigned lane=0;lane<4;++lane){constexpr std::array pc{0x16bac3u,0x16bad0u,0x16baddu,0x16bae7u};const auto n=sub(zero,scalar(word(m,plane+lane*4u)));store(m,o,pc[lane],out+40u+lane*4u,bits(n));}}
        else{const auto plane=word(m,collision+4u);for(unsigned lane=0;lane<4;++lane){constexpr std::array pc{0x16baf7u,0x16bafcu,0x16bb02u,0x16bb08u};store(m,o,pc[lane],out+40u+lane*4u,word(m,plane+lane*4u));}}
    }
    for(unsigned lane=0;lane<3;++lane){constexpr std::array pc{0x16bb0fu,0x16bb15u,0x16bb1bu};store(m,o,pc[lane],out+76u+lane*4u,word(m,collision+8u+lane*4u));}
    store(m,o,0x16bb21u,out+88u,word(m,collision+20u,1),1);store(m,o,0x16bb27u,out+89u,word(m,collision+21u,1),1);store(m,o,0x16bb2eu,out+90u,word(m,collision+22u,2),2);
#else
    (void)m;(void)out;(void)transform;(void)collision;(void)fp;(void)o;throw std::runtime_error("Native camera collision backend unavailable");
#endif
}
