#pragma once
// Native TU1.5 biped recovery, entered at E52D0 (live support unit): a
// biped's recovery countdown, and its getting up when it ends.
//
//   biped_recovery::update   E52D0(biped) -> AL, RET 4
//
// A biped (through the object headers at [4ECBD0]) with a countdown (byte
// +0x39D) of 1 in physics mode 3 (byte +0x3DC), whose definition (through the
// tag instances at 4F0414) allows recovery (bit 0x400 of its +0x1F0) and
// whose byte +0x10A lacks bit 2, gets up: E7E80 (EAX the biped), then the
// animation request 111B10 (EAX 2; the biped, the four recovery names and two
// zeros pushed); then, the biped read again, one in a seat (word +0x112 not
// NONE) leaves it (C1A70, EAX the biped, 0 pushed) when bit 29 of its +4 is
// set, and that bit is cleared; and a biped now in physics mode 2 has its
// +0x3EC block landed (2031F0, EAX the block). Finally a countdown above 0
// counts down, and the answer is 1; otherwise it is the original's BL (0).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_biped_recovery.hpp) maps the role to the original instruction
// and owns the ABI of the children. The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_biped_recovery {
using native_object_update::at;
using native_object_update::none;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,tag_instances=0x4f0414u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u;
namespace biped {inline constexpr std::uint32_t flags=4u,flags_byte=0x10au,seat=0x112u,countdown=0x39du,physics=0x3dcu,landing=0x3ecu;}
inline constexpr std::uint32_t recovering_mode=3u,landing_mode=2u,held_bit=4u,seated_bit=0x20000000u,recovers=0x400u,
    definition_flags_offset=0x1f0u,request_mode=2u;
// The animation names the recovery requests, in the order the request takes them.
inline constexpr std::uint32_t recovery_names[]{0x7000001u,0x7000101u,0x7000101u,0x7000001u};

namespace biped_recovery {
enum class Access {handle_argument,headers,datums,object_address,countdown,physics,tag,instances,definition,definition_flags,flags_byte,
    headers_again,datums_again,object_again,seat,seated,flags_load,flags_store,physics_again,countdown_again,countdown_store};
enum class Outcome {idle,counting};
// The original's BL (the answer), EBP (the biped's handle), ESI (the header offset, then the biped read again) and EDI (the
// biped) across the children.
struct Kept {std::uint32_t answer,handle,held,biped;};
// Frame:
//   argument()                         the biped's slot now
//   indexing(kept), offset(kept)       AND EAX,FFFF; PUSH ESI; LEA ESI; SHL ESI,2; PUSH EDI
//   counted(countdown, kept), count_tested(countdown)   XOR BL,BL; TEST AL,AL
//   definition_tested(flags), byte_tested(flags), last_tested(countdown), handle_tested(handle), seated_tested(flags)
//   getting_up(kept), request(kept)    E7E80(EAX = biped) and 111B10(EAX = 2; the biped, the names, 0, 0), refresh kept
//   leave_seat(kept)                   C1A70(EAX = biped; 0), refreshes kept
//   cleared(value)                     the AND's flags
//   land(block, kept)                  2031F0(EAX = block), refreshes kept
//   counted_down(value)                DEC AL
//   answered(kept)                     MOV AL,BL: the answer when the countdown is over
template<class Memory,class Frame> Outcome update(Memory &memory,Frame &frame) {
    using enum Access;
    Kept kept{};
    kept.handle=memory.load32(handle_argument,frame.argument());
    kept.held=header_size*(kept.handle&0xffffu);
    frame.indexing(kept);
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    frame.offset(kept);
    kept.biped=memory.load32(object_address,datums_address+kept.held+header_address);
    const auto count=memory.load8(Access::countdown,at(kept.biped,biped::countdown));
    kept.answer=0u;
    frame.counted(count,kept);
    if(count && memory.load8(Access::physics,at(kept.biped,biped::physics))==recovering_mode) {
        const auto tag_value=memory.load32(tag,kept.biped);
        const auto instances_address=memory.load32(instances,globals::tag_instances);
        const auto definition_address=memory.load32(definition,tag_size*(tag_value&0xffffu)+instances_address+tag_data);
        const auto definition_value=memory.load32(definition_flags,at(definition_address,definition_flags_offset));
        frame.definition_tested(definition_value);
        if(definition_value&recovers) {
            const auto byte=memory.load8(flags_byte,at(kept.biped,biped::flags_byte));
            frame.byte_tested(byte);
            if(!(byte&held_bit)) {
                frame.last_tested(count);
                if(count==1u) {
                    frame.getting_up(kept);
                    frame.request(kept);
                    frame.handle_tested(kept.handle);
                    if(kept.handle!=none) {
                        const auto again=memory.load32(datums_again,at(memory.load32(headers_again,globals::object_headers),datum_data));
                        kept.held=memory.load32(object_again,again+kept.held+header_address);
                        if(memory.load16(Access::seat,at(kept.held,biped::seat))!=0xffffu) {
                            const auto flags_value=memory.load32(seated,at(kept.held,biped::flags));
                            frame.seated_tested(flags_value);
                            if(flags_value&seated_bit)frame.leave_seat(kept);
                            const auto value=memory.load32(flags_load,at(kept.held,biped::flags))&~seated_bit;
                            frame.cleared(value);
                            memory.store32(flags_store,at(kept.held,biped::flags),value);
                        }
                    }
                    if(memory.load8(physics_again,at(kept.biped,biped::physics))==landing_mode)
                        frame.land(at(kept.biped,biped::landing),kept);
                }
            }
        }
    }
    const auto now=memory.load8(countdown_again,at(kept.biped,biped::countdown));
    frame.count_tested(now);
    if(!now) {
        frame.answered(kept);
        return Outcome::idle;
    }
    const auto value=(now-1u)&0xffu;
    frame.counted_down(value);
    memory.store8(countdown_store,at(kept.biped,biped::countdown),value);
    return Outcome::counting;
}
} // namespace biped_recovery
} // namespace halo2::native_biped_recovery
