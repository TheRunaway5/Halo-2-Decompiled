#include "halo2/native_campaign_ai_orders_camouflage_tu15.hpp"
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
class Memory final:public SourcePhysicsContactMemory{
public:
 std::vector<std::byte> bank=std::vector<std::byte>(size);std::map<std::uint32_t,std::byte> globals;mutable Json events=Json::array();int fault=-1;
 std::byte get(std::uint32_t p)const{if(p>=base&&p<base+size)return bank[p-base];const auto i=globals.find(p);if(i==globals.end())throw std::runtime_error("Missing camouflage source input");return i->second;}
 void seed(std::uint32_t p,unsigned width,std::uint32_t v){for(unsigned i=0;i<width;++i)if(p+i>=base&&p+i<base+size)bank[p+i-base]=std::byte(v>>(i*8));else globals[p+i]=std::byte(v>>(i*8));}
 void read(std::uint32_t p,std::span<std::byte>b)const override{for(unsigned i=0;i<b.size();++i)b[i]=get(p+i);events.push_back(Json::array({"r",p,b.size(),hex(b)}));if(int(events.size())-1==fault)throw std::runtime_error("Declared original read boundary");}
 void write(std::uint32_t p,std::span<const std::byte>b)override{std::vector<std::byte>old;for(unsigned i=0;i<b.size();++i)old.push_back(get(p+i));events.push_back(Json::array({"w",p,b.size(),hex(old),hex(b)}));if(int(events.size())-1==fault)throw std::runtime_error("Declared original write boundary");check(p>=base&&std::uint64_t(p)+b.size()<=base+size,"Camouflage changed external bank");std::copy(b.begin(),b.end(),bank.begin()+p-base);}
};
void replay(const Json &c){Memory m;for(const auto&w:c.at("writes"))m.seed(w[0],w[1],w[2]);auto expected=m.bank;const auto global=m.globals;if(!c.at("fault_at").is_null())m.fault=c.at("fault_at");
 NativeCampaignAiOrdersFp fp{c.at("initial_mxcsr").get<std::uint32_t>()};_mm_setcsr(0xbfc0u);bool failed=false,result=false;
 try{result=native_campaign_ai_orders_camouflage_tu15(m,c.at("actor"),c.at("enabled"),fp);}catch(const std::exception&){failed=true;}
 const auto ambient=_mm_getcsr();check(failed==(m.fault>=0),"Camouflage failure boundary differs");check(m.events==c.at("events"),c.at("name").get<std::string>()+": ordered camouflage events differ: "+m.events.dump());
 for(std::size_t i=0;i<c.at("events").size()-(m.fault>=0?1u:0u);++i){const auto&e=c.at("events")[i];if(e[0]=="w"){const auto b=unhex(e[4]);std::copy(b.begin(),b.end(),expected.begin()+e[1].get<std::uint32_t>()-base);}}
 check(m.bank==expected&&m.globals==global,"Camouflage complete retained state differs");check(fp.mxcsr==c.at("mxcsr")&&ambient==0xbfc0u,"Camouflage source/caller FP state differs");if(!failed)check(result==c.at("result"),"Original actor-unit boolean differs");
}
}
int main(int argc,char**argv){try{check(argc==2,"Supply independent original camouflage fixture");Json j;std::ifstream(argv[1])>>j;check(j.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong TU1.5 authority");check(j.at("instruction_patches").empty()&&j.at("supplied_dependency_returns").empty(),"Original result was supplied");check(j.at("required_backend")=="physical-i386-complete-bank-result-fp+unicorn-ordered-source-events","Missing physical bank/result/FP authority");for(const auto&c:j.at("cases"))replay(c);std::cout<<"PASS "<<j.at("cases").size()<<" original actor camouflage/dirty-entity complete banks, booleans, ordered read/store fault prefixes and FP\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
