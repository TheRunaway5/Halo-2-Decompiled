#include "halo2/native_tu15_contact_features_handoff.hpp"
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>
namespace halo2 {
namespace {
std::mutex bindings_mutex;
std::map<const SourcePhysicsContactMemory*,std::vector<NativeTu15ContactFeaturesHandoffBinding*>>bindings;
void require(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t address){std::array<std::byte,4>bytes{};memory.read(address,bytes);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;}
bool supported(std::uint32_t owner){return owner==0x2e5e20u||owner==0x3079c0u||owner==0x3067d0u||owner==0x3070f0u||owner==0x1f6690u||owner==0x2e8d40u;}
std::string context(const SourcePhysicsContactRuntimeCall&call,const SourcePhysicsContactRuntimeAbi&state){std::ostringstream out;out<<"Native TU1.5 contact feature handoff source 0x"<<std::hex<<call.instruction<<" target 0x"<<call.target<<" shape 0x"<<state.gp[1];return out.str();}
}
NativeTu15ContactFeaturesHandoffBinding::NativeTu15ContactFeaturesHandoffBinding(
 SourcePhysicsContactMemory&memory,NativeTu15ContactFeaturesHandoffPolicy policy,
 NativeTu15ContactFeaturesHandoffActivity*activity)
 :memory_(&memory),policy_(std::move(policy)),activity_(activity),executor_(std::this_thread::get_id()){
    require(policy_.stack_first<policy_.stack_end,
        "Native TU1.5 contact features require an actual stack owner");
    std::lock_guard lock(bindings_mutex);auto&stack=bindings[memory_];
    require(stack.empty()||stack.back()->executor_==executor_,
        "Native TU1.5 contact features memory already belongs to another executor");
    stack.push_back(this);
}
NativeTu15ContactFeaturesHandoffBinding::~NativeTu15ContactFeaturesHandoffBinding(){
    std::lock_guard lock(bindings_mutex);const auto found=bindings.find(memory_);
    if(executor_!=std::this_thread::get_id()||found==bindings.end()||found->second.empty()||found->second.back()!=this)std::terminate();
    found->second.pop_back();if(found->second.empty())bindings.erase(found);
}
bool native_tu15_handoff_contact_features(SourcePhysicsContactMemory&memory,
 const SourcePhysicsContactRuntimeCall&call,SourcePhysicsContactRuntimeAbi&state,
 const SourcePhysicsContactRecordObserver&observer){
    NativeTu15ContactFeaturesHandoffPolicy policy;NativeTu15ContactFeaturesHandoffActivity*activity{};
    {
        std::lock_guard lock(bindings_mutex);const auto found=bindings.find(&memory);
        if(found==bindings.end()||found->second.empty())return false;
        const auto*binding=found->second.back();activity=binding->activity_;
        if(activity)++activity->attempts;
        if(binding->executor_!=std::this_thread::get_id()){
            if(activity)++activity->refusals;
            throw std::runtime_error("Native TU1.5 contact features require their actual executor");
        }
        policy=binding->policy_;
    }
    try{
        require(!call.tail&&(call.instruction==0x30e96du||call.instruction==0x30e985u),
            "Native TU1.5 contact features require a qualified virtual34 source call");
        if(!supported(call.target))throw std::runtime_error(context(call,state)+": unsupported authored owner");
        require(!(state.eflags&0x400u),
            "Native TU1.5 contact features require the normal clear-DF call ABI");
        require(policy.observers_read_only&&
            (!policy.first_trace_active||!*policy.first_trace_active)&&
            (!policy.second_trace_active||!*policy.second_trace_active),
            "Native TU1.5 contact features require read-only observers without active instruction traces");
        const auto sp=state.gp[4];
        require(sp>=policy.stack_first&&std::uint64_t(sp)+16u<=policy.stack_end,
            "Native TU1.5 contact features arguments escape their actual stack owner");
        const auto expected_return=call.instruction==0x30e96du?0x30e970u:0x30e988u;
        require(word(memory,sp)==expected_return,
            "Native TU1.5 contact features require their actual return packet");
        require(state.floating_point.has_value()&&!state.active_mmx&&state.x87_depth<=7,
            "Native TU1.5 contact features require their retained numerical owner");
        const auto ids=word(memory,sp+4u),count=word(memory,sp+8u),output=word(memory,sp+12u);
        NativeTu15PhysicsFrameFp fp{state.floating_point->x87_control_word,state.x87_status,state.floating_point->mxcsr};
        const auto publish=[&]{state.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};state.x87_status=fp.status;};
        const SourcePhysicsContactRecordObserver observed=[&](const auto&event){
            if(policy.instruction)policy.instruction(event.instruction);
            if(observer)observer(event);
        };
        try{native_tu15_contact_shape_features(memory,{state.gp[1]},{call.target},ids,count,output,fp,state.x87_depth,observed);}
        catch(const std::exception&error){publish();throw std::runtime_error(context(call,state)+": "+error.what());}
        catch(...){publish();throw;}
        publish();state.gp[4]=sp+16u;
    }catch(...){std::lock_guard lock(bindings_mutex);if(activity)++activity->refusals;throw;}
    {
        std::lock_guard lock(bindings_mutex);if(activity){++activity->completed;
            if(call.instruction==0x30e96du)++activity->first_shape_calls;else ++activity->second_shape_calls;
            switch(call.target){case 0x2e5e20u:++activity->swept_calls;break;case 0x3079c0u:++activity->soa_calls;break;case 0x3067d0u:++activity->box_calls;break;case 0x3070f0u:++activity->capsule_calls;break;case 0x1f6690u:++activity->polygon_calls;break;case 0x2e8d40u:++activity->sphere_calls;break;}
        }
    }
    return true;
}
} // namespace halo2
