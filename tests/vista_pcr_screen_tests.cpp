#include "halo2/vista_pcr_screen.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <xmmintrin.h>
using namespace halo2;
using J=nlohmann::json;
namespace {
void require(bool b,const char* what){if(!b)throw std::runtime_error(what);}
std::string narrow(std::u16string_view s){std::string o;for(auto c:s){require(c<128,"fixture ASCII");o+=char(c);}return o;}
std::u16string wide(std::string_view s){return {s.begin(),s.end()};}
void put(std::span<std::byte> b,unsigned at,std::uint32_t v,unsigned n=4){for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(8*i));}
void ws(std::span<std::byte> b,unsigned at,std::string_view s,unsigned n){for(unsigned i=0;i<n;++i)put(b,at+2*i,i<s.size()?std::uint8_t(s[i]):0,2);}
VistaPcrResults bank(unsigned seed,unsigned variant){
 VistaPcrResults b;b.player_count=b.team_count=16;b.variant=variant;
 for(unsigned p=0;p<16;++p){
  auto& r=b.players[p];for(unsigned i=0;i<r.size();++i)r[i]=std::byte(((p*0x110+i)*37+seed*13)&255);
  ws(r,0,"Player "+std::to_string(p),32);ws(r,0x40,"Score "+std::to_string(p),16);ws(r,0xe0,"Place "+std::to_string(p),16);
  for(unsigned k=0;k<4;++k)put(r,0x60+4*k,seed*17+p*113+k*1009);
  constexpr std::int32_t counts[8][2]={{0,0},{1,2},{0x7fffffff,1},{0x7fffffff,0x7fffffff},{123456789,987654321},{-1,1},{9,-1},{1,0}};
  put(r,0x84,std::bit_cast<std::uint32_t>(counts[seed%8][0]));put(r,0x88,std::bit_cast<std::uint32_t>(counts[seed%8][1]));
  put(r,0x108,seed*99991+p*71);put(r,0x10c,0u-seed*119-p*31);
  constexpr std::uint32_t masks[8]={0,0x80000000,0xffffff,0x800001,0x124824,1,0x2000,0xaaaaaa};put(r,0x80,masks[seed%8]);
  auto& t=b.teams[p];for(unsigned i=0;i<t.size();++i)t[i]=std::byte(((p*0x84+i)*23+seed*19)&255);
  ws(t,0,"Team "+std::to_string(p),32);ws(t,0x40,"TScore "+std::to_string(p),16);ws(t,0x64,"TPlace "+std::to_string(p),16);put(t,0x62,p%8,2);
 }return b;
}
std::u16string localized(std::uint32_t key,unsigned seed){
 if(key==0xb000734)return u"TIME";
 if(key==0x11000757)return u"PERCENT";
 char buf[10];std::snprintf(buf,sizeof(buf),"M%08x",key);std::string s(buf);
 if(seed==23)for(unsigned i=0;i<9;++i)s+="abcdefghijklmnopqrstuvwxy";
 return wide(s);
}
} // namespace
int main(int argc,char** argv){try{
 require(argc==2,"fixture argument");std::ifstream file(argv[1]);J cases;file>>cases;unsigned checked{};
 for(const auto& record:cases){
  const auto& q=record.at("input");J events=J::array();
  auto log=[&](J e){events.push_back(std::move(e));};
  const VistaLoadingFloatingPoint fp{std::uint16_t(q.value("cw",0x27fu)),q.value("mx",0x1f80u)};
  const auto saved=_mm_getcsr();unsigned short savedcw;asm volatile("fnstcw %0":"=m"(savedcw));
  // Every explicit context is tested under an opposite ambient context.
  _mm_setcsr(0x1f80|((fp.mxcsr^0x6000)&0x6000));
  bool returned{};
  if(q.at("kind")=="row"){
   const auto seed=q.at("seed").get<unsigned>();auto b=bank(seed,q.at("variant"));VistaPcrRowServices s;
   s.grid_child=[&](unsigned i){log({"grid",i});return true;};
   s.text_child=[&](unsigned i){log({"child",i});return (q.at("mask").get<unsigned>()&(1u<<i))!=0;};
   s.bitmap_child=[&](unsigned i){log({"bitmap",i});return q.at("bitmap").get<unsigned>()!=0;};
   s.text=[&](unsigned i,std::u16string_view value){log({"text",i,narrow(value)});};
   s.visible=[&](unsigned i,bool value){log({"visible",i,unsigned(value)});};
   s.text_color=[&](unsigned i,const auto& value){log({"text_color",i,value[0],value[1],value[2]});};
   s.bitmap_color=[&](unsigned i,const auto& value){log({"bitmap_color",i,value[0],value[1],value[2]});};
   s.team_color=[&](std::uint16_t i){log({"team_color",i});const auto v=record.at("colors").get<std::array<std::uint32_t,6>>();return VistaPcrTeamColor{{v[0],v[1],v[2]},{v[3],v[4],v[5]}};};
   s.appearance=[&](unsigned i){log({"appearance",i});};unsigned selected{};
   s.selected_row=[&](){auto i=q.at("selected").at(selected++).get<std::uint32_t>();log({"selected",i});return i;};
   s.localized=[&](std::uint32_t i){log({"locale",i});return localized(i,seed);};
   s.format=[&](std::u16string_view f,std::span<const std::int32_t> values){
    J e={"format",narrow(f)};std::string text=narrow(f)+"|";
    for(unsigned i=0;i<values.size();++i){e.push_back(std::bit_cast<std::uint32_t>(values[i]));if(i)text+=',';text+=std::to_string(values[i]);}
    log(e);return wide(text);
   };
   vista_pcr_bind_row(VistaPcrPane(q.at("pane").get<unsigned>()),q.at("row"),b,s,fp);
  } else if(q.at("kind")=="input"){
   VistaPcrInputServices s;unsigned hosts{};
   s.host=[&](){auto v=q.at("hosts").at(hosts++).get<std::uint8_t>();log({"host",v});return v;};
   s.details_available=[&](){auto v=q.at("details").get<std::uint8_t>();log({"details",v});return v;};
   s.selected_identity=[&](auto& value){log({"identity",value[0],value[1]});value={0x12345678,0x9abcdef0};return q.at("identity").get<std::uint8_t>();};
   s.player_details=[&](const auto& value){log({"player_details",value[0],value[1]});};
   s.close=[&](unsigned i){log({"close",i});};s.continue_session=[&](){log({"continue"});};
   s.dialog=[&](const auto& value){J e={"dialog"};for(auto i:value)e.push_back(i);log(e);};
   returned=vista_pcr_input(q.at("lobby"),q.at("code"),q.at("controller"),s);
  } else {
   VistaPcrDrawServices s;
   s.elapsed=[&](std::uint32_t i){log({"elapsed",i});return q.at("elapsed").get<std::uint32_t>();};
   s.black=[&](std::uint32_t i){log({"black",i});};s.base_draw=[&](){log({"draw"});};
   vista_pcr_draw(q.at("lobby"),q.at("creation"),s,fp);
  }
  require(_mm_getcsr()==(0x1f80|((fp.mxcsr^0x6000)&0x6000)),"MXCSR ambient restoration");_mm_setcsr(saved);
  unsigned short after;asm volatile("fnstcw %0":"=m"(after));require(after==savedcw,"x87 control restoration");
  if(events!=record.at("events")){
   std::cerr<<"case="<<checked<<" input="<<q.dump()<<"\nexpected="<<record.at("events").dump()<<"\nactual="<<events.dump()<<'\n';throw std::runtime_error("original PCR callback mismatch");
  }
  if(!record.at("returned").is_null())require(returned==bool(record.at("returned").get<unsigned>()),"original PCR input AL");
  ++checked;
 }
 require(checked>1000,"complete original corpus");std::cout<<"Vista PCR screen original cases="<<checked<<" PASS\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
