#pragma once
// Native TU1.5 type hooks, entered at 10DB40 (physics live support unit):
// whether every hook of an object's type agrees.
//
//   type_hooks::agree   10DB40(EBX = object handle; first, second) -> AL, RET 8
//
// The object's type (signed byte +0xAA) names its type definition (the
// table at 474B90), whose hooks (+0x84, a list ended by zero) each may
// carry a function (+0x2C). While all agree, each function is asked
// (through the unit's dispatch: the handle, the first and the second
// argument pushed) and its AL is its answer; a hook without a function
// leaves the answer as it is. No hooks agree.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_type_hooks.hpp) maps the role to the original instruction and
// owns the ABI of the hooks; the handle, the second argument, the type
// definition and the hook index, which the original keeps in EBX, EBP, ESI
// and EDI across them, are a Kept, refreshed from the registers each
// returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_type_hooks {
using native_object_update::at;
using native_object_update::widen8;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t objects=0x4ecbd0u;       // DataArray* of 12-byte rows (+8 the object)
inline constexpr std::uint32_t types=0x474b90u;         // type definitions, by object type
}
inline constexpr std::uint32_t datum_data=0x44u,row_size=12u,row_object=8u,object_type=0xaau,hooks=0x84u,hook_function=0x2cu;

namespace type_hooks {
enum class Access {objects,object_data,object,type,second,definition,first_hook,hook,function,first,next_hook};
enum class Outcome {no_hooks,agreed,refused};
struct Kept {std::uint32_t handle,second,definition,index;};   // EBX, EBP, ESI, EDI
// Frame: argument(k) the k-th argument's slot now; ask(kept, function, first) -> AL, refreshes kept;
// refused() a function skipped once a hook refused.
template<class Memory,class Frame> Outcome agree(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto data=memory.load32(object_data,at(memory.load32(objects,globals::objects),datum_data));
    const auto object_address=memory.load32(object,data+row_size*(handle&0xffffu)+row_object);
    const auto type_value=widen8(memory.load8(type,at(object_address,object_type)));
    Kept kept{handle,memory.load32(Access::second,frame.argument(2u)),0u,0u};
    kept.definition=memory.load32(definition,globals::types+4u*type_value);
    auto hook_value=memory.load32(first_hook,at(kept.definition,hooks));
    if(!hook_value)return Outcome::no_hooks;
    bool agreed=true;
    auto slot=at(kept.definition,hooks);
    while(hook_value) {
        const auto function_value=memory.load32(function,at(memory.load32(hook,slot),hook_function));
        if(function_value) {
            if(agreed)agreed=frame.ask(kept,function_value,memory.load32(first,frame.argument(1u)))&0xffu;
            else frame.refused();
        }
        ++kept.index;
        slot=at(kept.definition,hooks)+4u*widen16(kept.index&0xffffu);
        hook_value=memory.load32(next_hook,slot);
    }
    return agreed?Outcome::agreed:Outcome::refused;
}
} // namespace type_hooks
} // namespace halo2::native_type_hooks
