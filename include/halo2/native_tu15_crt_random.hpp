#pragma once
#include "halo2/physics_contact_record.hpp"
#include <stdexcept>

namespace halo2 {
enum class NativeTu15CrtRandomOwnerReason { elevated_level,context_allocation };
class NativeTu15CrtRandomRequiredOwner final:public std::runtime_error {
public:
    NativeTu15CrtRandomOwnerReason reason;
    std::uint32_t instruction,target;
    std::array<std::uint32_t,2> arguments;
    unsigned argument_count;
    NativeTu15CrtRandomRequiredOwner(NativeTu15CrtRandomOwnerReason,std::uint32_t,
        std::uint32_t,std::array<std::uint32_t,2>,unsigned);
};
// Actual330179 initialized main-thread/TLS lookup. The caller must supply its
// genuine retained current-executor FS base. No thread bank, rand seed or TLS
// slot is synthesized. Missing allocation/elevated-level owners stop at their
// real invocation phase; the post-allocation branches remain separate work.
std::uint32_t native_tu15_crt_thread_context(const SourcePhysicsContactMemory &,
    std::uint32_t actual_fs_base);
// Actual32C075: update the same retained CRT context+14 used by the original
// caller and return its15-bit result. This is distinct from campaign AI RNG.
// All operations are integer/data-only; no fresh FP policy is introduced.
std::uint32_t native_tu15_crt_random(SourcePhysicsContactMemory &,
    std::uint32_t actual_fs_base);
}
