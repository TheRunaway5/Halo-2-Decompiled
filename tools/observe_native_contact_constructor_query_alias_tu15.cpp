// Original-only hardware writer trace. No trace value is supplied to native.
int main(int argc,char**argv){J report{{"passed",false},{"native_input_supplied",false},{"instruction_patches",J::array()}};try{
    require(argc==7,"Supply ELF, typed304 input, mode, offset, phase and result");std::ifstream in(argv[2]);const auto input=J::parse(in);const auto mode=unsigned(std::stoul(argv[3])),offset=unsigned(std::stoul(argv[4]));const bool from_candidate=std::string(argv[5])=="candidate";
    Child c;c.start(argv[1]);constexpr std::uint32_t sp=0x7200f000,stop=0x70000080;
    const std::array<std::uint32_t,4>args{input.at("first"),input.at("second"),input.at("configuration"),input.at("output")};
    c.put(sp,stop);for(unsigned n=0;n<4;++n)c.put(sp+4u+n*4u,args[n]);
    auto r=c.regs();r.rax=r.rbx=r.rdx=r.rsi=r.rdi=r.rbp=0;r.rcx=input.at("agent");r.rsp=sp;r.rip=0x304350u;r.eflags&=~0x400u;request(PTRACE_SETREGS,c.pid,nullptr,&r);
    auto f=c.fp();f.cwd=input.at("floating_point").at("x87_control_word");f.swd=input.at("floating_point").at("x87_status");f.mxcsr=input.at("floating_point").at("mxcsr");
    f.cwd=(f.cwd&~0xc00u)|(mode<<10);f.mxcsr=(f.mxcsr&~0x6000u)|(mode<<13);f.ftw=0;std::memset(f.st_space,0,sizeof(f.st_space));request(PTRACE_SETFPREGS,c.pid,nullptr,&f);
    const auto boundary=from_candidate?0x2f36f0u:0x2e4fe0u;
    require(c.go({boundary,stop})==boundary,"Actual caller missed requested phase");
    const auto query_sp=std::uint32_t(c.regs().rsp),caller_sp=query_sp+(from_candidate?20u:24u);
    const auto constructor=((caller_sp-8u)&~15u)-0x80u,watched=constructor+offset;
    const auto before=c.word(watched);J events=J::array();
    c.debug(0,0x303ee0u);c.debug(1,watched);c.debug(6,0);c.debug(7,1u|4u|(13u<<20u));
    bool reached=false;
    for(unsigned n=0;n<4096;++n){
        request(PTRACE_CONT,c.pid);c.wait();r=c.regs();require(WSTOPSIG(c.status)==SIGTRAP,"Hardware query/constructor watch faulted");
        if(r.rip==0x303ee0u){reached=true;break;}
        const auto pc=std::uint32_t(r.rip);const auto cause=request(PTRACE_PEEKUSER,c.pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)+6*sizeof(unsigned long)));
        require(cause&2u,"Unexpected constructor hardware boundary");
        events.push_back(J{{"after_pc",pc},{"value",c.word(watched)},{"preceding_code_hex15",hex(c.read(pc-15u,15u))}});c.debug(6,0);
    }
    c.debug(7,0);require(reached,"Constructor writer budget exhausted");
    const auto entry=std::uint32_t(c.regs().rsp);require(constructor==((entry-4u)&~15u)-0x80u,"Future constructor lifetime address differs");
    require(c.go({0x303f31u})==0x303f31u,"Constructor missed genuine copy");
    const auto actual_constructor=c.word(c.regs().rsp);require(actual_constructor==constructor,"Actual constructor input differs");
    report.update(J{{"passed",true},{"mode",mode},{"offset",offset},{"phase",argv[5]},{"phase_entry_sp",query_sp},{"constructor",constructor},{"watched",watched},{"before_word",before},{"constructor_word",c.word(watched)},{"events",events},{"last_writer_is_semantic_data",false},{"scope","Writer evidence only; classify authenticated Source instruction before any native alias admission."}});
}catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
if(argc==7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;}
