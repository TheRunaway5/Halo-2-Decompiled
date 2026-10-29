#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <map>
#include <stdexcept>
#include <exception>

namespace halo2 {
namespace {
thread_local std::map<const SourceRuntimeMemory *,SourceExecutionInputs *> bindings;
bool valid(SourceExecutionInputKind kind,std::uint32_t operand,std::uint64_t value) {
    switch(kind){
    case SourceExecutionInputKind::Milliseconds:
    case SourceExecutionInputKind::ResultWallSeconds:return !operand&&value<=UINT32_MAX;
    case SourceExecutionInputKind::TimestampCounter:
    case SourceExecutionInputKind::UtcFiletime:return !operand;
    case SourceExecutionInputKind::ReciprocalSquareRoot:return value<=UINT32_MAX;
    case SourceExecutionInputKind::GraphicsRead1:return value<=UINT8_MAX;
    case SourceExecutionInputKind::GraphicsRead2:return operand<=UINT32_MAX-1&&value<=UINT16_MAX;
    case SourceExecutionInputKind::GraphicsRead4:return operand<=UINT32_MAX-3&&value<=UINT32_MAX;
    case SourceExecutionInputKind::GraphicsRead8:return operand<=UINT32_MAX-7;
    }
    return false;
}
void limit(std::size_t count){if(!count||count>SourceExecutionInputs::maximum_entries)
    throw std::invalid_argument("Invalid native execution input bound");}
}
SourceExecutionInputs::SourceExecutionInputs(std::size_t count):limit_(count){limit(count);}
SourceExecutionInputs::SourceExecutionInputs(std::span<const SourceExecutionInput> entries,std::size_t count)
    :limit_(count),replay_(true){
    limit(count);if(entries.size()>count)throw std::invalid_argument("Native execution input transcript exceeds its bound");
    for(const auto &entry:entries)if(!valid(entry.kind,entry.operand,entry.value))
        throw std::invalid_argument("Invalid native execution input kind, operand or width");
    entries_.assign(entries.begin(),entries.end());
}
SourceExecutionInputs::SourceExecutionInputs(std::span<const SourceExecutionInput> entries,std::span<const SourceExecutionCall> calls,std::size_t count)
    :SourceExecutionInputs(entries,count){
    if(calls.size()>maximum_calls)throw std::invalid_argument("Native call journal exceeds its bound");
    std::size_t input_end{};
    for(const auto &call:calls){
        if(call.kind<SourceExecutionCallKind::MatchStep||call.kind>source_execution_call_kind_last||
            call.arguments.size()>maximum_call_bytes-call_bytes_)
            throw std::invalid_argument("Invalid native call kind or argument budget");
        if(call.input_begin!=input_end||call.input_end<call.input_begin||call.input_end>entries.size())
            throw std::invalid_argument("Invalid native call input-consumption boundary");
        input_end=call.input_end;
        call_bytes_+=call.arguments.size();
        if(call.result.size()>maximum_call_bytes-call_bytes_)
            throw std::invalid_argument("Native call results exceed their phase budget");
        call_bytes_+=call.result.size();
    }
    if(input_end!=entries.size())throw std::invalid_argument("Native calls omit external input operands");
    calls_.assign(calls.begin(),calls.end());track_calls_=true;
}
std::array<std::uint64_t,2> SourceExecutionInputs::counter_values()const noexcept{
    return {counters_[0]?*counters_[0]:0,counters_[1]?*counters_[1]:0};
}
void SourceExecutionInputs::track_calls(std::array<const std::uint64_t *,2> counters){
    require_executor();if(bound_memory_||closed_||call_depth_)throw std::logic_error("Native journal counters require an unbound phase");
    track_calls_=true;counters_=counters;counter_start_=counter_values();
}
std::span<const SourceExecutionCall> SourceExecutionInputs::calls()const{
    require_executor();if(!closed_||failed_)throw std::logic_error("Native calls require a closed complete phase");return calls_;
}
void SourceExecutionInputs::observe_call(SourceExecutionCallKind kind,std::vector<std::byte> arguments){
    if(replay_){
        if(failed_||call_cursor_==calls_.size()||calls_[call_cursor_].kind!=kind||calls_[call_cursor_].arguments!=arguments||calls_[call_cursor_].input_begin!=cursor_){
            failed_=true;throw std::runtime_error("Native source call order or arguments differ from the producer");}
        ++call_cursor_;return;
    }
    if(kind<SourceExecutionCallKind::MatchStep||kind>source_execution_call_kind_last){failed_=true;return;}
    if(calls_.size()==maximum_calls||arguments.size()>maximum_call_bytes-call_bytes_){failed_=true;return;}
    call_bytes_+=arguments.size();calls_.push_back({kind,std::move(arguments),std::uint32_t(entries_.size()),std::uint32_t(entries_.size())});
}
void SourceExecutionInputs::observe_result(std::size_t call,std::vector<std::byte> result){
    if(replay_){
        if(failed_||call>=calls_.size()||calls_[call].result!=result){
            failed_=true;throw std::runtime_error("Actual native call result differs from the producer");}
    }else if(!failed_){
        if(call>=calls_.size()||result.size()>maximum_call_bytes-call_bytes_){failed_=true;return;}
        call_bytes_+=result.size();calls_[call].result=std::move(result);
    }
}
void SourceExecutionInputs::untracked(std::uint32_t detail){
    require_executor();if(!track_calls_||(call_depth_&&!observer_depth_))return;
    failed_=true;if(detail)untracked_detail_=detail;
    if(replay_)throw std::runtime_error("Unrecorded native work during source replay");
}
void SourceExecutionInputs::invalidate_recording(){
    begin_sample();failed_=true;
    if(replay_)throw std::runtime_error("Native call recording instrumentation failed during replay");
}
SourceExecutionCallScope::~SourceExecutionCallScope(){
    if(!inputs_)return;
    if(std::this_thread::get_id()!=inputs_->executor_)std::terminate();
    if(root_){const auto after=inputs_->counter_values();for(unsigned i=0;i<2;++i)inputs_->known_mutations_[i]+=after[i]-before_[i];}
    if(root_&&call_index_!=std::size_t(-1)){
        if(result_required_&&!completed_)inputs_->failed_=true;
        if(inputs_->replay_){if(inputs_->cursor_!=inputs_->calls_[call_index_].input_end)inputs_->failed_=true;}
        else inputs_->calls_[call_index_].input_end=std::uint32_t(inputs_->entries_.size());
    }
    --inputs_->call_depth_;
}
SourceExecutionObserverScope::SourceExecutionObserverScope(const SourceRuntimeMemory &memory){
    inputs_=source_execution_inputs(memory);
    if(!inputs_||!inputs_->track_calls_){inputs_=nullptr;return;}
    inputs_->begin_sample();before_=inputs_->counter_values();++inputs_->observer_depth_;
}
SourceExecutionObserverScope::~SourceExecutionObserverScope(){
    if(!inputs_)return;
    if(std::this_thread::get_id()!=inputs_->executor_)std::terminate();
    if(inputs_->counter_values()!=before_)inputs_->failed_=true;
    --inputs_->observer_depth_;
}
SourceExecutionFailedCleanupScope::SourceExecutionFailedCleanupScope(const SourceRuntimeMemory &memory) {
    inputs_=source_execution_inputs(memory);if(!inputs_ || !inputs_->track_calls_){inputs_=nullptr;return;}
    inputs_->require_executor();
    if(!inputs_->failed_ && !std::uncaught_exceptions())
        throw std::logic_error("Native cleanup admission requires a failed or unwinding phase");
    inputs_->failed_=true;++inputs_->call_depth_;
}
SourceExecutionFailedCleanupScope::~SourceExecutionFailedCleanupScope() {
    if(inputs_){if(std::this_thread::get_id()!=inputs_->executor_)std::terminate();--inputs_->call_depth_;}
}
void SourceExecutionInputs::require_executor()const{
    if(std::this_thread::get_id()!=executor_)throw std::logic_error("Native execution inputs belong to another executor");
}
void SourceExecutionInputs::begin_sample()const {
    require_executor();if(closed_)throw std::logic_error("Native execution input transcript is closed");
}
std::uint64_t SourceExecutionInputs::replay_sample(SourceExecutionInputKind kind,std::uint32_t operand) {
    if(failed_||cursor_==entries_.size()||entries_[cursor_].kind!=kind||entries_[cursor_].operand!=operand){
        failed_=true;throw std::runtime_error("Native execution input order or operand differs from the producer");}
    return entries_[cursor_++].value;
}
void SourceExecutionInputs::record_sample(SourceExecutionInputKind kind,std::uint32_t operand,std::uint64_t value)noexcept {
    if(!failed_){
        if(entries_.size()==limit_||!valid(kind,operand,value))failed_=true;
        else try{entries_.push_back({kind,operand,value});}catch(...){failed_=true;}
    }
}
std::span<const SourceExecutionInput> SourceExecutionInputs::finish(){
    require_executor();if(closed_)throw std::logic_error("Native execution input transcript already closed");
    if(bound_memory_)throw std::logic_error("Native execution input phase is still bound");
    closed_=true;
    if(track_calls_){
        const auto counters=counter_values();
        for(unsigned i=0;i<2;++i)if(counters[i]-counter_start_[i]!=known_mutations_[i])failed_=true;
        if(call_depth_||observer_depth_||(replay_&&call_cursor_!=calls_.size()))failed_=true;
    }
    if(failed_||(replay_&&cursor_!=entries_.size()))
        throw std::runtime_error("Incomplete or failed native execution phase; untracked detail="+std::to_string(untracked_detail_));
    return entries_;
}
SourceExecutionInputBinding::SourceExecutionInputBinding(const SourceRuntimeMemory &memory,SourceExecutionInputs &inputs)
    :memory_(&memory),inputs_(&inputs){
    inputs.require_executor();if(inputs.bound_memory_||inputs.closed_)
        throw std::logic_error("Native execution input transcript is already bound or closed");
    // Retain admission even for an untracked tape, whose TLS association has
    // no write/topology callbacks. Publication must see foreign bindings too.
    policy_=std::make_unique<SourceRuntimePolicyChange>(memory);
    if(!bindings.emplace(&memory,&inputs).second)
        throw std::logic_error("Native execution inputs already bound to this memory owner");
    inputs.bound_memory_=&memory;
    if(inputs.track_calls_)try{
        source_runtime_register_write_observer(memory,this,[&inputs](auto address,auto){inputs.untracked(address);});
        try{source_runtime_register_topology_guard(memory,this,[&inputs](auto address,auto){inputs.untracked(address);});}
        catch(...){source_runtime_unregister_write_observer(memory,this);throw;}
        guards_=true;
    }catch(...){bindings.erase(&memory);inputs.bound_memory_=nullptr;throw;}
}
SourceExecutionInputBinding::~SourceExecutionInputBinding(){
    if(std::this_thread::get_id()!=executor_)std::terminate();
    if(guards_){source_runtime_unregister_topology_guard(*memory_,this);source_runtime_unregister_write_observer(*memory_,this);}
    bindings.erase(memory_);inputs_->bound_memory_=nullptr;
}
SourceExecutionInputs *source_execution_inputs(const SourceRuntimeMemory &memory)noexcept{
    const auto at=bindings.find(&memory);return at==bindings.end()?nullptr:at->second;
}
void source_execution_untracked(const SourceRuntimeMemory &memory,std::uint32_t detail){
    if(auto *inputs=source_execution_inputs(memory))inputs->untracked(detail);
}
}
