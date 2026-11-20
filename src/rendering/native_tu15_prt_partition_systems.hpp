#pragma once
// Native TU1.5 PRT object partition: 136D50, the first step of the PRT
// (precomputed radiance transfer) lighting pass 136230
// (docs/native_replacement/136D50_prt_partition.md).
//
//   prt_partition::partition(view)
//     The view's object render queue (built by the object admission, 135B60)
//     lists every visible object. This step moves the objects whose render
//     model carries PRT data to the front of the queue, in queue order: each
//     such entry is swapped (166C30, the queue's four-stream swap) with the
//     first entry that is not one. An entry has PRT data when it names an
//     object whose definition's model names a render model whose PRT info
//     block is not empty. The queue's count is unchanged.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access) and the boundary adapter
// (native_tu15_prt_partition.hpp) maps the role to the original instruction.
// The argument and the swap come from a Frame object that the adapter also
// supplies. Registers, flags and stack slots belong to the adapter.
//
// What stays exact, because translated neighbours and the swap observe it:
//   - memory accesses happen in the original order and width; after each
//     entry the view, its queue and the queue count are read again (the swap
//     is a separate owner and may change them);
//   - the entry index is the original's full 32-bit counter; the queue is
//     indexed with its sign-extended low word;
//   - the values the original keeps in callee-saved registers across the
//     swap are the fields of Kept. The adapter hands back what the swap
//     actually returned, so the logic continues exactly as the original does.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_prt_partition {
using native_object_update::at;
using native_object_update::element;
using native_object_update::signed_value;
using native_object_update::widen16;
using native_object_update::none;
using native_object_update::handle_index;
using native_object_update::DataArray;
using native_object_update::ObjectHeader;
using native_object_update::ObjectData;

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t object_headers=native_object_update::globals::object_headers; // DataArray* of ObjectHeader
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // TagInstance[]: the loaded tags
}
// A tag instance (16 bytes) of the loaded tag table.
struct TagInstance {
    std::uint32_t unused_here[2];
    std::uint32_t definition;       // +8: the tag's data, or null
    std::uint32_t unused_here_c;
};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,definition)==8);
// The fields of an object definition (obje) read here.
struct ObjectDefinition {
    std::uint8_t unused_here[0x38];
    std::uint32_t model;            // +0x38: the model (hlmt) tag's handle, NONE for none
};
static_assert(offsetof(ObjectDefinition,model)==0x38);
// The fields of a model definition (hlmt) read here.
struct ModelDefinition {
    std::uint32_t render_model_group;
    std::uint32_t render_model;     // +4: the render model (mode) tag's handle, NONE for none
};
static_assert(offsetof(ModelDefinition,render_model)==4);
// The fields of a render model definition (mode) read here.
struct RenderModelDefinition {
    std::uint8_t unused_here[0x74];
    std::int32_t prt_info_count;    // +0x74: entries of the PRT info block
};
static_assert(offsetof(RenderModelDefinition,prt_info_count)==0x74);
// The view (the same layout 134D30 and 135B60 use): only its object queue.
struct View {
    std::uint8_t unused_here[0x14];
    std::uint32_t object_queue;     // +0x14: RenderQueue*
};
static_assert(offsetof(View,object_queue)==0x14);
// A render queue (166B60 appends, 166C30 swaps): four parallel streams with
// a shared count. Only the count and the handle stream are read here.
struct RenderQueue {
    std::int32_t capacity;
    std::int16_t count;             // +4
    std::uint16_t unused_here;
    std::uint32_t flags;            // +8: uint16[]
    std::uint32_t handles;          // +0xC: uint32[], object handles (NONE for none)
};
static_assert(offsetof(RenderQueue,count)==4 && offsetof(RenderQueue,handles)==0xc);

