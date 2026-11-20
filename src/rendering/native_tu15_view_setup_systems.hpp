#pragma once
// Native TU1.5 render-view visibility setup: 135690, called in
// source_shadow_frame to populate a RenderView's camera, frustum, plane, and
// frustum-node table before visibility and object submission.
//
//   view_setup::initialize(memory, frame) -> uint32 status
//     1. Read the caller's stack arguments (view, kind, tag_high, source_nodes,
//        threshold, origin, flag_ptr, bias, use_plane) and entry registers
//        (ECX = view_flags, BX = node_count).
//     2. Populate the view header (+4 = NONE, +8 = tag_high, +A = [474C24])
//        and view state (+0x2A60 = view_flags, +0x2A64 = node_count,
//        +0x2A68..+0x2A70 = origin xyz, +0x2A74 = threshold, +0x2A78 = kind,
//        +0x2A84 = bias).
//     3. If use_plane != 0:
//          - set bit 9 (0x200) on view_flags (+0x2A60);
//          - copy the 16-byte plane from source_nodes + 0x74 into view + 0x2A88;
//          - add bias to the plane distance (+0x2A94);
//        otherwise clear bit 9 (~0x200) on view_flags (+0x2A60).
//     4. Copy `node_count` 0x1BC-byte frustum nodes from `source_nodes` to
//        `view->node_buffer + 4` when the two addresses differ, and write
//        `node_count` (16-bit) at `view->node_buffer`.
//     5. If the caller's `flag_ptr` slot (+0x1C) equals NONE (-1), call 11E600
//        with ESI = &view->first_handle (+4) and EDX = bias_bits.
//     6. Store `flag_ptr` at view + 0x2AD0; when nonzero, set bit 0 (0x01) on
//        view_flags (+0x2A60) and zero the 13 dwords at view + 0x2A98..+0x2AC8.
//     7. Choose and store the view result (+0x2A7C) and return it in EAX:
//          - 2 when view->kind == 0 and the low byte of view->flags is negative;
//          - 0 when node_count != 0 and (1.0f <= threshold or view->flags carries
//            bit 8 (0x100));
//          - 1 otherwise.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access) and the boundary adapter
// (native_tu15_view_setup.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_view_setup {
using native_object_update::at;
using native_object_update::none;

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t default_tag_low=0x474c24u; // uint16
inline constexpr std::uint32_t one=0x46a0e4u;             // float 1.0f
}
namespace view_flags {
inline constexpr std::uint32_t has_flag_ptr=0x0001u;
inline constexpr std::uint32_t force_zero_result=0x0100u; // bit 0 of CH
inline constexpr std::uint32_t has_clip_plane=0x0200u;
}
inline constexpr std::uint32_t frustum_node_stride=0x1bcu;

struct SourceNodes {
    std::uint8_t unused_00[0x74];
    std::uint32_t plane[4];         // +0x74..+0x83
};
static_assert(offsetof(SourceNodes,plane)==0x74);

struct RenderView {
    std::uint32_t node_buffer;      // +0x0000: destination buffer (+0 = count16, +4 = nodes[])
    std::uint32_t first_handle;     // +0x0004: initialized to NONE (-1)
    std::uint16_t tag_high;         // +0x0008
    std::uint16_t tag_low;          // +0x000A
    std::uint8_t unused_0c[0x2a54];
    std::uint32_t flags;            // +0x2A60
    std::uint16_t node_count;       // +0x2A64
    std::uint16_t unused_2a66;
    std::uint32_t origin[3];        // +0x2A68..+0x2A73
    std::uint32_t threshold;        // +0x2A74 (float)
    std::uint32_t kind;             // +0x2A78
    std::uint32_t status;           // +0x2A7C (0, 1, or 2)
    std::uint32_t unused_2a80;
    std::uint32_t bias;             // +0x2A84 (float)
    std::uint32_t plane[4];         // +0x2A88..+0x2A97 (+0x2A94 is plane[3] distance)
    std::uint32_t cleared_state[13];// +0x2A98..+0x2AC8
    std::uint32_t unused_2acc;
    std::uint32_t flag_pointer;     // +0x2AD0
};
static_assert(offsetof(RenderView,first_handle)==0x04 &&
              offsetof(RenderView,tag_high)==0x08 &&
              offsetof(RenderView,tag_low)==0x0a &&
              offsetof(RenderView,flags)==0x2a60 &&
              offsetof(RenderView,node_count)==0x2a64 &&
              offsetof(RenderView,origin)==0x2a68 &&
              offsetof(RenderView,threshold)==0x2a74 &&
              offsetof(RenderView,kind)==0x2a78 &&
              offsetof(RenderView,status)==0x2a7c &&
              offsetof(RenderView,bias)==0x2a84 &&
              offsetof(RenderView,plane)==0x2a88 &&
              offsetof(RenderView,cleared_state)==0x2a98 &&
              offsetof(RenderView,flag_pointer)==0x2ad0);

