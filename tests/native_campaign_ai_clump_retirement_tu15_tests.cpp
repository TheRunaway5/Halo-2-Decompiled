#include "halo2/native_campaign_ai_clump_retirement_tu15.hpp"
#include "halo2/native_campaign_ai_clump_callbacks_tu15.hpp"
#include "halo2/native_campaign_ai_clump_retirement_tu15_binding.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using Json=nlohmann::json;constexpr std::uint32_t base=0x2000000u,size=0x20000u;
void check(bool v,const std::string &why){if(!v)throw std::runtime_error(why);}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
std::vector<std::byte> unhex(const std::string&s){std::vector<std::byte>b;for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return b;}
class Memory final:public SourcePhysicsContactMemory {
public:
 std::vector<std::byte> bank=std::vector<std::byte>(size);std::map<std::uint32_t,std::byte> globals;mutable Json events=Json::array();int fault=-1;
 std::byte get(std::uint32_t p)const{if(p>=base&&p<base+size)return bank[p-base];const auto i=globals.find(p);if(i==globals.end())throw std::runtime_error("Missing clump source input at"+std::to_string(p));return i->second;}
 void seed(std::uint32_t p,unsigned width,std::uint32_t v){for(unsigned i=0;i<width;++i)if(p+i>=base&&p+i<base+size)bank[p+i-base]=std::byte(v>>(i*8));else globals[p+i]=std::byte(v>>(i*8));}
 void read(std::uint32_t p,std::span<std::byte>b)const override{for(unsigned i=0;i<b.size();++i)b[i]=get(p+i);events.push_back(Json::array({"r",p,b.size(),hex(b)}));if(int(events.size())-1==fault)throw std::runtime_error("Declared original read boundary");}
 void write(std::uint32_t p,std::span<const std::byte>b)override{std::vector<std::byte>old;for(unsigned i=0;i<b.size();++i)old.push_back(get(p+i));events.push_back(Json::array({"w",p,b.size(),hex(old),hex(b)}));if(int(events.size())-1==fault)throw std::runtime_error("Declared original store boundary");check(p>=base&&std::uint64_t(p)+b.size()<=base+size,"Clump modified external input");std::copy(b.begin(),b.end(),bank.begin()+p-base);}
};
void seed(Memory &m,const Json &spec){for(const auto&w:spec.at("writes"))m.seed(w[0],w[1],w[2]);}
void replay(const Json &spec,const Json &result,const Json &whole,bool default_service=false,bool complete=false) {
 Memory m;seed(m,spec);auto expected=m.bank;const auto globals=m.globals;
 if(!result.at("fault_at").is_null())m.fault=result.at("fault_at");
 auto events=Json::array();const auto count=m.fault>=0?std::size_t(m.fault)+1u:whole.at("events").size();for(std::size_t i=0;i<count;++i)events.push_back(whole.at("events")[i]);
 Json boundary=Json::array(),calls=Json::array();std::string error;_mm_setcsr(0xbfc0u);
 NativeCampaignAiReferenceInvalidationServiceTu15 service;
 if(!default_service)service=[&](const NativeCampaignAiReferenceInvalidationTu15&r){Json request{{"callback",r.callback},{"actor",r.actor},{"behavior_record",r.behavior_record},{"reference",r.reference}};if(complete){calls.push_back(request);native_campaign_ai_clump_reference_invalidation_tu15(m,r);}else{boundary.push_back(request);throw std::runtime_error("Actual unrecovered behavior callback");}};
 try{native_campaign_ai_clump_retirement_tu15(m,spec.at("clump"),service);}catch(const std::exception&e){error=e.what();}
 const auto ambient=_mm_getcsr();const bool unresolved=!result.at("behavior_boundary").empty();
 check(!error.empty()==(m.fault>=0||unresolved),"Clump failure boundary differs: "+error);
 check(m.events==events,spec.at("name").get<std::string>()+": ordered clump events differ: "+m.events.dump());
 for(std::size_t i=0;i<count-(m.fault>=0?1u:0u);++i){const auto&e=events[i];if(e[0]=="w"){const auto b=unhex(e[4]);std::copy(b.begin(),b.end(),expected.begin()+e[1].get<std::uint32_t>()-base);}}
 check(m.bank==expected&&m.globals==globals,"Clump complete retained state differs");
 check(result.at("mxcsr")==spec.at("initial_mxcsr")&&ambient==0xbfc0u,"Integer teardown changed source/caller FP policy");
 if(default_service)check(error.find("1ea562 -> ")!=std::string::npos,"Missing real callback did not retain its exact native frontier");
 else check(boundary==result.at("behavior_boundary"),"Actual behavior invalidation request differs");
 if(complete){auto expected_calls=whole.at("behavior_calls");if(result.contains("behavior_call_count")){const auto n=result.at("behavior_call_count").get<std::size_t>();while(expected_calls.size()>n)expected_calls.erase(expected_calls.end()-1);}check(calls==expected_calls,"Original complete behavior callback order differs");}
}
void binding(const Json &spec,const Json &whole) {
 Memory m;seed(m,spec);constexpr std::uint32_t stack=0x3001000u;m.seed(stack,4,0x12345678u);m.seed(stack+4u,4,spec.at("clump"));
 SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;const auto before=abi;unsigned entries{};
 SourcePhysicsWorldRuntimeServices services;services.instruction=[&](std::uint32_t p){check(p==0x272ec0u,"Unexpected semantic boundary");++entries;};
 const auto activity=native_campaign_ai_clump_retirement_activity_tu15();
 native_campaign_ai_clump_retirement_tu15_run(m,0x272ec0u,abi,services);
 check(native_campaign_ai_clump_retirement_activity_tu15()==activity+1u,"Native clump diagnostic entry activity differs");
 check(abi.gp[4]==stack+8u&&entries==1,"Actual clump callee stack consumption differs");
 for(unsigned i=0;i<abi.gp.size();++i)if(i!=4)check(abi.gp[i]==before.gp[i],"Void clump ABI fabricated a volatile register result");
 auto expected=whole.at("events");expected.insert(expected.begin(),Json::array({"r",stack+4u,4,"000000e1"}));expected.push_back(Json::array({"r",stack,4,"78563412"}));check(m.events==expected,"Actual ABI clump argument/return accesses differ");
 check(native_campaign_ai_clump_retirement_tu15_owns_entry(0x272ec0u)&&!native_campaign_ai_clump_retirement_tu15_owns_entry(0x274800u),"Clump ownership overclaims an external child ABI");
 m.events=Json::array();abi.eflags|=0x400u;bool failed=false;try{native_campaign_ai_clump_retirement_tu15_run(m,0x272ec0u,abi);}catch(const std::invalid_argument&){failed=true;}check(failed&&m.events.empty(),"Unqualified backwards-string caller was admitted");
 failed=false;try{native_campaign_ai_clump_reference_invalidation_tu15(m,{0x12345678u,0,0,0});}catch(const std::runtime_error&e){failed=std::string(e.what()).find("1ea562 -> 12345678")!=std::string::npos;}check(failed&&m.events.empty(),"Unknown behavior selector supplied a successful result");
}
}
int main(int argc,char**argv){try{check(argc==2,"Supply independent original clump fixture");Json j;std::ifstream(argv[1])>>j;check(j.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong TU1.5 authority");check(j.at("instruction_patches").empty()&&j.at("supplied_dependency_returns").empty(),"Original result supplied");check(j.at("required_backend")=="physical-i386-complete-bank-fp+unicorn-ordered-source-events","Missing physical bank/FP authority");const bool complete=j.value("complete_callbacks",false);for(std::size_t i=0;i<j.at("specs").size();++i){const auto&s=j.at("specs")[i];const auto&r=j.at("scenarios")[i];replay(s,r,r,false,complete);if(!r.at("behavior_boundary").empty())replay(s,r,r,true);}for(const auto&r:j.at("faults")){const auto i=r.at("scenario").get<std::size_t>();replay(j.at("specs")[i],r,j.at("scenarios")[i],false,complete);}binding(j.at("specs")[0],j.at("scenarios")[0]);std::cout<<"PASS original clump teardown, actual intrusive/pool state, callback closure/frontier, source faults and native ABI\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
