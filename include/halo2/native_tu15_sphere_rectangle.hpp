#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
struct NativeTu15SphereRectangleResidue {
    std::uint32_t eax{},ecx{};
    std::uint8_t x87_triples{};
};
// Whole30270: original pointer reads, four ordered output stores and guest
// CW/SW/MXCSR. The caller owns an empty x87 stack with masked exceptions.
// EAX/ECX residues support the sole2CD49 caller; XMM scratch is unconsumed there.
NativeTu15SphereRectangleResidue native_tu15_sphere_rectangle_30270(
    SourcePhysicsContactMemory&,std::uint32_t center,std::uint32_t projection,
    std::uint32_t camera,std::uint32_t output,std::uint32_t radius_bits,
    NativeTu15PhysicsFrameFp&,const SourcePhysicsContactRecordObserver& = {});
std::uint32_t native_tu15_sphere_rectangle_return_flags(std::uint32_t stack,std::uint32_t flags);
}
