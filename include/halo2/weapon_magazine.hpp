#pragma once
#include <cstdint>
#include <array>
#include <functional>
#include <span>

namespace halo2 {
// Literal weapon+224 magazine record; unrelated retained fields stay intact.
struct WeaponMagazineState {
  std::int16_t mode{}, ticks{}, unknown4{}, reserve{}, loaded{}, recharge{};
  std::int16_t transfer_ticks{}, other_ticks{};
};
static_assert(sizeof(WeaponMagazineState)==16);
struct WeaponMagazineDefinition {
  std::uint32_t flags{};
  std::int16_t capacity{}, reload_amount{};
  float chamber_seconds{};
  std::int16_t maximum_reserve{};
  std::int16_t recharge_per_second{};
  std::uint32_t reload_effect=0xffffffffu,chamber_effect=0xffffffffu;
};
struct WeaponAmmunitionView {
  std::uint32_t datum=0xffffffffu,definition=0xffffffffu;
  std::span<WeaponMagazineState> magazines;
  std::span<const WeaponMagazineDefinition> definitions;
};
struct WeaponAmmunitionContext {
  WeaponAmmunitionView current;
  std::array<WeaponAmmunitionView,4> inventory;
  bool has_unit{},infinite{}; // Actual retained unit and10A1D0/10B0F0 result.
};
// Complete104AA0/1059C0 and104D30/105C50 shared reserve consumers. Duplicate
// slot references and signed-short stores retain their original ordering.
std::int32_t weapon_reserve_available(const WeaponAmmunitionContext &,unsigned magazine,
    bool self_only=false);
bool weapon_reserve_spend(WeaponAmmunitionContext &,unsigned magazine,std::int32_t amount,
    const std::function<void()> &changed);
struct WeaponMagazineTransferServices {
  // Actual104AA0/1059C0 and104D30/105C50 ammunition owner. The latter
  // owns reserve/infinite-ammunition stores, rather than this consumer.
  std::function<std::int32_t()> available;
  std::function<void(std::int32_t amount)> spend;
  std::function<void()> changed; // Actual A8AB0/B1F00 notification.
};
struct WeaponMagazineTransitionServices {
  std::function<std::uint32_t(unsigned)> activity_symbol; //1680B3/169493.
  std::function<std::int32_t(std::uint32_t,unsigned section)> timing; //16A06A/16B44A.
  std::function<void(unsigned)> activity; //10A140/10B060, after all stores.
};
struct WeaponMagazineServices {
  WeaponMagazineTransferServices transfer;
  WeaponMagazineTransitionServices transition;
  std::function<bool()> idle; //106030/106F50 trigger0/trigger1/own-state gate.
  std::function<void(unsigned)> request_state; //109A60/10A980, result ignored.
  std::function<void(std::uint32_t)> effect; //107B50/108A70 with NONE prior/A0/B0.
  std::function<bool(unsigned)> fire_admitted; //1066F0/107610, argument3=true.
  std::function<bool()> request_reload; //101D50/102C70 actual unit action request.
};
// Complete106D40/107C60 transfer and109C30/10AB50 mode-transition consumers.
// Required external owners throw when absent. These do not admit a reload.
void weapon_magazine_transfer(WeaponMagazineState &,const WeaponMagazineDefinition &,
    std::uint32_t item_flags,const WeaponMagazineTransferServices &);
void weapon_magazine_transition(WeaponMagazineState &,const WeaponMagazineDefinition &,
    std::int16_t requested,std::int16_t tick_rate,const WeaponMagazineTransitionServices &);
// Complete106C30/107B50 reload admission,106E70/107D90 chamber admission,
// and101E30/102D50 magazine update, composing the consumers above.
bool weapon_magazine_reload(WeaponMagazineState &,const WeaponMagazineDefinition &,
    std::int16_t weapon_type,unsigned magazine,std::int16_t tick_rate,const WeaponMagazineServices &);
void weapon_magazine_chamber(WeaponMagazineState &,const WeaponMagazineDefinition &,
    unsigned magazine,std::int16_t tick_rate,const WeaponMagazineServices &);
bool weapon_magazine_step(WeaponMagazineState &,const WeaponMagazineDefinition &,
    std::uint32_t item_flags,std::uint16_t input_flags,unsigned magazine,
    std::int16_t tick_rate,const WeaponMagazineServices &);
} // namespace halo2
