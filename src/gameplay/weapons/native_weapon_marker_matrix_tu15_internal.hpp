// Included inside the native weapon dispatcher's private namespace.
// Marker owners retain real lower calls and exact original memory ordering.
void marker_matrix_locations(Memory &m,Abi &a,const Observer &o,const Services &s) {
    bool sse_active{};std::optional<SseEnvironment> environment;
    const auto instruction=[&](auto pc){if(sse_active)a.floating_point->mxcsr=_mm_getcsr();if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    const auto right=[&](std::uint32_t value,unsigned count){const auto result=value>>count;iterator_logic_flags(r,result,4);a.eflags=(a.eflags&~1u)|((value>>(count-1u))&1u);return result;};
    const auto signed_word=[](std::uint32_t value){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));};
    instruction(0xbad60);a.gp[4]=iterator_flags(r,a.gp[4],8,4,true);
    instruction(0xbad63);push(m,a,o,0xbad63,a.gp[5]);
    instruction(0xbad64);push(m,a,o,0xbad64,a.gp[6]);
    instruction(0xbad65);a.gp[6]=read(m,a.gp[4]+0x14);
    instruction(0xbad69);a.gp[5]=0;iterator_logic_flags(r,0,4);
    instruction(0xbad6b);iterator_logic_flags(r,a.gp[1]&255u,1);
    instruction(0xbad6d);push(m,a,o,0xbad6d,a.gp[7]);
    instruction(0xbad6e);a.gp[0]=a.gp[6];
    instruction(0xbad70);if(!(a.eflags&64u))goto parent_ready;
    instruction(0xbad72);push(m,a,o,0xbad72,a.gp[6]);
    call(m,a,o,s,0xbad73,0xbacd0);
parent_ready:
    instruction(0xbad78);iterator_flags(r,a.gp[0],none,4,true);
    instruction(0xbad7b);if(a.eflags&64u)goto fallback;
    instruction(0xbad81);a.gp[1]=read(m,0x4ecbd0);
    instruction(0xbad87);a.gp[2]=read(m,a.gp[1]+0x44);
    instruction(0xbad8a);a.gp[0]&=65535u;iterator_logic_flags(r,a.gp[0],4);
    instruction(0xbad8f);a.gp[0]*=3u;
    instruction(0xbad92);a.gp[0]=read(m,a.gp[2]+a.gp[0]*4u+8);
    instruction(0xbad96);a.gp[1]=read(m,a.gp[0]);
    instruction(0xbad98);a.gp[2]=read(m,0x4f0414);
    instruction(0xbad9e);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0xbada4);a.gp[1]<<=4;iterator_logic_flags(r,a.gp[1],4);
    instruction(0xbada7);a.gp[1]=read(m,a.gp[1]+a.gp[2]+8);
    instruction(0xbadab);a.gp[1]=read(m,a.gp[1]+0x38);
    instruction(0xbadae);iterator_flags(r,a.gp[1],none,4,true);
    instruction(0xbadb1);if(a.eflags&64u)goto fallback;
    instruction(0xbadb3);a.gp[6]=signed_word(read(m,a.gp[0]+0x11a,2));
    instruction(0xbadba);a.gp[5]=signed_word(read(m,a.gp[0]+0x116,2));
    instruction(0xbadc1);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0xbadc7);a.gp[1]<<=4;iterator_logic_flags(r,a.gp[1],4);
    instruction(0xbadca);a.gp[7]=read(m,a.gp[1]+a.gp[2]+8);
    instruction(0xbadce);a.gp[2]=read(m,a.gp[0]+4);
    instruction(0xbadd1);a.gp[1]=signed_word(read(m,a.gp[0]+0x114,2));
    instruction(0xbadd8);a.gp[2]=right(a.gp[2],10);
    instruction(0xbaddb);a.gp[2]=(a.gp[2]&0xffffff00u)|(a.gp[2]&1u);iterator_logic_flags(r,a.gp[2]&255u,1);
    instruction(0xbadde);a.gp[6]=iterator_flags(r,a.gp[6],a.gp[0],4,false);
    instruction(0xbade0);a.gp[5]=iterator_flags(r,a.gp[5],a.gp[0],4,false);
    instruction(0xbade2);store(m,o,0xbade2,a.gp[4]+0xc,a.gp[2]&255u,1);
    instruction(0xbade6);a.gp[0]=0x4ec4ec4fu;
    instruction(0xbadeb);{const auto product=std::uint64_t(a.gp[0])*a.gp[1];a.gp[0]=std::uint32_t(product);a.gp[2]=std::uint32_t(product>>32u);a.eflags=(a.eflags&~0x801u)|(a.gp[2]?0x801u:0u);}
    instruction(0xbaded);a.gp[1]=read(m,a.gp[7]+4);
    instruction(0xbadf0);a.gp[7]=read(m,a.gp[4]+0x1c);
    instruction(0xbadf4);a.gp[2]=right(a.gp[2],4);
    instruction(0xbadf7);store(m,o,0xbadf7,a.gp[4]+0x10,a.gp[2]);
    instruction(0xbadfb);store(m,o,0xbadfb,a.gp[4]+0x10,a.gp[1]);
    call(m,a,o,s,0xbadff,0x1e11b0);
    instruction(0xbae04);a.gp[2]=signed_word(read(m,a.gp[4]+0x20,2));
    instruction(0xbae09);a.gp[1]=read(m,a.gp[4]+0xc);
    instruction(0xbae0d);push(m,a,o,0xbae0d,a.gp[2]);
    instruction(0xbae0e);push(m,a,o,0xbae0e,a.gp[3]);
    instruction(0xbae0f);push(m,a,o,0xbae0f,a.gp[1]);
    instruction(0xbae10);a.gp[1]=read(m,a.gp[4]+0x1c);
    instruction(0xbae14);push(m,a,o,0xbae14,a.gp[5]);
    instruction(0xbae15);push(m,a,o,0xbae15,0);
    instruction(0xbae17);push(m,a,o,0xbae17,a.gp[6]);
    call(m,a,o,s,0xbae18,0x1e1200);
    instruction(0xbae1d);a.gp[5]=a.gp[0];
    instruction(0xbae1f);iterator_logic_flags(r,a.gp[5]&65535u,2);
    instruction(0xbae22);if(!(a.eflags&64u))goto result_ready;
    instruction(0xbae28);a.gp[6]=read(m,a.gp[4]+0x18);
