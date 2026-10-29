#include "halo2/native_campaign_camera_reset_tu15_binding.hpp"
#include "halo2/native_campaign_camera_reset_tu15.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {std::atomic<std::uint64_t> calls{};}
bool native_campaign_camera_reset_tu15_owns_entry(std::uint32_t entry){return entry==0x157eb0u;}
void native_campaign_camera_reset_tu15_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_camera_reset_tu15_owns_entry(entry))throw std::invalid_argument("Unowned native camera reset entry");
    if(abi.eflags&0x400u)throw std::invalid_argument("Native camera reset requires the ordinary clear-direction caller");
    calls.fetch_add(1,std::memory_order_relaxed);native_campaign_camera_director_reset_tu15(memory,observe);
    std::array<std::byte,4> returned;memory.read(abi.gp[4],returned);abi.gp[4]+=4u;
}
std::uint64_t native_campaign_camera_reset_calls_tu15(){return calls.load(std::memory_order_relaxed);}
}
