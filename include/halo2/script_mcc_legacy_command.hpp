#pragma once
#include "halo2/native_mcc_script_builtin.hpp"
#include "halo2/scripts.hpp"
#include "halo2/kernels.hpp"
#include <bit>

namespace halo2 {
// The authenticated MCC-only legacy command is VOID(SHORT), with an empty
// effect. Argument evaluation and scheduling remain the ScriptVm's owners.
inline std::optional<ScriptValue> mcc_script_legacy_command(const std::string &name,
    const std::vector<ScriptValue> &arguments) {
    if(name!="rasterizer_blur")return {};
    if(arguments.size()!=1)throw FormatError("MCC rasterizer_blur requires one SHORT argument");
    const auto bits=scalar_cast_bits(arguments[0].bits,arguments[0].type,7);
    native_mcc_legacy_rasterizer_blur(std::bit_cast<std::int16_t>(std::uint16_t(bits)));
    return ScriptValue{};
}
}