fallback:
    instruction(0xbae2c);a.xmm[0]={};
    instruction(0xbae2f);a.gp[1]=read(m,0x4ecbd0);
    instruction(0xbae35);a.gp[2]=read(m,a.gp[1]+0x44);
    instruction(0xbae38);scalar(m,a,1,0x46a0d0);
    instruction(0xbae40);a.gp[6]&=65535u;iterator_logic_flags(r,a.gp[6],4);
    instruction(0xbae46);a.gp[0]=a.gp[6];
    instruction(0xbae48);a.gp[0]*=3u;
    instruction(0xbae4b);{const auto old=a.gp[0];a.gp[0]<<=2;iterator_logic_flags(r,a.gp[0],4);a.eflags=(a.eflags&~1u)|((old>>30u)&1u);}
    instruction(0xbae4e);a.gp[2]=read(m,a.gp[0]+a.gp[2]+8);
    instruction(0xbae52);store(m,o,0xbae52,a.gp[3],0,2);
    for(unsigned n=0;n<13;++n){const auto pc=0xbae57u+5u*n;instruction(pc);store(m,o,pc,a.gp[3]+4u+4u*n,a.xmm[(n==0 || n==1 || n==5 || n==9)?1:0][0]);}
    instruction(0xbae98);a.gp[1]=read(m,a.gp[1]+0x44);
    instruction(0xbae9b);a.gp[0]=read(m,a.gp[0]+a.gp[1]+8);
    instruction(0xbae9f);a.gp[6]=signed_word(read(m,a.gp[0]+0x116,2));
    instruction(0xbaea6);a.gp[6]=iterator_flags(r,a.gp[6],a.gp[0],4,false);
    instruction(0xbaea8);a.gp[7]=a.gp[3]+0x38;
    instruction(0xbaeab);a.gp[1]=13;
    do {instruction(0xbaeb0);const auto value=read(m,a.gp[6]);store(m,o,0xbaeb0,a.gp[7],value);const auto delta=(a.eflags&1024u)?std::uint32_t(-4):4u;a.gp[6]+=delta;a.gp[7]+=delta;--a.gp[1];}while(a.gp[1]);
    instruction(0xbaeb2);store(m,o,0xbaeb2,a.gp[3]+0x6c,a.xmm[0][0]);
    instruction(0xbaeb7);a.gp[2]=read(m,a.gp[2]+4);
    instruction(0xbaeba);a.gp[2]=right(a.gp[2],10);
    instruction(0xbaebd);iterator_logic_flags(r,a.gp[2]&1u,1);
    instruction(0xbaec0);if(a.eflags&64u)goto fallback_ready;
    instruction(0xbaec2);a.xmm[1]=a.xmm[0];
    instruction(0xbaec5);{const auto bits=read(m,a.gp[3]+0x48);environment.emplace(a);sse_active=true;vector(a,1,_mm_sub_ss(vector(a,1),_mm_castsi128_ps(_mm_cvtsi32_si128(bits))));}
    instruction(0xbaeca);store(m,o,0xbaeca,a.gp[3]+0x48,a.xmm[1][0]);
    instruction(0xbaecf);a.xmm[1]=a.xmm[0];
    instruction(0xbaed2);{const auto bits=read(m,a.gp[3]+0x4c);vector(a,1,_mm_sub_ss(vector(a,1),_mm_castsi128_ps(_mm_cvtsi32_si128(bits))));}
    instruction(0xbaed7);store(m,o,0xbaed7,a.gp[3]+0x4c,a.xmm[1][0]);
    instruction(0xbaedc);{const auto bits=read(m,a.gp[3]+0x50);vector(a,0,_mm_sub_ss(vector(a,0),_mm_castsi128_ps(_mm_cvtsi32_si128(bits))));}
    instruction(0xbaee1);store(m,o,0xbaee1,a.gp[3]+0x50,a.xmm[0][0]);
