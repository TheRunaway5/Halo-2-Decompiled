#pragma once
#include "halo2/mcc_script_layout.hpp"
#include <memory>
#include <thread>

namespace halo2 {
class SourcePhysicsContactMemory;
// Immutable owning identities for authenticated MCC-only calls. No
// source/native addresses or live backing views are kept.
class NativeMccScriptBuiltinProfile {
public:
    struct Impl;
    // Command admission stays opt-in until its World/VM consumers qualify.
    NativeMccScriptBuiltinProfile(const MccScriptLayout&,std::span<const std::byte> strings,
        bool enable_mcc_blur=false);
    bool owns(const MccScriptLayoutRequirement&) const noexcept;
    std::size_t calls() const noexcept;
private:
    friend class NativeMccScriptBuiltinBinding;
    std::shared_ptr<const Impl> impl_;
};
// Bind around a source entry on its executor. Same-profile nested entries
// are supported. A checkpoint may replace the backing: hooks resolve current
// scenario/pool/string addresses and full salted identities afresh.
class NativeMccScriptBuiltinBinding {
public:
    NativeMccScriptBuiltinBinding(SourcePhysicsContactMemory&,const NativeMccScriptBuiltinProfile&);
    ~NativeMccScriptBuiltinBinding();
    NativeMccScriptBuiltinBinding(const NativeMccScriptBuiltinBinding&)=delete;
    NativeMccScriptBuiltinBinding& operator=(const NativeMccScriptBuiltinBinding&)=delete;
private:
    SourcePhysicsContactMemory* memory_;
    std::shared_ptr<const NativeMccScriptBuiltinProfile::Impl> profile_,previous_;
    const std::thread::id executor_=std::this_thread::get_id();
};
// Unbound banks return nullopt without reading bank data. These are genuine
// clock reads/metadata, not replacements for original Xbox function returns.
std::optional<std::uint32_t> native_mcc_script_builtin_value(
    const SourcePhysicsContactMemory&,std::uint32_t syntax_address);
std::optional<std::uint16_t> native_mcc_script_builtin_return_type(
    const SourcePhysicsContactMemory&,std::uint32_t syntax_address);
enum class NativeMccScriptCommand { RasterizerBlur };
// This is a command receipt, never a supplied parser result or getter value.
std::optional<NativeMccScriptCommand> native_mcc_script_builtin_command(
    const SourcePhysicsContactMemory&,std::uint32_t syntax_address);
// Authenticated MCC180030CC0 is a literal RET0; it preserves no blur state.
// Its caller still parses, reads the signed SHORT and completes a VOID call.
void native_mcc_legacy_rasterizer_blur(std::int16_t amount) noexcept;
} // namespace halo2
