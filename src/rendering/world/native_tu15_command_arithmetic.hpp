#pragma once

#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include "halo2/native_tu15_command_emit.hpp"
#include "halo2/native_tu15_command_part_prepare.hpp"
#include "halo2/native_tu15_shader_lod.hpp"
#include <cstdlib>
#include <optional>

namespace halo2::detail {
// The retained command walker lends a callback-free, read-only World bank
// to these kernels. Generic providers still use the full FP environment.
enum class CommandArithmeticPolicy { Full, RetainedSse };

inline bool reference_command_x87_scope() {
    static const bool enabled=[] {
        const char *value=std::getenv("HALO2_REFERENCE_COMMAND_X87_SCOPE");
        return value && *value;
    }();
    return enabled;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class CommandArithmeticScope {
    NativeTu15PhysicsFrameFp &fp_;
    std::optional<NativeTu15ArithmeticEnvironment> full_;
    unsigned host_mxcsr_{};
public:
    CommandArithmeticScope(NativeTu15PhysicsFrameFp &fp,CommandArithmeticPolicy policy):fp_(fp) {
        if(policy==CommandArithmeticPolicy::Full) {
            full_.emplace(fp);
            return;
        }
        // Keep the full environment's refusal domain. These paths execute
        // SSE only and neither inspect nor update the emulated x87 stack.
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
           (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native vector math requires masked original FP exceptions");
        host_mxcsr_=_mm_getcsr();
        _mm_setcsr(fp.mxcsr);
    }
    ~CommandArithmeticScope() {
        if(!full_) {
            fp_.mxcsr=_mm_getcsr();
            _mm_setcsr(host_mxcsr_);
        }
    }
    CommandArithmeticScope(const CommandArithmeticScope&)=delete;
    CommandArithmeticScope& operator=(const CommandArithmeticScope&)=delete;
};
#endif

NativeTu15PartPacket45cd0 command_part_prepare(const SourcePhysicsContactMemory&,std::uint32_t unit,
    std::uint32_t part,std::uint32_t matrix,const std::array<std::byte,44>&,NativeTu15PhysicsFrameFp&,
    CommandArithmeticPolicy);
NativeTu15CommandEmitStores command_emit(const SourcePhysicsContactMemory&,
    const NativeTu15CommandEmitArgs&,NativeTu15PhysicsFrameFp&,CommandArithmeticPolicy);
NativeTu15ShaderLod1bf30 command_shader_lod(const SourcePhysicsContactMemory&,std::uint32_t bias,
    std::uint32_t shader,std::uint32_t metric_bits,NativeTu15PhysicsFrameFp&,CommandArithmeticPolicy);
} // namespace halo2::detail
