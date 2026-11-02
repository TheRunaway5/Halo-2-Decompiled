#include "halo2/native_campaign_view_rectangle_tu15_binding.hpp"
#include "halo2/native_campaign_view_rectangle_tu15.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {std::atomic<std::uint64_t> calls{};}
bool native_campaign_view_rectangle_owns_entry_tu15(std::uint32_t entry){return entry==0x2f950u;}
void native_campaign_view_rectangle_run_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_view_rectangle_owns_entry_tu15(entry) || (caller!=0x2b8b4u && caller!=0x13c8au))
        throw std::invalid_argument("Unqualified native view rectangle entry/caller");
    if(!abi.floating_point || (abi.eflags&0x400u))
        throw std::invalid_argument("Native view rectangle needs its actual caller FP/direction policy");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    calls.fetch_add(1,std::memory_order_relaxed);
    try{native_campaign_view_rectangle_tu15(memory,abi.gp[0],abi.gp[6],fp,observe);}
    catch(...){abi.floating_point->mxcsr=fp.mxcsr;throw;}
    abi.floating_point->mxcsr=fp.mxcsr;
    std::array<std::byte,4> return_word{};memory.read(abi.gp[4],return_word);abi.gp[4]+=4u;
}
std::uint64_t native_campaign_view_rectangle_calls_tu15(){return calls.load(std::memory_order_relaxed);}
}
