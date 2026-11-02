// Complete TU1.5 17C140. Included in the native weapon owner so its established
// register/flag, memory-store and FP operations have one implementation.
// Lower update/selection/retirement owners execute through their actual ABI.
void advance_effect_event(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    IteratorRegisters r{a.gp,a.eflags};
    const auto low=[&](unsigned reg,std::uint32_t value,unsigned width=1) {
        const auto mask=width==1?255u:65535u;
        a.gp[reg]=(a.gp[reg]&~mask)|(value&mask);
    };
    const auto shift_byte=[&](unsigned reg,unsigned bits) {
        const auto old=a.gp[reg]&255u,value=old>>bits;
        low(reg,value);iterator_logic_flags(r,value,1);
        a.eflags|=(old>>(bits-1u))&1u;
    };
    instruction(0x17c140);a.gp[0]=read(m,0x4f720c);
    instruction(0x17c145);a.gp[1]=read(m,a.gp[0]+0x44);
    instruction(0x17c148);a.gp[4]=iterator_flags(r,a.gp[4],12,4,true);
    instruction(0x17c14b);a.gp[2]=read(m,0x4f0414);
    instruction(0x17c151);push(m,a,o,0x17c151,a.gp[5]);
    instruction(0x17c152);push(m,a,o,0x17c152,a.gp[6]);
    instruction(0x17c153);push(m,a,o,0x17c153,a.gp[7]);
    instruction(0x17c154);a.gp[7]=read(m,a.gp[4]+0x1c);
    instruction(0x17c158);a.gp[6]=a.gp[7];
    instruction(0x17c15a);a.gp[6]&=65535u;iterator_logic_flags(r,a.gp[6],4);
    instruction(0x17c160);a.gp[6]=state_product(r,a.gp[6],400);
    instruction(0x17c166);low(0,read(m,a.gp[6]+a.gp[1]+2,1));
    instruction(0x17c16a);a.gp[6]=iterator_flags(r,a.gp[6],a.gp[1],4,false);
    instruction(0x17c16c);a.gp[1]=read(m,a.gp[6]+4);
    instruction(0x17c16f);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17c175);a.gp[1]<<=4;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17c178);a.gp[5]=read(m,a.gp[1]+a.gp[2]+8);
    instruction(0x17c17c);shift_byte(0,6);
    instruction(0x17c17f);iterator_logic_flags(r,a.gp[0]&1u,1);
    instruction(0x17c181);if(!(a.eflags&64u))goto live_return;
    instruction(0x17c183);scalar(m,a,0,a.gp[4]+0x20);
    instruction(0x17c189);a.xmm[2]={};
    instruction(0x17c18c);state_scalar_compare(a,vector(a,0),vector(a,2),false);
    instruction(0x17c18f);push(m,a,o,0x17c18f,a.gp[3]);
    instruction(0x17c190);store(m,o,0x17c190,a.gp[4]+0x10,0);
    instruction(0x17c198);if(!(a.eflags&1u))goto iteration;
clear_active:
    instruction(0x17c19a);and_effect_byte(m,a,o,0x17c19a,a.gp[6]+3,0xfd);
    instruction(0x17c19e);a.gp[3]=pop(m,a);
live_return:
    instruction(0x17c19f);a.gp[0]=0;iterator_logic_flags(r,0,4);
    instruction(0x17c1a1);a.gp[7]=pop(m,a);
    instruction(0x17c1a2);iterator_logic_flags(r,a.gp[6],4);
    instruction(0x17c1a4);a.gp[6]=pop(m,a);
    instruction(0x17c1a5);low(0,bool(a.eflags&64u));
    instruction(0x17c1a8);a.gp[5]=pop(m,a);
    instruction(0x17c1a9);a.gp[4]=iterator_flags(r,a.gp[4],12,4,false);
    instruction(0x17c1ac);(void)pop(m,a);a.gp[4]+=8;return;
reload_interval:
    instruction(0x17c1af);scalar(m,a,0,a.gp[4]+0x24);
iteration:
    instruction(0x17c1b5);low(0,read(m,a.gp[6]+2,2),2);
    instruction(0x17c1b9);low(1,a.gp[0]);
    instruction(0x17c1bb);shift_byte(1,2);
    instruction(0x17c1be);iterator_logic_flags(r,a.gp[1]&1u,1);
    instruction(0x17c1c1);if(!(a.eflags&64u))goto clear_active;
    instruction(0x17c1c3);iterator_flags(r,read(m,a.gp[4]+0x10),8,4,true);
    instruction(0x17c1c8);if(bool(a.eflags&128u)==bool(a.eflags&2048u))goto clear_active;
    instruction(0x17c1ca);scalar(m,a,1,a.gp[6]+0x64);
    instruction(0x17c1cf);{const auto elapsed=read(m,a.gp[6]+0x60);SseEnvironment environment(a);
        vector(a,1,_mm_sub_ss(vector(a,1),_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(elapsed)))));}
    instruction(0x17c1d4);state_scalar_compare(a,vector(a,1),vector(a,0),false);
    instruction(0x17c1d7);scalar(m,a,3,a.gp[6]+0x60);
    instruction(0x17c1dc);store(m,o,0x17c1dc,a.gp[4]+0x18,a.xmm[3][0]);
    instruction(0x17c1e2);store(m,o,0x17c1e2,a.gp[4]+0x14,a.xmm[0][0]);
    instruction(0x17c1e8);if(!(a.eflags&0x41u))goto selected_interval;
    instruction(0x17c1ea);store(m,o,0x17c1ea,a.gp[4]+0x14,a.xmm[1][0]);
