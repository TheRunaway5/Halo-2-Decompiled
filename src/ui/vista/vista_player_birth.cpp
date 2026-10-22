#include "halo2/vista_player_birth.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
namespace {
std::uint32_t read(std::span<const std::byte> b,std::size_t at,unsigned count=4) noexcept {
  std::uint32_t result{};
  for(unsigned i=0;i<count;++i)result|=std::to_integer<std::uint32_t>(b[at+i])<<(i*8);
  return result;
}
void admit(std::span<const std::byte> b) {
  if(b.size()!=0x1188 || read(b,0)!=2 || (read(b,4,1)!=1 && read(b,4,1)!=3 && read(b,4,1)!=5))
    throw FormatError("Vista player birth requires initialized supported multiplayer options");
}
}
std::uint16_t vista_player_birth_flags(std::uint8_t fresh,std::uint8_t left) noexcept {
  return std::uint16_t((fresh?8:0)|(left?2:0));
}
std::uint16_t native_player_birth_flags(std::uint8_t fresh,std::uint8_t left) noexcept {
  return std::uint16_t((fresh?8:0)|(left?2:0));
}
std::uint16_t vista_player_spawn_completed_flags(std::uint16_t flags) noexcept {return flags&0xf3f7u;}
std::uint16_t native_player_spawn_completed_flags(std::uint16_t flags) noexcept {return flags&0xf9f7u;}
VistaPlayerBirthConsumed vista_player_birth_from_initialized_options(
    std::span<const std::byte> options,unsigned slot,std::uint32_t datum) {
  admit(options);
  if(slot>=16 || datum==0xffffffffu || (datum&0xffffu)!=slot)
    throw FormatError("Vista player birth requires the actual native salted slot handle");
  const auto row=options.subspan(0x448+slot*0xd4,0xd4);
  if(read(row,0,1)==0)throw FormatError("Vista player birth slot is absent from completed options");
  VistaPlayerBirthConsumed result;
  result.slot=slot;result.native_datum=datum;
  result.flags=vista_player_birth_flags(1,std::uint8_t(read(row,1,1)));
  result.creation_user=std::uint16_t(read(row,2,2));result.creation_controller=read(row,4);
  std::copy_n(row.begin()+8,6,result.machine.begin());
  std::copy_n(row.begin()+0xe,8,result.identity.begin());
  std::copy_n(row.begin()+0x18,132,result.configuration.begin());
  return result;
}
VistaLocalActivationIdentity vista_player_local_identity_from_initialized_options(
    std::span<const std::byte> options,std::span<const std::byte,6> published) {
  admit(options);
  if(read(options,0x43e,1)==0 ||
      !std::equal(published.begin(),published.end(),options.begin()+0x43f))
    throw FormatError("Vista activation requires the actual published local machine identity");
  VistaLocalActivationIdentity result;
  // Actual5DDC98 jump table: mode1 ->5DDB6B kind1; mode3 ->5DDB74 kind2;
  // mode5 ->5DDB86 kind4. Each admitted full constructor is executed by the oracle.
  const auto mode=read(options,4,1);
  result.world_kind=mode==5?4u:(mode==3?2u:1u);
  std::copy(published.begin(),published.end(),result.machine.begin());
  bool found=false;const auto mask=read(options,0x3d4);
  for(unsigned i=0;i<17;++i)
    if((mask&(1u<<i)) && std::equal(published.begin(),published.end(),options.begin()+0x3d8+i*6)) {
      result.local_index=i;found=true;
    }
  if(!found)throw FormatError("Vista activation machine is absent from completed valid-machine table");
  return result;
}
std::uint8_t vista_player_local_activation_query(const VistaLocalActivationIdentity& owner,
    const VistaPlayerBirthConsumed& player) {
  if((owner.world_kind!=1 && owner.world_kind!=2 && owner.world_kind!=4) || owner.complete_simulation_metadata || player.complete_player_record ||
      owner.local_index>=17 || owner.machine!=player.machine)
    throw FormatError("Vista activation query requires the verified normal local simulation branch");
  return 1;
}
} // namespace halo2
