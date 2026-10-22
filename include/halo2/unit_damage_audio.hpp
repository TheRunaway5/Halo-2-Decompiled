#pragma once
#include "halo2/unit_damage.hpp"
#include "halo2/multiplayer_audio.hpp"
#include <map>

namespace halo2 {
// Original13A720 state bits, consumed by hudg+418 /22BEB5.
std::uint32_t shield_sound_mask(float previous,float health,float shield,
    float maximum_shield,bool dead,bool charging,bool playing=true);
struct DamageVoiceState {
  int decay{},count{},cooldown{},outside_cooldown{},priority{};
};
void advance_damage_voice_state(DamageVoiceState &);
// Ordinary player CA0B0 ->1147E0 selection. Actor vitality bounds, scripted
// dialogue and campaign suppression have separate owners.
std::optional<unsigned> select_player_damage_voice(DamageVoiceState &,
    const UnitDamageVoiceEvent &,int damage_type,bool special,float first_limit,
    float second_limit,std::uint32_t &random,bool source_mode_four=false);
class NativeUnitDamageAudio {
  struct ContinuationSnapshot;
  struct ContinuationPrepared;
public:
  class Checkpoint {
  public:
    Checkpoint()=default;
  private:
    std::shared_ptr<const ContinuationSnapshot> data_;
    friend class NativeUnitDamageAudio;
  };
  class PreparedReplacement {
  public:
    PreparedReplacement();
    ~PreparedReplacement();
    PreparedReplacement(PreparedReplacement&&) noexcept;
    PreparedReplacement& operator=(PreparedReplacement&&) noexcept;
    PreparedReplacement(const PreparedReplacement&)=delete;
  private:
    std::unique_ptr<ContinuationPrepared> data_;
    friend class NativeUnitDamageAudio;
  };
  NativeUnitDamageAudio(const Cache &,MultiplayerAudio &,std::uint32_t &random);
  ~NativeUnitDamageAudio();
  NativeUnitDamageAudio(const NativeUnitDamageAudio&)=delete;
  NativeUnitDamageAudio& operator=(const NativeUnitDamageAudio&)=delete;
  // Opt in before retaining any playback handle; Audio must already be
  // enrolled. Ordinary users preserve their existing producer/ID behavior.
  void enroll_continuation();
  Checkpoint checkpoint() const;
  PreparedReplacement prepare_restore(const Checkpoint&,
      const MultiplayerAudio::PreparedReplacement&) const;
  bool ready(const PreparedReplacement&,
      const MultiplayerAudio::PreparedReplacement&) const noexcept;
  bool commit(PreparedReplacement&,
      const MultiplayerAudio::PreparedReplacement&) noexcept;
  void advance(std::uint64_t tick);
  void hud(unsigned view,std::uint32_t unit,bool dead,float health,float shield,
      float maximum_shield,bool charging,bool elite);
  void damage(const BipedDefinition &,const UnitDamageVoiceEvent &);
  void remove(std::uint32_t unit);
private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
};
}