selected_interval:
    instruction(0x17c1f0);state_scalar_compare(a,vector(a,0),vector(a,1),false);
    instruction(0x17c1f3);if(a.eflags&1u)goto partial_interval;
    instruction(0x17c1f5);a.gp[2]=read(m,a.gp[6]+0x64);
    instruction(0x17c1f8);low(3,1);
    instruction(0x17c1fa);store(m,o,0x17c1fa,a.gp[6]+0x60,a.gp[2]);
    instruction(0x17c1fd);{SseEnvironment environment(a);vector(a,0,_mm_sub_ss(vector(a,0),vector(a,1)));}
    instruction(0x17c201);goto dispatch_interval;
partial_interval:
    instruction(0x17c203);scalar(m,a,1,a.gp[6]+0x60);
    instruction(0x17c208);{SseEnvironment environment(a);vector(a,1,_mm_add_ss(vector(a,1),vector(a,0)));}
    instruction(0x17c20c);scalar(m,a,0,0x46a0dc);
    instruction(0x17c214);low(3,0);iterator_logic_flags(r,0,1);
    instruction(0x17c216);store(m,o,0x17c216,a.gp[6]+0x60,a.xmm[1][0]);
dispatch_interval:
    instruction(0x17c21b);iterator_logic_flags(r,read(m,a.gp[6]+2,1)&1u,1);
    instruction(0x17c21f);store(m,o,0x17c21f,a.gp[4]+0x24,a.xmm[0][0]);
    instruction(0x17c225);if(a.eflags&64u)goto begin_interval;
    instruction(0x17c227);shift_byte(0,3);
    instruction(0x17c22a);iterator_logic_flags(r,a.gp[0]&1u,1);
    instruction(0x17c22c);if(!(a.eflags&64u))goto after_particles;
    instruction(0x17c22e);a.gp[0]=read(m,a.gp[4]+0x14);
    instruction(0x17c232);a.gp[1]=read(m,a.gp[4]+0x18);
    instruction(0x17c236);push(m,a,o,0x17c236,a.gp[0]);
    instruction(0x17c237);push(m,a,o,0x17c237,a.gp[1]);
    instruction(0x17c238);push(m,a,o,0x17c238,a.gp[7]);
    call(m,a,o,s,0x17c239,0x17beb0);
after_particles:
    instruction(0x17c23e);low(2,read(m,a.gp[6]+2,1));
    instruction(0x17c241);shift_byte(2,3);
    instruction(0x17c244);iterator_logic_flags(r,a.gp[2]&1u,1);
    instruction(0x17c247);if(!(a.eflags&64u))goto next_interval;
    call(m,a,o,s,0x17c249,0x17ce40);
next_interval:
    instruction(0x17c24e);iterator_logic_flags(r,a.gp[3]&255u,1);
    instruction(0x17c250);if(a.eflags&64u)goto finish_iteration;
    instruction(0x17c256);push(m,a,o,0x17c256,a.gp[6]);
    call(m,a,o,s,0x17c257,0x17c020);
    instruction(0x17c25c);iterator_flags(r,a.gp[0],read(m,a.gp[5]+0x14),4,true);
    instruction(0x17c25f);if(bool(a.eflags&128u)==bool(a.eflags&2048u))goto retire;
    instruction(0x17c265);a.gp[2]=read(m,a.gp[4]+0x20);
    instruction(0x17c269);a.gp[7]=a.gp[0];
    call(m,a,o,s,0x17c26b,0x17b130);
    instruction(0x17c270);a.gp[7]=read(m,a.gp[4]+0x20);
    instruction(0x17c274);goto finish_iteration;
