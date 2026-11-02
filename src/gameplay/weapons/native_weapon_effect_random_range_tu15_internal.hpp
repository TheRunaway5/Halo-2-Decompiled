// Complete TU1.5 259B0. ECX supplies the actual RNG word; the two original
// stack words supply min/max. Its scratch PUSH is consumed as volatile ECX.
// Read the arguments after the RNG store: legitimate operand aliases remain.
void effect_random_range(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    IteratorRegisters r{a.gp,a.eflags};
    instruction(0x259b0);push(m,a,o,0x259b0,a.gp[1]);
    instruction(0x259b1);a.gp[0]=read(m,a.gp[1]);
    instruction(0x259b3);a.gp[0]=state_product(r,a.gp[0],1664525);
    instruction(0x259b9);a.gp[0]=iterator_flags(r,a.gp[0],1013904223,4,false);
    instruction(0x259be);store(m,o,0x259be,a.gp[1],a.gp[0]);
    instruction(0x259c0);{const auto old=a.gp[0];a.gp[0]>>=16;iterator_logic_flags(r,a.gp[0],4);a.eflags|=(old>>15u)&1u;}
    instruction(0x259c3);iterator_logic_flags(r,a.gp[0],4);
    instruction(0x259c5);store(m,o,0x259c5,a.gp[4],a.gp[0]);
    instruction(0x259c8);effect_fp(a,EffectFp::integer,read(m,a.gp[4]));
    instruction(0x259cb);if(bool(a.eflags&128u)==bool(a.eflags&2048u))goto scale;
    instruction(0x259cd);effect_fp(a,EffectFp::add,read(m,0x46a0c8));
scale:
    instruction(0x259d3);effect_fp(a,EffectFp::multiply,read(m,0x46a160));
    instruction(0x259d9);effect_fp(a,EffectFp::single,read(m,a.gp[4]+12));
    instruction(0x259dd);effect_fp(a,EffectFp::subtract,read(m,a.gp[4]+8));
    instruction(0x259e1);effect_fp(a,EffectFp::multiply_pop);
    instruction(0x259e3);effect_fp(a,EffectFp::add,read(m,a.gp[4]+8));
    instruction(0x259e7);a.gp[1]=pop(m,a);
    instruction(0x259e8);(void)pop(m,a);a.gp[4]+=8;
}
