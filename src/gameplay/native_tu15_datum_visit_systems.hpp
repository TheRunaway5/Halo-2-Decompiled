#pragma once
// Native TU1.5 datum visit, entered at 118760 (death cycle unit): a datum's
// list walked with the visitor at 8FDB0.
//
//   datum_visit::visit   118760(word quiet; handle) -> RET 8
//
// Unless quiet, the handle's datum (0x2BC bytes of the data array at
// [4ECC04]) has its list (+0x10, its first entry +0xC) walked by 47860
// (ESI the list, EDI the visitor 8FDB0; the handle and the first entry
// pushed).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_datum_visit.hpp) maps the role to the original instruction
// and owns the ABI of the child.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_datum_visit {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t datums=0x4ecc04u;}   // DataArray* of 0x2BC-byte datums
inline constexpr std::uint32_t datum_data=0x44u,datum_size=0x2bcu,first_entry=0xcu,list=0x10u,visitor=0x8fdb0u;

namespace datum_visit {
enum class Access {quiet,handle,datums,data,first};
// Frame: argument(k) the k-th argument's slot; walk(handle, first, list) 47860.
template<class Memory,class Frame> bool visit(Memory &memory,Frame &frame) {
    using enum Access;
    if(memory.load16(quiet,frame.argument(1u)))return false;
    const auto handle_value=memory.load32(handle,frame.argument(2u));
    const auto array=memory.load32(Access::datums,globals::datums);
    const auto datum=datum_size*(handle_value&0xffffu)+memory.load32(data,at(array,datum_data));
    frame.walk(handle_value,memory.load32(first,at(datum,first_entry)),datum+list);
    return true;
}
} // namespace datum_visit
} // namespace halo2::native_datum_visit
