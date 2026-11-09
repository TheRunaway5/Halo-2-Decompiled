#pragma once
#include "halo2/native_tu15_page_allocate_kernel.hpp"
#include "physics/runtime/native_tu15_stack_probe_abi.hpp"
#include "physics/runtime/native_tu15_page_retire_abi.hpp"
#include "physics/runtime/native_tu15_resource_busy_abi.hpp"
#include "gameplay/weapons/native_tu15_effect_datum_allocate.hpp"

// Private candidate: no production body or owner-dispatch binding. The typed
// DATA owner chooses all paths; this adapter publishes its retained ABI at the
// genuine memory/call boundaries. Physical and exceptional proof is separate.
namespace halo2::native_page_allocate_detail {
using P=NativeTu15PageAllocatePhase;using F=NativeTu15PageFrameSlot;using D=NativeTu15PageAllocation;
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_PAGE_ALLOCATE");return p&&*p;}();return enabled;}
struct Activity {std::uint64_t calls{},native_probes{},native_preferences{},native_retirements{},native_allocations{},native_busy{};std::map<std::uint32_t,std::uint64_t> residuals;};
inline Activity &activity(){thread_local Activity value;return value;}
template<unsigned Count,class State>std::uint32_t shift_immediate(State &s,std::uint32_t value){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);unsigned overflow{};
    asm volatile("addb $127,%%al;sahf;shll %3,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(overflow),"+r"(value):"I"(Count):"cc");
    s.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
#else
    return s.flags.shift(value,Count,4);
#endif
}
template<class State,class Before=native_page_window_detail::Noop>struct Access {
    State &s;Before before{};
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    void push(std::uint32_t pc,std::uint32_t value){
        at(pc);const auto target=s.gp[4]-4u;SourcePhysicsContactRecordStore event{pc,target,{},std::vector<std::byte>(4)};
        if(s.observe){event.before.resize(4);s.memory.read(target,event.before);}
        for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(8u*n));
        s.memory.write(target,event.written);s.gp[4]=target;if(s.observe)s.observe(event);
    }
    static std::uint32_t offset(F slot){switch(slot){
        case F::pages:return 0x1cu;case F::result:return 0x20u;case F::policy:return 0x2cu;case F::chosen:return 0x13u;
        case F::ring_begin:return 0x14u;case F::ring_end:return 0x18u;case F::current_handle:return 0x28u;
        case F::previous:return 0x48u;case F::span_tick:return 0x24u;case F::span_pages:return 0x30u;
        case F::victim:return 0x34u;case F::victim_tick:return 0x50u;case F::best_previous:return 0x38u;
        case F::best_tick:return 0x3cu;case F::best_start:return 0x40u;case F::best_pages:return 0x44u;
        case F::next_index:return 0x4cu;case F::datum_cursor:return 0x60u;
    }throw std::runtime_error("Unknown page allocation frame field");}
    std::uint32_t frame_read(P p,const D &d,F f){return read(p,d,s.gp[4]+offset(f)-(d.scan_started?0u:4u),f==F::chosen?1u:4u);}
    void frame_write(P p,const D &d,F f,std::uint32_t value){write(p,d,s.gp[4]+offset(f)-(d.scan_started?0u:4u),value,f==F::chosen?1u:4u);}
    std::uint32_t window_address(const D&,std::uint32_t index)const{return s.gp[4]+0x64u+index*16u;}
    std::uint32_t best_window_address(const D&)const{return s.gp[4]+0x38u;}
    void ring(unsigned target,std::uint32_t value){auto &g=s.gp;g[target]=s.flags.logic(value&0x800000ffu,4);if(g[target]&0x80000000u){g[target]=s.flags.increment(g[target],4,true);g[target]=s.flags.logic(g[target]|0xffffff00u,4);g[target]=s.flags.increment(g[target],4,false);}}
    std::uint32_t argument(P p,const D&){auto &g=s.gp;std::uint32_t pc{},address{};
        if(p==P::request_bytes){pc=0x13fe5au;address=g[4]+0x105cu;}
        else if(p==P::owner){push(0x13fe61u,g[3]);push(0x13fe62u,g[5]);pc=0x13fe63u;address=g[4]+0x1060u;}
        else if(p==P::priority){pc=0x13fe87u;address=g[4]+0x106cu;}
        else{pc=p==P::failure_priority?0x13fef0u:0x13ffb6u;address=g[4]+0x1070u;}
        at(pc);const auto v=physics_step_detail::r(s.memory,address,4);
        if(p==P::request_bytes)g[0]=v;else if(p==P::owner)g[5]=v;else if(p==P::live_priority)g[2]=v;
        else if(p==P::failure_priority)s.flags.minus(v,g[1],4);else g[1]=v;return v;
    }
    std::uint32_t read(P p,const D &d,std::uint32_t address,unsigned width=4){auto &g=s.gp;std::uint32_t pc{};
        switch(p){
        case P::page_shift:pc=0x13fe6au;break;case P::policy_read:pc=0x13feb2u;break;case P::budget:pc=0x13feb6u;break;
        case P::total_initial:if(std::bit_cast<std::int32_t>(d.pages)>=std::bit_cast<std::int32_t>(d.budget))s.flags.minus(g[1],g[3],4);pc=0x13fec0u;break;
        case P::head_initial:pc=0x13fec4u;break;case P::failure_policy:pc=0x13feecu;break;case P::failure_budget:pc=0x13fef9u;break;
        case P::failure_pages:pc=0x13fefdu;break;case P::result_failure:pc=0x13ff0au;break;
        case P::ring_begin_next:pc=0x13ff1bu;break;case P::ring_end_next:pc=0x13ff1fu;break;
        case P::ring_previous:ring(1,g[0]+1u);s.flags.minus(g[1],g[2],4);pc=0x13ff3au;break;
        case P::tail_total:if(d.next_index==d.ring_begin){ring(1,g[0]+1u);s.flags.minus(g[1],g[2],4);}s.flags.minus(g[7],0xffffffffu,4);pc=0x13ff5bu;break;
        case P::row_pool:if(d.next_index==d.ring_begin){ring(1,g[0]+1u);s.flags.minus(g[1],g[2],4);}s.flags.minus(g[7],0xffffffffu,4);pc=0x13ff75u;break;
        case P::row_rows:pc=0x13ff78u;break;
        case P::row_start:g[0]=s.flags.logic(g[7]&0xffffu,4)*3u;g[6]=g[1]+g[0]*8u;pc=0x13ff88u;break;
        case P::gap_start:pc=0x140020u;break;case P::row_pages:pc=0x13ff91u;break;case P::row_tick:pc=0x13ff94u;break;
        case P::callback:pc=0x13ff9bu;break;case P::row_tick_live:if(!d.callback)s.set(1,s.flags.logic(0u,1),1,0);pc=0x13ffb3u;break;
        case P::owner_tick:pc=0x13ffbdu;break;
        case P::victim_current:s.flags.logic(s.reg(1,1,0),1);pc=0x13ffceu;break;
        case P::victim_tick_current:pc=0x13ffd5u;break;
        case P::row_next:if(d.priority+d.record_tick<d.epoch&&d.busy)s.flags.logic(s.reg(1,1,0),1);pc=0x13ffe3u;break;case P::row_next_pool:pc=0x13ffe6u;break;case P::row_next_pages:pc=0x13ffe9u;break;case P::row_next_rows:pc=0x13ffecu;break;
        case P::row_next_start:pc=0x13fffcu;break;
        case P::handle_reload:g[0]*=3u;g[3]=s.flags.plus(g[3],g[7],4);s.flags.logic(s.reg(1,1,0),1);pc=0x14000au;break;
        case P::busy_ring_end:pc=0x140013u;break;case P::window_begin:pc=0x140037u;break;
        case P::window_end_before:pc=0x14003bu;break;case P::window_span_tick:pc=0x140047u;break;
        case P::window_tick:g[1]=shift_immediate<4>(s,g[0]);g[6]=g[4]+0x64u+g[1];ring(1,g[0]+1u);g[0]=shift_immediate<4>(s,g[1])+g[4]+0x64u;pc=0x14007au;break;
        case P::live_span_pages:pc=0x140086u;break;case P::window_pages:pc=0x14008au;break;
        case P::window_required:g[7]=s.flags.plus(g[7],g[0],4);pc=0x14008fu;break;case P::chosen_read:pc=0x14009cu;break;
        case P::strategy:pc=0x1400a4u;break;
        case P::rank_epoch:g[0]=s.flags.minus(g[0],0,4);g[0]=s.flags.increment(g[0],4,true);pc=0x1400afu;break;
        case P::rank_tick:pc=0x1400b2u;break;
        case P::rank_best_tick:g[0]=s.flags.minus(g[2],g[1],4);g[1]=s.flags.logic(0u,4);s.flags.minus(g[1],g[0],4);g[1]=s.flags.borrow(g[1],g[1],4);g[1]=s.flags.logic(g[1]&g[0],4);g[0]=g[2];pc=0x1400c3u;break;
        case P::rank_best_pages:g[2]=s.flags.logic(0u,4);s.flags.minus(g[2],g[0],4);g[2]=s.flags.borrow(g[2],g[2],4);g[2]=s.flags.logic(g[2]&g[0],4);pc=0x1400cfu;break;
        case P::rank_tick_simple:g[0]=s.flags.minus(g[0],0,4);pc=0x1400edu;break;case P::rank_best_tick_simple:pc=0x1400f0u;break;case P::rank_best_pages_simple:pc=0x1400fcu;break;
        case P::next_index_reload:s.flags.logic(s.reg(0,1,0),1);pc=0x140106u;break;
        case P::choose_previous:pc=0x14010cu;break;case P::choose_tick:pc=0x14010eu;break;case P::choose_start:pc=0x140115u;break;case P::choose_pages:pc=0x14011cu;break;
        case P::advance_begin:pc=0x14012cu;break;case P::span_tick_reload:pc=0x140145u;break;
        case P::window_end_after:g[0]=g[1];pc=0x14014bu;break;case P::window_handle_reload:pc=0x140155u;break;case P::total_again:pc=0x140159u;break;
        case P::chosen_finish:pc=0x140162u;break;case P::scan_pool:pc=0x14016eu;break;case P::scan_cursor:pc=0x140180u;break;case P::scan_limit:pc=0x140187u;break;case P::scan_bitmap:s.flags.minus(g[2],g[7],4);pc=0x14018eu;break;
        case P::bitmap_word:
            if(d.bitmap_index!=d.scan_index){g[2]=s.flags.increment(g[2],4,false);s.flags.minus(g[2],g[7],4);}
            g[1]=s.flags.logic(g[2]&31u,4);g[3]=s.flags.shift(1u,g[1],4);g[1]=s.flags.right(g[2],5u,4,true);s.flags.load(native_effect_datum_allocate_detail::scan_shift_flags(g[2]));pc=0x1401a2u;break;
        case P::scan_stride:pc=0x1401f1u;break;case P::scan_rows:pc=0x1401f4u;break;
        case P::scan_salt:{const auto flags=native_effect_datum_allocate_detail::product_flags(g[0],g[2]);g[0]=s.flags.product(g[0],g[2]);s.flags.load(flags);}pc=0x1401fau;break;case P::overlap_start:pc=0x1401feu;break;
        case P::overlap_required:g[0]=s.flags.plus(g[0],g[3],4);pc=0x140204u;break;
        case P::occupied_start:pc=0x140211u;break;case P::occupied_pages:pc=0x14021eu;break;
        case P::finished_pool:pc=0x1401acu;break;case P::finished_occupancy:pc=0x1401afu;break;case P::finished_capacity:pc=0x1401b2u;break;
        case P::victim_load:pc=0x1401bbu;break;case P::victim_previous:pc=0x1401c4u;break;case P::victim_rows:pc=0x1401ccu;break;
        case P::victim_previous_link:g[0]=s.flags.logic(g[3]&0xffffu,4)*3u;pc=0x1401d9u;break;
        case P::predecessor_load:pc=0x14023bu;break;case P::allocate_pool:pc=0x14023fu;break;
        case P::allocated_pool:pc=0x140254u;break;case P::allocated_rows:pc=0x140257u;break;case P::result_reload:pc=0x14025au;break;
        case P::head_load:g[0]=s.flags.logic(g[7]&0xffffu,4);g[3]=s.flags.logic(g[3]|0xffffffffu,4);s.flags.minus(g[6],g[3],4);g[2]=g[0]*3u;g[0]=g[1]+g[2]*8u;pc=0x140272u;break;
        case P::empty_head_reload:pc=0x14027cu;break;case P::head_reload:pc=0x140299u;break;
        case P::previous_next:g[0]=s.flags.logic(g[7]&0xffffu,4);g[3]=s.flags.logic(g[3]|0xffffffffu,4);s.flags.minus(g[6],g[3],4);g[2]=g[0]*3u;g[0]=g[1]+g[2]*8u;g[6]=s.flags.logic(g[6]&0xffffu,4)*3u;g[6]=shift_immediate<3>(s,g[6]);pc=0x1402b0u;break;
        case P::tail_load:pc=0x1402b8u;break;
        case P::next_previous:g[2]=s.flags.logic(g[2]&0xffffu,4)*3u;g[1]+=g[2]*8u;pc=0x140302u;break;
        case P::link_pool:pc=0x1402c1u;break;case P::link_rows:pc=0x1402c4u;break;case P::link_next:pc=0x1402c7u;break;
        case P::best_start_load:pc=0x1402d3u;break;case P::required_load:pc=0x1402d7u;break;case P::epoch_load:pc=0x1402e1u;break;
        default:throw std::runtime_error("Invalid native page allocation read phase");
        }
        at(pc);const auto v=physics_step_detail::r(s.memory,address,width);
        switch(p){
        case P::page_shift:case P::policy_read:case P::total_initial:case P::failure_policy:case P::row_pool:case P::row_rows:case P::rank_tick:case P::rank_tick_simple:case P::allocated_rows:case P::empty_head_reload:case P::head_reload:case P::tail_load:case P::link_rows:case P::best_start_load:case P::epoch_load:g[1]=v;break;
        case P::budget:s.flags.minus(g[6],v,4);break;
        case P::head_initial:case P::window_handle_reload:case P::overlap_start:case P::result_reload:g[7]=v;break;
        case P::failure_budget:case P::result_failure:case P::ring_end_next:case P::tail_total:case P::row_pages:case P::callback:case P::row_tick_live:case P::row_next:case P::busy_ring_end:case P::live_span_pages:case P::strategy:case P::choose_tick:case P::choose_pages:case P::scan_bitmap:case P::scan_stride:case P::allocated_pool:g[0]=v;break;
        case P::failure_pages:g[2]=v;s.flags.minus(g[0],g[2],4);if(std::bit_cast<std::int32_t>(g[0])>std::bit_cast<std::int32_t>(g[2]))g[0]=g[2];break;
        case P::ring_begin_next:case P::row_tick:case P::row_next_pool:case P::row_next_rows:case P::window_begin:case P::window_span_tick:case P::rank_epoch:case P::choose_previous:case P::choose_start:case P::advance_begin:case P::span_tick_reload:case P::finished_occupancy:case P::victim_previous_link:case P::allocate_pool:case P::head_load:case P::next_previous:case P::link_pool:case P::link_next:case P::required_load:g[2]=v;break;
        case P::ring_previous:case P::gap_start:case P::scan_pool:case P::predecessor_load:g[6]=v;break;
        case P::row_start:s.flags.minus(g[3],v,4);break;
        case P::owner_tick:g[3]=v;g[2]=s.flags.plus(g[2],g[0],4);s.flags.minus(g[2],g[3],4);if(g[2]>=g[3])s.set(1,1u,1,0);break;
        case P::handle_reload:g[7]=v;g[0]=g[2]+g[0]*8u;break;
        case P::previous_next:g[2]=v;s.flags.minus(g[2],g[3],4);break;case P::victim_previous:g[6]=v;s.flags.minus(g[6],g[3],4);break;
        case P::victim_current:s.flags.minus(v,0xffffffffu,4);break;case P::victim_tick_current:s.flags.minus(g[0],v,4);break;
        case P::row_next_pages:g[3]=v;break;case P::row_next_start:g[7]=v;break;
        case P::window_end_before:s.flags.minus(g[2],v,4);g[0]=g[2];break;
        case P::window_tick:s.flags.minus(g[2],v,4);break;case P::window_pages:g[7]=v;break;case P::window_required:s.flags.minus(g[7],v,4);break;
        case P::chosen_read:case P::chosen_finish:s.set(0,v,1,0);s.flags.logic(s.reg(0,1,0),1);break;
        case P::rank_best_tick:g[0]=s.flags.minus(g[0],v,4);break;
        case P::rank_best_pages_simple:s.flags.minus(g[7],v,4);s.set(0,s.flags.sf!=s.flags.of,1,0);break;
        case P::rank_best_tick_simple:g[2]=v;s.set(0,s.flags.logic(0u,1),1,0);s.flags.minus(g[1],g[2],4);if(g[1]<g[2])s.set(0,1u,1,0);break;
        case P::rank_best_pages:g[0]=v;g[7]=s.flags.product(g[7],g[2]);g[0]=s.flags.product(g[0],g[1]);s.flags.minus(g[0],g[7],4);s.set(0,!s.flags.zf&&(s.flags.sf==s.flags.of),1,0);break;
        case P::next_index_reload:g[1]=v;break;
        case P::window_end_after:s.flags.minus(g[0],v,4);break;case P::total_again:s.flags.minus(g[3],v,4);break;
        case P::scan_cursor:g[2]=s.flags.increment(v,4,false);break;case P::scan_limit:g[7]=v;s.flags.minus(g[2],g[7],4);break;
        case P::bitmap_word:s.flags.logic(v&g[3],4);break;case P::scan_rows:g[3]=v;break;
        case P::scan_salt:g[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));break;
        case P::overlap_required:g[3]=v;g[1]=s.flags.shift(g[1],16u,4);g[1]=s.flags.logic(g[1]|g[2],4);break;
        case P::occupied_start:g[2]=v;g[3]=s.flags.plus(g[3],g[7],4);s.flags.minus(g[2],g[3],4);break;
        case P::occupied_pages:g[0]=s.flags.plus(v,g[2],4);g[2]=g[7];s.flags.minus(g[0],g[2],4);break;
        case P::finished_pool:case P::victim_rows:g[1]=v;break;case P::finished_capacity:s.flags.minus(g[2],v,4);break;
        case P::victim_load:g[3]=v;s.flags.minus(g[3],0xffffffffu,4);break;
        default:break;
        }
        return v;
    }
    void write(P p,const D &d,std::uint32_t address,std::uint32_t value,unsigned width=4){auto &g=s.gp;std::uint32_t pc{};
        switch(p){
        case P::pages_initial:g[2]=1u;push(0x13fe72u,g[6]);g[2]=s.flags.shift(g[2],g[1],4);g[6]=s.flags.right(g[0],g[1],4,true);g[2]=s.flags.increment(g[2],4,true);s.flags.logic(g[0]&g[2],4);pc=0x13fe7cu;address=g[4]+0x18u;break;
        case P::pages_rounded:g[6]=s.flags.increment(g[6],4,false);pc=0x13fe83u;break;
        case P::result_reset:g[0]=s.flags.logic(g[0]|0xffffffffu,4);g[3]=s.flags.logic(0u,4);s.flags.minus(g[1],g[3],4);pc=0x13fe95u;break;
        case P::policy_zero:pc=0x13fe9bu;break;case P::policy_cap:s.flags.minus(g[1],8u,4);pc=0x13fea4u;break;case P::policy_input:pc=0x13feaeu;break;
        case P::victim_reset:pc=0x13fec7u;break;
        case P::chosen_reset:g[2]=s.flags.logic(0u,4);g[0]=s.flags.logic(0u,4);s.flags.logic(g[1],4);pc=0x13fed1u;break;
        case P::ring_begin_reset:pc=0x13fed6u;break;case P::ring_end_reset:pc=0x13fedau;break;case P::handle_reset:pc=0x13fedeu;break;case P::previous_reset:pc=0x13fee2u;break;
        case P::budget_publish:pc=0x13ff06u;break;
        case P::ring_previous:g[0]=shift_immediate<4>(s,g[0])+g[4]+0x64u;pc=0x13ff45u;break;
        case P::ring_start:g[6]=s.flags.logic(0u,4);pc=0x13ff49u;break;case P::ring_tick:pc=0x13ff4cu;break;case P::ring_pages:pc=0x13ff4fu;break;case P::ring_end_publish:pc=0x13ff52u;break;
        case P::tail_tick:g[1]=s.flags.minus(g[0],g[3],4);pc=0x13ff62u;break;case P::tail_pages:pc=0x13ff6au;break;
        case P::gap_tick:g[0]=s.flags.minus(g[6],g[3],4);pc=0x140027u;break;case P::gap_pages:pc=0x14002fu;break;
        case P::span_pages:pc=0x13ff97u;break;case P::span_tick:s.flags.logic(g[0],4);pc=0x13ffa0u;break;
        case P::victim_publish:pc=0x13ffdbu;break;case P::victim_tick_publish:pc=0x13ffdfu;break;
        case P::handle_publish:pc=0x13ffefu;break;case P::previous_publish:g[0]=s.flags.logic(g[0]&0xffffu,4);pc=0x13fff8u;break;case P::busy_ring_reset:pc=0x140017u;break;
        case P::window_next_publish:pc=0x14007du;break;case P::window_tick_publish:pc=0x140083u;break;case P::window_pages_publish:pc=0x140093u;break;
        case P::choose_previous_publish:pc=0x140111u;break;case P::choose_tick_publish:pc=0x140118u;break;case P::choose_start_publish:pc=0x14011fu;break;case P::choose_pages_publish:pc=0x140123u;break;case P::choose_publish:pc=0x140127u;break;
        case P::advance_begin_publish:ring(2,s.flags.increment(g[2],4,false));pc=0x140141u;break;case P::scan_cursor_reset:pc=0x140171u;break;case P::scan_cursor_publish:pc=0x14020du;break;
        case P::victim_previous_publish:pc=0x1401ddu;break;
        case P::result_publish:s.flags.minus(g[0],0xffffffffu,4);pc=0x14024au;break;
        case P::new_previous:s.flags.minus(g[2],g[3],4);pc=0x140277u;break;case P::empty_tail_publish:pc=0x14027fu;break;case P::empty_next_publish:pc=0x140282u;break;case P::empty_head_publish:pc=0x140285u;break;
        case P::head_previous_publish:g[2]=s.flags.logic(g[2]&0xffffu,4)*3u;g[1]+=g[2]*8u;pc=0x140296u;break;case P::head_next_publish:pc=0x14029cu;break;case P::head_publish:pc=0x14029fu;break;
        case P::tail_previous_publish:pc=0x1402bbu;break;case P::tail_publish:pc=0x1402beu;break;case P::new_previous_publish:pc=0x140305u;break;case P::next_previous_publish:pc=0x140308u;break;
        case P::new_next_publish:g[1]=s.flags.plus(g[1],g[6],4);pc=0x1402cdu;break;case P::previous_next_publish:pc=0x1402d0u;break;
        case P::page_start_publish:pc=0x1402dbu;break;case P::page_pages_publish:pc=0x1402deu;break;case P::page_epoch_publish:pc=0x1402e4u;break;
        default:throw std::runtime_error("Invalid native page allocation store phase");
        }
        at(pc);physics_step_detail::w(s.memory,s.observe,pc,address,width,value);
        if(p==P::tail_pages)g[3]=g[0];else if(p==P::gap_pages)g[3]=g[6];else if(p==P::victim_previous_publish)g[6]=g[2];
    }
    void prepare(P p,const D &d){auto &g=s.gp;
        if(p==P::begin_scan){push(0x13fec3u,g[7]);}
        else if(p==P::bitmap_done){
            if(!d.bitmap_result.found){g[2]=s.flags.increment(g[2],4,false);s.flags.minus(g[2],g[7],4);}
            else s.flags.minus(g[2],0xffffffffu,4);
        }else if(p==P::prefer_call){g[0]=s.flags.minus(g[0],0u,4);g[0]=s.flags.increment(g[0],4,true);g[7]=g[4]+0x38u;g[1]=g[5];}
        else if(p==P::retire_overlap_call){g[3]=g[1];g[7]=g[5];}
        else if(p==P::retire_victim_call){g[7]=g[5];}
    }
    void complete(P p,const D&){auto &g=s.gp;
        const bool success=p==P::finish_success;
        if(success){g[0]=g[7];at(0x1402e9u);g[7]=s.pop();}
        else if(p==P::finish_failure){at(0x13ff0eu);g[7]=s.pop();}
        else s.flags.minus(g[1],g[3],4);
        const std::uint32_t first=success?0x1402eau:0x13ff0fu;
        at(first);g[6]=s.pop();at(first+1u);g[5]=s.pop();at(first+2u);g[3]=s.pop();
        g[4]=s.flags.plus(g[4],0x1054u,4);at(success?0x1402f3u:0x13ff18u);s.pop();g[4]+=12u;
    }
};
template<class State,class Call,class Before=native_page_window_detail::Noop>bool run(State &s,Call call,Before before={}){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false; // Exact undefined arithmetic flag backend is unavailable.
#endif
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().calls;Access<State,Before> access{s,before};D data;
    struct Services {
        State &s;Call &call;Access<State,Before> &access;
        void fallback(std::uint32_t owner,std::uint32_t site,bool tail=false){++activity().residuals[owner];call(s,owner,site,tail);}
        NativeTu15PageBusyResume busy(std::uint32_t owner,std::uint32_t,const D&){
            access.push(0x13ffa6u,s.gp[7]);access.push(0x13ffa7u,0x13ffa9u);access.before(s,owner);
            if(owner==0x12fcb0u&&!native_resource_busy_detail::reference_mode()&&native_partition_temporary_detail::admitted(s)){
                // Compose the same qualified DATA/ABI access owner, retaining
                // the real nested query entry before its first instruction.
                // Its separately bound facade needs no such hook in production.
                ++activity().native_busy;++native_resource_busy_detail::activity().callbacks;
                NativeTu15PageBusy child;native_resource_busy_detail::CallbackAccess<State,Before> memory{s,access.before};
                const auto resource=[&](std::uint32_t,const NativeTu15PageBusy&){
                    access.before(s,0x4057f0u);
                    if(!native_resource_busy_detail::query(s,access.before)){++native_resource_busy_detail::activity().reference_queries;fallback(0x4057f0u,0x12fcf5u);}
                    s.flags.logic(s.gp[0],4);return s.gp[0]!=0u;
                };
                native_tu15_page_is_busy(memory,child,resource);
            }else fallback(owner,0x13ffa7u);
            s.flags.logic(s.reg(0,1,0),1);const bool busy=(s.gp[0]&255u)!=0;if(busy)s.set(1,1u,1,0);else s.set(1,s.flags.logic(0u,1),1,0);
            return{s.gp[5],s.gp[6],s.gp[7],busy};
        }
        NativeTu15PageWindowResume prefer(std::uint32_t,std::uint32_t,std::uint32_t,const D&){access.push(0x1400e6u,0x1400ebu);access.before(s,0x13fe00u);
            if(native_page_window_detail::run(s,access.before))++activity().native_preferences;else fallback(0x13fe00u,0x1400e6u);
            return{s.gp[5],s.gp[6],s.gp[3],(s.gp[0]&255u)!=0};}
        NativeTu15PageOverlapResume retire_overlap(std::uint32_t,std::uint32_t,const D&){access.push(0x140231u,0x140236u);access.before(s,0x140310u);
            if(native_page_retire_detail::run(s,call,access.before))++activity().native_retirements;else fallback(0x140310u,0x140231u);return{s.gp[5],s.gp[6]};}
        NativeTu15PageVictimResume retire_victim(std::uint32_t,std::uint32_t,const D&){access.push(0x1401e5u,0x1401eau);access.before(s,0x140310u);
            if(native_page_retire_detail::run(s,call,access.before))++activity().native_retirements;else fallback(0x140310u,0x1401e5u);return{s.gp[5],s.gp[6]};}
        NativeTu15PageAllocationResume allocate(std::uint32_t,const D&){access.push(0x140242u,0x140247u);access.before(s,0x16e6a0u);
            auto flags=s.flags.bits()|s.other_flags;bool accepted{};try{accepted=native_effect_datum_allocate_detail::try_native(s.memory,s.gp,flags,s.services,s.observe);}
            catch(...){s.other_flags=flags&~0x8d5u;s.flags.load(flags);throw;}s.other_flags=flags&~0x8d5u;s.flags.load(flags);
            if(accepted)++activity().native_allocations;else fallback(0x16e6a0u,0x140242u);return{s.gp[5],s.gp[6],s.gp[0]};}
    }services{s,call,access};
    s.gp[0]=0x1054u;access.push(0x13fe55u,0x13fe5au);access.before(s,0x32a660u);
    if(native_stack_probe_detail::run(s,before))++activity().native_probes;else services.fallback(0x32a660u,0x13fe55u);
    native_tu15_allocate_pages(access,data,services);return true;
}
} // namespace halo2::native_page_allocate_detail
