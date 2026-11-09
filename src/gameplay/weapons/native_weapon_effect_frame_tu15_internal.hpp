std::uint32_t effect_frame_shift_left(IteratorRegisters &a,std::uint32_t value,unsigned count) {
    count&=31u;if(!count)return value;
    const auto result=value<<count;iterator_logic_flags(a,result,4);
    const auto carry=(value>>(32u-count))&1u;
    a.eflags=(a.eflags&~0x801u)|carry|((count==1u && ((result>>31u)^carry))?2048u:0u);
    return result;
}
std::uint32_t effect_frame_signed_right_five(IteratorRegisters &a,std::uint32_t value) {
    const auto result=std::uint32_t(std::bit_cast<std::int32_t>(value)>>5);
    iterator_logic_flags(a,result,4);a.eflags=(a.eflags&~0x801u)|((value>>4u)&1u);return result;
}
std::uint32_t effect_frame_increment(IteratorRegisters &a,std::uint32_t value) {
    const auto carry=a.eflags&1u;const auto result=iterator_flags(a,value,1u,4,false);
    a.eflags=(a.eflags&~1u)|carry;return result;
}
// TU1.5 179DF0: occupied effect slots, actual component chains, then the
// original177830 tail owner. This routine performs no FP operation. Every
// real lower call owns and returns its complete GP/FP/stack state.
void advance_effect_frame(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    IteratorRegisters r{a.gp,a.eflags};
    const auto compare=[&](auto x,auto y){iterator_flags(r,x,y,4,true);};
    const auto less=[&]{return bool(a.eflags&128u)!=bool(a.eflags&2048u);};
    const auto signed_word=[&](std::uint32_t at) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,at,2)))));
    };
    instruction(0x179df0);push(m,a,o,0x179df0,a.gp[3]);
    instruction(0x179df1);a.gp[3]=read(m,0x4f720c);
    instruction(0x179df7);push(m,a,o,0x179df7,a.gp[6]);
    instruction(0x179df8);a.gp[2]=0;iterator_logic_flags(r,0,4);
    instruction(0x179dfa);a.gp[1]=a.gp[3];
    call(m,a,o,s,0x179dfc,0x16ea60);
    instruction(0x179e01);a.gp[6]=none;iterator_logic_flags(r,none,4);
    instruction(0x179e04);compare(a.gp[0],none);
    instruction(0x179e07);
    if(!(a.eflags&64u)) {
        instruction(0x179e09);a.gp[1]=read(m,a.gp[3]+0x24);
        instruction(0x179e0c);a.gp[2]=read(m,a.gp[3]+0x44);
        instruction(0x179e0f);a.gp[1]=state_product(r,a.gp[1],a.gp[0]);
        instruction(0x179e12);a.gp[6]=signed_word(a.gp[1]+a.gp[2]);
        instruction(0x179e16);a.gp[6]=effect_frame_shift_left(r,a.gp[6],16);
        instruction(0x179e19);a.gp[6]|=a.gp[0];iterator_logic_flags(r,a.gp[6],4);
    }
    instruction(0x179e1b);compare(a.gp[6],none);
    instruction(0x179e1e);
    if(!(a.eflags&64u)) {
        instruction(0x179e24);push(m,a,o,0x179e24,a.gp[5]);
        instruction(0x179e25);push(m,a,o,0x179e25,a.gp[7]);
        instruction(0x179e26);
        do {
            instruction(0x179e30);a.gp[1]=read(m,a.gp[3]+0x44);
            instruction(0x179e33);a.gp[7]=a.gp[6];
            instruction(0x179e35);a.gp[7]&=65535u;iterator_logic_flags(r,a.gp[7],4);
            instruction(0x179e3b);a.gp[0]=a.gp[7];
            instruction(0x179e3d);a.gp[0]=state_product(r,a.gp[0],0x190);
            instruction(0x179e43);a.gp[0]=read(m,a.gp[0]+a.gp[1]+0xfc);
            instruction(0x179e4a);compare(a.gp[0],none);
            instruction(0x179e4d);
            if(!(a.eflags&64u)) {
                instruction(0x179e4f);
                do {
                    instruction(0x179e50);push(m,a,o,0x179e50,a.gp[6]);
                    call(m,a,o,s,0x179e51,0x179ed0);
                    instruction(0x179e56);compare(a.gp[0],none);
                    instruction(0x179e59);
                }while(!(a.eflags&64u));
                instruction(0x179e5b);a.gp[3]=read(m,0x4f720c);
            }
            instruction(0x179e61);compare(a.gp[6],none);
            instruction(0x179e64);
            if(a.eflags&64u) {
                instruction(0x179e66);a.gp[0]=0;iterator_logic_flags(r,0,4);
                instruction(0x179e68);
            }else {instruction(0x179e6a);a.gp[0]=a.gp[7]+1u;}
            instruction(0x179e6d);a.gp[6]=none;iterator_logic_flags(r,none,4);
            instruction(0x179e70);iterator_logic_flags(r,a.gp[0],4);
            instruction(0x179e72);
            if(!less()) {
                instruction(0x179e74);a.gp[7]=read(m,a.gp[3]+0x38);
                instruction(0x179e77);compare(a.gp[0],a.gp[7]);
                instruction(0x179e79);
                if(less()) {
                    instruction(0x179e7b);a.gp[2]=read(m,a.gp[3]+0x48);
                    instruction(0x179e7e);
                    for(;;) {
                        instruction(0x179e80);a.gp[1]=a.gp[0];
                        instruction(0x179e82);a.gp[1]&=31u;iterator_logic_flags(r,a.gp[1],4);
                        instruction(0x179e85);a.gp[5]=1;
                        instruction(0x179e8a);a.gp[5]=effect_frame_shift_left(r,a.gp[5],a.gp[1]&255u);
                        instruction(0x179e8c);a.gp[1]=a.gp[0];
                        instruction(0x179e8e);a.gp[1]=effect_frame_signed_right_five(r,a.gp[1]);
                        instruction(0x179e91);iterator_logic_flags(r,read(m,a.gp[2]+a.gp[1]*4u)&a.gp[5],4);
                        instruction(0x179e94);
                        if(!(a.eflags&64u)) {instruction(0x179e9d);a.gp[6]=a.gp[0];break;}
                        instruction(0x179e96);a.gp[0]=effect_frame_increment(r,a.gp[0]);
                        instruction(0x179e97);compare(a.gp[0],a.gp[7]);
                        instruction(0x179e99);
                        if(!less()){instruction(0x179e9b);break;}
                    }
                }
            }
            instruction(0x179e9f);a.gp[0]=none;iterator_logic_flags(r,none,4);
            instruction(0x179ea2);compare(a.gp[6],none);
            instruction(0x179ea5);
            if(!(a.eflags&64u)) {
                instruction(0x179ea7);a.gp[2]=read(m,a.gp[3]+0x24);
                instruction(0x179eaa);a.gp[0]=read(m,a.gp[3]+0x44);
                instruction(0x179ead);a.gp[2]=state_product(r,a.gp[2],a.gp[6]);
                instruction(0x179eb0);a.gp[0]=signed_word(a.gp[2]+a.gp[0]);
                instruction(0x179eb4);a.gp[0]=effect_frame_shift_left(r,a.gp[0],16);
                instruction(0x179eb7);a.gp[0]|=a.gp[6];iterator_logic_flags(r,a.gp[0],4);
            }
            instruction(0x179eb9);compare(a.gp[0],none);
            instruction(0x179ebc);a.gp[6]=a.gp[0];
            instruction(0x179ebe);
        }while(!(a.eflags&64u));
        instruction(0x179ec4);a.gp[7]=pop(m,a);
        instruction(0x179ec5);a.gp[5]=pop(m,a);
    }
    instruction(0x179ec6);a.gp[6]=pop(m,a);
    instruction(0x179ec7);a.gp[3]=pop(m,a);
    instruction(0x179ec8);
    if(!s.required_owner)throw std::runtime_error("Native effect frame requires its actual particle tail owner");
    s.required_owner(m,{0x179ec8,0x177830,true},a,o);
}

