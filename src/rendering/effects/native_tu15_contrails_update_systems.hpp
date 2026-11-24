#pragma once
// Native TU1.5 contrails update, entered at 17EAD0 (contrail geometry
// unit): every contrail advanced by the frame's time.
//
//   contrails_update::update   17EAD0(dt) -> RET 4
//
// The contrails are the 0x48-byte records of [4F7214], walked in index
// order: the first through 16EA60 (ECX the array), the next by the array's
// active bits (+0x48, below its count +0x38). For each, the time since its
// last update is dt less its accumulated time (+0x2C), which clears. A
// contrail on an object (+8) asks BCB70 for its marker (+0x10) and scale
// (+0x14): when that answer differs from its active flag (+2 bit 0) it
// emits one point at scale 0 (17EF70: EAX the contrail, 1 and 1 pushed, RET
// 8); while active it emits as many points as 17EED0 counts for dt (one
// pushed, RET 4) and keeps the flag, otherwise clears it. When time passed
// and either speed of its definition (+0x24, +0x28) is positive: with a
// frame rate (+0x2C, times the scale for +2 bit 5) its frame time (+0x28)
// advances by whole frames (17EE10, EAX the contrail) and keeps the rest,
// otherwise it gains all of it; its first position (+0x1C) falls by the
// first speed and its second (+0x20) rises by the second, each times the
// scale for +3 bit 0 or bit 1. 17F3D0 then updates its points (the handle
// and dt pushed, RET 8); a contrail with no points (+0x38, four) and no
// object is deleted (17F710, EAX the handle; 16E8A0, EDX the array).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); scalar SSE as the original orders it; the
// boundary adapter (native_tu15_contrails_update.hpp) maps the role to the
// original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them, and those it keeps
// in XMM1 to XMM5 across 17EE10, are a Kept, refreshed from the registers
// each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_contrails_update {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t contrails=0x4f7214u,tag_instances=0x4f0414u,one=0x46a0d0u;}
inline constexpr std::uint32_t tag_data=8u,handle_mask=0xffffu,none=0xffffffffu,points=4u;
namespace array {inline constexpr std::uint32_t size=0x24u,count=0x38u,data=0x44u,active=0x48u;}
namespace contrail {inline constexpr std::uint32_t flags=2u,tag=4u,object=8u,marker=0x10u,scale=0x14u,first=0x1cu,second=0x20u,
    frame_time=0x28u,age=0x2cu,point_indices=0x38u,size=0x48u;inline constexpr std::uint32_t active_bit=1u;}
namespace definition {inline constexpr std::uint32_t flags02=2u,flags03=3u,first_speed=0x24u,second_speed=0x28u,frame_rate=0x2cu;
    inline constexpr std::uint32_t scaled_rate_bit=0x20u,scaled_first_bit=1u,scaled_second_bit=2u;}
// The frame (offsets from the ESP after all four registers are pushed):
// the handle, the elapsed time, the index, the saved scale, and dt.
namespace local {inline constexpr std::uint32_t handle=0x10u,elapsed=0x14u,index=0x18u,saved_scale=0x1cu,dt=0x24u,size=0x10u;}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

