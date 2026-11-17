#pragma once
// Native TU1.5 marker attachments of an object, entered at 41970 (model render
// unit; the native render prepare 41C10 calls it for each object it draws).
//
//   marker_attachments::submit   41970  places the object's lens flares,
//                                       light volumes and 'tdtl' attachments
//                                       at its markers
//
// An object definition lists attachments (tag reference, marker name, change
// color, scale functions). The ones whose tag is a lens flare ('lens'), a
// light volume ('MGS2') or a 'tdtl' and that name a marker are drawn here, at
// every marker of the object with that name (BAD60 finds them, at most 65).
// Each attachment's primary scale (C0700) and color (white, or the object's
// change color named by the attachment, BCD80) apply to all its markers.
// Per marker:
//   - a lens flare is updated (3E9B0) and submitted (2DB80), at most 64 per
//     call; the 65th sets a once-only flag and ends that attachment. Its
//     scale is 1.0, or the marker's scale when the lens flare asks for it;
//   - a light volume is submitted (42840) with a scale of 1.0, or the
//     marker's scale when its first entry asks for it;
//   - a 'tdtl' is submitted (42990) with a scale of 1.0.
// The whole pass is skipped when the caller's first argument is nonzero.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores. Every access
// names its role (the Access enum); the boundary adapter
// (native_tu15_attachment_schedule.hpp) maps the role to the original
// instruction for the diagnostic cursor. The calls into translated code,
// registers and flags belong to that adapter, reached through a Frame object.
// The pass's locals live in the original's stack frame, so they are read and
// written as memory, with roles, in the original's order; Frame::local gives
// their addresses (the frame moves when a child does not restore the stack
// pointer).
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width (locals are
//     read again after every call, as the original does);
//   - address arithmetic is 32-bit and wraps as the original's does;
//   - the values the original keeps in callee-saved registers across a call
//     (the object, the attachment row, the marker count and the current
//     marker, the attachment's index) are a Walk. A conforming callee returns
//     them unchanged; the adapter hands back what the callee actually
//     returned, so the pass continues exactly as the original does even when
//     a callee does not preserve them.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_attachments {
// The object and data_array layouts are shared with the object update systems.
using native_object_update::DataArray;
using native_object_update::ObjectData;
using native_object_update::ObjectHeader;
using native_object_update::at;
using native_object_update::element;
using native_object_update::handle_index;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;

// ------------------------------------------------------------- game layouts
// Globals at fixed TU1.5 addresses.
namespace globals {
inline constexpr std::uint32_t object_headers=native_object_update::globals::object_headers;
inline constexpr std::uint32_t tag_instances=0x4f0414u;        // TagInstance[]: the loaded tags
inline constexpr std::uint32_t white=0x474c70u;                // -> the default attachment color (3 floats)
inline constexpr std::uint32_t one=0x46a0d0u;                  // 1.0f
inline constexpr std::uint32_t lens_flare_limit_seen=0x56e197u;  // byte: set once an object passed the lens flare limit
}
constexpr std::uint32_t tag_row(std::uint32_t tag){return tag&0xffffu;}

// A tag block: a count and the address of its elements.
struct TagBlock {
    std::int32_t count;
    std::uint32_t address;
};
// A loaded tag's instance row (16 bytes).
struct TagInstance {
    std::uint32_t group;
    std::uint32_t identifier;
    std::uint32_t definition;          // +8: the tag's data
    std::uint32_t size;
};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,definition)==8);

// An object definition: the field this system reads.
struct ObjectDefinition {
    std::uint8_t unused_here_00[0x94];
    TagBlock attachments;              // +0x94: ObjectAttachment
};
static_assert(offsetof(ObjectDefinition,attachments)==0x94);

