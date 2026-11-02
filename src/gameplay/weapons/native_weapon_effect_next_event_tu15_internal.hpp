// Complete TU1.5 17C020: choose the next event using the effect's actual
// primary/auxiliary RNG, including aliased operands and the live x87 file.
void next_effect_event(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    IteratorRegisters r{a.gp,a.eflags};
    const auto low=[&](unsigned reg,std::uint32_t value,unsigned width=1) {
        const auto mask=width==1?255u:65535u;a.gp[reg]=(a.gp[reg]&~mask)|(value&mask);
    };
    const auto shift=[&](unsigned reg,unsigned bits,unsigned width=4) {
        const auto old=width==1?a.gp[reg]&255u:a.gp[reg];const auto value=old>>bits;
        if(width==1)low(reg,value);else a.gp[reg]=value;
        iterator_logic_flags(r,value,width);a.eflags|=(old>>(bits-1u))&1u;
        if(bits==1)a.eflags|=((old>>(width*8u-1u))&1u)<<11u;
    };
    const auto step=[&](std::uint32_t value,bool down) {
        const auto carry=a.eflags&1u;const auto result=iterator_flags(r,value,1,4,down);
        a.eflags=(a.eflags&~1u)|carry;return result;
    };
    instruction(0x17c020);a.gp[4]=iterator_flags(r,a.gp[4],8,4,true);
    instruction(0x17c023);a.gp[1]=read(m,0x4f0414);
    instruction(0x17c029);push(m,a,o,0x17c029,a.gp[3]);
    instruction(0x17c02a);push(m,a,o,0x17c02a,a.gp[5]);
    instruction(0x17c02b);a.gp[5]=read(m,a.gp[4]+0x14);
    instruction(0x17c02f);a.gp[0]=read(m,a.gp[5]+4);
    instruction(0x17c032);a.gp[0]&=65535u;iterator_logic_flags(r,a.gp[0],4);
    instruction(0x17c037);{const auto old=a.gp[0];a.gp[0]<<=4;iterator_logic_flags(r,a.gp[0],4);a.eflags|=(old>>28u)&1u;}
    instruction(0x17c03a);a.gp[2]=read(m,a.gp[0]+a.gp[1]+8);
    instruction(0x17c03e);a.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[2]+4,2)))));
    instruction(0x17c042);a.gp[1]=0;iterator_logic_flags(r,0,4);
    instruction(0x17c044);low(1,read(m,a.gp[5]+2,2),2);
    instruction(0x17c048);store(m,o,0x17c048,a.gp[4]+0x14,a.gp[0]);
    instruction(0x17c04c);a.gp[0]=read(m,a.gp[2]+0x14);
    instruction(0x17c04f);a.gp[0]=step(a.gp[0],true);
    instruction(0x17c050);low(3,a.gp[1]);
    instruction(0x17c052);shift(3,1,1);
    instruction(0x17c054);iterator_logic_flags(r,a.gp[3]&1u,1);
    instruction(0x17c057);push(m,a,o,0x17c057,a.gp[6]);
    instruction(0x17c058);store(m,o,0x17c058,a.gp[4]+12,a.gp[0]);
    instruction(0x17c05c);a.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[5]+0x5c,2)))));
    instruction(0x17c060);push(m,a,o,0x17c060,a.gp[7]);
    instruction(0x17c061);if(!(a.eflags&64u))goto initial_loop;
    instruction(0x17c063);shift(1,9);
    instruction(0x17c066);iterator_logic_flags(r,a.gp[1]&1u,1);
    instruction(0x17c069);if(a.eflags&64u)goto begin;
initial_loop:
    instruction(0x17c06b);iterator_flags(r,read(m,a.gp[4]+0x1c),none,4,true);
    instruction(0x17c070);if(!(a.eflags&64u))goto begin;
    instruction(0x17c072);store(m,o,0x17c072,a.gp[4]+0x1c,0);
begin:
    instruction(0x17c07a);a.xmm[0]={};
    instruction(0x17c07d); // LEA ECX,[ECX] retains the incoming flags.
iteration:
    instruction(0x17c080);a.gp[1]=0;iterator_logic_flags(r,0,4);
    instruction(0x17c082);low(1,read(m,a.gp[5]+2,2),2);
    instruction(0x17c086);low(3,a.gp[1]);
    instruction(0x17c088);shift(3,1,1);
    instruction(0x17c08a);iterator_logic_flags(r,a.gp[3]&1u,1);
    instruction(0x17c08d);if(!(a.eflags&64u))goto loop;
    instruction(0x17c08f);shift(1,9);
    instruction(0x17c092);iterator_logic_flags(r,a.gp[1]&1u,1);
    instruction(0x17c095);if(a.eflags&64u)goto increment;
