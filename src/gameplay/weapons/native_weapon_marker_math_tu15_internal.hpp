// Included inside the native weapon dispatcher's private namespace.
// Complete TU1.5 quaternion and transform-product owners. Each half-vector
// memory operation retains its one original eight-byte access.
void marker_load_high(const Memory &m,Abi &a,unsigned n,std::uint32_t at) {
    std::array<std::byte,8> bytes;m.read(at,bytes);
    std::memcpy(a.xmm[n].data()+2,bytes.data(),8);
}
void marker_store_high(Memory &m,const Observer &o,const Abi &a,
    std::uint32_t pc,std::uint32_t at,unsigned n) {
    SourcePhysicsContactRecordStore event{pc,at,{},std::vector<std::byte>(8)};
    if(o){event.before.resize(8);m.read(at,event.before);}
    std::memcpy(event.written.data(),a.xmm[n].data()+2,8);
    m.write(at,event.written);if(o)o(event);
}
void marker_quaternion_matrix(Memory &m,Abi &a,const Observer &o,const Services &s) {
    bool sse_active{};std::optional<SseEnvironment> environment;
    const auto instruction=[&](auto pc){if(sse_active)a.floating_point->mxcsr=_mm_getcsr();if(s.instruction)s.instruction(pc);};
    IteratorRegisters r{a.gp,a.eflags};
    instruction(0x144910);scalar(m,a,3,a.gp[1]+4);
    instruction(0x144915);scalar(m,a,2,a.gp[1]+8);
    instruction(0x14491a);scalar(m,a,0,a.gp[1]+12);
    instruction(0x14491f);scalar(m,a,1,a.gp[1]);
    instruction(0x144923);a.xmm[4]=a.xmm[3];
    instruction(0x144926);environment.emplace(a);sse_active=true;vector(a,4,_mm_mul_ss(vector(a,4),vector(a,3)));
    instruction(0x14492a);a.xmm[3]=a.xmm[2];
    instruction(0x14492d);vector(a,1,_mm_mul_ss(vector(a,1),vector(a,1)));
    instruction(0x144931);vector(a,3,_mm_mul_ss(vector(a,3),vector(a,2)));
    instruction(0x144935);vector(a,1,_mm_add_ss(vector(a,1),vector(a,4)));
    instruction(0x144939);a.xmm[2]=a.xmm[0];
    instruction(0x14493c);vector(a,1,_mm_add_ss(vector(a,1),vector(a,3)));
    instruction(0x144940);vector(a,2,_mm_mul_ss(vector(a,2),vector(a,0)));
    instruction(0x144944);a.gp[4]=iterator_flags(r,a.gp[4],12,4,true);
    instruction(0x144947);vector(a,1,_mm_add_ss(vector(a,1),vector(a,2)));
    instruction(0x14494b);{
        const auto right=_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,0x46a0ec)));const auto left=vector(a,1);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        unsigned char carry,parity,zero;
        asm volatile("comiss %4,%3; setb %0; setp %1; sete %2":"=qm"(carry),"=qm"(parity),"=qm"(zero):"x"(left),"x"(right):"cc");
        a.eflags=(a.eflags&~0x8d5u)|(carry?1u:0u)|(parity?4u:0u)|(zero?64u:0u);
#else
        throw std::runtime_error("Native marker quaternion COMISS backend unavailable");
#endif
    }
    instruction(0x144952);if(a.eflags&65u)goto zero_scale;
    instruction(0x144954);scalar(m,a,0,0x46a11c);
    instruction(0x14495c);vector(a,0,_mm_div_ss(vector(a,0),vector(a,1)));
    instruction(0x144960);goto scale_ready;
zero_scale:
    instruction(0x144962);a.xmm[0]={};
