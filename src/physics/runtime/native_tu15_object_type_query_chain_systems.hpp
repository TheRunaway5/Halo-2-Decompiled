#pragma once
// Native TU1.5 object type query chain, entered at 10DDE0 (physics live
// support unit): the types in an object's type chain are asked in order
// until one answers.
//
//   object_type_query_chain::ask   10DDE0(EDI = object handle, EBX = extra; argument) -> AL
//
// The object's type (the signed byte +0xAA) names its type definition
// (474B90); each type definition in that definition's type chain (+0x84,
// null-terminated) with a query (+0x48) is asked (object handle, argument,
// extra); the first nonzero answer ends the walk and is the result, which
// is otherwise 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_type_query_chain.hpp) maps the role to the original
// instruction and owns the ABI of the queries. The chain position and the
// values the original keeps in callee-saved registers across them are a
// Kept, refreshed from the registers each query returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_type_query_chain {
using native_object_update::ObjectTypeDefinition;
using native_object_update::at;
using native_object_update::object_of;
namespace globals {using native_object_update::globals::object_type_definitions;}
inline constexpr std::uint32_t object_type=0xaau,type_query=0x48u;

namespace object_type_query_chain {
enum class Access {object_table,object_datums,object_address,type,definition,argument,chain_entry,query};
struct Kept {std::uint32_t position;};   // ESI: the position in the type chain
// Frame:
//   argument()                 the argument's slot
//   started(kept)              PUSH ESI; XOR AL,AL; LEA ESI,[definition+84]
//   ask(query, kept)           the query(handle, argument, extra); refreshes kept; returns AL
//   stepped(kept)              ADD ESI,4; TEST AL,AL
template<class Memory,class Frame> std::uint32_t ask(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto object=object_of(memory,handle,object_table,object_datums,object_address);
    const auto type_value=std::uint32_t(std::int32_t(std::int8_t(memory.load8(type,at(object,object_type)))));
    const auto type_definition=memory.load32(definition,globals::object_type_definitions+4u*type_value);
    (void)memory.load32(argument,frame.argument());
    Kept kept{at(type_definition,offsetof(ObjectTypeDefinition,type_chain))};
    frame.started(kept);
    std::uint32_t answer=0;
    for(;;) {
        const auto entry=memory.load32(chain_entry,kept.position);
        if(!entry)break;
        if(const auto query_value=memory.load32(query,at(entry,type_query)))answer=frame.ask(query_value,kept);
        frame.stepped(kept);
        kept.position+=4u;
        if(answer)break;
    }
    return answer;
}
} // namespace object_type_query_chain
} // namespace halo2::native_object_type_query_chain
