// Included inside the native weapon dispatcher's private namespace.
// Complete TU1.5 1E11B0 owner; model marker records have a 12-byte stride.
void marker_lookup(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    IteratorRegisters r{a.gp,a.eflags};
    const auto signed_less=[&]{return bool(a.eflags&128u)!=bool(a.eflags&2048u);};
    instruction(0x1e11b0);a.gp[0]=none;iterator_logic_flags(r,a.gp[0],4);
    instruction(0x1e11b3);iterator_flags(r,a.gp[1],none,4,true);
    instruction(0x1e11b6);if(a.eflags&64u)goto early_return;
    instruction(0x1e11b8);iterator_flags(r,a.gp[7],none,4,true);
    instruction(0x1e11bb);if(a.eflags&64u)goto early_return;
    instruction(0x1e11bd);iterator_logic_flags(r,a.gp[7],4);
    instruction(0x1e11bf);if(a.eflags&64u)goto early_return;
    instruction(0x1e11c1);a.gp[2]=read(m,0x4f0414);
    instruction(0x1e11c7);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x1e11cd);{const auto before=a.gp[1];a.gp[1]<<=4;iterator_logic_flags(r,a.gp[1],4);a.eflags=(a.eflags&~1u)|((before>>28u)&1u);}
    instruction(0x1e11d0);a.gp[1]=read(m,a.gp[1]+a.gp[2]+8);
    instruction(0x1e11d4);push(m,a,o,0x1e11d4,a.gp[6]);
    instruction(0x1e11d5);a.gp[6]=read(m,a.gp[1]+0x58);
    instruction(0x1e11d8);a.gp[2]=0;iterator_logic_flags(r,0,4);
    instruction(0x1e11da);iterator_logic_flags(r,a.gp[6],4);
    instruction(0x1e11dc);if((a.eflags&64u)||signed_less())goto pop_return;
    instruction(0x1e11de);a.gp[1]=read(m,a.gp[1]+0x5c);
next_marker:
    instruction(0x1e11e1);iterator_flags(r,read(m,a.gp[1]),a.gp[7],4,true);
    instruction(0x1e11e3);if(a.eflags&64u)goto matched;
    instruction(0x1e11e5);{const auto carry=a.eflags&1u;a.gp[2]=iterator_flags(r,a.gp[2],1,4,false);a.eflags=(a.eflags&~1u)|carry;}
    instruction(0x1e11e6);a.gp[1]=iterator_flags(r,a.gp[1],12,4,false);
    instruction(0x1e11e9);iterator_flags(r,a.gp[2],a.gp[6],4,true);
    instruction(0x1e11eb);if(signed_less())goto next_marker;
    instruction(0x1e11ed);a.gp[6]=pop(m,a);
    instruction(0x1e11ee);(void)pop(m,a);return;
matched:
    instruction(0x1e11ef);a.gp[0]=a.gp[2];
pop_return:
    instruction(0x1e11f1);a.gp[6]=pop(m,a);
early_return:
    instruction(0x1e11f2);(void)pop(m,a);
}
