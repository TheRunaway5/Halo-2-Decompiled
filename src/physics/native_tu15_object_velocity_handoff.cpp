#include "halo2/native_tu15_object_velocity_handoff.hpp"
#include <cstring>
#include <map>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>
namespace halo2 {
namespace {
std::mutex bindings_mutex;
std::map<const SourcePhysicsContactMemory*,std::vector<NativeTu15ObjectVelocityHandoffBinding*>>bindings;
void require(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t address){std::array<std::byte,4>bytes{};memory.read(address,bytes);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;}
}
NativeTu15ObjectVelocityHandoffBinding::NativeTu15ObjectVelocityHandoffBinding(
 SourcePhysicsContactMemory&memory,NativeTu15ObjectVelocityHandoff callback,
 NativeTu15ObjectVelocityHandoffActivity*activity)
 :memory_(&memory),callback_(std::move(callback)),activity_(activity),executor_(std::this_thread::get_id()){
    if(!callback_)throw std::invalid_argument("Native TU1.5 object velocity handoff requires an actual owner");
    std::lock_guard lock(bindings_mutex);auto&stack=bindings[memory_];
    if(!stack.empty()&&stack.back()->executor_!=executor_)
        throw std::runtime_error("Native TU1.5 object velocity memory owner already belongs to another executor");
    stack.push_back(this);
}
NativeTu15ObjectVelocityHandoffBinding::~NativeTu15ObjectVelocityHandoffBinding(){
    std::lock_guard lock(bindings_mutex);const auto found=bindings.find(memory_);
    if(executor_!=std::this_thread::get_id()||found==bindings.end()||found->second.empty()||found->second.back()!=this)
        std::terminate();
    found->second.pop_back();if(found->second.empty())bindings.erase(found);
}
bool native_tu15_handoff_object_velocity(SourcePhysicsContactMemory&memory,
 const SourcePhysicsContactRuntimeCall&call,SourcePhysicsContactRuntimeAbi&state,
 const SourcePhysicsContactRecordObserver&observer){
    NativeTu15ObjectVelocityHandoff callback;NativeTu15ObjectVelocityHandoffActivity*activity{};
    {
        std::lock_guard lock(bindings_mutex);const auto found=bindings.find(&memory);
        if(found==bindings.end()||found->second.empty())return false;
        const auto*binding=found->second.back();activity=binding->activity_;
        if(activity)++activity->attempts;
        if(binding->executor_!=std::this_thread::get_id()){
            if(activity)++activity->refusals;
            throw std::runtime_error("Native TU1.5 object velocity handoff requires its actual executor");
        }
        callback=binding->callback_;
    }
    try{
        require(call.instruction==0xf2091u&&call.target==0xb9800u&&!call.tail,
            "Native TU1.5 object velocity handoff requires the qualified F2091 source call");
        require(state.floating_point.has_value()&&!state.active_mmx&&state.x87_depth<=5,
            "Native TU1.5 object velocity handoff requires its retained numerical context");
        require(state.gp[4]<=0xfffffff7u&&word(memory,state.gp[4])==0xf2096u,
            "Native TU1.5 object velocity handoff requires the actual F2096 return packet");
        const auto incoming=state;
        callback(memory,call,state,observer);
        require(state.gp[4]==incoming.gp[4]+8u,
            "Native TU1.5 object velocity handoff requires actual RET4 completion");
        require(state.floating_point.has_value()&&!state.active_mmx&&state.x87_depth==incoming.x87_depth,
            "Native TU1.5 object velocity handoff changed its preserved numerical owner");
        for(const auto index:{3u,5u,6u,7u})require(state.gp[index]==incoming.gp[index],
            "Native TU1.5 object velocity handoff changed a preserved caller register");
        for(unsigned n=0;n<incoming.x87_depth;++n)require(std::memcmp(&incoming.x87[n],&state.x87[n],10)==0,
            "Native TU1.5 object velocity handoff changed a preserved live x87 value");
    }catch(...){std::lock_guard lock(bindings_mutex);if(activity)++activity->refusals;throw;}
    {std::lock_guard lock(bindings_mutex);if(activity)++activity->completed;}
    return true;
}
} // namespace halo2