begin_interval:
    instruction(0x17c276);iterator_logic_flags(r,a.gp[3]&255u,1);
    instruction(0x17c278);if(a.eflags&64u)goto finish_iteration;
    instruction(0x17c27a);a.gp[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[6]+0x5c,2)))));
    instruction(0x17c27e);a.gp[0]=read(m,a.gp[5]+0x18);
    instruction(0x17c281);a.gp[1]=state_product(r,a.gp[1],0x38);
    instruction(0x17c284);{const auto value=read(m,a.gp[6]+2,1)|1u;iterator_logic_flags(r,value,1);store(m,o,0x17c284,a.gp[6]+2,value,1);}
    instruction(0x17c288);scalar(m,a,0,0x46a0dc);
    instruction(0x17c290);a.gp[1]=iterator_flags(r,a.gp[1],a.gp[0],4,false);
    instruction(0x17c292);store(m,o,0x17c292,a.gp[6]+0x60,a.xmm[2][0]);
    instruction(0x17c297);store(m,o,0x17c297,a.gp[6]+0x68,a.xmm[0][0]);
    instruction(0x17c29c);scalar(m,a,0,a.gp[1]+0x10);
    instruction(0x17c2a1);state_scalar_compare(a,vector(a,0),_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(m,a.gp[1]+0x14)))),true);
    instruction(0x17c2a5);a.gp[0]=(a.gp[0]&0xffff00ffu)|(((a.eflags&0xd5u)|2u)<<8u);
    instruction(0x17c2a6);iterator_logic_flags(r,(a.gp[0]>>8u)&0x44u,1);
    instruction(0x17c2a9);a.gp[0]=read(m,a.gp[1]+0x10);
    instruction(0x17c2ac);if(a.eflags&4u)goto random_duration;
    instruction(0x17c2ae);store(m,o,0x17c2ae,a.gp[6]+0x64,a.gp[0]);
    instruction(0x17c2b1);goto begin_parts;
random_duration:
    instruction(0x17c2b3);a.gp[2]=read(m,a.gp[1]+0x14);
    instruction(0x17c2b6);push(m,a,o,0x17c2b6,a.gp[2]);
    instruction(0x17c2b7);push(m,a,o,0x17c2b7,a.gp[0]);
    instruction(0x17c2b8);a.gp[0]=read(m,a.gp[6]+4);
    call(m,a,o,s,0x17c2bb,0x17aab0);
    instruction(0x17c2c0);a.gp[1]=a.gp[0];
    call(m,a,o,s,0x17c2c2,0x259b0);
    instruction(0x17c2c7);state_store_single(m,a,o,0x17c2c7);
begin_parts:
    instruction(0x17c2ca);low(1,read(m,a.gp[6]+2,1));
    instruction(0x17c2cd);shift_byte(1,3);
    instruction(0x17c2d0);iterator_logic_flags(r,a.gp[1]&1u,1);
    instruction(0x17c2d3);if(!(a.eflags&64u))goto sync_components;
    instruction(0x17c2d5);push(m,a,o,0x17c2d5,a.gp[6]);
    call(m,a,o,s,0x17c2d6,0x17d210);
sync_components:
    instruction(0x17c2db);a.gp[2]=a.gp[6];
    call(m,a,o,s,0x17c2dd,0x17dc60);
    instruction(0x17c2e2);low(2,read(m,a.gp[6]+0x5c,2),2);
    instruction(0x17c2e6);store(m,o,0x17c2e6,a.gp[6]+0x5e,a.gp[2],2);
finish_iteration:
    instruction(0x17c2ea);a.gp[0]=read(m,a.gp[4]+0x10);
    instruction(0x17c2ee);scalar(m,a,0,a.gp[4]+0x24);
    instruction(0x17c2f4);a.xmm[2]={};
    instruction(0x17c2f7);{const auto carry=a.eflags&1u;a.gp[0]=iterator_flags(r,a.gp[0],1,4,false);a.eflags=(a.eflags&~1u)|carry;}
    instruction(0x17c2f8);state_scalar_compare(a,vector(a,0),vector(a,2),false);
    instruction(0x17c2fb);store(m,o,0x17c2fb,a.gp[4]+0x10,a.gp[0]);
    instruction(0x17c2ff);if(!(a.eflags&1u))goto reload_interval;
    instruction(0x17c305);goto clear_active;
retire:
    instruction(0x17c30a);a.gp[0]=a.gp[7];
    call(m,a,o,s,0x17c30c,0x17a4a0);
    instruction(0x17c311);iterator_logic_flags(r,a.gp[0]&255u,1);
    instruction(0x17c313);if(a.eflags&64u)goto clear_active;
    instruction(0x17c319);a.gp[3]=pop(m,a);
    instruction(0x17c31a);a.gp[6]=0;iterator_logic_flags(r,0,4);
    instruction(0x17c31c);a.gp[0]=0;iterator_logic_flags(r,0,4);
    instruction(0x17c31e);a.gp[7]=pop(m,a);
    instruction(0x17c31f);iterator_logic_flags(r,a.gp[6],4);
    instruction(0x17c321);a.gp[6]=pop(m,a);
    instruction(0x17c322);low(0,bool(a.eflags&64u));
    instruction(0x17c325);a.gp[5]=pop(m,a);
    instruction(0x17c326);a.gp[4]=iterator_flags(r,a.gp[4],12,4,false);
    instruction(0x17c329);(void)pop(m,a);a.gp[4]+=8;
}
