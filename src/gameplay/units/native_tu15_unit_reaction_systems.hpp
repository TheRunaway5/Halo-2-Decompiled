#pragma once
// Native TU1.5 unit reaction, entered at E71A0 (physics live support unit,
// reached through the dispatch): a unit's reaction counter and its pulse.
//
//   unit_reaction::update   E71A0(EDI = unit)
//
// A unit (through the object headers at [4ECBD0]) in mode 0 (int8 +0x34A)
// with its counter (int8 +0x39B) above 0 counts on; once the count's time
// (the count times the seconds per tick, +4 of [51DF54]) passes [46A178] the
// counter clears and the unit pulses at its level. In mode 1 the counter is
// 1, in any other 0. Otherwise the unit pulses when its animation block (at
// the int16 offset +0x12A) has one of two ids (+0x7C: 900000E, A00000F) and
// is idle there: its reference unusable (+0 NONE or word +6 NONE) or its
// weight (+0x1C) times [46A17C] exactly 0 (UCOMISS/LAHF). A pulse hands
// E72E0 the unit, 3 and a value (EDX 2): 1, or at its level (int8 +0x39F)
// times [456690] clamped to 0 to 1 (COMISS, in the original's order); the
// level then clears.
//
// The single-precision arithmetic is the original's scalar SSE, in its order,
// on the live MXCSR. This is the game logic only. The game's memory is the
// retained TU1.5 bank, reached through a Memory object with typed loads and
// stores; every access names its role (Access), including the original's
// frame (the value, in the slot of the ECX it pushes); the boundary adapter
// (native_tu15_unit_reaction.hpp) maps the role to the original instruction
// and owns the ABI of E72E0. The unit's address the original keeps in ESI
// across it is a Kept, refreshed from the registers it returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_reaction {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen8;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace scalar=native_record_fade::scalar;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,clock=0x51df54u,limit=0x46a178u,one=0x46a0d0u,
    level_scale=0x456690u,weight_scale=0x46a17cu;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,pulse_kind=3u,pulse_mode=2u,zero_float=0u;
namespace unit {inline constexpr std::uint32_t block=0x12au,mode=0x34au,counter=0x39bu,level=0x39fu;}
namespace block {inline constexpr std::uint32_t handle=0u,high=6u,weight=0x1cu,id=0x7cu;}
namespace ids {inline constexpr std::uint32_t first=0x900000eu,second=0xa00000fu;}
namespace clock {inline constexpr std::uint32_t seconds_per_tick=4u;}
// The frame's slot, by offset from the return address: the pulse's value, in the pushed ECX.
namespace local {inline constexpr std::int32_t value=-4;}

