#include "halo2/vista_pcr_appearance.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <xmmintrin.h>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const std::string& why) {
  if(!value)throw std::runtime_error(why);
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string result;
  for(const auto b:bytes) {
    const auto value=std::to_integer<unsigned>(b);
    result+=digits[value>>4];result+=digits[value&15];
  }
  return result;
}
void put(std::span<std::byte> bytes,unsigned at,std::uint32_t value,unsigned count=4) {
  for(unsigned i=0;i<count;++i)bytes[at+i]=std::byte(value>>(i*8));
}
std::string narrow(std::u16string_view text) {
  std::string result;
  for(const auto c:text) {check(c<128,"original fixture requires ASCII");result+=char(c);}
  return result;
}
VistaPcrResults bank(const Json& q) {
  VistaPcrResults result;const auto row=q.at("row").get<unsigned>();
  check(row<16,"original fixture row outside bank");result.player_count=row+1;
  auto& player=result.players[row];const auto seed=q.at("seed").get<unsigned>();
  for(unsigned i=0;i<player.size();++i)player[i]=std::byte((i*37+seed*13)&255);
  std::fill_n(player.begin(),64,std::byte{});const auto name=q.at("name").get<std::string>();
  check(name.size()<32,"original fixture name too long");
  for(unsigned i=0;i<name.size();++i)put(player,i*2,std::uint8_t(name[i]),2);
  put(player,0x72,q.at("team"),2);put(player,0x74,q.at("no_team"),1);put(player,0x75,0);
  put(player,0x78,std::bit_cast<std::uint32_t>(q.at("rank_value").get<std::int32_t>()));
  const auto appearance=q.at("appearance").get<std::array<std::uint8_t,16>>();
  for(unsigned i=0;i<appearance.size();++i)player[0xd0+i]=std::byte(appearance[i]);
  for(unsigned i=0;i<8;++i)player[0x100+i]=std::byte(i+1);
  return result;
}
struct Services final:VistaPcrAppearanceServices {
  const Json& q;Json events=Json::array();
  explicit Services(const Json& input):q(input){}
  bool identity_query(std::array<std::byte,8> identity) override {
    const auto value=q.at("identity").get<unsigned>();events.push_back({"identity",hex(identity),value});return value!=0;
  }
  std::uint32_t primary_player_query(std::array<std::byte,8> identity) override {
    const auto value=q.at("primary_player").get<std::uint32_t>();events.push_back({"primary_player",hex(identity),value});return value;
  }
  bool player_query(std::uint32_t index) override {
    const auto value=q.at("player").get<unsigned>();events.push_back({"player",index,value});return value!=0;
  }
};
// Run every explicit source context under different ambient rounding modes,
// and restore the caller even if a fixture comparison throws.
class AmbientContext {
  unsigned mxcsr_=_mm_getcsr();std::uint16_t cw_{};
 public:
  unsigned alternate_mxcsr;std::uint16_t alternate_cw;
  explicit AmbientContext(const VistaLoadingFloatingPoint& fp)
      :alternate_mxcsr(0x1f80|((fp.mxcsr^0x6000)&0x6000)),
       alternate_cw(std::uint16_t(0x27f|((fp.x87_control_word^0xc00)&0xc00))) {
    asm volatile("fnstcw %0":"=m"(cw_));
    _mm_setcsr(alternate_mxcsr);asm volatile("fldcw %0"::"m"(alternate_cw):"memory");
  }
  ~AmbientContext() {_mm_setcsr(mxcsr_);asm volatile("fldcw %0"::"m"(cw_):"memory");}
  void verify() const {
    std::uint16_t actual;asm volatile("fnstcw %0":"=m"(actual));
    check(actual==alternate_cw,"appearance changed ambient x87 control word");
    check(_mm_getcsr()==alternate_mxcsr,"appearance changed ambient MXCSR");
  }
};
template<class F> void rejected(F&& action,const char* why) {
  try {action();}catch(const FormatError&){return;}
  throw std::runtime_error(std::string("appearance boundary admitted: ")+why);
}
void boundaries() {
  const Json q={{"identity",0},{"primary_player",3},{"player",1}};Services service(q);
  VistaPcrResults b;VistaPcrAppearanceAssets assets;
  rejected([&]{(void)vista_pcr_player_presentation(b,0,assets,service,{0x27f,0x1f80});},"empty retained bank");
  b.player_count=1;
  rejected([&]{(void)vista_pcr_player_presentation(b,1,assets,service,{0x27f,0x1f80});},"inactive retained row");
  b.player_count=17;
  rejected([&]{(void)vista_pcr_player_presentation(b,16,assets,service,{0x27f,0x1f80});},"outside physical bank");
  check(service.events.empty(),"row rejection consumed original identity services");
  constexpr VistaPcrRgb rgb{0x3e000000,0x3f000000,0x3f600000};
  for(const VistaLoadingFloatingPoint fp:std::array<VistaLoadingFloatingPoint,4>{{
       {0x027e,0x1f80},{0x017f,0x1f80},{0x027f,0x1f00},{0x027f,0x11f80}}}) {
    AmbientContext ambient({});
    rejected([&]{(void)vista_pcr_profile_name_color(rgb,fp);},"unsupported explicit floating point context");ambient.verify();
  }
}
} // namespace
int main(int argc,char** argv) {
  try {
    check(argc==2,"provide original appearance fixture path");std::ifstream file(argv[1]);check(bool(file),"appearance fixture open failed");
    std::string line;unsigned count{};std::uint64_t visits{};
    while(std::getline(file,line)) {
      if(line.empty())continue;
      const auto record=Json::parse(line);const auto& q=record.at("input");
      try {
        const auto palette=q.at("palette").get<std::vector<VistaPcrRgb>>();
        const auto teams=q.at("teams").get<std::vector<VistaPcrRgb>>();
        std::vector<VistaPcrRankRange> ranges;
        for(const auto& range:q.at("ranges"))ranges.push_back({range.at(0).get<std::int16_t>(),range.at(1).get<std::int16_t>()});
        const VistaPcrAppearanceAssets assets{q.at("globals"),palette,teams,q.at("rank_owner"),ranges,
          q.at("primary_bitmap"),q.at("rank_bitmap"),q.at("primary_pages"),q.at("rank_pages"),
          q.at("previous_primary_frame"),q.at("previous_rank_frame"),q.at("emblem_flags")};
        const VistaLoadingFloatingPoint fp{q.value("cw",std::uint16_t(0x27f)),q.value("mx",0x1f80u)};
        auto b=bank(q);const auto before=b;Services services(q);AmbientContext ambient(fp);
        const auto view=vista_pcr_player_presentation(b,q.at("row"),assets,services,fp);ambient.verify();
        check(b==before,"appearance consumer changed retained bank");
        const Json actual={{"descriptor",{{"flags",view.descriptor_flags},{"appearance",hex(view.appearance)},
          {"team",view.team},{"no_team",view.no_team},{"rank",view.rank_range}}},
          {"primary_color",view.primary_color},{"name_color",view.name_color},
          {"primary_visible",unsigned(view.primary_visible)},{"secondary_visible",unsigned(view.secondary_visible)},
          {"rank_visible",unsigned(view.rank_visible)},{"emblem_visible",unsigned(view.emblem_visible)},
          {"primary_frame",view.primary_frame},{"rank_frame",view.rank_frame},{"groups",view.group_visible}};
        if(actual!=record.at("view"))throw std::runtime_error("source view differs\nexpected="+record.at("view").dump()+"\nactual="+actual.dump());
        // The native API returns the text for its widget adapter. Its final
        // assignment has the same operand and follows all identity services.
        services.events.push_back({"text",narrow(view.name)});
        if(services.events!=record.at("events"))throw std::runtime_error("source services differ\nexpected="+record.at("events").dump()+"\nactual="+services.events.dump());
        check(vista_pcr_appearance_colors(view.appearance,view.team,view.no_team,assets)[0]==view.primary_color,
          "public color owner differs from original primary output");
        if(q.at("identity").get<unsigned>()) {
          check(vista_pcr_profile_name_color(view.primary_color,fp)==view.name_color,"public name color owner differs from original output");ambient.verify();
        }
        visits+=record.at("visits").get<std::uint64_t>();++count;
      }catch(const std::exception& e) {
        throw std::runtime_error("appearance original case "+std::to_string(count)+" input="+q.dump()+"\n"+e.what());
      }
    }
    check(count==568,"complete original appearance corpus changed");check(visits==1530220,"original instruction coverage changed");boundaries();
    std::cout<<"PASS "<<count<<" original Vista appearance cases, "<<visits
      <<" original instructions; complete descriptors, colors, visibility, frames, groups and ordered services; caller FP restoration\n";
    return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
