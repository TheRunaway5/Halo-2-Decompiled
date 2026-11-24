#pragma once
// Native TU1.5 child agents, entered at 2F20F0 (map world support unit):
// a list shape's children each collided with another body through the
// collision dispatcher's agent for the pair of shape types.
//
//   child_agents::collide   2F20F0(body, other body, dispatcher, fourth)
//
// Under a C++ exception frame (fs:[0], handler 37E516, state +0xAC), the
// body's motion (its six rows) is copied to the stack and a wrapper shape is
// built there (2E8FC0, through the dispatch). A stack body names the
// wrapper, a key, the copied motion and the body as its parent; the other
// body's shape answers its type (vtable +0x14). Each child of the body's
// list (count +0xC, 16-byte children from +0x10: an offset and the shape
// +0xC) is placed (the motion's translation plus the offset turned by the
// copied rotation: each column scaled by its component, the x term plus the
// y term, plus the z term), wrapped (the wrapper's child +0xC; the key its
// index), and the dispatcher's agent (+0x218C, 32 by 32 by type: the
// wrapper's type from 2E8CF0, through the dispatch, then the other's) takes
// the stack body, the other body, the dispatcher and the fourth argument.
// The wrapper is then destroyed: its base vtable 45EDDC, and when it owns
// its storage (bit 15 of +4) the bit cleared and 2E12A0 (through the
// dispatch) frees it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores (rows as the
// original's 16-byte moves); every access names its role (Access); the
// boundary adapter (native_tu15_child_agents.hpp) maps the role to the
// original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them are a Kept, refreshed
// from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include "physics/runtime/native_tu15_packed_sse.hpp"
#include <cstdint>

