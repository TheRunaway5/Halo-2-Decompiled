// TU1.5 179ED0 walks the authored systems of an existing effect component.
// Its own instructions are integer-only; every real lower owner retains the
// complete FP/MMX/GP ABI. Stack bytes, including the reused caller argument,
// remain the source's storage and are never normalized into a C++ bool.
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
    instruction(0x179ed0);a.gp[4]=iterator_flags(r,a.gp[4],0x54,4,true);
    instruction(0x179ed3);a.gp[2]=read(m,0x51df74);
    instruction(0x179ed9);a.gp[1]=read(m,0x4f720c);
    instruction(0x179edf);push(m,a,o,0x179edf,a.gp[3]);
    instruction(0x179ee0);a.gp[3]=read(m,a.gp[1]+0x44);
    instruction(0x179ee3);push(m,a,o,0x179ee3,a.gp[5]);
    instruction(0x179ee4);a.gp[5]=read(m,a.gp[4]+0x60);
    instruction(0x179ee8);a.gp[0]&=65535u;iterator_logic_flags(r,a.gp[0],4);
    instruction(0x179eed);a.gp[0]=state_product(r,a.gp[0],0x54);
    instruction(0x179ef0);a.gp[5]&=65535u;iterator_logic_flags(r,a.gp[5],4);
    instruction(0x179ef6);push(m,a,o,0x179ef6,a.gp[6]);
    instruction(0x179ef7);a.gp[5]=state_product(r,a.gp[5],0x190);
    instruction(0x179efd);a.gp[0]=iterator_flags(r,a.gp[0],read(m,a.gp[2]+0x44),4,false);
    instruction(0x179f00);a.gp[6]=a.gp[0];
    instruction(0x179f02);push(m,a,o,0x179f02,a.gp[7]);
    instruction(0x179f03);a.gp[7]=read(m,a.gp[6]+0x38);
    instruction(0x179f06);a.gp[1]=a.gp[6];
    instruction(0x179f08);a.gp[5]=iterator_flags(r,a.gp[5],a.gp[3],4,false);
    instruction(0x179f0a);store(m,o,0x179f0a,a.gp[4]+0x20,a.gp[6]);
    instruction(0x179f0e);store(m,o,0x179f0e,a.gp[4]+0x18,a.gp[7]);
    call(m,a,o,s,0x179f12,0x178070);
    instruction(0x179f17);a.gp[1]=read(m,a.gp[0]+8);
    instruction(0x179f1a);a.gp[2]=read(m,a.gp[5]+a.gp[1]*4u+0x7c);
    instruction(0x179f1e);store(m,o,0x179f1e,a.gp[4]+0x14,a.gp[2]);
    instruction(0x179f22);compare(read(m,a.gp[0]+0xc,2),0,2);
    instruction(0x179f27);store(m,o,0x179f27,a.gp[4]+0x10,a.gp[0]);
    instruction(0x179f2b);
    if(!(a.eflags&64u)) {
        instruction(0x179f31);a.gp[1]=read(m,a.gp[6]+0x30);
        instruction(0x179f34);a.gp[0]=read(m,a.gp[5]+0x104);
        instruction(0x179f3a);a.gp[7]=read(m,a.gp[5]+0x108);
        instruction(0x179f40);store(m,o,0x179f40,a.gp[4]+0x2c,a.gp[1]);
        instruction(0x179f44);a.gp[1]=a.gp[6];
        instruction(0x179f46);store(m,o,0x179f46,a.gp[4]+0x1c,a.gp[0]);
        call(m,a,o,s,0x179f4a,0x178070);
        instruction(0x179f4f);a.gp[2]=read(m,a.gp[0]+4);
        call(m,a,o,s,0x179f52,0x13a6b0);
        instruction(0x179f57);a.gp[3]=a.gp[0];
        instruction(0x179f59);a.gp[2]=read(m,a.gp[3]);
        instruction(0x179f5b);a.gp[1]=a.gp[3];
        indirect(0x179f5d,0x48);
        instruction(0x179f60);a.gp[2]=read(m,a.gp[3]);
        instruction(0x179f62);a.gp[1]=a.gp[3];
        instruction(0x179f64);store(m,o,0x179f64,a.gp[4]+0x68,a.gp[0],1);
        indirect(0x179f68,0x44);
        instruction(0x179f6b);a.gp[1]=read(m,a.gp[4]+0x1c);
        instruction(0x179f6f);push(m,a,o,0x179f6f,a.gp[0]);
        instruction(0x179f70);a.gp[0]=(a.gp[0]&0xffffff00u)|read(m,a.gp[4]+0x6c,1);
        call(m,a,o,s,0x179f74,0x178920);
        instruction(0x179f79);a.gp[0]=read(m,a.gp[4]+0x10);
        instruction(0x179f7d);a.gp[0]=read(m,a.gp[0]+0x12,2);
        instruction(0x179f81);push(m,a,o,0x179f81,a.gp[0]);
        instruction(0x179f82);a.gp[1]=a.gp[4]+0x18;
        instruction(0x179f86);push(m,a,o,0x179f86,a.gp[1]);
        instruction(0x179f87);push(m,a,o,0x179f87,a.gp[5]);
        call(m,a,o,s,0x179f88,0x17dce0);
        instruction(0x179f8d);a.gp[3]=a.gp[0];
        instruction(0x179f8f);iterator_logic_flags(r,a.gp[3],4);
        instruction(0x179f91);
        if(!(a.eflags&64u)) {
            do {
                instruction(0x179f93);push(m,a,o,0x179f93,1);
                instruction(0x179f95);a.gp[7]=a.gp[4]+0x34;
                instruction(0x179f99);a.gp[6]=a.gp[5];
                call(m,a,o,s,0x179f9b,0x17ba50);
                instruction(0x179fa0);a.gp[3]=(a.gp[3]&0xffff0000u)|read(m,a.gp[3]+2,2);
                instruction(0x179fa4);compare(a.gp[3],none,2);
                instruction(0x179fa8);
                if(!(a.eflags&64u)) {
                    instruction(0x179faa);iterator_logic_flags(r,(a.gp[3]>>8u)&255u,1);
                    instruction(0x179fac);store(m,o,0x179fac,a.gp[4]+0x68,1,1);
                    instruction(0x179fb1);
                    if(!(a.eflags&128u)) {
                        instruction(0x179fb3);store(m,o,0x179fb3,a.gp[4]+0x68,0,1);
                    }
                }else {instruction(0x179fb3);store(m,o,0x179fb3,a.gp[4]+0x68,0,1);}
                instruction(0x179fb8);a.gp[6]=read(m,a.gp[4]+0x2c);
                instruction(0x179fbc);compare(a.gp[6],none);
                instruction(0x179fbf);
                if(!(a.eflags&64u)) {
                    instruction(0x179fc1);a.gp[2]=read(m,0x52d71c);
                    instruction(0x179fc7);a.gp[1]=read(m,a.gp[4]+0x68);
                    instruction(0x179fcb);a.gp[7]=read(m,a.gp[2]+0x44);
                    instruction(0x179fce);a.gp[6]&=65535u;iterator_logic_flags(r,a.gp[6],4);
                    instruction(0x179fd4);a.gp[6]=state_product(r,a.gp[6],0x34);
                    instruction(0x179fd7);push(m,a,o,0x179fd7,a.gp[1]);
                    instruction(0x179fd8);a.gp[1]=read(m,a.gp[4]+0x24);
                    instruction(0x179fdc);a.gp[6]=iterator_flags(r,a.gp[6],a.gp[7],4,false);
                    instruction(0x179fde);push(m,a,o,0x179fde,a.gp[0]);
                    instruction(0x179fdf);a.gp[0]=a.gp[6];
                    call(m,a,o,s,0x179fe1,0x251f70);
                    instruction(0x179fe6);a.gp[2]=read(m,a.gp[6]+0xc);
                    instruction(0x179fe9);store(m,o,0x179fe9,a.gp[4]+0x2c,a.gp[2]);
                }
                instruction(0x179fed);a.gp[0]=read(m,a.gp[4]+0x10);
                instruction(0x179ff1);a.gp[0]=read(m,a.gp[0]+0x12,2);
                instruction(0x179ff5);push(m,a,o,0x179ff5,a.gp[0]);
                instruction(0x179ff6);a.gp[1]=a.gp[4]+0x18;
                instruction(0x179ffa);push(m,a,o,0x179ffa,a.gp[1]);
                instruction(0x179ffb);push(m,a,o,0x179ffb,a.gp[5]);
                call(m,a,o,s,0x179ffc,0x17dce0);
                instruction(0x17a001);a.gp[3]=a.gp[0];
                instruction(0x17a003);iterator_logic_flags(r,a.gp[3],4);
                instruction(0x17a005);
            }while(!(a.eflags&64u));
        }
        instruction(0x17a007);a.gp[0]=read(m,a.gp[4]+0x18);
        instruction(0x17a00b);a.gp[7]=pop(m,a);
        instruction(0x17a00c);a.gp[6]=pop(m,a);
        instruction(0x17a00d);a.gp[5]=pop(m,a);
        instruction(0x17a00e);a.gp[3]=pop(m,a);
        instruction(0x17a00f);a.gp[4]=iterator_flags(r,a.gp[4],0x54,4,false);
        instruction(0x17a012);(void)pop(m,a);a.gp[4]+=4u;return;
    }
    instruction(0x17a015);a.gp[0]=a.gp[7];
    instruction(0x17a017);a.gp[7]=pop(m,a);
    instruction(0x17a018);a.gp[6]=pop(m,a);
    instruction(0x17a019);a.gp[5]=pop(m,a);
    instruction(0x17a01a);a.gp[3]=pop(m,a);
    instruction(0x17a01b);a.gp[4]=iterator_flags(r,a.gp[4],0x54,4,false);
    instruction(0x17a01e);(void)pop(m,a);a.gp[4]+=4u;
}