// One of an object definition's attachments (0x18 bytes).
struct ObjectAttachment {
    std::uint32_t group;               // the attachment tag's group ('lens', 'MGS2', 'tdtl', ...)
    std::uint32_t tag;                 // +4: its tag index, or NONE
    std::uint32_t marker;              // +8: the marker name (a string id)
    std::int16_t change_color;         // +0xC: the object's change color to use, 0 for white
    std::uint16_t unused_here_0e;
    std::uint32_t primary_scale;       // +0x10: read by C0700
    std::uint32_t secondary_scale;     // +0x14
};
static_assert(sizeof(ObjectAttachment)==0x18 && offsetof(ObjectAttachment,tag)==4 && offsetof(ObjectAttachment,marker)==8 &&
              offsetof(ObjectAttachment,change_color)==0xc);
namespace attachment_group {
inline constexpr std::uint32_t lens_flare=0x6c656e73u;    // 'lens'
inline constexpr std::uint32_t light_volume=0x4d475332u;  // 'MGS2'
inline constexpr std::uint32_t tdtl=0x7464746cu;          // 'tdtl'
}
constexpr bool placed_at_markers(std::uint32_t group) {
    return group==attachment_group::lens_flare || group==attachment_group::light_volume || group==attachment_group::tdtl;
}

// A lens flare definition: the field this system reads.
struct LensFlareDefinition {
    std::uint8_t unused_here_00[0x28];
    std::uint8_t flags;                // +0x28: lens_flare_flags
};
static_assert(offsetof(LensFlareDefinition,flags)==0x28);
namespace lens_flare_flags {
inline constexpr std::uint32_t marker_scale=0x40u;   // bit 6: scaled by the marker
}
// A light volume definition: the field this system reads. Its first entry's
// byte 0, bit 5, asks for the marker's scale.
struct LightVolumeDefinition {
    std::uint8_t unused_here_00[8];
    TagBlock entries;                  // +8
};
static_assert(offsetof(LightVolumeDefinition,entries)==8);
namespace light_volume_flags {
inline constexpr std::uint32_t marker_scale=0x20u;
}

// real_matrix4x3: a scale, three axes and a position.
struct RealMatrix4x3 {
    float scale;
    float forward[3];
    float left[3];
    float up[3];
    float position[3];
};
static_assert(sizeof(RealMatrix4x3)==0x34);
// One of an object's markers as BAD60 writes it (0x70 bytes).
struct ObjectMarker {
    std::uint8_t unused_here_00[0x38];   // the node and its relative transform (not read here)
    RealMatrix4x3 world;                 // +0x38: the marker in the world
    std::uint8_t unused_here_6c[4];
};
static_assert(sizeof(ObjectMarker)==0x70 && offsetof(ObjectMarker,world)==0x38);
inline constexpr std::uint32_t marker_limit=65u;       // markers BAD60 may return
inline constexpr std::int32_t lens_flare_limit=64;     // lens flares per call
inline constexpr std::uint32_t unit_scale=0x3f800000u; // 1.0f, passed as a constant to 3E9B0 and 42990

// The pass's locals: the original's whole 0x1CA0-byte stack frame.
struct Locals {
    std::int32_t lens_flares;          // submitted so far
    std::uint32_t attachment_offset;   // the attachment's byte offset in the list
    std::int32_t attachment_index;
    std::int32_t marker_index;
    std::uint32_t primary_scale;       // float bits
    std::uint32_t flare_scale;         // float bits
    std::uint32_t object_index;        // the object handle's index
    std::uint32_t definition;          // the ObjectDefinition
    std::uint32_t color[3];            // float bits
    std::int32_t marker_count;
    ObjectMarker markers[marker_limit];
};
static_assert(sizeof(Locals)==0x1ca0 && offsetof(Locals,color)==0x20 && offsetof(Locals,markers)==0x30);

