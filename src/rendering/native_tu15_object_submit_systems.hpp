#pragma once
// Native TU1.5 render-object submission: 1362A0, called by 136230 for every
// entry in the visibility queue.
//
//   object_submit::run(memory, frame)
//     1. Determines force_flag = (view->pass_kind == 2) || (is_retry != 0).
//     2. Looks up object_handle = view->object_queue->handles[slot_index],
//        resolves the object from [4ECBD0], reads its bounding sphere
//        (center at +0x30..+0x38, radius at +0x3C), and stores object_handle,
//        position, and radius into the 0x1E4-byte RenderModelSubmission record.
//     3. Computes camera-plane signed distance (z*pz + y*py + x*px + pd),
//        takes its magnitude (clamped up to [46A178]), computes projected
//        screen size ([4C66F0] / dist * radius * [46A11C] * [4C6848]), and
//        computes the 80-bit x87 Euclidean camera distance scaled by [4C65EC].
//     4. Calls 4BAE0 (object_fade) to fill submission.fade_byte and
//        local_shadow_fade, then calls 135E00 (object_draw_flags) with the
//        queue's row_flags[slot_index] to obtain submission.render_flags.
//     5. Initializes submission sort/override fields, attaches the queue's
//        visibility index when row_flags bit 11 (0x0800) is clear and the
//        index is valid (extra_flags=0x40 vs 0x01), and calls 4B5C0
//        (source_model_build) to populate the model geometry sections.
//     6. Sets extra_flags |= 0x208; on retry clears skip_children and forces
//        render_flags = (render_flags & ~2) | 4; otherwise when skip_children
//        is set in pass_kind==1 clears bit 2 of render_flags.
//     7. If int8(submission.fade_byte) <= 0, marks bit 0 of
//        queue->row_flags[slot_index] (culled) and returns. Otherwise, unless
//        (layer_flags & 2) && (view->pass_kind != 0), calls 1365C0
//        (model_hierarchy) and returns submission.skip_children in AL.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access) and the boundary adapter
// (native_tu15_object_submit.hpp) maps the role to the original instruction.
#include "rendering/native_tu15_model_hierarchy_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_submit {
using native_object_update::at;
using native_object_update::element;
using native_object_update::handle_index;
using native_object_update::none;
using native_model_hierarchy::ObjectDatumRow;
using native_model_hierarchy::ObjectTableHeader;
using native_model_hierarchy::RenderModelSubmission;

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t projection_scale=0x46a11cu;   // float
inline constexpr std::uint32_t min_plane_distance=0x46a178u; // float
inline constexpr std::uint32_t camera_pos_x=0x4c65c0u;       // float
inline constexpr std::uint32_t camera_pos_y=0x4c65c4u;       // float
inline constexpr std::uint32_t camera_pos_z=0x4c65c8u;       // float
inline constexpr std::uint32_t distance_scale=0x4c65ecu;     // float
inline constexpr std::uint32_t plane_x=0x4c6640u;            // float
inline constexpr std::uint32_t plane_y=0x4c664cu;            // float
inline constexpr std::uint32_t plane_z=0x4c6658u;            // float
inline constexpr std::uint32_t plane_d=0x4c6664u;            // float
inline constexpr std::uint32_t screen_scale=0x4c66f0u;       // float
inline constexpr std::uint32_t lod_bias=0x4c6848u;           // float
inline constexpr std::uint32_t object_table=0x4ecbd0u;       // ObjectTableHeader*
}
namespace pass_kinds {
inline constexpr std::uint32_t primary=0u;
inline constexpr std::uint32_t secondary=1u;
inline constexpr std::uint32_t forced=2u;
}
namespace row_bits {
inline constexpr std::uint32_t culled=0x0001u;
inline constexpr std::uint32_t vis_mask_shift=9u;
inline constexpr std::uint32_t skip_vis_index=0x0800u;
}
namespace extra_flags {
inline constexpr std::uint32_t default_value=0x0001u;
inline constexpr std::uint32_t visible_indexed=0x0040u;
inline constexpr std::uint32_t built_mask=0x0208u;
}
namespace render_bits {
inline constexpr std::uint32_t retry_clear_mask=0xfffffffdu;
inline constexpr std::uint32_t retry_set_bit=0x00000004u;
inline constexpr std::uint32_t skip_clear_mask=0xfffffffbu;
}

struct ObjectQueue {
    std::uint32_t unused_00[2];
    std::uint32_t row_flags;        // +0x08: uint16_t[]
    std::uint32_t handles;          // +0x0C: uint32_t[]
    std::uint32_t unused_10;
    std::uint32_t vis_indices;      // +0x14: uint16_t[]
};
static_assert(offsetof(ObjectQueue,row_flags)==0x08 &&
              offsetof(ObjectQueue,handles)==0x0c &&
              offsetof(ObjectQueue,vis_indices)==0x14);

