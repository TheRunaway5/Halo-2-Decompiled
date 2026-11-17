#pragma once
// Native TU1.5 object widget rendering, entered at D7230 (model render unit;
// the native render prepare 41C70 calls it for each object it draws).
//
//   widget_render::run      D7230  hands each of an object's widgets to the
//                                  renderer
//
// Widgets are the small simulated parts an object can carry (the widget
// types are 'ant!' antennas, 'clwd' cloth and 'tdtl', per their creators in
// D6E40). They live in the widget data array ([4ECBF0]), chained from the
// object's first widget; each one's type selects a row of the widget type
// table at 4739F8. For each widget whose type renders in the current render
// mode:
//   - when the type has render callbacks, the widget is submitted to the
//     render queue (41480) with the shader its type's shader routine returns
//     for it;
//   - when the type has a direct render routine, it runs with the mode and
//     the widget's data.
// Modes 1 and 2 only render when the caller's request flags allow that mode.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores. Every access
// names its role (the Access enum); the boundary adapter
// (native_tu15_attachment_update.hpp) maps the role to the original
// instruction for the diagnostic cursor. The calls into translated code, the
// stack argument, registers and flags belong to that adapter, reached
// through a Frame object.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width;
//   - address arithmetic is 32-bit and wraps as the original's does;
//   - the values the original keeps in callee-saved registers across the
//     calls (the object, the mode, the widget and its type) are a WidgetWalk.
//     A conforming callee returns them unchanged; the adapter hands back what
//     the callee actually returned, so the walk continues exactly as the
//     original's does even when a callee does not preserve them.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_widgets {
// The object and data_array lookups are shared with the object update systems.
using native_object_update::DataArray;
using native_object_update::at;
using native_object_update::element;
using native_object_update::handle_index;
using native_object_update::none;
using native_object_update::object_of;
using native_object_update::widen16;

// ------------------------------------------------------------- game layouts
// Globals at fixed TU1.5 addresses.
namespace globals {
inline constexpr std::uint32_t widgets=0x4ecbf0u;        // DataArray* of Widget
inline constexpr std::uint32_t widget_types=0x4739f8u;   // WidgetType[]
}

// An object's data: the fields this system reads.
struct WidgetOwner {
    std::uint8_t unused_here_00[0x30];
    float center[3];                 // +0x30: where the render queue places the object's widgets
    std::uint8_t unused_here_3c[0xa0];
    std::uint32_t first_widget;      // +0xDC: Widget handle, or NONE
};
static_assert(offsetof(WidgetOwner,center)==0x30 && offsetof(WidgetOwner,first_widget)==0xdc);
// The address of an object's center (the render submission's last argument).
constexpr std::uint32_t center_of(std::uint32_t object){return at(object,offsetof(WidgetOwner,center));}

// A widget datum (12 bytes).
struct Widget {
    std::uint16_t identifier;        // the datum's salt
    std::int16_t type;               // +2: row of the widget type table
    std::uint32_t data;              // +4: the widget's own datum (handed to its type's routines)
    std::uint32_t next;              // +8: the object's next widget, or NONE
};
static_assert(sizeof(Widget)==12 && offsetof(Widget,type)==2 && offsetof(Widget,data)==4 && offsetof(Widget,next)==8);

// A widget type (0x38 bytes); the rows this system calls through.
struct WidgetType {
    std::uint32_t group;             // the widget tag's group
    std::uint8_t unused_here_04[0x24];
    std::uint32_t render_callbacks[2];  // +0x28, +0x2C: handed to the render queue (none without the first)
    std::uint32_t shader;            // +0x30: routine(data) -> the shader to submit with
    std::uint32_t render;            // +0x34: routine(mode, data) that renders directly, or 0
};
static_assert(sizeof(WidgetType)==0x38 && offsetof(WidgetType,render_callbacks)==0x28 &&
              offsetof(WidgetType,shader)==0x30 && offsetof(WidgetType,render)==0x34);

