#pragma once
#include "halo2/sound_driver_master_gain.hpp"
#include <string>

namespace halo2 {
// Literal immutable owned-image operands and strings, never runtime state.
struct SoundLoopingConstants {
  std::uint32_t one_bits,minimum_db_bits,game_allocator_method;
  std::string pool_name,empty_selector,dialogue_selector;
  std::array<std::string,54> class_names;
};
// Addresses of retained mutable globals, not their initial values. Every
// referenced pool, arena, engine, fade and driver must have its actual owner.
struct SoundLoopingBindings {
  std::uint32_t allocator_pointer,arena_base_pointer,arena_used_pointer,arena_crc_pointer;
  std::uint32_t pool_pointer,state_pointer,engine_pointer,engine_table,class_fades_pointer,class_fades_flag;
  std::uint32_t driver_pointer,master_output_globals;
};
struct SoundLoopingServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Complete alternate source allocator method, only if the actual vtable
  // method differs from the paired125E50 game-arena owner implemented here.
  // Missing ownership stops, rather than allocating an unrelated host buffer.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> source_allocate;
};
// Complete18A200 constructor CPU sequence, including its fixed16D070 layout
// and scalar1656A0 allocation checksum. Source CRC lookup-cache bookkeeping is
// not represented as native game memory. Pool/state storage remains real.
void source_sound_looping_construct(const SoundLoopingBindings &,
    const SoundLoopingConstants &,const SoundLoopingServices &,
    const SoundDriverStartupObserver & = {});
// Complete18A2A0 CPU reset,16D220 and actual2230E0 composition, followed by
// complete18A790. Missing driver backing stops at its original gain store;
// separately produced driver ABI backing does not establish natural startup.
void source_sound_looping_reset(const SoundLoopingBindings &,
    const SoundLoopingConstants &,const SoundDriverMasterGainConstants &,
    const SoundDriverFloatingPointContext &,const SoundLoopingServices &,
    const SoundDriverStartupObserver & = {});
// Complete18A790 engine-gated class setup and223A90 substring selection.
// This entry can return independently of an unreturned driver/map owner.
void source_sound_looping_engine_reset(const SoundLoopingBindings &,
    const SoundLoopingConstants &,const SoundLoopingServices &,
    const SoundDriverStartupObserver & = {});
// Actual223960/2239B0 class storage constructors on the existing game arena.
void source_sound_looping_class_construct(const SoundLoopingBindings &,
    const SoundLoopingServices &,const SoundDriverStartupObserver & = {});
void source_sound_looping_class_reset(const SoundLoopingBindings &,
    const SoundLoopingServices &,const SoundDriverStartupObserver & = {});
// Complete18A9C0/16D2C0 allocation/registration. It preserves source salts,
// bitmap/count/cursor and raw request fields; it does not start playback.
std::uint32_t source_sound_looping_register(const SoundLoopingBindings &,
    std::uint32_t definition,std::uint32_t object,std::uint16_t marker,
    std::uint32_t parameter,const SoundLoopingServices &,
    const SoundDriverStartupObserver & = {});
// Complete223A90 direct ABI for independently measured class-fade controls.
void source_sound_looping_class_gain(const SoundLoopingBindings &,
    const SoundLoopingConstants &,std::string_view selector,
    std::uint32_t gain_db_bits,std::uint32_t seconds_bits,
    const SoundLoopingServices &,const SoundDriverStartupObserver & = {});
} // namespace halo2