namespace unit_reaction {
enum class Access {headers,datums,object_address,mode,counter,clock,counter_store,tick,limit,counter_reset,counter_set,counter_clear,
    block_offset,block_id,block_handle,block_high,block_weight,weight_scale,one,value_store,level,level_scale,value_level,
    value_ceiling,value_floor,value_load,level_clear};
enum class Outcome {unmatched,active,pulsed,expired};
// The original's ESI (the unit's address) across E72E0.
struct Kept {std::uint32_t unit;};
// Frame:
//   saving(n), indexing(unit), local(offset)     PUSH ECX; XORPS XMM1; MOV EAX,EDI; AND; LEA; PUSH ESI
//   mode_tested(mode), counter_tested(count), counted(count) -> count + 1   XOR DL,DL; SUB/DEC EAX; TEST AL,AL; INC AL
//   converted(value, xmm0), multiplied(value, xmm0)   the CVTSI2SS and MULSS results in XMM0's low lane
//   compared(a, b, unordered), expired()          COMISS/UCOMISS (LAHF; TEST AH,44 for the weight); MOV DL,1
//   block_found(offset, unit, id), handle_tested(handle), high_tested(high), weight_loaded(weight)
//   pulse_tested(pulse, one), pulse(unit, value, kept)   TEST DL,DL; XMM2; PUSH value, 3, unit; MOV EDX,2; CALL E72E0
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame,std::uint32_t unit_handle) {
    using enum Access;
    Kept kept{};
    frame.saving(0u);
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    frame.indexing(unit_handle);
    kept.unit=memory.load32(object_address,datums_address+header_size*(unit_handle&0xffffu)+header_address);
    const auto mode_value=widen8(memory.load8(Access::mode,at(kept.unit,unit::mode)));
    frame.mode_tested(mode_value);
    bool pulse=false,checked=true;
    if(mode_value==0u) {
        const auto count=memory.load8(counter,at(kept.unit,unit::counter));
        frame.counter_tested(count);
        if(std::int8_t(count)>0) {
            const auto clock_address=memory.load32(Access::clock,globals::clock);
            const auto next=frame.counted(count);
            memory.store8(counter_store,at(kept.unit,unit::counter),next);
            const auto ticks=scalar::from_integer(widen8(next));
            frame.converted(ticks);
            const auto tick_value=memory.load32(tick,at(clock_address,clock::seconds_per_tick));
            const auto elapsed=sse::multiply(ticks,tick_value);
            frame.multiplied(elapsed);
            const auto limit_value=memory.load32(limit,globals::limit);
            frame.compared(elapsed,limit_value,false);
            if(!sse::not_above(elapsed,limit_value)) {
                memory.store8(counter_reset,at(kept.unit,unit::counter),0u);
                frame.expired();
                pulse=true;checked=false;
            }
        }
    } else if(mode_value==1u)memory.store8(counter_set,at(kept.unit,unit::counter),1u);
    else memory.store8(counter_clear,at(kept.unit,unit::counter),0u);
    if(checked) {
        // The animation block: one of the two ids, idle there.
        const auto offset=widen16(memory.load16(block_offset,at(kept.unit,unit::block)));
        const auto id=memory.load32(block_id,offset+kept.unit+block::id);
        const auto block_address=offset+kept.unit;
        frame.block_found(offset,kept.unit,id);
        if(id!=ids::first && id!=ids::second)return Outcome::unmatched;
        const auto handle=memory.load32(block_handle,at(block_address,block::handle));
        frame.handle_tested(handle);
        if(handle!=none) {
            const auto high=memory.load16(block_high,at(block_address,block::high));
            frame.high_tested(high);
            if(high!=0xffffu) {
                const auto weight=memory.load32(block_weight,at(block_address,block::weight));
                frame.weight_loaded(weight);
                const auto scaled=sse::multiply(weight,memory.load32(weight_scale,globals::weight_scale));
                frame.multiplied(scaled);
                frame.compared(scaled,zero_float,true);
                if(!scalar::ordered_equal(scaled,zero_float))return Outcome::active;
            }
        }
    }
    // The pulse's value: 1, or the level's clamped to 0 to 1.
    const auto one_value=memory.load32(one,globals::one);
    frame.pulse_tested(pulse,one_value);
    memory.store32(value_store,frame.local(local::value),one_value);
    if(pulse) {
        const auto level_value=widen8(memory.load8(level,at(kept.unit,unit::level)));
        const auto converted=scalar::from_integer(level_value);
        frame.converted(converted);
        const auto scaled=sse::multiply(converted,memory.load32(level_scale,globals::level_scale));
        frame.multiplied(scaled);
        frame.compared(zero_float,scaled,false);
        memory.store32(value_level,frame.local(local::value),scaled);
        if(sse::not_above(zero_float,scaled)) {
            frame.compared(scaled,one_value,false);
            if(!sse::not_above(scaled,one_value))memory.store32(value_ceiling,frame.local(local::value),one_value);
        } else memory.store32(value_floor,frame.local(local::value),zero_float);
    }
    frame.pulse(unit_handle,memory.load32(value_load,frame.local(local::value)),kept);
    memory.store8(level_clear,at(kept.unit,unit::level),0u);
    return pulse?Outcome::expired:Outcome::pulsed;
}
} // namespace unit_reaction
} // namespace halo2::native_unit_reaction
