#pragma once
// Native TU1.5 render-model hierarchy & layer submission: 1365C0, called by
// 1362A0 and recursively by itself for overlay passes and attached child
// objects.
//
//   model_hierarchy::submit(memory, frame)
//     1. If submission->section_count > 0:
//        - When direct_submit is clear, splits overlay/transparent passes
//          indicated by submission->layer_flags:
//            * bit 7 (0x80): translucent sort pass (sort_kind=3, shader[3],
//              weight[3]), stores camera depth on submission, allocates a
//              sort slot if needed, copies the 0x1E4-byte submission, swaps
//              extra_flags into render_flags, and recurses with
//              (include_children=1, direct_submit=1).
//            * bit 1 (0x02): alpha-override sort pass (sort_kind=1,
//              shader[1], weight[1], use_alpha_override=1), suppresses the
//              base draw, allocates a sort slot if needed, copies the
//              submission, stores camera depth on the copy, and recurses
//              with (include_children=1, direct_submit=1).
//            * bit 0 (0x01): clears bit 0 on submission->layer_flags, copies
//              the submission, sets render_flags |= 0x8000, overrides shader[0]
//              and weight[0], updates skip_base from bit 1 of the remaining
//              layer_flags, and recurses with (include_children=0,
//              direct_submit=1).
//            * bit 5 (0x20): clears bit 5 on submission->layer_flags, copies
//              the submission, sets render_flags |= 0x8000, overrides shader[2]
//              and weight[2], updates skip_base from bit 1 of the remaining
//              layer_flags, and recurses with (include_children=0,
//              direct_submit=1).
//        - If direct_submit is set or skip_base is clear, submits the model
//          to 136A80 (native_model_draw_submission) with
//          suppress_children = (include_children == 0).
//     2. If include_children != 0 and submission->skip_children == 0:
//        Walks the object's attached child linked list (first_child at +0x10,
//        next_sibling at +0x0C via the global object table [4ECBD0]). For each
//        child: copies the submission, updates object_handle, resets sort
//        override fields when sort_kind==1 and the child is a type-2 object
//        whose tag definition has a non-zero word at +0x290, clears bit 6 and
//        sets bit 0 of extra_flags, sets visibility_index=0xFFFF, builds the
//        child model via 4B5C0 (source_model_build), and recurses with
//        (include_children=1, direct_submit=direct_submit).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access) and the boundary adapter
// (native_tu15_model_hierarchy.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_model_hierarchy {
using native_object_update::at;
using native_object_update::element;
using native_object_update::handle_index;
using native_object_update::none;

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t camera_forward_x=0x4c65ccu;
inline constexpr std::uint32_t camera_forward_y=0x4c65d0u;
inline constexpr std::uint32_t camera_forward_z=0x4c65d4u;
inline constexpr std::uint32_t object_table=0x4ecbd0u;     // ObjectTableHeader*
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // TagInstance[]
}
namespace layer_bits {
inline constexpr std::uint32_t overlay_a=0x01u;
inline constexpr std::uint32_t alpha_pass=0x02u;
inline constexpr std::uint32_t overlay_b=0x20u;
inline constexpr std::uint32_t translucent_pass=0x80u;
}
namespace render_flags {
inline constexpr std::uint32_t overlay_bit=0x8000u;
}
namespace extra_flags {
inline constexpr std::uint32_t child_clear_mask=0xffffffbfu;
inline constexpr std::uint32_t child_set_bit=0x00000001u;
}
namespace sort_kinds {
inline constexpr std::uint32_t alpha=1u;
inline constexpr std::uint32_t translucent=3u;
inline constexpr std::uint32_t shadow_override_slot=0x1fu;
}
inline constexpr std::uint8_t shadowed_object_type=2u;

