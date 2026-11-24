#pragma once
// Native TU1.5 shadow schedule, entered at 4AC20 (shadow frame unit): the
// order and the weights of the frame's shadow records.
//
//   shadow_schedule::schedule   4AC20(cache, output, visible, dt) -> EAX count; RET 10
//
// Unless the cache ([+0x480] records of 0x24 bytes) is empty or the game is
// paused (byte +5 of [51DF50]), the records with both their first flags
// (+0, +1) come to the front (each swapped with the first record), else the
// first active one (+0) after them; the visible count clamps to the
// records. Past the visible count, a record with the stale flag (+2) and
// this frame's stamp ([4C6854] at +0x20) loses its weight (+0x18) and moves
// to the end. 4B150 sorts the records (EAX the front; the cache, 1 and the
// last pushed); a record repeating the previous one's key (+8, not NONE),
// without its hold flag (+3), loses its weight, takes the rate -dt (+0x14)
// and moves to the end; 4B150 sorts again (2). The records past the visible
// count fade: unless held or weightless, their rate falls by dt and their
// weight takes it, clamped to 0 and to their maximum (+0xC), four at a time
// while four remain, from the last; the visible ones grow towards their
// maximum the same way, the rate rising by dt; a record with nothing to do
// has its rate cleared. Every record's five words and weight (as +0xC) go to
// the output (0x14 bytes each).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_shadow_schedule.hpp) maps the role to the original
// instruction, owns the ABI of 4B150 and leaves through the original's
// epilogue. The values the original keeps in callee-saved registers across
// 4B150 are a Kept, refreshed from the registers it returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_shadow_schedule {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t pause=0x51df50u,stamp=0x4c6854u;}
inline constexpr std::uint32_t none=0xffffffffu,record_words=9u,output_words=5u,unrolled=4u,first_sort=1u,second_sort=2u;
namespace cache {inline constexpr std::uint32_t count=0x480u;}
namespace pause {inline constexpr std::uint32_t paused=5u;}
namespace record {inline constexpr std::uint32_t size=0x24u,first_flag=0u,second_flag=1u,stale_flag=2u,hold_flag=3u,key=8u,
    maximum=0xcu,rate=0x14u,weight=0x18u,stamp=0x20u;}
namespace output {inline constexpr std::uint32_t size=0x14u,weight=0xcu;}
// The frame (offsets from the ESP after EBX, EBP, ESI and EDI are pushed):
// the front count, the index (later a record), the visible count as given,
// a record's copy in a swap, and the four arguments.
namespace local {inline constexpr std::uint32_t front=0x10u,index=0x14u,limit=0x18u,swap=0x1cu,cache=0x44u,output=0x48u,
    visible=0x4cu,dt=0x50u;}