// 16EA60: first occupied slot at or after signed EDX. Keep the original
// descriptor/bitmap read order, all defined flags and faulting PUSH boundary.
void find_effect_slot(Memory &m,IteratorRegisters &a,const Observer &o,const Services &s) {
    if(halo2::native_effect_slot_scan_detail::try_native(m,a.gp,a.eflags,s,o))return;
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    const auto less=[&]{return bool(a.eflags&128u)!=bool(a.eflags&2048u);};
    instruction(0x16ea60);a.gp[0]=none;iterator_logic_flags(a,none,4);
    instruction(0x16ea63);iterator_logic_flags(a,a.gp[2],4);
    instruction(0x16ea65);
    if(!less()) {
        instruction(0x16ea67);push(m,a,o,0x16ea67,a.gp[7]);
        instruction(0x16ea68);a.gp[7]=read(m,a.gp[1]+0x38);
        instruction(0x16ea6b);iterator_flags(a,a.gp[2],a.gp[7],4,true);
        instruction(0x16ea6d);
        if(less()) {
            instruction(0x16ea6f);push(m,a,o,0x16ea6f,a.gp[6]);
            instruction(0x16ea70);a.gp[6]=read(m,a.gp[1]+0x48);
            instruction(0x16ea73);push(m,a,o,0x16ea73,a.gp[3]);
            for(;;) {
                instruction(0x16ea74);a.gp[1]=a.gp[2];
                instruction(0x16ea76);a.gp[1]&=31u;iterator_logic_flags(a,a.gp[1],4);
                instruction(0x16ea79);a.gp[3]=1;
                instruction(0x16ea7e);a.gp[3]=effect_frame_shift_left(a,a.gp[3],a.gp[1]&255u);
                instruction(0x16ea80);a.gp[1]=a.gp[2];
                instruction(0x16ea82);a.gp[1]=effect_frame_signed_right_five(a,a.gp[1]);
                instruction(0x16ea85);iterator_logic_flags(a,read(m,a.gp[6]+a.gp[1]*4u)&a.gp[3],4);
                instruction(0x16ea88);
                if(!(a.eflags&64u)) {
                    instruction(0x16ea93);a.gp[3]=pop(m,a);
                    instruction(0x16ea94);a.gp[0]=a.gp[2];
                    instruction(0x16ea96);a.gp[6]=pop(m,a);
                    break;
                }
                instruction(0x16ea8a);a.gp[2]=effect_frame_increment(a,a.gp[2]);
                instruction(0x16ea8b);iterator_flags(a,a.gp[2],a.gp[7],4,true);
                instruction(0x16ea8d);
                if(!less()) {
                    instruction(0x16ea8f);a.gp[3]=pop(m,a);
                    instruction(0x16ea90);a.gp[6]=pop(m,a);
                    instruction(0x16ea91);a.gp[7]=pop(m,a);
                    instruction(0x16ea92);(void)pop(m,a);return;
                }
            }
        }
        instruction(0x16ea97);a.gp[7]=pop(m,a);
    }
    instruction(0x16ea98);(void)pop(m,a);
}