struct RenderPassView {
    std::uint8_t unused_00[0x14];
    std::uint32_t object_queue;     // +0x14: ObjectQueue*
    std::uint8_t unused_18[0x2a60];
    std::uint32_t pass_kind;        // +0x2A78
};
static_assert(offsetof(RenderPassView,object_queue)==0x14 &&
              offsetof(RenderPassView,pass_kind)==0x2a78);

struct RenderObject {
    std::uint8_t unused_00[0x30];
    std::uint32_t center_x;         // +0x30 (float bits)
    std::uint32_t center_y;         // +0x34
    std::uint32_t center_z;         // +0x38
    float bounding_radius;          // +0x3C
};
static_assert(offsetof(RenderObject,center_x)==0x30 &&
              offsetof(RenderObject,center_y)==0x34 &&
              offsetof(RenderObject,center_z)==0x38 &&
              offsetof(RenderObject,bounding_radius)==0x3c);

inline constexpr std::uint32_t submission_fade_offset=0x1bcu;
inline constexpr std::uint32_t submission_reserved_1a4=0x1a4u;

inline std::uint32_t widen16(std::uint32_t value){return std::uint32_t(std::int32_t(std::int16_t(value)));}

// ------------------------------------------------------------ access roles
enum class Access : unsigned {
    pass_kind,retry_arg_entry,clear_force_flag,set_force_flag,
    queue_ptr,handles_ptr,slot_index_arg,object_handle,
    object_table,object_data,object_ptr,bounding_radius,
    center_x,store_pos_x,center_y,center_z,store_pos_y,
    reload_pos_y,plane_y,store_pos_z,reload_pos_z,plane_z,
    reload_pos_x,plane_x,plane_d,
    store_handle,store_radius,min_plane_dist,
    x87_pos_x,screen_scale,x87_cam_x,x87_pos_y,x87_cam_y,
    x87_pos_z,proj_scale,x87_cam_z,lod_bias,
    store_proj_size,x87_dist_scale,store_cam_dist,reload_cam_dist,
    flags_queue,flags_array,row_flags,
    sort_counter_arg,store_render_flags,store_vis_mask_byte,initial_slot_arg,store_sort_slot,
    store_cam_depth,store_has_sort,store_sort_counter,store_override_shader,store_use_alpha,
    store_override_alpha,store_override_param,store_reserved_1a4,store_vis_index_default,
    vis_array,vis_index,store_extra_visible,store_vis_index,store_extra_default,
    post_extra_flags,post_retry_arg,store_post_extra,
    retry_render_flags,retry_clear_skip,retry_store_render_flags,
    check_skip_children,check_pass_kind_secondary,clear_render_bit2,
    check_fade_byte,check_layer_flags,check_pass_kind_primary,
    hierarchy_force_dword,hierarchy_include_arg,return_skip_children,
    cull_queue,cull_flags_array,cull_or_bit0
};

struct Kept {
    std::uint32_t view{};
    std::uint32_t slot_index{};
    std::uint32_t handle{};
    std::uint32_t queue{};
    std::uint32_t row_flags{};
    std::uint32_t zero{};
};

namespace object_submit {
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    Kept k{};
    k.view=frame.entry_view();
    const auto kind=memory.load32(pass_kind,at(k.view,offsetof(RenderPassView,pass_kind)));
    frame.allocate_frame();
    bool force=(kind==pass_kinds::forced);
    if(!force) {
        const auto retry=memory.load8(retry_arg_entry,frame.arg_retry());
        memory.store8(clear_force_flag,frame.force_flag_slot(),0u);
        force=(retry!=0u);
    }
    if(force) memory.store8(set_force_flag,frame.force_flag_slot(),1u);

    const auto queue=memory.load32(queue_ptr,at(k.view,offsetof(RenderPassView,object_queue)));
    const auto handles=memory.load32(handles_ptr,at(queue,offsetof(ObjectQueue,handles)));
    k.slot_index=widen16(frame.enter_body_and_load_slot());
    k.handle=memory.load32(object_handle,element(handles,k.slot_index,4u));
    const auto table=memory.load32(object_table,globals::object_table);
    const auto data=memory.load32(object_data,at(table,offsetof(ObjectTableHeader,data)));
    const auto obj=memory.load32(object_ptr,
        at(element(data,handle_index(k.handle),sizeof(ObjectDatumRow)),offsetof(ObjectDatumRow,object)));

