#pragma once
#include "gameplay/objects/native_tu15_object_ratio_caller_combat_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_ratio_caller_detail {
template<class State,class Before> void run_whole_frame(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Setup<State,Before> setup{state,before,fp};setup.open_object();
    DispatchFrame<State,Before> entry{state,before,fp};
    MiddleFrame<State,Before> exit{state,before,fp};
    HighFrame<State,Before> high{state,before,fp};
    LowFrame<State,Before> low{state,before,fp};
    CombatFrame<State,Before> combat{state,before,fp};
    ExtendedFrame<State,Before> extended{state,before,fp};
    auto extended_result=[&](ExtendedStage next) {
        if(next==ExtendedStage::finish)return HighResult::finish;
        if(next!=ExtendedStage::scan_records){
            MaximumFrame<State,Before> maximum{state,before,fp};scan_initial_maximum(maximum,next);
        }
        scan_extended_records(extended);return HighResult::clamp;
    };
    auto clamp_if=[](bool condition){return condition?HighResult::clamp:HighResult::finish;};
    auto result=HighResult::finish;
    switch(select_initial_route(entry)){
    case Route::high:result=run_high_keys(high);break;
    case Route::exact:result=clamp_if(high.sample_record(true));break;
    case Route::unknown:result=HighResult::unknown;break;
    case Route::extended:result=extended_result(prepare_extended(extended));break;
    case Route::middle:
        switch(select_middle_route(exit)){
        case MiddleRoute::first_key:result=clamp_if(high.sample_record(true));break;
        case MiddleRoute::upper:result=run_upper_middle_keys(high);break;
        case MiddleRoute::second_key:result=clamp_if(low.sample_signed_ratio());break;
        case MiddleRoute::record_ratio:run_record_branch(exit);return;
        case MiddleRoute::mode_ratio:result=clamp_if(run_combat_ratio(combat,true));break;
        case MiddleRoute::unknown:result=HighResult::unknown;break;
        }
        break;
    case Route::low:
        switch(select_low_route(low)){
        case LowRoute::direct_flag:low.direct_flag();break;
        case LowRoute::conditional_flag:low.conditional_flag();break;
        case LowRoute::first_field:low.sample_field(0x1048d4u,0x180u);result=HighResult::clamp;break;
        case LowRoute::second_field:low.sample_field(0x1048e1u,0x184u);result=HighResult::clamp;break;
        case LowRoute::third_field:low.sample_field(0x1048eeu,0x18cu);result=HighResult::clamp;break;
        case LowRoute::comparison:result=clamp_if(low.compare_position());break;
        case LowRoute::scaled_byte:low.scaled_byte();result=HighResult::clamp;break;
        case LowRoute::recent_record:result=clamp_if(low.recent_record());break;
        case LowRoute::record_ratio:run_record_branch(exit);return;
        case LowRoute::alternate_extended:result=extended_result(extended.locate_initial_scan());break;
        case LowRoute::combat_ratio:result=clamp_if(run_combat_ratio(combat,false));break;
        case LowRoute::unknown:result=HighResult::unknown;break;
        }
        break;
    }
    if(result==HighResult::unknown){high.finish_unknown();return;}
    if(result==HighResult::clamp)exit.clamp_result();
    exit.finish_result();
}
} // namespace halo2::native_object_ratio_caller_detail
#endif