// 177830 owns the occupied component walk and the actual update/release/free
// calls. Its saved pool pointer and low-byte return test retain original order.
void advance_particle_frame(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    IteratorRegisters r{a.gp,a.eflags};
    const auto compare=[&](auto x,auto y){iterator_flags(r,x,y,4,true);};
    const auto less=[&]{return bool(a.eflags&128u)!=bool(a.eflags&2048u);};
    instruction(0x177830);a.gp[4]=iterator_flags(r,a.gp[4],0x10u,4,true);
    instruction(0x177833);push(m,a,o,0x177833,a.gp[3]);
    instruction(0x177834);push(m,a,o,0x177834,a.gp[5]);
    instruction(0x177835);push(m,a,o,0x177835,a.gp[6]);
    instruction(0x177836);push(m,a,o,0x177836,a.gp[7]);
    instruction(0x177837);a.gp[7]=read(m,0x51df74);
    instruction(0x17783d);store(m,o,0x17783d,a.gp[4]+0x14,a.gp[7]);
    instruction(0x177841);a.gp[5]=none;iterator_logic_flags(r,none,4);
    for(;;) {
        instruction(0x177844);a.gp[0]=a.gp[5]+1u;
        instruction(0x177847);iterator_logic_flags(r,a.gp[0],4);
        instruction(0x177849);
        if(less())break;
        instruction(0x17784b);a.gp[6]=read(m,a.gp[7]+0x38);
        instruction(0x17784e);compare(a.gp[0],a.gp[6]);
        instruction(0x177850);
        if(!less())break;
        instruction(0x177852);a.gp[2]=read(m,a.gp[7]+0x48);
        bool found{};
        for(;;) {
            instruction(0x177855);a.gp[1]=a.gp[0];
            instruction(0x177857);a.gp[1]&=31u;iterator_logic_flags(r,a.gp[1],4);
            instruction(0x17785a);a.gp[3]=1;
            instruction(0x17785f);a.gp[3]=effect_frame_shift_left(r,a.gp[3],a.gp[1]&255u);
            instruction(0x177861);a.gp[1]=a.gp[0];
            instruction(0x177863);a.gp[1]=effect_frame_signed_right_five(r,a.gp[1]);
            instruction(0x177866);iterator_logic_flags(r,read(m,a.gp[2]+a.gp[1]*4u)&a.gp[3],4);
            instruction(0x177869);
            if(!(a.eflags&64u)) {found=true;break;}
            instruction(0x17786b);a.gp[0]=effect_frame_increment(r,a.gp[0]);
            instruction(0x17786c);compare(a.gp[0],a.gp[6]);
            instruction(0x17786e);
            if(!less())break;
        }
        if(!found)break;
        instruction(0x17787a);compare(a.gp[0],none);
        instruction(0x17787d);
        if(a.eflags&64u)break;
        instruction(0x17787f);a.gp[3]=read(m,a.gp[7]+0x24);
        instruction(0x177882);a.gp[6]=read(m,a.gp[7]+0x44);
        instruction(0x177885);a.gp[3]=state_product(r,a.gp[3],a.gp[0]);
        instruction(0x177888);a.gp[3]=iterator_flags(r,a.gp[3],a.gp[6],4,false);
        instruction(0x17788a);a.gp[6]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[3],2)))));
        instruction(0x17788d);a.gp[6]=effect_frame_shift_left(r,a.gp[6],16);
        instruction(0x177890);a.gp[6]|=a.gp[0];iterator_logic_flags(r,a.gp[6],4);
        instruction(0x177892);a.gp[5]=a.gp[0];
        instruction(0x177894);compare(read(m,a.gp[3]+0x4c),none);
        instruction(0x177898);
        if(!(a.eflags&64u))continue;
        instruction(0x17789a);a.gp[2]=read(m,a.gp[4]+0x24);
        instruction(0x17789e);push(m,a,o,0x17789e,a.gp[2]);
        instruction(0x17789f);a.gp[1]=a.gp[3];
        call(m,a,o,s,0x1778a1,0x1778d0);
        instruction(0x1778a6);iterator_logic_flags(r,a.gp[0]&255u,1);
        instruction(0x1778a8);
        if(!(a.eflags&64u))continue;
        instruction(0x1778aa);a.gp[7]=read(m,a.gp[3]+0x18);
        instruction(0x1778ad);compare(a.gp[7],none);
        instruction(0x1778b0);
        if(!(a.eflags&64u)) {
            call(m,a,o,s,0x1778b2,0x17de10);
            instruction(0x1778b7);store(m,o,0x1778b7,a.gp[3]+0x18,none);
        }
        instruction(0x1778be);push(m,a,o,0x1778be,a.gp[6]);
        call(m,a,o,s,0x1778bf,0x177020);
        instruction(0x1778c4);a.gp[7]=read(m,a.gp[4]+0x14);
        instruction(0x1778c8);
    }
    instruction(0x177870);a.gp[7]=pop(m,a);
    instruction(0x177871);a.gp[6]=pop(m,a);
    instruction(0x177872);a.gp[5]=pop(m,a);
    instruction(0x177873);a.gp[3]=pop(m,a);
    instruction(0x177874);a.gp[4]=iterator_flags(r,a.gp[4],0x10u,4,false);
    instruction(0x177877);(void)pop(m,a);a.gp[4]+=4;
}