// ------------------------------------------------------------ access roles
enum class Access : unsigned {
    arg_tag_high,arg_threshold,arg_bias,arg_view,store_tag_high,
    global_tag_low,store_tag_low,arg_kind,store_first_handle,
    arg_origin,store_kind,store_flags_initial,store_node_count,
    load_origin_x,store_origin_x,load_origin_y,store_origin_y,load_origin_z,store_origin_z,
    arg_use_plane,store_threshold,store_bias,
    arg_source_plane,store_flags_plane,
    load_plane_0,store_plane_0,load_plane_1,store_plane_1,load_plane_2,store_plane_2,load_plane_3,store_plane_3,
    load_plane_d,store_plane_d,
    arg_source_no_plane,store_flags_no_plane,
    load_node_buffer,load_node_buffer_again,store_buffer_count,
    check_flag_sentinel,load_bias_for_call,
    arg_flag_ptr,store_flag_ptr,
    load_flags_for_clear,store_clear_0,store_flags_with_ptr,
    store_clear_1,store_clear_2,store_clear_3,store_clear_4,store_clear_5,store_clear_6,
    store_clear_7,store_clear_8,store_clear_9,store_clear_10,store_clear_11,store_clear_12,
    check_view_kind,load_flags_low_byte,store_status_two,reload_status_two,
    load_one_constant,load_flags_for_bit8,store_status_zero,store_status_one
};

