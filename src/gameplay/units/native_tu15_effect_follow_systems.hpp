#pragma once
// Native TU1.5 effect follow, entered at 17C370 (physics live support and
// death cycle units): an effect brought up to date with the object it
// belongs to; the result is whether the effect was removed.
//
//   effect_follow::update   17C370(effect handle) -> AL; RET 4
//
// The effect is a 0x190-byte record of the effect array (4F720C). An owner
// (+0x48) that BCDF0 no longer verifies (EAX the owner, any type pushed)
// removes the effect through 17A380 (EAX the handle). With an owner, its
// root object (BCFB0, ECX the owner) gives the effect's center (+0x30, from
// the object's +0x88) and location (+0x1C, +0x20, from its +0x28 and
// +0x2C) when the object has one (+4 bit 8); otherwise the effect has no
// location (NONE, and the word of [474C24] at +0x22). An effect following
// its owner (bit 1 or bit 9 of +2), when 17C330 accepts the owner (pushed),
// places its two markers through BCB70 (the owner, the marker name +0x10 or
// +0x14, the output +0x6C or +0x70 pushed; the first only when named) and its
// node (the word +0x18, through BCD80 into +0x24: EAX the owner, EDI the
// node, ESI the output); without, both outputs clear and a node offset
// becomes the default ([474C70]). An effect whose first marker failed, or
// whose tag (+4 among the tag instances) has bit 0 set, is offered for
// retirement (17A4A0, EAX the handle) when its tag has bit 0 or it has bit
// 4 of +2, and is removed when that accepts; one with bit 2 of +2 clear and
// a failed first marker restarts (17A0F0, 0 pushed); one with bit 2 set and
// bit 4 clear stops (17A1A0, the handle pushed). Finally a tag with bit 3 or
// bit 4 takes, for a verified owner whose root 3DDC0 finds (pushed) an
// active entry (+2) of [515F14] (0x100 bytes each), that entry's +0x2C and
// +0x28 into +0x104 and +0x108.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_effect_follow.hpp) maps the role to the original instruction,
// owns the ABI of the children and leaves through the original's epilogue
// for each exit. The values the original keeps in callee-saved registers
// across the children are a Kept, refreshed from the registers each
// returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_effect_follow {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t effects=0x4f720cu,tag_instances=0x4f0414u,object_headers=0x4ecbd0u,
    no_cluster=0x474c24u,default_offset=0x474c70u,lights=0x515f14u;}
inline constexpr std::uint32_t datum_data=0x44u,effect_size=0x190u,header_address=8u,tag_data=8u,handle_mask=0xffffu,
    none=0xffffffffu,any_type=0xffffffffu,light_shift=8u;
namespace effect {inline constexpr std::uint32_t flags=2u,tag=4u,first_marker=0x10u,second_marker=0x14u,node=0x18u,
    position=0x1cu,location=0x20u,cluster=0x22u,offset=0x24u,center=0x30u,owner=0x48u,first_out=0x6cu,second_out=0x70u,
    light_first=0x104u,light_second=0x108u;
    inline constexpr unsigned kept_bit=1u,persistent_bit=9u,restarted_bit=2u,retire_bit=4u;}
namespace object {inline constexpr std::uint32_t flags=4u,position=0x28u,location=0x2cu,center=0x88u;inline constexpr unsigned located_bit=8u;}
namespace definition {inline constexpr std::uint32_t flags=0u;inline constexpr unsigned retire_bit=0u,light_bit=3u,second_light_bit=4u;}
namespace light {inline constexpr std::uint32_t active=2u,first=0x2cu,second=0x28u;}
inline constexpr std::uint32_t x=0u,y=4u,z=8u;
// The frame (offsets from the ESP after all five registers are pushed):
// the tag's definition in ECX's slot, and the effect's handle.
namespace local {inline constexpr std::uint32_t definition=0x10u,argument=0x18u;}
inline constexpr bool bit(std::uint32_t v,unsigned n){return ((v>>n)&1u)!=0u;}

