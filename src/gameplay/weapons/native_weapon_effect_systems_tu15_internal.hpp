// TU1.5 179ED0 walks the authored systems of an existing effect component.
// Its own instructions are integer-only; every real lower owner retains the
// complete FP/MMX/GP ABI. Stack bytes, including the reused caller argument,
// remain the source's storage and are never normalized into a C++ bool.
#include "native_weapon_effect_systems_kernel_tu15_internal.hpp"

void advance_effect_systems(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    IteratorRegisters r{a.gp,a.eflags};
    const auto compare=[&](auto x,auto y,unsigned width=4){iterator_flags(r,x,y,width,true);};
    const auto indirect=[&](std::uint32_t pc,std::uint32_t table_offset) {
        instruction(pc);const auto target=read(m,a.gp[2]+table_offset);
        push(m,a,o,pc,pc+3u);
        if(!s.required_owner)throw std::runtime_error("Native effect system requires its actual virtual owner");
        s.required_owner(m,{pc,target,false},a,o);
    };
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_WEAPON_EFFECT_SYSTEMS");
        return value && *value;
    }();
    if (!reference && (!s.instruction || s.immutable_instruction_cursor.identifies(s)) &&
        native_model_marker_lookup_detail::protocol(s, o)) {
        try {
            effect_systems_update::Frame(m, a, o, s).run();
            return;
        } catch (effect_systems_update::Resume next) {
            switch (next.pc) {
            case 0x179ed9u: goto resume_179ed9;
            case 0x179edfu: goto resume_179edf;
            case 0x179ee0u: goto resume_179ee0;
            case 0x179ee3u: goto resume_179ee3;
            case 0x179ee4u: goto resume_179ee4;
            case 0x179ee8u: goto resume_179ee8;
            case 0x179ef7u: goto resume_179ef7;
            case 0x179f00u: goto resume_179f00;
            case 0x179f03u: goto resume_179f03;
            case 0x179f06u: goto resume_179f06;
            case 0x179f0eu: goto resume_179f0e;
            case 0x179f12u: goto resume_179f12;
            case 0x179f17u: goto resume_179f17;
            case 0x179f1au: goto resume_179f1a;
            case 0x179f1eu: goto resume_179f1e;
            case 0x179f22u: goto resume_179f22;
            case 0x179f27u: goto resume_179f27;
            case 0x179f2bu: goto resume_179f2b;
            case 0x179f34u: goto resume_179f34;
            case 0x179f3au: goto resume_179f3a;
            case 0x179f40u: goto resume_179f40;
            case 0x179f44u: goto resume_179f44;
            case 0x179f4au: goto resume_179f4a;
            case 0x179f4fu: goto resume_179f4f;
            case 0x179f52u: goto resume_179f52;
            case 0x179f57u: goto resume_179f57;
            case 0x179f5bu: goto resume_179f5b;
            case 0x179f60u: goto resume_179f60;
            case 0x179f62u: goto resume_179f62;
            case 0x179f68u: goto resume_179f68;
            case 0x179f6bu: goto resume_179f6b;
            case 0x179f6fu: goto resume_179f6f;
            case 0x179f70u: goto resume_179f70;
            case 0x179f74u: goto resume_179f74;
            case 0x179f79u: goto resume_179f79;
            case 0x179f7du: goto resume_179f7d;
            case 0x179f81u: goto resume_179f81;
            case 0x179f82u: goto resume_179f82;
            case 0x179f87u: goto resume_179f87;
            case 0x179f88u: goto resume_179f88;
            case 0x179f8du: goto resume_179f8d;
            case 0x179f95u: goto resume_179f95;
            case 0x179fa0u: goto resume_179fa0;
            case 0x179fa4u: goto resume_179fa4;
            case 0x179fb1u: goto resume_179fb1;
            case 0x179fb8u: goto resume_179fb8;
            case 0x179fbcu: goto resume_179fbc;
            case 0x179fc7u: goto resume_179fc7;
            case 0x179fcbu: goto resume_179fcb;
            case 0x179fceu: goto resume_179fce;
            case 0x179fd8u: goto resume_179fd8;
            case 0x179fdcu: goto resume_179fdc;
            case 0x179fdfu: goto resume_179fdf;
            case 0x179fe6u: goto resume_179fe6;
            case 0x179fe9u: goto resume_179fe9;
            case 0x179fedu: goto resume_179fed;
            case 0x179ff1u: goto resume_179ff1;
            case 0x179ff5u: goto resume_179ff5;
            case 0x179ff6u: goto resume_179ff6;
            case 0x179ffbu: goto resume_179ffb;
            case 0x179ffcu: goto resume_179ffc;
            case 0x17a001u: goto resume_17a001;
            case 0x17a00bu: goto resume_17a00b;
            case 0x17a00cu: goto resume_17a00c;
            case 0x17a00du: goto resume_17a00d;
            case 0x17a00eu: goto resume_17a00e;
            case 0x17a00fu: goto resume_17a00f;
            case 0x17a018u: goto resume_17a018;
            case 0x17a019u: goto resume_17a019;
            case 0x17a01au: goto resume_17a01a;
            case 0x17a01bu: goto resume_17a01b;
            }
            throw;
        }
    }
    instruction(0x179ed0);a.gp[4]=iterator_flags(r,a.gp[4],0x54,4,true);
    instruction(0x179ed3);a.gp[2]=read(m,0x51df74);
