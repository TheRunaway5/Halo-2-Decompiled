#include "halo2/native_vista_pcr.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/full_scoreboard.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
std::uint32_t word(std::span<const std::byte> bytes,unsigned at){std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(bytes[at+i])<<(8*i);return value;}
std::u16string text(std::span<const std::byte> bytes,unsigned at,unsigned capacity){std::u16string value;for(unsigned i=0;i<capacity;++i){const auto c=std::to_integer<unsigned>(bytes[at+2*i])|(std::to_integer<unsigned>(bytes[at+2*i+1])<<8);if(!c)break;value+=char16_t(c);}return value;}
bool gray(const VistaPcrRgb &value){return value==VistaPcrRgb{0x3e99999a,0x3e99999a,0x3e99999a};}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply original source root");
  VistaLoadingFloatingPoint fp{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("fnstcw %0":"=m"(fp.x87_control_word));fp.mxcsr=_mm_getcsr();
#else
  return 77;
#endif
  const std::filesystem::path root=argv[1];std::filesystem::path pc;
  {std::ifstream sources(root/"assets/asset-sources.tsv");for(std::string line;std::getline(sources,line);)if(line.starts_with("vista\t"))pc=line.substr(6);}
  if(!std::filesystem::is_regular_file(pc/"maps/mainmenu.map"))return 77;
  FrontendContent content;content.vista=load_vista_settings(pc/"maps/mainmenu.map");
  unsigned cases{};
  for(unsigned engine=0;engine<7;++engine) {
    const auto variant=source_game_variant_default(engine,u"Source PCR family");
    const auto type=source_game_variant_type(variant);const bool teams=source_game_variant_get(variant,91)!=0;
    check(source_game_variant_valid(variant),"Original family default is invalid");
    SourceResultStatistics numeric;
    // Explicit component input bank. The separate unchanged23975C controls
    // prove these getters; natural-source-world/GPU endings are independent.
    for(unsigned slot=0;slot<16;++slot)for(unsigned stat=0;stat<437;++stat)numeric.players[slot][stat]=std::uint16_t(1000+slot*50+stat);
    VistaPcrPublicationInput input;input.variant=vista_pcr_multiplayer_variant(variant);input.statistics=&numeric;
    input.recording={1,1};input.current=input.finalized=1;input.engine_present=true;
    std::vector<OnlineAdmission> roster;
    for(unsigned n=0;n<2;++n) {
      const unsigned slot=n?7:3;OnlineAdmission admitted;admitted.slot=slot;admitted.name="Same Pilot";admitted.team=teams?int(n):-1;
      admitted.identity.fill(std::byte(slot+1));if(n==0)admitted.matchmaking_level=27;
      roster.push_back(admitted);auto &player=input.players[slot];player.occupied=1;player.identity=admitted.identity;
      std::array<std::uint8_t,9> appearance{};appearance[0]=std::uint8_t(n?8:4);
      player.configuration=native_vista_selected_profile_payload(native_selected_player_configuration(admitted.name,appearance,admitted.team));
      player.configuration[0x7c]=std::byte(teams?n:0xff);player.score=50-int(n);player.placement=int(n);
      if(teams)input.teams[n]={1,50-int(n),int(n)};
    }
    const auto source=vista_pcr_publish_consumed_snapshot(input);
    check(std::equal(input.variant.bytes.begin(),input.variant.bytes.end(),source.source.begin()+0x20),"Family publication changed its actual304byte variant");
    std::optional<std::uint16_t> presence=std::uint16_t((1u<<3)|(1u<<7));
    NativeVistaPcr pcr(source,roster,content,fp,[&]{return presence;});const auto results=pcr.results();const auto before=*results;
    check(results->variant==unsigned(type) && results->player_count==2 && results->team_count==(teams?2u:0u),"Original family PCR lost player/team/type");
    unsigned first{},second{};switch(type){case 2:first=13;second=12;break;case 1:first=16;second=15;break;case 3:first=26;second=25;break;case 4:first=28;second=27;break;case 7:first=38;second=39;break;case 8:first=43;second=44;break;case 9:first=21;second=20;break;default:throw FormatError("Unexpected original family");}
    for(unsigned row=0;row<2;++row) {
      const auto &display=results->players[row];const auto identity=std::to_integer<unsigned>(display[0x100]);const unsigned slot=identity-1;
      check(slot==3 || slot==7,"Sparse original slot lost its identity");
      const auto expected=type==2?numeric.players[slot][13]/std::max(unsigned(numeric.players[slot][9])+1,1u):numeric.players[slot][first];
      check(word(display,0x108)==expected && word(display,0x10c)==numeric.players[slot][second],"Family-specific original numeric columns differ");
      check(text(display,0x40,16)==full_scoreboard_score_text(slot==3?50:49,type),"Timed-family PCR score was printed as a decimal counter");
      check(word(display,0x78)==(slot==3?26u:0xffffffffu),"Ranked admission level or unranked absence changed");
      const auto view=pcr.appearance(row);
      check(!gray(view.name_color),"Actual present sparse slot was grayed");
      check(view.rank_visible==(slot==3) && (slot!=3 || view.rank_range==26),"Original rank range consumed another admitted hopper level");
    }
    presence=std::uint16_t(1u<<3);
    for(unsigned row=0;row<2;++row)check(gray(pcr.appearance(row).name_color)==(results->players[row][0x100]==std::byte{8}),"Departed sparse slot stayed bright or current slot was lost");
    presence.reset();for(unsigned row=0;row<2;++row)check(gray(pcr.appearance(row).name_color),"Unknown current membership fabricated admission presence");
    check(*results==before,"Dynamic source presence changed retained numeric report");++cases;
  }
  check(cases==7,"Original PCR family coverage incomplete");
  std::cout<<"PASS seven original family carriers/numeric columns, source-valid304byte options, sparse current/departed/unknown membership, admitted rank metadata; declared component banks, natural source/GPU/ONLINE flow tested separately\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
