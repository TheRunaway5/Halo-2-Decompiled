// Included inside the native weapon dispatcher's private namespace.
// Complete TU1.5 1E1200 owner. Math remains with its actual native callees.
void marker_emission(Memory &m,Abi &a,const Observer &o,const Services &s) {
    bool sse_active{};std::optional<SseEnvironment> environment;
    const auto instruction=[&](auto pc){if(sse_active)a.floating_point->mxcsr=_mm_getcsr();if(s.instruction)s.instruction(pc);};
    IteratorRegisters r{a.gp,a.eflags};
    const auto signed_less=[&]{return bool(a.eflags&128u)!=bool(a.eflags&2048u);};
    const auto increment=[&](std::uint32_t value){const auto carry=a.eflags&1u;const auto result=iterator_flags(r,value,1,4,false);a.eflags=(a.eflags&~1u)|carry;return result;};
    instruction(0x1e1200);a.gp[4]=iterator_flags(r,a.gp[4],12,4,true);
    instruction(0x1e1203);push(m,a,o,0x1e1203,a.gp[3]);
    instruction(0x1e1204);a.gp[3]=0;iterator_logic_flags(r,0,4);
    instruction(0x1e1206);iterator_flags(r,a.gp[0],none,4,true);
    instruction(0x1e1209);if(a.eflags&64u)goto no_output;
    instruction(0x1e120f);a.gp[2]=read(m,0x4f0414);
    instruction(0x1e1215);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x1e121b);{const auto before=a.gp[1];a.gp[1]<<=4;iterator_logic_flags(r,a.gp[1],4);a.eflags=(a.eflags&~1u)|((before>>28u)&1u);}
    instruction(0x1e121e);a.gp[1]=read(m,a.gp[1]+a.gp[2]+8);
    instruction(0x1e1222);a.gp[2]=a.gp[0]*3u;
    instruction(0x1e1225);a.gp[0]=read(m,a.gp[1]+0x5c);
    instruction(0x1e1228);a.gp[1]=a.gp[0]+a.gp[2]*4u;
    instruction(0x1e122b);iterator_flags(r,read(m,a.gp[1]+4),a.gp[3],4,true);
    instruction(0x1e122e);store(m,o,0x1e122e,a.gp[4]+8,a.gp[1]);
    instruction(0x1e1232);store(m,o,0x1e1232,a.gp[4]+12,a.gp[3]);
    instruction(0x1e1236);if((a.eflags&64u)||signed_less())goto no_output;
    instruction(0x1e123c);push(m,a,o,0x1e123c,a.gp[5]);
    instruction(0x1e123d);a.gp[5]=read(m,a.gp[4]+0x28);
    instruction(0x1e1241);push(m,a,o,0x1e1241,a.gp[6]);
    instruction(0x1e1242);store(m,o,0x1e1242,a.gp[4]+12,a.gp[3]);
    instruction(0x1e1246);push(m,a,o,0x1e1246,a.gp[7]);
    instruction(0x1e1247);goto next_marker;
next_marker:
    instruction(0x1e1250);a.gp[7]=read(m,a.gp[1]+8);
    instruction(0x1e1253);a.gp[2]=read(m,a.gp[4]+16);
    instruction(0x1e1257);a.gp[6]=read(m,a.gp[4]+0x20);
    instruction(0x1e125b);a.gp[7]=iterator_flags(r,a.gp[7],a.gp[2],4,false);
    instruction(0x1e125d);iterator_logic_flags(r,a.gp[6],4);
    instruction(0x1e125f);if(a.eflags&64u)goto accepted;
    instruction(0x1e1261);a.gp[0]=(a.gp[0]&0xffffff00u)|read(m,a.gp[7],1);
    instruction(0x1e1263);iterator_flags(r,a.gp[0]&255u,255,1,true);
    instruction(0x1e1265);if(a.eflags&64u)goto accepted;
    instruction(0x1e1267);a.gp[2]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(a.gp[0]))));
    instruction(0x1e126a);a.gp[0]=(a.gp[0]&0xffffff00u)|read(m,a.gp[2]+a.gp[6],1);
    instruction(0x1e126d);iterator_flags(r,a.gp[0]&255u,read(m,a.gp[7]+1,1),1,true);
    instruction(0x1e1270);a.gp[0]=(a.gp[0]&0xffffff00u)|((a.eflags&64u)?1u:0u);
    instruction(0x1e1273);iterator_logic_flags(r,a.gp[0]&255u,1);
    instruction(0x1e1275);if(a.eflags&64u)goto advance;
