#include "halo2/vista_pcr_publication.hpp"
#include <algorithm>
#include <stdexcept>

namespace halo2 {
namespace {
void size(std::span<std::byte> source) {
  if(source.size()<vista_pcr_source_size)
    throw std::invalid_argument("Incomplete PC PCR retained storage");
}
void word(std::span<std::byte> source,std::size_t at,std::uint32_t value,unsigned width) {
  for(unsigned i=0;i<width;++i)source[at+i]=std::byte((value>>(i*8))&0xffu);
}
constexpr std::array<int,25> admitted_parameters{
  0,1,2,3,4,5,6,7,8,9,10,17,18,29,30,15,90,89,88,76,91,23,25,26,94};
}
VistaPcrPcVariant vista_pcr_slayer_variant(const SourceGameVariant& source) {
  if(source_game_variant_type(source)!=2)
    throw std::invalid_argument("PC PCR native bridge requires admitted Slayer");
  auto reconstructed=source_game_variant_default(0,source_game_variant_title(source));
  for(const auto parameter:admitted_parameters) {
    const auto& d=source_game_variant_descriptors()[unsigned(parameter)];
    if(d.kind==1) {
      source_game_variant_set(reconstructed,parameter,source_game_variant_get(source,parameter));
    } else {
      // Numeric getters intentionally round through float and are not an
      // inverse for every32-bit writer value. Copy the verified field width.
      const auto width=d.kind==3?1u:d.kind==4?2u:4u;
      std::copy_n(source.bytes.begin()+d.offset,width,reconstructed.bytes.begin()+d.offset);
    }
  }
  if(reconstructed.bytes!=source.bytes)
    throw std::invalid_argument("PC PCR Slayer variant contains unverified fields");
  VistaPcrPcVariant result;
  for(unsigned i=0;i<source.bytes.size();++i)result.bytes[i]=std::byte(source.bytes[i]);
  return result;
}
VistaPcrPcVariant vista_pcr_objective_variant(const SourceGameVariant& source) {
  const auto type=source_game_variant_type(source);
  if((type!=1 && type!=9) || !source_game_variant_valid(source))
    throw std::invalid_argument("PC objective PCR requires a valid original CTF/Assault variant");
  // Original PC45B33D defaults and all112 descriptor leaves are byte-identical
  // to the Xbox option schema.469DFD copies this complete selected buffer;
  //40C8CE consumes its type/team flag, never reinterprets the mode union.
  VistaPcrPcVariant result;
  std::transform(source.bytes.begin(),source.bytes.end(),result.bytes.begin(),
      [](std::uint8_t value){return std::byte(value);});
  return result;
}
VistaPcrPcVariant vista_pcr_multiplayer_variant(const SourceGameVariant& source) {
  const auto type=source_game_variant_type(source);
  if(type==2)return vista_pcr_slayer_variant(source);
  if(type==1 || type==9)return vista_pcr_objective_variant(source);
  if((type!=3 && type!=4 && type!=7 && type!=8) || !source_game_variant_valid(source))
    throw std::invalid_argument("PC PCR requires a valid ported original multiplayer family");
  // 232D77 has explicit original Oddball/King/Juggernaut/Territories
  // numeric getters. The already verified304-byte option schema is copied
  // unchanged, as469DFD does; this does not reinterpret their mode unions.
  VistaPcrPcVariant result;
  std::transform(source.bytes.begin(),source.bytes.end(),result.bytes.begin(),
      [](std::uint8_t value){return std::byte(value);});
  return result;
}
void vista_pcr_write_player_result(std::span<std::byte> source,int player,
                                  std::int32_t score,std::int32_t placement) {
  size(source);
  if(source[0]==std::byte{} || source[0x11]!=std::byte{} || player==-1)return;
  if(player<0 || player>=16)throw std::out_of_range("PC PCR player slot");
  const auto at=0x370+std::size_t(player)*0x94;
  if(source[at]==std::byte{})return;
  word(source,at+0x92,std::uint32_t(score),2);
  word(source,at+0x90,std::uint32_t(placement),1);
}
void vista_pcr_write_team_result(std::span<std::byte> source,int team,
                                std::int32_t score,std::int32_t placement) {
  size(source);
  if(source[0]==std::byte{} || source[0x11]!=std::byte{} || team<0 || team>7)return;
  const auto at=0xcb0+std::size_t(team)*0x18;
  if(source[at]==std::byte{})return;
  word(source,at+2,std::uint32_t(std::clamp(score,-32768,32767)),2);
  word(source,at+1,std::uint32_t(placement),1);
}
void vista_pcr_clear_publication(std::span<std::byte> source,std::uint32_t clock) {
  size(source);
  std::fill(source.begin()+0x10,source.begin()+vista_pcr_source_size,std::byte{});
  word(source,4,0,4);word(source,8,clock,4);
}
VistaPcrConsumedSnapshot vista_pcr_publish_consumed_snapshot(const VistaPcrPublicationInput& input) {
  VistaPcrConsumedSnapshot out;
  auto bank=std::span<std::byte>(out.source);
  bank[0]=std::byte(input.recording.enabled);bank[1]=std::byte(input.recording.frozen);
  if(!input.engine_present)return out;
  if(!input.statistics)throw std::invalid_argument("Actual numeric PCR statistics required");
  const auto& variant=input.variant.bytes;
  const auto type=std::to_integer<unsigned>(variant[0x44]);
  if((type!=1 && type!=2 && type!=3 && type!=4 && type!=7 && type!=8 && type!=9) || variant[0x45]!=std::byte{} ||
     variant[0x46]!=std::byte{} || variant[0x47]!=std::byte{})
    throw std::invalid_argument("PC PCR native bridge requires an admitted original multiplayer variant");
  std::copy(variant.begin(),variant.end(),bank.begin()+0x20);
  bank[0x13]=variant[0x48]&std::byte{1};
  for(unsigned i=0;i<16;++i) {
    const auto at=0x370+i*0x94;const auto& player=input.players[i];
    bank[at]=std::byte(player.occupied);
    if(player.occupied) {
      std::copy(player.identity.begin(),player.identity.end(),bank.begin()+at+2);
      std::copy(player.configuration.begin(),player.configuration.end(),bank.begin()+at+0xc);
      vista_pcr_write_player_result(bank,int(i),player.score,player.placement);
    }
    const auto team=0xcb0+i*0x18;
    bank[team]=std::byte(input.teams[i].occupied);
    vista_pcr_write_team_result(bank,int(i),input.teams[i].score,input.teams[i].placement);
  }
  std::size_t at=0xe30;
  for(const auto& player:input.statistics->players)
    for(const auto value:player){word(bank,at,value,2);at+=2;}
  for(const auto& player:input.statistics->pvp)
    for(const auto& opponent:player)
      for(const auto value:opponent){word(bank,at,value,2);at+=2;}
  for(const auto& team:input.statistics->teams)
    for(const auto value:team){word(bank,at,value,2);at+=2;}
  bank[0x10]=std::byte(input.current);bank[0x11]=std::byte(input.finalized);
  return out;
}
std::int16_t vista_pcr_accumulated_player_score(const SourceGameEndStats& statistics,
                                               std::uint32_t player) {
  const auto slot=player&0xffffu;
  if(player==0xffffffffu || slot>=statistics.players.size())
    throw std::invalid_argument("PC PCR score requires an actual native player datum");
  return statistics.players[slot].fields[1];
}
bool vista_pcr_ranking_available(std::uint32_t flags) noexcept {return (flags&0x20u)!=0;}
std::int8_t vista_pcr_single_player_placement(
    std::span<const std::uint32_t> participants,std::uint32_t player,std::uint32_t flags) {
  if(!vista_pcr_ranking_available(flags) || player==0xffffffffu ||
     participants.size()!=1 || participants[0]!=player)
    throw std::invalid_argument("PC PCR rank bridge requires the sole actual FFA participant");
  return 0;
}
} // namespace halo2