fallback_ready:
    instruction(0xbaee6);a.gp[0]=read(m,a.gp[4]+0x1c);
    instruction(0xbaeea);iterator_logic_flags(r,a.gp[0],4);
    instruction(0xbaeec);if(!(a.eflags&64u))goto result_ready;
    instruction(0xbaeee);a.gp[7]=pop(m,a);
    instruction(0xbaeef);a.gp[6]=pop(m,a);
    instruction(0xbaef0);a.gp[0]=1;
    instruction(0xbaef5);a.gp[5]=pop(m,a);
    instruction(0xbaef6);a.gp[4]=iterator_flags(r,a.gp[4],8,4,false);
    instruction(0xbaef9);(void)pop(m,a);a.gp[4]+=12;return;
result_ready:
    instruction(0xbaefc);a.gp[7]=pop(m,a);
    instruction(0xbaefd);a.gp[6]=pop(m,a);
    instruction(0xbaefe);a.gp[0]=(a.gp[0]&0xffff0000u)|(a.gp[5]&65535u);
    instruction(0xbaf01);a.gp[5]=pop(m,a);
    instruction(0xbaf02);a.gp[4]=iterator_flags(r,a.gp[4],8,4,false);
    instruction(0xbaf05);(void)pop(m,a);a.gp[4]+=12;
}

void marker_parent_unit(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    const auto shift_left=[&](std::uint32_t value,unsigned count){
        count&=31u;if(!count)return value;const auto result=value<<count;
        iterator_logic_flags(r,result,4);const auto carry=(value>>(32u-count))&1u;
        a.eflags=(a.eflags&~0x801u)|carry|((count==1u && ((result>>31u)^carry))?2048u:0u);return result;
    };
parent_again:
    instruction(0xbacd0);a.gp[1]=read(m,0x4ecbd0);
    instruction(0xbacd6);a.gp[2]=read(m,a.gp[1]+0x44);
    instruction(0xbacd9);push(m,a,o,0xbacd9,a.gp[6]);
    instruction(0xbacda);push(m,a,o,0xbacda,a.gp[7]);
    instruction(0xbacdb);a.gp[7]=read(m,a.gp[4]+0xc);
    instruction(0xbacdf);a.gp[0]=a.gp[7];
    instruction(0xbace1);a.gp[0]&=65535u;iterator_logic_flags(r,a.gp[0],4);
    instruction(0xbace6);a.gp[0]*=3u;
    instruction(0xbace9);a.gp[6]=read(m,a.gp[2]+a.gp[0]*4u+8);
    instruction(0xbaced);a.gp[0]=read(m,a.gp[6]+0x14);
    instruction(0xbacf0);iterator_flags(r,a.gp[0],none,4,true);
    instruction(0xbacf3);if(a.eflags&64u)goto parent_selected;
    instruction(0xbacf5);a.gp[1]=read(m,a.gp[6]);
    instruction(0xbacf7);a.gp[2]=read(m,0x4f0414);
    instruction(0xbacfd);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0xbad03);a.gp[1]=shift_left(a.gp[1],4);
    instruction(0xbad06);a.gp[1]=read(m,a.gp[1]+a.gp[2]+8);
    instruction(0xbad0a);a.gp[2]=(a.gp[2]&0xffffff00u)|read(m,a.gp[1]+2,1);
    instruction(0xbad0d);{const auto value=a.gp[2]&255u;const auto result=value>>4u;iterator_logic_flags(r,result,1);a.eflags=(a.eflags&~1u)|((value>>3u)&1u);a.gp[2]=(a.gp[2]&0xffffff00u)|result;}
    instruction(0xbad10);iterator_logic_flags(r,a.gp[2]&1u,1);
    instruction(0xbad13);if(a.eflags&64u)goto parent_selected;
    instruction(0xbad15);a.gp[7]=pop(m,a);
    instruction(0xbad16);a.gp[6]=pop(m,a);
    instruction(0xbad17);store(m,o,0xbad17,a.gp[4]+4,a.gp[0]);
    instruction(0xbad1b);goto parent_again;
