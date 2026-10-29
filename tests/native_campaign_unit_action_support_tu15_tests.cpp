#include "halo2/native_campaign_unit_action_support_tu15.hpp"
#include "halo2/native_campaign_unit_action_support_tu15_binding.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using Json=nlohmann::json;constexpr std::uint32_t base=0x2000000u,size=0x12000u;
void check(bool v,const std::string &why){if(!v)throw std::runtime_error(why);}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
std::vector<std::byte> unhex(const std::string&s){std::vector<std::byte>b;for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return b;}
class Memory final:public SourcePhysicsContactMemory {
public:
 std::vector<std::byte> bank=std::vector<std::byte>(size);std::map<std::uint32_t,std::byte> globals;mutable Json events=Json::array();int fault=-1;
 std::byte get(std::uint32_t p)const{if(p>=base&&p<base+size)return bank[p-base];const auto i=globals.find(p);if(i==globals.end())throw std::runtime_error("Missing unit action source input at"+std::to_string(p));return i->second;}
 void seed(std::uint32_t p,unsigned width,std::uint32_t v){for(unsigned i=0;i<width;++i)if(p+i>=base&&p+i<base+size)bank[p+i-base]=std::byte(v>>(i*8));else globals[p+i]=std::byte(v>>(i*8));}
 void read(std::uint32_t p,std::span<std::byte>b)const override{for(unsigned i=0;i<b.size();++i)b[i]=get(p+i);events.push_back(Json::array({"r",p,b.size(),hex(b)}));if(int(events.size())-1==fault)throw std::runtime_error("Declared original read boundary");}
 void write(std::uint32_t p,std::span<const std::byte>b)override{std::vector<std::byte>old;for(unsigned i=0;i<b.size();++i)old.push_back(get(p+i));events.push_back(Json::array({"w",p,b.size(),hex(old),hex(b)}));if(int(events.size())-1==fault)throw std::runtime_error("Declared original store boundary");check(p>=base&&std::uint64_t(p)+b.size()<=base+size,"Unit action modified external input");std::copy(b.begin(),b.end(),bank.begin()+p-base);}
};
void seed(Memory &m,const Json &spec){for(const auto&w:spec.at("writes"))m.seed(w[0],w[1],w[2]);}
constexpr std::uint32_t stack=0x203f000u;
std::uint32_t input(Memory&m,std::uint32_t p,unsigned n){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8u);return v;}
void execute(Memory&m,const Json&s,Json&calls){
 NativeCampaignUnitActionArgumentsTu15 arguments{[&]{return std::uint16_t(input(m,stack+4u,2));},[&]{return std::uint8_t(input(m,stack+8u,1));}};
 const auto entry=s.at("entry").get<std::uint32_t>();
 if(entry==0xb9390u)native_campaign_unit_mark_update_tu15(m,s.at("eax"));
 else if(entry==0xe8f10u)native_campaign_unit_interrupt_action_tu15(m,s.at("ecx"),s.at("edi"),[&](const NativeCampaignUnitInterruptCallTu15&c){calls.push_back(Json{{"callback",c.callback},{"unit",c.unit},{"type",c.type},{"event_index",m.events.size()}});});
 else if(entry==0x1efa70u)native_campaign_unit_notify_action_tu15(m,s.at("eax"),arguments);
 else native_campaign_unit_record_action_tu15(m,s.at("eax"),arguments);
}
void replay(const Json&spec,const Json&result,const Json&whole){
 Memory m;seed(m,spec);auto expected=m.bank;const auto globals=m.globals;if(!result.at("fault_at").is_null())m.fault=result.at("fault_at");
 auto events=Json::array();const auto count=m.fault>=0?std::size_t(m.fault)+1u:whole.at("events").size();for(std::size_t i=0;i<count;++i)events.push_back(whole.at("events")[i]);Json calls=Json::array();std::string error;_mm_setcsr(0xbfc0u);
 try{execute(m,spec,calls);}catch(const std::exception&e){error=e.what();}const auto ambient=_mm_getcsr();const bool frontier=!result.at("frontier").empty();
 check(!error.empty()==(m.fault>=0||frontier),"Unit action support failure boundary differs");check(m.events==events,spec.at("name").get<std::string>()+": ordered unit action support events differ: "+m.events.dump());check(calls==result.at("calls"),"Actual interrupt callback order differs");if(frontier)check(error.find("e8f41 -> ")!=std::string::npos,"Unknown interrupt callback supplied completion");
 for(std::size_t i=0;i<count-(m.fault>=0?1u:0u);++i){const auto&e=events[i];if(e[0]=="w"){const auto b=unhex(e[4]);std::copy(b.begin(),b.end(),expected.begin()+e[1].get<std::uint32_t>()-base);}}
 check(m.bank==expected&&m.globals==globals,"Unit action complete retained state differs");check(result.at("mxcsr")==spec.at("initial_mxcsr")&&ambient==0xbfc0u,"Integer unit action support changed source/caller FP status");
}
void binding(const Json&spec,const Json&result,const Json&whole,bool return_fault=false){
 Memory m;seed(m,spec);m.seed(stack,4,0x12345678u);auto expected_bank=m.bank;const auto globals=m.globals;SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.gp[0]=spec.at("eax");abi.gp[1]=spec.at("ecx");abi.gp[7]=spec.at("edi");const auto before=abi;const auto entry=spec.at("entry").get<std::uint32_t>();
 if(!result.at("fault_at").is_null())m.fault=result.at("fault_at");else if(return_fault)m.fault=whole.at("events").size();
 const auto activity=native_campaign_unit_action_support_activity_tu15();const bool admitted=native_campaign_unit_action_support_tu15_owns_call(entry,abi);check(m.events.empty()&&abi.gp==before.gp,"Production admission peeked at source memory or changed inputs");
 if(!admitted){check(!whole.at("frontier").empty()&&entry==0xe8f10u,"Proved profile rejected by production admission");const auto after=native_campaign_unit_action_support_activity_tu15();check(after.marks+after.interrupts+after.notifications+after.records==activity.marks+activity.interrupts+activity.notifications+activity.records,"Unqualified profile was executed");check(m.bank==expected_bank&&m.globals==globals,"Unqualified profile supplied action state");return;}
 auto events=Json::array();const auto count=!result.at("fault_at").is_null()?std::size_t(m.fault)+1u:whole.at("events").size();for(std::size_t i=0;i<count;++i)events.push_back(whole.at("events")[i]);if(result.at("fault_at").is_null())events.push_back(Json::array({"r",stack,4,"78563412"}));
 _mm_setcsr(0xbfc0u);bool failed=false;try{native_campaign_unit_action_support_tu15_run(m,entry,abi);}catch(const std::exception&){failed=true;}const auto ambient=_mm_getcsr();const auto after=native_campaign_unit_action_support_activity_tu15();check(after.marks+after.interrupts+after.notifications+after.records==activity.marks+activity.interrupts+activity.notifications+activity.records+1u,"Native support entry diagnostic differs");check(failed==(m.fault>=0),"Production support failure boundary differs");
 check(m.events==events,"Production admission+run lazy source events differ: "+m.events.dump());const auto consumed=entry==0x1efa70u||entry==0x1eec30u?12u:4u;check(abi.gp[4]==stack+(failed?0u:consumed),"Production action support stack consumption differs");for(unsigned i=0;i<abi.gp.size();++i)if(i!=4)check(abi.gp[i]==before.gp[i],"Void unit support fabricated dead volatile registers");
 for(std::size_t i=0;i<events.size()-(failed?1u:0u);++i){const auto&e=events[i];if(e[0]=="w"){const auto b=unhex(e[4]);std::copy(b.begin(),b.end(),expected_bank.begin()+e[1].get<std::uint32_t>()-base);}}
 check(m.bank==expected_bank&&m.globals==globals&&ambient==0xbfc0u,"Production support changed unexpected bank/FP state");check(!native_campaign_unit_action_support_tu15_owns_call(0xe8e10u,abi)&&!native_campaign_unit_action_support_tu15_owns_call(0xe8e50u,abi),"Unclosed action begin was admitted");
}

}
int main(int argc,char**argv){try{check(argc==2,"Supply independent original unit action support fixture");Json j;std::ifstream(argv[1])>>j;check(j.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong TU1.5 authority");check(j.at("instruction_patches").empty()&&j.at("supplied_dependency_returns").empty(),"Original unit action outcome supplied");check(j.at("required_backend")=="physical-i386-complete-bank-fp+unicorn-ordered-source-events","Missing physical authority");for(std::size_t i=0;i<j.at("specs").size();++i){const auto&s=j.at("specs")[i];const auto&r=j.at("scenarios")[i];replay(s,r,r);binding(s,r,r);if(i%17==0&&r.at("frontier").empty())binding(s,r,r,true);}for(const auto&r:j.at("faults")){const auto i=r.at("scenario").get<std::size_t>();replay(j.at("specs")[i],r,j.at("scenarios")[i]);binding(j.at("specs")[i],r,j.at("scenarios")[i]);}std::cout<<"PASS actual object marking, action interruption, local notifications, lazy source faults and ABI\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