// ------------------------------------------------------------ access roles
enum class Access : unsigned {
    entry_view,entry_queue,entry_count,object_headers,tag_instances,
    queue_handles,queue_handle,object_rows,object_data,
    object_definition_tag,object_definition,model_tag,model_definition,render_model_tag,render_model,prt_info_count,
    next_view,next_queue,next_count
};

// The values the original keeps in callee-saved registers across the swap
// (EBX, ESI, EBP, EDI), by use.
struct Kept {
    std::uint32_t index;        // the queue entry being examined
    std::uint32_t front;        // the PRT entries found so far: the next front slot
    std::uint32_t objects;      // the object header array
    std::uint32_t tags;         // the loaded tag table
};

namespace prt_partition {
// Frame:
//   argument(role)       the view, read from its slot
//   swap(k, queue)       166C30: swap queue entries k.index and k.front
//   finish(looped)       the return (looped: the queue was not empty)

// The definition of a loaded tag (the tag instance's data pointer).
template<class Memory> std::uint32_t definition_of(Memory &memory,Access access,std::uint32_t tags,std::uint32_t tag) {
    return memory.load32(access,at(element(tags,handle_index(tag),sizeof(TagInstance)),offsetof(TagInstance,definition)));
}

// Whether a queue entry names an object whose render model has PRT data.
// Every link may be missing; the render model's own data is not tested.
template<class Memory> bool has_prt(Memory &memory,const Kept &k,std::uint32_t handle) {
    using enum Access;
    if(handle==none)return false;
    const auto rows=memory.load32(object_rows,at(k.objects,offsetof(DataArray,data)));
    const auto object=memory.load32(object_data,at(element(rows,handle_index(handle),sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
    if(!object)return false;
    const auto object_tag=memory.load32(object_definition_tag,at(object,offsetof(ObjectData,definition)));
    if(object_tag==none)return false;
    const auto definition=definition_of(memory,object_definition,k.tags,object_tag);
    if(!definition)return false;
    const auto model=memory.load32(model_tag,at(definition,offsetof(ObjectDefinition,model)));
    if(model==none)return false;
    const auto model_data=definition_of(memory,model_definition,k.tags,model);
    if(!model_data)return false;
    const auto mode=memory.load32(render_model_tag,at(model_data,offsetof(ModelDefinition,render_model)));
    if(mode==none)return false;
    const auto mode_data=definition_of(memory,render_model,k.tags,mode);
    return signed_value(memory.load32(prt_info_count,at(mode_data,offsetof(RenderModelDefinition,prt_info_count))))>0;
}

// 136D50(view): the whole step.
template<class Memory,class Frame> void partition(Memory &memory,Frame &frame) {
    using enum Access;
    const auto view=frame.argument(entry_view);
    auto queue=memory.load32(entry_queue,at(view,offsetof(View,object_queue)));
    if(signed_value(widen16(memory.load16(entry_count,at(queue,offsetof(RenderQueue,count)))))<=0)return frame.finish(false);
    Kept k{};
    k.objects=memory.load32(Access::object_headers,globals::object_headers);
    k.tags=memory.load32(Access::tag_instances,globals::tag_instances);
    for(;;) {
        const auto handles=memory.load32(queue_handles,at(queue,offsetof(RenderQueue,handles)));
        const auto handle=memory.load32(queue_handle,element(handles,widen16(k.index),4));
        if(has_prt(memory,k,handle)) {
            // The entry joins the front: unless it is already there, it trades
            // places with the first entry that has no PRT data.
            if(k.index!=k.front)frame.swap(k,queue);
            ++k.front;
        }
        // The view, its queue and the count are read again for every entry.
        const auto view_again=frame.argument(next_view);
        queue=memory.load32(next_queue,at(view_again,offsetof(View,object_queue)));
        const auto count=widen16(memory.load16(next_count,at(queue,offsetof(RenderQueue,count))));
        if(signed_value(++k.index)>=signed_value(count))return frame.finish(true);
    }
}
} // namespace prt_partition
} // namespace halo2::native_prt_partition
