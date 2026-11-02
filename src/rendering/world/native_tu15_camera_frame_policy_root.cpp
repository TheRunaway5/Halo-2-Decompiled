#include "halo2/native_tu15_camera_frame_policy_root.hpp"
#include "halo2/native_tu15_camera_frame_policy.hpp"
#include <atomic>
namespace halo2 {
namespace {std::atomic<std::uint64_t> calls{};}
void native_tu15_camera_frame_policy_root_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    calls.fetch_add(1,std::memory_order_relaxed);
    native_tu15_camera_frame_policy(memory,abi.gp[0],observe);
    std::array<std::byte,4> bytes{};memory.read(abi.gp[4],bytes);std::uint32_t target{};
    for(unsigned n=0;n<4;++n)target|=std::to_integer<std::uint32_t>(bytes[n])<<(8u*n);
    if(target!=0x60300000u)throw NativeTu15FramePolicyRootReturnRequired(target);
    abi.gp[4]+=4u;
}
std::uint64_t native_tu15_camera_frame_policy_root_calls(){return calls.load(std::memory_order_relaxed);}
}