    const auto sub=frame.submission_address();
    const auto radius=memory.load_float(bounding_radius,at(obj,offsetof(RenderObject,bounding_radius)));
    const auto cx=memory.load32(center_x,at(obj,offsetof(RenderObject,center_x)));
    memory.store32(store_pos_x,at(sub,offsetof(RenderModelSubmission,position_x)),cx);
    const auto cy=memory.load32(center_y,at(obj,offsetof(RenderObject,center_y)));
    const auto cz=memory.load32(center_z,at(obj,offsetof(RenderObject,center_z)));
    memory.store32(store_pos_y,at(sub,offsetof(RenderModelSubmission,position_y)),cy);

    const auto py=memory.load_float(reload_pos_y,at(sub,offsetof(RenderModelSubmission,position_y)));
    const auto y_term=frame.mul_scalar(py,memory.load_float(plane_y,globals::plane_y));
    memory.store32(store_pos_z,at(sub,offsetof(RenderModelSubmission,position_z)),cz);
    const auto pz=memory.load_float(reload_pos_z,at(sub,offsetof(RenderModelSubmission,position_z)));
    const auto z_term=frame.mul_scalar(pz,memory.load_float(plane_z,globals::plane_z));
    auto dist=frame.add_scalar(z_term,y_term);
    const auto px=memory.load_float(reload_pos_x,at(sub,offsetof(RenderModelSubmission,position_x)));
    const auto x_term=frame.mul_scalar(px,memory.load_float(plane_x,globals::plane_x));
    dist=frame.add_scalar(frame.add_scalar(dist,x_term),memory.load_float(plane_d,globals::plane_d));

    const bool neg=frame.compare_plane_zero(dist);
    memory.store32(store_handle,at(sub,offsetof(RenderModelSubmission,object_handle)),k.handle);
    memory.store_float(store_radius,at(sub,offsetof(RenderModelSubmission,position_z)+4u),radius);
    if(neg) dist=frame.negate_plane(dist);
    const auto min_dist=memory.load_float(min_plane_dist,globals::min_plane_distance);
    if(frame.clamp_plane_min(dist,min_dist)) dist=min_dist;

    // Interleaved x87 Euclidean camera distance and SSE projected size
    frame.x87_load(x87_pos_x,at(sub,offsetof(RenderModelSubmission,position_x)));
    auto proj=memory.load_float(screen_scale,globals::screen_scale);
    frame.x87_sub(x87_cam_x,globals::camera_pos_x);
    proj=frame.div_scalar(proj,dist);
    frame.x87_load(x87_pos_y,at(sub,offsetof(RenderModelSubmission,position_y)));
    frame.save_edi();
    frame.x87_sub(x87_cam_y,globals::camera_pos_y);
    proj=frame.mul_scalar(proj,radius);
    frame.x87_load(x87_pos_z,at(sub,offsetof(RenderModelSubmission,position_z)));
    proj=frame.mul_scalar(proj,memory.load_float(proj_scale,globals::projection_scale));
    frame.x87_sub(x87_cam_z,globals::camera_pos_z);
    proj=frame.mul_scalar(proj,memory.load_float(lod_bias,globals::lod_bias));

    frame.push_shadow_fade_out();
    frame.x87_square_dz();
    memory.store_float(store_proj_size,at(sub,offsetof(RenderModelSubmission,build_flags)),proj);
    frame.x87_dup_dy_and_push_fade_out();
    frame.x87_finish_distance_and_scale(x87_dist_scale,globals::distance_scale);
    frame.x87_store_distance(store_cam_dist,at(sub,offsetof(RenderModelSubmission,build_param)));
    const auto cam_dist=memory.load_float(reload_cam_dist,at(sub,offsetof(RenderModelSubmission,build_param)));
    frame.call_object_fade(k,cam_dist);

    k.queue=memory.load32(flags_queue,at(k.view,offsetof(RenderPassView,object_queue)));
    const auto rflags_arr=memory.load32(flags_array,at(k.queue,offsetof(ObjectQueue,row_flags)));
    k.row_flags=widen16(memory.load16(row_flags,element(rflags_arr,k.slot_index,2u)));
    const auto draw_flags=frame.call_draw_flags(k);

    const auto sort_counter=memory.load32(sort_counter_arg,frame.arg_sort_counter());
    memory.store32(store_render_flags,at(sub,offsetof(RenderModelSubmission,render_flags)),draw_flags);
    const auto vis_byte=(k.row_flags>>row_bits::vis_mask_shift)&1u;
    memory.store8(store_vis_mask_byte,at(sub,offsetof(RenderModelSubmission,visibility_mask)),vis_byte);
    const auto init_slot=memory.load32(initial_slot_arg,frame.arg_initial_slot());
    k.zero=0u;
    memory.store32(store_sort_slot,at(sub,offsetof(RenderModelSubmission,sort_slot)),init_slot);

