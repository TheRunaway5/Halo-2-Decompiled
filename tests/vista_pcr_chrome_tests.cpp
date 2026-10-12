#include "halo2/vista_pcr_screen.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
std::u16string locale(std::uint32_t key){switch(key){case 0x06000763:return u"\ue101 DONE";
 case 0x0d000764:return u"\ue101 LEAVE PARTY";case 0x13000906:return u"\ue100 SHOW GAMERCARD ";}
 throw std::runtime_error("unexpected footer locale");}
std::string utf8(std::u16string_view value){std::string out;for(auto c:value){if(c<128)out+=char(c);
 else if(c<2048){out+=char(0xc0|(c>>6));out+=char(0x80|(c&63));}
 else {out+=char(0xe0|(c>>12));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}}return out;}
}
int main(int argc,char **argv)try {
 check(argc==2,"supply original chrome fixture");std::ifstream file(argv[1]);check(bool(file),"missing original chrome fixture");
 const auto rows=Json::parse(file);unsigned cases{};std::uint64_t visits{};
 for(const auto &entry:rows){const auto &q=entry.at("input");const auto kind=q.at("kind").get<std::string>();
  if(kind=="headings") {
   const auto actual=vista_pcr_headings(VistaPcrPane(q.at("pane").get<unsigned>()),q.at("variant"));
   std::array<std::optional<std::uint32_t>,5> expected;
   for(const auto &event:entry.at("events"))if(event[0]=="string")expected.at(event[1].get<unsigned>())=event[2].get<std::uint32_t>();
   check(actual==expected,"source heading replacements differ");
  }else if(kind=="chrome") {
   const auto result=vista_pcr_footer(VistaPcrPane(q.at("pane").get<unsigned>()),q.at("lobby"),q.at("host"),q.at("details"),locale);
   check(utf8(result)==entry.at("output").at("footer").get<std::string>(),"source active-pane footer differs");
   check((std::bit_cast<std::int32_t>(q.at("players").get<std::uint32_t>())<=0)==(entry.at("output").at("visible").get<unsigned>()!=0),"source empty-report visibility differs");
  }else if(kind=="voice") {
   VistaPcrResults bank;bank.player_count=8;const std::array<std::byte,8> identity{std::byte{1},std::byte{0x23},std::byte{0x45},std::byte{0x67},std::byte{0x89},std::byte{0xab},std::byte{0xcd},std::byte{0xef}};
   std::copy(identity.begin(),identity.end(),bank.players[7].begin()+0x100);const auto before=bank;
   Json events=Json::array();VistaPcrVoiceServices services;
   services.identity_slot=[&](const auto &actual){check(actual==identity,"voice queried wrong retained XUID");events.push_back({"identity","0123456789abcdef",q.at("slot")});return q.at("slot").get<std::uint32_t>();};
   services.enabled=[&](auto slot){events.push_back({"enabled",slot,q.at("enabled")});return q.at("enabled").get<std::uint8_t>();};
   services.talking=[&](auto slot){events.push_back({"talking",slot,q.at("talking")});return q.at("talking").get<std::uint8_t>();};
   const auto result=vista_pcr_voice(bank,7,q.at("lobby"),services);const auto &output=entry.at("output");
   check(result.visible==(output.at("visible").get<unsigned>()!=0),"source voice bitmap visibility differs");
   if(result.visible)check(result.frame==output.at("frame").get<unsigned>(),"source voice bitmap frame differs");
   Json expected=Json::array();for(const auto &event:entry.at("events"))if(event[0]=="identity" || event[0]=="enabled" || event[0]=="talking")expected.push_back(event);
   check(events==expected,"source voice service order differs");check(bank==before,"voice consumer changed retained bank");
  }else throw std::runtime_error("unknown source fixture kind");
  ++cases;visits+=entry.at("visits").get<std::uint64_t>();
 }
 check(cases==1242,"original chrome controls missing");
 std::cout<<cases<<" original heading/chrome/voice cases matched; "<<visits<<" original instruction visits\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
