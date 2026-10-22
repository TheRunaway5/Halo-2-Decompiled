#include "halo2/vista_slayer_publication.hpp"

namespace halo2 {
bool vista_slayer_activation_publication(std::uint8_t world_phase_byte,
    std::uint32_t recipient,
    std::span<std::byte,vista_slayer_activation_storage_size> storage) noexcept {
  if(world_phase_byte==4)return false;
  const auto word=[&](std::size_t offset,std::uint32_t value) {
    for(unsigned i=0;i<4;++i)storage[offset+i]=std::byte((value>>(i*8))&0xffu);
  };
  //45B05D defines eight dwords and one ushort, not0x34 bytes. Preserve padding.
  word(0,2);word(4,0);
  for(std::size_t offset=8;offset<=0x18;offset+=4)word(offset,0xffffffffu);
  word(0x1c,0);
  storage[0x20]=std::byte{0xff};storage[0x21]=std::byte{0xff};
  word(8,recipient); //510C4D, after construction and before publication.
  return true;
}

bool vista_player_activation_query_admitted(std::uint8_t world_phase_byte,
                                           std::uint16_t flags_before_query) noexcept {
  return world_phase_byte!=4 && (flags_before_query&2u)==0;
}

VistaPlayerActivationUpdate vista_player_activation_after_query(
    std::uint16_t flags_after_query,std::uint8_t query_al) noexcept {
  VistaPlayerActivationUpdate update;
  update.flags=flags_after_query;
  if((flags_after_query&1u)==query_al)return update;
  if(query_al!=0) {
    update.flags=std::uint16_t(flags_after_query|1u);
    update.calls[update.call_count++]=VistaPlayerActivationService::engine_activation;
  } else {
    update.flags=std::uint16_t(flags_after_query&0xfffeu);
  }
  update.calls[update.call_count++]=VistaPlayerActivationService::player_notification;
  return update;
}
} // namespace halo2
