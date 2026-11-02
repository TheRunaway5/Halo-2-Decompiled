#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <stdexcept>
namespace halo2 {
// Actual dispatcher provenance, never a substitute for source data or a
// captured result. Only TU1.5 C3DC0's C4879 CALL is admitted by this profile.
struct SourceShadowProfileDirect {
    SourcePhysicsContactImage image;
    SourcePhysicsContactRuntimeCall call;
};
class NativeTu15ShadowInverseReturnRequired final:public std::runtime_error {
    std::uint32_t actual_;
public:
    explicit NativeTu15ShadowInverseReturnRequired(std::uint32_t actual):
        std::runtime_error("Native shadow inverse requires its actual C487E continuation"),actual_(actual){}
    std::uint32_t actual_return()const noexcept{return actual_;}
};
bool native_tu15_shadow_inverse_owns_call(const SourceShadowProfileDirect &) noexcept;
// EAX=record+4 destination, ECX=record+38 source; ordinary DF=0, real FP
// policy. The inverse is a typed dependency, not a new instruction body.
// The complete retained record is disjoint from the active save/RET span.
// Volatile GP/SIMD/flags remain opaque. Real RET is read/authenticated last;
// an unknown continuation keeps completed data/FP and does not advance SP.
void native_tu15_shadow_inverse_run(SourcePhysicsContactMemory &,
    const SourceShadowProfileDirect &,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_tu15_shadow_inverse_calls();
}
