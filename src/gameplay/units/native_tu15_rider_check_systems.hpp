#pragma once
// Native TU1.5 rider check, entered at 1AE6F0 (combat action unit): what a
// vehicle's riders say to the action target check.
//
//   rider_check::check   1AE6F0(ECX = vehicle; context, result*) -> AL, RET 8
//
// The result byte is cleared first. The unit chain (D2B00, its cursor in the
// frame: the vehicle, then the found unit NONE, its seat 0xFFFF and the
// vehicle's first child +0x10) walks the vehicle's units. A seated unit (its
// seat word not 0xFFFF) hostile to the context (1E7810: the unit's team word
// +0x138) makes the answer one; a seated unit that is not, in a seat whose
// flags (the vehicle definition's 0xB0-byte seats +0x1CC) lack bit 11,
// passes at once: the result byte one and the answer one.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the cursor in the original's frame and
// the arguments it rereads; the boundary adapter (native_tu15_rider_check.hpp)
// maps the role to the original instruction and owns the ABI of the children
// (D2B00, 1E7810). The values the original keeps in callee-saved registers
// across them are a Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_rider_check {
using native_object_update::at;
using native_object_update::widen16;
inline constexpr std::uint32_t none=0xffffffffu,no_seat=0xffffu;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,tag_instances=0x4f0414u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u,seat_size=0xb0u;
namespace object {inline constexpr std::uint32_t tag=0u,first_child=0x10u,team=0x138u;}
namespace definition {inline constexpr std::uint32_t seats=0x1ccu;}
inline constexpr std::uint32_t open_seat_bit=11u;
namespace cursor {inline constexpr std::uint32_t vehicle=0u,unit=4u,seat=8u,next=0xcu;}
namespace argument {inline constexpr std::uint32_t context=0u,result=1u;}

namespace rider_check {
enum class Access {instances,headers,datums,vehicle_address,vehicle_tag,definition,result_argument,cursor_vehicle,result_clear,
    cursor_unit,cursor_seat,datums_again,vehicle_again,first_child,cursor_next,unit_handle,unit_datums,seat,unit_address,unit_team,
    context_argument,seats,seat_flags,result_again,result_store,headers_again};
enum class Outcome {none_hostile,hostile,passed};
// The original's EDI (the object headers, its low word the unit's seat),
// EBP (the vehicle's definition) and BL (the answer) across the children.
struct Kept {std::uint32_t headers,definition,answer;};
// Frame:
//   argument(k), cursor(offset)       an argument's slot, the cursor's field, now
//   next_unit(kept) -> unit           D2B00(ESI the cursor), refreshes kept
//   hostile(team, context, kept) -> AL   1E7810(SI the team, ECX the context), refreshes kept
//   marked(kept)                      the answer one (MOV BL,1)
template<class Memory,class Frame> Outcome check(Memory &memory,Frame &frame,std::uint32_t vehicle) {
    using enum Access;
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    Kept kept{};
    kept.headers=memory.load32(headers,globals::object_headers);
    const auto vehicle_index=header_size*(vehicle&0xffffu);
    const auto vehicle_object=memory.load32(vehicle_address,memory.load32(datums,at(kept.headers,datum_data))+vehicle_index+header_address);
    const auto tag=memory.load32(vehicle_tag,at(vehicle_object,object::tag));
    kept.definition=memory.load32(Access::definition,tag_size*(tag&0xffffu)+instances_address+tag_data);
    const auto result=memory.load32(result_argument,frame.argument(argument::result));
    memory.store32(cursor_vehicle,frame.cursor(cursor::vehicle),vehicle);
    kept.answer=0u;
    memory.store8(result_clear,result,0u);
    memory.store32(cursor_unit,frame.cursor(cursor::unit),none);
    memory.store16(cursor_seat,frame.cursor(cursor::seat),no_seat);
    const auto vehicle_again_object=memory.load32(Access::vehicle_again,
        memory.load32(datums_again,at(kept.headers,datum_data))+vehicle_index+header_address);
    memory.store32(cursor_next,frame.cursor(cursor::next),memory.load32(first_child,at(vehicle_again_object,object::first_child)));
    for(bool more=frame.next_unit(kept)!=0u;more;) {
        const auto unit=memory.load32(unit_handle,frame.cursor(cursor::unit));
        const auto unit_headers=memory.load32(unit_datums,at(kept.headers,datum_data));
        const auto seat_value=memory.load16(seat,frame.cursor(cursor::seat))&0xffffu;
        kept.headers=(kept.headers&0xffff0000u)|seat_value;
        const auto unit_object=memory.load32(unit_address,unit_headers+header_size*(unit&0xffffu)+header_address);
        if(seat_value!=no_seat) {
            const auto team=memory.load16(unit_team,at(unit_object,object::team));
            if(frame.hostile(team,memory.load32(context_argument,frame.argument(argument::context)),kept)&0xffu){kept.answer=1u;frame.marked(kept);}
            else if(!((memory.load32(seat_flags,seat_size*widen16(kept.headers)+memory.load32(seats,at(kept.definition,definition::seats)))>>open_seat_bit)&1u)) {
                memory.store8(result_store,memory.load32(result_again,frame.argument(argument::result)),1u);
                return Outcome::passed;
            }
        }
        more=frame.next_unit(kept)!=0u;
        if(more)kept.headers=memory.load32(headers_again,globals::object_headers);
    }
    return kept.answer&0xffu?Outcome::hostile:Outcome::none_hostile;
}
} // namespace rider_check
} // namespace halo2::native_rider_check