// The caller's request flags that allow modes 1 and 2.
namespace request_flags {
inline constexpr std::uint32_t mode_one=0x1000u;
inline constexpr std::uint32_t mode_two=0x2000u;
}

// The render queue submission (41480): the mode, -1, a 640.0 limit (its
// meaning is not established), the shader-record filler D7100, the type's
// render callbacks, the widget's data and the object's center; the shader
// comes from the type's shader routine.
namespace submission {
inline constexpr std::uint32_t render_queue=0x41480u;
inline constexpr std::uint32_t unlimited=none;
inline constexpr std::uint32_t limit=0x44200000u;           // 640.0f
inline constexpr std::uint32_t shader_record_filler=0xd7100u;
}

// ------------------------------------------------------------ widget render
// D7230. Frame:
//   request_flags(mode)         the caller's request flags (a stack argument)
//   render(walk, shader, s)     the type's shader routine, then the render
//                               queue submission
//   render_direct(walk, routine, data)
namespace widget_render {
enum class Access {
    object_table,object_rows,object,first_widget,
    widget_table,widget_rows,widget_type,
    render_callback,widget_data,render_context,shader_routine,
    direct_routine,direct_data,next_widget
};
enum class Exit {no_widgets,rendered};
// Kept across the calls.
struct WidgetWalk {
    std::uint32_t object;
    std::uint32_t mode;     // the low word is the render mode; the caller's whole value is passed on
    std::uint32_t type;     // the widget's WidgetType
    std::uint32_t widget;   // the Widget
};
// The submission's values the logic reads (the rest are constants).
struct Submission {
    std::uint32_t data;
    std::uint32_t render_callbacks[2];
};
template<class Frame> bool renders_in_mode(Frame &frame,std::uint32_t mode) {
    const auto render_mode=mode&0xffffu;
    if(render_mode==2u)return (frame.request_flags(render_mode)&request_flags::mode_two)!=0;
    if(render_mode==1u)return (frame.request_flags(render_mode)&request_flags::mode_one)!=0;
    return true;
}
template<class Memory,class Frame> Exit run(Memory &memory,Frame &frame,std::uint32_t object_handle,std::uint32_t mode) {
    using enum Access;
    WidgetWalk walk{object_of(memory,object_handle,object_table,object_rows,object),mode,0,0};
    auto link=memory.load32(first_widget,at(walk.object,offsetof(WidgetOwner,first_widget)));
    if(link==none)return Exit::no_widgets;
    do {
        const auto widgets=memory.load32(widget_table,globals::widgets);
        walk.widget=element(memory.load32(widget_rows,at(widgets,offsetof(DataArray,data))),handle_index(link),sizeof(Widget));
        walk.type=element(globals::widget_types,widen16(memory.load16(widget_type,at(walk.widget,offsetof(Widget,type)))),
                          sizeof(WidgetType));
        if(renders_in_mode(frame,walk.mode)) {
            if(const auto callback=memory.load32(render_callback,at(walk.type,offsetof(WidgetType,render_callbacks[0])))) {
                Submission queued{};
                queued.render_callbacks[0]=callback;
                queued.data=memory.load32(widget_data,at(walk.widget,offsetof(Widget,data)));
                queued.render_callbacks[1]=memory.load32(render_context,at(walk.type,offsetof(WidgetType,render_callbacks[1])));
                frame.render(walk,memory.load32(shader_routine,at(walk.type,offsetof(WidgetType,shader))),queued);
            }
            if(const auto routine=memory.load32(direct_routine,at(walk.type,offsetof(WidgetType,render))))
                frame.render_direct(walk,routine,memory.load32(direct_data,at(walk.widget,offsetof(Widget,data))));
        }
        link=memory.load32(next_widget,at(walk.widget,offsetof(Widget,next)));
    } while(link!=none);
    return Exit::rendered;
}
} // namespace widget_render
} // namespace halo2::native_object_widgets
