// Shared native masked x87 file operations for17C020 and259B0.
// Preserve the caller host file and every live source lane/status bit.
enum class EffectFp { integer, single, add, multiply, compare, pop, subtract, multiply_pop };
void effect_fp(Abi &a,EffectFp operation,std::uint32_t operand=0) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER) && LDBL_MANT_DIG==64
    if(!a.floating_point || a.x87_depth>8 || a.active_mmx ||
        (a.floating_point->x87_control_word&0x3fu)!=0x3fu || (a.x87_status&0x8080u))
        throw std::runtime_error("Native effect owner requires its actual masked x87 file");
    const auto depth=a.x87_depth;
    if((operation==EffectFp::integer || operation==EffectFp::single) && depth==8)
        throw std::runtime_error("Native effect owner requires its actual free x87 stack slot");
    if(((operation==EffectFp::compare || operation==EffectFp::multiply_pop) && depth<2) ||
        ((operation==EffectFp::add || operation==EffectFp::multiply || operation==EffectFp::subtract || operation==EffectFp::pop) && !depth))
        throw std::runtime_error("Native effect owner lost its live x87 operands");
    alignas(16) std::array<std::byte,512> host{};
    alignas(16) std::array<std::byte,108> source{},result{};
    const auto cw=a.floating_point->x87_control_word;std::uint16_t tags=0xffffu;
    std::memcpy(source.data(),&cw,2);std::memcpy(source.data()+4,&a.x87_status,2);
    const auto top=(a.x87_status>>11u)&7u;
    for(unsigned n=0;n<depth;++n) {
        std::array<std::byte,10> raw;std::memcpy(raw.data(),&a.x87[n],10);
        std::uint64_t significand{};std::uint16_t exponent{};
        std::memcpy(&significand,raw.data(),8);std::memcpy(&exponent,raw.data()+8,2);exponent&=0x7fffu;
        const unsigned tag=(!exponent && !significand)?1u:(!exponent || exponent==0x7fffu || !(significand>>63u))?2u:0u;
        const auto shift=((top+n)&7u)*2u;tags=std::uint16_t((tags&~(3u<<shift))|(tag<<shift));
        std::memcpy(source.data()+28+n*10,raw.data(),10);
    }
    std::memcpy(source.data()+8,&tags,2);
#if defined(__x86_64__)
#define H2_EFFECT_FP_SAVE "fxsave64 %0\n\tfrstor %3\n\t"
#define H2_EFFECT_FP_RESTORE "\n\tfnsave %1\n\tfxrstor64 %0"
#else
#define H2_EFFECT_FP_SAVE "fxsave %0\n\tfrstor %3\n\t"
#define H2_EFFECT_FP_RESTORE "\n\tfnsave %1\n\tfxrstor %0"
#endif
#define H2_EFFECT_FP_OP(op) asm volatile(H2_EFFECT_FP_SAVE op H2_EFFECT_FP_RESTORE \
        :"=m"(host),"=m"(result):"m"(operand),"m"(source) \
        :"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)")
    switch(operation) {
    case EffectFp::integer:H2_EFFECT_FP_OP("fildl %2");break;
    case EffectFp::single:H2_EFFECT_FP_OP("flds %2");break;
    case EffectFp::add:H2_EFFECT_FP_OP("fadds %2");break;
    case EffectFp::multiply:H2_EFFECT_FP_OP("fmuls %2");break;
    case EffectFp::pop:H2_EFFECT_FP_OP("fstp %%st(0)");break;
    case EffectFp::subtract:H2_EFFECT_FP_OP("fsubs %2");break;
    case EffectFp::multiply_pop:H2_EFFECT_FP_OP("fmulp %%st,%%st(1)");break;
    case EffectFp::compare: {
        unsigned char carry,zero,parity;
#if defined(__x86_64__)
        asm volatile("fxsave64 %0\n\tfrstor %5\n\tfcomip %%st(1),%%st\n\tsetb %2\n\tsete %3\n\tsetp %4\n\tfnsave %1\n\tfxrstor64 %0"
#else
        asm volatile("fxsave %0\n\tfrstor %5\n\tfcomip %%st(1),%%st\n\tsetb %2\n\tsete %3\n\tsetp %4\n\tfnsave %1\n\tfxrstor %0"
#endif
            :"=m"(host),"=m"(result),"=qm"(carry),"=qm"(zero),"=qm"(parity):"m"(source)
            :"memory","cc","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        a.eflags=(a.eflags&~0x8d5u)|unsigned(carry)|(unsigned(parity)<<2u)|(unsigned(zero)<<6u);break;
    }
    }
#undef H2_EFFECT_FP_OP
#undef H2_EFFECT_FP_SAVE
#undef H2_EFFECT_FP_RESTORE
    const auto next=operation==EffectFp::integer || operation==EffectFp::single?depth+1:
        operation==EffectFp::compare || operation==EffectFp::pop || operation==EffectFp::multiply_pop?depth-1:depth;
    a.x87_depth=next;std::memcpy(&a.x87_status,result.data()+4,2);
    for(unsigned n=0;n<next;++n)std::memcpy(&a.x87[n],result.data()+28+n*10,10);
#else
    (void)a;(void)operation;(void)operand;
    throw std::runtime_error("Native effect x87 backend unavailable");
#endif
}
