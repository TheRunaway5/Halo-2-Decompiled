#pragma once
// Native TU1.5 light volume shape builder: C3AB0, pure leaf shared by
// native_light_shape, source_shadow_frame and source_map_world_support.
//
//   light_shape::build(memory, frame)
//     Builds a LightVolumeShape from a LightTransform (ECX) and LightDefinition
//     ([ESP+4]):
//       - kind == 0 (spherical light): scales radius_a and radius_b by
//         transform->scale and stores max(a, b).
//       - kind != 0 (frustum / spot / projected light): computes direction =
//         target - origin, projects onto transform->forward (falling back to
//         forward with length 1.0 when |proj| <= eps or proj <= 0), normalizes
//         direction, applies gel bitmap aspect ratio when present, computes
//         tan_x/tan_y (either from [474CD4] for kind==1 or via x87 FPTAN of
//         0.5*fov), and populates near/far clip extents and far center.
//       - Common tail: computes right = cross(forward, up), copies origin into
//         apex_position, and for kind == 2 with tan_x > eps shifts apex_position
//         back along direction by (near_width / tan_x) * 0.5. Marks valid = 1.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access) and the boundary adapter
// (native_tu15_light_shape.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_light_shape {
using native_object_update::at;
using native_object_update::element;
using native_object_update::handle_index;
using native_object_update::none;

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t half=0x46a0ccu;           // float 0.5
inline constexpr std::uint32_t one=0x46a0d0u;            // float 1.0
inline constexpr std::uint32_t epsilon=0x46a0ecu;        // float 0.0001
inline constexpr std::uint32_t parallel_tan=0x474cd4u;   // float(*)[2]
inline constexpr std::uint32_t tag_instances=0x4f0414u;  // TagInstance[]
}
namespace shape_kinds {
inline constexpr std::uint32_t sphere=0u;
inline constexpr std::uint32_t parallel=1u;
inline constexpr std::uint32_t spot=2u;
}

struct LightTransform {
    float origin_x,origin_y,origin_z;   // +0x00..+0x08
    float target_x,target_y,target_z;   // +0x0C..+0x14
    std::uint32_t unused_18[4];         // +0x18..+0x27
    float forward_x,forward_y,forward_z;// +0x28..+0x30
    float up_x,up_y,up_z;               // +0x34..+0x3C
    float scale;                        // +0x40
};
static_assert(offsetof(LightTransform,origin_x)==0x00 &&
              offsetof(LightTransform,target_x)==0x0c &&
              offsetof(LightTransform,forward_x)==0x28 &&
              offsetof(LightTransform,up_x)==0x34 &&
              offsetof(LightTransform,scale)==0x40);

struct LightFrustum {
    std::uint32_t unused_00;            // +0x00 (shape +0x38)
    float dir_x,dir_y,dir_z;            // +0x04..+0x0C
    float length;                       // +0x10
    float tan_x,tan_y;                  // +0x14, +0x18
    float aspect;                       // +0x1C
    float near_x,near_y;                // +0x20, +0x24
    float near_clip,far_clip;           // +0x28, +0x2C
    float far_x,far_y;                  // +0x30, +0x34
    float far_center_x,far_center_y,far_center_z; // +0x38..+0x40
};
static_assert(offsetof(LightFrustum,dir_x)==0x04 &&
              offsetof(LightFrustum,length)==0x10 &&
              offsetof(LightFrustum,tan_x)==0x14 &&
              offsetof(LightFrustum,aspect)==0x1c &&
              offsetof(LightFrustum,near_x)==0x20 &&
              offsetof(LightFrustum,near_clip)==0x28 &&
              offsetof(LightFrustum,far_x)==0x30 &&
              offsetof(LightFrustum,far_center_x)==0x38);