accepted:
    instruction(0x1e127b);iterator_flags(r,a.gp[3],read(m,a.gp[4]+0x34),4,true);
    instruction(0x1e127f);if(!signed_less())goto finished;
    instruction(0x1e1285);a.gp[1]=read(m,a.gp[4]+0x24);
    instruction(0x1e1289);a.gp[0]=read(m,a.gp[7]+2,1);
    instruction(0x1e128d);a.gp[6]=a.gp[5];
    instruction(0x1e128f);a.gp[3]=increment(a.gp[3]);
    instruction(0x1e1290);a.gp[5]=iterator_flags(r,a.gp[5],0x70,4,false);
    instruction(0x1e1293);iterator_logic_flags(r,a.gp[1],4);
    instruction(0x1e1295);if(a.eflags&64u)goto node_ready;
    instruction(0x1e1297);a.gp[0]=read(m,a.gp[1]+a.gp[0]*4u);
node_ready:
    instruction(0x1e129a);store(m,o,0x1e129a,a.gp[6],a.gp[0]&65535u,2);
    instruction(0x1e129d);a.gp[2]=a.gp[6]+4;
    instruction(0x1e12a0);a.gp[1]=a.gp[7]+16;
    instruction(0x1e12a3);a.gp[0]=a.gp[2]+4;
    call(m,a,o,s,0x1e12a6,0x144910);
    instruction(0x1e12ab);a.xmm[0]={};
    instruction(0x1e12ae);store(m,o,0x1e12ae,a.gp[2]+0x28,a.xmm[0][0]);
    instruction(0x1e12b3);store(m,o,0x1e12b3,a.gp[2]+0x2c,a.xmm[0][0]);
    instruction(0x1e12b8);store(m,o,0x1e12b8,a.gp[2]+0x30,a.xmm[0][0]);
    instruction(0x1e12bd);scalar(m,a,0,0x46a0d0);
    instruction(0x1e12c5);store(m,o,0x1e12c5,a.gp[2],a.xmm[0][0]);
    instruction(0x1e12c9);a.gp[1]=a.gp[7]+4;
    instruction(0x1e12cc);a.gp[0]=read(m,a.gp[1]);
    instruction(0x1e12ce);store(m,o,0x1e12ce,a.gp[2]+0x28,a.gp[0]);
    instruction(0x1e12d1);a.gp[0]=read(m,a.gp[1]+4);
    instruction(0x1e12d4);store(m,o,0x1e12d4,a.gp[2]+0x2c,a.gp[0]);
    instruction(0x1e12d7);a.gp[1]=read(m,a.gp[1]+8);
    instruction(0x1e12da);store(m,o,0x1e12da,a.gp[2]+0x30,a.gp[1]);
    instruction(0x1e12dd);a.gp[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[6],2)))));
    instruction(0x1e12e0);{const auto product=std::int64_t(std::bit_cast<std::int32_t>(a.gp[1]))*52;a.gp[1]=std::uint32_t(product);const auto overflow=product!=std::int64_t(std::bit_cast<std::int32_t>(a.gp[1]));a.eflags=(a.eflags&~0x801u)|(overflow?0x801u:0u);}
    instruction(0x1e12e3);a.gp[0]=a.gp[6]+0x38;
    instruction(0x1e12e6);push(m,a,o,0x1e12e6,a.gp[0]);
    instruction(0x1e12e7);a.gp[1]=iterator_flags(r,a.gp[1],read(m,a.gp[4]+0x2c),4,false);
    call(m,a,o,s,0x1e12eb,0x145560);
    instruction(0x1e12f0);a.gp[1]=read(m,a.gp[7]+0x20);
    instruction(0x1e12f3);a.gp[0]=(a.gp[0]&0xffffff00u)|read(m,a.gp[4]+0x2c,1);
    instruction(0x1e12f7);iterator_logic_flags(r,a.gp[0]&255u,1);
    instruction(0x1e12f9);store(m,o,0x1e12f9,a.gp[6]+0x6c,a.gp[1]);
    instruction(0x1e12fc);a.gp[1]=read(m,a.gp[4]+0x14);
    instruction(0x1e1300);if(a.eflags&64u)goto advance;
    instruction(0x1e1302);a.xmm[0]={};
    instruction(0x1e1305);a.xmm[1]=a.xmm[0];
    instruction(0x1e1308);{const auto bits=read(m,a.gp[6]+0x48);environment.emplace(a);sse_active=true;vector(a,1,_mm_sub_ss(vector(a,1),_mm_castsi128_ps(_mm_cvtsi32_si128(bits))));}
    instruction(0x1e130d);store(m,o,0x1e130d,a.gp[6]+0x48,a.xmm[1][0]);
    instruction(0x1e1312);a.xmm[1]=a.xmm[0];
    instruction(0x1e1315);vector(a,1,_mm_sub_ss(vector(a,1),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[6]+0x4c)))));
    instruction(0x1e131a);store(m,o,0x1e131a,a.gp[6]+0x4c,a.xmm[1][0]);
    instruction(0x1e131f);vector(a,0,_mm_sub_ss(vector(a,0),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[6]+0x50)))));
    instruction(0x1e1324);store(m,o,0x1e1324,a.gp[6]+0x50,a.xmm[0][0]);
    environment.reset();sse_active=false;
