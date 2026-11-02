// Combined with independent Child helpers and the frozen native graph.
namespace {
struct ParentStorage final:halo2::SourcePhysicsContactMemory {
    Child&child;explicit ParentStorage(Child&c):child(c){}
    void read(std::uint32_t p,std::span<std::byte>b)const override{const auto value=child.read(p,b.size());std::copy(value.begin(),value.end(),b.begin());}
    void write(std::uint32_t p,std::span<const std::byte>b)override{child.write(p,b);}
};
void enter_original_parent(Child&c,const char*exe,const J&input,unsigned mode){
    c.start(exe);constexpr std::uint32_t sp=0x7200f000,stop=0x70000080;
    const std::array<std::uint32_t,4>args{input.at("first"),input.at("second"),input.at("configuration"),input.at("output")};
    c.put(sp,stop);for(unsigned n=0;n<4;++n)c.put(sp+4u+n*4u,args[n]);
    auto r=c.regs();r.rax=r.rbx=r.rdx=r.rsi=r.rdi=r.rbp=0;r.rcx=input.at("agent");r.rsp=sp;r.rip=0x304350u;r.eflags&=~0x400u;request(PTRACE_SETREGS,c.pid,nullptr,&r);
    auto f=c.fp();f.cwd=input.at("floating_point").at("x87_control_word");f.swd=input.at("floating_point").at("x87_status");f.mxcsr=input.at("floating_point").at("mxcsr");f.cwd=(f.cwd&~0xc00u)|(mode<<10);f.mxcsr=(f.mxcsr&~0x6000u)|(mode<<13);f.ftw=0;std::memset(f.st_space,0,sizeof(f.st_space));request(PTRACE_SETFPREGS,c.pid,nullptr,&f);
    require(c.go({stop})==stop,"Original parent did not return");require(c.regs().rsp==sp+20u,"Original parent RET16 differs");
}
J parent_activity(const ParentActivities&s){return J{{"candidate_calls",s.providers.candidates.completed},{"query_calls",s.providers.query.completed},{"solver_calls",s.solver.completed},{"appends",s.providers.append_requests},{"resolutions",s.providers.resolutions},{"pair_tests",s.providers.pair_tests},{"component_constructions",s.providers.component_constructions},{"component_destructions",s.providers.component_destructions},{"publications",s.providers.publication.completed},{"distances",s.providers.distances.completed},{"rejected_process_requests",s.providers.rejected_process_requests}};}
}
int main(int argc,char**argv){J report{{"passed",false},{"native_original_executor_calls",0},{"retained_large_artifacts",J::array()}};try{
    require(argc==6,"Supply ELF, typed304 input, regions, mode and result");std::ifstream in(argv[2]),meta_in(argv[3]);const auto input=J::parse(in),metadata=J::parse(meta_in);const auto mode=unsigned(std::stoul(argv[4]));
    Child original;enter_original_parent(original,argv[1],input,mode);const auto expected_fp=fp(original);
    struct Bank{std::uint32_t address;std::vector<std::byte>after;};std::vector<Bank>banks;std::vector<halo2::NativeTu15ContactPrivateRange>retained;
    for(const auto&row:metadata.at("regions")){const auto p=row.at("address").get<std::uint32_t>(),n=row.at("size").get<std::uint32_t>();require(!(std::uint64_t(p)<std::uint64_t(PARENT_DATA)+0x10000u&&PARENT_DATA<std::uint64_t(p)+n),"Parent private arena aliases genuine retained DATA");banks.push_back({p,original.read(p,n)});retained.push_back({p,n});}
    J cases=J::array();
    for(auto poison:{std::byte(0xa5),std::byte(0x5a)}){
        Child child;child.start(argv[1]);ParentStorage storage(child);
        const std::array<halo2::NativeTu15ContactPrivateRange,1>owned{{{PARENT_DATA,0x10000u}}};
        halo2::NativeTu15ContactCandidatePrivateData defined(storage,owned,retained);
        const std::vector<std::byte>poison_bytes(0x10000u,poison);defined.write(PARENT_DATA,poison_bytes);require(defined.withhold_defined_data(PARENT_DATA,0x10000u),"Private arena cannot invalidate its initial test poison");
        halo2::NativeTu15PhysicsFrameFp current{input.at("floating_point").at("x87_control_word"),input.at("floating_point").at("x87_status"),input.at("floating_point").at("mxcsr")};current.control_word=(current.control_word&~0xc00u)|(mode<<10);current.mxcsr=(current.mxcsr&~0x6000u)|(mode<<13);
        ParentActivities activity;struct FinalActivity{J&r;const ParentActivities&a;~FinalActivity(){r["last_native_activity"]=parent_activity(a);}} finalize{report,activity};
        run_native_parent(defined,input,current,activity);
        std::uint64_t compared{},different{};J diffs=J::array();for(const auto&bank:banks){const auto actual=child.read(bank.address,bank.after.size());compared+=actual.size();for(std::size_t n=0;n<actual.size();++n)if(actual[n]!=bank.after[n]){++different;if(diffs.size()<24)diffs.push_back(J{{"address",bank.address+n},{"native",std::to_integer<unsigned>(actual[n])},{"original",std::to_integer<unsigned>(bank.after[n])}});}}
        const auto after=J::array({current.control_word,current.status,current.mxcsr});const bool passed=!different&&after==expected_fp;
        cases.push_back(J{{"passed",passed},{"declared_poison",std::to_integer<unsigned>(poison)},{"compared_bytes",compared},{"different_bytes",different},{"differences",diffs},{"fp",after},{"expected_fp",expected_fp},{"native_activity",parent_activity(activity)}});
    }
    report.update(J{{"passed",std::all_of(cases.begin(),cases.end(),[](const auto&c){return c.at("passed").template get<bool>();})},{"mode",mode},{"cases",cases},{"captured_gp_used",false},{"source_after_used_as_native_input",false},{"private_arena_initially_undefined",true}});
}catch(const std::exception&e){report["passed"]=false;report["error"]=e.what();std::cerr<<e.what()<<'\n';}
if(argc==6){std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;}