// ------------------------------------------------------- marker attachments
// 41970. Frame:
//   allocate()                     the stack frame (32A660, the stack probe)
//   local(Local)                   the address of a local or argument
//   find_markers(walk, name, out)  BAD60: the object's markers with that
//                                  name, into out; AX is the count
//   primary_scale(walk)            C0700 (attachment index, object): float bits
//   change_color(walk, object, color, out)  BCD80: the object's change color
//   lens_flare(walk, flare, scale) 3E9B0 (the scale is the flare's, as
//                                  float bits): updates the flare; its result
//   submit_lens_flare(walk, tag, flare)  2DB80
//   light_volume(walk, volume)     42840 (object, tag, the marker's forward
//                                  axis, the primary scale; the marker's up
//                                  axis and position, the color, the scale)
//   tdtl(walk, tag)                42990 (object, tag, -1, 0, the marker's
//                                  forward axis, 1.0; its position and up axis)
namespace marker_attachments {
enum class Local {
    lens_flares,attachment_offset,attachment_index,marker_index,primary_scale,flare_scale,object_index,definition,
    color,marker_count,markers,
    skip,        // argument: nonzero skips the pass (a 16-bit value)
    object       // argument: the object handle
};
enum class Access {
    skip,object_table,object_rows,object_argument,object_index_store,object,definition_tag,tag_instances,definition_data,
    attachment_count,definition_store,lens_flares_clear,index_clear,offset_clear,
    attachment_list,offset_read,attachment_group,attachment_tag,marker_name,marker_count_store,
    white,white_red,red_store,white_green,green_store,white_blue,blue_store,change_color,primary_scale_store,
    object_for_color,object_after_color,marker_index_clear,
    marker_group,
    lens_count,limit_seen,limit_seen_set,lens_tag,lens_instances,lens_one,lens_definition,lens_flags,lens_scale_store,
    lens_marker_scale,lens_marker_scale_store,lens_scale,lens_primary,lens_index,lens_object_index,lens_tag_again,
    lens_count_again,lens_object_again,lens_count_store,
    volume_tag,volume_instances,volume_one,volume_definition,volume_entries_count,volume_entries,volume_flags,
    volume_marker_scale,volume_primary,
    tdtl_tag,
    marker_index_read,marker_count_read,marker_index_store,
    index_read,offset_again,definition_again,count_again,index_store,offset_store
};
enum class Exit {skipped,no_attachments,submitted};
// Kept across the calls.
struct Walk {
    std::uint32_t object;        // the object handle
    std::uint32_t attachment;    // the ObjectAttachment
    std::uint32_t index;         // the attachment's index (across BAD60)
    std::uint32_t marker_count;  // across C0700 and BCD80
    std::uint32_t marker;        // the ObjectMarker (across the marker's calls)
};
// The places a marker's attachment is drawn at.
constexpr std::uint32_t forward_of(std::uint32_t marker){return at(marker,offsetof(ObjectMarker,world.forward));}
constexpr std::uint32_t up_of(std::uint32_t marker){return at(marker,offsetof(ObjectMarker,world.up));}
constexpr std::uint32_t position_of(std::uint32_t marker){return at(marker,offsetof(ObjectMarker,world.position));}
// 3E9B0's arguments (besides the marker's forward axis): 0, the object's
// index, the flare's index among those submitted, the marker's position, the
// color, 1.0, the primary scale and the flare's scale.
struct LensFlare {
    std::uint32_t object_index;
    std::uint32_t flare_index;
    std::uint32_t primary_scale;
    std::uint32_t flare_scale;
};
// 42840's arguments (besides the marker's axes, position and the color).
struct LightVolume {
    std::uint32_t tag;
    std::uint32_t primary_scale;
    std::uint32_t scale;         // 1.0, or the marker's scale
};

// One marker of the walk's attachment; false when the lens flare limit ends
// the attachment.
template<class Memory,class Frame> bool submit_at_marker(Memory &memory,Frame &frame,Walk &walk) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    // The group is read again for every marker.
    const auto group=memory.load32(marker_group,at(walk.attachment,offsetof(ObjectAttachment,group)));
    if(group==attachment_group::lens_flare) {
        if(signed_value(memory.load32(lens_count,local(Local::lens_flares)))>=lens_flare_limit) {
            if(!memory.load8(limit_seen,globals::lens_flare_limit_seen))memory.store8(limit_seen_set,globals::lens_flare_limit_seen,1);
            return false;
        }
        const auto tag=memory.load32(lens_tag,at(walk.attachment,offsetof(ObjectAttachment,tag)));
        const auto instances=memory.load32(lens_instances,globals::tag_instances);
        auto scale=memory.load32(lens_one,globals::one);
        const auto definition=memory.load32(lens_definition,
            at(element(instances,tag_row(tag),sizeof(TagInstance)),offsetof(TagInstance,definition)));
        const bool marker_scaled=memory.load8(lens_flags,at(definition,offsetof(LensFlareDefinition,flags)))&lens_flare_flags::marker_scale;
        memory.store32(lens_scale_store,local(Local::flare_scale),scale);
        if(marker_scaled) {
            scale=memory.load32(lens_marker_scale,at(walk.marker,offsetof(ObjectMarker,world.scale)));
            memory.store32(lens_marker_scale_store,local(Local::flare_scale),scale);
        }
        LensFlare flare{};
        flare.flare_scale=memory.load32(lens_scale,local(Local::flare_scale));
        flare.primary_scale=memory.load32(lens_primary,local(Local::primary_scale));
        flare.flare_index=memory.load32(lens_index,local(Local::lens_flares));
        flare.object_index=memory.load32(lens_object_index,local(Local::object_index));
        const auto state=frame.lens_flare(walk,flare,scale);
        frame.submit_lens_flare(walk,memory.load32(lens_tag_again,at(walk.attachment,offsetof(ObjectAttachment,tag))),state);
        const auto submitted=memory.load32(lens_count_again,local(Local::lens_flares));
        walk.object=memory.load32(lens_object_again,local(Local::object));
        memory.store32(lens_count_store,local(Local::lens_flares),submitted+1u);
    } else if(group==attachment_group::light_volume) {
        LightVolume volume{};
        volume.tag=memory.load32(volume_tag,at(walk.attachment,offsetof(ObjectAttachment,tag)));
        const auto instances=memory.load32(volume_instances,globals::tag_instances);
        volume.scale=memory.load32(volume_one,globals::one);
        const auto definition=memory.load32(volume_definition,
            at(element(instances,tag_row(volume.tag),sizeof(TagInstance)),offsetof(TagInstance,definition)));
        if(signed_value(memory.load32(volume_entries_count,at(definition,offsetof(LightVolumeDefinition,entries.count))))>0) {
            const auto first=memory.load32(volume_entries,at(definition,offsetof(LightVolumeDefinition,entries.address)));
            if(memory.load8(volume_flags,first)&light_volume_flags::marker_scale)
                volume.scale=memory.load32(volume_marker_scale,at(walk.marker,offsetof(ObjectMarker,world.scale)));
        }
        volume.primary_scale=memory.load32(volume_primary,local(Local::primary_scale));
        frame.light_volume(walk,volume);
    } else if(group==attachment_group::tdtl) {
        frame.tdtl(walk,memory.load32(tdtl_tag,at(walk.attachment,offsetof(ObjectAttachment,tag))));
    }
    return true;
}

