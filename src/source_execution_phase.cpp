#include "halo2/source_match_authority.hpp"
#include "halo2/source_execution_calls.hpp"
#include <new>
#include <optional>

namespace halo2 {
struct SourceMatchAuthority::ExecutionRecording::Impl {
    SourceMatchAuthority &owner;
    SourceExecutionInputs tape;
    std::optional<SourceExecutionInputBinding> binding;
    std::optional<NativeAuthorityBoundary::Call> active;
    Impl(SourceMatchAuthority &o,std::size_t limit):owner(o),tape(limit){
        tape.track_calls(owner.match_.execution_call_counters());
        binding.emplace(owner.world_.memory(),tape);
        active.emplace(owner.calls_);++owner.calls_.mutations;
    }
};
SourceMatchAuthority::ExecutionRecording::ExecutionRecording(SourceMatchAuthority &owner,std::size_t limit)
    :impl_(std::make_unique<Impl>(owner,limit)){}
SourceMatchAuthority::ExecutionRecording::~ExecutionRecording()=default;
SourceMatchAuthority::ExecutionRecording::ExecutionRecording(ExecutionRecording &&)noexcept=default;
SourceMatchAuthority::ExecutionRecording &SourceMatchAuthority::ExecutionRecording::operator=(ExecutionRecording &&)noexcept=default;
SourceMatchAuthority::ExecutionRecording SourceMatchAuthority::start_execution_phase(std::size_t limit){
    world_.require_authority_executor(true);calls_.capture();match_.authority_calls_.capture();
    return ExecutionRecording(*this,limit);
}
SourceExecutionPhase SourceMatchAuthority::ExecutionRecording::finish(){
    if(!impl_)throw FormatError("Native recording scope is already complete");
    auto &s=*impl_;s.tape.require_executor();
    s.binding.reset();s.active.reset();
    const auto inputs=s.tape.finish();
    s.owner.world_.require_authority_executor(true);s.owner.calls_.capture();s.owner.match_.authority_calls_.capture();
    SourceExecutionPhase phase;phase.inputs.assign(inputs.begin(),inputs.end());
    const auto calls=s.tape.calls();phase.calls.assign(calls.begin(),calls.end());
    phase.clocks=s.owner.world_.tu15_world().execution_clocks();phase.result_seconds=s.owner.match_.result_wall_time();
    impl_.reset();return phase;
}
SourceExecutionPhase SourceMatchAuthority::capture_execution_phase(const std::function<void()> &execute,std::size_t limit) {
    world_.require_authority_executor(true);calls_.capture();match_.authority_calls_.capture();
    if(!execute)throw FormatError("Native capture requires its complete source phase");
    SourceExecutionInputs tape(limit);
    tape.track_calls(match_.execution_call_counters());
    std::optional<SourceExecutionInputBinding> binding;
    try{binding.emplace(world_.memory(),tape);}
    catch(const std::bad_alloc &){
        // Binding storage failed before any native work. Execute the real
        // host phase once without a recording, then reject its publication.
        try{NativeAuthorityBoundary::Call phase(calls_);++calls_.mutations;execute();}
        catch(...){calls_.fault=true;throw;}throw;
    }
    try{NativeAuthorityBoundary::Call phase(calls_);++calls_.mutations;execute();}
    catch(...){calls_.fault=true;throw;}
    binding.reset();
    // A recording/allocation failure here must not poison the actual host.
    const auto inputs=tape.finish();
    world_.require_authority_executor(true);match_.authority_calls_.capture();
    SourceExecutionPhase phase;
    phase.inputs.assign(inputs.begin(),inputs.end());
    const auto calls=tape.calls();phase.calls.assign(calls.begin(),calls.end());
    phase.clocks=world_.tu15_world().execution_clocks();
    phase.result_seconds=match_.result_wall_time();return phase;
}
void SourceMatchAuthority::replay_execution_phase(const SourceExecutionPhase &phase,const std::function<void()> &execute) {
    world_.require_authority_executor(true);calls_.capture();match_.authority_calls_.capture();
    if(!execute)throw FormatError("Native replay requires its complete source phase");
    validate_source_execution_calls(phase.calls);
    SourceExecutionInputs tape(phase.inputs,phase.calls);
    tape.track_calls(match_.execution_call_counters());
    calls_.fault=true;match_.authority_calls_.fault=true;
    {SourceExecutionInputBinding binding(world_.memory(),tape);
        NativeAuthorityBoundary::Call active(calls_);++calls_.mutations;execute();}
    tape.finish();world_.require_authority_executor(true);
    // Sample every fallible provider before changing any clock adjustment.
    // The world providers are noexcept and its completed-call guard still runs.
    const auto result_adjustment=phase.result_seconds-match_.result_wall_clock_();
    world_.tu15_world().anchor_execution_clocks(phase.clocks);
    match_.result_wall_adjustment_=result_adjustment;
    match_.authority_calls_.fault=false;calls_.fault=false;
}
void SourceMatchAuthority::replay_execution_phase(const SourceExecutionPhase &phase){
    replay_execution_phase(phase,[&]{replay_source_execution_calls(match_,world_,phase.calls);});
}
}
