#include "halo2/native_tu15_compound_process.hpp"
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
 check(argc==2&&std::filesystem::exists(argv[1]),"Required physical compound phase input absent");std::ifstream file(argv[1]);const auto fixture=J::parse(file);check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("required_backend")=="unchanged-tu15-native-compound-process-v1","Required original compound authority missing");check(fixture.at("body_sites")==J(40),"Original compound closure changed");std::size_t count{},writes{},reads{};
 for(const auto&c:fixture.at("cases")){
  const auto name=c.at("name").get<std::string>();Memory before;for(const auto&b:c.at("banks"))before.banks.emplace(b.at("address"),bytes(b.at("before")));const auto fault=c.value("fault",J{});if(!fault.is_null()){if(fault.value("access",std::string{})=="write")before.write_fault=fault;else before.fault=fault;}
  for(bool read_pass:{false,true}){
   if(read_pass&&c.contains("observer_fault"))continue;auto m=before;m.log_reads=read_pass;const bool fault_case=!fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&store){stores.push_back(store);if(c.contains("observer_fault")&&stores.size()==c.at("observer_fault"))throw ObserverFault{};};NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15CompoundProcessServices services;bool boundary_hit{};unsigned recursion{};
   services.process_child=[&](const NativeTu15CompoundChildProcessCall&call,NativeTu15PhysicsFrameFp&state){
    if(call.owner.identity==0x2f7a50u){
     check(++recursion<16,"compound recursion exceeded declared source cohort");
     native_tu15_process_compound_agent(m,call.receiver,{call.first.identity},call.second,call.configuration,call.output,{call.first.identity-52u},state,services,observer);
     --recursion;return;
    }
    const auto&b=c.at("boundary");
    check(!b.is_null()&&b.at("owner")==call.owner.identity&&b.at("receiver")==call.receiver.identity&&b.at("arguments")==J::array({call.first.identity,call.second.identity,call.configuration.identity,call.output.identity}),name+" actual compound dependency ABI differs");
    boundary_hit=true;throw std::runtime_error("actual-island-owner-boundary");
   };
   const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();bool failed{},observer_failed{};NativeTu15CompoundProcessStats stats;
   try{
    native_tu15_process_compound_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{c.at("proxy")},fp,services,observer,&stats);
   }catch(const Fault&){failed=true;}catch(const ObserverFault&){observer_failed=true;}catch(const std::runtime_error&e){if(std::string(e.what())!="actual-island-owner-boundary")throw;}
   check(failed==fault_case&&observer_failed==c.contains("observer_fault")&&boundary_hit==!c.at("boundary").is_null(),name+" compound fault/dependency completion differs");check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+" native compound leaked host FP");
   if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original compound read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?c.at("reads").at(n).dump():"end"));}reads+=m.reads.size();}
   else{if(fault_case)stores=m.raw_stores;const auto&expected=c.at("writes");check(stores.size()==expected.size(),name+" ordered compound store count differs "+std::to_string(stores.size())+"/"+std::to_string(expected.size()));for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=expected[n];check((fault_case||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered compound store differs at "+std::to_string(n)+" native="+hex(a.written)+" original="+b.at("written").get<std::string>());}writes+=stores.size();}
   for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained compound bytes differ");const auto&after=c.at("after_fp");check(fp.control_word==after[0]&&fp.status==after[1]&&fp.mxcsr==after[2],name+" complete compound FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+after.dump());
  }++count;
 }std::cout<<count<<" unchanged original compound/buffer/minimum cases, "<<reads<<" reads and "<<writes<<" stores passed without supplied allocation/collision outcomes; nonconverted callback returns and live composition remain separate\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
