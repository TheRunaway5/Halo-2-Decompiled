#include "halo2/native_tu15_camera_frame_policy_binding.hpp"
#include "halo2/native_tu15_camera_frame_policy.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {std::atomic<std::uint64_t> calls{};}
bool native_tu15_camera_frame_policy_owns_entry(std::uint32_t entry){return entry==0x2c470u||entry==0x25940u;}
void native_tu15_camera_frame_policy_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!((entry==0x2c470u&&caller==0x229b9eu)||(entry==0x25940u&&
        (caller==0x14d67u||caller==0x14d9au||caller==0x198a8u))))
        throw std::invalid_argument("Unqualified native camera frame-policy entry/caller");
    calls.fetch_add(1,std::memory_order_relaxed);
    if(entry==0x2c470u)native_tu15_camera_frame_policy(memory,abi.gp[0],observe);
    else abi.gp[0]=native_tu15_camera_target_query(memory);
    std::array<std::byte,4> return_word{};memory.read(abi.gp[4],return_word);abi.gp[4]+=4u;
}
std::uint64_t native_tu15_camera_frame_policy_calls(){return calls.load(std::memory_order_relaxed);}
}