template<class Memory,class Frame> Exit submit(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    frame.allocate();
    if(memory.load16(skip,local(Local::skip)))return Exit::skipped;

    // The object and its definition. The attachment index starts at zero.
    Walk walk{};
    const auto headers=memory.load32(object_table,globals::object_headers);
    const auto rows=memory.load32(object_rows,at(headers,offsetof(DataArray,data)));
    walk.object=memory.load32(object_argument,local(Local::object));
    memory.store32(object_index_store,local(Local::object_index),handle_index(walk.object));
    const auto data=memory.load32(object,at(element(rows,handle_index(walk.object),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
    const auto definition_index=memory.load32(definition_tag,at(data,offsetof(ObjectData,definition)));
    const auto instances=memory.load32(tag_instances,globals::tag_instances);
    auto definition=memory.load32(definition_data,
        at(element(instances,tag_row(definition_index),sizeof(TagInstance)),offsetof(TagInstance,definition)));
    const auto count=memory.load32(attachment_count,at(definition,offsetof(ObjectDefinition,attachments.count)));
    memory.store32(definition_store,local(Local::definition),definition);
    memory.store32(lens_flares_clear,local(Local::lens_flares),0);
    memory.store32(index_clear,local(Local::attachment_index),0);
    if(signed_value(count)<=0)return Exit::no_attachments;
    memory.store32(offset_clear,local(Local::attachment_offset),0);

    for(;;) {
        const auto list=memory.load32(attachment_list,at(definition,offsetof(ObjectDefinition,attachments.address)));
        const auto offset=memory.load32(offset_read,local(Local::attachment_offset));
        const auto group=memory.load32(attachment_group,list+offset);
        walk.attachment=list+offset;
        if(placed_at_markers(group) && memory.load32(attachment_tag,at(walk.attachment,offsetof(ObjectAttachment,tag)))!=none) {
            // The object's markers with the attachment's marker name.
            const auto name=memory.load32(marker_name,at(walk.attachment,offsetof(ObjectAttachment,marker)));
            walk.marker_count=widen16(frame.find_markers(walk,name,local(Local::markers)));
            memory.store32(marker_count_store,local(Local::marker_count),walk.marker_count);
            const auto primary_scale=frame.primary_scale(walk);
            // The color: white, or the object's change color.
            const auto white_color=memory.load32(white,globals::white);
            const auto red=memory.load32(white_red,white_color);
            memory.store32(red_store,local(Local::color),red);
            const auto green=memory.load32(white_green,white_color+4u);
            memory.store32(green_store,local(Local::color)+4u,green);
            const auto blue=memory.load32(white_blue,white_color+8u);
            memory.store32(blue_store,local(Local::color)+8u,blue);
            const auto color=memory.load16(change_color,at(walk.attachment,offsetof(ObjectAttachment,change_color)));
            memory.store32(primary_scale_store,local(Local::primary_scale),primary_scale);
            if(color) {
                frame.change_color(walk,memory.load32(object_for_color,local(Local::object)),widen16(color),local(Local::color));
                walk.object=memory.load32(object_after_color,local(Local::object));
            }
            memory.store32(marker_index_clear,local(Local::marker_index),0);
            if(signed_value(walk.marker_count)>0) {
                walk.marker=local(Local::markers);
                for(;;) {
                    if(!submit_at_marker(memory,frame,walk))break;
                    const auto next=memory.load32(marker_index_read,local(Local::marker_index))+1u;
                    const auto markers=memory.load32(marker_count_read,local(Local::marker_count));
                    walk.marker+=std::uint32_t(sizeof(ObjectMarker));
                    memory.store32(marker_index_store,local(Local::marker_index),next);
                    if(!(signed_value(next)<signed_value(markers)))break;
                }
            }
        }
        // The next attachment.
        const auto index=memory.load32(index_read,local(Local::attachment_index))+1u;
        const auto next_offset=memory.load32(offset_again,local(Local::attachment_offset))+std::uint32_t(sizeof(ObjectAttachment));
        definition=memory.load32(definition_again,local(Local::definition));
        const auto total=memory.load32(count_again,at(definition,offsetof(ObjectDefinition,attachments.count)));
        memory.store32(index_store,local(Local::attachment_index),index);
        memory.store32(offset_store,local(Local::attachment_offset),next_offset);
        if(!(signed_value(index)<signed_value(total)))return Exit::submitted;
        walk.index=index;
    }
}
} // namespace marker_attachments
} // namespace halo2::native_object_attachments
