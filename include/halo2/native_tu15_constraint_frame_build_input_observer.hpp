#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>
namespace halo2 {
// Actual31D930 formals and the four currently published builder DATA words.
// No GP/SIMD/stack image, schema rows, after-values or results are captured.
struct NativeTu15ConstraintFrameBuildInput {
    std::uint32_t constraint{},jacobian_input{},builder_packet{};
    std::array<std::uint32_t,4> builder_words{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
using NativeTu15ConstraintFrameBuildInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15ConstraintFrameBuildInput&)>;
class NativeTu15ConstraintFrameBuildInputObserverBinding;
namespace native_constraint_frame_input_detail {
inline std::mutex&mutex(){static std::mutex value;return value;}
inline std::vector<NativeTu15ConstraintFrameBuildInputObserverBinding*>&bindings(){
    static std::vector<NativeTu15ConstraintFrameBuildInputObserverBinding*> value;return value;
}
}
class NativeTu15ConstraintFrameBuildInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;
    NativeTu15ConstraintFrameBuildInputObserver observer_;
    std::thread::id executor_;
    friend bool native_tu15_observe_constraint_frame_build_input(
        const SourcePhysicsContactMemory&,const NativeTu15ConstraintFrameBuildInput&);
public:
    NativeTu15ConstraintFrameBuildInputObserverBinding(const SourcePhysicsContactMemory&memory,
        NativeTu15ConstraintFrameBuildInputObserver observer):memory_(&memory),
        observer_(std::move(observer)),executor_(std::this_thread::get_id()){
        std::lock_guard lock(native_constraint_frame_input_detail::mutex());
        native_constraint_frame_input_detail::bindings().push_back(this);
    }
    ~NativeTu15ConstraintFrameBuildInputObserverBinding(){
        std::lock_guard lock(native_constraint_frame_input_detail::mutex());
        auto&bindings=native_constraint_frame_input_detail::bindings();
        const auto found=std::find(bindings.begin(),bindings.end(),this);
        if(found!=bindings.end())bindings.erase(found);
    }
    NativeTu15ConstraintFrameBuildInputObserverBinding(const NativeTu15ConstraintFrameBuildInputObserverBinding&)=delete;
    NativeTu15ConstraintFrameBuildInputObserverBinding&operator=(const NativeTu15ConstraintFrameBuildInputObserverBinding&)=delete;
};
inline bool native_tu15_observe_constraint_frame_build_input(const SourcePhysicsContactMemory&memory,
    const NativeTu15ConstraintFrameBuildInput&input){
    NativeTu15ConstraintFrameBuildInputObserver observer;
    {
        std::lock_guard lock(native_constraint_frame_input_detail::mutex());
        const auto&bindings=native_constraint_frame_input_detail::bindings();
        for(auto i=bindings.rbegin();i!=bindings.rend();++i)
            if((*i)->memory_==&memory&&(*i)->executor_==std::this_thread::get_id()){
                observer=(*i)->observer_;break;
            }
    }
    if(!observer)return false;
    observer(memory,input);return true;
}
} // namespace halo2
