#include "halo2/source_king_replication.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
void source_engine_replication_initialize(SourceEngineReplicationBank& bank) noexcept {
  bank={};
  for(unsigned i=5;i<14;++i)bank.words[i]=0xffff;
}
void source_engine_replication_difference(const SourceEngineReplicationState& state,
    std::uint32_t requested,std::uint32_t& changed,SourceEngineReplicationBank& bank) noexcept {
  if((requested&1) && bank.words!=state.words_06_21) {
    bank.words=state.words_06_21;changed|=1;
  }
  // Compare before narrowing. Negative / large phases can remain dirty after
  // writing the low byte, just as MOVZX/CMP SI/MOV BL do in the source owner.
  if((requested&2) && std::uint16_t(bank.phase)!=std::uint16_t(state.phase)) {
    bank.phase=std::uint8_t(state.phase);changed|=2;
  }
  if((requested&4) && std::uint8_t(bank.game_over!=0)!=state.game_over) {
    bank.game_over=std::uint8_t(state.game_over!=0);changed|=4;
  }
  if((requested&8) && bank.round!=state.round) {bank.round=state.round;changed|=8;}
  if((requested&0x10) && bank.clock_seconds!=state.clock_seconds) {
    bank.clock_seconds=state.clock_seconds;changed|=0x10;
  }
}
bool source_engine_replication_apply(SourceEngineReplicationState& state,
    std::uint32_t mask,const SourceEngineReplicationBank& bank,const SourceEngineReplicationServices& services) {
  if(mask&1) {
    state.words_06_21=bank.words;
    if(!services.refresh_player_properties)throw std::runtime_error("Replication player-property owner unavailable");
    services.refresh_player_properties();
    if(!services.refresh_recording)throw std::runtime_error("Replication recording refresh owner unavailable");
    services.refresh_recording();
  }
  if((mask&2) && std::int32_t(state.phase)!=std::int32_t(bank.phase)) {
    if(!services.phase_changed)throw std::runtime_error("Replication phase-change owner unavailable");
    services.phase_changed(bank.phase);
    state.phase=bank.phase;
  }
  if(mask&4) {
    if(!services.begin_game_over)throw std::runtime_error("Replication game-over owner unavailable");
    services.begin_game_over();
  }
  if(mask&8)state.round=bank.round;
  if(mask&0x10)state.clock_seconds=bank.clock_seconds;
  return true;
}
void source_king_replication_initialize(SourceKingReplicationBank& bank) noexcept {
  bank={};source_engine_replication_initialize(bank.shared);
}
void source_king_replication_difference(const SourceEngineReplicationState& shared,
    const SourceKingState& king,std::uint32_t& mask,SourceKingReplicationBank& bank) noexcept {
  std::uint32_t changed=0;
  if(mask&0x1f)source_engine_replication_difference(shared,mask&0x1f,changed,bank.shared);
  if((mask&0x20) && std::int32_t(bank.hill)!=king.hill) {
    bank.hill=std::bit_cast<std::int16_t>(std::uint16_t(king.hill));changed|=0x20;
  }
  if((mask&0x40) && bank.occupants!=king.occupants) {bank.occupants=king.occupants;changed|=0x40;}
  mask=changed;
}
bool source_king_replication_apply(SourceEngineReplicationState& shared,SourceKingState& king,
    std::span<const SourceKingMarker> markers,std::uint32_t mask,const SourceKingReplicationBank& bank,
    const SourceEngineReplicationServices& services) {
  bool result=true;
  if(mask&0x1f)result=source_engine_replication_apply(shared,mask&0x1f,bank.shared,services);
  if((mask&0x20) && king.hill!=std::int32_t(bank.hill)) {
    king.hill=bank.hill;
    source_king_build_hill(king,markers,king.hill);
  }
  if((mask&0x40) && king.occupants!=bank.occupants)king.occupants=bank.occupants;
  return result;
}
} // namespace halo2
