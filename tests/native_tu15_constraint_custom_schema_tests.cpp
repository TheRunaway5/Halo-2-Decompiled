#include "halo2/native_tu15_constraint_custom_schema.hpp"
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
int main(int argc,char**argv){try{
    check(argc==2&&std::filesystem::exists(argv[1]),"Required original custom schema fixture absent");std::ifstream in(argv[1]);const auto f=J::parse(in);
    check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-custom-schema-three-owners-physical-fp-v1","Required custom schema authority missing");check(f.at("body_sites")==J::array({34,26,60}),"Original custom schema closure changed");
    std::size_t count{},writes{},reads{},faults{};
    for(const auto&c:f.at("cases")){
        const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" missing unchanged original");Memory before;for(const auto&b:c.at("banks"))check(before.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate custom schema bank");before.fault=c.value("fault",J{});before.write_fault=c.value("write_fault",J{});const bool observer_case=c.contains("observer_fault");
        const NativeTu15PhysicsVirtualOwner owner{c.at("entry")};check(native_tu15_owns_constraint_custom_schema(owner),name+" native schema owner missing");
        for(bool read_pass:{false,true}){
            if(read_pass&&observer_case)continue;auto m=before;m.log_reads=read_pass;const bool fault_case=!m.fault.is_null()||!m.write_fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&s){stores.push_back(s);if(observer_case&&stores.size()==c.at("observer_fault"))throw ObserverFault{};};NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
            const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();bool failed{},observer_failed{};NativeTu15ConstraintCustomSchemaStats stats;
            try{native_tu15_apply_constraint_custom_schema(m,owner,c.at("row"),c.at("payload"),fp,0,observer,&stats);}catch(const Fault&){failed=true;}catch(const ObserverFault&){observer_failed=true;}
            check(failed==fault_case&&observer_failed==observer_case,name+" custom schema fault completion differs");check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+" leaked host FP");check(stats.calls==1,name+" native owner did not run");
            if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original custom read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?c.at("reads").at(n).dump():"end"));}reads+=m.reads.size();}
            else{if(fault_case)stores=m.raw_stores;const auto&w=c.at("writes");check(stores.size()==w.size(),name+" ordered custom schema store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&x=stores[n];const auto&y=w[n];check((fault_case||x.instruction==y.at("instruction"))&&x.address==y.at("address")&&hex(x.before)==y.at("before").get<std::string>()&&hex(x.written)==y.at("written").get<std::string>(),name+" ordered custom schema store differs at "+std::to_string(n));}writes+=stores.size();}
            for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained custom bytes differ");const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete custom schema FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+end.dump());
        }++count;faults+=!c.value("fault",J{}).is_null();
    }check(count>0&&!native_tu15_owns_constraint_custom_schema({0xf1234567u}),"Custom schema owner whitelist is not bounded");std::cout<<count<<" physical original three-owner custom schemas, "<<writes<<" ordered stores, "<<reads<<" ordered reads and "<<faults<<" exact readfault prefixes passed; full solver/live composition remains separate\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
