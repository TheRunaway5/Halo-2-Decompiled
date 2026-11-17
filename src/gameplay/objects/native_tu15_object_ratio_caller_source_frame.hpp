#pragma once

#include "gameplay/objects/native_tu15_object_ratio_caller_combat_source_frame.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_dispatch_source_frame.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_extended_source_frame.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_low_source_frame.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_middle_source_frame.hpp"

namespace halo2::native_object_ratio_caller_detail {
template<bool Literal,bool Ordered,class State,class Children>
void run_source_frame(State& state,Children& calls) {
    SourceMiddleFrame<Literal,Ordered,State,Children> exit{state,calls};
    SourceHighFrame<Literal,Ordered,State> high{state};
    SourceLowFrame<Literal,State,Children> low{state,calls};
    SourceCombatFrame<Literal,State,Children> combat{state,calls};
    SourceExtendedFrame<Literal,Ordered,State,Children> extended{state,calls};
    auto extended_result=[&](ExtendedStage next) {
        if(next==ExtendedStage::finish)return HighResult::finish;
        if(next!=ExtendedStage::scan_records)scan_initial_maximum(extended,next);
        scan_extended_records(extended);return HighResult::clamp;
    };
    auto clamp_if=[](bool condition){return condition?HighResult::clamp:HighResult::finish;};
    auto result=HighResult::finish;
    switch(run_source_dispatch<Literal,Ordered>(state)){
    case Route::high:result=run_high_keys(high);break;
    case Route::exact:result=clamp_if(high.sample_record(true));break;
    case Route::unknown:result=HighResult::unknown;break;
    case Route::extended:result=extended_result(prepare_extended(extended));break;
    case Route::middle:
        switch(select_middle_route(exit)){
        case MiddleRoute::first_key:result=clamp_if(high.sample_record(true));break;
        case MiddleRoute::upper:result=run_upper_middle_keys(high);break;
        case MiddleRoute::second_key:result=clamp_if(exit.sample_signed_ratio());break;
        case MiddleRoute::record_ratio:run_record_branch(exit);return;
        case MiddleRoute::mode_ratio:result=clamp_if(run_combat_ratio(combat,true));break;
        case MiddleRoute::unknown:result=HighResult::unknown;break;
        }
        break;
    case Route::low:
        switch(low.select()){
        case LowRoute::direct_flag:low.direct_flag();break;
        case LowRoute::conditional_flag:low.conditional_flag();break;
        case LowRoute::first_field:low.sample_field(0x1048d4u,0x180u,0x1048dcu);result=HighResult::clamp;break;
        case LowRoute::second_field:low.sample_field(0x1048e1u,0x184u,0x1048e9u);result=HighResult::clamp;break;
        case LowRoute::third_field:low.sample_field(0x1048eeu,0x18cu,0x1048f6u);result=HighResult::clamp;break;
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
