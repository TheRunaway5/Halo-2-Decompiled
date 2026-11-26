// Complete TU1.5 17C140. Included in the native weapon owner so its established
// register/flag, memory-store and FP operations have one implementation.
// Lower update/selection/retirement owners execute through their actual ABI.
#include "native_weapon_effect_event_frame_tu15_internal.hpp"

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

    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_WEAPON_EFFECT_EVENT_ADVANCE");
        return value && *value;
    }();
    if (!reference && (!s.instruction || s.immutable_instruction_cursor.identifies(s)) &&
        native_model_marker_lookup_detail::protocol(s, o)) {
        try {
            effect_event_advance::Frame(m, a, o, s).run();
            return;
        } catch (effect_event_advance::Resume next) {
            switch (next.pc) {
            case 0x17c145u: goto resume_17c145;
            case 0x17c148u: goto resume_17c148;
            case 0x17c151u: goto resume_17c151;
            case 0x17c152u: goto resume_17c152;
            case 0x17c153u: goto resume_17c153;
            case 0x17c154u: goto resume_17c154;
            case 0x17c158u: goto resume_17c158;
            case 0x17c16au: goto resume_17c16a;
            case 0x17c16fu: goto resume_17c16f;
            case 0x17c17cu: goto resume_17c17c;
            case 0x17c189u: goto resume_17c189;
            case 0x17c190u: goto resume_17c190;
            case 0x17c198u: goto resume_17c198;
            case 0x17c19eu: goto resume_17c19e;
            case 0x17c19fu: goto resume_17c19f;
            case 0x17c1a2u: goto resume_17c1a2;
            case 0x17c1a5u: goto resume_17c1a5;
            case 0x17c1a9u: goto resume_17c1a9;
            case 0x17c1b5u: goto resume_17c1b5;
            case 0x17c1b9u: goto resume_17c1b9;
            case 0x17c1c8u: goto resume_17c1c8;
            case 0x17c1cfu: goto resume_17c1cf;
            case 0x17c1d4u: goto resume_17c1d4;
            case 0x17c1dcu: goto resume_17c1dc;
            case 0x17c1e2u: goto resume_17c1e2;
            case 0x17c1e8u: goto resume_17c1e8;
            case 0x17c1f0u: goto resume_17c1f0;
            case 0x17c1f8u: goto resume_17c1f8;
            case 0x17c1fdu: goto resume_17c1fd;
            case 0x17c208u: goto resume_17c208;
            case 0x17c214u: goto resume_17c214;
            case 0x17c21bu: goto resume_17c21b;
            case 0x17c21fu: goto resume_17c21f;
            case 0x17c225u: goto resume_17c225;
            case 0x17c232u: goto resume_17c232;
            case 0x17c236u: goto resume_17c236;
            case 0x17c237u: goto resume_17c237;
            case 0x17c238u: goto resume_17c238;
            case 0x17c239u: goto resume_17c239;
            case 0x17c23eu: goto resume_17c23e;
            case 0x17c241u: goto resume_17c241;
            case 0x17c24eu: goto resume_17c24e;
            case 0x17c257u: goto resume_17c257;
            case 0x17c25cu: goto resume_17c25c;
            case 0x17c25fu: goto resume_17c25f;
            case 0x17c269u: goto resume_17c269;
            case 0x17c270u: goto resume_17c270;
            case 0x17c274u: goto resume_17c274;
            case 0x17c27eu: goto resume_17c27e;
            case 0x17c281u: goto resume_17c281;
            case 0x17c288u: goto resume_17c288;
            case 0x17c290u: goto resume_17c290;
            case 0x17c297u: goto resume_17c297;
            case 0x17c29cu: goto resume_17c29c;
            case 0x17c2a1u: goto resume_17c2a1;
            case 0x17c2a5u: goto resume_17c2a5;
            case 0x17c2acu: goto resume_17c2ac;
            case 0x17c2b1u: goto resume_17c2b1;
            case 0x17c2b6u: goto resume_17c2b6;
            case 0x17c2b7u: goto resume_17c2b7;
            case 0x17c2b8u: goto resume_17c2b8;
            case 0x17c2bbu: goto resume_17c2bb;
            case 0x17c2c0u: goto resume_17c2c0;
            case 0x17c2c7u: goto resume_17c2c7;
            case 0x17c2cau: goto resume_17c2ca;
            case 0x17c2cdu: goto resume_17c2cd;
            case 0x17c2d6u: goto resume_17c2d6;
            case 0x17c2dbu: goto resume_17c2db;
            case 0x17c2e2u: goto resume_17c2e2;
            case 0x17c2e6u: goto resume_17c2e6;
            case 0x17c2eau: goto resume_17c2ea;
            case 0x17c2eeu: goto resume_17c2ee;
            case 0x17c2f4u: goto resume_17c2f4;
            case 0x17c2ffu: goto resume_17c2ff;
            case 0x17c311u: goto resume_17c311;
            case 0x17c31au: goto resume_17c31a;
            case 0x17c31fu: goto resume_17c31f;
            case 0x17c322u: goto resume_17c322;
            case 0x17c326u: goto resume_17c326;
            }
            throw;
        }
    }
    instruction(0x17c140);a.gp[0]=read(m,0x4f720c);
