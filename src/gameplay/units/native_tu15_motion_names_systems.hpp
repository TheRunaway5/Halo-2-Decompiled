#pragma once
// Native TU1.5 motion names, entered at E53E0 (live support unit): the
// animation mode and action names a biped's motion asks for.
//
//   motion_names::choose   E53E0(EAX = biped, EBX = mode*, EDI = action*; motion, flag, flag*) -> RET C
//
// A biped (through the object headers at [4ECBD0]) whose word +0x34C is 1
// is left alone. Otherwise its physics mode (byte +0x3DC) 5 asks for the
// mode name 0x50000CB, and 2 for 0x6000542 when its animation (the block at
// its int16 offset +0x12A) has that mode (1D3BD0, EAX the block, ESI the
// name). An action that is not the idle name 0x400000C is left as it is and
// ends the choice. Otherwise a motion of 1 to 11 names the action through
// the original's jump table (E54C0), and a motion that is not 0 stores the
// flag byte at flag*.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the jump table's entry; the boundary
// adapter (native_tu15_motion_names.hpp) maps the role to the original
// instruction and owns the ABI of the child. The outputs and the motion the
// original keeps in EBX, EDI and EBP across it are a Kept, refreshed from
// the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_motion_names {
using native_object_update::at;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,action_table=0xe54c0u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace biped {inline constexpr std::uint32_t still=0x34cu,physics=0x3dcu,animation=0x12au;}
inline constexpr std::uint32_t still_value=1u,climbing=2u,crouched=5u,motions=11u;
// String ids: the mode and action names.
namespace names {inline constexpr std::uint32_t crouch_mode=0x50000cbu,climb_mode=0x6000542u,idle=0x400000cu;}
// The table's targets, as the unit's canonical_branch names them, and the action each names.
struct Case {std::uint32_t target,action;};
inline constexpr Case cases[]{{0xe5457u,0x400000cu},{0xe545fu,0xa000017u},{0xe5467u,0x9000016u},{0xe546fu,0xa000014u},
    {0xe5477u,0x9000015u},{0xe547fu,0xd000035u},{0xe5487u,0xc000034u},{0xe548fu,0xd000032u},{0xe5497u,0xc000033u},
    {0xe549fu,0x800001cu},{0xe54a7u,0xa00001du}};
namespace argument {inline constexpr std::uint32_t motion=0u,flag=1u,flag_out=2u;}

namespace motion_names {
enum class Access {headers,datums,object_address,still,motion_argument,physics,crouch_store,animation,climb_store,action,table_entry,
    action_store,flag_out_argument,flag_argument,flag_store};
enum class Outcome {still,not_idle,named,unnamed,flagged};
// The original's EBX (mode*), EDI (action*) and EBP (the motion) across 1D3BD0.
struct Kept {std::uint32_t mode_out,action_out,motion;};
// Frame:
//   argument(k)                          the arguments' slots now
//   saving(), restoring()                EBP around the choice
//   physics_tested(mode)                 SUB EAX,2; (SUB EAX,3)
//   has_mode(block, name, kept) -> AL    1D3BD0(EAX = block, ESI = name), ESI saved around it, refreshes kept
//   indexed(motion)                      LEA EAX,[EBP-1]; CMP EAX,A
//   unknown_case()                       a table entry that is no case (throws)
//   motion_tested(motion)                TEST EBP,EBP
template<class Memory,class Frame> Outcome choose(Memory &memory,Frame &frame,std::uint32_t handle,Kept kept) {
    using enum Access;
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    const auto object=memory.load32(object_address,datums_address+header_size*(handle&0xffffu)+header_address);
    const bool still_now=memory.load16(Access::still,at(object,biped::still))==still_value;
    frame.saving();
    kept.motion=memory.load32(motion_argument,frame.argument(argument::motion));
    auto outcome=Outcome::still;
    if(!still_now) {
        const auto mode=memory.load8(Access::physics,at(object,biped::physics));
        frame.physics_tested(mode);
        if(mode==crouched)memory.store32(crouch_store,kept.mode_out,names::crouch_mode);
        else if(mode==climbing) {
            const auto block=object+widen16(memory.load16(animation,at(object,biped::animation)));
            if(frame.has_mode(block,names::climb_mode,kept)&0xffu)memory.store32(climb_store,kept.mode_out,names::climb_mode);
        }
        if(memory.load32(action,kept.action_out)!=names::idle)outcome=Outcome::not_idle;
        else {
            outcome=Outcome::unnamed;
            const auto index=kept.motion-1u;
            frame.indexed(index);
            if(index<=motions-1u) {
                const auto target=memory.load32(table_entry,globals::action_table+4u*index);
                const Case *chosen=nullptr;
                for(const auto &c:cases)if(c.target==target)chosen=&c;
                if(!chosen)frame.unknown_case();
                else {
                    memory.store32(action_store,kept.action_out,chosen->action);
                    outcome=Outcome::named;
                }
            }
            frame.motion_tested(kept.motion);
            if(kept.motion) {
                const auto out=memory.load32(flag_out_argument,frame.argument(argument::flag_out));
                memory.store8(flag_store,out,memory.load8(flag_argument,frame.argument(argument::flag)));
                outcome=Outcome::flagged;
            }
        }
    }
    frame.restoring();
    return outcome;
}
} // namespace motion_names
} // namespace halo2::native_motion_names