namespace view_setup {
template<class Memory,class Frame> void initialize(Memory &memory,Frame &frame) {
    using enum Access;
    const auto tag_high=frame.read_arg16(arg_tag_high,0x10u);
    const auto threshold=frame.read_arg_float(arg_threshold,0x1cu);
    const auto bias=frame.read_arg_float(arg_bias,0x24u);
    const auto view=frame.read_view_arg(arg_view);
    memory.store16(store_tag_high,at(view,offsetof(RenderView,tag_high)),tag_high);
    const auto tag_low=memory.load16(global_tag_low,globals::default_tag_low);
    memory.store16(store_tag_low,at(view,offsetof(RenderView,tag_low)),tag_low);
    const auto kind=frame.read_arg32(arg_kind,0x0cu);
    memory.store32(store_first_handle,at(view,offsetof(RenderView,first_handle)),none);
    const auto first_handle_ptr=at(view,offsetof(RenderView,first_handle));
    const auto origin=frame.read_origin_arg(arg_origin,first_handle_ptr);
    memory.store32(store_kind,at(view,offsetof(RenderView,kind)),kind);
    auto flags=frame.entry_flags();
    const auto node_count=frame.entry_node_count();
    memory.store32(store_flags_initial,at(view,offsetof(RenderView,flags)),flags);
    memory.store16(store_node_count,at(view,offsetof(RenderView,node_count)),node_count);

    const auto ox=frame.read_first_origin_x(load_origin_x,origin);
    const auto view_origin=at(view,offsetof(RenderView,origin));
    memory.store32(store_origin_x,view_origin,ox);
    const auto oy=memory.load32(load_origin_y,at(origin,4u));
    memory.store32(store_origin_y,at(view_origin,4u),oy);
    const auto oz=memory.load32(load_origin_z,at(origin,8u));
    memory.store32(store_origin_z,at(view_origin,8u),oz);

    const auto use_plane=frame.read_arg8(arg_use_plane,0x2cu);
    memory.store_float(store_threshold,at(view,offsetof(RenderView,threshold)),threshold);
    memory.store_float(store_bias,at(view,offsetof(RenderView,bias)),bias);

    std::uint32_t source_nodes{};
    if(use_plane) {
        source_nodes=frame.read_arg32(arg_source_plane,0x18u);
        flags|=view_flags::has_clip_plane;
        memory.store32(store_flags_plane,at(view,offsetof(RenderView,flags)),flags);
        const auto src_plane=at(source_nodes,offsetof(SourceNodes,plane));
        const auto dst_plane=at(view,offsetof(RenderView,plane));
        const auto p0=memory.load32(load_plane_0,src_plane);
        memory.store32(store_plane_0,dst_plane,p0);
        const auto p1=memory.load32(load_plane_1,at(src_plane,4u));
        memory.store32(store_plane_1,at(dst_plane,4u),p1);
        const auto p2=memory.load32(load_plane_2,at(src_plane,8u));
        memory.store32(store_plane_2,at(dst_plane,8u),p2);
        const auto p3=memory.load32(load_plane_3,at(src_plane,12u));
        memory.store32(store_plane_3,at(dst_plane,12u),p3);
        const auto pd=memory.load_float(load_plane_d,at(view,offsetof(RenderView,plane)+12u));
        const auto biased_d=frame.add_plane_bias(pd,bias);
        memory.store_float(store_plane_d,at(view,offsetof(RenderView,plane)+12u),biased_d);
    } else {
        source_nodes=frame.read_arg32(arg_source_no_plane,0x18u);
        flags&=~view_flags::has_clip_plane;
        memory.store32(store_flags_no_plane,at(view,offsetof(RenderView,flags)),flags);
    }

    const auto node_buffer=memory.load32(load_node_buffer,at(view,offsetof(RenderView,node_buffer)));
    frame.copy_frustum_nodes(source_nodes,at(node_buffer,4u),node_count,frustum_node_stride);
    const auto node_buffer_again=memory.load32(load_node_buffer_again,at(view,offsetof(RenderView,node_buffer)));
    memory.store16(store_buffer_count,node_buffer_again,node_count);

    if(frame.read_arg32(check_flag_sentinel,0x1cu)==none) {
        const auto bias_bits=frame.read_arg32(load_bias_for_call,0x24u);
        frame.call_init_handle(first_handle_ptr,bias_bits);
    }

    const auto flag_ptr=frame.read_arg32(arg_flag_ptr,0x20u);
    memory.store32(store_flag_ptr,at(view,offsetof(RenderView,flag_pointer)),flag_ptr);
    if(flag_ptr!=0u) {
        const auto cur_flags=memory.load32(load_flags_for_clear,at(view,offsetof(RenderView,flags)));
        const auto clear_base=at(view,offsetof(RenderView,cleared_state));
        memory.store32(store_clear_0,clear_base,0u);
        memory.store32(store_flags_with_ptr,at(view,offsetof(RenderView,flags)),cur_flags|view_flags::has_flag_ptr);
        memory.store32(store_clear_1,at(clear_base,0x04u),0u);
        memory.store32(store_clear_2,at(clear_base,0x08u),0u);
        memory.store32(store_clear_3,at(clear_base,0x0cu),0u);
        memory.store32(store_clear_4,at(clear_base,0x10u),0u);
        memory.store32(store_clear_5,at(clear_base,0x14u),0u);
        memory.store32(store_clear_6,at(clear_base,0x18u),0u);
        memory.store32(store_clear_7,at(clear_base,0x1cu),0u);
        memory.store32(store_clear_8,at(clear_base,0x20u),0u);
        memory.store32(store_clear_9,at(clear_base,0x24u),0u);
        memory.store32(store_clear_10,at(clear_base,0x28u),0u);
        memory.store32(store_clear_11,at(clear_base,0x2cu),0u);
        memory.store32(store_clear_12,at(clear_base,0x30u),0u);
    }

    if(memory.load32(check_view_kind,at(view,offsetof(RenderView,kind)))==0u) {
        const auto low_byte=memory.load8(load_flags_low_byte,at(view,offsetof(RenderView,flags)));
        if(low_byte&0x80u) {
            return frame.finish_status_two(view);
        }
    }

    if(!frame.node_count_is_zero()) {
        const auto one=memory.load_float(load_one_constant,globals::one);
        if(frame.one_not_above_threshold(one,threshold)) {
            return frame.finish_status_zero(view);
        }
        const auto final_flags=memory.load32(load_flags_for_bit8,at(view,offsetof(RenderView,flags)));
        if(final_flags&view_flags::force_zero_result) {
            return frame.finish_status_zero(view);
        }
    }
    return frame.finish_status_one(view);
}
} // namespace view_setup
} // namespace halo2::native_view_setup