struct RenderPassView {
    std::uint8_t unused_here[0x2a68];
    float camera_x;                 // +0x2A68
    float camera_y;                 // +0x2A6C
    float camera_z;                 // +0x2A70
    std::uint32_t unused_2a74;
    std::uint32_t pass_kind;        // +0x2A78
};
static_assert(offsetof(RenderPassView,camera_x)==0x2a68 &&
              offsetof(RenderPassView,camera_y)==0x2a6c &&
              offsetof(RenderPassView,camera_z)==0x2a70 &&
              offsetof(RenderPassView,pass_kind)==0x2a78);

struct RenderModelSubmission {
    std::uint32_t object_handle;    // +0x000
    std::uint8_t model_data[0x162]; // +0x004..+0x165
    std::int16_t section_count;     // +0x166
    std::uint8_t unused_168[4];
    std::uint8_t skip_children;     // +0x16C
    std::uint8_t unused_16d[3];
    std::uint32_t layer_flags;      // +0x170
    std::uint32_t effect_shaders[4];// +0x174..+0x183
    float effect_weights[4];        // +0x184..+0x193
    float position_x;               // +0x194
    float position_y;               // +0x198
    float position_z;               // +0x19C
    std::uint8_t unused_1a0[8];
    std::uint32_t visibility_mask;  // +0x1A8
    std::uint32_t render_flags;     // +0x1AC
    std::uint32_t extra_flags;      // +0x1B0
    std::uint32_t build_flags;      // +0x1B4
    std::uint32_t build_param;      // +0x1B8
    std::uint16_t unused_1bc;
    std::uint16_t visibility_index; // +0x1BE
    std::uint32_t sort_slot;        // +0x1C0
    std::uint32_t sort_kind;        // +0x1C4
    std::uint32_t sort_counter;     // +0x1C8
    std::uint8_t has_sort_slot;     // +0x1CC
    std::uint8_t unused_1cd[3];
    std::uint32_t override_shader;  // +0x1D0
    std::uint8_t use_alpha_override;// +0x1D4
    std::uint8_t unused_1d5[3];
    float override_alpha;           // +0x1D8
    float override_param;           // +0x1DC
    float camera_depth;             // +0x1E0
};
static_assert(sizeof(RenderModelSubmission)==0x1e4 &&
              sizeof(RenderModelSubmission)/4==0x79 &&
              offsetof(RenderModelSubmission,section_count)==0x166 &&
              offsetof(RenderModelSubmission,skip_children)==0x16c &&
              offsetof(RenderModelSubmission,layer_flags)==0x170 &&
              offsetof(RenderModelSubmission,effect_shaders)==0x174 &&
              offsetof(RenderModelSubmission,effect_weights)==0x184 &&
              offsetof(RenderModelSubmission,position_x)==0x194 &&
              offsetof(RenderModelSubmission,visibility_mask)==0x1a8 &&
              offsetof(RenderModelSubmission,render_flags)==0x1ac &&
              offsetof(RenderModelSubmission,extra_flags)==0x1b0 &&
              offsetof(RenderModelSubmission,build_flags)==0x1b4 &&
              offsetof(RenderModelSubmission,build_param)==0x1b8 &&
              offsetof(RenderModelSubmission,visibility_index)==0x1be &&
              offsetof(RenderModelSubmission,sort_slot)==0x1c0 &&
              offsetof(RenderModelSubmission,sort_kind)==0x1c4 &&
              offsetof(RenderModelSubmission,sort_counter)==0x1c8 &&
              offsetof(RenderModelSubmission,has_sort_slot)==0x1cc &&
              offsetof(RenderModelSubmission,override_shader)==0x1d0 &&
              offsetof(RenderModelSubmission,use_alpha_override)==0x1d4 &&
              offsetof(RenderModelSubmission,override_alpha)==0x1d8 &&
              offsetof(RenderModelSubmission,override_param)==0x1dc &&
              offsetof(RenderModelSubmission,camera_depth)==0x1e0);

struct ObjectTableHeader {
    std::uint8_t unused_here[0x44];
    std::uint32_t data;             // +0x44
};
static_assert(offsetof(ObjectTableHeader,data)==0x44);

