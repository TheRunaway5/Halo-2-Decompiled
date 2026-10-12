#include "halo2/vista_pcr_results.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace halo2;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v,unsigned n=4){for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(8*i));}
std::u16string wide(const std::string& s){return {s.begin(),s.end()};}
std::array<std::byte,vista_pcr_source_size> make(const std::vector<std::uint32_t>& q) {
  std::array<std::byte,vista_pcr_source_size> b;
  const auto seed=q[0];for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(i*37+seed*13+(i>>8)*17);
  b[0x10]=std::byte(q[4]);b[0x11]=std::byte(q[5]);put(b,0x64,q[3]);b[0x68]=std::byte(q[6]);
  for(unsigned p=0;p<16;++p){const auto at=0x370+p*0x94;b[at]=std::byte(bool(q[1]&(1u<<p)));
    auto name="Source "+std::to_string(p)+"-"+std::to_string(seed);
    if(seed==47)name="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 long";
    std::fill_n(b.begin()+at+12,64,std::byte{});
    for(unsigned i=0;i<std::min<std::size_t>(32,name.size());++i)put(b,at+12+2*i,std::uint8_t(name[i]),2);
    b[at+0x88]=std::byte(q[6]?(p%8):255);
    b[at+0x90]=std::byte(q[7]?0:((p*5+seed)%7)-1);
    put(b,at+0x92,q[7]?123:((seed*113+p*947+32768)%65536)-32768,2);
  }
  for(unsigned t=0;t<16;++t){const auto at=0xcb0+t*0x18;b[at]=std::byte(bool(q[2]&(1u<<t)));
    b[at+1]=std::byte(q[7]?0:(t*3+seed)%9);
    put(b,at+2,q[7]?123:((seed*127+t*491+32768)%65536)-32768,2);
  }
  return b;
}
std::vector<std::byte> hex(const std::string& s) {
  check(s.size()%2==0,"Odd original hex extent");std::vector<std::byte> out;
  for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
  return out;
}
VistaPcrResults poison(){VistaPcrResults out;out.variant=out.player_count=out.team_count=0x6d6d6d6d;
 for(auto& r:out.players)r.fill(std::byte{0x6d});
 for(auto& r:out.teams)r.fill(std::byte{0x6d});
 return out;}
void controls() {
 const auto b=make({17,1,1,2,1,1,0,0,1});auto out=poison();const auto before=out;
 VistaPcrServices s;
 vista_pcr_build(out,{},s);check(out==before,"Absent engine consumed source or changed old report");
 s.engine_present=true;
 try{vista_pcr_build(out,{},s);throw std::runtime_error("Truncated source admitted");}catch(const FormatError&){}
 check(out==before,"Truncated source changed report");
 try{vista_pcr_build(out,b,s);throw std::runtime_error("Missing locale admitted");}catch(const FormatError&){}
 check(out==before,"Missing locale mutated report before admission");
 s.score=[](auto n){return wide(std::to_string(n));};s.placement=s.score;
 try{vista_pcr_build(out,b,s);throw std::runtime_error("Missing live lookup admitted");}catch(const FormatError&){}
 check(out==before,"Missing lookup manufactured an absent live player");
 unsigned lookups{},selections{},awards{};std::array<std::byte,8> identity;identity.fill(std::byte{0xac});
 s.live_identity=[&](unsigned p){check(p==0,"Wrong live identity slot");++lookups;return identity;};
 s.selected_identity=[&]{++selections;return identity;};s.medal_achievement=[&](unsigned n){++awards;check(n>0 && n<=32,"Wrong distinct medal count");};
 vista_pcr_build(out,b,s);check(lookups==1 && selections==1 && awards==1,"Matching identity achievement lost");
 s.selected_identity=[] {return std::array<std::byte,8>{};};vista_pcr_build(out,b,s);check(awards==1,"Different player received achievement");
 s.team_play=true;s.team_name=[](auto n){return wide(std::to_string(n));};vista_pcr_build(out,b,s);
 check(lookups==2 && awards==1,"Team play consumed FFA achievement services");
}
}
int main(int argc,char** argv){try{
 check(argc==2,"Supply original PCR fixture");std::ifstream file(argv[1]);check(bool(file),"Missing PCR fixture");
 std::string line;unsigned cases{};bool version{};
 while(std::getline(file,line)){
  if(line=="# H2V_PCR_RESULTS_ORIGINAL_V1")version=true;
  if(line.empty() || line[0]=='#')continue;
  std::istringstream input(line);unsigned label{};std::string words,bytes;check(bool(input>>label>>words>>bytes),"Malformed PCR fixture");
  std::replace(words.begin(),words.end(),',',' ');std::istringstream stream(words);std::vector<std::uint32_t> values;std::uint64_t word{};
  while(stream>>word){check(word<=0xffffffffu,"PCR word overflow");values.push_back(std::uint32_t(word));}
  check(values.size()>=15,"Truncated PCR control");const auto b=make(values);std::size_t cursor=12;
  const auto order=vista_pcr_order(b);
  check(order.player_count==values[cursor++],"Player order count differs");
  for(unsigned i=0;i<order.player_count;++i)check(order.players[i]==values.at(cursor++),"Original player tie ordering differs");
  check(order.team_count==values.at(cursor++),"Team order count differs");
  for(unsigned i=0;i<order.team_count;++i)check(order.teams[i]==values.at(cursor++),"Original team tie ordering differs");
  std::vector<std::pair<unsigned,std::uint32_t>> calls;
  VistaPcrServices s;s.engine_present=values[8]!=0;s.team_play=values[6]!=0;
  s.team_name=[&](auto n){calls.emplace_back(0,n);return wide("Team "+std::to_string(n));};
  s.score=[&](auto n){calls.emplace_back(1,std::uint32_t(n));return wide("score "+std::to_string(n));};
  s.placement=[&](auto n){calls.emplace_back(2,std::uint32_t(n));return wide("rank "+std::to_string(n));};
  s.live_identity=[](unsigned)->std::optional<std::array<std::byte,8>>{return std::nullopt;};
  auto out=poison();vista_pcr_build(out,b,s);
  check(out.player_count==values[9] && out.team_count==values[10] && out.variant==values[11],"Original published report fields differ");
  const auto count=values.at(cursor++);check(calls.size()==count,"Locale call count differs");
  for(const auto& call:calls){check(call.first==values.at(cursor++) && call.second==values.at(cursor++),"Locale call order or value differs");}
  check(cursor==values.size(),"Extra original words");const auto expected=hex(bytes);check(expected.size()==0x1940,"Original bank extent differs");std::size_t at{};
  for(const auto& bank:out.players)for(auto byte:bank){if(byte!=expected[at]){std::ostringstream e;e<<"Case "<<label<<" player byte +"<<std::hex<<at<<" differs";throw std::runtime_error(e.str());}++at;}
  for(const auto& bank:out.teams)for(auto byte:bank){if(byte!=expected[at]){std::ostringstream e;e<<"Case "<<label<<" team byte +"<<std::hex<<at<<" differs";throw std::runtime_error(e.str());}++at;}
  ++cases;
 }
 check(file.eof() && version && cases==47,"Required original PCR controls missing");controls();
 std::cout<<cases<<" original PCR banks/order/locale calls matched\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