namespace effect_follow {
enum class Access {effects_load,effects_data,instances_load,argument_load,tag_load,definition_load,definition_store,owner_load,
    owner_reload,headers_load,datums_load,object_load,object_flags,center_x,center_x_store,center_y,center_y_store,center_z,
    center_z_store,object_position,position_store,object_location,location_store,no_cluster_load,position_clear,location_clear,
    cluster_store,flags_load,marker_owner,first_marker_load,first_marker_owner,second_marker_load,second_marker_owner,node_load,
    node_owner,handle_reload,node_test,first_out_clear,second_out_clear,default_offset_load,default_x,default_x_store,default_y,
    default_y_store,default_z,default_z_store,definition_reload,definition_retire,flags_byte,flags_word,lights_definition,
    lights_definition_flags,light_owner,lights_load,lights_data,light_active,light_first_load,light_first_store,
    light_second_load,light_second_store};
enum class Exit {removed,retired,updated};
// The original's EBX (whether the first marker was placed, in BL), EBP
// (the effect), ESI (no owner, later the owner) and EDI (the handle).
struct Kept {std::uint32_t placed,effect_address,object,handle;};

// Frame: see native_tu15_effect_follow.hpp.
template<class Memory,class Frame> Exit update(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The effect and its tag.
    frame.reserving();
    const auto effects=memory.load32(effects_load,globals::effects);
    frame.saving(1u);
    const auto data=memory.load32(effects_data,at(effects,datum_data));
    frame.saving(2u);
    const auto instances=memory.load32(instances_load,globals::tag_instances);
    frame.saving(3u);
    frame.saving(4u);
    kept.handle=memory.load32(argument_load,frame.local(local::argument));
    const auto offset=frame.effect_offset(kept.handle);
    const auto tag=memory.load32(tag_load,offset+data+effect::tag);
    kept.effect_address=frame.effect_address(offset,data);
    const auto definition=memory.load32(definition_load,frame.tag_offset(tag)+instances+tag_data);
    memory.store32(definition_store,frame.local(local::definition),definition);
    // An owner no longer verified removes the effect.
    const auto owner=memory.load32(owner_load,at(kept.effect_address,effect::owner));
    kept.object=frame.no_owner();
    if(frame.different(owner,kept.object) && !frame.verified(kept,owner,false)) {
        frame.remove(kept);
        return Exit::removed;
    }
    const auto owner_now=memory.load32(owner_reload,at(kept.effect_address,effect::owner));
    if(!frame.different(owner_now,kept.object))return Exit::updated;
    // The root object's center and location.
    const auto root=frame.root(kept,false);
    const auto headers=memory.load32(headers_load,globals::object_headers);
    const auto datums=memory.load32(datums_load,at(headers,datum_data));
    const auto object_address=memory.load32(object_load,datums+4u*frame.root_index(root)+header_address);
    if(frame.located(memory.load32(object_flags,at(object_address,object::flags)))) {
        frame.center_source(object_address);
        const auto cx=memory.load32(center_x,at(object_address,object::center+x));
        kept.object=frame.center_target(kept.effect_address);
        memory.store32(center_x_store,at(kept.effect_address,effect::center+x),cx);
        memory.store32(center_y_store,at(kept.effect_address,effect::center+y),memory.load32(center_y,at(object_address,object::center+y)));
        memory.store32(center_z_store,at(kept.effect_address,effect::center+z),memory.load32(center_z,at(object_address,object::center+z)));
        memory.store32(position_store,at(kept.effect_address,effect::position),memory.load32(object_position,at(object_address,object::position)));
        memory.store32(location_store,at(kept.effect_address,effect::location),memory.load32(object_location,at(object_address,object::location)));
    } else {
        const auto cluster=memory.load16(no_cluster_load,globals::no_cluster);
        memory.store32(position_clear,at(kept.effect_address,effect::position),kept.object);
        memory.store16(location_clear,at(kept.effect_address,effect::location),kept.object);
        memory.store16(cluster_store,at(kept.effect_address,effect::cluster),cluster);
    }
    // The markers and the node.
    const auto flags=frame.flags_word(memory.load16(flags_load,at(kept.effect_address,effect::flags)));
    if(frame.following(flags)) {
        memory.load32(marker_owner,at(kept.effect_address,effect::owner));
        bool restart_or_retire=true;
        if(frame.markers(kept)) {
            const auto first=memory.load32(first_marker_load,at(kept.effect_address,effect::first_marker));
            bool placed=true;
            if(frame.named(first)) {
                frame.first_output(kept.effect_address);
                memory.load32(first_marker_owner,at(kept.effect_address,effect::owner));
                placed=frame.marked(kept,true);
            }
            kept.placed=frame.placing(placed);
            memory.load32(second_marker_load,at(kept.effect_address,effect::second_marker));
            memory.load32(second_marker_owner,at(kept.effect_address,effect::owner));
            frame.second_output(kept);
            const auto node=memory.load16(node_load,at(kept.effect_address,effect::node));
            if(frame.noded(node)) {
                frame.node_index(node);
                memory.load32(node_owner,at(kept.effect_address,effect::owner));
                frame.place_node(kept);
                kept.handle=memory.load32(handle_reload,frame.local(local::argument));
            }
            restart_or_retire=!frame.placed_tested(kept.placed);
        } else {
            const auto node=memory.load16(node_test,at(kept.effect_address,effect::node));
            const auto cleared=frame.cleared(node);
            memory.store32(first_out_clear,at(kept.effect_address,effect::first_out),cleared);
            memory.store32(second_out_clear,at(kept.effect_address,effect::second_out),cleared);
            if(node!=0u) {
                const auto source=memory.load32(default_offset_load,globals::default_offset);
                frame.offset_target(kept.effect_address);
                memory.store32(default_x_store,at(kept.effect_address,effect::offset+x),memory.load32(default_x,source+x));
                memory.store32(default_y_store,at(kept.effect_address,effect::offset+y),memory.load32(default_y,source+y));
                memory.store32(default_z_store,at(kept.effect_address,effect::offset+z),memory.load32(default_z,source+z));
            }
        }
        // Restart, stop or retire.
        bool offer=false;
        if(restart_or_retire) {
            const auto definition_now=memory.load32(definition_reload,frame.local(local::definition));
            if(frame.tag_retire(memory.load8(definition_retire,at(definition_now,definition::flags))))offer=true;
            else if(!frame.restarted(memory.load8(flags_byte,at(kept.effect_address,effect::flags))))frame.restart(kept);
        } else {
            const auto word=memory.load16(flags_word,at(kept.effect_address,effect::flags));
            if(frame.placed_restarted(word)) {
                if(frame.placed_retire(word))offer=true;
                else frame.stop(kept);
            }
        }
        if(offer && frame.retire(kept))return Exit::retired;
    }
    // The light.
    const auto definition_last=memory.load32(lights_definition,frame.local(local::definition));
    const auto tag_flags=memory.load32(lights_definition_flags,at(definition_last,definition::flags));
    if(frame.lit(tag_flags)) {
        kept.object=memory.load32(light_owner,at(kept.effect_address,effect::owner));
        if(frame.verified(kept,kept.object,true)) {
            const auto light_root=frame.root(kept,true);
            const auto index=frame.light_index(kept,light_root);
            if(frame.different(index,none)) {
                const auto lights=memory.load32(lights_load,globals::lights);
                const auto lights_data_address=memory.load32(lights_data,at(lights,datum_data));
                const auto offset_light=frame.light_offset(index);
                const auto active=memory.load8(light_active,offset_light+lights_data_address+light::active);
                const auto entry=frame.light_address(offset_light,lights_data_address);
                if(frame.active(active)) {
                    memory.store32(light_first_store,at(kept.effect_address,effect::light_first),memory.load32(light_first_load,at(entry,light::first)));
                    memory.store32(light_second_store,at(kept.effect_address,effect::light_second),memory.load32(light_second_load,at(entry,light::second)));
                }
            }
        }
    }
    return Exit::updated;
}
} // namespace effect_follow
} // namespace halo2::native_effect_follow
