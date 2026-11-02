#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <stdexcept>
namespace halo2 {
class NativeTu15FramePolicyRootReturnRequired final:public std::runtime_error {
    std::uint32_t actual_;
public:
    explicit NativeTu15FramePolicyRootReturnRequired(std::uint32_t actual):
        std::runtime_error("Native frame policy requires the prepared source-call root continuation"),actual_(actual){}
    std::uint32_t actual_return()const noexcept{return actual_;}
};
// Native renderer public-call domain, separate from literal229B9E. The
// caller supplies EAX=view count and discards volatile GP/flags afterwards.
// Its real prepared-source-call RET is authenticated after semantic stores.
void native_tu15_camera_frame_policy_root_run(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_tu15_camera_frame_policy_root_calls();
}
