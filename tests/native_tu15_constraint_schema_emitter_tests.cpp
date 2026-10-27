#include "halo2/native_tu15_constraint_schema_emitter.hpp"
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
struct Fault{};struct ObserverFault{};
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;bool log_reads{},log_writes{};mutable J reads=J::array();J fault,write_fault;mutable unsigned fault_count{};unsigned write_fault_count{};std::vector<SourcePhysicsContactRecordStore>raw_stores;
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing solver bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing solver extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
    void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
    void write(std::uint32_t p,std::span<const std::byte>b)override{if(!write_fault.is_null()&&p==write_fault.at("address")&&b.size()==write_fault.at("bytes")&&++write_fault_count==write_fault.at("occurrence"))throw Fault{};auto i=at(p,b.size());if(log_writes)raw_stores.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
}
std::uint32_t word(Memory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;}
int main(int argc,char**argv){try{
    check(argc==2&&std::filesystem::exists(argv[1]),"Required actual original schema emitter input absent");std::ifstream in(argv[1]);const auto f=J::parse(in);check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-schema14-emitter-late-argument-alias-v1","Required schema emitter authority missing");check(f.at("body_sites")==15,"Original schema emitter owner changed");std::size_t count{},writes{},reads{},faults{},write_faults{},observer_faults{};
    for(const auto&c:f.at("cases")){
        const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" missing actual original");Memory before;for(const auto&b:c.at("banks"))check(before.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate schema emitter bank");before.fault=c.value("fault",J{});before.write_fault=c.value("write_fault",J{});const bool observer_case=c.contains("observer_fault");
        for(bool read_pass:{false,true}){
            if(read_pass&&observer_case)continue;auto m=before;m.log_reads=read_pass;const bool fault_case=!m.fault.is_null()||!m.write_fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&s){stores.push_back(s);if(observer_case&&stores.size()==c.at("observer_fault"))throw ObserverFault{};};
            const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();bool failed{},observer_failed{};std::uint32_t result{};NativeTu15ConstraintSchemaEmitterStats stats;
            try{const auto payload_bytes=word(m,0x200f008u);const NativeTu15ConstraintSchemaPacket packet{word(m,0x200f010u)};result=native_tu15_emit_custom_constraint_schema(m,payload_bytes,packet,[&]{return NativeTu15PhysicsVirtualOwner{word(m,0x200f004u)};},observer,&stats);}catch(const Fault&){failed=true;}catch(const ObserverFault&){observer_failed=true;}
            check(failed==fault_case&&observer_failed==observer_case,name+" emitter fault/dependency completion differs");check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+" integer emitter leaked FP");if(!failed&&!observer_failed)check(result==c.at("boundary_gp")[0],name+" actual payload return differs");
            if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original emitter read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?c.at("reads").at(n).dump():"end"));}reads+=m.reads.size();}
            else{if(fault_case)stores=m.raw_stores;const auto&w=c.at("writes");check(stores.size()==w.size(),name+" ordered emitter store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&x=stores[n];const auto&y=w[n];check((fault_case||x.instruction==y.at("instruction"))&&x.address==y.at("address")&&hex(x.before)==y.at("before").get<std::string>()&&hex(x.written)==y.at("written").get<std::string>(),name+" ordered emitter store differs at "+std::to_string(n));}writes+=stores.size();}
            for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained emitter bytes differ");
        }++count;faults+=c.contains("fault");write_faults+=c.contains("write_fault");observer_faults+=observer_case;
    }check(count>0,"Empty schema emitter proof");std::cout<<count<<" unchanged original schema14 emissions, "<<writes<<" ordered stores, "<<reads<<" ordered reads, "<<faults<<" readfaults, "<<write_faults<<" rejected stores and "<<observer_faults<<" committed observer exceptions passed, including actual argument/header aliases\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
