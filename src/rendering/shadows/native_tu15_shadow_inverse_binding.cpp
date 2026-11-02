#include "halo2/native_tu15_shadow_inverse_binding.hpp"
#include "halo2/native_tu15_camera_projection.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {std::atomic<std::uint64_t> calls{};}
bool native_tu15_shadow_inverse_owns_call(const SourceShadowProfileDirect &profile) noexcept {
    return profile.image==SourcePhysicsContactImage::TitleUpdate15 &&
        profile.call.target==0x144090u && profile.call.instruction==0xc4879u && !profile.call.tail;
}
void native_tu15_shadow_inverse_run(SourcePhysicsContactMemory &memory,
    const SourceShadowProfileDirect &profile,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!native_tu15_shadow_inverse_owns_call(profile))
        throw std::invalid_argument("Unqualified native shadow inverse source caller");
    const auto destination=abi.gp[0],source=abi.gp[1];
    const auto record_begin=destination>=4u?std::uint64_t(destination)-4u:0u;
    const auto record_end=record_begin+0x1bcu;
    const auto stack_begin=abi.gp[4]>=4u?std::uint64_t(abi.gp[4])-4u:0u;
    const auto stack_end=std::uint64_t(abi.gp[4])+4u;
    const bool disjoint=record_end<=stack_begin || stack_end<=record_begin;
    if(std::uint64_t(destination)+0x34u!=source || destination<4u ||
       abi.gp[4]<4u || record_end>0x100000000ull || stack_end>0x100000000ull || !disjoint ||
       !abi.floating_point || (abi.eflags&0x400u) || abi.active_mmx || abi.x87_depth>8)
        throw std::invalid_argument("Native shadow inverse requires its ordinary adjacent-frame caller");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    calls.fetch_add(1,std::memory_order_relaxed);
    try {native_tu15_camera_inverse_frame(memory,source,destination,fp,observe);}
    catch(...) {abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};throw;}
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
    std::array<std::byte,4> real_return{};memory.read(abi.gp[4],real_return);std::uint32_t actual_return{};
    for(unsigned n=0;n<4;++n)actual_return|=std::to_integer<std::uint32_t>(real_return[n])<<(8u*n);
    if(actual_return!=0xc487eu)throw NativeTu15ShadowInverseReturnRequired(actual_return);
    abi.gp[4]+=4u;
}
std::uint64_t native_tu15_shadow_inverse_calls(){return calls.load(std::memory_order_relaxed);}
}
