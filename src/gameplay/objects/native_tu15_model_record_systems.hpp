#pragma once
// Native TU1.5 first-person model record, entered at 168D76 (sandbox special
// unit): one of the 0xD0C-byte records 168DED gathers, a model and its node
// matrices.
//
//   model_record::fill   168D76(EAX = model tag, ECX = node count, EDX = node matrices;
//                               object, mode, transform, node map, record)
//
// The record holds the model tag (+0), the object (+4), the mode (+8) and
// up to 64 52-byte node matrices (+0xC). With a node map, 16B240 (the node
// gather) fills them from the matrices in the model's node order. Without
// one, the first count matrices (capped at 64) are taken in order: through
// 145560 with a transform (the transform, the source matrix and the
// destination), else copied as REP MOVSD copies them (13 dwords each), in
// the direction the string direction flag gives; the byte tail (REP MOVSB)
// is always empty, as the matrices are whole dwords.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the arguments the original rereads; the
// boundary adapter (native_tu15_model_record.hpp) maps the role to the
// original instruction and owns the ABI of the children (16B240, 145560).
// The values the original keeps in registers across 145560 are a Kept,
// refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_model_record {
using native_object_update::at;
using native_object_update::signed_value;
namespace record {inline constexpr std::uint32_t model=0u,object=4u,mode=8u,matrices=0xcu;}
inline constexpr std::uint32_t capacity=0x40u,matrix_size=0x34u,matrix_words=13u;
// The arguments, by their index.
namespace argument {inline constexpr std::uint32_t object=0u,mode=1u,transform=2u,node_map=3u,record=4u;}

namespace model_record {
enum class Access {node_map_test,object,record_address,object_store,mode,model_store,mode_store,node_map,transform_push,
    transform_test,transform,copy_read,copy_write};
enum class Outcome {gathered,transformed,empty,copied};
// The original's ESI, EDX and EDI across 145560: the destination, the
// source matrix and the matrices left.
struct Kept {std::uint32_t destination,source,remaining;};
// Frame:
//   argument(k)                          an argument's address now
//   gather(transform, destination, capacity, matrices, node map)   16B240 (EAX the model tag)
//   clamp()                              the count capped (PUSH 40; POP ECX)
//   combine(transform, kept)             145560(ECX the transform, EDX the source; the destination pushed), refreshes kept
//   next(kept)                           the next destination, source and count, in their registers
//   copy_started(count) / copy_step() / copy_finished()   the REP MOVSD's registers and direction
template<class Memory,class Frame> Outcome fill(Memory &memory,Frame &frame,std::uint32_t model,std::uint32_t count,std::uint32_t matrices) {
    using enum Access;
    const auto map=memory.load32(node_map_test,frame.argument(argument::node_map));
    const auto object_value=memory.load32(object,frame.argument(argument::object));
    const auto destination=memory.load32(record_address,frame.argument(argument::record));
    memory.store32(object_store,at(destination,record::object),object_value);
    const auto mode_value=memory.load32(mode,frame.argument(argument::mode));
    memory.store32(model_store,at(destination,record::model),model);
    memory.store32(mode_store,at(destination,record::mode),mode_value);
    if(map) {
        const auto map_again=memory.load32(node_map,frame.argument(argument::node_map));
        const auto transform_value=memory.load32(transform_push,frame.argument(argument::transform));
        frame.gather(transform_value,at(destination,record::matrices),capacity,matrices,map_again);
        return Outcome::gathered;
    }
    if(count>capacity)count=frame.clamp();
    if(memory.load32(transform_test,frame.argument(argument::transform))) {
        if(signed_value(count)<=0)return Outcome::empty;
        Kept kept{at(destination,record::matrices),matrices,count};
        do {
            frame.combine(memory.load32(transform,frame.argument(argument::transform)),kept);
            kept.destination+=matrix_size;kept.source+=matrix_size;--kept.remaining;
            frame.next(kept);
        } while(kept.remaining);
        return Outcome::transformed;
    }
    frame.copy_started(count);
    const auto target=at(destination,record::matrices);
    const auto step=frame.copy_step();
    for(std::uint32_t word=0;word<count*matrix_words;++word)
        memory.store32(copy_write,target+word*step,memory.load32(copy_read,matrices+word*step));
    frame.copy_finished();
    return Outcome::copied;
}
} // namespace model_record
} // namespace halo2::native_model_record
