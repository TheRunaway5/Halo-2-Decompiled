#pragma once
// Native TU1.5 render-object submission loop: 136230, which runs the PRT
// object partition (136D50) on a view's visible object queue and submits each
// queued object through 1362A0.
//
//   object_pass::run(view, parameter)
//     1. Partition the view's object queue so objects whose render models carry
//        PRT data come first (136D50).
//     2. Clear the per-pass local slot and the retry byte (stored in the low
//        byte of the stack argument slot, as in the original).
//     3. While index < queue->count:
//        - call 1362A0(index, retry_word, &local_slot, -1, parameter) with the
//          view in ESI;
//        - clear the retry byte;
//        - if 1362A0 returns nonzero (different from kept.zero) and the view's
//          kind (+0x2A78) is 1, set the retry byte to 1 and decrement index so
//          the next iteration retries the same object.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access) and the boundary adapter
// (native_tu15_object_pass.hpp) maps the role to the original instruction.
// Registers, flags, stack slots and child calls belong to the adapter.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_pass {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;

// ------------------------------------------------------------- game layouts
struct View {
    std::uint8_t unused_here_00[0x14];
    std::uint32_t object_queue;       // +0x14: RenderQueue*
    std::uint8_t unused_here_18[0x2a60];
    std::uint32_t kind;               // +0x2A78: 1 enables retry on nonzero 1362A0 return
};
static_assert(offsetof(View,object_queue)==0x14 && offsetof(View,kind)==0x2a78);
inline constexpr std::uint32_t retry_view_kind=1u;

struct RenderQueue {
    std::int32_t capacity;
    std::int16_t count;               // +4
};
static_assert(offsetof(RenderQueue,count)==4);

// ------------------------------------------------------------ access roles
enum class Access : unsigned {
    argument,entry_queue,local_clear,entry_count,retry_clear,
    retry_word,retry_reset,view_kind,retry_set,next_queue,next_count
};

// Values the original keeps in callee-saved registers across 136D50 and 1362A0
// (EBX zero/sentinel, EBP parameter, ESI view, EDI index).
struct Kept {
    std::uint32_t zero;       // EBX (0 after XOR EBX,EBX; low byte compared against AL and stored)
    std::uint32_t parameter;  // EBP (caller's stack argument)
    std::uint32_t view;       // ESI (entry EAX)
    std::uint32_t index;      // EDI (current queue entry)
};

namespace object_pass {
// Frame (the adapter):
//   entry_view()                                    view pointer in EAX
//   argument()                                      read caller's stack argument
//   local_slot() / retry_slot()                     frame addresses
//   partition(k)                                    136D50(k.view)
//   submit(k, retry_word, local_slot) -> uint8_t    1362A0(k.index, retry_word, local_slot, -1, k.parameter)
//   retry_step(k)                                   DEC EDI
//   finish()                                        POP EDI/ESI/EBP/EBX/ECX, RET 4
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    Kept k{};
    k.view=frame.entry_view();
    k.parameter=frame.argument();
    frame.partition(k);
    const auto queue=memory.load32(entry_queue,at(k.view,offsetof(View,object_queue)));
    k.zero=0u;
    memory.store32(local_clear,frame.local_slot(),k.zero);
    k.index=0u;
    const auto initial_count=widen16(memory.load16(entry_count,at(queue,offsetof(RenderQueue,count))));
    memory.store8(retry_clear,frame.retry_slot(),k.zero&0xffu);
    if(signed_value(initial_count)<=signed_value(widen16(k.zero)))return frame.finish();
    for(;;) {
        const auto retry_word=memory.load32(Access::retry_word,frame.retry_slot());
        const auto result=frame.submit(k,retry_word,frame.local_slot());
        memory.store8(retry_reset,frame.retry_slot(),k.zero&0xffu);
        if(result!=(k.zero&0xffu)) {
            if(memory.load32(Access::view_kind,at(k.view,offsetof(View,kind)))==retry_view_kind) {
                memory.store8(retry_set,frame.retry_slot(),1u);
                frame.retry_step(k);
            }
        }
        const auto queue_again=memory.load32(next_queue,at(k.view,offsetof(View,object_queue)));
        const auto count=widen16(memory.load16(next_count,at(queue_again,offsetof(RenderQueue,count))));
        if(signed_value(++k.index)>=signed_value(count))return frame.finish();
    }
}
} // namespace object_pass
} // namespace halo2::native_object_pass
