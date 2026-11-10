void inverse_frame(SourcePhysicsContactMemory &memory,std::uint32_t source,
    std::uint32_t destination,const SourcePhysicsContactRecordObserver &observe) {
    const auto scale=read(memory,source);
    if(equal(scale,0)) {
        for(unsigned n=0;n<13;++n)publish(memory,observe,0x1441b7u,destination+n*4u,0);
        return;
    }
    const auto retained_scale=read(memory,source),one=read(memory,0x46a0d0u);
    const auto unit_scale=equal(retained_scale,one);
    auto px=sub(0,read(memory,source+0x28u));
    auto py=sub(0,read(memory,source+0x2cu));
    auto pz=sub(0,read(memory,source+0x30u));
    if(!unit_scale) {
        const auto reciprocal=div(one,retained_scale);px=mul(reciprocal,px);
        publish(memory,observe,0x1440e2u,destination,reciprocal);
        py=mul(reciprocal,py);pz=mul(reciprocal,pz);
    }else publish(memory,observe,0x1440f6u,destination,one);
    const auto fx=read(memory,source+4u);publish(memory,observe,0x1440fdu,destination+4u,fx);
    const auto ly=read(memory,source+0x14u);
    const auto output_fx=read(memory,destination+4u);publish(memory,observe,0x144108u,destination+0x14u,ly);
    const auto uz=read(memory,source+0x24u);publish(memory,observe,0x14410eu,destination+0x24u,uz);
    const auto lx=read(memory,source+0x10u),fy=read(memory,source+8u);
    publish(memory,observe,0x144119u,destination+8u,lx);publish(memory,observe,0x14411eu,destination+0x10u,fy);
    const auto ux=read(memory,source+0x1cu),fz=read(memory,source+0xcu);
    const auto forward_product=mul(output_fx,px);publish(memory,observe,0x14412du,destination+0x1cu,fz);
    auto tx=mul(read(memory,destination+0x1cu),pz);tx=add(tx,forward_product);
    const auto output_lx=read(memory,destination+0x10u);
    publish(memory,observe,0x144142u,destination+0xcu,ux);
    const auto uy=read(memory,source+0x20u),lz=read(memory,source+0x18u);
    tx=add(tx,mul(output_lx,py));const auto output_fy=read(memory,destination+8u);
    publish(memory,observe,0x14415cu,destination+0x28u,tx);
    auto ty=mul(output_fy,px);publish(memory,observe,0x144165u,destination+0x20u,lz);
    ty=add(mul(read(memory,destination+0x20u),pz),ty);
    ty=add(ty,mul(py,read(memory,destination+0x14u)));publish(memory,observe,0x144181u,destination+0x2cu,ty);
    auto tz=mul(read(memory,destination+0xcu),px);const auto up_component=read(memory,destination+0x24u);
    publish(memory,observe,0x144194u,destination+0x18u,uy);
    const auto up_product=mul(up_component,pz);
    tz=add(tz,up_product);tz=add(tz,mul(uy,py));publish(memory,observe,0x1441a9u,destination+0x30u,tz);
}
