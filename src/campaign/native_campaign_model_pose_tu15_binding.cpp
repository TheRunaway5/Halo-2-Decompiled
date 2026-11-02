#include "halo2/native_campaign_model_pose_tu15_binding.hpp"
#include "halo2/native_campaign_model_pose_tu15.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {
std::atomic<std::uint64_t> calls{};
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t at) {
    std::array<std::byte,4> raw{};m.read(at,raw);std::uint32_t out{};
    for(unsigned n=0;n<4;++n)out|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return out;
}
}
bool native_campaign_model_pose_tu15_owns_entry(std::uint32_t entry){return entry==0x170820u;}
void native_campaign_model_pose_tu15_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_model_pose_tu15_owns_entry(entry) || caller!=0x3eaffu || !abi.floating_point)
        throw std::invalid_argument("Unqualified native model pose caller/FP policy");
    const auto sp=abi.gp[4];
    // Actual3EAFF has a valid disjoint three-word private caller frame.
    // These values are inputs; the body owns no source-addressed local matrix.
    const auto model=read(memory,sp+4u),destination=read(memory,sp+8u),poses=read(memory,sp+12u);
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    struct Publication {SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
        ~Publication(){abi.floating_point->mxcsr=fp.mxcsr;}} publication{abi,fp};
    calls.fetch_add(1,std::memory_order_relaxed);
    native_campaign_model_pose_tu15(memory,model,destination,poses,abi.gp[2],abi.gp[1],abi.gp[0],fp,observe);
    (void)read(memory,sp);abi.gp[4]+=16u;
}
std::uint64_t native_campaign_model_pose_calls_tu15(){return calls.load(std::memory_order_relaxed);}
}
