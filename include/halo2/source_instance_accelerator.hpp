#pragma once
#include "halo2/cache.hpp"
#include <functional>

namespace halo2 {
// Original244980 passes cluster+AC directly as MoppInfo to301800. The
// serialized info contains float4 at0, bytecode length at20, code at30.
struct SourceInstanceAccelerator {
    ShaderVector offset_scale{};
    std::vector<std::byte> code;
};
SourceInstanceAccelerator decode_source_instance_accelerator(
    std::span<const std::byte> serialized,unsigned padded_code_size);
struct SourceInstanceAcceleratorInstruction {
    unsigned byte_offset{},opcode{};
    bool operator==(const SourceInstanceAcceleratorInstruction&)const=default;
};
std::vector<SourceInstanceAcceleratorInstruction> source_instance_accelerator_inventory(
    const SourceInstanceAccelerator&);
struct SourceInstanceAcceleratorVisit {
    std::vector<std::uint32_t> keys;
    std::vector<SourceInstanceAcceleratorInstruction> instructions;
    float fraction{};
};
// Callback reproduces the source collector: it returns its current fraction
// after handling this key, even on a miss. That state affects later branches.
// Keys are literal32-bit source values;244C10 consumes their signed low16.
using SourceInstanceAcceleratorCallback=std::function<float(std::uint32_t,float)>;
SourceInstanceAcceleratorVisit source_instance_accelerator_visit(
    const SourceInstanceAccelerator&,Vec3 origin,Vec3 ray,float fraction,
    const SourceInstanceAcceleratorCallback&);
} // namespace halo2
