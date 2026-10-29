#include "halo2/native_campaign_ai_orders_tu15.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using Json=nlohmann::json;
constexpr std::uint32_t base=0x2000000u,size=0x12000u;
void check(bool v,const std::string &why){if(!v)throw std::runtime_error(why);}
std::string hex(std::span<const std::byte> bytes){constexpr char digits[]="0123456789abcdef";std::string out;for(const auto b:bytes){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}return out;}
std::vector<std::byte> unhex(const std::string &text){std::vector<std::byte> out;for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return out;}
class Memory final:public SourcePhysicsContactMemory {
public:
 std::vector<std::byte> bank=std::vector<std::byte>(size);
 std::map<std::uint32_t,std::byte> globals;
 mutable Json events=Json::array();int fault=-1;
 std::byte get(std::uint32_t p)const{if(p>=base&&p<base+size)return bank[p-base];const auto at=globals.find(p);if(at==globals.end())throw std::runtime_error("Missing retained assignment input");return at->second;}
 void seed(std::uint32_t p,std::uint32_t v){for(unsigned i=0;i<4;++i){if(p+i>=base&&p+i<base+size)bank[p+i-base]=std::byte(v>>(i*8));else globals[p+i]=std::byte(v>>(i*8));}}
 void read(std::uint32_t p,std::span<std::byte> output)const override{
  for(unsigned i=0;i<output.size();++i)output[i]=get(p+i);
  events.push_back(Json::array({"r",p,output.size(),hex(output)}));
  if(int(events.size())-1==fault)throw std::runtime_error("Declared source read fault");
 }
 void write(std::uint32_t p,std::span<const std::byte> input)override{
  std::vector<std::byte> old;for(unsigned i=0;i<input.size();++i)old.push_back(get(p+i));
  events.push_back(Json::array({"w",p,input.size(),hex(old),hex(input)}));
  if(int(events.size())-1==fault)throw std::runtime_error("Declared source store fault");
  if(p<base||std::uint64_t(p)+input.size()>base+size)throw std::runtime_error("Native assignment wrote outside its retained bank");
  std::copy(input.begin(),input.end(),bank.begin()+p-base);
 }
};
void replay(const Json &c){
 Memory m;for(const auto &w:c.at("writes"))m.seed(w[0],w[1]);
 auto expected=m.bank;const auto before_globals=m.globals;
 if(!c.at("fault_at").is_null())m.fault=c.at("fault_at");
 _mm_setcsr(0x1f80u);bool failed=false;
 try{native_campaign_ai_order_script_assignment_tu15(m,c.at("actor"),c.at("target"));}catch(const std::exception &){failed=true;}
 const auto fp=_mm_getcsr();
 check(failed==(m.fault>=0),c.at("name").get<std::string>()+": assignment failed at a different boundary");
 check(m.events==c.at("events"),c.at("name").get<std::string>()+": ordered assignment events differ: "+m.events.dump());
 const auto completed=c.at("events").size()-(m.fault>=0?1u:0u);
 for(std::size_t i=0;i<completed;++i){const auto &e=c.at("events")[i];if(e[0]=="w"){const auto bytes=unhex(e[4]);std::copy(bytes.begin(),bytes.end(),expected.begin()+e[1].get<std::uint32_t>()-base);}}
 check(m.bank==expected,"Assignment changed bytes outside the original write/fault prefix");
 check(m.globals==before_globals,"Assignment changed immutable/global owner identities");
 check(fp==c.at("mxcsr"),"Assignment changed the original FP environment");
}
}
int main(int argc,char **argv){try{
 check(argc==2,"Supply independently executed original assignment fixture");Json fixture;std::ifstream(argv[1])>>fixture;
 check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong TU1.5 authority");
 check(fixture.at("instruction_patches").empty()&&fixture.at("supplied_dependency_returns").empty(),"Reference supplied an original dependency result");
 check(fixture.at("required_backend")=="physical-i386-complete-bank-fp+unicorn-ordered-source-events","Reference lacks physical retained-bank proof");
 for(const auto &c:fixture.at("cases"))replay(c);
 std::cout<<"PASS "<<fixture.at("cases").size()<<" whole original actor/script reassignments and ordered source fault prefixes; complete bank/free-list/actor identity and unchanged FP\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
