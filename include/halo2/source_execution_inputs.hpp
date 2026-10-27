#pragma once
#include <cstddef>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace halo2 {
class SourceRuntimeMemory;
class SourceRuntimePolicyChange;
// External operands consumed by the native source executor, never original
// gameplay function returns. The operand binds the exact estimate request.
enum class SourceExecutionInputKind:std::uint8_t {
    Milliseconds,TimestampCounter,ReciprocalSquareRoot,UtcFiletime,ResultWallSeconds,
    GraphicsRead1,GraphicsRead2,GraphicsRead4,GraphicsRead8
};
struct SourceExecutionInput {
    SourceExecutionInputKind kind{};
    std::uint32_t operand{};
    std::uint64_t value{};
    bool operator==(const SourceExecutionInput &)const=default;
};
// Explicit end-of-phase external clock domains. Never infer these from the
// last clock read: a phase may not consume every kind. No host pointers travel.
struct SourceExecutionClocks {
    std::uint32_t milliseconds{};
    std::uint64_t cycles{},utc_filetime{};
    bool operator==(const SourceExecutionClocks &)const=default;
};
enum class SourceExecutionCallKind:std::uint8_t {
    MatchStep=1,EffectParticles,BeginModelFrame,BeginModelView,ObserverPosition,
    RenderVisibility,WeatherView,BuildWorldModel,FirstPersonModels,AdvanceAudio,
    TakeSounds,TakePropEvents,TakeGrenadeEvents,DepartPlayer,WriteSourceCallInput,
    PrepareSourceCall,RunSourceCall,RunActualSourceCall,CallSource,CallActualSource,LiveModelDraws,
    RenderAttachments,RenderGeometry,MatchState,PlayerCamera,LensQuery,LensQueryFinish,LensDraw,ParticleCollision,ObjectLoopEmitters,
    AdmittedCamera=31,
    // MatchStep with the producer's rewind mode and remote displayed views
    // (lag-compensated hitscan, NETCODE_ENHANCED_GOAL item 2).
    MatchStepViews=32
};
inline constexpr SourceExecutionCallKind source_execution_call_kind_last=SourceExecutionCallKind::MatchStepViews;
constexpr bool source_execution_call_has_result(SourceExecutionCallKind kind)noexcept {
    return (kind>=SourceExecutionCallKind::PrepareSourceCall&&kind<=SourceExecutionCallKind::RenderGeometry)||
        kind==SourceExecutionCallKind::MatchState||kind==SourceExecutionCallKind::PlayerCamera||
        kind==SourceExecutionCallKind::LensQuery||kind==SourceExecutionCallKind::LensDraw||kind==SourceExecutionCallKind::ParticleCollision||
        kind==SourceExecutionCallKind::ObjectLoopEmitters||kind==SourceExecutionCallKind::AdmittedCamera;
}
struct SourceExecutionCall {
    SourceExecutionCallKind kind{};
    std::vector<std::byte> arguments;
    std::uint32_t input_begin{},input_end{};
    // Compared with the actual source return. Never supplied to the executor
    // as a replacement for executing the original function.
    std::vector<std::byte> result;
    bool operator==(const SourceExecutionCall &)const=default;
};
struct SourceExecutionPhase {
    std::vector<SourceExecutionInput> inputs;
    std::vector<SourceExecutionCall> calls;
    SourceExecutionClocks clocks;
    std::uint32_t result_seconds{};
};
class SourceExecutionInputs {
public:
    static constexpr std::size_t maximum_entries=262144;
    // Capture failure invalidates the transcript without interrupting the host
    // or changing its real external input. finish() then rejects publication.
    explicit SourceExecutionInputs(std::size_t limit=maximum_entries);
    // Own/validate the transcript. Replay rejects a missing, reordered or
    // differently bound read before supplying an input to the native executor.
    explicit SourceExecutionInputs(std::span<const SourceExecutionInput>,std::size_t limit=maximum_entries);
    SourceExecutionInputs(std::span<const SourceExecutionInput>,std::span<const SourceExecutionCall>,std::size_t limit=maximum_entries);
    static constexpr std::size_t maximum_calls=65536,maximum_call_bytes=4*1024*1024;
    void track_calls(std::array<const std::uint64_t *,2> counters={});
    std::span<const SourceExecutionCall> calls()const;
    // Unknown native work invalidates capture but lets the real host run.
    // Replay rejects before a native mutation whenever the boundary permits.
    void untracked(std::uint32_t detail=0);
    // Recording instrumentation failed inside a known call. Revoke the tape
    // without interrupting real host work; replay must stop immediately.
    void invalidate_recording();
    SourceExecutionInputs(const SourceExecutionInputs &)=delete;
    SourceExecutionInputs &operator=(const SourceExecutionInputs &)=delete;
    std::span<const SourceExecutionInput> finish();
    bool replaying()const noexcept{return replay_;}
    bool tracks_calls()const noexcept{return track_calls_;}
    template<class F>std::uint64_t sample(SourceExecutionInputKind kind,std::uint32_t operand,F &&actual) {
        begin_sample();
        if(track_calls_&&(!call_depth_||observer_depth_))untracked();
        if(replay_)return replay_sample(kind,operand);
        try {
            const std::uint64_t value=std::invoke(std::forward<F>(actual));
            record_sample(kind,operand,value);return value;
        }catch(...){failed_=true;throw;}
    }
    void require_executor()const;
private:
    friend class SourceExecutionInputBinding;
    friend class SourceExecutionCallScope;
    friend class SourceExecutionObserverScope;
    const std::thread::id executor_=std::this_thread::get_id();
    const SourceRuntimeMemory *bound_memory_{};
    std::vector<SourceExecutionInput> entries_;
    std::size_t limit_{},cursor_{};
    bool replay_{},failed_{},closed_{};
    bool track_calls_{};
    std::vector<SourceExecutionCall> calls_;
    std::size_t call_cursor_{},call_depth_{},call_bytes_{},observer_depth_{};
    std::array<const std::uint64_t *,2> counters_{};
    std::array<std::uint64_t,2> counter_start_{},known_mutations_{};
    std::uint32_t untracked_detail_{};
    std::array<std::uint64_t,2> counter_values()const noexcept;
    void observe_call(SourceExecutionCallKind,std::vector<std::byte>);
    void observe_result(std::size_t,std::vector<std::byte>);
    void begin_sample()const;
    std::uint64_t replay_sample(SourceExecutionInputKind,std::uint32_t);
    void record_sample(SourceExecutionInputKind,std::uint32_t,std::uint64_t)noexcept;
};
SourceExecutionInputs *source_execution_inputs(const SourceRuntimeMemory &)noexcept;
class SourceExecutionCallScope {
public:
    template<class F>SourceExecutionCallScope(const SourceRuntimeMemory *memory,SourceExecutionCallKind kind,F &&arguments) {
        if(!memory)return;
        inputs_=source_execution_inputs(*memory);
        if(!inputs_||!inputs_->track_calls_){inputs_=nullptr;return;}
        inputs_->begin_sample();if(inputs_->observer_depth_)inputs_->untracked();root_=inputs_->call_depth_==0;
        if(root_){
            result_required_=source_execution_call_has_result(kind);
            before_=inputs_->counter_values();
            if(inputs_->replay_){inputs_->observe_call(kind,std::invoke(std::forward<F>(arguments)));call_index_=inputs_->call_cursor_-1;}
            else if(!inputs_->failed_)try{
                const auto count=inputs_->calls_.size();inputs_->observe_call(kind,std::invoke(std::forward<F>(arguments)));
                if(inputs_->calls_.size()>count)call_index_=count;
            }
                catch(...){inputs_->failed_=true;}
        }
        ++inputs_->call_depth_;
    }
    ~SourceExecutionCallScope();
    SourceExecutionCallScope(const SourceExecutionCallScope &)=delete;
    SourceExecutionCallScope &operator=(const SourceExecutionCallScope &)=delete;
    template<class F>void complete(F &&result) {
        if(!inputs_||!root_)return;
        inputs_->begin_sample();
        if(inputs_->replay_){
            if(completed_||!result_required_){inputs_->failed_=true;throw std::logic_error("Unexpected native call result");}
            inputs_->observe_result(call_index_,std::invoke(std::forward<F>(result)));completed_=true;
        }else if(!inputs_->failed_)try{
            if(completed_||!result_required_)throw std::logic_error("Unexpected native call result");
            inputs_->observe_result(call_index_,std::invoke(std::forward<F>(result)));completed_=true;
        }catch(...){inputs_->failed_=true;}
    }
private:
    SourceExecutionInputs *inputs_{};
    bool root_{},result_required_{},completed_{};
    std::array<std::uint64_t,2> before_{};
    std::size_t call_index_=std::size_t(-1);
};
// Local observers may copy presentation data, but cannot add source calls,
// external operands, bank writes/topology changes or Match/Simulation work.
class SourceExecutionObserverScope {
public:
    explicit SourceExecutionObserverScope(const SourceRuntimeMemory &);
    ~SourceExecutionObserverScope();
    SourceExecutionObserverScope(const SourceExecutionObserverScope &)=delete;
    SourceExecutionObserverScope &operator=(const SourceExecutionObserverScope &)=delete;
private:
    SourceExecutionInputs *inputs_{};
    std::array<std::uint64_t,2> before_{};
};
// Binding and mutable tape stay on one executor. A sealed owning entry vector
// may later cross threads; no live native owner or callback is in those values.
class SourceExecutionInputBinding {
public:
    SourceExecutionInputBinding(const SourceRuntimeMemory &,SourceExecutionInputs &);
    ~SourceExecutionInputBinding();
    SourceExecutionInputBinding(const SourceExecutionInputBinding &)=delete;
    SourceExecutionInputBinding &operator=(const SourceExecutionInputBinding &)=delete;
private:
    const SourceRuntimeMemory *memory_;
    SourceExecutionInputs *inputs_;
    std::unique_ptr<SourceRuntimePolicyChange> policy_;
    bool guards_{};
    const std::thread::id executor_=std::this_thread::get_id();
};
void source_execution_untracked(const SourceRuntimeMemory &,std::uint32_t detail=0);
template<class F>std::uint64_t source_execution_input(const SourceRuntimeMemory &memory,
    SourceExecutionInputKind kind,std::uint32_t operand,F &&actual) {
    if(auto *inputs=source_execution_inputs(memory))return inputs->sample(kind,operand,std::forward<F>(actual));
    return std::invoke(std::forward<F>(actual));
}
}
