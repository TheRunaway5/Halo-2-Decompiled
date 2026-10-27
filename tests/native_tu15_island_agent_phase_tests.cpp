#include "halo2/native_tu15_island_agent_phase.hpp"
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
 check(argc==2&&std::filesystem::exists(argv[1]),"Required physical island agent phase input absent");std::ifstream file(argv[1]);const auto fixture=J::parse(file);check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("required_backend")=="unchanged-tu15-island-agent-phases-buffer-minimum-physical-v1","Required original island phase authority missing");check(fixture.at("body_sites")==J::array({97,205,51,55,63}),"Original island phase closure changed");std::size_t count{},writes{},reads{};
 for(const auto&c:fixture.at("cases")){
  const auto name=c.at("name").get<std::string>();Memory before;for(const auto&b:c.at("banks"))before.banks.emplace(b.at("address"),bytes(b.at("before")));const auto fault=c.value("fault",J{});if(!fault.is_null()){if(fault.value("access",std::string{})=="write")before.write_fault=fault;else before.fault=fault;}
  for(bool read_pass:{false,true}){
   if(read_pass&&c.contains("observer_fault"))continue;auto m=before;m.log_reads=read_pass;const bool fault_case=!fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&store){stores.push_back(store);if(c.contains("observer_fault")&&stores.size()==c.at("observer_fault"))throw ObserverFault{};};NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15IslandAgentPhaseServices services;std::size_t ticks{};bool boundary_hit{};
   const auto stop=[&](const char*kind,std::uint32_t owner,std::uint32_t receiver,const J&arguments){const auto&b=c.at("boundary");check(!b.is_null()&&b.at("kind")==kind&&b.at("owner")==owner&&b.at("receiver")==receiver&&b.at("arguments")==arguments,name+" actual island agent dependency ABI differs");boundary_hit=true;throw std::runtime_error("actual-island-owner-boundary");};
   services.timestamp_counter=[&]()->std::uint64_t{check(ticks<c.at("timestamps").size(),name+" missing actual original timestamp input");return c.at("timestamps").at(ticks++).at("value").get<std::uint64_t>();};
   services.allocate_stack_block=[&](auto allocator,auto owner,auto size,auto&)->std::uint32_t{stop("allocate",owner.identity,allocator.identity,J::array({size}));return 0;};
   services.release_stack_block=[&](auto allocator,auto owner,auto block,auto size,auto&){stop("release",owner.identity,allocator.identity,J::array({block,size}));};
   services.release_owned_contacts=[&](auto allocator,auto owner,auto block,auto size,auto memory_class,auto&){stop("owned",owner.identity,allocator.identity,J::array({block,size,memory_class}));};
   services.process_agent=[&](auto agent,auto owner,auto first,auto second,auto config,auto output,auto&){stop("process",owner.identity,agent.identity,J::array({first.identity,second.identity,config.identity,output.identity}));};
   services.finish_agent=[&](auto agent,auto owner,auto first,auto second,auto config,auto output,auto&){stop("finish",owner.identity,agent.identity,J::array({first.identity,second.identity,config.identity,output.identity}));};
   services.advance_contact_time=[&](auto island,auto config,auto entry,auto output,auto time,auto&){stop("advance",0x3205b0,island.identity,J::array({config.identity,entry.identity,output.identity,time}));};
   const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();bool failed{},observer_failed{};NativeTu15IslandAgentPhaseStats stats;NativeTu15IslandContactMinimum minimum{};
   try{switch(c.at("entry").get<std::uint32_t>()){
    case 0x321180:native_tu15_construct_island_contact_buffer(m,{c.at("header")},c.at("capacity"),fp,services,observer,&stats);break;
    case 0x321220:native_tu15_destroy_island_contact_buffer(m,{c.at("header")},fp,services,observer,&stats);break;
    case 0x321d20:native_tu15_collide_island_flag_clear(m,{c.at("island")},{c.at("configuration")},{{c.at("header")},{c.at("output")}},fp,services,observer,&stats);break;
    case 0x321a70:native_tu15_collide_island_flag_set(m,{c.at("island")},{c.at("configuration")},{{c.at("header")},{c.at("output")}},fp,0,services,observer,&stats);break;
    case 0x320510:minimum=native_tu15_minimum_island_contact_distance(m,{c.at("output")},fp,0);break;
    default:throw std::runtime_error("Unknown actual island phase fixture owner");
   }}catch(const Fault&){failed=true;}catch(const ObserverFault&){observer_failed=true;}catch(const std::runtime_error&e){if(std::string(e.what())!="actual-island-owner-boundary")throw;}
   check(failed==fault_case&&observer_failed==c.contains("observer_fault")&&boundary_hit==!c.at("boundary").is_null(),name+" island agent fault/dependency completion differs");check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+" native island phase leaked host FP");check(ticks==c.at("timestamps").size(),name+" genuine timestamp participation differs");if(c.at("entry")==0x320510&&!failed)check(hex(minimum.extended_value)==c.at("extended_result").get<std::string>(),name+" actual extended minimum return differs");
   if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original island agent read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?c.at("reads").at(n).dump():"end"));}reads+=m.reads.size();}
   else{if(fault_case)stores=m.raw_stores;const auto&expected=c.at("writes");check(stores.size()==expected.size(),name+" ordered island agent store count differs "+std::to_string(stores.size())+"/"+std::to_string(expected.size()));for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=expected[n];check((fault_case||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered island agent store differs at "+std::to_string(n)+" native="+hex(a.written)+" original="+b.at("written").get<std::string>());}writes+=stores.size();}
   for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained island agent bytes differ");const auto&after=c.at("after_fp");check(fp.control_word==after[0]&&fp.status==after[1]&&fp.mxcsr==after[2],name+" complete island agent FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+after.dump());
  }++count;
 }std::cout<<count<<" unchanged original island phase/buffer/minimum cases, "<<reads<<" reads and "<<writes<<" stores passed without supplied allocation/collision outcomes; nonconverted callback returns and live composition remain separate\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
