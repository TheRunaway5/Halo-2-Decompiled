#include "halo2/native_tu15_constraint_solver.hpp"
#include "halo2/native_tu15_constraint_solver_initializer.hpp"
#include "halo2/native_tu15_constraint_custom_schema.hpp"
#include "halo2/native_tu15_constraint_clock.hpp"
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
std::vector<std::byte>bytes(const std::string&s){check(s.size()%2==0,"Invalid solver fixture bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Fault{};
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;bool log_reads{},log_writes{};mutable J reads=J::array();J fault;mutable unsigned fault_count{};std::vector<SourcePhysicsContactRecordStore>raw_stores;
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing solver bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing solver extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
    void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
    void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());if(log_writes)raw_stores.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
}
int main(int argc,char**argv){try{
    check(argc==2&&std::filesystem::exists(argv[1]),"Required unchanged physical solver fixture absent");std::ifstream in(argv[1]);const auto f=J::parse(in);
    check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-whole-solver-native-clock-initializer-custom-composition-v1","Required physical solver authority absent");check(f.at("body_sites")==J::array({2208,4,5,152,125,4,22,43,1,34,26,60}),"Original solver/cursor closure changed");
    std::size_t count{},writes{},reads{},faults{},boundaries{};
    for(const auto&c:f.at("cases")){
        const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" missing physical original");Memory before;for(const auto&b:c.at("banks"))check(before.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate solver bank");before.fault=c.value("fault",J{});const auto ready=before.at(0x48d480u,1);check(ready->second.at(0x48d480u-ready->first)==std::byte{0},name+" original entry must start ready-zero");
        const auto&a=c.at("arguments");const NativeTu15ConstraintSolverInput input{a[0],a[1],{a[2]},a[3]};
        for(bool read_pass:{false,true}){
            auto m=before;m.log_reads=read_pass;const bool fault_case=!m.fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&s){stores.push_back(s);};NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
            NativeTu15ConstraintSolverServices services;bool boundary{},time_called{};NativeTu15ConstraintSolverInitializerServices initialization;NativeTu15ConstraintSolverInitializerStats initializer_stats;
            initialization.read_time_seconds=[&](auto&actual_fp){check(!time_called,name+" repeated initialized solver time call");time_called=true;const auto&t=c.at("actual_time");check(!t.is_null()&&t.at("genuine_original_return").get<bool>(),name+" missing genuine original time output");check(actual_fp.control_word==t.at("fp")[0]&&actual_fp.status==t.at("fp")[1]&&actual_fp.mxcsr==t.at("fp")[2],name+" actual integer clock FP differs");const auto raw=t.at("filetime").template get<std::string>();std::uint64_t ticks{};for(unsigned n=0;n<8;++n)ticks|=std::stoull(raw.substr(n*2,2),nullptr,16)<<(n*8u);return native_tu15_constraint_clock_seconds([&]{return ticks;});};
            initialization.log_failure=[&](std::uint32_t message,auto&){const auto&b=c.at("boundary");check(!b.is_null()&&b.at("kind")=="log"&&b.at("message")==message,name+" unexpected initialized solver logger boundary");boundary=true;throw std::runtime_error("original-solver-boundary");};
            services.initialize=[&](auto&actual_fp){native_tu15_initialize_constraint_solver(m,actual_fp,initialization,observer,&initializer_stats);};
            services.custom_schema=[&](auto owner,auto row,auto payload,auto&actual_fp){native_tu15_apply_constraint_custom_schema(m,owner,row,payload,actual_fp,0,observer);};
            const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();bool failed{};std::uint32_t result{};NativeTu15ConstraintSolverStats stats;
            try{result=native_tu15_solve_constraint_rows(m,input,fp,0,services,observer,&stats);}catch(const Fault&){failed=true;}catch(const std::runtime_error&e){if(std::string(e.what())!="original-solver-boundary")throw;}
            check(failed==fault_case&&boundary==!c.value("boundary",J{}).is_null(),name+" initialized solver fault/dependency completion differs");check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+" leaked host FP state");if(!failed&&!boundary)check(result==c.at("boundary_gp")[0],name+" actual solver result differs");check(stats.calls==1,name+" native solver root did not run");check(time_called==c.at("time_called").get<bool>(),name+" initialized solver time participation differs");if(!failed&&!boundary)check(initializer_stats.calls==1&&stats.initializations==1,name+" actual native initializer did not run");
            if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original solver read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?c.at("reads").at(n).dump():"end"));}reads+=m.reads.size();}
            else{if(fault_case)stores=m.raw_stores;const auto&w=c.at("writes");check(stores.size()==w.size(),name+" ordered solver store count differs "+std::to_string(stores.size())+"/"+std::to_string(w.size()));for(std::size_t n=0;n<stores.size();++n){const auto&x=stores[n];const auto&y=w[n];check((fault_case||x.instruction==y.at("instruction"))&&x.address==y.at("address")&&hex(x.before)==y.at("before").get<std::string>()&&hex(x.written)==y.at("written").get<std::string>(),name+" ordered solver store differs at "+std::to_string(n)+" native "+hex(x.written)+" original "+y.at("written").get<std::string>());}writes+=stores.size();}
            for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained solver bytes differ");const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete initialized solver FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+end.dump());
        }++count;faults+=!c.value("fault",J{}).is_null();boundaries+=!c.value("boundary",J{}).is_null();
    }check(count>0,"Empty original solver fixture");std::cout<<count<<" unchanged physical TU1.5 solver cases, "<<writes<<" ordered stores, "<<reads<<" ordered reads, "<<faults<<" fault prefixes and "<<boundaries<<" actual dependency entry stops passed through the native solver and native initializer; full live closure remains separate\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
