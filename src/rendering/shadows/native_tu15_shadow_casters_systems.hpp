#pragma once
// Native TU1.5 shadow caster collection, entered at 333F0 (shadow frame
// unit): the objects that cast shadows this frame, with their fade and
// projected size.
//
//   shadow_casters::collect   333F0(EBX = candidates; records, mode)
//
//   1. The shadow quality (4C66F4) is clamped to 0..3: the level.
//   2. Each candidate (handles at +0xC, 16-bit flags at +8, the count at +4)
//      is considered by the mode: 0 every candidate, 1 those flagged 0x4000,
//      2 those flagged 0x8000.
//   3. The object shadow fade (3D7E0) decides whether it is shown. While
//      fewer than 32 records are taken, a shown object becomes a 20-byte
//      record: its handle (twice), the fade, and its projected size: the
//      bounding radius times the projection scale (4C66F0) over its depth
//      along the view (the view depth row 4C6640.. applied to its center;
//      absolute, at least the minimum 46A178), times 46A11C. 3E9B0's class
//      of the object fills the record's first byte.
//   4. 4A7B0 then takes the records with the level's table (5332C8 + level
//      * 0x484), and 4AC20 the table it returns.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_shadow_casters.hpp) maps the role to the original
// instruction and owns the ABI of the children (3D7E0, 3E9B0, 4A7B0, 4AC20).
// The locals (the record count, the level, the candidate count, the fade,
// the shown byte and the center) live in the original's frame and are read
// and written as memory through Frame::local, in the original's order; the
// values the original keeps in callee-saved registers across the children
// are a Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_shadow_casters {
using native_object_update::at;
using native_object_update::element;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;

namespace globals {
inline constexpr std::uint32_t shadow_quality=0x4c66f4u;   // int32
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of object headers
inline constexpr std::uint32_t view_depth_x=0x4c6640u,view_depth_y=0x4c664cu,view_depth_z=0x4c6658u,
    view_depth_offset=0x4c6664u;                           // float: depth = x*cx + y*cy + z*cz + offset
inline constexpr std::uint32_t projection_scale=0x4c66f0u; // float
inline constexpr std::uint32_t depth_minimum=0x46a178u;    // float
inline constexpr std::uint32_t size_scale=0x46a11cu;       // float
inline constexpr std::uint32_t level_tables=0x5332c8u;     // one 0x484-byte table per level
}
inline constexpr std::uint32_t level_table_size=0x484u,level_count=3u;
inline constexpr std::uint32_t candidate_count=4u,candidate_flags=8u,candidate_handles=0xcu;
inline constexpr std::uint32_t flag_mode_one=0x4000u,flag_mode_two=0x8000u;
inline constexpr std::uint32_t record_limit=0x20u,record_stride=0x14u;
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_data=8u;
inline constexpr std::uint32_t object_center=0x30u,object_radius=0x3cu;
// The fourth argument of 4AC20 (the bit pattern of a float, 0.0025).
inline constexpr std::uint32_t final_constant=0x3ba3d70au,final_count=4u;

