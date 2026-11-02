// Uses only typed304350 formals/FP and a native-owned poisoned packet.
#include "halo2/native_tu15_contact_producer.hpp"
namespace {
using namespace halo2;
constexpr std::uint32_t LOCAL=0x74000000u;
struct ReachedCandidate{};
struct PrefixMemory final:SourcePhysicsContactMemory {
    Child&child;std::vector<std::byte>owned;mutable std::vector<std::uint32_t>reads;
    PrefixMemory(Child&c,std::byte poison):child(c),owned(4096,poison){}
    bool local(std::uint32_t p,std::size_t n)const{return p>=LOCAL&&std::uint64_t(p-LOCAL)+n<=owned.size();}
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        if(local(p,b.size()))std::copy_n(owned.begin()+p-LOCAL,b.size(),b.begin());
        else{reads.push_back(p);const auto bytes=child.read(p,b.size());std::copy(bytes.begin(),bytes.end(),b.begin());}
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        if(local(p,b.size()))std::copy(b.begin(),b.end(),owned.begin()+p-LOCAL);
        else child.write(p,b);
    }
};
void enter_candidate(Child&c,const char*exe,const J&input,unsigned mode){
    c.start(exe);constexpr std::uint32_t sp=0x7200f000,stop=0x70000080;
    const std::array<std::uint32_t,4>args{input.at("first"),input.at("second"),input.at("configuration"),input.at("output")};
    c.put(sp,stop);for(unsigned n=0;n<4;++n)c.put(sp+4u+n*4u,args[n]);
    auto r=c.regs();r.rax=r.rbx=r.rdx=r.rsi=r.rdi=r.rbp=0;r.rcx=input.at("agent");r.rsp=sp;r.rip=0x304350u;r.eflags&=~0x400u;request(PTRACE_SETREGS,c.pid,nullptr,&r);
    auto f=c.fp();f.cwd=input.at("floating_point").at("x87_control_word");f.swd=input.at("floating_point").at("x87_status");f.mxcsr=input.at("floating_point").at("mxcsr");
    if(mode){f.cwd=(f.cwd&~0xc00u)|(mode<<10);f.mxcsr=(f.mxcsr&~0x6000u)|(mode<<13);}f.ftw=0;std::memset(f.st_space,0,sizeof(f.st_space));request(PTRACE_SETFPREGS,c.pid,nullptr,&f);
    require(c.word(0x48c974u)>=c.word(0x48c978u),"Selected prefix requires its actual inactive profiler");
    require(c.go({0x2f36f0u,stop})==0x2f36f0u,"Original304 did not reach2F36F0");
}
}
int main(int argc,char**argv){J report{{"passed",false}};try{
    require(argc==6,"Supply original ELF, typed input, metadata, FP mode and result");std::ifstream in(argv[2]),meta_in(argv[3]);const auto input=J::parse(in),metadata=J::parse(meta_in);const auto mode=unsigned(std::stoul(argv[4]));
    Child original;enter_candidate(original,argv[1],input,mode);const auto r=original.regs();const auto actual_second=original.word(r.rsp+8u);const auto expected_packet=original.read(actual_second,16);const auto expected_fp=fp(original);
    const auto second=input.at("second").get<std::uint32_t>(),shape=original.word(second);
    const std::vector<std::uint32_t>expected_reads{0x48c974u,0x48c978u,second+8u,second,shape+0xcu,second+4u};
    const std::vector<std::uint32_t>expected_sites{0x3043b1u,0x3043b5u,0x3043c4u,0x3043d0u};
    J cases=J::array();
    for(auto poison:{std::byte(0xa5),std::byte(0x5a)}){
        // Original initialized payload fields are comparison outputs only.
        // Native starts directly from the authenticated304 BEFORE banks.
        Child native;native.start(argv[1]);PrefixMemory memory(native,poison);
        NativeTu15PhysicsFrameFp current{input.at("floating_point").at("x87_control_word"),input.at("floating_point").at("x87_status"),input.at("floating_point").at("mxcsr")};
        if(mode){current.control_word=(current.control_word&~0xc00u)|(mode<<10);current.mxcsr=(current.mxcsr&~0x6000u)|(mode<<13);}
        NativeTu15ContactProducerServices services;std::vector<std::uint32_t>sites;std::vector<std::byte>packet;unsigned calls{};
        services.update_candidates=[&](auto producer,auto first,auto temporary,auto context,auto factory,auto&state){
            require(producer.identity==input.at("agent")&&first.identity==input.at("first")&&context.identity==input.at("configuration")&&factory==0x2f3140u,"Native candidate formals differ");
            packet.resize(16);memory.read(temporary.identity,packet);++calls;require(J::array({state.control_word,state.status,state.mxcsr})==expected_fp,"Native prefix FP differs");throw ReachedCandidate{};
        };
        bool reached=false;
        try{native_tu15_produce_contacts(memory,{input.at("agent")},{input.at("first")},{second},{input.at("configuration")},{input.at("output")},{LOCAL},current,0,services,[&](const auto&store){sites.push_back(store.instruction);});}catch(const ReachedCandidate&){reached=true;}
        std::uint64_t compared{},different{};for(const auto&row:metadata.at("regions")){const auto p=row.at("address").get<std::uint32_t>(),n=row.at("size").get<std::uint32_t>();const auto a=native.read(p,n),b=original.read(p,n);compared+=n;for(std::size_t k=0;k<n;++k)different+=a[k]!=b[k];}
        const bool passed=reached&&calls==1&&packet==expected_packet&&sites==expected_sites&&memory.reads==expected_reads&&different==0&&J::array({current.control_word,current.status,current.mxcsr})==expected_fp;
        cases.push_back(J{{"passed",passed},{"poison",std::to_integer<unsigned>(poison)},{"compared_bytes",compared},{"different_bytes",different},{"packet",hex(packet)},{"expected_packet",hex(expected_packet)},{"store_sites",sites},{"expected_store_sites",expected_sites},{"reads",memory.reads},{"expected_reads",expected_reads},{"fp",J::array({current.control_word,current.status,current.mxcsr})},{"expected_fp",expected_fp}});
    }
    report.update(J{{"passed",std::all_of(cases.begin(),cases.end(),[](const auto&v){return v.at("passed").template get<bool>();})},{"cases",cases},{"mode",mode},{"source_after_used_as_native_input",false},{"captured_gp_used",false},{"remaining_child_executed_by_native",false},{"scope","Actual304 prefix to genuine2F entry; initialized16B payload/read/store phase/FP/allcapturedbanks. No raw private-preimage/wholeproducer/nativeWorld claim."}});
}catch(const std::exception&e){report["passed"]=false;report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc==6){std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;}