resume_17c145:
    instruction(0x17c145);a.gp[1]=read(m,a.gp[0]+0x44);
resume_17c148:
    instruction(0x17c148);a.gp[4]=iterator_flags(r,a.gp[4],12,4,true);
    instruction(0x17c14b);a.gp[2]=read(m,0x4f0414);
resume_17c151:
    instruction(0x17c151);push(m,a,o,0x17c151,a.gp[5]);
resume_17c152:
    instruction(0x17c152);push(m,a,o,0x17c152,a.gp[6]);
resume_17c153:
    instruction(0x17c153);push(m,a,o,0x17c153,a.gp[7]);
resume_17c154:
    instruction(0x17c154);a.gp[7]=read(m,a.gp[4]+0x1c);
resume_17c158:
    instruction(0x17c158);a.gp[6]=a.gp[7];
    instruction(0x17c15a);a.gp[6]&=65535u;iterator_logic_flags(r,a.gp[6],4);
    instruction(0x17c160);a.gp[6]=state_product(r,a.gp[6],400);
    instruction(0x17c166);low(0,read(m,a.gp[6]+a.gp[1]+2,1));
resume_17c16a:
    instruction(0x17c16a);a.gp[6]=iterator_flags(r,a.gp[6],a.gp[1],4,false);
    instruction(0x17c16c);a.gp[1]=read(m,a.gp[6]+4);
resume_17c16f:
    instruction(0x17c16f);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17c175);a.gp[1]<<=4;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17c178);a.gp[5]=read(m,a.gp[1]+a.gp[2]+8);
resume_17c17c:
    instruction(0x17c17c);shift_byte(0,6);
    instruction(0x17c17f);iterator_logic_flags(r,a.gp[0]&1u,1);
    instruction(0x17c181);if(!(a.eflags&64u))goto live_return;
    instruction(0x17c183);scalar(m,a,0,a.gp[4]+0x20);
resume_17c189:
    instruction(0x17c189);a.xmm[2]={};
    instruction(0x17c18c);state_scalar_compare(a,vector(a,0),vector(a,2),false);
    instruction(0x17c18f);push(m,a,o,0x17c18f,a.gp[3]);
resume_17c190:
    instruction(0x17c190);store(m,o,0x17c190,a.gp[4]+0x10,0);
resume_17c198:
    instruction(0x17c198);if(!(a.eflags&1u))goto iteration;
clear_active:
    instruction(0x17c19a);and_effect_byte(m,a,o,0x17c19a,a.gp[6]+3,0xfd);
resume_17c19e:
    instruction(0x17c19e);a.gp[3]=pop(m,a);
live_return:
resume_17c19f:
    instruction(0x17c19f);a.gp[0]=0;iterator_logic_flags(r,0,4);
    instruction(0x17c1a1);a.gp[7]=pop(m,a);
resume_17c1a2:
    instruction(0x17c1a2);iterator_logic_flags(r,a.gp[6],4);
    instruction(0x17c1a4);a.gp[6]=pop(m,a);