namespace shadow_casters {
// A record as the cursor (record + 8) addresses it.
namespace record {
inline constexpr std::uint32_t class_byte=0xfffffff8u,pad1=0xfffffff9u,pad2=0xfffffffau,handle=0xfffffffcu,
    handle_again=0u,fade=4u,size=8u;
}
enum class Local {records,mode,shown,count,level,fade,candidates,center};
enum class Access {
    quality,count_clear,level_zero,level_three,level_setting,candidate_count_read,candidate_count_store,
    records_argument,
    handles_table,flags_table,handle_entry,mode_argument,flag_entry,count_check,fade_read,headers,rows,object_data,
    radius,center_x,center_x_store,center_y,center_z,record_fade,record_handle,record_handle_again,view_z,view_x,
    center_x_reload,center_z_store,center_z_reload,view_y,center_y_store,center_y_reload,view_offset,depth_minimum,
    projection,size_scale,record_size,count_reload,record_class,record_pad2,record_pad1,count_store,candidates_reload,
    level_read,count_read,records_reload,records_again
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t candidates;   // EBX
    std::uint32_t index;        // EBP: the candidate index
    std::uint32_t handle;       // ESI: the candidate's object handle
    std::uint32_t cursor;       // EDI: the next record + 8
};
// The values the projection leaves in XMM0-XMM2 and the comparison that
// decided the depth floor.
struct Projection {std::uint32_t radius,depth,floored_depth,minimum,size;};

// Frame:
//   local(Local)            a local's or argument's address now
//   entry()                 EBX at entry
//   shadow_fade(kept)       3D7E0 for the candidate; returns AL
//   projected(Projection)   the depth and size ran
//   classify(kept)          3E9B0 for the candidate; returns AL
//   arrange(kept)           4A7B0 with the loaded values; returns EAX
//   finish(kept)            4AC20 with the loaded values
template<class Memory,class Frame> void collect(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    Kept kept{frame.entry(),0,0,0};
    // 1. The level.
    const auto setting=memory.load32(quality,globals::shadow_quality);
    memory.store32(count_clear,local(Local::count),0);
    if(signed_value(setting)<0)memory.store32(level_zero,local(Local::level),0);
    else {
        memory.store32(level_three,local(Local::level),level_count);
        if(!(signed_value(setting)>signed_value(level_count)))memory.store32(level_setting,local(Local::level),setting);
    }
    // 2-3. The candidates.
    const auto count=widen16(memory.load16(candidate_count_read,at(kept.candidates,candidate_count)));
    memory.store32(candidate_count_store,local(Local::candidates),count);
    const bool any=signed_value(count)>0;
    if(any) {
        kept.cursor=memory.load32(records_argument,local(Local::records))+8u;
        for(;;) {
            const auto handles=memory.load32(handles_table,at(kept.candidates,candidate_handles));
            const auto flags=memory.load32(flags_table,at(kept.candidates,candidate_flags));
            const auto index=widen16(kept.index&0xffffu);
            kept.handle=memory.load32(handle_entry,element(handles,index,4));
            const auto mode=memory.load32(mode_argument,local(Local::mode));
            const auto flag=memory.load16(flag_entry,element(flags,index,2));
            const bool considered=mode==0 || (mode==1 && (flag&flag_mode_one)) || (mode==2 && (flag&flag_mode_two));
            if(considered && (frame.shadow_fade(kept)&0xffu) &&
               signed_value(memory.load32(count_check,local(Local::count)))<signed_value(record_limit)) {
                const auto fade=memory.load32(fade_read,local(Local::fade));
                const auto rows_table=memory.load32(rows,at(memory.load32(headers,globals::object_headers),datum_data));
                const auto data=memory.load32(object_data,rows_table+(kept.handle&0xffffu)*header_size+header_data);
                Projection p{};
                p.radius=memory.load32(radius,at(data,object_radius));
                const auto center=at(data,object_center);
                memory.store32(center_x_store,local(Local::center),memory.load32(center_x,center));
                const auto cy=memory.load32(center_y,center+4u);
                const auto cz=memory.load32(center_z,center+8u);
                memory.store32(record_fade,kept.cursor+record::fade,fade);
                memory.store32(record_handle,kept.cursor+record::handle,kept.handle);
                memory.store32(record_handle_again,kept.cursor+record::handle_again,kept.handle);
                const auto weight_z=memory.load32(view_z,globals::view_depth_z);
                const auto weight_x=memory.load32(view_x,globals::view_depth_x);
                const auto x_part=sse::multiply(weight_x,memory.load32(center_x_reload,local(Local::center)));
                memory.store32(center_z_store,local(Local::center)+8u,cz);
                auto depth=sse::add(sse::multiply(weight_z,memory.load32(center_z_reload,local(Local::center)+8u)),x_part);
                const auto weight_y=memory.load32(view_y,globals::view_depth_y);
                memory.store32(center_y_store,local(Local::center)+4u,cy);
                depth=sse::add(depth,sse::multiply(weight_y,memory.load32(center_y_reload,local(Local::center)+4u)));
                depth=sse::add(depth,memory.load32(view_offset,globals::view_depth_offset));
                if(sse::below(depth,0u))depth=sse::subtract(0u,depth);
                p.depth=depth;
                p.minimum=memory.load32(depth_minimum,globals::depth_minimum);
                p.floored_depth=sse::not_above(depth,p.minimum)?p.minimum:depth;
                p.size=sse::multiply(sse::divide(memory.load32(projection,globals::projection_scale),p.floored_depth),p.radius);
                p.size=sse::multiply(p.size,memory.load32(size_scale,globals::size_scale));
                frame.projected(p);
                memory.store32(record_size,kept.cursor+record::size,p.size);
                const auto class_value=frame.classify(kept);
                const auto taken=memory.load32(count_reload,local(Local::count))+1u;
                memory.store8(record_class,kept.cursor+record::class_byte,class_value);
                memory.store8(record_pad2,kept.cursor+record::pad2,0);
                memory.store8(record_pad1,kept.cursor+record::pad1,0);
                memory.store32(count_store,local(Local::count),taken);
                kept.cursor+=record_stride;
            }
            const auto bound=memory.load32(candidates_reload,local(Local::candidates));
            kept.index+=1u;
            if(!(signed_value(kept.index)<signed_value(bound)))break;
        }
    }
    // 4. The level's table, then 4AC20.
    (void)memory.load32(level_read,local(Local::level));
    (void)memory.load32(count_read,local(Local::count));
    (void)memory.load32(records_reload,local(Local::records));
    frame.arrange(kept);
    (void)memory.load32(records_again,local(Local::records));
    frame.finish(kept);
}
} // namespace shadow_casters
} // namespace halo2::native_shadow_casters
