#include "halo2/native_tu15_vehicle_flip_handoff.hpp"
#include <map>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>
namespace halo2 {
namespace {
std::mutex mutex;
std::map<const SourcePhysicsContactMemory*,std::vector<NativeTu15VehicleFlipHandoffBinding*>>bindings;
void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t out{};for(unsigned n=0;n<4;++n)out|=std::to_integer<unsigned>(b[n])<<(n*8u);return out;}
}
NativeTu15VehicleFlipHandoffBinding::NativeTu15VehicleFlipHandoffBinding(const SourcePhysicsContactMemory&memory,
 NativeTu15VehicleFlipHandoff callback,NativeTu15VehicleFlipHandoffPolicy policy,NativeTu15VehicleFlipHandoffActivity*activity)
 :memory_(&memory),callback_(std::move(callback)),policy_(policy),activity_(activity),executor_(std::this_thread::get_id()){
    require(bool(callback_),"Native TU1.5 vehicle flip handoff requires an actual native owner");require(policy.caller_stack_begin<policy.caller_stack_end,"Native TU1.5 vehicle flip requires its real caller stack bounds");
    std::lock_guard lock(mutex);auto&stack=bindings[memory_];require(stack.empty()||stack.back()->executor_==executor_,"Native TU1.5 vehicle flip memory belongs to another executor");stack.push_back(this);
}
NativeTu15VehicleFlipHandoffBinding::~NativeTu15VehicleFlipHandoffBinding(){std::lock_guard lock(mutex);const auto found=bindings.find(memory_);if(executor_!=std::this_thread::get_id()||found==bindings.end()||found->second.empty()||found->second.back()!=this)std::terminate();found->second.pop_back();if(found->second.empty())bindings.erase(found);}
bool native_tu15_handoff_vehicle_flip(SourcePhysicsContactMemory&memory,const SourcePhysicsContactRuntimeCall&call,
 SourcePhysicsContactRuntimeAbi&state,const SourcePhysicsContactRecordObserver&observer){
    NativeTu15VehicleFlipHandoff callback;NativeTu15VehicleFlipHandoffPolicy policy;NativeTu15VehicleFlipHandoffActivity*activity{};
    {
        std::lock_guard lock(mutex);const auto found=bindings.find(&memory);if(found==bindings.end()||found->second.empty())return false;const auto*binding=found->second.back();activity=binding->activity_;if(activity)++activity->attempts;
        if(binding->executor_!=std::this_thread::get_id()){if(activity)++activity->refusals;throw std::runtime_error("Native TU1.5 vehicle flip requires its actual executor");}callback=binding->callback_;policy=binding->policy_;
    }
    try{
        require(call.instruction==0xf24d8u&&call.target==0xf1dd0u&&!call.tail,"Native TU1.5 vehicle flip requires actual F24D8->F1DD0");
        require(!(policy.first_trace&&*policy.first_trace)&&!(policy.second_trace&&*policy.second_trace)&&(!observer||policy.observation_only_stores),"Native TU1.5 vehicle flip requires supported read-only observation");
        require(state.floating_point.has_value()&&!state.active_mmx&&state.x87_depth<=5&&!(state.eflags&0x400u),"Native TU1.5 vehicle flip requires its qualified retained numerical context");
        require(state.gp[4]>=policy.caller_stack_begin&&std::uint64_t(state.gp[4])+4u<=policy.caller_stack_end&&word(memory,state.gp[4])==0xf24ddu,"Native TU1.5 vehicle flip requires the actual F24DD return packet");
        const auto incoming=state;NativeTu15PhysicsFrameFp fp{state.floating_point->x87_control_word,state.x87_status,state.floating_point->mxcsr};
        bool result{};try{result=callback(incoming.gp[0],incoming.gp[4],incoming,fp,incoming.x87_depth,observer);}catch(...){state.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};state.x87_status=fp.status;throw;}
        require((fp.status&0x3800u)==(incoming.x87_status&0x3800u),"Native TU1.5 vehicle flip changed preserved x87 stack ownership");
        require((fp.control_word&0x3fu)==0x3fu&&!(fp.status&0x8080u)&&(fp.mxcsr&0x1f80u)==0x1f80u&&!(fp.mxcsr&0xffff0000u),"Native TU1.5 vehicle flip returned an unsupported numerical context");
        state.gp[0]=(incoming.gp[0]&0xffffff00u)|std::uint32_t(result);state.gp[4]=incoming.gp[4]+4u;
        state.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};state.x87_status=fp.status;
    }catch(...){std::lock_guard lock(mutex);if(activity)++activity->refusals;throw;}
    {std::lock_guard lock(mutex);if(activity)++activity->completed;}return true;
}
} // namespace halo2
