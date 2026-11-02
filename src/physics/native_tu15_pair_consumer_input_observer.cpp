#include "halo2/native_tu15_pair_consumer_input_observer.hpp"
#include <map>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>
namespace halo2 {
namespace {
std::mutex mutex;
std::map<const SourcePhysicsContactMemory*,std::vector<NativeTu15PairConsumerInputObserverBinding*>>bindings;
}
NativeTu15PairConsumerInputObserverBinding::NativeTu15PairConsumerInputObserverBinding(
 const SourcePhysicsContactMemory&memory,NativeTu15PairConsumerInputObserver observer,
 NativeTu15PairConsumerInputObserverActivity*activity)
 :memory_(&memory),observer_(std::move(observer)),activity_(activity),executor_(std::this_thread::get_id()){
    if(!observer_)throw std::invalid_argument("Native TU1.5 pair consumer input observation requires an observer");
    std::lock_guard lock(mutex);auto&stack=bindings[memory_];
    if(!stack.empty()&&stack.back()->executor_!=executor_)
        throw std::runtime_error("Native TU1.5 pair consumer input memory belongs to another executor");
    stack.push_back(this);
}
NativeTu15PairConsumerInputObserverBinding::~NativeTu15PairConsumerInputObserverBinding(){
    std::lock_guard lock(mutex);const auto found=bindings.find(memory_);
    if(executor_!=std::this_thread::get_id()||found==bindings.end()||found->second.empty()||found->second.back()!=this)std::terminate();
    found->second.pop_back();if(found->second.empty())bindings.erase(found);
}
bool native_tu15_observe_pair_consumer_input(const SourcePhysicsContactMemory&memory,const NativeTu15PairConsumerInput&input){
    NativeTu15PairConsumerInputObserver observer;NativeTu15PairConsumerInputObserverActivity*activity{};
    {
        std::lock_guard lock(mutex);const auto found=bindings.find(&memory);
        if(found==bindings.end()||found->second.empty())return false;
        const auto*binding=found->second.back();activity=binding->activity_;if(activity)++activity->attempts;
        if(binding->executor_!=std::this_thread::get_id()){
            if(activity)++activity->failures;
            throw std::runtime_error("Native TU1.5 pair consumer input observation requires its actual executor");
        }
        observer=binding->observer_;
    }
    try{observer(memory,input);}catch(...){std::lock_guard lock(mutex);if(activity)++activity->failures;throw;}
    {std::lock_guard lock(mutex);if(activity)++activity->completed;}
    return true;
}
} // namespace halo2