parent_selected:
    instruction(0xbad20);iterator_logic_flags(r,read(m,a.gp[6]+4,1)&1u,1);
    instruction(0xbad24);if(a.eflags&64u)goto current_unit;
    instruction(0xbad26);a.gp[1]=(a.gp[1]&0xffffff00u)|read(m,a.gp[6]+0xaa,1);
    instruction(0xbad2c);a.gp[0]=1;
    instruction(0xbad31);a.gp[0]=shift_left(a.gp[0],a.gp[1]&255u);
    instruction(0xbad33);iterator_logic_flags(r,a.gp[0]&0x1cu,1);
    instruction(0xbad35);if(a.eflags&64u)goto current_unit;
    instruction(0xbad37);a.gp[1]=a.gp[7];
    call(m,a,o,s,0xbad39,0xfc510);
    instruction(0xbad3e);iterator_logic_flags(r,a.gp[0]&255u,1);
    instruction(0xbad40);if(a.eflags&64u)goto current_unit;
    instruction(0xbad42);a.gp[0]=read(m,a.gp[6]+0x154);
    instruction(0xbad48);a.gp[7]=pop(m,a);
    instruction(0xbad49);a.gp[6]=pop(m,a);
    instruction(0xbad4a);(void)pop(m,a);a.gp[4]+=4;return;
current_unit:
    instruction(0xbad4d);a.gp[0]=a.gp[7];
    instruction(0xbad4f);a.gp[7]=pop(m,a);
    instruction(0xbad50);a.gp[6]=pop(m,a);
    instruction(0xbad51);(void)pop(m,a);a.gp[4]+=4;
}

void marker_parent_mode(Memory &m,Abi &a,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0xfc510);a.gp[2]=read(m,0x4ecbd0);
    instruction(0xfc516);a.gp[2]=read(m,a.gp[2]+0x44);
    instruction(0xfc519);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0xfc51f);a.gp[1]*=3u;
    instruction(0xfc522);a.gp[1]=read(m,a.gp[2]+a.gp[1]*4u+8);
    instruction(0xfc526);a.gp[2]=(a.gp[2]&0xffffff00u)|read(m,a.gp[1]+0x12c,1);
    instruction(0xfc52c);a.gp[0]&=0xffffff00u;iterator_logic_flags(r,0,1);
    instruction(0xfc52e);iterator_logic_flags(r,a.gp[2]&1u,1);
    instruction(0xfc531);if(a.eflags&64u)goto mode_ready;
    instruction(0xfc533);a.gp[1]=(a.gp[1]&0xffffff00u)|(a.gp[2]&255u);
    instruction(0xfc535);{const auto value=a.gp[1]&255u;const auto result=value>>1u;iterator_logic_flags(r,result,1);a.eflags=(a.eflags&~0x801u)|(value&1u)|((value&128u)?2048u:0u);a.gp[1]=(a.gp[1]&0xffffff00u)|result;}
    instruction(0xfc537);iterator_logic_flags(r,a.gp[1]&1u,1);
    instruction(0xfc53a);if(!(a.eflags&64u))goto mode_ready;
    instruction(0xfc53c);a.gp[0]=(a.gp[0]&0xffffff00u)|1u;
mode_ready:
    instruction(0xfc53e);(void)pop(m,a);
}