loop:
    instruction(0x17c097);iterator_flags(r,a.gp[0],read(m,a.gp[4]+0x10),4,true);
    instruction(0x17c09b);if(!(a.eflags&64u))goto increment;
    instruction(0x17c09d);a.gp[1]=read(m,a.gp[4]+0x1c);
    instruction(0x17c0a1);iterator_flags(r,a.gp[1],none,4,true);
    instruction(0x17c0a4);if(a.eflags&64u)goto increment;
    instruction(0x17c0a6);a.gp[0]=a.gp[1];
    instruction(0x17c0a8);goto check;
increment:
    instruction(0x17c0aa);a.gp[0]=step(a.gp[0],false);
check:
    instruction(0x17c0ab);iterator_flags(r,a.gp[0],read(m,a.gp[2]+0x14),4,true);
    instruction(0x17c0ae);if(bool(a.eflags&128u)==bool(a.eflags&2048u))goto finish;
    instruction(0x17c0b0);a.gp[1]=read(m,a.gp[2]+0x18);
    instruction(0x17c0b3);a.gp[7]=a.gp[0];
    instruction(0x17c0b5);a.gp[7]=state_product(r,a.gp[7],0x38);
    instruction(0x17c0b8);scalar(m,a,1,a.gp[1]+a.gp[7]+4);
    instruction(0x17c0be);state_scalar_compare(a,vector(a,1),vector(a,0),false);
    instruction(0x17c0c1);if(a.eflags&0x41u)goto finish;
    instruction(0x17c0c3);a.gp[1]=read(m,a.gp[5]+4);
    instruction(0x17c0c6);a.gp[6]=read(m,0x4f0414);
    instruction(0x17c0cc);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17c0d2);{const auto old=a.gp[1];a.gp[1]<<=4;iterator_logic_flags(r,a.gp[1],4);a.eflags|=(old>>28u)&1u;}
    instruction(0x17c0d5);a.gp[1]=read(m,a.gp[1]+a.gp[6]+8);
    instruction(0x17c0d9);a.gp[1]=read(m,a.gp[1]);
    instruction(0x17c0db);a.gp[6]=read(m,0x4f3cd8);
    instruction(0x17c0e1);shift(1,2);
    instruction(0x17c0e4);iterator_logic_flags(r,a.gp[1]&1u,1);
    instruction(0x17c0e7);if(!(a.eflags&64u))goto random;
    instruction(0x17c0e9);a.gp[6]=iterator_flags(r,a.gp[6],4,4,false);
random:
    instruction(0x17c0ec);a.gp[1]=read(m,a.gp[6]);
    instruction(0x17c0ee);a.gp[1]=state_product(r,a.gp[1],1664525);
    instruction(0x17c0f4);a.gp[1]=iterator_flags(r,a.gp[1],1013904223,4,false);
    instruction(0x17c0fa);store(m,o,0x17c0fa,a.gp[6],a.gp[1]);
    instruction(0x17c0fc);shift(1,16);
    instruction(0x17c0ff);iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17c101);store(m,o,0x17c101,a.gp[4]+0x14,a.gp[1]);
    instruction(0x17c105);effect_fp(a,EffectFp::integer,read(m,a.gp[4]+0x14));
    instruction(0x17c109);if(bool(a.eflags&128u)==bool(a.eflags&2048u))goto scale;
    instruction(0x17c10b);effect_fp(a,EffectFp::add,read(m,0x46a0c8));
scale:
    instruction(0x17c111);effect_fp(a,EffectFp::multiply,read(m,0x46a160));
    instruction(0x17c117);a.gp[1]=read(m,a.gp[2]+0x18);
    instruction(0x17c11a);effect_fp(a,EffectFp::single,read(m,a.gp[1]+a.gp[7]+4));
    instruction(0x17c11e);effect_fp(a,EffectFp::compare);
    instruction(0x17c120);effect_fp(a,EffectFp::pop);
    instruction(0x17c122);if(!(a.eflags&0x41u))goto iteration;
finish:
    instruction(0x17c128);a.gp[7]=pop(m,a);
    instruction(0x17c129);a.gp[6]=pop(m,a);
    instruction(0x17c12a);a.gp[5]=pop(m,a);
    instruction(0x17c12b);a.gp[3]=pop(m,a);
    instruction(0x17c12c);a.gp[4]=iterator_flags(r,a.gp[4],8,4,false);
    instruction(0x17c12f);(void)pop(m,a);a.gp[4]+=4;
}
