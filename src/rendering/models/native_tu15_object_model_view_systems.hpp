#pragma once
// Native TU1.5 object model view, entered at 212C70 (physics live support
// and map world support units): the view of an object's model a renderer
// or query works from.
//
//   object_model_view::fill   212C70(EAX = object handle, ESI = view) -> AL
//
// The object's tag (its definition) names a model (+0x38), and the model a
// render model (+0x24); without either the view is not filled and the
// result is 0. Otherwise the view holds the handle (+0), the object's +0xB4
// (+4), what BC190 fills from +8, the model's and the render model's
// handles (+0x3C, +0x40) and addresses (+0x44, +0x48), and the object's
// marker and node blocks (+0x4C, +0x50: the object plus its signed offsets
// +0x11A and +0x116); the result is 1. The definition and the render model's
// address pass through the original's two frame slots, and the model's
// handles are read again after BC190.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_model_view.hpp) maps the role to the original
// instruction and owns the ABI of BC190; the view, the model and the two
// blocks the original keeps in callee-saved registers across it are a Kept,
// refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_model_view {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_instance_shift=4u,
    instance_address=8u;
namespace object {inline constexpr std::uint32_t tag=0u,extra=0xb4u,nodes=0x116u,markers=0x11au;}
inline constexpr std::uint32_t definition_model=0x38u,model_render=0x24u;
namespace view {
inline constexpr std::uint32_t handle=0u,extra=4u,filled=8u,model_handle=0x3cu,render_handle=0x40u,
    model_address=0x44u,render_address=0x48u,markers=0x4cu,nodes=0x50u;
}

namespace object_model_view {
enum class Local {definition,render};
enum class Access {
    headers,header_datums,object_address,object_tag,tag_instances,definition_address,definition_store,model,
    model_address,render,nodes,render_address,markers,render_store,handle_store,extra,extra_store,
    definition_reload,model_reload,render_reload,model_store,render_handle_reload,model_address_store,markers_store,
    render_address_store,nodes_store,render_handle_store
};
struct Kept {
    std::uint32_t view;      // ESI
    std::uint32_t model;     // EBP: the model's address
    std::uint32_t markers;   // EDI
    std::uint32_t nodes;     // EBX
};
enum class Outcome {no_model,no_render,filled};
// Frame:
//   saved_model()             PUSH EBP once the model is named
//   blocks(kept, object)      the two blocks' registers (the offsets added to the object)
//   pushed_target(target)     BC190's argument (view + 8)
//   fill_from(kept)           BC190; refreshes kept
//   restored_model(), restored_markers()   the original's POP EBP, POP EDI
//   local(Local)
template<class Memory,class Frame> Outcome fill(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t view_address) {
    using enum Access;
    const auto tag_address=[&](Access role,std::uint32_t instances_address,std::uint32_t tag_handle) {
        return memory.load32(role,((tag_handle&0xffffu)<<tag_instance_shift)+instances_address+instance_address);
    };
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto address=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    const auto tag_handle=memory.load32(object_tag,at(address,object::tag));
    const auto instances=memory.load32(tag_instances,globals::tag_instances);
    const auto definition=tag_address(definition_address,instances,tag_handle);
    memory.store32(definition_store,frame.local(Local::definition),definition);
    const auto model_handle=memory.load32(model,at(definition,definition_model));
    if(model_handle==none)return Outcome::no_model;
    frame.saved_model();
    Kept kept{view_address,tag_address(model_address,instances,model_handle),0u,0u};
    const auto render_handle=memory.load32(render,at(kept.model,model_render));
    if(render_handle==none)return Outcome::no_render;
    const auto nodes_offset=widen16(memory.load16(nodes,at(address,object::nodes)));
    const auto render_location=tag_address(render_address,instances,render_handle);
    const auto markers_offset=widen16(memory.load16(markers,at(address,object::markers)));
    memory.store32(render_store,frame.local(Local::render),render_location);
    kept.markers=markers_offset+address;kept.nodes=nodes_offset+address;
    frame.blocks(kept,address);
    memory.store32(handle_store,at(kept.view,view::handle),handle);
    const auto extra_value=memory.load32(extra,at(address,object::extra));
    frame.pushed_target(at(kept.view,view::filled));
    memory.store32(extra_store,at(kept.view,view::extra),extra_value);
    frame.fill_from(kept);
    const auto stored_definition=memory.load32(definition_reload,frame.local(Local::definition));
    const auto stored_model=memory.load32(model_reload,at(stored_definition,definition_model));
    const auto stored_render=memory.load32(render_reload,frame.local(Local::render));
    memory.store32(model_store,at(kept.view,view::model_handle),stored_model);
    const auto stored_render_handle=memory.load32(render_handle_reload,at(kept.model,model_render));
    memory.store32(model_address_store,at(kept.view,view::model_address),kept.model);
    frame.restored_model();
    memory.store32(markers_store,at(kept.view,view::markers),kept.markers);
    frame.restored_markers();
    memory.store32(render_address_store,at(kept.view,view::render_address),stored_render);
    memory.store32(nodes_store,at(kept.view,view::nodes),kept.nodes);
    memory.store32(render_handle_store,at(kept.view,view::render_handle),stored_render_handle);
    return Outcome::filled;
}
} // namespace object_model_view
} // namespace halo2::native_object_model_view
