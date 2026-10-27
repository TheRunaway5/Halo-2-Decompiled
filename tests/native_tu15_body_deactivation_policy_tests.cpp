#include "halo2/native_tu15_body_deactivation_policy.hpp"
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
std::vector<std::byte>bytes(const std::string&s){check(s.size()%2==0,"Invalid policy fixture bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Fault{};
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;bool log_reads{},log_writes{};mutable J reads=J::array();J fault;mutable unsigned fault_count{};
    std::vector<SourcePhysicsContactRecordStore>raw_stores;std::function<void(std::uint32_t,std::size_t)>on_write;
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing policy bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing policy extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
    void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
    void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());if(log_writes)raw_stores.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);if(on_write)on_write(p,b.size());}
    void put(std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8u*i));write(p,b);}
    std::uint32_t get(std::uint32_t p,unsigned n=4)const{std::array<std::byte,4>b{};read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(8u*i);return v;}
};
std::uint16_t host_status(){std::uint16_t n;asm volatile("fnstsw %0":"=a"(n));return n;}
std::uint16_t host_control(){std::uint16_t n;asm volatile("fnstcw %0":"=m"(n));return n;}
void models(){
    constexpr std::uint32_t base=0x1000000,policy=base+0x100,body=base+0x300,motion=base+0x500,output=base+0x900;
    Memory m;m.banks[base].resize(0x1000);m.put(body+0x3c,motion);m.put(policy+0x50,0x3f800000);m.put(policy+0x54,0x3f800000);m.put(policy+8,0x3f800000);m.put(motion+0xb0,0x40000000);m.put(motion+0x30,0x3f800000);
    m.on_write=[&](auto p,auto n){if(p==policy+0x20&&n==16){m.put(body+0x3c,motion+0x200);m.put(motion+0x2b0,0x42c60000);}};
    NativeTu15BodyDeactivationPolicyMemory storage(m,policy,{body},output);NativeTu15PhysicsFrameFp fp;
    check(!native_tu15_body_deactivation_policy(storage,NativeTu15DeactivationPeriod::short_period,fp,0),"Moving body policy must reject sleep");
    check(m.get(policy+0x20)==0x3f800000&&m.get(policy+0x10)==0x42c60000&&m.get(output,1)==0,"False path must retain quaternion then reread actual motion/position after publication");
    m.on_write={};m.put(body+0x3c,motion);m.put(policy+0x10,0x40000000);m.put(policy+0x20,0x3f800000);
    check(native_tu15_body_deactivation_policy(storage,NativeTu15DeactivationPeriod::short_period,fp,0)&&m.get(output,1)==1,"Unchanged body inside strict thresholds must permit sleep");
    auto unsupported=fp;unsupported.control_word&=~1u;bool rejected{};try{native_tu15_body_deactivation_policy(storage,NativeTu15DeactivationPeriod::short_period,unsupported,0);}catch(const std::runtime_error&){rejected=true;}check(rejected,"Unowned unmasked platform trap must be diagnosed");
    std::cout<<"Body policy live motion reread, retained quaternion snapshot, strict sleep result and explicit unmasked-trap models passed\n";
}
void oracle(const std::filesystem::path&path){
    check(std::filesystem::exists(path),"Required original body policy fixture missing");std::ifstream input(path);const auto f=J::parse(input);
    check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-body-policy-physical-operands-stores-full-fp-v1","Required body policy original authority absent");
    check(f.at("body_sites")==J::array({66,66}),"Original body policy closure changed");std::size_t count{},writes{},read_count{},faults{};
    for(const auto&c:f.at("cases")){
        const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" physical original not verified");Memory original;
        for(const auto&b:c.at("banks"))check(original.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate policy bank");original.fault=c.value("fault",J{});
        for(bool read_pass:{false,true}){
            auto m=original;m.log_reads=read_pass;const bool fault_case=!m.fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;
            SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&s){stores.push_back(s);};
            NativeTu15BodyDeactivationPolicyMemory storage(m,c.at("policy"),{c.at("body")},c.at("output"),observer);NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
            bool failed{},result{};const auto cw=host_control(),sw=host_status();const auto mxcsr=_mm_getcsr();
            try{result=native_tu15_body_deactivation_policy(storage,c.at("period")==0?NativeTu15DeactivationPeriod::short_period:NativeTu15DeactivationPeriod::long_period,fp,0);}catch(const Fault&){failed=true;}
            check(failed==fault_case,name+" read-fault prefix completion differs");check(host_control()==cw&&host_status()==sw&&_mm_getcsr()==mxcsr,name+" leaked host FP context");
            if(!failed)check(result==c.at("result").get<bool>(),name+" private output bool differs");
            if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original policy read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?expected[n].dump():"end"));}read_count+=m.reads.size();}
            else{if(fault_case)stores=m.raw_stores;const auto&w=c.at("writes");check(stores.size()==w.size(),name+" ordered body policy store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=w[n];check((fault_case||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered body policy store differs at "+std::to_string(n));}writes+=stores.size();}
            for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" complete retained policy bytes differ");const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete policy FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+end.dump());
        }++count;faults+=!c.value("fault",J{}).is_null();
    }check(count>0,"Empty physical original policy fixture");std::cout<<count<<" original body policy cases, "<<writes<<" ordered stores, "<<read_count<<" ordered reads and "<<faults<<" read-fault prefixes with complete FP passed\n";
}
}
int main(int argc,char**argv){try{if(argc==3&&std::string(argv[1])=="--oracle-only")oracle(argv[2]);else{models();if(argc==2)oracle(argv[1]);else check(argc==1,"Supply optional required original policy fixture");}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