resume_17c1a5:
    instruction(0x17c1a5);low(0,bool(a.eflags&64u));
    instruction(0x17c1a8);a.gp[5]=pop(m,a);
resume_17c1a9:
    instruction(0x17c1a9);a.gp[4]=iterator_flags(r,a.gp[4],12,4,false);
    instruction(0x17c1ac);(void)pop(m,a);a.gp[4]+=8;return;
reload_interval:
    instruction(0x17c1af);scalar(m,a,0,a.gp[4]+0x24);
iteration:
resume_17c1b5:
    instruction(0x17c1b5);low(0,read(m,a.gp[6]+2,2),2);
resume_17c1b9:
    instruction(0x17c1b9);low(1,a.gp[0]);
    instruction(0x17c1bb);shift_byte(1,2);
    instruction(0x17c1be);iterator_logic_flags(r,a.gp[1]&1u,1);
    instruction(0x17c1c1);if(!(a.eflags&64u))goto clear_active;
    instruction(0x17c1c3);iterator_flags(r,read(m,a.gp[4]+0x10),8,4,true);
resume_17c1c8:
    instruction(0x17c1c8);if(bool(a.eflags&128u)==bool(a.eflags&2048u))goto clear_active;
    instruction(0x17c1ca);scalar(m,a,1,a.gp[6]+0x64);
resume_17c1cf:
    instruction(0x17c1cf);{const auto elapsed=read(m,a.gp[6]+0x60);SseEnvironment environment(a);
        vector(a,1,_mm_sub_ss(vector(a,1),_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(elapsed)))));}
resume_17c1d4:
    instruction(0x17c1d4);state_scalar_compare(a,vector(a,1),vector(a,0),false);
    instruction(0x17c1d7);scalar(m,a,3,a.gp[6]+0x60);
resume_17c1dc:
    instruction(0x17c1dc);store(m,o,0x17c1dc,a.gp[4]+0x18,a.xmm[3][0]);
resume_17c1e2:
    instruction(0x17c1e2);store(m,o,0x17c1e2,a.gp[4]+0x14,a.xmm[0][0]);
resume_17c1e8:
    instruction(0x17c1e8);if(!(a.eflags&0x41u))goto selected_interval;
    instruction(0x17c1ea);store(m,o,0x17c1ea,a.gp[4]+0x14,a.xmm[1][0]);
selected_interval:
resume_17c1f0:
    instruction(0x17c1f0);state_scalar_compare(a,vector(a,0),vector(a,1),false);
    instruction(0x17c1f3);if(a.eflags&1u)goto partial_interval;
    instruction(0x17c1f5);a.gp[2]=read(m,a.gp[6]+0x64);
resume_17c1f8:
    instruction(0x17c1f8);low(3,1);
    instruction(0x17c1fa);store(m,o,0x17c1fa,a.gp[6]+0x60,a.gp[2]);
resume_17c1fd:
    instruction(0x17c1fd);{SseEnvironment environment(a);vector(a,0,_mm_sub_ss(vector(a,0),vector(a,1)));}
    instruction(0x17c201);goto dispatch_interval;
partial_interval:
    instruction(0x17c203);scalar(m,a,1,a.gp[6]+0x60);
resume_17c208:
    instruction(0x17c208);{SseEnvironment environment(a);vector(a,1,_mm_add_ss(vector(a,1),vector(a,0)));}
    instruction(0x17c20c);scalar(m,a,0,0x46a0dc);
resume_17c214:
    instruction(0x17c214);low(3,0);iterator_logic_flags(r,0,1);
    instruction(0x17c216);store(m,o,0x17c216,a.gp[6]+0x60,a.xmm[1][0]);
dispatch_interval:
resume_17c21b:
    instruction(0x17c21b);iterator_logic_flags(r,read(m,a.gp[6]+2,1)&1u,1);
resume_17c21f:
    instruction(0x17c21f);store(m,o,0x17c21f,a.gp[4]+0x24,a.xmm[0][0]);
resume_17c225:
    instruction(0x17c225);if(a.eflags&64u)goto begin_interval;
    instruction(0x17c227);shift_byte(0,3);
    instruction(0x17c22a);iterator_logic_flags(r,a.gp[0]&1u,1);
    instruction(0x17c22c);if(!(a.eflags&64u))goto after_particles;
    instruction(0x17c22e);a.gp[0]=read(m,a.gp[4]+0x14);