advance:
    instruction(0x1e1329);a.gp[0]=read(m,a.gp[4]+0x18);
    instruction(0x1e132d);a.gp[6]=read(m,a.gp[4]+0x10);
    instruction(0x1e1331);a.gp[2]=read(m,a.gp[1]+4);
    instruction(0x1e1334);a.gp[0]=increment(a.gp[0]);
    instruction(0x1e1335);a.gp[6]=iterator_flags(r,a.gp[6],0x24,4,false);
    instruction(0x1e1338);iterator_flags(r,a.gp[0],a.gp[2],4,true);
    instruction(0x1e133a);store(m,o,0x1e133a,a.gp[4]+0x18,a.gp[0]);
    instruction(0x1e133e);store(m,o,0x1e133e,a.gp[4]+0x10,a.gp[6]);
    instruction(0x1e1342);if(signed_less())goto next_marker;
finished:
    instruction(0x1e1348);a.gp[7]=pop(m,a);
    instruction(0x1e1349);a.gp[6]=pop(m,a);
    instruction(0x1e134a);a.gp[5]=pop(m,a);
    instruction(0x1e134b);a.gp[0]=a.gp[3];
    instruction(0x1e134d);a.gp[3]=pop(m,a);
    instruction(0x1e134e);a.gp[4]=iterator_flags(r,a.gp[4],12,4,false);
    instruction(0x1e1351);(void)pop(m,a);a.gp[4]+=24;return;
no_output:
    instruction(0x1e1354);a.gp[0]=a.gp[3];
    instruction(0x1e1356);a.gp[3]=pop(m,a);
    instruction(0x1e1357);a.gp[4]=iterator_flags(r,a.gp[4],12,4,false);
    instruction(0x1e135a);(void)pop(m,a);a.gp[4]+=24;
}