scale_ready:
    instruction(0x144965);scalar(m,a,1,a.gp[1]);
    instruction(0x144969);scalar(m,a,4,a.gp[1]+4);
    instruction(0x14496e);scalar(m,a,3,a.gp[1]+4);
    instruction(0x144973);scalar(m,a,7,0x46a0d0);
    instruction(0x14497b);vector(a,1,_mm_mul_ss(vector(a,1),vector(a,0)));
    instruction(0x14497f);vector(a,4,_mm_mul_ss(vector(a,4),vector(a,0)));
    instruction(0x144983);vector(a,0,_mm_mul_ss(vector(a,0),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[1]+8)))));
    instruction(0x144988);a.xmm[2]=a.xmm[0];
    instruction(0x14498b);vector(a,2,_mm_mul_ss(vector(a,2),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[1]+12)))));
    instruction(0x144990);a.xmm[5]=a.xmm[1];
    instruction(0x144993);vector(a,5,_mm_mul_ss(vector(a,5),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[1]+12)))));
    instruction(0x144998);store(m,o,0x144998,a.gp[4]+4,a.xmm[2][0]);
    instruction(0x14499e);scalar(m,a,2,a.gp[1]);
    instruction(0x1449a2);vector(a,2,_mm_mul_ss(vector(a,2),vector(a,1)));
    instruction(0x1449a6);scalar(m,a,1,a.gp[1]);
    instruction(0x1449aa);vector(a,1,_mm_mul_ss(vector(a,1),vector(a,4)));
    instruction(0x1449ae);store(m,o,0x1449ae,a.gp[4],a.xmm[2][0]);
    instruction(0x1449b3);scalar(m,a,2,a.gp[1]);
    instruction(0x1449b7);vector(a,2,_mm_mul_ss(vector(a,2),vector(a,0)));
    instruction(0x1449bb);vector(a,3,_mm_mul_ss(vector(a,3),vector(a,4)));
    instruction(0x1449bf);a.xmm[6]=a.xmm[4];
    instruction(0x1449c2);scalar(m,a,4,a.gp[1]+4);
    instruction(0x1449c7);vector(a,6,_mm_mul_ss(vector(a,6),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[1]+12)))));
    instruction(0x1449cc);vector(a,4,_mm_mul_ss(vector(a,4),vector(a,0)));
    instruction(0x1449d0);vector(a,0,_mm_mul_ss(vector(a,0),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[1]+8)))));
    instruction(0x1449d5);store(m,o,0x1449d5,a.gp[4]+8,a.xmm[0][0]);
    instruction(0x1449db);vector(a,0,_mm_add_ss(vector(a,0),vector(a,3)));
    instruction(0x1449df);vector(a,3,_mm_add_ss(vector(a,3),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[4])))));
    instruction(0x1449e4);vector(a,7,_mm_sub_ss(vector(a,7),vector(a,0)));
    instruction(0x1449e8);scalar(m,a,0,a.gp[4]+4);
    instruction(0x1449ee);store(m,o,0x1449ee,a.gp[0],a.xmm[7][0]);
    instruction(0x1449f2);a.xmm[7]=a.xmm[1];
    instruction(0x1449f5);vector(a,7,_mm_sub_ss(vector(a,7),vector(a,0)));
    instruction(0x1449f9);vector(a,1,_mm_add_ss(vector(a,1),vector(a,0)));
    instruction(0x1449fd);scalar(m,a,0,0x46a0d0);
    instruction(0x144a05);store(m,o,0x144a05,a.gp[0]+12,a.xmm[7][0]);
    instruction(0x144a0a);store(m,o,0x144a0a,a.gp[0]+4,a.xmm[1][0]);
    instruction(0x144a0f);scalar(m,a,1,a.gp[4]+8);
    instruction(0x144a15);vector(a,1,_mm_add_ss(vector(a,1),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[4])))));
    instruction(0x144a1a);a.xmm[7]=a.xmm[2];
    instruction(0x144a1d);vector(a,7,_mm_add_ss(vector(a,7),vector(a,6)));
    instruction(0x144a21);store(m,o,0x144a21,a.gp[0]+24,a.xmm[7][0]);
    instruction(0x144a26);a.xmm[7]=a.xmm[0];
    instruction(0x144a29);vector(a,7,_mm_sub_ss(vector(a,7),vector(a,1)));
    instruction(0x144a2d);a.xmm[1]=a.xmm[4];
    instruction(0x144a30);vector(a,1,_mm_sub_ss(vector(a,1),vector(a,5)));
    instruction(0x144a34);vector(a,2,_mm_sub_ss(vector(a,2),vector(a,6)));
    instruction(0x144a38);vector(a,4,_mm_add_ss(vector(a,4),vector(a,5)));
    instruction(0x144a3c);vector(a,0,_mm_sub_ss(vector(a,0),vector(a,3)));
    instruction(0x144a40);store(m,o,0x144a40,a.gp[0]+16,a.xmm[7][0]);
    instruction(0x144a45);store(m,o,0x144a45,a.gp[0]+28,a.xmm[1][0]);
    instruction(0x144a4a);store(m,o,0x144a4a,a.gp[0]+8,a.xmm[2][0]);
    instruction(0x144a4f);store(m,o,0x144a4f,a.gp[0]+20,a.xmm[4][0]);
    instruction(0x144a54);store(m,o,0x144a54,a.gp[0]+32,a.xmm[0][0]);
    instruction(0x144a59);a.gp[4]=iterator_flags(r,a.gp[4],12,4,false);
    instruction(0x144a5c);(void)pop(m,a);
}
void marker_transform_product(Memory &m,Abi &a,const Observer &o,const Services &s) {
    if(halo2::native_effect_transform_product_detail::try_native(m,a,s,o))return;
    bool sse_active{};std::optional<SseEnvironment> environment;
    const auto instruction=[&](auto pc){if(sse_active)a.floating_point->mxcsr=_mm_getcsr();if(s.instruction)s.instruction(pc);};
    instruction(0x145560);a.gp[0]=read(m,a.gp[4]+4);
    instruction(0x145564);scalar(m,a,7,a.gp[1]);
    instruction(0x145568);vector(a,7,_mm_shuffle_ps(vector(a,7),vector(a,7),0));
    instruction(0x14556c);scalar(m,a,3,a.gp[2]);
    instruction(0x145570);scalar(m,a,0,a.gp[2]+40);
    instruction(0x145575);scalar(m,a,1,a.gp[2]+44);
    instruction(0x14557a);scalar(m,a,2,a.gp[2]+48);
    instruction(0x14557f);vector(a,0,_mm_shuffle_ps(vector(a,0),vector(a,0),0));
    instruction(0x145583);vector(a,1,_mm_shuffle_ps(vector(a,1),vector(a,1),0));
    instruction(0x145587);vector(a,2,_mm_shuffle_ps(vector(a,2),vector(a,2),0));
    instruction(0x14558b);environment.emplace(a);sse_active=true;vector(a,3,halo2::native_tu15_leaves::ordered_mul_ss(vector(a,3),vector(a,7)));
    instruction(0x14558f);scalar(m,a,4,a.gp[1]+4);
    instruction(0x145594);marker_load_high(m,a,4,a.gp[1]+8);
    instruction(0x145598);scalar(m,a,5,a.gp[1]+16);
    instruction(0x14559d);marker_load_high(m,a,5,a.gp[1]+20);
    instruction(0x1455a1);scalar(m,a,6,a.gp[1]+28);
    instruction(0x1455a6);marker_load_high(m,a,6,a.gp[1]+32);
    instruction(0x1455aa);vector(a,0,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,0),vector(a,4)));
    instruction(0x1455ad);vector(a,1,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,1),vector(a,5)));
    instruction(0x1455b0);vector(a,2,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,2),vector(a,6)));
    instruction(0x1455b3);vector(a,0,halo2::native_tu15_leaves::ordered_add_ps(vector(a,0),vector(a,1)));
    instruction(0x1455b6);vector(a,0,halo2::native_tu15_leaves::ordered_add_ps(vector(a,0),vector(a,2)));
    instruction(0x1455b9);vector(a,0,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,0),vector(a,7)));
    instruction(0x1455bc);scalar(m,a,1,a.gp[1]+40);
    instruction(0x1455c1);marker_load_high(m,a,1,a.gp[1]+44);
    instruction(0x1455c5);vector(a,0,halo2::native_tu15_leaves::ordered_add_ps(vector(a,0),vector(a,1)));
    instruction(0x1455c8);scalar(m,a,2,a.gp[2]+36);
    instruction(0x1455cd);store(m,o,0x1455cd,a.gp[0]+40,a.xmm[0][0]);
    instruction(0x1455d2);marker_store_high(m,o,a,0x1455d2,a.gp[0]+44,0);
    instruction(0x1455d6);scalar(m,a,1,a.gp[2]+32);
    instruction(0x1455db);scalar(m,a,0,a.gp[2]+28);
    instruction(0x1455e0);vector(a,2,_mm_shuffle_ps(vector(a,2),vector(a,2),0));
    instruction(0x1455e4);vector(a,1,_mm_shuffle_ps(vector(a,1),vector(a,1),0));
    instruction(0x1455e8);vector(a,0,_mm_shuffle_ps(vector(a,0),vector(a,0),0));
    instruction(0x1455ec);vector(a,2,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,2),vector(a,6)));
    instruction(0x1455ef);vector(a,1,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,1),vector(a,5)));
    instruction(0x1455f2);vector(a,0,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,0),vector(a,4)));
    instruction(0x1455f5);vector(a,1,halo2::native_tu15_leaves::ordered_add_ps(vector(a,1),vector(a,2)));
    instruction(0x1455f8);vector(a,0,halo2::native_tu15_leaves::ordered_add_ps(vector(a,0),vector(a,1)));
    instruction(0x1455fb);scalar(m,a,2,a.gp[2]+24);
    instruction(0x145600);store(m,o,0x145600,a.gp[0]+28,a.xmm[0][0]);
    instruction(0x145605);marker_store_high(m,o,a,0x145605,a.gp[0]+32,0);
    instruction(0x145609);scalar(m,a,1,a.gp[2]+20);
    instruction(0x14560e);scalar(m,a,0,a.gp[2]+16);
    instruction(0x145613);vector(a,2,_mm_shuffle_ps(vector(a,2),vector(a,2),0));
    instruction(0x145617);vector(a,1,_mm_shuffle_ps(vector(a,1),vector(a,1),0));
    instruction(0x14561b);vector(a,0,_mm_shuffle_ps(vector(a,0),vector(a,0),0));
    instruction(0x14561f);vector(a,2,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,2),vector(a,6)));
    instruction(0x145622);vector(a,1,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,1),vector(a,5)));
    instruction(0x145625);vector(a,0,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,0),vector(a,4)));
    instruction(0x145628);vector(a,1,halo2::native_tu15_leaves::ordered_add_ps(vector(a,1),vector(a,2)));
    instruction(0x14562b);vector(a,0,halo2::native_tu15_leaves::ordered_add_ps(vector(a,0),vector(a,1)));
    instruction(0x14562e);scalar(m,a,2,a.gp[2]+12);
    instruction(0x145633);store(m,o,0x145633,a.gp[0]+16,a.xmm[0][0]);
    instruction(0x145638);marker_store_high(m,o,a,0x145638,a.gp[0]+20,0);
    instruction(0x14563c);scalar(m,a,1,a.gp[2]+8);
    instruction(0x145641);scalar(m,a,0,a.gp[2]+4);
    instruction(0x145646);vector(a,2,_mm_shuffle_ps(vector(a,2),vector(a,2),0));
    instruction(0x14564a);vector(a,1,_mm_shuffle_ps(vector(a,1),vector(a,1),0));
    instruction(0x14564e);vector(a,0,_mm_shuffle_ps(vector(a,0),vector(a,0),0));
    instruction(0x145652);vector(a,2,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,2),vector(a,6)));
    instruction(0x145655);vector(a,1,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,1),vector(a,5)));
    instruction(0x145658);vector(a,0,halo2::native_tu15_leaves::ordered_mul_ps(vector(a,0),vector(a,4)));
    instruction(0x14565b);vector(a,1,halo2::native_tu15_leaves::ordered_add_ps(vector(a,1),vector(a,2)));
    instruction(0x14565e);vector(a,0,halo2::native_tu15_leaves::ordered_add_ps(vector(a,0),vector(a,1)));
    instruction(0x145661);store(m,o,0x145661,a.gp[0]+4,a.xmm[0][0]);
    instruction(0x145666);marker_store_high(m,o,a,0x145666,a.gp[0]+8,0);
    instruction(0x14566a);store(m,o,0x14566a,a.gp[0],a.xmm[3][0]);
    instruction(0x14566e);(void)pop(m,a);a.gp[4]+=4;
}