resume_179ed9:
    instruction(0x179ed9);a.gp[1]=read(m,0x4f720c);
resume_179edf:
    instruction(0x179edf);push(m,a,o,0x179edf,a.gp[3]);
resume_179ee0:
    instruction(0x179ee0);a.gp[3]=read(m,a.gp[1]+0x44);
resume_179ee3:
    instruction(0x179ee3);push(m,a,o,0x179ee3,a.gp[5]);
resume_179ee4:
    instruction(0x179ee4);a.gp[5]=read(m,a.gp[4]+0x60);
resume_179ee8:
    instruction(0x179ee8);a.gp[0]&=65535u;iterator_logic_flags(r,a.gp[0],4);
    instruction(0x179eed);a.gp[0]=state_product(r,a.gp[0],0x54);
    instruction(0x179ef0);a.gp[5]&=65535u;iterator_logic_flags(r,a.gp[5],4);
    instruction(0x179ef6);push(m,a,o,0x179ef6,a.gp[6]);
resume_179ef7:
    instruction(0x179ef7);a.gp[5]=state_product(r,a.gp[5],0x190);
    instruction(0x179efd);a.gp[0]=iterator_flags(r,a.gp[0],read(m,a.gp[2]+0x44),4,false);
resume_179f00:
    instruction(0x179f00);a.gp[6]=a.gp[0];
    instruction(0x179f02);push(m,a,o,0x179f02,a.gp[7]);
resume_179f03:
    instruction(0x179f03);a.gp[7]=read(m,a.gp[6]+0x38);
resume_179f06:
    instruction(0x179f06);a.gp[1]=a.gp[6];
    instruction(0x179f08);a.gp[5]=iterator_flags(r,a.gp[5],a.gp[3],4,false);
    instruction(0x179f0a);store(m,o,0x179f0a,a.gp[4]+0x20,a.gp[6]);
resume_179f0e:
    instruction(0x179f0e);store(m,o,0x179f0e,a.gp[4]+0x18,a.gp[7]);
resume_179f12:
    call(m,a,o,s,0x179f12,0x178070);
resume_179f17:
    instruction(0x179f17);a.gp[1]=read(m,a.gp[0]+8);
resume_179f1a:
    instruction(0x179f1a);a.gp[2]=read(m,a.gp[5]+a.gp[1]*4u+0x7c);
resume_179f1e:
    instruction(0x179f1e);store(m,o,0x179f1e,a.gp[4]+0x14,a.gp[2]);
resume_179f22:
    instruction(0x179f22);compare(read(m,a.gp[0]+0xc,2),0,2);
resume_179f27:
    instruction(0x179f27);store(m,o,0x179f27,a.gp[4]+0x10,a.gp[0]);