struct LightVolumeShape {
    std::uint32_t kind;                 // +0x00
    float near_width;                   // +0x04 (radius_a for sphere)
    float aspect;                       // +0x08 (radius_b for sphere)
    float fov;                          // +0x0C
    float near_distance;                // +0x10
    float far_distance;                 // +0x14
    std::uint8_t valid;                 // +0x18
    std::uint8_t pad_19[3];
    float right_x,right_y,right_z;      // +0x1C..+0x24
    float apex_offset;                  // +0x28
    float apex_x,apex_y,apex_z;         // +0x2C..+0x34
    LightFrustum frustum;               // +0x38 (sphere uses +0x3C/+0x40/+0x44)
};
static_assert(offsetof(LightVolumeShape,near_width)==0x04 &&
              offsetof(LightVolumeShape,valid)==0x18 &&
              offsetof(LightVolumeShape,right_x)==0x1c &&
              offsetof(LightVolumeShape,apex_offset)==0x28 &&
              offsetof(LightVolumeShape,apex_x)==0x2c &&
              offsetof(LightVolumeShape,frustum)==0x38);

struct LightDefinition {
    std::uint8_t unused_00[0x7c];
    std::uint32_t gel_bitmap;           // +0x7C
};
static_assert(offsetof(LightDefinition,gel_bitmap)==0x7c);

struct TagInstance {
    std::uint32_t unused_00[2];
    std::uint32_t definition;           // +0x08
    std::uint32_t unused_0c;
};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,definition)==8);

struct BitmapGroup {
    std::uint8_t unused_00[0x48];
    std::uint32_t bitmaps_data;         // +0x48
};
static_assert(offsetof(BitmapGroup,bitmaps_data)==0x48);

struct BitmapData {
    std::uint32_t unused_00;
    std::int16_t width;                 // +0x04
    std::int16_t height;                // +0x06
};
static_assert(offsetof(BitmapData,width)==4 && offsetof(BitmapData,height)==6);

inline std::int32_t widen16(std::uint32_t v){return std::int32_t(std::int16_t(v));}

// ------------------------------------------------------------ access roles
enum class Access : unsigned {
    kind_low16,
    sphere_ra,sphere_scale_a,store_sphere_a,sphere_rb,sphere_scale_b,reload_sphere_a,store_sphere_b,store_sphere_max,
    target_x,origin_x,store_dir_x,target_y,origin_y,store_dir_y,target_z,origin_z,store_dir_z,
    fwd_z,dot_dir_z,fwd_y,dot_dir_y,fwd_x,dot_dir_x,one_constant,
    spill_proj,x87_reload_proj,store_length,x87_epsilon,
    fallback_fwd_x,store_fb_x,fallback_fwd_y,store_fb_y,fallback_fwd_z,store_fb_z,store_fb_len,
    reload_length,def_arg,norm_dir_x,store_norm_x,norm_dir_y,store_norm_y,norm_dir_z,store_norm_z,
    shape_aspect,store_aspect,gel_bitmap,tag_instances,gel_tag_def,gel_bitmap_data,gel_height,gel_width,store_gel_aspect,
    kind_parallel,parallel_ptr,parallel_tan_x,store_par_tan_x,parallel_tan_y,store_par_tan_y,
    x87_fov,x87_half,store_tan_x,x87_aspect,store_tan_y,
    near_width_bits,reload_aspect_near,store_near_x,near_width_mul,store_near_y,
    far_dist,scale_far,store_far_clip,near_dist,scale_near,store_near_clip,
    reload_far_clip,reload_tan_x,reload_near_x,store_far_x,reload_tan_y,reload_near_y,store_far_y,
    reload_far_clip2,far_dir_x,far_origin_x,store_far_cx,far_dir_y,far_origin_y,store_far_cy,far_dir_z,far_origin_z,store_far_cz,
    cross_up_y,cross_fwd_y,cross_up_x,cross_fwd_x,cross_up_z,cross_fwd_x2,cross_fwd_z,cross_up_y2,
    cross_fwd_z2,cross_up_x2,cross_fwd_y2,cross_up_z2,
    store_right_y,store_right_z,store_right_x,store_apex_offset_zero,
    copy_origin_x,store_apex_x,copy_origin_y,store_apex_y,copy_origin_z,store_apex_z,
    kind_spot,spot_tan_x,spot_epsilon,spot_near_width,spot_div_tan_x,spot_half,store_apex_offset,
    spot_dir_x,spot_origin_x,store_spot_ax,spot_dir_y,spot_origin_y,store_spot_ay,spot_dir_z,spot_origin_z,store_spot_az,
    store_valid
};

