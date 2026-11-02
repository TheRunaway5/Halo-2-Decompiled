// Combined with the authenticated original-caller observer's Child helpers.
// Only ordinary formals/FP enter the native factory. Original code is never
// resumed from the native factory boundary; its memory adapter reads DATA only.
#include "halo2/native_tu15_convex_agent_binding.hpp"
namespace {
using namespace halo2;
constexpr std::uint32_t NATIVE_DATA=0x74000000u;
struct FactoryMemory final:SourcePhysicsContactMemory {
    Child&child;std::vector<std::byte>owned;
    FactoryMemory(Child&c,std::byte poison):child(c),owned(16384,poison){}
    bool local(std::uint32_t p,std::size_t n)const{return p>=NATIVE_DATA&&std::uint64_t(p-NATIVE_DATA)+n<=owned.size();}
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        if(local(p,b.size()))std::copy_n(owned.begin()+p-NATIVE_DATA,b.size(),b.begin());
        else{const auto bytes=child.read(p,b.size());std::copy(bytes.begin(),bytes.end(),b.begin());}
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        if(local(p,b.size()))std::copy(b.begin(),b.end(),owned.begin()+p-NATIVE_DATA);
        else child.write(p,b);
    }
};
void enter_factory(Child&c,const char*exe,const J&input,unsigned mode){
    c.start(exe);constexpr std::uint32_t sp=0x7200f000,stop=0x70000080;
    const std::array<std::uint32_t,4>arguments{input.at("first"),input.at("second"),input.at("configuration"),input.at("output")};
    c.put(sp,stop);for(unsigned n=0;n<4;++n)c.put(sp+4u+n*4u,arguments[n]);
    auto r=c.regs();r.rax=r.rbx=r.rdx=r.rsi=r.rdi=r.rbp=0;r.rcx=input.at("agent");r.rsp=sp;r.rip=0x304350u;r.eflags&=~0x400u;request(PTRACE_SETREGS,c.pid,nullptr,&r);
    auto f=c.fp();f.cwd=input.at("floating_point").at("x87_control_word");f.swd=input.at("floating_point").at("x87_status");f.mxcsr=input.at("floating_point").at("mxcsr");
    if(mode){f.cwd=(f.cwd&~0xc00u)|(mode<<10);f.mxcsr=(f.mxcsr&~0x6000u)|(mode<<13);}
    f.ftw=0;std::memset(f.st_space,0,sizeof(f.st_space));request(PTRACE_SETFPREGS,c.pid,nullptr,&f);
    require(c.go({0x2f3140u,stop})==0x2f3140u,"Actual caller did not enter the factory dispatcher");
    require(c.go({0x2e4eb0u,stop})==0x2e4eb0u,"Actual class pair did not select2E4EB0");
}
}
int main(int argc,char**argv){J report{{"passed",false},{"retained_large_artifacts",J::array()},{"native_factory_uses_original_services",false}};try{
    require(argc==6,"Supply original ELF, typed input, region metadata, mode and result");
    std::ifstream in(argv[2]),regions_in(argv[3]);const auto input=J::parse(in),metadata=J::parse(regions_in);const auto mode=unsigned(std::stoul(argv[4]));
    Child original;enter_factory(original,argv[1],input,mode);const auto entry=original.regs();const auto before_fp=fp(original);
    const auto sp=std::uint32_t(entry.rsp),return_address=original.word(sp);
    const std::array<std::uint32_t,4>args{original.word(sp+4),original.word(sp+8),original.word(sp+12),original.word(sp+16)};
    require(original.word(original.word(original.word(args[0]))+0x14u)==0xa9520u&&original.word(original.word(original.word(args[1]))+0x14u)==0xab4b0u,"Actual source shape-class getters differ");
    struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;
    for(const auto&row:metadata.at("regions")){
        const auto p=row.at("address").get<std::uint32_t>(),n=row.at("size").get<std::uint32_t>();
        require(!(std::uint64_t(p)<std::uint64_t(NATIVE_DATA)+16384u&&NATIVE_DATA<std::uint64_t(p)+n),"Native-only scratch overlaps a genuine bank");
        banks.push_back({p,original.read(p,n),{}});
    }
    const auto second_payload=original.read(args[1],12),surface=original.read(original.word(args[1]),256);
    original.go({return_address});const auto expected_agent=std::uint32_t(original.regs().rax);const auto expected_fp=fp(original);const auto expected_object=original.read(expected_agent,256);
    for(auto&bank:banks)bank.after=original.read(bank.address,bank.before.size());J cases=J::array();
    for(const auto poison:{std::byte(0xa5),std::byte(0x5a)}){
        Child native;enter_factory(native,argv[1],input,mode);const auto current=native.regs();
        require(fp(native)==before_fp,"Independent native BEFORE FP differs");
        for(unsigned n=0;n<4;++n)require(native.word(current.rsp+4u+n*4u)==args[n],"Independent native formal DATA differs");
        require(native.read(args[1],12)==second_payload&&native.read(native.word(args[1]),256)==surface,"Independent native surface DATA differs");
        for(const auto&bank:banks)require(native.read(bank.address,bank.before.size())==bank.before,"Independent genuine bank BEFORE differs");
        FactoryMemory memory(native,poison);const NativeTu15ConvexAgentBindingScratch scratch{
            NATIVE_DATA,NATIVE_DATA+0x100u,NATIVE_DATA+0x200u,NATIVE_DATA+0x300u,NATIVE_DATA+0x400u,NATIVE_DATA+0x500u};
        NativeTu15PhysicsFrameFp state{before_fp[0],before_fp[1],before_fp[2]};NativeTu15ConvexAgentBindingActivity activity;
        const NativeTu15ConvexAgentArguments arguments{[&]{return args[0];},[&]{return args[1];},[&]{return args[3];}};
        const auto agent=native_tu15_create_bound_convex_contact_agent(memory,arguments,scratch,state,0,{},&activity);
        std::uint64_t compared{},different{};J differences=J::array();
        for(const auto&bank:banks){const auto actual=native.read(bank.address,bank.after.size());compared+=actual.size();for(std::size_t n=0;n<actual.size();++n)if(actual[n]!=bank.after[n]){++different;if(differences.size()<12)differences.push_back(J{{"address",bank.address+n},{"expected",std::to_integer<unsigned>(bank.after[n])},{"actual",std::to_integer<unsigned>(actual[n])}});}}
        const auto actual_fp=J::array({state.control_word,state.status,state.mxcsr});
        const bool passed=different==0&&agent.identity==expected_agent&&actual_fp==expected_fp&&native.read(agent.identity,256)==expected_object&&activity.completed==1&&activity.refusals==0&&activity.class_queries==1;
        cases.push_back(J{{"passed",passed},{"poison",std::to_integer<unsigned>(poison)},{"compared_bytes",compared},{"different_bytes",different},{"differences",differences},{"agent",agent.identity},{"expected_agent",expected_agent},{"before_fp",before_fp},{"after_fp",actual_fp},{"expected_fp",expected_fp},{"native_calls",activity.completed},{"native_refusals",activity.refusals},{"class_queries",activity.class_queries},{"pool_allocations",activity.pool_allocations},{"centers",activity.centers},{"supports",activity.supports},{"encodings",activity.encodings}});
    }
    report.update(J{{"passed",std::all_of(cases.begin(),cases.end(),[](const auto&c){return c.at("passed").template get<bool>();})},{"cases",cases},{"mode",mode},{"actual_formals",args},{"actual_shape_classes",J::array({23,24})},{"source_factory",0x2e4eb0u},{"captured_gp_used",false},{"source_after_used_as_native_input",false},{"ordered_operand_claimed",false}});
}catch(const std::exception&e){report["passed"]=false;report["error"]=e.what();std::cerr<<e.what()<<'\n';}
if(argc==6){std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;}