resume_17c232:
    instruction(0x17c232);a.gp[1]=read(m,a.gp[4]+0x18);
resume_17c236:
    instruction(0x17c236);push(m,a,o,0x17c236,a.gp[0]);
resume_17c237:
    instruction(0x17c237);push(m,a,o,0x17c237,a.gp[1]);
resume_17c238:
    instruction(0x17c238);push(m,a,o,0x17c238,a.gp[7]);
resume_17c239:
    call(m,a,o,s,0x17c239,0x17beb0);
after_particles:
resume_17c23e:
    instruction(0x17c23e);low(2,read(m,a.gp[6]+2,1));
resume_17c241:
    instruction(0x17c241);shift_byte(2,3);
    instruction(0x17c244);iterator_logic_flags(r,a.gp[2]&1u,1);
    instruction(0x17c247);if(!(a.eflags&64u))goto next_interval;
    call(m,a,o,s,0x17c249,0x17ce40);
next_interval:
resume_17c24e:
    instruction(0x17c24e);iterator_logic_flags(r,a.gp[3]&255u,1);
    instruction(0x17c250);if(a.eflags&64u)goto finish_iteration;
    instruction(0x17c256);push(m,a,o,0x17c256,a.gp[6]);
resume_17c257:
    call(m,a,o,s,0x17c257,0x17c020);
resume_17c25c:
    instruction(0x17c25c);iterator_flags(r,a.gp[0],read(m,a.gp[5]+0x14),4,true);
resume_17c25f:
    instruction(0x17c25f);if(bool(a.eflags&128u)==bool(a.eflags&2048u))goto retire;
    instruction(0x17c265);a.gp[2]=read(m,a.gp[4]+0x20);
resume_17c269:
    instruction(0x17c269);a.gp[7]=a.gp[0];
    call(m,a,o,s,0x17c26b,0x17b130);
resume_17c270:
    instruction(0x17c270);a.gp[7]=read(m,a.gp[4]+0x20);
resume_17c274:
    instruction(0x17c274);goto finish_iteration;
begin_interval:
    instruction(0x17c276);iterator_logic_flags(r,a.gp[3]&255u,1);
    instruction(0x17c278);if(a.eflags&64u)goto finish_iteration;
    instruction(0x17c27a);a.gp[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[6]+0x5c,2)))));
resume_17c27e:
    instruction(0x17c27e);a.gp[0]=read(m,a.gp[5]+0x18);
resume_17c281:
    instruction(0x17c281);a.gp[1]=state_product(r,a.gp[1],0x38);
    instruction(0x17c284);modify_effect_byte(m,a,o,0x17c284,a.gp[6]+2,[](auto value){return value|1u;});
resume_17c288:
    instruction(0x17c288);scalar(m,a,0,0x46a0dc);
resume_17c290:
    instruction(0x17c290);a.gp[1]=iterator_flags(r,a.gp[1],a.gp[0],4,false);
    instruction(0x17c292);store(m,o,0x17c292,a.gp[6]+0x60,a.xmm[2][0]);
resume_17c297:
    instruction(0x17c297);store(m,o,0x17c297,a.gp[6]+0x68,a.xmm[0][0]);
resume_17c29c:
    instruction(0x17c29c);scalar(m,a,0,a.gp[1]+0x10);
resume_17c2a1:
    instruction(0x17c2a1);state_scalar_compare(a,vector(a,0),_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(m,a.gp[1]+0x14)))),true);
resume_17c2a5:
    instruction(0x17c2a5);a.gp[0]=(a.gp[0]&0xffff00ffu)|(((a.eflags&0xd5u)|2u)<<8u);
    instruction(0x17c2a6);iterator_logic_flags(r,(a.gp[0]>>8u)&0x44u,1);
    instruction(0x17c2a9);a.gp[0]=read(m,a.gp[1]+0x10);
resume_17c2ac:
    instruction(0x17c2ac);if(a.eflags&4u)goto random_duration;
    instruction(0x17c2ae);store(m,o,0x17c2ae,a.gp[6]+0x64,a.gp[0]);