struct ObjectDatumRow {
    std::uint32_t unused_here[2];
    std::uint32_t object;           // +0x08
};
static_assert(sizeof(ObjectDatumRow)==12 && offsetof(ObjectDatumRow,object)==8);

struct ObjectHeader {
    std::uint32_t tag_handle;       // +0x00
    std::uint32_t unused_04[2];
    std::uint32_t next_sibling;     // +0x0C
    std::uint32_t first_child;      // +0x10
    std::uint8_t unused_14[0x96];
    std::uint8_t object_type;       // +0xAA
};
static_assert(offsetof(ObjectHeader,tag_handle)==0 &&
              offsetof(ObjectHeader,next_sibling)==0x0c &&
              offsetof(ObjectHeader,first_child)==0x10 &&
              offsetof(ObjectHeader,object_type)==0xaa);

struct TagInstance {
    std::uint32_t unused_here[2];
    std::uint32_t definition;       // +0x08
    std::uint32_t unused_c;
};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,definition)==8);

struct ItemDefinition {
    std::uint8_t unused_here[0x290];
    std::uint16_t shadow_override;  // +0x290
};
static_assert(offsetof(ItemDefinition,shadow_override)==0x290);

// ------------------------------------------------------------ access roles
enum class Access : unsigned {
    submission_arg,section_count,save_view,direct_arg,clear_skip,
    // Translucent pass (layer_flags & 0x80)
    t_flags,t_pos_y,t_pos_z,t_cam_z,t_pos_x,t_cam_y,t_fwd_z,t_cam_x,t_fwd_y,t_fwd_x,
    t_has_sort,t_store_depth,t_clear_skip,
    t_sort_counter_ptr,t_set_has_sort,t_read_counter,t_store_slot,t_inc_counter,t_store_kind,
    t_pass_arg,t_copy_extra_flags,t_copy_shader,t_copy_weight,
    t_reload_view,t_store_render_flags,t_clear_extra_flags,t_store_override_shader,t_store_override_param,
    t_after_view,
    // Alpha pass (layer_flags & 0x02)
    a_flags,a_pos_y,a_pos_z,a_cam_z,a_pos_x,a_cam_y,a_fwd_z,a_has_sort,a_cam_x,a_fwd_y,a_fwd_x,
    a_set_skip,
    a_sort_counter_ptr,a_set_has_sort,a_read_counter,a_store_slot,a_inc_counter,a_store_kind,
    a_extra_flags,a_pass_arg,a_copy_shader,a_copy_weight,
    a_reload_view,a_store_render_flags,a_clear_extra_flags,a_store_override_shader,a_set_use_alpha,
    a_store_override_alpha,a_store_depth,
    // Overlay A pass (layer_flags & 0x01)
    o1_flags,o1_pass_arg,o1_copy_render_flags,o1_copy_shader,o1_copy_weight,
    o1_store_flags,o1_store_skip,o1_store_override_shader,o1_reload_view,
    o1_store_render_flags,o1_store_override_param,
    // Overlay B pass (layer_flags & 0x20)
    o2_flags,o2_pass_arg,o2_copy_render_flags,o2_copy_shader,o2_copy_weight,
    o2_store_flags,o2_store_skip,o2_store_override_shader,o2_reload_view,
    o2_store_render_flags,o2_store_override_param,
    // Base submission to 136A80
    check_skip,submit_include_children,
    // Attached children traversal
    children_include_arg,skip_children_flag,
    first_object_table,parent_handle,first_object_data,parent_object,first_child,save_child_handle,
    reload_child_handle,
    child_sort_kind,child_store_handle,
    child_data_ptr,child_object,child_object_type,
    child_tag_handle,child_tag_instances,child_tag_def,child_shadow_override,
    child_clear_has_sort,child_reset_slot,child_reset_kind,child_reset_shader,child_clear_use_alpha,child_clear_alpha,
    child_extra_flags,child_reload_view,child_store_vis_index,child_vis_mask,child_store_extra_flags,
    child_build_flags,child_build_param,child_view_pass_kind,child_pass_arg_byte,
    child_recurse_pass_arg,child_recurse_direct_arg,
    next_child_handle,next_object_table,next_object_data,next_child_object,next_sibling,store_next_child
};

