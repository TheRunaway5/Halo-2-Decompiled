#pragma once
#include "halo2/native_tu15_bitmap_scan_kernel.hpp"
#include <bit>
#include <cstdint>

namespace halo2 {
// Caller-frame fields are lazy storage, rather than cached C++ locals. The
// retained ABI owner supplies their real backing, including ring/frame aliases.
enum class NativeTu15PageFrameSlot {pages,result,policy,chosen,ring_begin,ring_end,current_handle,
    previous,span_tick,span_pages,victim,victim_tick,best_previous,best_tick,best_start,best_pages,next_index,datum_cursor};
enum class NativeTu15PageAllocatePhase {
    begin,request_bytes,owner,page_shift,pages_initial,pages_rounded,priority,result_reset,
    policy_zero,policy_cap,policy_input,policy_read,budget,total_initial,begin_scan,head_initial,
    victim_reset,chosen_reset,ring_begin_reset,ring_end_reset,handle_reset,previous_reset,
    failure_policy,failure_priority,failure_budget,failure_pages,budget_publish,result_failure,finish_early,finish_failure,
    ring_begin_next,ring_end_next,ring_previous,ring_start,ring_tick,ring_pages,ring_end_publish,
    tail_total,tail_tick,tail_pages,row_pool,row_rows,row_start,gap_start,gap_tick,gap_pages,
    row_pages,row_tick,span_pages,callback,span_tick,busy_call,row_tick_live,live_priority,owner_tick,
    victim_current,victim_tick_current,victim_publish,victim_tick_publish,row_next,row_next_pool,
    row_next_pages,row_next_rows,handle_publish,previous_publish,row_next_start,handle_reload,
    busy_ring_end,busy_ring_reset,window_begin,window_end_before,window_span_tick,window_tick,
    window_next_publish,window_tick_publish,live_span_pages,window_pages,window_required,window_pages_publish,
    chosen_read,strategy,rank_epoch,rank_tick,rank_best_tick,rank_best_pages,rank_tick_simple,
    rank_best_tick_simple,rank_best_pages_simple,prefer_call,next_index_reload,choose_previous,choose_tick,
    choose_previous_publish,choose_start,choose_tick_publish,choose_pages,choose_start_publish,
    choose_pages_publish,choose_publish,advance_begin,advance_begin_publish,span_tick_reload,
    window_end_after,window_handle_reload,total_again,chosen_finish,scan_pool,scan_cursor_reset,
    scan_cursor,scan_limit,scan_bitmap,bitmap_word,bitmap_done,scan_stride,scan_rows,scan_salt,
    overlap_start,overlap_required,scan_cursor_publish,occupied_start,occupied_pages,overlap_start_again,
    retire_overlap_call,finished_pool,finished_occupancy,finished_capacity,victim_load,victim_previous,
    victim_rows,victim_previous_link,victim_previous_publish,retire_victim_call,predecessor_load,
    allocate_pool,allocate_call,result_publish,allocated_pool,allocated_rows,result_reload,head_load,
    new_previous,empty_head_reload,empty_tail_publish,empty_next_publish,empty_head_publish,
    head_previous_publish,head_reload,head_next_publish,head_publish,previous_next,tail_load,
    tail_previous_publish,tail_publish,next_previous,new_previous_publish,next_previous_publish,
    link_pool,link_rows,link_next,new_next_publish,previous_next_publish,best_start_load,required_load,
    page_start_publish,page_pages_publish,epoch_load,page_epoch_publish,finish_success
};
struct NativeTu15PageAllocation {
    std::uint32_t bytes{},owner{},page_shift{},page_mask{},pages{},priority{},policy{},budget{},total{},
        result{0xffffffffu},handle{0xffffffffu},span_end{},ring_begin{},ring_end{},next_index{},
        descriptor{},rows{},record{},record_start{},record_pages{},record_tick{},callback{},epoch{},
        span_tick{},span_pages{},next_handle{},window_index{},window_address{},window_tick{},window_pages{},
        strategy{},best_tick{},best_pages{},candidate_age{},best_age{},victim{},victim_tick{},
        selected_previous{},selected_tick{},selected_start{},selected_pages{},predecessor{},
        scan_index{},scan_limit{},bitmap{},bitmap_index{},stride{},salt{},overlap_start{},overlap_end{},
        new_record{},new_handle{},previous_offset{},link_handle{},link_record{},link_rows{},link_value{};
    bool scan_started{},busy{},prefer{};
    NativeTu15BitmapScan bitmap_result{};
};
struct NativeTu15PageBusyResume {std::uint32_t owner{},record{},handle{};bool busy{};};
struct NativeTu15PageWindowResume {std::uint32_t owner{},window{},span_end{};bool prefer{};};
struct NativeTu15PageOverlapResume {std::uint32_t owner{},descriptor{};};
struct NativeTu15PageVictimResume {std::uint32_t owner{},predecessor{};};
struct NativeTu15PageAllocationResume {std::uint32_t owner{},predecessor{},handle{};};

// Complete 13FE50 DATA algorithm. Access owns real fields, lazy caller-frame
// storage and publication boundaries. Services execute actual children and
// return only the semantic state that those children really resumed with.
// This candidate remains unbound until its DATA/ABI gates qualify.
template<class Access,class Services>std::uint32_t native_tu15_allocate_pages(Access &memory,
    NativeTu15PageAllocation &s,Services &services){
    using P=NativeTu15PageAllocatePhase;using F=NativeTu15PageFrameSlot;
    const auto signed_word=[](std::uint32_t value){return std::bit_cast<std::int32_t>(value);};
    const auto next_ring=[&](std::uint32_t index){return std::uint32_t(signed_word(index+1u)%256);};
    const auto field=[&](P phase,std::uint32_t address){return memory.read(phase,s,address,4u);};
    const auto put=[&](P phase,std::uint32_t address,std::uint32_t value){memory.write(phase,s,address,value,4u);};
    const auto frame=[&](P phase,F slot){return memory.frame_read(phase,s,slot);};
    const auto save=[&](P phase,F slot,std::uint32_t value){memory.frame_write(phase,s,slot,value);};
    memory.prepare(P::begin,s);
    s.bytes=memory.argument(P::request_bytes,s);s.owner=memory.argument(P::owner,s);
    s.page_shift=field(P::page_shift,s.owner+0x34u);s.page_mask=(1u<<(s.page_shift&31u))-1u;
    s.pages=std::uint32_t(signed_word(s.bytes)>>(s.page_shift&31u));save(P::pages_initial,F::pages,s.pages);
    if(s.bytes&s.page_mask){++s.pages;save(P::pages_rounded,F::pages,s.pages);}
    s.priority=memory.argument(P::priority,s);s.result=0xffffffffu;save(P::result_reset,F::result,s.result);
    if(signed_word(s.priority)<0)save(P::policy_zero,F::policy,0u);
    else{save(P::policy_cap,F::policy,8u);if(signed_word(s.priority)<=8)save(P::policy_input,F::policy,s.priority);}
    s.policy=frame(P::policy_read,F::policy);s.budget=field(P::budget,s.owner+s.policy*4u+0x44u);
    if(signed_word(s.pages)>=signed_word(s.budget)&&signed_word(s.policy)>0){memory.complete(P::finish_early,s);return s.result;}
    s.total=field(P::total_initial,s.owner+0x30u);memory.prepare(P::begin_scan,s);s.scan_started=true;
    s.handle=field(P::head_initial,s.owner+0x3cu);save(P::victim_reset,F::victim,0xffffffffu);
    s.ring_begin=s.ring_end=s.span_end=0u;save(P::chosen_reset,F::chosen,0u);
    save(P::ring_begin_reset,F::ring_begin,0u);save(P::ring_end_reset,F::ring_end,0u);
    save(P::handle_reset,F::current_handle,s.handle);save(P::previous_reset,F::previous,0xffffffffu);
    const auto failed=[&](){
        s.policy=frame(P::failure_policy,F::policy);s.priority=memory.argument(P::failure_priority,s);
        if(s.priority==s.policy){s.budget=field(P::failure_budget,s.owner+s.policy*4u+0x44u);s.pages=frame(P::failure_pages,F::pages);
            if(signed_word(s.budget)>signed_word(s.pages))s.budget=s.pages;put(P::budget_publish,s.owner+s.policy*4u+0x44u,s.budget);}
        s.result=frame(P::result_failure,F::result);memory.complete(P::finish_failure,s);return s.result;
    };
    if(signed_word(s.total)<=0)return failed();
    for(;;){
        s.next_index=next_ring(s.ring_end);
        if(s.next_index!=s.ring_begin){
            s.predecessor=frame(P::ring_previous,F::previous);s.window_address=memory.window_address(s,s.ring_end);
            put(P::ring_previous,s.window_address,s.predecessor);put(P::ring_start,s.window_address+8u,s.span_end);
            put(P::ring_tick,s.window_address+4u,0u);put(P::ring_pages,s.window_address+12u,0u);
            save(P::ring_end_publish,F::ring_end,s.next_index);
        }
        bool process_windows=true;
        if(s.handle==0xffffffffu){
            s.total=field(P::tail_total,s.owner+0x30u);s.span_pages=s.total-s.span_end;
            save(P::tail_tick,F::span_tick,0u);save(P::tail_pages,F::span_pages,s.span_pages);s.span_end=s.total;
        }else{
            s.descriptor=field(P::row_pool,s.owner+0x64u);s.rows=field(P::row_rows,s.descriptor+0x44u);
            s.record=s.rows+(s.handle&0xffffu)*24u;s.record_start=field(P::row_start,s.record+8u);
            if(s.span_end!=s.record_start){
                s.record_start=field(P::gap_start,s.record+8u);s.span_pages=s.record_start-s.span_end;
                save(P::gap_tick,F::span_tick,0u);save(P::gap_pages,F::span_pages,s.span_pages);s.span_end=s.record_start;
            }else{
                s.record_pages=field(P::row_pages,s.record+4u);s.record_tick=field(P::row_tick,s.record+0x14u);
                save(P::span_pages,F::span_pages,s.record_pages);s.callback=field(P::callback,s.owner+0x24u);
                save(P::span_tick,F::span_tick,s.record_tick);s.busy=false;
                if(s.callback){memory.prepare(P::busy_call,s);const auto resumed=services.busy(s.callback,s.handle,s);
                    s.owner=resumed.owner;s.record=resumed.record;s.handle=resumed.handle;s.busy=resumed.busy;}
                s.record_tick=field(P::row_tick_live,s.record+0x14u);s.priority=memory.argument(P::live_priority,s);
                s.epoch=field(P::owner_tick,s.owner+0x38u);
                if(s.priority+s.record_tick>=s.epoch)s.busy=true;
                else if(!s.busy){s.victim=frame(P::victim_current,F::victim);
                    if(s.victim==0xffffffffu||s.record_tick<frame(P::victim_tick_current,F::victim_tick)){
                        save(P::victim_publish,F::victim,s.handle);save(P::victim_tick_publish,F::victim_tick,s.record_tick);}}
                s.next_handle=field(P::row_next,s.record+12u);s.descriptor=field(P::row_next_pool,s.owner+0x64u);
                s.record_pages=field(P::row_next_pages,s.record+4u);s.rows=field(P::row_next_rows,s.descriptor+0x44u);
                save(P::handle_publish,F::current_handle,s.next_handle);save(P::previous_publish,F::previous,s.handle);
                s.record_start=field(P::row_next_start,s.record+8u);s.span_end=s.record_pages+s.record_start;
                s.handle=frame(P::handle_reload,F::current_handle);
                if(s.busy){s.ring_begin=frame(P::busy_ring_end,F::ring_end);save(P::busy_ring_reset,F::ring_begin,s.ring_begin);process_windows=false;}
                else s.ring_begin=frame(P::window_begin,F::ring_begin);
            }
        }
        if(process_windows){
            s.window_index=s.ring_begin;
            if(s.window_index!=frame(P::window_end_before,F::ring_end)){
                s.span_tick=frame(P::window_span_tick,F::span_tick);
                do{
                    s.window_address=memory.window_address(s,s.window_index);s.next_index=next_ring(s.window_index);
                    s.window_tick=field(P::window_tick,s.window_address+4u);save(P::window_next_publish,F::next_index,s.next_index);
                    if(s.span_tick>s.window_tick)put(P::window_tick_publish,s.window_address+4u,s.span_tick);
                    s.span_pages=frame(P::live_span_pages,F::span_pages);s.window_pages=field(P::window_pages,s.window_address+12u)+s.span_pages;
                    s.pages=frame(P::window_required,F::pages);put(P::window_pages_publish,s.window_address+12u,s.window_pages);
                    if(signed_word(s.window_pages)>=signed_word(s.pages)){
                        const auto chosen=frame(P::chosen_read,F::chosen);s.prefer=true;
                        if(chosen){
                            s.strategy=field(P::strategy,s.owner+0x2cu);
                            if(s.strategy==0u){s.window_tick=field(P::rank_tick_simple,s.window_address+4u);s.best_tick=frame(P::rank_best_tick_simple,F::best_tick);
                                s.prefer=s.window_tick<s.best_tick||(s.window_tick==s.best_tick&&signed_word(s.window_pages)<signed_word(frame(P::rank_best_pages_simple,F::best_pages)));}
                            else if(s.strategy==1u){memory.prepare(P::prefer_call,s);const auto resumed=services.prefer(s.owner,s.window_address,memory.best_window_address(s),s);
                                s.owner=resumed.owner;s.window_address=resumed.window;s.span_end=resumed.span_end;s.prefer=resumed.prefer;}
                            else{s.epoch=field(P::rank_epoch,s.owner+0x38u);s.window_tick=field(P::rank_tick,s.window_address+4u);s.candidate_age=s.epoch-s.window_tick;
                                s.best_tick=frame(P::rank_best_tick,F::best_tick);s.best_age=s.epoch-s.best_tick;s.best_pages=frame(P::rank_best_pages,F::best_pages);
                                s.prefer=signed_word(s.best_pages*s.candidate_age)>signed_word(s.window_pages*s.best_age);}
                            s.next_index=frame(P::next_index_reload,F::next_index);
                        }
                        if(s.prefer){
                            s.selected_previous=field(P::choose_previous,s.window_address);s.selected_tick=field(P::choose_tick,s.window_address+4u);
                            save(P::choose_previous_publish,F::best_previous,s.selected_previous);s.selected_start=field(P::choose_start,s.window_address+8u);
                            save(P::choose_tick_publish,F::best_tick,s.selected_tick);s.selected_pages=field(P::choose_pages,s.window_address+12u);
                            save(P::choose_start_publish,F::best_start,s.selected_start);save(P::choose_pages_publish,F::best_pages,s.selected_pages);save(P::choose_publish,F::chosen,1u);
                        }
                        s.ring_begin=next_ring(frame(P::advance_begin,F::ring_begin));save(P::advance_begin_publish,F::ring_begin,s.ring_begin);
                        s.span_tick=frame(P::span_tick_reload,F::span_tick);
                    }
                    s.window_index=s.next_index;
                }while(s.window_index!=frame(P::window_end_after,F::ring_end));
                s.handle=frame(P::window_handle_reload,F::current_handle);
            }
        }
        s.total=field(P::total_again,s.owner+0x30u);if(signed_word(s.span_end)>=signed_word(s.total))break;
        s.ring_begin=frame(P::ring_begin_next,F::ring_begin);s.ring_end=frame(P::ring_end_next,F::ring_end);
    }
    if(!frame(P::chosen_finish,F::chosen))return failed();
    s.descriptor=field(P::scan_pool,s.owner+0x64u);save(P::scan_cursor_reset,F::datum_cursor,0xffffffffu);
    for(;;){
        s.scan_index=frame(P::scan_cursor,F::datum_cursor)+1u;if(signed_word(s.scan_index)<0)break;
        s.scan_limit=field(P::scan_limit,s.descriptor+0x38u);if(signed_word(s.scan_index)>=signed_word(s.scan_limit))break;
        s.bitmap=field(P::scan_bitmap,s.descriptor+0x48u);s.bitmap_index=s.scan_index;
        struct BitmapMemory {
            Access &memory;NativeTu15PageAllocation &state;
            void read(std::uint32_t address,std::span<std::byte>out)const{
                const auto value=memory.read(P::bitmap_word,state,address,4u);
                for(unsigned n=0;n<4;++n)out[n]=std::byte(value>>(8u*n));++state.bitmap_index;
            }
        };
        s.bitmap_result=native_tu15_scan_bitmap_slots(BitmapMemory{memory,s},s.bitmap,s.scan_index,s.scan_limit);
        memory.prepare(P::bitmap_done,s);s.scan_index=s.bitmap_result.index;if(!s.bitmap_result.found||s.scan_index==0xffffffffu)break;
        s.stride=field(P::scan_stride,s.descriptor+0x24u);s.rows=field(P::scan_rows,s.descriptor+0x44u);s.record=s.rows+s.stride*s.scan_index;
        s.salt=memory.read(P::scan_salt,s,s.record,2u);s.overlap_start=frame(P::overlap_start,F::best_start);s.pages=frame(P::overlap_required,F::pages);
        s.handle=(std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(s.salt))))<<16u)|s.scan_index;
        save(P::scan_cursor_publish,F::datum_cursor,s.scan_index);s.record_start=field(P::occupied_start,s.record+8u);s.overlap_end=s.overlap_start+s.pages;
        if(signed_word(s.record_start)>=signed_word(s.overlap_end))continue;
        s.record_pages=field(P::occupied_pages,s.record+4u);s.record_pages+=s.record_start;
        if(signed_word(s.record_pages)<=signed_word(s.overlap_start))continue;
        memory.prepare(P::retire_overlap_call,s);const auto resumed=services.retire_overlap(s.owner,s.handle,s);s.owner=resumed.owner;s.descriptor=resumed.descriptor;
    }
    s.descriptor=field(P::finished_pool,s.owner+0x64u);s.pages=field(P::finished_occupancy,s.descriptor+0x3cu);s.total=field(P::finished_capacity,s.descriptor+0x20u);
    bool retired_victim=false;
    if(s.pages==s.total){
        s.victim=frame(P::victim_load,F::victim);
        if(s.victim!=0xffffffffu){
            s.predecessor=frame(P::victim_previous,F::best_previous);
            if(s.predecessor==s.victim){s.rows=field(P::victim_rows,s.descriptor+0x44u);s.predecessor=field(P::victim_previous_link,s.rows+(s.victim&0xffffu)*24u+0x10u);
                save(P::victim_previous_publish,F::best_previous,s.predecessor);}
            memory.prepare(P::retire_victim_call,s);const auto resumed=services.retire_victim(s.owner,s.victim,s);s.owner=resumed.owner;s.predecessor=resumed.predecessor;retired_victim=true;
        }
    }
    if(!retired_victim)s.predecessor=frame(P::predecessor_load,F::best_previous);
    s.descriptor=field(P::allocate_pool,s.owner+0x64u);memory.prepare(P::allocate_call,s);const auto allocation=services.allocate(s.descriptor,s);
    s.owner=allocation.owner;s.predecessor=allocation.predecessor;s.result=allocation.handle;save(P::result_publish,F::result,s.result);
    if(s.result==0xffffffffu)return failed();
    s.descriptor=field(P::allocated_pool,s.owner+0x64u);s.rows=field(P::allocated_rows,s.descriptor+0x44u);s.new_handle=frame(P::result_reload,F::result);
    s.new_record=s.rows+(s.new_handle&0xffffu)*24u;
    if(s.predecessor==0xffffffffu){
        s.link_handle=field(P::head_load,s.owner+0x3cu);put(P::new_previous,s.new_record+0x10u,0xffffffffu);
        if(s.link_handle==0xffffffffu){s.link_handle=field(P::empty_head_reload,s.owner+0x3cu);put(P::empty_tail_publish,s.owner+0x40u,s.new_handle);
            put(P::empty_next_publish,s.new_record+12u,s.link_handle);put(P::empty_head_publish,s.owner+0x3cu,s.new_handle);}
        else{put(P::head_previous_publish,s.rows+(s.link_handle&0xffffu)*24u+0x10u,s.new_handle);s.link_handle=field(P::head_reload,s.owner+0x3cu);
            put(P::head_next_publish,s.new_record+12u,s.link_handle);put(P::head_publish,s.owner+0x3cu,s.new_handle);}
    }else{
        s.previous_offset=(s.predecessor&0xffffu)*24u;s.link_handle=field(P::previous_next,s.rows+s.previous_offset+12u);
        if(s.link_handle==0xffffffffu){s.link_value=field(P::tail_load,s.owner+0x40u);put(P::tail_previous_publish,s.new_record+0x10u,s.link_value);put(P::tail_publish,s.owner+0x40u,s.new_handle);}
        else{s.link_record=s.rows+(s.link_handle&0xffffu)*24u;s.link_value=field(P::next_previous,s.link_record+0x10u);
            put(P::new_previous_publish,s.new_record+0x10u,s.link_value);put(P::next_previous_publish,s.link_record+0x10u,s.new_handle);}
        s.descriptor=field(P::link_pool,s.owner+0x64u);s.link_rows=field(P::link_rows,s.descriptor+0x44u);s.link_value=field(P::link_next,s.link_rows+s.previous_offset+12u);
        put(P::new_next_publish,s.new_record+12u,s.link_value);put(P::previous_next_publish,s.link_rows+s.previous_offset+12u,s.new_handle);
    }
    s.selected_start=frame(P::best_start_load,F::best_start);s.pages=frame(P::required_load,F::pages);
    put(P::page_start_publish,s.new_record+8u,s.selected_start);put(P::page_pages_publish,s.new_record+4u,s.pages);
    s.epoch=field(P::epoch_load,s.owner+0x38u);put(P::page_epoch_publish,s.new_record+0x14u,s.epoch);
    s.result=s.new_handle;memory.complete(P::finish_success,s);return s.result;
}
} // namespace halo2
