#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace halo2 {
class Cache;
enum class MccScriptLayoutNeed {
    NativeClockGetter, NativeBuiltin, UnknownBuiltin, BuiltinIdentity,
    ArgumentCount, ExternalGlobalsDisabled, UnknownExternalGlobal,
    ExternalGlobalIdentity
};
struct MccScriptBuiltinSignature {
    std::uint16_t mcc_opcode{},declared_return_type{};
    std::string_view name;
    std::span<const std::uint16_t> argument_types;
    // Absent for MCC-only definitions and same-name incompatible overloads.
    std::optional<std::uint16_t> xbox_opcode;
    std::uint16_t xbox_flags{};
};
// Immutable signatures audited from MCC and authenticated Xbox TU1.5
// executable metadata. Runtime operation reads neither executable nor XML.
std::optional<MccScriptBuiltinSignature> mcc_script_builtin_signature(std::uint16_t);
// Exact projected opcode plus retained donor name identifies the single
// authenticated vibration/rumble rename. Raw donor opcodes do not match.
std::optional<std::string_view> mcc_script_projected_builtin_name(
    std::uint16_t projected_opcode,std::string_view retained_name);
struct MccScriptLayoutRequirement {
    MccScriptLayoutNeed need{};
    std::uint32_t node_datum{};
    std::uint16_t donor_index{};
    std::string name;
};
struct MccScriptLayoutOptions {
    bool convert_external_globals=false;
};
struct MccScriptLayout {
    std::vector<std::byte> nodes;
    std::vector<MccScriptLayoutRequirement> requirements;
    std::size_t builtin_calls{},converted_calls{},rewritten_calls{},static_calls{};
    std::size_t external_globals{},converted_external_globals{};
    bool native_ready() const noexcept {return requirements.empty();}
    void require_native_ready() const;
};
// nodes consists of complete 20-byte syntax records; script_definitions uses
// the original/MCC 0x28-byte scenario script records. Only builtin parent
// opcode words and explicitly enabled external-global index bits can change.
// Function-name nodes, static script indices, datum salts, context return
// types, literals, links, strings and internal global references stay exact.
// Unowned semantics remain explicit requirements, allowing inspection/cache
// preview to retain the donor graph without claiming native admission.
MccScriptLayout project_mcc_script_layout(std::span<const std::byte> nodes,
    std::span<const std::byte> strings,std::span<const std::byte> script_definitions,
    MccScriptLayoutOptions={});
// The cache-aware path additionally restores three authenticated retail Cairo
// subtrees to their Xbox authoring. Full donor rows, strings, namespaces and
// incoming links must match; custom variants retain their original requirements.
// Apply once to raw MCC syntax, before opcode/global-index normalization.
MccScriptLayout project_mcc_script_layout(const Cache&,
    std::span<const std::byte> nodes,std::span<const std::byte> strings,
    std::span<const std::byte> script_definitions,MccScriptLayoutOptions={});
} // namespace halo2
