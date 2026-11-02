#include "halo2/native_campaign_screen_flash_sample_tu15_binding.hpp"
#include "halo2/native_campaign_screen_flash_sample_tu15.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {std::atomic<std::uint64_t> calls{};}
bool native_campaign_screen_flash_sample_tu15_owns_entry(std::uint32_t entry){return entry==0x156e40u;}
void native_campaign_screen_flash_sample_tu15_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_screen_flash_sample_tu15_owns_entry(entry) || caller!=0x2b8ecu)
        throw std::invalid_argument("Unqualified native screen flash entry/caller");
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>5)
        throw std::invalid_argument("Native screen flash requires retained FP policy and free private hardware operands");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    struct Publication {
        SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
        ~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}
    } publication{abi,fp};
    calls.fetch_add(1,std::memory_order_relaxed);
    native_campaign_screen_flash_sample_tu15(memory,abi.gp[0],abi.gp[6],fp,observe);
    std::array<std::byte,4> returned;memory.read(abi.gp[4],returned);abi.gp[4]+=4u;
}
std::uint64_t native_campaign_screen_flash_sample_calls_tu15(){return calls.load(std::memory_order_relaxed);}
}
