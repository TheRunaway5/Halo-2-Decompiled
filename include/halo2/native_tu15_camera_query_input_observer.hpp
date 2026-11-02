#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>
namespace halo2 {
// Actual16C620 geometric-query INPUT before its current native refusal.
// No GP/SIMD/stack image, schema rows, after-values or results are captured.
struct NativeTu15CameraQueryInput {
    std::uint32_t owner{},mask{},origin{},displacement{},ignore_first{},ignore_second{},result{};
    std::uint32_t source_call_site{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
};
using NativeTu15CameraQueryInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15CameraQueryInput&)>;
class NativeTu15CameraQueryInputObserverBinding;
namespace native_camera_query_input_detail {
inline std::mutex&mutex(){static std::mutex value;return value;}
inline std::vector<NativeTu15CameraQueryInputObserverBinding*>&bindings(){
    static std::vector<NativeTu15CameraQueryInputObserverBinding*> value;return value;
}
}
class NativeTu15CameraQueryInputObserverBinding {
    const SourcePhysicsContactMemory*memory_;
    NativeTu15CameraQueryInputObserver observer_;
    std::thread::id executor_;
    friend bool native_tu15_observe_camera_query_input(
        const SourcePhysicsContactMemory&,const NativeTu15CameraQueryInput&);
public:
    NativeTu15CameraQueryInputObserverBinding(const SourcePhysicsContactMemory&memory,
        NativeTu15CameraQueryInputObserver observer):memory_(&memory),
        observer_(std::move(observer)),executor_(std::this_thread::get_id()){
        std::lock_guard lock(native_camera_query_input_detail::mutex());
        native_camera_query_input_detail::bindings().push_back(this);
    }
    ~NativeTu15CameraQueryInputObserverBinding(){
        std::lock_guard lock(native_camera_query_input_detail::mutex());
        auto&bindings=native_camera_query_input_detail::bindings();
        const auto found=std::find(bindings.begin(),bindings.end(),this);
        if(found!=bindings.end())bindings.erase(found);
    }
    NativeTu15CameraQueryInputObserverBinding(const NativeTu15CameraQueryInputObserverBinding&)=delete;
    NativeTu15CameraQueryInputObserverBinding&operator=(const NativeTu15CameraQueryInputObserverBinding&)=delete;
};
inline bool native_tu15_observe_camera_query_input(const SourcePhysicsContactMemory&memory,
    const NativeTu15CameraQueryInput&input){
    NativeTu15CameraQueryInputObserver observer;
    {
        std::lock_guard lock(native_camera_query_input_detail::mutex());
        const auto&bindings=native_camera_query_input_detail::bindings();
        for(auto i=bindings.rbegin();i!=bindings.rend();++i)
            if((*i)->memory_==&memory&&(*i)->executor_==std::this_thread::get_id()){
                observer=(*i)->observer_;break;
            }
    }
    if(!observer)return false;
    observer(memory,input);return true;
}
} // namespace halo2
