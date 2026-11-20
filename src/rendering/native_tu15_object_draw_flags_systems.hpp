#pragma once
// Native TU1.5 render-object draw flags: 135E00, called by 1362A0 for every
// visible object in the render queue before building its model draw call.
//
//   object_draw_flags::compute(row_flags, view) -> uint32 draw flags
//     Translates an object's visibility-queue row flags (DX) and the current
//     view/fog state into the model draw flags returned in EAX:
//       - bit 1  (0x0002) <- row flag bit 12 (0x1000)
//       - bit 2  (0x0004) <- row flag bit 13 (0x2000)
//       - bit 4  (0x0010) <- row flag bit 15 (0x8000)
//       - bit 5  (0x0020) <- row flag bit 14 (0x4000)
//       - bit 14 (0x4000) <- row flag bit 9  (0x0200)
//       - bit 0  (0x0001) <- view->byte[+0x2A60] bit 7 (0x80)
//       - when the global fog mode ([4C6710]) is 3 (planar/atmospheric fog):
//           * bit 8 (0x0100) if a fog tag ([4C67AC]) is present, the fog
//             density ([4C681C]) is ordered-equal to zero ([46A0E8]), row flag
//             bit 10 (0x0400) is clear, and the fog definition's first byte
//             does not have bit 1 (0x02) set;
//           * bit 7 (0x0080) if [4C6824] is zero or row flag bit 10 (0x0400)
//             is set.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads; every access names its
// role (Access) and the boundary adapter (native_tu15_object_draw_flags.hpp)
// maps the role to the original instruction. Registers, flags, the UCOMISS
// comparison and stack slots belong to the adapter.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_draw_flags {
using native_object_update::at;
using native_object_update::element;
using native_object_update::handle_index;
using native_object_update::none;

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t fog_mode=0x4c6710u;       // int32: active fog mode (3 enables planar/fog flags)
inline constexpr std::uint32_t fog_tag=0x4c67acu;        // tag handle of the active planar fog, NONE for none
inline constexpr std::uint32_t fog_density=0x4c681cu;    // float: current planar fog density
inline constexpr std::uint32_t fog_override=0x4c6824u;   // byte: when zero, forces draw flag 0x80
inline constexpr std::uint32_t zero=0x46a0e8u;           // float 0.0
inline constexpr std::uint32_t tag_instances=0x4f0414u;  // TagInstance[]
}
namespace row_flags {
inline constexpr std::uint32_t draw_flag_14=0x0200u;     // maps to draw flag 0x4000
inline constexpr std::uint32_t inside_fog=0x0400u;       // bit 2 of DH: object is inside fog
inline constexpr std::uint32_t shifted_mask=0x2c00u;     // (row >> 2) & 0x2C00 -> bits 1, 2, 4 after >> 9
inline constexpr std::uint32_t bit_14=0x4000u;           // maps to draw flag 0x0020 after >> 9
}
namespace draw_flags {
inline constexpr std::uint32_t view_flag=0x0001u;
inline constexpr std::uint32_t fog_pass=0x0080u;
inline constexpr std::uint32_t skip_fog=0x0100u;
}
struct TagInstance {
    std::uint32_t unused_here[2];
    std::uint32_t definition;       // +8
    std::uint32_t unused_here_c;
};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,definition)==8);
struct FogDefinition {
    std::uint8_t flags;             // +0: bit 1 (0x02) inhibits draw flag 0x0100
};
inline constexpr std::uint8_t fog_definition_no_skip_bit=0x02u;
struct View {
    std::uint8_t unused_here[0x2a60];
    std::uint8_t draw_flag_byte;    // +0x2A60: sign bit (0x80) sets draw flag 0x0001
};
static_assert(offsetof(View,draw_flag_byte)==0x2a60);
inline constexpr std::uint32_t planar_fog_mode=3u;

// ------------------------------------------------------------ access roles
enum class Access : unsigned {
    view_argument,view_flag_byte,fog_mode,fog_tag,fog_density,zero_constant,
    tag_instances,fog_definition,fog_definition_flags,fog_override
};

namespace object_draw_flags {
// Frame (the adapter):
//   row_flags()                       the 16-bit visibility row flags in DX
//   view_argument()                   [ESP+0xC] after saving EBX and ESI
//   density_is_zero(density, zero)    UCOMISS density, zero -> ordered equal
//   finish(flags)                     restore ESI/EBX, return flags in EAX, RET 4
template<class Memory,class Frame> void compute(Memory &memory,Frame &frame) {
    using enum Access;
    const auto row=frame.row_flags();
    auto flags=((((row>>2u)&row_flags::shifted_mask)|(row&row_flags::bit_14))>>9u)|
               ((row&row_flags::draw_flag_14)<<5u);
    const auto view=frame.view_argument();
    if(memory.load8(view_flag_byte,at(view,offsetof(View,draw_flag_byte)))&0x80u)
        flags|=draw_flags::view_flag;
    if(memory.load32(Access::fog_mode,globals::fog_mode)==planar_fog_mode) {
        const auto tag=memory.load32(Access::fog_tag,globals::fog_tag);
        if(tag!=none) {
            const auto density=memory.load_float(Access::fog_density,globals::fog_density);
            const auto zero=memory.load_float(zero_constant,globals::zero);
            if(frame.density_is_zero(density,zero) && !(row&row_flags::inside_fog)) {
                const auto tags=memory.load32(Access::tag_instances,globals::tag_instances);
                const auto definition=memory.load32(fog_definition,
                    at(element(tags,handle_index(tag),sizeof(TagInstance)),offsetof(TagInstance,definition)));
                if(!(memory.load8(fog_definition_flags,at(definition,offsetof(FogDefinition,flags)))&
                     fog_definition_no_skip_bit))
                    flags|=draw_flags::skip_fog;
            }
        }
        const auto override_byte=memory.load8(Access::fog_override,globals::fog_override);
        if(!override_byte || (row&row_flags::inside_fog))
            flags|=draw_flags::fog_pass;
    }
    frame.finish(flags);
}
} // namespace object_draw_flags
} // namespace halo2::native_object_draw_flags
