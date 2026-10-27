#include "halo2/native_tu15_constraint_step.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte>bytes(const std::string&s){check(s.size()%2==0,"Invalid continuation fixture bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Fault{};
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;bool log_reads{},log_writes{};mutable J reads=J::array();J fault;mutable unsigned fault_count{};
    std::vector<SourcePhysicsContactRecordStore>raw_stores;
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing continuation bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing continuation extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
    void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());if(log_writes)raw_stores.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
}
int main(int argc,char**argv){try{
    check((argc==2||argc==3)&&std::filesystem::exists(argv[argc-1]),"Required genuine solver-return fixture absent");std::ifstream in(argv[argc-1]);const auto f=J::parse(in);
    check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-full-constraint-call-genuine-solver-return-v1","Required full original solver-return authority missing");
    check(f.at("root_sites")==306&&f.at("core_sites")==2208&&f.at("service_return_instruction")==0x3264e7,"Original root/core boundary changed");
    std::size_t count{},writes{},reads{},faults{};
    for(const auto&c:f.at("cases")){
        const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>()&&c.at("genuine_service_return").get<bool>(),name+" missing actual original solver execution");check(c.at("before_x87_tag")==0,name+" original service return needs a live x87 transport");Memory before;
        for(const auto&b:c.at("banks"))check(before.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate continuation bank");before.fault=c.value("fault",J{});
        const auto&a=c.at("arguments");NativeTu15PhysicsConstraintInput input{{a[0]},a[1],a[2],{a[3]},{a[4]},{a[5]},a[6],a[7],a[8]};
        const auto&state=c.at("continuation");NativeTu15ConstraintStepContinuation context{input,{state.at("first_accumulator")},state.at("body_end"),state.at("allocated_block"),state.at("arena_end")};
        for(bool read_pass:{false,true}){
            auto m=before;m.log_reads=read_pass;const bool fault_case=!m.fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;
            SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&s){stores.push_back(s);};NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
            bool failed{},boundary{};NativeTu15ConstraintStepServices services;
            auto stop=[&](const char*kind,std::uint32_t receiver,std::uint32_t owner,const J&args){const auto&e=c.at("boundary");check(e!=nullptr&&e.at("kind")==kind&&e.at("receiver")==receiver&&e.at("owner")==owner&&e.at("arguments")==args,name+" genuine post-core dependency ABI differs");boundary=true;throw std::runtime_error("original-continuation-boundary");};
            services.commit_motion=[&](auto motion,auto owner,auto step,auto parameter,auto accumulator,auto&)->NativeTu15ConstraintAccumulator{stop("commit",motion.identity,owner.identity,J::array({step.identity,parameter,accumulator.identity}));return{};};
            services.release_stack_block=[&](auto allocator,auto owner,auto pointer,auto size,auto&){stop("free",allocator.identity,owner.identity,J::array({pointer,size}));};
            const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();
            try{native_tu15_finish_island_constraints(m,context,c.at("actual_solver_result"),fp,0,services,observer);}catch(const Fault&){failed=true;}catch(const std::runtime_error&e){if(std::string(e.what())!="original-continuation-boundary")throw;}
            check(failed==fault_case&&boundary==!c.at("boundary").is_null(),name+" post-core boundary/fault completion differs");check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+" leaked host FP state");
            if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original continuation read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?expected[n].dump():"end"));}reads+=m.reads.size();}
            else{if(fault_case)stores=m.raw_stores;const auto&w=c.at("writes");check(stores.size()==w.size(),name+" ordered continuation store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&x=stores[n];const auto&y=w[n];check((fault_case||x.instruction==y.at("instruction"))&&x.address==y.at("address")&&hex(x.before)==y.at("before").get<std::string>()&&hex(x.written)==y.at("written").get<std::string>(),name+" ordered continuation store differs at "+std::to_string(n));}writes+=stores.size();}
            for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained post-core bytes differ");const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete continuation FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+end.dump());
        }++count;faults+=!c.value("fault",J{}).is_null();
    }check(count>0,"Empty genuine solver-return fixture");std::cout<<count<<" genuine original solver-return continuations, "<<writes<<" ordered stores, "<<reads<<" ordered reads and "<<faults<<" fault prefixes passed; core iteration/native-only closure remains open\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
