#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>
namespace halo2 {
// Actual pool-release service INPUT before its current owner is invoked.
// No GP/SIMD/stack image, schema rows, after-values or results are captured.
struct NativeTu15PairPoolReleaseInput {
    std::uint32_t allocator{},owner{},data{},bytes{},tag{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
using NativeTu15PairPoolReleaseInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15PairPoolReleaseInput&)>;
class NativeTu15PairPoolReleaseInputObserverBinding;
namespace native_pair_pool_release_input_detail {
inline std::mutex&mutex(){static std::mutex value;return value;}
inline std::vector<NativeTu15PairPoolReleaseInputObserverBinding*>&bindings(){
    static std::vector<NativeTu15PairPoolReleaseInputObserverBinding*> value;return value;
}
}
class NativeTu15PairPoolReleaseInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;
    NativeTu15PairPoolReleaseInputObserver observer_;
    std::thread::id executor_;
    friend bool native_tu15_observe_pair_pool_release_input(
        const SourcePhysicsContactMemory&,const NativeTu15PairPoolReleaseInput&);
public:
    NativeTu15PairPoolReleaseInputObserverBinding(const SourcePhysicsContactMemory&memory,
        NativeTu15PairPoolReleaseInputObserver observer):memory_(&memory),
        observer_(std::move(observer)),executor_(std::this_thread::get_id()){
        std::lock_guard lock(native_pair_pool_release_input_detail::mutex());
        native_pair_pool_release_input_detail::bindings().push_back(this);
    }
    ~NativeTu15PairPoolReleaseInputObserverBinding(){
        std::lock_guard lock(native_pair_pool_release_input_detail::mutex());
        auto&bindings=native_pair_pool_release_input_detail::bindings();
        const auto found=std::find(bindings.begin(),bindings.end(),this);
        if(found!=bindings.end())bindings.erase(found);
    }
    NativeTu15PairPoolReleaseInputObserverBinding(const NativeTu15PairPoolReleaseInputObserverBinding&)=delete;
    NativeTu15PairPoolReleaseInputObserverBinding&operator=(const NativeTu15PairPoolReleaseInputObserverBinding&)=delete;
};
inline bool native_tu15_observe_pair_pool_release_input(const SourcePhysicsContactMemory&memory,
    const NativeTu15PairPoolReleaseInput&input){
    NativeTu15PairPoolReleaseInputObserver observer;
    {
        std::lock_guard lock(native_pair_pool_release_input_detail::mutex());
        const auto&bindings=native_pair_pool_release_input_detail::bindings();
        for(auto i=bindings.rbegin();i!=bindings.rend();++i)
            if((*i)->memory_==&memory&&(*i)->executor_==std::this_thread::get_id()){
                observer=(*i)->observer_;break;
            }
    }
    if(!observer)return false;
    observer(memory,input);return true;
}
} // namespace halo2
