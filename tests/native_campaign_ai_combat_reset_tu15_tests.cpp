#include "halo2/native_campaign_ai_combat_reset_tu15.hpp"
#include "halo2/native_campaign_ai_combat_reset_tu15_binding.hpp"
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
 std::byte get(std::uint32_t p)const{if(p>=base&&p<base+size)return bank[p-base];const auto i=globals.find(p);if(i==globals.end())throw std::runtime_error("Missing combat source input at"+std::to_string(p));return i->second;}
 void seed(std::uint32_t p,unsigned width,std::uint32_t v){for(unsigned i=0;i<width;++i)if(p+i>=base&&p+i<base+size)bank[p+i-base]=std::byte(v>>(i*8));else globals[p+i]=std::byte(v>>(i*8));}
 void read(std::uint32_t p,std::span<std::byte>b)const override{for(unsigned i=0;i<b.size();++i)b[i]=get(p+i);events.push_back(Json::array({"r",p,b.size(),hex(b)}));if(int(events.size())-1==fault)throw std::runtime_error("Declared original read boundary");}
 void write(std::uint32_t p,std::span<const std::byte>b)override{std::vector<std::byte>old;for(unsigned i=0;i<b.size();++i)old.push_back(get(p+i));events.push_back(Json::array({"w",p,b.size(),hex(old),hex(b)}));if(int(events.size())-1==fault)throw std::runtime_error("Declared original store boundary");check(p>=base&&std::uint64_t(p)+b.size()<=base+size,"Combat reset modified external input");std::copy(b.begin(),b.end(),bank.begin()+p-base);}
};
void seed(Memory &m,const Json &spec){for(const auto&w:spec.at("writes"))m.seed(w[0],w[1],w[2]);}
void replay(const Json&spec,const Json&result,const Json&whole){
 Memory m;seed(m,spec);auto expected=m.bank;const auto globals=m.globals;if(!result.at("fault_at").is_null())m.fault=result.at("fault_at");
 auto events=Json::array();const auto count=m.fault>=0?std::size_t(m.fault)+1u:whole.at("events").size();for(std::size_t i=0;i<count;++i)events.push_back(whole.at("events")[i]);
 _mm_setcsr(0xbfc0u);bool failed=false;try{native_campaign_ai_combat_reset_tu15(m,spec.at("actor"),spec.at("mode").get<std::int16_t>());}catch(const std::exception&){failed=true;}const auto ambient=_mm_getcsr();
 check(failed==(m.fault>=0),"Combat reset failure boundary differs");check(m.events==events,spec.at("name").get<std::string>()+": ordered combat reset events differ: "+m.events.dump());
 for(std::size_t i=0;i<count-(m.fault>=0?1u:0u);++i){const auto&e=events[i];if(e[0]=="w"){const auto b=unhex(e[4]);std::copy(b.begin(),b.end(),expected.begin()+e[1].get<std::uint32_t>()-base);}}
 check(m.bank==expected&&m.globals==globals,"Combat reset complete retained state differs");check(result.at("mxcsr")==spec.at("initial_mxcsr")&&ambient==0xbfc0u,"Combat reset changed source/caller FP state");
}
void binding(const Json&spec,const Json&whole){
 Memory m;seed(m,spec);constexpr std::uint32_t stack=0x3001000u;m.seed(stack,4,0x12345678u);SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.gp[0]=spec.at("actor");abi.gp[7]=0x56780000u|std::uint16_t(spec.at("mode").get<std::int16_t>());const auto before=abi;
 const auto activity=native_campaign_ai_combat_reset_activity_tu15();native_campaign_ai_combat_reset_tu15_run(m,0x1ec7b0u,abi);check(native_campaign_ai_combat_reset_activity_tu15()==activity+1u,"Combat reset diagnostic entry activity differs");check(abi.gp[4]==stack+4u,"Combat reset callee consumed an invented argument");for(unsigned i=0;i<abi.gp.size();++i)if(i!=4)check(abi.gp[i]==before.gp[i],"Combat reset fabricated volatile register results");
 auto expected=whole.at("events");expected.push_back(Json::array({"r",stack,4,"78563412"}));check(m.events==expected,"Combat reset register argument/RET access boundary differs");
 m.events=Json::array();abi.eflags|=0x400u;bool failed=false;try{native_campaign_ai_combat_reset_tu15_run(m,0x1ec7b0u,abi);}catch(const std::invalid_argument&){failed=true;}check(failed&&m.events.empty(),"Backwards-string combat reset caller was admitted");check(native_campaign_ai_combat_reset_tu15_owns_entry(0x1ec7b0u)&&!native_campaign_ai_combat_reset_tu15_owns_entry(0x266490u),"Combat reset overclaims a child ABI");
}
}
int main(int argc,char**argv){try{check(argc==2,"Supply independent original combat reset fixture");Json j;std::ifstream(argv[1])>>j;check(j.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong TU1.5 authority");check(j.at("instruction_patches").empty()&&j.at("supplied_dependency_returns").empty(),"Original reset result supplied");check(j.at("required_backend")=="physical-i386-complete-bank-fp+unicorn-ordered-source-events","Missing physical authority");for(std::size_t i=0;i<j.at("specs").size();++i){const auto&s=j.at("specs")[i];const auto&r=j.at("scenarios")[i];replay(s,r,r);if(i%7==0)binding(s,r);}for(const auto&r:j.at("faults")){const auto i=r.at("scenario").get<std::size_t>();replay(j.at("specs")[i],r,j.at("scenarios")[i]);}std::cout<<"PASS original combat mode/reset/query retirement, all retained banks, read/store faults and actual ABI\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