enum class LocalCopy : unsigned {
    child,alpha,translucent,overlay_a,overlay_b
};

struct Kept {
    std::uint32_t submission{};
    std::uint32_t view{};
    std::uint32_t object_data_slot{};
};

namespace model_hierarchy {
// Frame (the adapter):
//   entry_view()                                    ECX on entry
//   enter(Kept&)                                    prologue, loads submission, checks section_count
//   copy_submission(LocalCopy, src, dst)            REP MOVSD 0x79 dwords
//   copy_address(LocalCopy)                         address of one of the 5 local submission copies
//   skip_slot() / view_slot() / child_handle_slot() local frame stack slots
//   arg_include_children() / arg_direct() / arg_pass_param()  EBP+0x0C / +0x10 / +0x14
//   dot_depth(...)                                  exact SSE single-precision dot product
//   submit_translucent(...) / submit_alpha(...) / submit_overlay_a(...) / submit_overlay_b(...)
//   submit_base(Kept&, std::uint8_t include_children)
//   build_and_submit_child(Kept&, std::uint32_t child_handle, ...)
//   finish()
template<class Memory,class Frame> void submit(Memory &memory,Frame &frame) {
    using enum Access;
    Kept kept{};
    kept.view=frame.entry_view();
    const auto sections=frame.enter(kept);
    memory.store32(save_view,frame.view_slot(),kept.view);
    if(sections>0) {
        const auto direct=memory.load8(direct_arg,frame.arg_direct());
        memory.store8(clear_skip,frame.skip_slot(),0u);
        bool run_base=true;
        if(!direct) {
            // 1. Translucent pass (layer_flags & 0x80)
            if(memory.load8(t_flags,at(kept.submission,offsetof(RenderModelSubmission,layer_flags)))&
               layer_bits::translucent_pass) {
                const auto py=memory.load_float(t_pos_y,at(kept.submission,offsetof(RenderModelSubmission,position_y)));
                const auto pz=memory.load_float(t_pos_z,at(kept.submission,offsetof(RenderModelSubmission,position_z)));
                const auto dz=frame.sub_scalar(pz,memory.load_float(t_cam_z,at(kept.view,offsetof(RenderPassView,camera_z))));
                const auto px=memory.load_float(t_pos_x,at(kept.submission,offsetof(RenderModelSubmission,position_x)));
                const auto dy=frame.sub_scalar(py,memory.load_float(t_cam_y,at(kept.view,offsetof(RenderPassView,camera_y))));
                const auto wz=frame.mul_scalar(dz,memory.load_float(t_fwd_z,globals::camera_forward_z));
                const auto dx=frame.sub_scalar(px,memory.load_float(t_cam_x,at(kept.view,offsetof(RenderPassView,camera_x))));
                const auto wy=frame.mul_scalar(dy,memory.load_float(t_fwd_y,globals::camera_forward_y));
                const auto wx=frame.mul_scalar(dx,memory.load_float(t_fwd_x,globals::camera_forward_x));
                const auto has_sort=memory.load8(t_has_sort,at(kept.submission,offsetof(RenderModelSubmission,has_sort_slot)));
                const auto depth=frame.add_scalar(frame.add_scalar(wz,wy),wx);
                memory.store_float(t_store_depth,at(kept.submission,offsetof(RenderModelSubmission,camera_depth)),depth);
                memory.store8(t_clear_skip,frame.skip_slot(),0u);
                if(!has_sort) {
                    const auto counter_ptr=memory.load32(t_sort_counter_ptr,
                        at(kept.submission,offsetof(RenderModelSubmission,sort_counter)));
                    memory.store8(t_set_has_sort,at(kept.submission,offsetof(RenderModelSubmission,has_sort_slot)),1u);
                    const auto slot=memory.load32(t_read_counter,counter_ptr);
                    memory.store32(t_store_slot,at(kept.submission,offsetof(RenderModelSubmission,sort_slot)),slot);
                    frame.increment32(t_inc_counter,counter_ptr);
                    memory.store32(t_store_kind,at(kept.submission,offsetof(RenderModelSubmission,sort_kind)),
                        sort_kinds::translucent);
                }
                const auto dst=frame.copy_address(LocalCopy::translucent);
                frame.copy_submission(LocalCopy::translucent,kept.submission,dst);
                const auto pass_param=memory.load32(t_pass_arg,frame.arg_pass_param());
                const auto extra=memory.load32(t_copy_extra_flags,at(dst,offsetof(RenderModelSubmission,extra_flags)));
                const auto shader=memory.load32(t_copy_shader,at(dst,offsetof(RenderModelSubmission,effect_shaders)+12u));
                const auto weight=memory.load_float(t_copy_weight,at(dst,offsetof(RenderModelSubmission,effect_weights)+12u));
                frame.submit_translucent(kept,dst,pass_param,extra,shader,weight);
                kept.view=memory.load32(t_after_view,frame.view_slot());
            }
            // 2. Alpha pass (layer_flags & 0x02)
            if(memory.load8(a_flags,at(kept.submission,offsetof(RenderModelSubmission,layer_flags)))&
               layer_bits::alpha_pass) {
                const auto py=memory.load_float(a_pos_y,at(kept.submission,offsetof(RenderModelSubmission,position_y)));
                const auto pz=memory.load_float(a_pos_z,at(kept.submission,offsetof(RenderModelSubmission,position_z)));
                const auto dz=frame.sub_scalar(pz,memory.load_float(a_cam_z,at(kept.view,offsetof(RenderPassView,camera_z))));
                const auto px=memory.load_float(a_pos_x,at(kept.submission,offsetof(RenderModelSubmission,position_x)));
                const auto dy=frame.sub_scalar(py,memory.load_float(a_cam_y,at(kept.view,offsetof(RenderPassView,camera_y))));
                const auto wz=frame.mul_scalar(dz,memory.load_float(a_fwd_z,globals::camera_forward_z));
                const auto has_sort=memory.load8(a_has_sort,at(kept.submission,offsetof(RenderModelSubmission,has_sort_slot)));
                const auto dx=frame.sub_scalar(px,memory.load_float(a_cam_x,at(kept.view,offsetof(RenderPassView,camera_x))));
                const auto wy=frame.mul_scalar(dy,memory.load_float(a_fwd_y,globals::camera_forward_y));
                const auto wx=frame.mul_scalar(dx,memory.load_float(a_fwd_x,globals::camera_forward_x));
                const auto depth=frame.add_scalar(frame.add_scalar(wz,wy),wx);
                memory.store8(a_set_skip,frame.skip_slot(),1u);
                if(!has_sort) {
                    const auto counter_ptr=memory.load32(a_sort_counter_ptr,
                        at(kept.submission,offsetof(RenderModelSubmission,sort_counter)));
                    memory.store8(a_set_has_sort,at(kept.submission,offsetof(RenderModelSubmission,has_sort_slot)),1u);
                    const auto slot=memory.load32(a_read_counter,counter_ptr);
                    memory.store32(a_store_slot,at(kept.submission,offsetof(RenderModelSubmission,sort_slot)),slot);
                    frame.increment32(a_inc_counter,counter_ptr);
                    memory.store32(a_store_kind,at(kept.submission,offsetof(RenderModelSubmission,sort_kind)),
                        sort_kinds::alpha);
                }
                const auto extra=memory.load32(a_extra_flags,at(kept.submission,offsetof(RenderModelSubmission,extra_flags)));
                const auto dst=frame.copy_address(LocalCopy::alpha);
                frame.copy_submission(LocalCopy::alpha,kept.submission,dst);
                const auto pass_param=memory.load32(a_pass_arg,frame.arg_pass_param());
                const auto shader=memory.load32(a_copy_shader,at(dst,offsetof(RenderModelSubmission,effect_shaders)+4u));
                const auto weight=memory.load_float(a_copy_weight,at(dst,offsetof(RenderModelSubmission,effect_weights)+4u));
                frame.submit_alpha(kept,dst,pass_param,extra,shader,weight,depth);
            }
            // 3. Overlay A pass (layer_flags & 0x01)
            const auto flags1=memory.load32(o1_flags,at(kept.submission,offsetof(RenderModelSubmission,layer_flags)));
            if(flags1&layer_bits::overlay_a) {
                const auto pass_param=memory.load32(o1_pass_arg,frame.arg_pass_param());
                const auto cleared=flags1&~layer_bits::overlay_a;
                const auto dst=frame.copy_address(LocalCopy::overlay_a);
                frame.copy_submission(LocalCopy::overlay_a,kept.submission,dst);
                const auto rflags=memory.load32(o1_copy_render_flags,at(dst,offsetof(RenderModelSubmission,render_flags)));
                const auto shader=memory.load32(o1_copy_shader,at(dst,offsetof(RenderModelSubmission,effect_shaders)));
                const auto weight=memory.load_float(o1_copy_weight,at(dst,offsetof(RenderModelSubmission,effect_weights)));
                memory.store32(o1_store_flags,at(kept.submission,offsetof(RenderModelSubmission,layer_flags)),cleared);
                frame.submit_overlay_a(kept,dst,pass_param,cleared,rflags|render_flags::overlay_bit,shader,weight);
            }
            // 4. Overlay B pass (layer_flags & 0x20)
            const auto flags2=memory.load32(o2_flags,at(kept.submission,offsetof(RenderModelSubmission,layer_flags)));
            if(flags2&layer_bits::overlay_b) {
                const auto pass_param=memory.load32(o2_pass_arg,frame.arg_pass_param());
                const auto cleared=flags2&~layer_bits::overlay_b;
                const auto dst=frame.copy_address(LocalCopy::overlay_b);
                frame.copy_submission(LocalCopy::overlay_b,kept.submission,dst);
                const auto rflags=memory.load32(o2_copy_render_flags,at(dst,offsetof(RenderModelSubmission,render_flags)));
                const auto shader=memory.load32(o2_copy_shader,at(dst,offsetof(RenderModelSubmission,effect_shaders)+8u));
                const auto weight=memory.load_float(o2_copy_weight,at(dst,offsetof(RenderModelSubmission,effect_weights)+8u));
                memory.store32(o2_store_flags,at(kept.submission,offsetof(RenderModelSubmission,layer_flags)),cleared);
                frame.submit_overlay_b(kept,dst,pass_param,cleared,rflags|render_flags::overlay_bit,shader,weight);
            }
            run_base=(memory.load8(check_skip,frame.skip_slot())==0u);
        }
        if(run_base) {
            const auto include_children=memory.load8(submit_include_children,frame.arg_include_children());
            frame.submit_base(kept,include_children==0u?1u:0u);
        }
    }

    // Attached children traversal
    if(memory.load8(children_include_arg,frame.arg_include_children())!=0u &&
       memory.load8(skip_children_flag,at(kept.submission,offsetof(RenderModelSubmission,skip_children)))==0u) {
        const auto table=memory.load32(first_object_table,globals::object_table);
        const auto parent_id=memory.load32(parent_handle,at(kept.submission,offsetof(RenderModelSubmission,object_handle)));
        kept.object_data_slot=at(table,offsetof(ObjectTableHeader,data));
        const auto data=memory.load32(first_object_data,kept.object_data_slot);
        const auto parent_obj=memory.load32(parent_object,
            at(element(data,handle_index(parent_id),sizeof(ObjectDatumRow)),offsetof(ObjectDatumRow,object)));
        auto child_handle=memory.load32(first_child,at(parent_obj,offsetof(ObjectHeader,first_child)));
        memory.store32(save_child_handle,frame.child_handle_slot(),child_handle);
        if(child_handle!=none) {
            for(bool first=true;;first=false) {
                if(!first) child_handle=memory.load32(reload_child_handle,frame.child_handle_slot());
                const auto dst=frame.copy_address(LocalCopy::child);
                frame.copy_submission(LocalCopy::child,kept.submission,dst);
                const auto sort_kind=memory.load32(child_sort_kind,at(dst,offsetof(RenderModelSubmission,sort_kind)));
                memory.store32(child_store_handle,at(dst,offsetof(RenderModelSubmission,object_handle)),child_handle);
                bool reset_override=false;
                if(sort_kind==sort_kinds::alpha) {
                    const auto child_data=memory.load32(child_data_ptr,kept.object_data_slot);
                    const auto child_obj=memory.load32(child_object,
                        at(element(child_data,handle_index(child_handle),sizeof(ObjectDatumRow)),
                           offsetof(ObjectDatumRow,object)));
                    if(memory.load8(child_object_type,at(child_obj,offsetof(ObjectHeader,object_type)))==
                       shadowed_object_type) {
                        const auto tag=memory.load32(child_tag_handle,at(child_obj,offsetof(ObjectHeader,tag_handle)));
                        const auto tags=memory.load32(child_tag_instances,globals::tag_instances);
                        const auto def=memory.load32(child_tag_def,
                            at(element(tags,handle_index(tag),sizeof(TagInstance)),offsetof(TagInstance,definition)));
                        if(memory.load16(child_shadow_override,at(def,offsetof(ItemDefinition,shadow_override)))!=0u) {
                            reset_override=true;
                            memory.store8(child_clear_has_sort,at(dst,offsetof(RenderModelSubmission,has_sort_slot)),0u);
                            memory.store32(child_reset_slot,at(dst,offsetof(RenderModelSubmission,sort_slot)),
                                sort_kinds::shadow_override_slot);
                            memory.store32(child_reset_kind,at(dst,offsetof(RenderModelSubmission,sort_kind)),none);
                            memory.store32(child_reset_shader,at(dst,offsetof(RenderModelSubmission,override_shader)),none);
                            memory.store8(child_clear_use_alpha,at(dst,offsetof(RenderModelSubmission,use_alpha_override)),0u);
                            memory.store_float(child_clear_alpha,at(dst,offsetof(RenderModelSubmission,override_alpha)),0.0f);
                        }
                    }
                }
                if(!reset_override) frame.set_default_child_index();
                const auto extra=memory.load32(child_extra_flags,at(dst,offsetof(RenderModelSubmission,extra_flags)));
                kept.view=memory.load32(child_reload_view,frame.view_slot());
                const auto updated_extra=(extra&extra_flags::child_clear_mask)|extra_flags::child_set_bit;
                memory.store16(child_store_vis_index,at(dst,offsetof(RenderModelSubmission,visibility_index)),0xffffu);
                const auto vis_mask=memory.load32(child_vis_mask,at(dst,offsetof(RenderModelSubmission,visibility_mask)));
                frame.build_and_submit_child(kept,dst,child_handle,updated_extra,vis_mask);

                const auto cur=memory.load32(next_child_handle,frame.child_handle_slot());
                const auto next_table=memory.load32(next_object_table,globals::object_table);
                kept.object_data_slot=at(next_table,offsetof(ObjectTableHeader,data));
                const auto next_data=memory.load32(next_object_data,kept.object_data_slot);
                const auto cur_obj=memory.load32(next_child_object,
                    at(element(next_data,handle_index(cur),sizeof(ObjectDatumRow)),offsetof(ObjectDatumRow,object)));
                const auto sibling=memory.load32(next_sibling,at(cur_obj,offsetof(ObjectHeader,next_sibling)));
                memory.store32(store_next_child,frame.child_handle_slot(),sibling);
                if(sibling==none) break;
            }
        }
    }
    frame.finish();
}
} // namespace model_hierarchy
} // namespace halo2::native_model_hierarchy
