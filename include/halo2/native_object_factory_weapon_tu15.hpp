#pragma once
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <string_view>

// Native weapon construction owners (track "weapon"): the type table's item
// (FA820 -> FD100) and weapon (102AD0) "new" slots and everything 102AD0
// executes, the weapon state change 10A980 and the animation manager state
// owner 1D5320 with its lookups. See docs/NATIVE_OBJECT_FACTORY.md and
// docs/native_object_factory_tracks/weapon.md. Every owner replicates its
// TU1.5 routine's stores (address, width, value, order) over the live world
// memory; unported callees are named refusals.
namespace halo2::native_object_factory_tu15 {
using NativeObjectFactoryFp=NativeCampaignScriptFloatingPoint;

// B96B0 (object scale): `scale_bits` with `seconds_bits` interpolation time.
// Production binds native_tu15_object_scale with its BC380/BF050 services.
using ItemScaleService=std::function<void(std::uint32_t object,std::uint32_t scale_bits,std::uint32_t seconds_bits)>;
// FA820: item "new" slot. Body +4 bit 12, +164/+168 (0, 1.0), +14C/+154
// NONE, +158..+160 from the vector at [0x474D10], +150 the game tick
// ([[0x51DF54]+8]), then FD100. Always true.
bool item_new(SourcePhysicsContactMemory&,std::uint32_t object,const NativeObjectFactoryFp&,const ItemScaleService&);
// FD100: the definition's scale (+C4 in game mode 2 ([[0x4F3218]+8]), else
// +C8) clamped to [0.5, 3.0]; 1.0 when not positive or when body +12C bit 0
// is set. Applied through B96B0 with zero interpolation time.
void item_restore_scale(SourcePhysicsContactMemory&,std::uint32_t object,const NativeObjectFactoryFp&,const ItemScaleService&);

// 102AD0: weapon "new" slot. Resets the weapon state, enters state 0
// through 10A980, then the magazine (+2C0 rows of 0x5C), barrel (+2C8) and
// trigger (+2D0) blocks and the +250..+258 fields. Always true.
bool weapon_new(SourcePhysicsContactMemory&,std::uint32_t object,const NativeObjectFactoryFp&);
// 10A980: marks the object for update (B9390), then (unless `force`) admits
// only states >= the current +178 state 1..2, starts the state's animation
// on the manager at body+[+12A] and records the state. False only when the
// current state rejects the change.
bool weapon_set_state(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t state,bool force,
    const NativeObjectFactoryFp&);
// 106FF0: body +154 when body +12C bit 0 is set, else NONE.
std::uint32_t weapon_owner(const SourcePhysicsContactMemory&,std::uint32_t object);

namespace weapon_animation {
using M=SourcePhysicsContactMemory;
using Fp=NativeObjectFactoryFp;
// A word of an original caller stack frame (an out-parameter, or a local
// passed by address). Native callers keep it private; an oracle kernel may
// bind it to a declared bank address so the original's frame stores are
// compared in order. Binding loads the word and never changes values.
class FrameWord {
public:
  FrameWord()=default;
  explicit FrameWord(std::uint32_t value):value_(value){}
  void bind(M& memory,std::uint32_t address);
  std::uint32_t operator()()const{return value_;}
  void set(std::uint32_t value);
  void set_byte(std::uint8_t value);  // byte store to the word's first byte
private:
  std::uint32_t value_=0xffffffffu;
  M* mirror_=nullptr;
  std::uint32_t address_=0;
};

// ---- Animation key names on MCC caches ----
//
// MCC caches name a few first-person animation keys in their own StringID
// namespace: the staged reload and melee states (TU1.5's reload_enter
// 0x0C0005AE is the cache's 0x0C0005BA) and the dual weapon class. The
// translated route joins them only where the original copies a key into a
// graph search (src/gameplay/weapons/source_weapon_animation_query_symbols_
// tu15.cpp, which also checks every cache name below when the world loads):
//
// - Each 1D5320 call opens a query scope named by its caller (QueryOrigin).
//   The innermost scope decides; an `other` scope hides any outer one.
// - In a first_person_state scope (169863's call), 1E3420's action search
//   (the key pushed at 1E34A3) uses the cache name of the state key the call
//   entered with, when that key is one of first_person_state_names.
// - In a first_person_state or first_person_channel scope (16AD45's 1D37D0
//   and 1D3C80 lookups), 2A0100's class search (the key pushed at 2A019A)
//   uses the cache name of the dual class.
//
// Only those searched copies change. The keys a caller keeps stay in
// TU1.5's namespace: the manager's +70..+7C, the channel key, 2A0100's
// searched-key outputs and 1D45A0's dual exception. The live world sets a
// QueryNamespace for MCC banks; without one (the oracle's default) every key
// is unchanged.
struct QueryName {std::uint32_t tu15,mcc;std::string_view name;};
inline constexpr std::array<QueryName,12> first_person_state_names{{
  {0x0e000607u,0x0e000613u,"melee_strike_1"},
  {0x0e000608u,0x0e000614u,"melee_strike_2"},
  {0x0e000609u,0x0e000615u,"melee_strike_3"},
  {0x0e00060au,0x0e000616u,"melee_strike_4"},
  {0x0a0005bbu,0x0a0005c7u,"melee_dash"},
  {0x0b0005b2u,0x0b0005beu,"melee_lunge"},
  {0x130005bcu,0x130005c8u,"melee_dash_airborne"},
  {0x140005b3u,0x140005bfu,"melee_lunge_airborne"},
  {0x0c0005aeu,0x0c0005bau,"reload_enter"},
  {0x150005afu,0x150005bbu,"reload_continue_empty"},
  {0x140005b0u,0x140005bcu,"reload_continue_full"},
  {0x0b0005b1u,0x0b0005bdu,"reload_exit"},
}};
inline constexpr QueryName dual_class_name{0x0400054bu,0x04000557u,"dual"};

// While a QueryNamespace with `mcc` is alive on this thread, the query scopes
// below search by cache names.
class QueryNamespace {
public:
  explicit QueryNamespace(bool mcc);
  ~QueryNamespace();
  QueryNamespace(const QueryNamespace&)=delete;
  QueryNamespace& operator=(const QueryNamespace&)=delete;
private:
  bool previous_;
};
// Who entered 1D5320 (or made a lookup the translated route scopes).
enum class QueryOrigin : std::uint8_t {
  other,                 // every other 1D5320 caller: no join
  first_person_state,    // 169863's 1D5320 call (returning to 169A56)
  first_person_channel,  // 16AD45's 1D37D0 and 1D3C80 lookups (16B0D1, 16B0E5, 16B108, 16B128)
};
class QueryScope {
public:
  QueryScope(QueryOrigin origin,std::uint32_t state_key);
  ~QueryScope();
  QueryScope(const QueryScope&)=delete;
  QueryScope& operator=(const QueryScope&)=delete;
  // The key 1E3420 searches an action table for, given the state key.
  static std::uint32_t action_search_key(std::uint32_t key);
  // The key 2A0100 searches a class table for, given the class key.
  static std::uint32_t class_search_key(std::uint32_t key);
private:
  QueryOrigin origin_;
  std::uint32_t state_key_;
  const QueryScope* outer_;
};

// Animation graph ("jmad" data): +30 animations (0x6C rows), +34 mode
// table, +4C/+50 child graphs (0x20 rows, tag at +4), +54 key remaps (8-byte
// rows), +B0 resource rows (20 bytes). Handles: high word animation, low
// word child graph (0xFFFF: the graph itself).
//
// 1E5810: binary search of a {count, rows} tag block for a signed key at
// row +0; the row or 0.
std::uint32_t tag_block_search(const M&,std::uint32_t block,std::uint32_t key,std::uint32_t size);
// 1E3270: child graph `index` (signed) or 0.
std::uint32_t graph_child(const M&,std::uint32_t graph,std::uint32_t index);
// 1E5C80: with animation streaming enabled ([0x51DF20] and [0x51DF21]),
// requests the animation's and variants' resources through the node
// builder's shared resource controller and its bound World requester.
void graph_require_resident(const M&,std::uint32_t graph,std::uint32_t handle);
// 1E3150: the handle's animation row (0x6C bytes) or 0, after 1E5C80.
std::uint32_t graph_animation(const M&,std::uint32_t graph,std::uint32_t handle);
// 1D4E50: resets the manager's three 0x20-byte channels.
void manager_reset_channels(M&,std::uint32_t manager);
// 1D45A0: resolves the four state keys (0x7000101: the manager's current
// key at +70..+7C; defaults 0x6000086/0x7000083/0x30000D9/0x400000C) and
// remaps the second through the graph's +54 table.
void resolve_keys(const M&,std::uint32_t manager,std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d,
    FrameWord (&out)[4]);
// 2A0100 iterator state (+0 graph, +4/+8/+C keys, +10 fallback state, +14/+18
// mode rows).
struct ModeIterator {FrameWord graph,key0,key1,key2,state,row1,row2;};
// 2A0100: the next mode row over the 0x30000D9 fallback combinations; 0
// when exhausted. `out*` receive the keys searched (the class table is
// searched for QueryScope::class_search_key of `out1`).
std::uint32_t mode_iterate(const M&,ModeIterator&,FrameWord& out0,FrameWord& out1,FrameWord& out2);
// 1E33D0: the mode row for three keys or 0.
std::uint32_t mode_entry(const M&,std::uint32_t graph,std::uint32_t k0,std::uint32_t k1,std::uint32_t k2);
// 1E5E40 / 1E5F20 / 1E5FB0: resource prefetch of a mode row, of its five
// fallback rows, and of those rows in the graph and its children. Enabled
// resource rows use the bound 126200 controller in load/unload order.
void mode_prefetch(const M&,std::uint32_t graph,std::uint32_t k0,std::uint32_t k1,std::uint32_t k2,bool load,bool unload);
void mode_prefetch_fallbacks(const M&,std::uint32_t graph,std::uint32_t k0,std::uint32_t k1,std::uint32_t k2,bool load,bool unload);
void graph_prefetch(const M&,std::uint32_t graph,std::uint32_t k0,std::uint32_t k1,std::uint32_t k2,bool load,bool unload);
// 1D6430: graph_prefetch of the manager's resolved keys (fourth 0x7000101).
void manager_prefetch(const M&,std::uint32_t manager,std::uint32_t k0,std::uint32_t k1,std::uint32_t k2,bool load,bool unload);
// 1E3420: the animation handle for `key3` (searched as
// QueryScope::action_search_key) under the first mode row (2A0100 order)
// that has it; NONE when `r1` is NONE or none matches. Unless `r1` is NONE
// the searched keys go to the non-null `out*`, then the result.
void animation_lookup(const M&,std::uint32_t graph,std::uint32_t key3,std::uint32_t r0,std::uint32_t r1,std::uint32_t r2,
    FrameWord* out0,FrameWord* out1,FrameWord* out2,FrameWord& result);
// 1D46A0: resolves the keys into `resolved`, copies them to `selected`, finds
// the animation (flags bit 1: 0x400000C retry; bit 2: mode-row fallbacks)
// and prefetches. `result` receives the handle.
bool choose_animation(const M&,std::uint32_t manager,FrameWord (&selected)[4],FrameWord (&resolved)[4],
    std::uint32_t a0,std::uint32_t a1,std::uint32_t a2,std::uint32_t a3,std::uint32_t flags,FrameWord& result);
// 1E3320: the physical-query handle of a mode row matching the nine keys.
void physical_query_find(const M&,std::uint32_t row,FrameWord& out,std::uint32_t key,std::uint8_t b0,std::uint8_t b1,
    std::uint32_t k4,std::uint32_t k5,std::uint8_t c0,std::uint8_t c1,std::uint8_t c2);
// 1E4B70: physical-query search over the mode rows. A match continues into
// the 1E4C39 selection path, which is a named refusal.
void physical_query_select(const M&,std::uint32_t graph,std::uint32_t key1,FrameWord& out,std::uint32_t key0,
    std::uint32_t key2,std::uint32_t state,std::uint32_t a5,std::uint32_t a6,std::uint32_t a7,std::uint32_t a8,
    std::uint32_t a9,std::uint32_t a10,std::uint32_t a11);
// 1D5050: the manager's physical-query blend state ([0x47C1B5] policy).
void physical_query_state(const M&,std::uint32_t manager,FrameWord& out,std::uint32_t a1,std::uint32_t a2,
    FrameWord& flag,const Fp&);
// 1D4F80: the chained-state selector of [0x47C1B8] for `key`.
void physical_query_selector(const M&,std::uint32_t key,FrameWord& result,std::uint32_t manager,std::uint32_t a1,
    std::uint32_t a2,std::uint32_t a3,FrameWord& mapped);
// Channel records (0x20 bytes): +0 graph tag, +4 handle, +8 key, +C/+D
// bytes, +E word, +10/+11 flags, +12 start flags, +16 event bits, +18 rate,
// +1C frame.
bool channel_stopped(const M&,std::uint32_t record);       // E8D50
std::uint32_t channel_animation(const M&,std::uint32_t record);  // B8780
std::uint32_t channel_valid(const M&,std::uint32_t record);  // B8760
bool channel_finished(const M&,std::uint32_t record);      // 1CF5B0
// 1CE720: starts a channel; false for a NONE graph or animation.
bool channel_start(M&,std::uint32_t record,std::uint32_t graph,std::uint32_t flags,std::uint32_t handle,
    std::uint32_t key,std::uint8_t b0,std::uint8_t b1,std::uint8_t b2);
// 1CEC60: the channel's event bits between its frame and the animation end.
void channel_mark_events(M&,std::uint32_t record,const Fp&);
// 1CE810: clamps and stores the channel frame, the at-end flag, then 1CEC60.
void channel_set_frame(M&,std::uint32_t record,std::uint32_t frame_bits,const Fp&);
// 1E5880: the animation's +30 alias in the handle's high word.
void variant_alias(const M&,std::uint32_t graph,FrameWord& out,std::uint32_t handle);
// 1E58E0: weighted random variant (+1C weights, +32 chain); advances the
// primary or secondary seed at [[0x4F3CD8]].
void random_variant(M&,std::uint32_t graph,FrameWord& out,std::uint32_t handle,bool primary,const Fp&);
// 1D3660: random_variant on the manager's graph (+6C bit 1 primary seed).
void manager_variant(M&,std::uint32_t manager,FrameWord& out,std::uint32_t handle,const Fp&);
// 1D36C0: starts `record` on the manager's graph (start flags: 0x10 random
// variant, 4 frame 0, 8 unit rate).
bool manager_start_channel(M&,std::uint32_t manager,std::uint32_t flags,std::uint32_t record,std::uint32_t handle,
    std::uint32_t key,std::uint8_t b0,std::uint8_t b1,std::uint8_t b2,const Fp&);
// 1D5320: enters an animation state (keys a0, a1, a2, key3) with `flags`
// and channel start flags; records the keys at +70..+7C. Its lookups run in
// a query scope of `origin` (see QueryNamespace).
bool manager_set_state(M&,std::uint32_t manager,std::uint32_t key3,std::uint32_t a0,std::uint32_t a1,std::uint32_t a2,
    std::uint32_t flags,std::uint32_t start_flags,const Fp&,QueryOrigin origin=QueryOrigin::other);
}
}
