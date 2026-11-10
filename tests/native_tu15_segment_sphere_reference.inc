bool native_tu15_segment_near_sphere(SourcePhysicsContactMemory&memory,
    std::uint32_t first,std::uint32_t center,std::uint32_t direction,std::uint32_t radius_bits,NativeTu15PhysicsFrameFp&fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SseEnvironment environment(fp);const SourcePhysicsContactRecordObserver empty;Memory m(memory,empty);
    const auto read=[&](std::uint32_t p){return scalar(m.word(p));};
    const auto cy=read(center+4u),fy=read(first+4u);const auto dy=sub(cy,fy);
    const auto cz=read(center+8u),fz=read(first+8u);const auto dz=sub(cz,fz);
    const auto cx=read(center),fx=read(first);const auto dx=sub(cx,fx);
    auto distance=add(mul(dz,dz),mul(dy,dy));distance=add(distance,mul(dx,dx));
    const auto radius=scalar(radius_bits),zero=F{};distance=sub(distance,mul(radius,radius));
    if(compare(zero,distance).above())return true;
    const auto raw_x=m.word(direction),raw_y=m.word(direction+4u),raw_z=m.word(direction+8u);
    const auto vx=scalar(raw_x),vy=scalar(raw_y),vz=scalar(raw_z);
    auto dot=add(mul(vz,dz),mul(vy,dy));dot=add(dot,mul(vx,dx));
    if(!compare(dot,zero).below)return false;
    auto length=add(mul(vz,vz),mul(vy,vy));length=add(length,mul(vx,vx));
    const auto discriminant=sub(mul(dot,dot),mul(length,distance));
    if(!compare(zero,discriminant).below)return false;
    auto endpoint=sub(zero,length);endpoint=sub(endpoint,dot);
    if(compare(zero,endpoint).above())return true;
    return compare(discriminant,mul(endpoint,endpoint)).above();
#else
    (void)memory;(void)first;(void)center;(void)direction;(void)radius_bits;(void)fp;
    throw std::runtime_error("Exact native camera sphere query backend unavailable");
#endif
}
