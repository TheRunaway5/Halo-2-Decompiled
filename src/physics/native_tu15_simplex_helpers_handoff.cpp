#include "halo2/native_tu15_simplex_helpers_handoff.hpp"
#include <map>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>
namespace halo2 {
namespace {
std::mutex bindings_mutex;
std::map<const SourcePhysicsContactMemory*,std::vector<NativeTu15SimplexHelpersHandoffBinding*>>bindings;
void require(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t address){std::array<std::byte,4>bytes{};memory.read(address,bytes);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;}
bool triangle_call(std::uint32_t pc){return pc==0x30dad9u||pc==0x30e3b6u||pc==0x30eb85u||pc==0x30ece2u;}
bool segment_call(std::uint32_t pc){return pc==0x30eaccu;}
}
NativeTu15SimplexHelpersHandoffBinding::NativeTu15SimplexHelpersHandoffBinding(
 SourcePhysicsContactMemory&memory,NativeTu15SimplexHelpersHandoffPolicy policy,
 NativeTu15SimplexHelpersHandoffActivity*activity)
 :memory_(&memory),policy_(std::move(policy)),activity_(activity),executor_(std::this_thread::get_id()){
    require(policy_.stack_first<policy_.stack_end,
        "Native TU1.5 simplex helpers require an actual stack owner");
    std::lock_guard lock(bindings_mutex);auto&stack=bindings[memory_];
    require(stack.empty()||stack.back()->executor_==executor_,
        "Native TU1.5 simplex helpers memory already belongs to another executor");
    stack.push_back(this);
}
NativeTu15SimplexHelpersHandoffBinding::~NativeTu15SimplexHelpersHandoffBinding(){
    std::lock_guard lock(bindings_mutex);const auto found=bindings.find(memory_);
    if(executor_!=std::this_thread::get_id()||found==bindings.end()||found->second.empty()||found->second.back()!=this)std::terminate();
    found->second.pop_back();if(found->second.empty())bindings.erase(found);
}
bool native_tu15_handoff_simplex_helpers(SourcePhysicsContactMemory&memory,
 const SourcePhysicsContactRuntimeCall&call,SourcePhysicsContactRuntimeAbi&state,
 const SourcePhysicsContactRecordObserver&observer){
    NativeTu15SimplexHelpersHandoffPolicy policy;NativeTu15SimplexHelpersHandoffActivity*activity{};
    {
        std::lock_guard lock(bindings_mutex);const auto found=bindings.find(&memory);
        if(found==bindings.end()||found->second.empty())return false;
        const auto*binding=found->second.back();activity=binding->activity_;
        if(activity)++activity->attempts;
        if(binding->executor_!=std::this_thread::get_id()){
            if(activity)++activity->refusals;
            throw std::runtime_error("Native TU1.5 simplex helpers require their actual executor");
        }
        policy=binding->policy_;
    }
    try{
        const bool triangle=triangle_call(call.instruction),segment=segment_call(call.instruction);
        require(!call.tail&&(triangle||segment),
            "Native TU1.5 simplex helpers require a qualified source call");
        require(call.target==(triangle?0x30d3f0u:0x312620u),
            "Native TU1.5 simplex helpers require the authenticated typed owner");
        require(policy.observers_read_only&&
            (!policy.first_trace_active||!*policy.first_trace_active)&&
            (!policy.second_trace_active||!*policy.second_trace_active),
            "Native TU1.5 simplex helpers require read-only observers without active instruction traces");
        const auto sp=state.gp[4];const unsigned argument_bytes=triangle?16u:24u;
        require(sp>=policy.stack_first&&std::uint64_t(sp)+argument_bytes<=policy.stack_end,
            "Native TU1.5 simplex helper arguments escape their actual stack owner");
        require(word(memory,sp)==call.instruction+5u,
            "Native TU1.5 simplex helpers require their actual return packet");
        require(state.floating_point.has_value()&&!state.active_mmx&&state.x87_depth<=(triangle?7u:6u),
            "Native TU1.5 simplex helpers require their retained numerical owner");
        std::array<std::uint32_t,5>args{};for(unsigned n=0;n<(triangle?3u:5u);++n)args[n]=word(memory,sp+4u+n*4u);
        NativeTu15ClosestSegmentsScratch scalars{};
        if(segment){
            require(sp>=0x34u,"Native TU1.5 segment scalar DATA underflows its owner");
            scalars.identity=((sp-4u)&~15u)-0x30u;
            require(scalars.identity>=policy.stack_first&&std::uint64_t(scalars.identity)+0x30u<=policy.stack_end,
                "Native TU1.5 segment scalar DATA escapes its actual stack owner");
        }
        NativeTu15PhysicsFrameFp fp{state.floating_point->x87_control_word,state.x87_status,state.floating_point->mxcsr};
        const auto publish=[&]{state.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};state.x87_status=fp.status;};
        const SourcePhysicsContactRecordObserver observed=[&](const auto&event){
            if(policy.instruction)policy.instruction(event.instruction);
            if(observer)observer(event);
        };
        std::uint32_t result{};
        try{result=triangle?native_tu15_orient_simplex_triangle(memory,state.gp[1],args[0],args[1],args[2],fp,state.x87_depth,observed):native_tu15_closest_segment_pair(memory,args[0],args[1],args[2],args[3],args[4],scalars,fp,state.x87_depth,observed);}
        catch(...){publish();throw;}
        publish();state.gp[0]=result;state.gp[4]=sp+(triangle?16u:4u);
    }catch(...){std::lock_guard lock(bindings_mutex);if(activity)++activity->refusals;throw;}
    {
        std::lock_guard lock(bindings_mutex);if(activity){++activity->completed;
            if(triangle_call(call.instruction))++activity->triangle_calls;else ++activity->segment_calls;
            switch(call.instruction){case 0x30dad9u:++activity->triangle_30dad9_calls;break;case 0x30e3b6u:++activity->triangle_30e3b6_calls;break;case 0x30eb85u:++activity->triangle_30eb85_calls;break;case 0x30ece2u:++activity->triangle_30ece2_calls;break;case 0x30eaccu:++activity->segment_30eacc_calls;break;}
        }
    }
    return true;
}
} // namespace halo2
