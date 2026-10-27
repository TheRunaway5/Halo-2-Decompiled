#include "halo2/vista_pcr_owner_state.hpp"
#include "halo2/cache.hpp"
#include "halo2/vista_session_birth.hpp"
#include <algorithm>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> bank,std::size_t offset) {
  std::uint32_t value{};
  for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bank[offset+i])<<(8*i);
  return value;
}
}
VistaPcrInitialFocus vista_pcr_initial_focus(const VistaPcrResults& bank) {
  if(bank.player_count>16 || bank.team_count>16)
    throw FormatError("Vista PCR initial focus outside physical display bank");
  const unsigned pane=bank.team_count?0u:1u;
  const auto count=bank.team_count?bank.team_count:bank.player_count;
  return {pane,count?0u:0xffffffffu};
}
unsigned vista_pcr_tab_step(unsigned pane,int direction,bool teams) {
  if(pane>=6)throw FormatError("PCR tab focus outside its six authored children");
  if(!direction)return pane;
  do {pane=direction>0?(pane+1)%6:(pane+5)%6;} while(!teams && !pane);
  return pane;
}
std::uint32_t vista_pcr_session_identity_slot(std::span<const std::byte> session,
    bool enabled,std::uint32_t manager,const std::array<std::byte,8>& identity) {
  //5AD782 does not consume the session unless the outer owner admits it.
  if(!enabled || manager<1 || manager>5)return 0xffffffffu;
  if(session.size()<vista_session_source_size)
    throw FormatError("Missing Vista PCR live session identity owner");
  const auto state=word(session,0x73a0);
  //5AD423's exact11-byte table only admits states3..8.
  if(state<3 || state>8)return 0xffffffffu;
  const auto mask=word(session,0x1258);
  for(unsigned slot=0;slot<16;++slot)if(mask&(1u<<slot)) {
    const auto at=session.begin()+0x125c+slot*0x128;
    if(std::equal(identity.begin(),identity.end(),at))return slot;
  }
  return 0xffffffffu;
}
VistaPcrVoicePresentation vista_pcr_offline_voice(const VistaPcrResults& bank,
    unsigned row,std::uint8_t lobby,std::span<const std::byte> session,
    bool enabled,std::uint32_t manager) {
  if(row>=bank.player_count || row>=16)
    throw FormatError("Invalid Vista PCR offline voice row");
  if(lobby)return {};
  std::array<std::byte,8> identity;
  std::copy_n(bank.players[row].begin()+0x100,8,identity.begin());
  if(vista_pcr_session_identity_slot(session,enabled,manager,identity)==0xffffffffu)
    return {};
  return {true,2};
}
} // namespace halo2