namespace contrails_update {
enum class Access {contrails_load,element_size,first_data,first_salt,handle_store,handle_next,data_load,dt_load,tag_load,age_load,
    instances_load,definition_load,object_load,index_store,elapsed_store,age_clear,marker_load,flags_load,scale_load,handle_reload,
    scale_save,scale_clear,scale_saved,scale_restore,dt_again,handle_again,flags_set_load,flags_set_store,flags_clear_load,
    flags_clear_store,elapsed_reload,index_reload,handle_reload_again,first_speed,second_speed,frame_rate,rate_flags,rate_scale,
    one_load,frame_time_load,frame_time_rest,frame_time_all,frame_time_store,first_flags,first_speed_again,first_scale,first_load,
    first_store,second_flags,second_speed_again,second_scale,second_load,second_store,dt_points,point_load,contrails_again,
    object_test,count_load,bits_load,active_test,next_size,next_data,next_salt,next_store};
// The original's EBX (the handle; whether the marker answered, in BL), EBP
// (the array, later the definition), ESI (the contrail) and EDI (the index,
// later the scale's address and the handle), and the frame-rate loop's
// XMM1 (the step), XMM2 (the rest), XMM3 (the period), XMM4 (zero) and XMM5
// (the elapsed time) across 17EE10.
struct Kept {std::uint32_t handle,base,contrail_address,index,step,rest,period,zero,elapsed;};

// Frame: see native_tu15_contrails_update.hpp.
template<class Memory,class Frame> void update(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The first contrail.
    frame.framing();
    frame.saving(1u);
    frame.saving(2u);
    kept.base=memory.load32(contrails_load,globals::contrails);
    const auto first=frame.first_index(kept);
    auto handle=frame.no_handle();
    if(frame.found(first)) {
        const auto size=memory.load32(element_size,at(kept.base,array::size));
        const auto data=memory.load32(first_data,at(kept.base,array::data));
        const auto salt=signed_word(memory.load16(first_salt,frame.element_offset(size,first)+data));
        handle=frame.handle_of(salt,first,false);
    }
    kept.handle=frame.walking(handle);
    memory.store32(handle_store,frame.local(local::handle),handle);
    if(handle==none){frame.finished(false);return;}
    frame.saving(3u);
    frame.saving(4u);
    for(bool first_pass=true;;first_pass=false) {
        if(!first_pass)kept.handle=memory.load32(handle_next,frame.local(local::handle));
        // The contrail, its definition and the time since its last update.
        const auto data=memory.load32(data_load,at(kept.base,array::data));
        const auto dt=memory.load32(dt_load,frame.local(local::dt));
        frame.zeroed();
        kept.index=frame.contrail_index(kept.handle);
        const auto contrail_address=data+contrail::size*kept.index;
        const auto tag=memory.load32(tag_load,at(contrail_address,contrail::tag));
        kept.elapsed=sse::subtract(dt,memory.load32(age_load,at(contrail_address,contrail::age)));
        frame.aged(kept.elapsed);
        kept.contrail_address=frame.contrail_address(contrail_address);
        const auto instances=memory.load32(instances_load,globals::tag_instances);
        kept.base=memory.load32(definition_load,frame.tag_offset(tag)+instances+tag_data);
        const auto object=memory.load32(object_load,at(kept.contrail_address,contrail::object));
        const bool attached=frame.attached(object);
        memory.store32(index_store,frame.local(local::index),kept.index);
        memory.store32(elapsed_store,frame.local(local::elapsed),kept.elapsed);
        memory.store32(age_clear,at(kept.contrail_address,contrail::age),0u);
        if(attached) {
            // Its marker: a change of state emits a point at scale 0; while active it emits for dt.
            const auto name=memory.load32(marker_load,at(kept.contrail_address,contrail::marker));
            const bool active=frame.marker(kept,name,object);
            kept.handle=frame.answered(active);
            if(frame.changed(memory.load8(flags_load,at(kept.contrail_address,contrail::flags)))) {
                const auto scale=memory.load32(scale_load,kept.index);
                memory.load32(handle_reload,frame.local(local::handle));
                frame.forced_pushed();
                memory.store32(scale_save,frame.local(local::saved_scale),scale);
                frame.count_pushed();
                memory.store32(scale_clear,kept.index,frame.cleared());
                frame.emit(kept,true);
                memory.store32(scale_restore,kept.index,memory.load32(scale_saved,frame.local(local::saved_scale)));
            }
            if(frame.active_now(kept.handle)) {
                const auto dt_now=memory.load32(dt_again,frame.local(local::dt));
                kept.index=memory.load32(handle_again,frame.local(local::handle));
                const auto flags_now=memory.load8(flags_set_load,at(kept.contrail_address,contrail::flags));
                memory.store8(flags_set_store,at(kept.contrail_address,contrail::flags),frame.flag_set(flags_now));
                frame.emit_for(kept,dt_now);
            } else {
                const auto flags_now=memory.load8(flags_clear_load,at(kept.contrail_address,contrail::flags));
                memory.store8(flags_clear_store,at(kept.contrail_address,contrail::flags),frame.flag_cleared(flags_now));
            }
            frame.zeroed();
            kept.zero=0u;
            kept.elapsed=memory.load32(elapsed_reload,frame.local(local::elapsed));
            kept.index=memory.load32(index_reload,frame.local(local::index));
            kept.handle=memory.load32(handle_reload_again,frame.local(local::handle));
        } else kept.zero=0u;
        // Its frame time and positions, when time passed and it moves.
        if(frame.above(kept.elapsed,kept.zero) &&
           (frame.above(memory.load32(first_speed,at(kept.base,definition::first_speed)),kept.zero) ||
            frame.above(memory.load32(second_speed,at(kept.base,definition::second_speed)),kept.zero))) {
            const auto rate_value=memory.load32(frame_rate,at(kept.base,definition::frame_rate));
            const bool timed=frame.above(rate_value,kept.zero);
            kept.rest=frame.rest_from_elapsed();
            if(timed) {
                auto rate=rate_value;
                if(frame.flagged(memory.load8(rate_flags,at(kept.base,definition::flags02)),definition::scaled_rate_bit)) {
                    rate=sse::multiply(memory.load32(rate_scale,at(kept.contrail_address,contrail::scale)),rate);
                    frame.rate_scaled(rate);
                }
                kept.period=sse::divide(memory.load32(one_load,globals::one),rate);
                frame.period_set(kept.period);
                for(;;) {
                    frame.step_from_period();
                    kept.step=sse::subtract(kept.period,memory.load32(frame_time_load,at(kept.contrail_address,contrail::frame_time)));
                    frame.step_set(kept.step);
                    if(frame.short_of(kept.rest,kept.step)) {
                        const auto frame_time=sse::add(memory.load32(frame_time_rest,at(kept.contrail_address,contrail::frame_time)),kept.rest);
                        frame.frame_time_set(frame_time);
                        memory.store32(frame_time_store,at(kept.contrail_address,contrail::frame_time),frame_time);
                        break;
                    }
                    frame.advance(kept);
                    kept.rest=sse::subtract(kept.rest,kept.step);
                    frame.rest_set(kept.rest);
                    if(!frame.above(kept.rest,kept.zero))break;
                }
            } else {
                const auto frame_time=sse::add(memory.load32(frame_time_all,at(kept.contrail_address,contrail::frame_time)),kept.elapsed);
                frame.frame_time_set(frame_time);
                memory.store32(frame_time_store,at(kept.contrail_address,contrail::frame_time),frame_time);
            }
            const bool first_scaled=frame.flagged(memory.load8(first_flags,at(kept.base,definition::flags03)),definition::scaled_first_bit);
            auto first_speed_value=memory.load32(first_speed_again,at(kept.base,definition::first_speed));
            if(first_scaled) {
                first_speed_value=sse::multiply(memory.load32(first_scale,at(kept.contrail_address,contrail::scale)),first_speed_value);
                frame.speed_scaled(first_speed_value);
            }
            const auto first_now=memory.load32(first_load,at(kept.contrail_address,contrail::first));
            const auto first_drop=sse::multiply(first_speed_value,kept.elapsed);
            const auto first_new=sse::subtract(first_now,first_drop);
            frame.first_moved(first_drop,first_new);
            memory.store32(first_store,at(kept.contrail_address,contrail::first),first_new);
            const bool second_scaled=frame.flagged(memory.load8(second_flags,at(kept.base,definition::flags03)),definition::scaled_second_bit);
            auto second_speed_value=memory.load32(second_speed_again,at(kept.base,definition::second_speed));
            if(second_scaled) {
                second_speed_value=sse::multiply(memory.load32(second_scale,at(kept.contrail_address,contrail::scale)),second_speed_value);
                frame.speed_scaled(second_speed_value);
            }
            const auto second_rise=sse::multiply(second_speed_value,kept.elapsed);
            const auto second_new=sse::add(second_rise,memory.load32(second_load,at(kept.contrail_address,contrail::second)));
            frame.second_moved(second_new);
            memory.store32(second_store,at(kept.contrail_address,contrail::second),second_new);
        }
        // Its points; a contrail with none and no object is deleted.
        frame.points(kept,memory.load32(dt_points,frame.local(local::dt)));
        std::uint32_t slot=frame.first_slot();
        for(;;) {
            const auto point=memory.load32(point_load,kept.contrail_address+4u*frame.slot_widened(slot)+contrail::point_indices);
            if(frame.has_point(point) || !frame.more_slots(slot=frame.next_slot(slot)))break;
        }
        const bool empty=frame.all_slots(slot);
        kept.base=memory.load32(contrails_again,globals::contrails);
        if(empty && !frame.object_kept(memory.load32(object_test,at(kept.contrail_address,contrail::object))))frame.remove(kept);
        // The next contrail: the array's next active index.
        auto index=frame.next_start(kept.handle,kept.index);
        auto found=frame.not_found();
        if(frame.counted(index)) {
            const auto count=memory.load32(count_load,at(kept.base,array::count));
            if(frame.below(index,count)) {
                const auto bits=memory.load32(bits_load,at(kept.base,array::active));
                for(;;) {
                    const auto bit=frame.bit_of(index);
                    if(frame.active(memory.load32(active_test,bits+4u*frame.word_of(index)),bit)){found=frame.found_at(index);break;}
                    index=frame.next_index(index);
                    if(!frame.below(index,count))break;
                }
            }
        }
        auto next=frame.next_none();
        if(frame.found(found)) {
            const auto size=memory.load32(next_size,at(kept.base,array::size));
            const auto data_now=memory.load32(next_data,at(kept.base,array::data));
            const auto offset=frame.element_offset_next(size,found);
            next=frame.handle_of(signed_word(memory.load16(next_salt,offset+data_now)),found,true);
        }
        const bool more=frame.found(next);
        memory.store32(next_store,frame.local(local::handle),next);
        if(!more)break;
    }
    frame.finished(true);
}
} // namespace contrails_update
} // namespace halo2::native_contrails_update