resume_179f2b:
    instruction(0x179f2b);
    if(!(a.eflags&64u)) {
        instruction(0x179f31);a.gp[1]=read(m,a.gp[6]+0x30);
resume_179f34:
        instruction(0x179f34);a.gp[0]=read(m,a.gp[5]+0x104);
resume_179f3a:
        instruction(0x179f3a);a.gp[7]=read(m,a.gp[5]+0x108);
resume_179f40:
        instruction(0x179f40);store(m,o,0x179f40,a.gp[4]+0x2c,a.gp[1]);
resume_179f44:
        instruction(0x179f44);a.gp[1]=a.gp[6];
        instruction(0x179f46);store(m,o,0x179f46,a.gp[4]+0x1c,a.gp[0]);
resume_179f4a:
        call(m,a,o,s,0x179f4a,0x178070);
resume_179f4f:
        instruction(0x179f4f);a.gp[2]=read(m,a.gp[0]+4);
resume_179f52:
        call(m,a,o,s,0x179f52,0x13a6b0);
resume_179f57:
        instruction(0x179f57);a.gp[3]=a.gp[0];
        instruction(0x179f59);a.gp[2]=read(m,a.gp[3]);
resume_179f5b:
        instruction(0x179f5b);a.gp[1]=a.gp[3];
        indirect(0x179f5d,0x48);
resume_179f60:
        instruction(0x179f60);a.gp[2]=read(m,a.gp[3]);
resume_179f62:
        instruction(0x179f62);a.gp[1]=a.gp[3];
        instruction(0x179f64);store(m,o,0x179f64,a.gp[4]+0x68,a.gp[0],1);
resume_179f68:
        indirect(0x179f68,0x44);
resume_179f6b:
        instruction(0x179f6b);a.gp[1]=read(m,a.gp[4]+0x1c);
resume_179f6f:
        instruction(0x179f6f);push(m,a,o,0x179f6f,a.gp[0]);
resume_179f70:
        instruction(0x179f70);a.gp[0]=(a.gp[0]&0xffffff00u)|read(m,a.gp[4]+0x6c,1);
resume_179f74:
        call(m,a,o,s,0x179f74,0x178920);
resume_179f79:
        instruction(0x179f79);a.gp[0]=read(m,a.gp[4]+0x10);
resume_179f7d:
        instruction(0x179f7d);a.gp[0]=read(m,a.gp[0]+0x12,2);
resume_179f81:
        instruction(0x179f81);push(m,a,o,0x179f81,a.gp[0]);
resume_179f82:
        instruction(0x179f82);a.gp[1]=a.gp[4]+0x18;
        instruction(0x179f86);push(m,a,o,0x179f86,a.gp[1]);
resume_179f87:
        instruction(0x179f87);push(m,a,o,0x179f87,a.gp[5]);
resume_179f88:
        call(m,a,o,s,0x179f88,0x17dce0);
resume_179f8d:
        instruction(0x179f8d);a.gp[3]=a.gp[0];
        instruction(0x179f8f);iterator_logic_flags(r,a.gp[3],4);
        instruction(0x179f91);
        if(!(a.eflags&64u)) {
            do {
                instruction(0x179f93);push(m,a,o,0x179f93,1);
resume_179f95:
                instruction(0x179f95);a.gp[7]=a.gp[4]+0x34;
                instruction(0x179f99);a.gp[6]=a.gp[5];
                call(m,a,o,s,0x179f9b,0x17ba50);
resume_179fa0:
                instruction(0x179fa0);a.gp[3]=(a.gp[3]&0xffff0000u)|read(m,a.gp[3]+2,2);
resume_179fa4:
                instruction(0x179fa4);compare(a.gp[3],none,2);
                instruction(0x179fa8);
                if(!(a.eflags&64u)) {
                    instruction(0x179faa);iterator_logic_flags(r,(a.gp[3]>>8u)&255u,1);
                    instruction(0x179fac);store(m,o,0x179fac,a.gp[4]+0x68,1,1);
resume_179fb1:
                    instruction(0x179fb1);
                    if(!(a.eflags&128u)) {
                        instruction(0x179fb3);store(m,o,0x179fb3,a.gp[4]+0x68,0,1);
                    }
                }else {instruction(0x179fb3);store(m,o,0x179fb3,a.gp[4]+0x68,0,1);}
resume_179fb8:
                instruction(0x179fb8);a.gp[6]=read(m,a.gp[4]+0x2c);
resume_179fbc:
                instruction(0x179fbc);compare(a.gp[6],none);
                instruction(0x179fbf);
                if(!(a.eflags&64u)) {
                    instruction(0x179fc1);a.gp[2]=read(m,0x52d71c);
resume_179fc7:
                    instruction(0x179fc7);a.gp[1]=read(m,a.gp[4]+0x68);
resume_179fcb:
                    instruction(0x179fcb);a.gp[7]=read(m,a.gp[2]+0x44);
resume_179fce:
                    instruction(0x179fce);a.gp[6]&=65535u;iterator_logic_flags(r,a.gp[6],4);
                    instruction(0x179fd4);a.gp[6]=state_product(r,a.gp[6],0x34);
                    instruction(0x179fd7);push(m,a,o,0x179fd7,a.gp[1]);
resume_179fd8:
                    instruction(0x179fd8);a.gp[1]=read(m,a.gp[4]+0x24);
resume_179fdc:
                    instruction(0x179fdc);a.gp[6]=iterator_flags(r,a.gp[6],a.gp[7],4,false);
                    instruction(0x179fde);push(m,a,o,0x179fde,a.gp[0]);
resume_179fdf:
                    instruction(0x179fdf);a.gp[0]=a.gp[6];
                    call(m,a,o,s,0x179fe1,0x251f70);
resume_179fe6:
                    instruction(0x179fe6);a.gp[2]=read(m,a.gp[6]+0xc);
resume_179fe9:
                    instruction(0x179fe9);store(m,o,0x179fe9,a.gp[4]+0x2c,a.gp[2]);
                }
resume_179fed:
                instruction(0x179fed);a.gp[0]=read(m,a.gp[4]+0x10);
resume_179ff1:
                instruction(0x179ff1);a.gp[0]=read(m,a.gp[0]+0x12,2);
resume_179ff5:
                instruction(0x179ff5);push(m,a,o,0x179ff5,a.gp[0]);
resume_179ff6:
                instruction(0x179ff6);a.gp[1]=a.gp[4]+0x18;
                instruction(0x179ffa);push(m,a,o,0x179ffa,a.gp[1]);
resume_179ffb:
                instruction(0x179ffb);push(m,a,o,0x179ffb,a.gp[5]);
resume_179ffc:
                call(m,a,o,s,0x179ffc,0x17dce0);
resume_17a001:
                instruction(0x17a001);a.gp[3]=a.gp[0];
                instruction(0x17a003);iterator_logic_flags(r,a.gp[3],4);
                instruction(0x17a005);
            }while(!(a.eflags&64u));
        }
        instruction(0x17a007);a.gp[0]=read(m,a.gp[4]+0x18);
resume_17a00b:
        instruction(0x17a00b);a.gp[7]=pop(m,a);
resume_17a00c:
        instruction(0x17a00c);a.gp[6]=pop(m,a);
resume_17a00d:
        instruction(0x17a00d);a.gp[5]=pop(m,a);
resume_17a00e:
        instruction(0x17a00e);a.gp[3]=pop(m,a);
resume_17a00f:
        instruction(0x17a00f);a.gp[4]=iterator_flags(r,a.gp[4],0x54,4,false);
        instruction(0x17a012);(void)pop(m,a);a.gp[4]+=4u;return;
    }
    instruction(0x17a015);a.gp[0]=a.gp[7];
    instruction(0x17a017);a.gp[7]=pop(m,a);
resume_17a018:
    instruction(0x17a018);a.gp[6]=pop(m,a);
resume_17a019:
    instruction(0x17a019);a.gp[5]=pop(m,a);
resume_17a01a:
    instruction(0x17a01a);a.gp[3]=pop(m,a);
resume_17a01b:
    instruction(0x17a01b);a.gp[4]=iterator_flags(r,a.gp[4],0x54,4,false);
    instruction(0x17a01e);(void)pop(m,a);a.gp[4]+=4u;
}
