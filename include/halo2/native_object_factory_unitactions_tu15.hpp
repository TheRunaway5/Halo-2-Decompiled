#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

// Native TU1.5 unit action system (track "unitactions"): the 0x473D28 action
// table, its dispatchers (E8E50 request, E8E10 typed request, E8D80 update,
// E8EB0 stop, E8F10 interrupt), the action callbacks ported so far, unit
// place's seat walk CECF0 and the vehicle exit script cores (11D990
// vehicle_unload, 11D670 unit_exit_vehicle, 27EAB0 ai_vehicle_exit). See
// docs/native_object_factory_tracks/unitactions.md. Each owner replicates its
// original routine over the live world memory: stores keep the original
// address, width, value and order. Callees that are not native yet are named
// refusals (std::runtime_error "Native object factory: <owner> ..."), never a
// call into original code.
namespace halo2::native_object_factory_tu15::unit_actions {
using Fp=NativeTu15PhysicsFrameFp;
inline constexpr std::uint32_t action_count=60;

// ---- 0x473D28 (TU1.5 .data, literal; checked against the XBE by the oracle) ----
// Per action type: the descriptor address and its {start, update, stop,
// interrupt} callbacks (0 where the descriptor has none).
struct Descriptor{std::uint32_t address,start,update,stop,interrupt;};
const std::array<Descriptor,action_count>& action_table();

// The 0x20-byte request E8E50 receives in EDI (a caller stack block): +0 type,
// +4/+5 flag bytes read by the start callbacks. Callers that leave bytes
// uninitialized (11D670 modes 1..3) pass zero there.
struct Request {
  std::array<std::uint8_t,0x20> bytes{};
  std::uint32_t type() const{return std::uint32_t(bytes[0])|std::uint32_t(bytes[1])<<8|std::uint32_t(bytes[2])<<16|std::uint32_t(bytes[3])<<24;}
  static Request of(std::uint32_t type,std::uint8_t flag4=0,std::uint8_t flag5=0) {
    Request r;for(unsigned n=0;n<4;++n)r.bytes[n]=std::uint8_t(type>>(8u*n));r.bytes[4]=flag4;r.bytes[5]=flag5;return r;
  }
};

// ---- Dispatchers ----
// E8E50 (EAX unit, EDI request): B9390 wake, the type's start callback, then
// (a live unit whose body +13C player is set) the 1EFA70 action history.
// Returns the start callback's AL.
std::uint8_t request(SourcePhysicsContactMemory&,std::uint32_t unit,const Request&,const Fp&);
// E8E10 (ECX type, stack unit): E8E50 with a zeroed request of `type`.
std::uint8_t request_type(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t type,const Fp&);
// E8D80 (stack unit): each active action (block +4 bit set) with an update
// callback runs it; a false result clears its bit. Returns whether any
// update returned true.
std::uint8_t update(SourcePhysicsContactMemory&,std::uint32_t unit,const Fp&);
// E8EB0 (ECX unit, EDI type): the stop callback, then the action bit cleared.
void stop(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t type,const Fp&);
// E8F10 (ECX unit, EDI type): the interrupt callback, then the bit cleared.
void interrupt(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t type,const Fp&);
// E9530 (EAX unit, ECX type): the action's bit in the action block.
bool active(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t type);

// ---- Action callbacks (natives the dispatchers bind by address) ----
// EAF00 (start of actions 8 and 0x12, weapon ready): 113190, CFB80, the
// interrupts of 0/0xA/0x1B/0x16, EAA60 and EAC70, each native up to its
// first unported callee (106670 from E9910, BBAC0 from E9E50, EAB70, 112420
// or D2DB0 from EAC70).
std::uint8_t weapon_ready_start(SourcePhysicsContactMemory&,std::uint32_t unit,const Request&,const Fp&);
// EB190 (8/0x12 interrupt): inactive actions return; active actions lower
// their held weapon (105200 refusal), or refresh readiness through EAC70.
void weapon_ready_interrupt(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t type,const Fp&);
// EB360 (9/0x13 start): empty hands return false; armed hands reach the
// native interrupt sequence, with named 106810/EA9B0 frontiers.
std::uint8_t weapon_put_away_start(SourcePhysicsContactMemory&,std::uint32_t unit,const Request&,const Fp&);
// 116920: clear the two aim weapon channels in the original store order.
void weapon_channels_clear(SourcePhysicsContactMemory&,std::uint32_t unit);
// D0520: interrupt weapon actions, request 0x13, then clear both channels.
void seat_weapon_actions_reset(SourcePhysicsContactMemory&,std::uint32_t unit,const Fp&);

// ---- Weapon readiness owners (EAF00's callees) ----
// 111D10 (EAX unit, ESI/EDX out): the animation manager (body + word +12A)
// plays (graph +68, channel graph +0 and channel handle word +6 set): mode
// key +70 to `mode`, state key +7C to `state`, AL 1; else both NONE, AL 0.
std::uint8_t unit_animation_keys(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t& mode,
    std::uint32_t& state);
// 116720 (EAX unit, EDI key): the aim block (body + word +33E) channel keys
// +A4/+C4 or the playing manager state (111D10) equal `key`. Returns AL.
std::uint8_t unit_animation_state_active(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t key);
// C9590 (EAX unit): C9550's listed mode, or manager state 0x80000C4,
// 0x400004A or 0x50000C3.
bool unit_animation_mode_locked(const SourcePhysicsContactMemory&,std::uint32_t unit);
// 113190 (stack unit): whether the animation state blocks a weapon change.
// C9590 blocks; else the manager state key, then the aim block's +A4 key,
// select "overlay" keys (blocked unless manager +14 bit 4 for a player) or
// "unloaded" keys (blocked unless a player's slot +212 weapon has word
// +17C <= 0 and its +213 weapon none > 0). Returns AL.
std::uint8_t weapon_change_blocked(const SourcePhysicsContactMemory&,std::uint32_t unit);
// 1123B0 (EAX unit, stack key, two class keys): weapon_animation's 1D46A0
// (keys, 0x400000C, flags 2) on the unit's manager finds an animation.
std::uint8_t weapon_animation_available(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t key,
    std::uint32_t class0,std::uint32_t class1);
// CFB80 (stack unit, key; EDX weapon): a vehicle may ready it; a seat
// without flag bit 5 may not; else 1123B0 with the key (NONE/0x7000101: the
// seat's +4 key, 0x7000101 unseated) and the weapon definition's +288/+28C
// classes. A player never readies a definition with +12C bit 29. Returns AL.
std::uint8_t weapon_ready_allowed(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t weapon,
    std::uint32_t key);
// F0E00 (ECX unit, stack index): slot +212 + index holds a weapon whose word
// +16C has bit 1 set and bit 2 clear. Returns AL.
std::uint8_t weapon_slot_flagged(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t index);
// EA970 (interrupt of action 0x16): E9E50 (unit, 1, 0, 0), then action
// block +C = 0. E9E50 without a held object (block +10 NONE) stores block +C
// = 3; with one it clears block +10/+14 and the object's +4 bit 17 and +C0
// bit 1, then needs BBAC0 (a refusal).
void weapon_pickup_interrupt(SourcePhysicsContactMemory&,std::uint32_t unit,const Fp&);
// EAA60 (stack unit, flag4, flag5; BL primary): 1 at once when the current
// slot (+212, or +213 for the secondary) holds no weapon; else 116720 of
// 0x8000025/0xD000539, then 105200, CABC0, F0D50, 116570 and EA9B0, which
// are refusals. Returns AL.
std::uint8_t weapon_select(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint8_t flag4,std::uint8_t flag5,
    bool primary,const Fp&);
// EAC70 (AL primary, stack unit, key, two flags): the desired slot's weapon
// (+216, +217 for the secondary) when no hand holds it, 1123B0 on its classes
// (0x400054B while the other hand's desired slot holds a weapon); then
// EAB70, 112420 and D2DB0 (refusals) and the readiness tail (+212 slot,
// +228 tick, F0E00, 105010, F0D50, CABC0, +1C8..+1E8, 116570).
void weapon_ready_animation(SourcePhysicsContactMemory&,std::uint32_t unit,bool primary,std::uint32_t key,
    std::uint8_t flag0,std::uint8_t flag1,const Fp&);
// E9870 (update of actions 0/1/0xA/0xB): 105950 on the slot's weapon.
std::uint8_t weapon_trigger_update(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t type);
// 105950 (EAX weapon, EBX trigger or -1): a trigger (+224 + 16 * n word) in 1..6.
std::uint8_t weapon_trigger_ready(const SourcePhysicsContactMemory&,std::uint32_t weapon,std::uint32_t trigger);
// E9910 (interrupt of actions 0/1/0xA/0xB): 106670 on the slot's weapon (a refusal).
void weapon_trigger_interrupt(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t type);
// EC750 (interrupt of action 0x1B): action block +2C = NONE.
void action_target_clear(SourcePhysicsContactMemory&,std::uint32_t unit);
// ED110 (start of action 0x1D, vehicle exit): refuses while 0x1D/0x1F/0x20
// run; interrupts 0x1D/0x1C/0x1F/0x20, +258 = 0, then ED060 (seat release:
// D0520 is a refusal for a seated unit), the 0x1D bit and the request's
// immediate flag (block +0 bit 0); an actor's speech 1C4600 is a refusal.
std::uint8_t vehicle_exit_start(SourcePhysicsContactMemory&,std::uint32_t unit,const Request&,const Fp&);
// ED060 (EAX unit): false for a unit without a parent or seat; a seated unit
// needs D0520 (a refusal).
std::uint8_t seat_release(SourcePhysicsContactMemory&,std::uint32_t unit,const Fp&);

// ---- Unit place tail ----
// CECF0 (stack unit): the unit's driver (+248) and gunner (+24C) from its
// seated children (definition +1CC seats, two passes), recursing into each
// child unit; a changed driver needs the AI hook 1C49D0 (a refusal).
void seat_drivers_update(SourcePhysicsContactMemory&,std::uint32_t unit,const Fp&);

// ---- Seat queries ----
// CB440 (EAX object, stack seat word): the child unit seated in `seat`, or NONE.
std::uint32_t seat_occupant(const SourcePhysicsContactMemory&,std::uint32_t object,std::uint16_t seat);
// 219B00 (ECX mapping, EDX object, stack seat): NONE admits every seat; else
// the scenario's mapping rows ([[0x4ECC20]+22C] + 8 * low word, high word
// count) for the object's tag hold the seat's bit.
std::uint8_t seat_mapping_admits(const SourcePhysicsContactMemory&,std::uint32_t mapping,std::uint32_t object,
    std::uint32_t seat);
// CAF20 entries: the seated object, the seat index and its definition row.
struct Seat{std::uint32_t object;std::int16_t seat;std::uint32_t definition;};
// CAF20 (stack object, out, count, max): the seats of a unit (not body +10A
// bit 2) then, recursively, of its child units with body +4 bit 26.
void object_seats(const SourcePhysicsContactMemory&,std::uint32_t object,std::vector<Seat>& out,std::int16_t max);
// 2199C0 (ECX mapping, EDX object, stack filter, occupancy, out, max): the
// CAF20 seats the mapping admits, filtered by seat flags (0: not bit 2, 1:
// bit 3, 2: neither, 3: bit 2, other: any) and occupancy (1: occupied, 2:
// empty, other: any), at most `max`.
std::vector<Seat> mapped_seats(const SourcePhysicsContactMemory&,std::uint32_t mapping,std::uint32_t object,
    std::uint32_t filter,std::uint32_t occupancy,std::int32_t max);

// ---- Script cores ----
// 11D990 (ECX mapping, EDX vehicle; vehicle_unload): action 0x1D for each
// occupied mapped seat's rider. Returns AX, the riders that started it.
std::uint16_t vehicle_unload(SourcePhysicsContactMemory&,std::uint32_t mapping,std::uint32_t vehicle,const Fp&);
// 11D670 (EAX unit, DX mode; unit_exit_vehicle): a seated unit requests 0x1D
// (mode 0), 0x20 (1), 0x20 with flag 1 (2) or 0x1D with flag 1 (3).
void unit_exit_vehicle(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint16_t mode,const Fp&);
// 27EAB0 (ECX AI reference, EBX mapping; ai_vehicle_exit): each actor the
// 27CAA0/27CB30 walk yields (`next_actor` returns the actor row, 0 at the
// end; production binds the script host's walk) whose unit (+18, with +26C
// set) sits in an admitted seat requests 0x1D.
using ActorWalk=std::function<std::uint32_t()>;
void ai_vehicle_exit(SourcePhysicsContactMemory&,std::uint32_t mapping,const ActorWalk& next_actor,const Fp&);
}
