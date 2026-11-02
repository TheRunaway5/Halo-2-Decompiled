// Combined with the independent original-caller Child helpers. The original
// prefix supplies ordinary query formals and FP only. It never runs as a
// native child service. Native scratch starts wholly undefined.
#include "halo2/native_tu15_contact_query_binding.hpp"
#include "halo2/native_tu15_convex_simplex_solver_binding.hpp"
#include "halo2/native_tu15_contact_candidate_private_data.hpp"
namespace {
using namespace halo2;
constexpr std::uint32_t QUERY_DATA=0x74000000u;
struct QueryStorage final:SourcePhysicsContactMemory {
    Child&child;std::vector<std::byte>owned;
    QueryStorage(Child&c,std::byte poison):child(c),owned(0x8000u,poison){}
    bool local(std::uint32_t p,std::size_t n)const{return p>=QUERY_DATA&&std::uint64_t(p-QUERY_DATA)+n<=owned.size();}
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        if(local(p,b.size()))std::copy_n(owned.begin()+p-QUERY_DATA,b.size(),b.begin());
        else{const auto bytes=child.read(p,b.size());std::copy(bytes.begin(),bytes.end(),b.begin());}
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        if(local(p,b.size()))std::copy(b.begin(),b.end(),owned.begin()+p-QUERY_DATA);
        else child.write(p,b);
    }
};
void enter_query(Child&c,const char*exe,const J&input,unsigned mode){
    c.start(exe);constexpr std::uint32_t sp=0x7200f000,stop=0x70000080;
    const std::array<std::uint32_t,4>arguments{input.at("first"),input.at("second"),input.at("configuration"),input.at("output")};
    c.put(sp,stop);for(unsigned n=0;n<4;++n)c.put(sp+4u+n*4u,arguments[n]);
    auto r=c.regs();r.rax=r.rbx=r.rdx=r.rsi=r.rdi=r.rbp=0;r.rcx=input.at("agent");r.rsp=sp;r.rip=0x304350u;r.eflags&=~0x400u;request(PTRACE_SETREGS,c.pid,nullptr,&r);
    auto f=c.fp();f.cwd=input.at("floating_point").at("x87_control_word");f.swd=input.at("floating_point").at("x87_status");f.mxcsr=input.at("floating_point").at("mxcsr");
    if(mode){f.cwd=(f.cwd&~0xc00u)|(mode<<10);f.mxcsr=(f.mxcsr&~0x6000u)|(mode<<13);}
    f.ftw=0;std::memset(f.st_space,0,sizeof(f.st_space));request(PTRACE_SETFPREGS,c.pid,nullptr,&f);
    const auto ordinal=input.value("declared_query_ordinal",0u);require(ordinal<3,"Query ordinal exceeds the genuine3-child cohort");
    for(unsigned n=0;n<=ordinal;++n){
        require(c.go({0x2e4fe0u,stop})==0x2e4fe0u,"Genuine304 did not enter requested query");
        if(n!=ordinal)c.go({c.word(std::uint32_t(c.regs().rsp))});
    }
}
}
int main(int argc,char**argv){J report{{"passed",false},{"native_original_executor_calls",0},{"retained_large_artifacts",J::array()}};try{
    require(argc==6,"Supply original ELF, typed input, regions, mode and result");
    std::ifstream in(argv[2]),regions_in(argv[3]);const auto input=J::parse(in),metadata=J::parse(regions_in);const auto mode=unsigned(std::stoul(argv[4]));
    Child original;enter_query(original,argv[1],input,mode);const auto entry=original.regs();const auto before_fp=fp(original);
    const auto sp=std::uint32_t(entry.rsp),ret=original.word(sp),receiver=std::uint32_t(entry.rcx);
    std::array<std::uint32_t,5>args{};for(unsigned n=0;n<5;++n)args[n]=original.word(sp+4u+n*4u);
    struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;
    std::vector<NativeTu15ContactPrivateRange>retained;
    for(const auto&row:metadata.at("regions")){
        const auto p=row.at("address").get<std::uint32_t>(),n=row.at("size").get<std::uint32_t>();
        require(!(std::uint64_t(p)<std::uint64_t(QUERY_DATA)+0x8000u&&QUERY_DATA<std::uint64_t(p)+n),"Scratch aliases capture");
        banks.push_back({p,original.read(p,n),{}});retained.push_back({p,n});
    }
    const auto first_payload=original.read(args[1],12),second_payload=original.read(args[2],12);
    original.go({ret});require(original.regs().rsp==sp+24u,"Original query RET20 differs");
    const auto expected_fp=fp(original);const auto valid=original.word(args[0],1),expected_result=std::uint32_t(original.regs().rax);
    const auto expected_output=original.read(args[4],0x50u);
    for(auto&bank:banks)bank.after=original.read(bank.address,bank.before.size());J cases=J::array();
    for(const auto poison:{std::byte(0xa5),std::byte(0x5a)}){
        Child child;enter_query(child,argv[1],input,mode);const auto current=child.regs();require(fp(child)==before_fp,"Independent BEFORE FP differs");
        require(std::uint32_t(current.rcx)==receiver,"Independent query receiver differs");
        for(unsigned n=0;n<5;++n)require(child.word(current.rsp+4u+n*4u)==args[n],"Independent query formal differs");
        require(child.read(args[1],12)==first_payload&&child.read(args[2],12)==second_payload,"Independent input payload differs");
        for(const auto&bank:banks)require(child.read(bank.address,bank.before.size())==bank.before,"Independent bank BEFORE differs");
        QueryStorage storage(child,poison);
        const std::array<NativeTu15ContactPrivateRange,3>owned{{{QUERY_DATA,0x8000u},{args[0],1u},{args[4],0x50u}}};
        NativeTu15ContactCandidatePrivateData defined(storage,owned,retained);
        NativeTu15ConvexSimplexMemory view(defined);NativeTu15PhysicsFrameFp state{before_fp[0],before_fp[1],before_fp[2]};
        const auto p=QUERY_DATA;
        const NativeTu15ContactShapeScratch support_child{p+0x3600u,p+0x3610u,nullptr},support{p+0x3500u,p+0x3510u,&support_child};
        NativeTu15ConvexSimplexBindingActivity solver;
        const auto geometry=native_tu15_make_bound_convex_contact_services(view,
            {{p+0x3800u,p+0x3804u,p+0x3808u,p+0x3810u,p+0x3820u,
                {p+0x3000u,p+0x3100u,p+0x3140u,{p+0x2800u,{p+0x2600u,p+0x2604u,
                    {p+0x2000u,p+0x2010u,p+0x2030u,p+0x2060u,p+0x2020u}}}}},support},0,{},&solver);
        const NativeTu15ContactShapeScratch cache_child{p+0x7120u,p+0x7130u,nullptr},cache_support{p+0x7100u,p+0x7110u,&cache_child};
        const NativeTu15ContactQueryBindingScratch scratch{{p},{p+0x1000u,p+0x1300u},
            {{p+0x7000u},cache_support},{{p+0x7040u},cache_support}};
        NativeTu15PairDeletionNativeScope scope;NativeTu15ContactQueryBindingActivity activity;
        const auto result=native_tu15_query_bound_contact(view,{receiver},args[0],{args[1]},{args[2]},
            {args[3]},args[4],scratch,state,0,geometry,scope,{},&activity);
        std::uint64_t compared{},different{},output_compared{},output_different{};J differences=J::array();
        for(const auto&bank:banks){const auto actual=child.read(bank.address,bank.after.size());compared+=actual.size();for(std::size_t n=0;n<actual.size();++n)if(actual[n]!=bank.after[n]){++different;if(differences.size()<12)differences.push_back(J{{"address",bank.address+n},{"expected",std::to_integer<unsigned>(bank.after[n])},{"actual",std::to_integer<unsigned>(actual[n])}});}}
        std::array<std::byte,1>actual_valid{};defined.read(args[0],actual_valid);
        for(unsigned n=0;n<0x50u;++n)if(defined.data_is_defined(args[4]+n,1)){
            std::array<std::byte,1>b{};defined.read(args[4]+n,b);++output_compared;
            output_different+=b[0]!=expected_output[n];
        }
        const auto actual_fp=J::array({state.control_word,state.status,state.mxcsr});
        const bool passed=!different&&!output_different&&result==expected_result&&std::to_integer<unsigned>(actual_valid[0])==valid&&actual_fp==expected_fp&&activity.completed==1&&activity.refusals==0&&!scope.active();
        cases.push_back(J{{"passed",passed},{"poison",std::to_integer<unsigned>(poison)},{"compared_bytes",compared},{"different_bytes",different},{"differences",differences},{"valid",valid},{"output_defined_bytes_compared",output_compared},{"output_different_bytes",output_different},{"fp",actual_fp},{"expected_fp",expected_fp},{"native_query",activity.completed},{"native_solver",solver.completed},{"unprovided_reads",view.refused_reads()}});
    }
    report.update(J{{"passed",std::all_of(cases.begin(),cases.end(),[](const auto&c){return c.at("passed").template get<bool>();})},{"cases",cases},{"mode",mode},{"formals",args},{"receiver",receiver},{"ordered_operand_claimed",false},{"raw_private_preimages_claimed",false}});
}catch(const std::exception&e){report["passed"]=false;report["error"]=e.what();std::cerr<<e.what()<<'\n';}
if(argc==6){std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;}