namespace light_shape {
template<class Memory,class Frame> void build(Memory &memory,Frame &frame) {
    using enum Access;
    const auto transform=frame.entry_transform();
    const auto shape=frame.entry_shape();
    const auto kind16=frame.enter_and_load_kind(shape);
    const auto frustum=at(shape,offsetof(LightVolumeShape,frustum));
    const auto dir_base=at(frustum,offsetof(LightFrustum,dir_x));

    if(kind16==shape_kinds::sphere) {
        const auto sra=memory.load_float(sphere_ra,at(shape,offsetof(LightVolumeShape,near_width)));
        const auto a=frame.mul_scalar(sra,memory.load_float(sphere_scale_a,at(transform,offsetof(LightTransform,scale))));
        memory.store_float(store_sphere_a,at(shape,0x3cu),a);
        const auto srb=memory.load_float(sphere_rb,at(shape,offsetof(LightVolumeShape,aspect)));
        const auto b=frame.mul_scalar(srb,memory.load_float(sphere_scale_b,at(transform,offsetof(LightTransform,scale))));
        const auto a_check=memory.load_float(reload_sphere_a,at(shape,0x3cu));
        const bool choose_a=frame.sphere_a_greater(a_check,b);
        memory.store_float(store_sphere_b,at(shape,0x40u),b);
        memory.store_float(store_sphere_max,at(shape,0x44u),choose_a?a_check:b);
    } else {
        const auto tx0=memory.load_float(target_x,at(transform,offsetof(LightTransform,target_x)));
        const auto dx=frame.sub_scalar(tx0,memory.load_float(origin_x,at(transform,offsetof(LightTransform,origin_x))));
        memory.store_float(store_dir_x,at(frustum,offsetof(LightFrustum,dir_x)),dx);
        const auto ty0=memory.load_float(target_y,at(transform,offsetof(LightTransform,target_y)));
        const auto dy=frame.sub_scalar(ty0,memory.load_float(origin_y,at(transform,offsetof(LightTransform,origin_y))));
        memory.store_float(store_dir_y,at(dir_base,4u),dy);
        const auto tz0=memory.load_float(target_z,at(transform,offsetof(LightTransform,target_z)));
        const auto dz=frame.sub_scalar(tz0,memory.load_float(origin_z,at(transform,offsetof(LightTransform,origin_z))));
        memory.store_float(store_dir_z,at(dir_base,8u),dz);

        const auto fz0=memory.load_float(fwd_z,at(transform,offsetof(LightTransform,forward_z)));
        const auto pz=frame.mul_scalar(fz0,memory.load_float(dot_dir_z,at(dir_base,8u)));
        const auto fy0=memory.load_float(fwd_y,at(transform,offsetof(LightTransform,forward_y)));
        const auto py=frame.mul_scalar(fy0,memory.load_float(dot_dir_y,at(dir_base,4u)));
        const auto pzy=frame.add_scalar(pz,py);
        const auto fx0=memory.load_float(fwd_x,at(transform,offsetof(LightTransform,forward_x)));
        const auto px=frame.mul_scalar(fx0,memory.load_float(dot_dir_x,dir_base));
        const auto proj=frame.add_scalar(pzy,px);
        const auto one=memory.load_float(one_constant,globals::one);
        memory.store_float(spill_proj,frame.proj_spill_slot(),proj);
        frame.x87_load_proj(x87_reload_proj,frame.proj_spill_slot());
        memory.store_float(store_length,at(frustum,offsetof(LightFrustum,length)),proj);
        const bool valid_proj=frame.x87_abs_gt_eps_and_positive(x87_epsilon,globals::epsilon);
        if(!valid_proj) {
            const auto fx=memory.load32(fallback_fwd_x,at(transform,offsetof(LightTransform,forward_x)));
            memory.store32(store_fb_x,dir_base,fx);
            const auto fy=memory.load32(fallback_fwd_y,at(transform,offsetof(LightTransform,forward_y)));
            memory.store32(store_fb_y,at(dir_base,4u),fy);
            const auto fz=memory.load32(fallback_fwd_z,at(transform,offsetof(LightTransform,forward_z)));
            memory.store32(store_fb_z,at(dir_base,8u),fz);
            memory.store_float(store_fb_len,at(frustum,offsetof(LightFrustum,length)),one);
        }

        const auto inv_len=frame.div_scalar(one,
            memory.load_float(reload_length,at(frustum,offsetof(LightFrustum,length))));
        const auto light_def=memory.load32(def_arg,frame.arg_definition());
        memory.store_float(store_norm_x,dir_base,
            frame.mul_scalar(memory.load_float(norm_dir_x,dir_base),inv_len));
        memory.store_float(store_norm_y,at(dir_base,4u),
            frame.mul_scalar(memory.load_float(norm_dir_y,at(dir_base,4u)),inv_len));
        memory.store_float(store_norm_z,at(dir_base,8u),
            frame.mul_scalar(memory.load_float(norm_dir_z,at(dir_base,8u)),inv_len));

        const auto aspect=memory.load_float(shape_aspect,at(shape,offsetof(LightVolumeShape,aspect)));
        memory.store_float(store_aspect,at(frustum,offsetof(LightFrustum,aspect)),aspect);
        const auto gel=memory.load32(gel_bitmap,at(light_def,offsetof(LightDefinition,gel_bitmap)));
        if(gel!=none) {
            const auto tags=memory.load32(tag_instances,globals::tag_instances);
            const auto group=memory.load32(gel_tag_def,
                at(element(tags,handle_index(gel),sizeof(TagInstance)),offsetof(TagInstance,definition)));
            const auto bdata=memory.load32(gel_bitmap_data,at(group,offsetof(BitmapGroup,bitmaps_data)));
            const auto h=widen16(memory.load16(gel_height,at(bdata,offsetof(BitmapData,height))));
            const auto w=widen16(memory.load16(gel_width,at(bdata,offsetof(BitmapData,width))));
            const auto gel_aspect=frame.mul_scalar(frame.div_scalar(frame.int_to_float(h),frame.int_to_float(w)),aspect);
            memory.store_float(store_gel_aspect,at(frustum,offsetof(LightFrustum,aspect)),gel_aspect);
        }

        if(memory.load32(kind_parallel,at(shape,offsetof(LightVolumeShape,kind)))==shape_kinds::parallel) {
            const auto pair=memory.load32(parallel_ptr,globals::parallel_tan);
            const auto tx=memory.load32(parallel_tan_x,pair);
            memory.store32(store_par_tan_x,at(frustum,offsetof(LightFrustum,tan_x)),tx);
            const auto ty=memory.load32(parallel_tan_y,at(pair,4u));
            memory.store32(store_par_tan_y,at(frustum,offsetof(LightFrustum,tan_y)),ty);
        } else {
            frame.x87_compute_tan_xy(
                at(shape,offsetof(LightVolumeShape,fov)),globals::half,
                at(frustum,offsetof(LightFrustum,tan_x)),
                at(frustum,offsetof(LightFrustum,aspect)),
                at(frustum,offsetof(LightFrustum,tan_y)));
        }

        const auto nw_bits=memory.load32(near_width_bits,at(shape,offsetof(LightVolumeShape,near_width)));
        const auto asp=memory.load_float(reload_aspect_near,at(frustum,offsetof(LightFrustum,aspect)));
        memory.store32(store_near_x,at(frustum,offsetof(LightFrustum,near_x)),nw_bits);
        const auto ny=frame.mul_scalar(asp,memory.load_float(near_width_mul,at(shape,offsetof(LightVolumeShape,near_width))));
        memory.store_float(store_near_y,at(frustum,offsetof(LightFrustum,near_y)),ny);

        const auto fd0=memory.load_float(far_dist,at(shape,offsetof(LightVolumeShape,far_distance)));
        const auto fclip=frame.mul_scalar(fd0,memory.load_float(scale_far,at(transform,offsetof(LightTransform,scale))));
        memory.store_float(store_far_clip,at(frustum,offsetof(LightFrustum,far_clip)),fclip);
        const auto nd0=memory.load_float(near_dist,at(shape,offsetof(LightVolumeShape,near_distance)));
        const auto nclip=frame.mul_scalar(nd0,memory.load_float(scale_near,at(transform,offsetof(LightTransform,scale))));
        memory.store_float(store_near_clip,at(frustum,offsetof(LightFrustum,near_clip)),nclip);

        const auto fc=memory.load_float(reload_far_clip,at(frustum,offsetof(LightFrustum,far_clip)));
        const auto tx_fc=frame.mul_scalar(memory.load_float(reload_tan_x,at(frustum,offsetof(LightFrustum,tan_x))),fc);
        const auto fx_ext=frame.add_scalar(tx_fc,memory.load_float(reload_near_x,at(frustum,offsetof(LightFrustum,near_x))));
        memory.store_float(store_far_x,at(frustum,offsetof(LightFrustum,far_x)),fx_ext);
        const auto ty_fc=frame.mul_scalar(memory.load_float(reload_tan_y,at(frustum,offsetof(LightFrustum,tan_y))),fc);
        const auto fy_ext=frame.add_scalar(ty_fc,memory.load_float(reload_near_y,at(frustum,offsetof(LightFrustum,near_y))));
        memory.store_float(store_far_y,at(frustum,offsetof(LightFrustum,far_y)),fy_ext);

        const auto fc2=memory.load_float(reload_far_clip2,at(frustum,offsetof(LightFrustum,far_clip)));
        const auto fdx_fc=frame.mul_scalar(memory.load_float(far_dir_x,dir_base),fc2);
        const auto far_cx=frame.add_scalar(fdx_fc,memory.load_float(far_origin_x,at(transform,offsetof(LightTransform,origin_x))));
        memory.store_float(store_far_cx,at(frustum,offsetof(LightFrustum,far_center_x)),far_cx);
        const auto fdy_fc=frame.mul_scalar(memory.load_float(far_dir_y,at(dir_base,4u)),fc2);
        const auto far_cy=frame.add_scalar(fdy_fc,memory.load_float(far_origin_y,at(transform,offsetof(LightTransform,origin_y))));
        memory.store_float(store_far_cy,at(frustum,offsetof(LightFrustum,far_center_y)),far_cy);
        const auto fdz_fc=frame.mul_scalar(memory.load_float(far_dir_z,at(dir_base,8u)),fc2);
        const auto far_cz=frame.add_scalar(fdz_fc,memory.load_float(far_origin_z,at(transform,offsetof(LightTransform,origin_z))));
        memory.store_float(store_far_cz,at(frustum,offsetof(LightFrustum,far_center_z)),far_cz);
    }

    // Common tail: right = cross(forward, up), apex = origin, spotlight apex shift
    const auto u_y=memory.load_float(cross_up_y,at(transform,offsetof(LightTransform,up_y)));
    const auto cf_y=memory.load_float(cross_fwd_y,at(transform,offsetof(LightTransform,forward_y)));
    const auto f_y_ux=frame.mul_scalar(cf_y,memory.load_float(cross_up_x,at(transform,offsetof(LightTransform,up_x))));
    const auto uy_fx=frame.mul_scalar(u_y,memory.load_float(cross_fwd_x,at(transform,offsetof(LightTransform,forward_x))));
    const auto cu_z=memory.load_float(cross_up_z,at(transform,offsetof(LightTransform,up_z)));
    const auto uz_fx=frame.mul_scalar(cu_z,memory.load_float(cross_fwd_x2,at(transform,offsetof(LightTransform,forward_x))));
    const auto cf_z=memory.load_float(cross_fwd_z,at(transform,offsetof(LightTransform,forward_z)));
    const auto fz_uy=frame.mul_scalar(cf_z,memory.load_float(cross_up_y2,at(transform,offsetof(LightTransform,up_y))));
    const auto rz=frame.sub_scalar(uy_fx,f_y_ux);
    const auto cf_z2=memory.load_float(cross_fwd_z2,at(transform,offsetof(LightTransform,forward_z)));
    const auto fz_ux=frame.mul_scalar(cf_z2,memory.load_float(cross_up_x2,at(transform,offsetof(LightTransform,up_x))));
    const auto ry=frame.sub_scalar(fz_ux,uz_fx);
    const auto cf_y2=memory.load_float(cross_fwd_y2,at(transform,offsetof(LightTransform,forward_y)));
    const auto fy_uz=frame.mul_scalar(cf_y2,memory.load_float(cross_up_z2,at(transform,offsetof(LightTransform,up_z))));
    memory.store_float(store_right_y,at(shape,offsetof(LightVolumeShape,right_y)),ry);
    memory.store_float(store_right_z,at(shape,offsetof(LightVolumeShape,right_z)),rz);
    const auto rx=frame.sub_scalar(fy_uz,fz_uy);
    memory.store_float(store_right_x,at(shape,offsetof(LightVolumeShape,right_x)),rx);
    memory.store_float(store_apex_offset_zero,at(shape,offsetof(LightVolumeShape,apex_offset)),0.0f);

    const auto apex=at(shape,offsetof(LightVolumeShape,apex_x));
    memory.store32(store_apex_x,apex,memory.load32(copy_origin_x,at(transform,offsetof(LightTransform,origin_x))));
    memory.store32(store_apex_y,at(apex,4u),memory.load32(copy_origin_y,at(transform,offsetof(LightTransform,origin_y))));
    memory.store32(store_apex_z,at(apex,8u),memory.load32(copy_origin_z,at(transform,offsetof(LightTransform,origin_z))));

    if(memory.load32(kind_spot,at(shape,offsetof(LightVolumeShape,kind)))==shape_kinds::spot) {
        const auto tx=memory.load_float(spot_tan_x,at(frustum,offsetof(LightFrustum,tan_x)));
        const auto eps=memory.load_float(spot_epsilon,globals::epsilon);
        if(frame.gt_scalar(tx,eps)) {
            const auto snw=memory.load_float(spot_near_width,at(shape,offsetof(LightVolumeShape,near_width)));
            const auto sdiv=frame.div_scalar(snw,memory.load_float(spot_div_tan_x,at(frustum,offsetof(LightFrustum,tan_x))));
            const auto off=frame.mul_scalar(sdiv,memory.load_float(spot_half,globals::half));
            memory.store_float(store_apex_offset,at(shape,offsetof(LightVolumeShape,apex_offset)),off);
            const auto sdx=memory.load_float(spot_dir_x,dir_base);
            const auto neg_off=frame.sub_scalar(0.0f,off);
            const auto sax=frame.add_scalar(frame.mul_scalar(sdx,neg_off),
                                            memory.load_float(spot_origin_x,at(transform,offsetof(LightTransform,origin_x))));
            memory.store_float(store_spot_ax,apex,sax);
            const auto sdy=memory.load_float(spot_dir_y,at(dir_base,4u));
            const auto say=frame.add_scalar(frame.mul_scalar(sdy,neg_off),
                                            memory.load_float(spot_origin_y,at(transform,offsetof(LightTransform,origin_y))));
            memory.store_float(store_spot_ay,at(apex,4u),say);
            const auto sdz=memory.load_float(spot_dir_z,at(dir_base,8u));
            const auto saz=frame.add_scalar(frame.mul_scalar(sdz,neg_off),
                                            memory.load_float(spot_origin_z,at(transform,offsetof(LightTransform,origin_z))));
            memory.store_float(store_spot_az,at(apex,8u),saz);
        }
    }

    frame.finish(shape);
}
} // namespace light_shape
} // namespace halo2::native_light_shape