namespace halo2::native_child_agents {
using native_object_update::at;
namespace sse=native_packed_sse;
inline constexpr std::uint32_t handler=0x37e516u,wrapper_vtable=0x45eddcu,no_state=0xffffffffu;
namespace body {inline constexpr std::uint32_t shape=0u,key=4u,motion=8u,parent=0xcu;}
namespace motion {inline constexpr std::uint32_t rows=6u,row=0x10u,first=0x20u,second=0x30u,third=0x40u,translation=0x50u;}
namespace list {inline constexpr std::uint32_t count=0xcu,children=0x10u;}
namespace child {inline constexpr std::uint32_t offset=0u,shape=0xcu,size=0x10u;}
namespace shape {inline constexpr std::uint32_t vtable=0u,type=0x14u;}
namespace dispatcher {inline constexpr std::uint32_t agents=0x218cu;inline constexpr unsigned type_bits=5u;}
namespace wrapper {inline constexpr std::uint32_t vtable=0u,flags=4u,child=0xcu;inline constexpr std::uint32_t owned=0x80u;}
// The frame below the saved registers: the wrapper, the other's type, the
// list, the stack body, the copied motion, the exception registration's
// link and the state.
namespace local {inline constexpr std::uint32_t wrapper=0x18u,other_type=0x28u,list=0x2cu,body=0x30u,motion=0x40u,link=0xa4u,
    state=0xacu,size=0x98u;}
namespace argument {inline constexpr unsigned body=0u,other=1u,dispatcher=2u,fourth=3u;}

// A child placed in the body's frame, with the original's partial terms.
struct Placed {__m128 y_term,z_term,turned,translation;};
inline Placed place(__m128 first,__m128 second,__m128 third,__m128 translation,__m128 offset) {
    Placed p{};
    const auto x_term=sse::multiply(first,sse::broadcast<0>(offset));
    p.y_term=sse::multiply(second,sse::broadcast<1>(offset));
    p.z_term=sse::multiply(third,sse::broadcast<2>(offset));
    p.turned=sse::add(sse::add(x_term,p.y_term),p.z_term);
    p.translation=sse::add(translation,p.turned);
    return p;
}

namespace child_agents {
enum class Access {link_load,link_store,body_argument,motion_load,motion_row,copy_store,list_load,list_store,other_argument,other_shape,
    parent_store,motion_store,other_vtable,state_store,type_slot,other_type_store,count_load,dispatcher_argument,child_offset,
    rotation_load,body_again,motion_again,translation_load,translation_store,child_shape,wrapper_child_store,shape_store,key_store,
    other_type_load,agents_load,fourth_argument,agent_load,other_again,list_again,count_again,vtable_store,wrapper_flags,state_done,
    owned_load,owned_store,link_again,link_restore};
enum class Outcome {empty,collided};
// The original's EBX (the dispatcher), ESI (the body, then the child's
// index) and EDI (the list, then the child) across the children.
struct Kept {std::uint32_t dispatcher,body,index,list,child;};
// Frame: framing(); exception_list() fs:[0]; registration(link); saving(n); argument(n) EBP-relative; local(offset) below the saved
// registers; wrapper_pushed(); constructed(kept) [2E8FC0]; motion_address(); typed(slot, kept) [the other's type]; counted(count);
// children(list); translated(motion); placed(placed); wrapper_address(); wrapper_type(kept) [2E8CF0]; agent_slot(type, other);
// agent(target, fourth, other, kept); advanced(kept, count); flag_tested(flags); anded(v, mask); destroyed(kept) [2E12A0];
// restoring(n).
template<class Memory,class Frame> Outcome collide(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The exception frame.
    frame.framing();
    const auto link=memory.load32(link_load,frame.exception_list());
    memory.store32(link_store,frame.exception_list(),frame.registration(link));
    frame.saving(0u);
    // The body's motion copied to the stack; the wrapper built.
    kept.body=memory.load32(body_argument,frame.argument(argument::body));
    const auto motion_address=memory.load32(motion_load,at(kept.body,body::motion));
    for(std::uint32_t row=0;row<4u;++row) {
        const auto value=memory.load128(motion_row,motion_address+motion::row*row);
        memory.store128(copy_store,frame.local(local::motion+motion::row*row),value);
    }
    const auto fifth_row=memory.load128(motion_row,motion_address+motion::row*4u);
    frame.saving(1u);
    kept.list=memory.load32(list_load,at(kept.body,body::shape));
    memory.store128(copy_store,frame.local(local::motion+motion::row*4u),fifth_row);
    const auto sixth_row=memory.load128(motion_row,motion_address+motion::row*5u);
    frame.wrapper_pushed();
    memory.store32(list_store,frame.local(local::list),kept.list);
    memory.store128(copy_store,frame.local(local::motion+motion::row*5u),sixth_row);
    frame.constructed(kept);
    // The stack body; the other's type.
    const auto other_body=memory.load32(other_argument,frame.argument(argument::other));
    const auto other_shape_address=memory.load32(other_shape,at(other_body,body::shape));
    const auto copied_motion=frame.motion_address();
    memory.store32(parent_store,frame.local(local::body+body::parent),kept.body);
    memory.store32(motion_store,frame.local(local::body+body::motion),copied_motion);
    const auto table=memory.load32(other_vtable,at(other_shape_address,shape::vtable));
    memory.store32(state_store,frame.local(local::state),0u);
    const auto type_target=memory.load32(type_slot,at(table,shape::type));
    const auto other_type=frame.typed(type_target,kept);
    memory.store32(other_type_store,frame.local(local::other_type),other_type);
    const auto count=memory.load32(count_load,at(kept.list,list::count));
    frame.counted(count);
    kept.index=0u;
    bool collided=false;
    if(std::int32_t(count)>0) {
        collided=true;
        kept.dispatcher=memory.load32(dispatcher_argument,frame.argument(argument::dispatcher));
        kept.child=frame.children(kept.list);
        for(;;) {
            // The child placed in the body's frame.
            const auto offset=memory.load128(child_offset,at(kept.child,child::offset));
            const auto first=memory.load128(rotation_load,frame.local(local::motion+motion::first));
            const auto body_now=memory.load32(body_again,frame.argument(argument::body));
            const auto second=memory.load128(rotation_load,frame.local(local::motion+motion::second));
            const auto motion_now=memory.load32(motion_again,at(body_now,body::motion));
            const auto third=memory.load128(rotation_load,frame.local(local::motion+motion::third));
            const auto translation=memory.load128(translation_load,motion_now+motion::translation);
            frame.translated(motion_now);
            const auto placed=place(first,second,third,translation,offset);
            frame.placed(placed);
            memory.store128(translation_store,frame.local(local::motion+motion::translation),placed.translation);
            // The child wrapped and keyed.
            const auto child_shape_address=memory.load32(child_shape,at(kept.child,child::shape));
            memory.store32(wrapper_child_store,frame.local(local::wrapper+wrapper::child),child_shape_address);
            const auto wrapper_address=frame.wrapper_address();
            memory.store32(shape_store,frame.local(local::body+body::shape),wrapper_address);
            memory.store32(key_store,frame.local(local::body+body::key),kept.index);
            const auto type=frame.wrapper_type(kept);
            // The dispatcher's agent for the pair.
            const auto other_type_now=memory.load32(other_type_load,frame.local(local::other_type));
            const auto agents=memory.load32(agents_load,kept.dispatcher);
            const auto slot=frame.agent_slot(type,other_type_now);
            const auto fourth=memory.load32(fourth_argument,frame.argument(argument::fourth));
            const auto agent=memory.load32(agent_load,agents+4u*slot+dispatcher::agents);
            const auto other_now=memory.load32(other_again,frame.argument(argument::other));
            frame.agent(agent,fourth,other_now,kept);
            const auto list_now=memory.load32(list_again,frame.local(local::list));
            const auto count_now=memory.load32(count_again,at(list_now,list::count));
            frame.advanced(kept,count_now);
            if(!(std::int32_t(kept.index)<std::int32_t(count_now)))break;
        }
    }
    // The wrapper destroyed; the exception frame unlinked.
    memory.store32(vtable_store,frame.local(local::wrapper+wrapper::vtable),wrapper_vtable);
    const auto flags=memory.load32(wrapper_flags,frame.local(local::wrapper+wrapper::flags));
    frame.flag_tested(flags);
    memory.store32(state_done,frame.local(local::state),1u);
    if((flags>>8)&wrapper::owned) {
        const auto owned=memory.load8(owned_load,frame.local(local::wrapper+wrapper::flags+1u));
        memory.store8(owned_store,frame.local(local::wrapper+wrapper::flags+1u),frame.anded(owned,wrapper::owned-1u));
        frame.destroyed(kept);
    }
    const auto link_now=memory.load32(link_again,frame.local(local::link));
    frame.restoring(0u);
    memory.store32(link_restore,frame.exception_list(),link_now);
    frame.restoring(1u);
    return collided?Outcome::collided:Outcome::empty;
}
} // namespace child_agents
} // namespace halo2::native_child_agents