resume_17c2b1:
    instruction(0x17c2b1);goto begin_parts;
random_duration:
    instruction(0x17c2b3);a.gp[2]=read(m,a.gp[1]+0x14);
resume_17c2b6:
    instruction(0x17c2b6);push(m,a,o,0x17c2b6,a.gp[2]);
resume_17c2b7:
    instruction(0x17c2b7);push(m,a,o,0x17c2b7,a.gp[0]);
resume_17c2b8:
    instruction(0x17c2b8);a.gp[0]=read(m,a.gp[6]+4);
resume_17c2bb:
    call(m,a,o,s,0x17c2bb,0x17aab0);
resume_17c2c0:
    instruction(0x17c2c0);a.gp[1]=a.gp[0];
    call(m,a,o,s,0x17c2c2,0x259b0);
resume_17c2c7:
    instruction(0x17c2c7);state_store_single(m,a,o,0x17c2c7);
begin_parts:
resume_17c2ca:
    instruction(0x17c2ca);low(1,read(m,a.gp[6]+2,1));
resume_17c2cd:
    instruction(0x17c2cd);shift_byte(1,3);
    instruction(0x17c2d0);iterator_logic_flags(r,a.gp[1]&1u,1);
    instruction(0x17c2d3);if(!(a.eflags&64u))goto sync_components;
    instruction(0x17c2d5);push(m,a,o,0x17c2d5,a.gp[6]);
resume_17c2d6:
    call(m,a,o,s,0x17c2d6,0x17d210);
sync_components:
resume_17c2db:
    instruction(0x17c2db);a.gp[2]=a.gp[6];
    call(m,a,o,s,0x17c2dd,0x17dc60);
resume_17c2e2:
    instruction(0x17c2e2);low(2,read(m,a.gp[6]+0x5c,2),2);
resume_17c2e6:
    instruction(0x17c2e6);store(m,o,0x17c2e6,a.gp[6]+0x5e,a.gp[2],2);
finish_iteration:
resume_17c2ea:
    instruction(0x17c2ea);a.gp[0]=read(m,a.gp[4]+0x10);
resume_17c2ee:
    instruction(0x17c2ee);scalar(m,a,0,a.gp[4]+0x24);
resume_17c2f4:
    instruction(0x17c2f4);a.xmm[2]={};
    instruction(0x17c2f7);{const auto carry=a.eflags&1u;a.gp[0]=iterator_flags(r,a.gp[0],1,4,false);a.eflags=(a.eflags&~1u)|carry;}
    instruction(0x17c2f8);state_scalar_compare(a,vector(a,0),vector(a,2),false);
    instruction(0x17c2fb);store(m,o,0x17c2fb,a.gp[4]+0x10,a.gp[0]);
resume_17c2ff:
    instruction(0x17c2ff);if(!(a.eflags&1u))goto reload_interval;
    instruction(0x17c305);goto clear_active;
retire:
    instruction(0x17c30a);a.gp[0]=a.gp[7];
    call(m,a,o,s,0x17c30c,0x17a4a0);
resume_17c311:
    instruction(0x17c311);iterator_logic_flags(r,a.gp[0]&255u,1);
    instruction(0x17c313);if(a.eflags&64u)goto clear_active;
    instruction(0x17c319);a.gp[3]=pop(m,a);
resume_17c31a:
    instruction(0x17c31a);a.gp[6]=0;iterator_logic_flags(r,0,4);
    instruction(0x17c31c);a.gp[0]=0;iterator_logic_flags(r,0,4);
    instruction(0x17c31e);a.gp[7]=pop(m,a);
resume_17c31f:
    instruction(0x17c31f);iterator_logic_flags(r,a.gp[6],4);
    instruction(0x17c321);a.gp[6]=pop(m,a);
resume_17c322:
    instruction(0x17c322);low(0,bool(a.eflags&64u));
    instruction(0x17c325);a.gp[5]=pop(m,a);
resume_17c326:
    instruction(0x17c326);a.gp[4]=iterator_flags(r,a.gp[4],12,4,false);
    instruction(0x17c329);(void)pop(m,a);a.gp[4]+=8;
}