    const bool skip_vis=frame.prepare_defaults_and_test_vis(k.row_flags);
    memory.store_float(store_cam_depth,at(sub,offsetof(RenderModelSubmission,camera_depth)),0.0f);
    memory.store8(store_has_sort,at(sub,offsetof(RenderModelSubmission,has_sort_slot)),0u);
    memory.store32(store_sort_counter,at(sub,offsetof(RenderModelSubmission,sort_counter)),sort_counter);
    memory.store32(store_override_shader,at(sub,offsetof(RenderModelSubmission,override_shader)),none);
    memory.store8(store_use_alpha,at(sub,offsetof(RenderModelSubmission,use_alpha_override)),0u);
    memory.store_float(store_override_alpha,at(sub,offsetof(RenderModelSubmission,override_alpha)),0.0f);
    memory.store_float(store_override_param,at(sub,offsetof(RenderModelSubmission,override_param)),0.0f);
    memory.store32(store_reserved_1a4,at(sub,submission_reserved_1a4),0u);
    memory.store16(store_vis_index_default,at(sub,offsetof(RenderModelSubmission,visibility_index)),0xffffu);

    bool wrote_vis=false;
    if(!skip_vis) {
        const auto vis_arr=memory.load32(vis_array,at(k.queue,offsetof(ObjectQueue,vis_indices)));
        const auto vidx=memory.load16(vis_index,element(vis_arr,k.slot_index,2u));
        if(vidx!=0xffffu) {
            memory.store32(store_extra_visible,at(sub,offsetof(RenderModelSubmission,extra_flags)),
                extra_flags::visible_indexed);
            memory.store16(store_vis_index,at(sub,offsetof(RenderModelSubmission,visibility_index)),vidx);
            wrote_vis=true;
        }
    }
    if(!wrote_vis) {
        memory.store32(store_extra_default,at(sub,offsetof(RenderModelSubmission,extra_flags)),
            extra_flags::default_value);
    }

    frame.call_model_build(k);

    const auto built_extra=memory.load32(post_extra_flags,at(sub,offsetof(RenderModelSubmission,extra_flags)));
    const auto post_retry=memory.load8(post_retry_arg,frame.arg_retry_with_edi());
    memory.store32(store_post_extra,at(sub,offsetof(RenderModelSubmission,extra_flags)),
        built_extra|extra_flags::built_mask);
    frame.restore_edi();

    if(post_retry!=(k.zero&0xffu)) {
        const auto rflags=memory.load32(retry_render_flags,at(sub,offsetof(RenderModelSubmission,render_flags)));
        const auto updated=(rflags&render_bits::retry_clear_mask)|render_bits::retry_set_bit;
        memory.store8(retry_clear_skip,at(sub,offsetof(RenderModelSubmission,skip_children)),0u);
        memory.store32(retry_store_render_flags,at(sub,offsetof(RenderModelSubmission,render_flags)),updated);
    } else {
        if(memory.load8(check_skip_children,at(sub,offsetof(RenderModelSubmission,skip_children)))!=
           (k.zero&0xffu)) {
            if(memory.load32(check_pass_kind_secondary,at(k.view,offsetof(RenderPassView,pass_kind)))==
               pass_kinds::secondary) {
                frame.and32(clear_render_bit2,at(sub,offsetof(RenderModelSubmission,render_flags)),
                    render_bits::skip_clear_mask);
            }
        }
    }

    const auto fade=memory.load8(check_fade_byte,at(sub,submission_fade_offset));
    if((fade&0xffu)<=(k.zero&0xffu)) {
        const auto cq=memory.load32(cull_queue,at(k.view,offsetof(RenderPassView,object_queue)));
        const auto cf=memory.load32(cull_flags_array,at(cq,offsetof(ObjectQueue,row_flags)));
        const auto cell=element(cf,k.slot_index,2u);
        frame.or8(cull_or_bit0,cell,row_bits::culled);
        frame.finish_culled(cell);
        return;
    }

    const auto lflags=memory.load8(check_layer_flags,at(sub,offsetof(RenderModelSubmission,layer_flags)));
    if((lflags&native_model_hierarchy::layer_bits::alpha_pass)!=0u &&
       memory.load32(check_pass_kind_primary,at(k.view,offsetof(RenderPassView,pass_kind)))!=k.zero) {
        frame.finish_skipped();
        return;
    }

    const auto force_dword=memory.load32(hierarchy_force_dword,frame.force_flag_slot());
    const auto include_children=memory.load32(hierarchy_include_arg,frame.arg_include_children());
    frame.call_model_hierarchy(k,force_dword,include_children);
    const auto ret=memory.load8(return_skip_children,at(sub,offsetof(RenderModelSubmission,skip_children)));
    frame.finish_submitted(ret);
}
} // namespace object_submit
} // namespace halo2::native_object_submit