namespace shadow_schedule {
enum class Access {
    arg_cache,count_load,pause_load,pause_flag,limit_load,front_clear,limit_store,limit_clamp,cache_load,index_store,first_flag,
    second_flag,front_save_load,front_save_store,front_move_load,front_move_store,front_restore_load,front_restore_store,
    index_reload,index_next,front_store,front_record,active_test,active_save_load,active_save_store,front_record_load,
    active_move_load,active_move_store,front_record_again,active_restore_load,active_restore_store,front_store_again,limit_original,
    count_compare,front_load,cache_again,stale_flag,record_stamp,stamp_test,weight_clear,stale_save_load,stale_save_store,
    stale_move_load,stale_move_store,stale_restore_load,stale_restore_store,cache_sort,front_sort,dt_load,index_sort,key_load,
    previous_key,repeat_flag,repeat_weight,repeat_rate,repeat_save_load,repeat_save_store,index_sort_load,repeat_move_load,
    repeat_move_store,repeat_restore_load,repeat_restore_store,cache_reload,front_reload,index_sort_store,count_weights,
    limit_weights,visible_load,decay_flag_0,decay_weight_0,decay_rate_0,decay_rate_store_0,decay_sum_0,decay_store_0,decay_zero_0,
    decay_again_0,decay_max_0,decay_max_load_0,decay_max_store_0,decay_keep_load_0,decay_keep_store_0,decay_clear_0,decay_flag_1,
    decay_weight_1,decay_rate_1,decay_rate_store_1,decay_sum_1,decay_store_1,decay_zero_1,decay_again_1,decay_max_1,
    decay_max_load_1,decay_max_store_1,decay_keep_load_1,decay_keep_store_1,decay_clear_1,decay_flag_2,decay_weight_2,decay_rate_2,
    decay_rate_store_2,decay_sum_2,decay_store_2,decay_zero_2,decay_again_2,decay_max_2,decay_max_load_2,decay_max_store_2,
    decay_keep_load_2,decay_keep_store_2,decay_clear_2,decay_flag_3,decay_weight_3,decay_rate_3,decay_rate_store_3,decay_sum_3,
    decay_store_3,decay_zero_3,decay_again_3,decay_max_3,decay_max_load_3,decay_max_store_3,decay_keep_load_3,decay_keep_store_3,
    decay_clear_3,decay_flag_rest,decay_weight_rest,decay_rate_rest,decay_rate_store_rest,decay_sum_rest,decay_store_rest,
    decay_zero_rest,decay_again_rest,decay_max_rest,decay_max_load_rest,decay_max_store_rest,decay_keep_load_rest,
    decay_keep_store_rest,decay_clear_rest,visible_reload,grow_max,grow_weight,grow_rate,grow_rate_store,grow_load,grow_rate_again,
    grow_store,grow_zero,grow_again,grow_max_again,grow_max_load,grow_max_store,grow_keep_load,grow_keep_store,grow_clear,
    cache_output,count_output,output_load,copy_load_0,copy_store_0,copy_load_1,copy_store_1,copy_load_2,copy_store_2,copy_load_3,
    copy_store_3,copy_load_4,copy_store_4,weight_load,weight_store,count_reload,count_final};
// The roles of one record's fade (or of the growth, which reads two more).
struct Fade {Access flag,weight,rate,rate_store,sum,store,zero,again,maximum,maximum_load,maximum_store,keep_load,keep_store,clear;};
inline constexpr Fade fades[unrolled+1u]{
    {Access::decay_flag_0,Access::decay_weight_0,Access::decay_rate_0,Access::decay_rate_store_0,Access::decay_sum_0,Access::decay_store_0,Access::decay_zero_0,Access::decay_again_0,Access::decay_max_0,Access::decay_max_load_0,Access::decay_max_store_0,Access::decay_keep_load_0,Access::decay_keep_store_0,Access::decay_clear_0},
    {Access::decay_flag_1,Access::decay_weight_1,Access::decay_rate_1,Access::decay_rate_store_1,Access::decay_sum_1,Access::decay_store_1,Access::decay_zero_1,Access::decay_again_1,Access::decay_max_1,Access::decay_max_load_1,Access::decay_max_store_1,Access::decay_keep_load_1,Access::decay_keep_store_1,Access::decay_clear_1},
    {Access::decay_flag_2,Access::decay_weight_2,Access::decay_rate_2,Access::decay_rate_store_2,Access::decay_sum_2,Access::decay_store_2,Access::decay_zero_2,Access::decay_again_2,Access::decay_max_2,Access::decay_max_load_2,Access::decay_max_store_2,Access::decay_keep_load_2,Access::decay_keep_store_2,Access::decay_clear_2},
    {Access::decay_flag_3,Access::decay_weight_3,Access::decay_rate_3,Access::decay_rate_store_3,Access::decay_sum_3,Access::decay_store_3,Access::decay_zero_3,Access::decay_again_3,Access::decay_max_3,Access::decay_max_load_3,Access::decay_max_store_3,Access::decay_keep_load_3,Access::decay_keep_store_3,Access::decay_clear_3},
    {Access::decay_flag_rest,Access::decay_weight_rest,Access::decay_rate_rest,Access::decay_rate_store_rest,Access::decay_sum_rest,Access::decay_store_rest,Access::decay_zero_rest,Access::decay_again_rest,Access::decay_max_rest,Access::decay_max_load_rest,Access::decay_max_store_rest,Access::decay_keep_load_rest,Access::decay_keep_store_rest,Access::decay_clear_rest}
};
// The original's EBX (the last record), EBP, ESI (the front) and EDI (the cache) across 4B150.
struct Kept {std::uint32_t last,base,front,cache_address;};

// A swap of two records through the frame, three REP MOVSD.
template<class Memory,class Frame> void copy(Memory &memory,Frame &frame,Access load,Access store,std::uint32_t from,std::uint32_t to,
                                            unsigned site) {
    from=frame.copying(site,from,to);
    const auto step=frame.step();
    for(std::uint32_t w=0;w<record_words;++w,from+=step,to+=step)memory.store32(store,to,memory.load32(load,from));
}

// The weight after the rate, clamped to 0 and to the maximum.
template<class Memory,class Frame> void settle(Memory &memory,Frame &frame,std::uint32_t at_weight,std::uint32_t sum,const Fade &r) {
    memory.store32(r.store,at_weight,sum);
    if(frame.negative(sum)) {
        memory.store32(r.zero,at_weight,frame.zeroed_weight());
        return;
    }
    const auto weight=memory.load32(r.again,at_weight);
    if(frame.exceeds(weight,memory.load32(r.maximum,at_weight-record::weight+record::maximum))) {
        memory.store32(r.maximum_store,at_weight,memory.load32(r.maximum_load,at_weight-record::weight+record::maximum));
        return;
    }
    memory.store32(r.keep_store,at_weight,memory.load32(r.keep_load,at_weight));
}
// A record past the visible count fades, unless held or weightless.
template<class Memory,class Frame> void fade(Memory &memory,Frame &frame,std::uint32_t at_weight,std::uint32_t dt,const Fade &r) {
    const auto base=at_weight-record::weight;
    if(!frame.set(memory.load8(r.flag,base+record::hold_flag)) && frame.positive(memory.load32(r.weight,at_weight))) {
        const auto rate=sse::subtract(memory.load32(r.rate,base+record::rate),dt);
        memory.store32(r.rate_store,base+record::rate,rate);
        settle(memory,frame,at_weight,frame.summed(sse::add(rate,memory.load32(r.sum,at_weight))),r);
        return;
    }
    memory.store32(r.clear,base+record::rate,frame.zero());
}

// Frame: see native_tu15_shadow_schedule.hpp.
template<class Memory,class Frame> void schedule(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    const auto cache_address=memory.load32(arg_cache,frame.local(local::cache));
    const auto records=memory.load32(count_load,at(cache_address,cache::count));
    frame.framing();
    const bool any=frame.positive_count(records);
    for(unsigned n=1;n<=4;++n)frame.saving(n);
    bool active=any;
    if(active) {
        const auto pause_address=memory.load32(pause_load,globals::pause);
        if(frame.present(pause_address) && frame.paused(memory.load8(pause_flag,at(pause_address,pause::paused))))active=false;
    }
    if(active) {
        // The front: records with both first flags, or the first active one after them.
        auto visible=memory.load32(limit_load,frame.local(local::visible));
        auto front=frame.no_front();
        const bool clamp=frame.above_int(visible,records);
        memory.store32(front_clear,frame.local(local::front),front);
        kept.last=frame.last_record(records);
        memory.store32(limit_store,frame.local(local::limit),visible);
        if(clamp)memory.store32(limit_clamp,frame.local(local::visible),records);
        const auto base=memory.load32(cache_load,frame.local(local::cache));
        std::uint32_t index=frame.first_index();
        bool moved=frame.unmoved();
        const bool nonempty=frame.non_negative(kept.last);
        memory.store32(index_store,frame.local(local::index),index);
        if(nonempty) {
            auto cursor=frame.cursor_start(base);
            for(;;) {
                if(frame.before(index,kept.last) && frame.flagged(memory.load8(first_flag,at(cursor,record::first_flag))) &&
                   frame.flagged(memory.load8(second_flag,at(cursor,record::second_flag)))) {
                    copy(memory,frame,front_save_load,front_save_store,cursor,frame.local(local::swap),0u);
                    copy(memory,frame,front_move_load,front_move_store,base,cursor,1u);
                    copy(memory,frame,front_restore_load,front_restore_store,frame.local(local::swap),base,2u);
                    index=memory.load32(index_reload,frame.local(local::index));
                    front=frame.moved_front(front);
                    moved=frame.moved();
                }
                index=frame.next_index(index);
                cursor=frame.next_cursor(cursor);
                const bool more=frame.not_before(kept.last,index);
                memory.store32(index_next,frame.local(local::index),index);
                if(!more)break;
            }
            frame.moved_test(moved);
            memory.store32(front_store,frame.local(local::front),front);
        }
        if(!moved && frame.within(front,kept.last)) {
            auto record_address=frame.front_record(front,base);
            memory.store32(Access::front_record,frame.local(local::index),record_address);
            for(std::uint32_t i=front;;) {
                if(frame.flagged(memory.load8(active_test,at(record_address,record::first_flag)))) {
                    const auto found=frame.found_record(i,base);
                    copy(memory,frame,active_save_load,active_save_store,found,frame.local(local::swap),3u);
                    copy(memory,frame,active_move_load,active_move_store,memory.load32(front_record_load,frame.local(local::index)),found,4u);
                    copy(memory,frame,active_restore_load,active_restore_store,frame.local(local::swap),
                        memory.load32(front_record_again,frame.local(local::index)),5u);
                    front=frame.moved_front(front);
                    memory.store32(front_store_again,frame.local(local::front),front);
                    break;
                }
                i=frame.next_active(i);
                record_address=frame.next_active_record(record_address);
                if(!frame.within(i,kept.last))break;
            }
        }

        // Past the visible count, the stale records of this frame lose their weight and move to the end.
        const auto limit=memory.load32(limit_original,frame.local(local::limit));
        const bool more_than_visible=frame.above_int(memory.load32(count_compare,at(base,cache::count)),limit);
        frame.weightless();
        if(more_than_visible) {
            auto index_stale=memory.load32(front_load,frame.local(local::front));
            if(frame.not_before(kept.last,index_stale)) {
                const auto cache_now=memory.load32(cache_again,frame.local(local::cache));
                auto last_address=frame.stale_cursors(cache_now,kept.last,index_stale);
                auto record_address=cache_now+record::size*index_stale;
                for(;;) {
                    bool stale=false;
                    if(frame.set(memory.load8(stale_flag,at(record_address,record::stale_flag)))) {
                        const auto stamp_value=memory.load32(record_stamp,at(record_address,record::stamp));
                        stale=frame.same(stamp_value,memory.load32(stamp_test,globals::stamp));
                    }
                    if(stale) {
                        memory.store32(weight_clear,at(record_address,record::weight),frame.zero());
                        copy(memory,frame,stale_save_load,stale_save_store,record_address,frame.local(local::swap),6u);
                        copy(memory,frame,stale_move_load,stale_move_store,last_address,record_address,7u);
                        kept.last=frame.earlier(kept.last,true);
                        const auto to=last_address;
                        last_address=frame.earlier_record(last_address,1u);
                        index_stale=frame.earlier(index_stale,false);
                        copy(memory,frame,stale_restore_load,stale_restore_store,frame.local(local::swap),to,8u);
                        record_address=frame.earlier_record(record_address,0u);
                    }
                    index_stale=frame.next_stale(index_stale);
                    record_address=frame.next_stale_record(record_address);
                    if(!frame.not_before(kept.last,index_stale))break;
                }
            }
        }

        // The sorts, and the repeated keys between them.
        kept.cache_address=memory.load32(cache_sort,frame.local(local::cache));
        kept.front=memory.load32(front_sort,frame.local(local::front));
        frame.sorted(kept,first_sort);
        const bool sortable=frame.within(kept.front,kept.last);
        const auto dt=memory.load32(dt_load,frame.local(local::dt));
        auto index_repeat=frame.repeat_start(kept.front);
        memory.store32(index_sort,frame.local(local::index),index_repeat);
        if(sortable) {
            auto end_address=frame.repeat_cursors(kept);
            auto key_address=kept.cache_address+record::size*kept.front+record::key;
            for(;;) {
                if(frame.positive_count(index_repeat)) {
                    const auto key=memory.load32(key_load,key_address);
                    if(frame.named(key) && frame.same(key,memory.load32(previous_key,key_address-record::size)) &&
                       !frame.set(memory.load8(repeat_flag,key_address-record::key+record::hold_flag))) {
                        const auto repeated=frame.repeated(key_address-record::key);
                        memory.store32(repeat_weight,key_address-record::key+record::weight,frame.zero());
                        memory.store32(repeat_rate,key_address-record::key+record::rate,frame.negated(dt));
                        copy(memory,frame,repeat_save_load,repeat_save_store,repeated,frame.local(local::swap),9u);
                        frame.repeat_move_registers(end_address,repeated);
                        index_repeat=memory.load32(index_sort_load,frame.local(local::index));
                        copy(memory,frame,repeat_move_load,repeat_move_store,end_address,repeated,10u);
                        copy(memory,frame,repeat_restore_load,repeat_restore_store,frame.local(local::swap),end_address,11u);
                        kept.cache_address=memory.load32(cache_reload,frame.local(local::cache));
                        kept.front=memory.load32(front_reload,frame.local(local::front));
                        kept.last=frame.earlier(kept.last,true);
                        end_address=frame.earlier_record(end_address,2u);
                    }
                }
                index_repeat=frame.next_repeat(index_repeat);
                key_address=frame.next_key(key_address);
                const bool more=!frame.after_int(index_repeat,kept.last);
                memory.store32(index_sort_store,frame.local(local::index),index_repeat);
                if(!more)break;
            }
        }
        frame.sorted(kept,second_sort);

        // The weights: past the visible count they fade, from the last, four at a time while four remain; the visible ones grow.
        const auto total=memory.load32(count_weights,at(kept.cache_address,cache::count));
        std::uint32_t visible_count;
        if(frame.positive_count(total) && !frame.covers(memory.load32(limit_weights,frame.local(local::limit)),total)) {
            visible_count=memory.load32(visible_load,frame.local(local::visible));
            auto last=frame.last_index(total);
            if(frame.four_or_more(last,visible_count)) {
                auto passes=frame.passes(last,visible_count);
                auto at_weight=frame.fade_cursor(kept.cache_address,last);
                last=frame.after_passes(last,passes);
                for(;;) {
                    for(std::uint32_t k=0;k<unrolled;++k)fade(memory,frame,at_weight-record::size*k,dt,fades[k]);
                    at_weight=frame.next_pass(at_weight);
                    passes=frame.counted_pass(passes);
                    if(!passes)break;
                }
            }
            if(frame.not_before(last,visible_count)) {
                auto left=frame.rest(last,visible_count);
                auto at_weight=frame.rest_cursor(kept.cache_address,last);
                for(;;) {
                    fade(memory,frame,at_weight,dt,fades[unrolled]);
                    at_weight=frame.previous_record(at_weight);
                    left=frame.counted_left(left);
                    if(!left)break;
                }
            }
        } else visible_count=memory.load32(visible_reload,frame.local(local::visible));
        if(frame.positive_count(visible_count)) {
            auto at_weight=frame.grow_cursor(kept.cache_address);
            for(auto left=visible_count;;) {
                const auto maximum=memory.load32(grow_max,at_weight-record::weight+record::maximum);
                if(frame.above(maximum,memory.load32(grow_weight,at_weight))) {
                    const auto rate=sse::add(memory.load32(grow_rate,at_weight-record::weight+record::rate),dt);
                    memory.store32(grow_rate_store,at_weight-record::weight+record::rate,rate);
                    const auto weight=memory.load32(grow_load,at_weight);
                    const auto sum=frame.summed(sse::add(weight,memory.load32(grow_rate_again,at_weight-record::weight+record::rate)));
                    settle(memory,frame,at_weight,sum,{grow_max,grow_weight,grow_rate,grow_rate_store,grow_load,grow_store,grow_zero,
                        grow_again,grow_max_again,grow_max_load,grow_max_store,grow_keep_load,grow_keep_store,grow_clear});
                } else memory.store32(grow_clear,at_weight-record::weight+record::rate,frame.zero());
                at_weight=frame.next_record(at_weight);
                left=frame.counted_left(left);
                if(!left)break;
            }
        }
    }

    // The output: each record's five words and weight.
    const auto cache_now=memory.load32(cache_output,frame.local(local::cache));
    const auto count=memory.load32(count_output,at(cache_now,cache::count));
    auto index=frame.output_start();
    if(frame.positive_count(count)) {
        auto out=memory.load32(output_load,frame.local(local::output));
        auto record_address=frame.output_cursor(cache_now);
        for(;;) {
            static constexpr Access loads[]{copy_load_0,copy_load_1,copy_load_2,copy_load_3,copy_load_4};
            static constexpr Access stores[]{copy_store_0,copy_store_1,copy_store_2,copy_store_3,copy_store_4};
            frame.output_registers(record_address,out);
            for(std::uint32_t w=0;w<output_words;++w)memory.store32(stores[w],out+4u*w,memory.load32(loads[w],record_address+4u*w));
            memory.store32(weight_store,at(out,output::weight),memory.load32(weight_load,at(record_address,record::weight)));
            const auto count_now=memory.load32(count_reload,at(cache_now,cache::count));
            index=frame.next_output(index);
            record_address=frame.next_output_record(record_address);
            out=frame.next_output_entry(out);
            if(!frame.before_int(index,count_now))break;
        }
    }
    memory.load32(count_final,at(cache_now,cache::count));
}
} // namespace shadow_schedule
} // namespace halo2::native_shadow_schedule
