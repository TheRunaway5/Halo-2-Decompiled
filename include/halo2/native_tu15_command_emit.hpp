#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <array>

namespace halo2 {
struct NativeTu15CommandEmitArgs {
    std::uint32_t unit{},part{},mode{},coverage{},packet{},shader{},grouped{},mask{},add{};
};
struct NativeTu15CommandEmitStore {
    std::uint32_t instruction,address,value;
    unsigned width;
};
class NativeTu15CommandEmitStores {
    // At most two nine-store bank appends and one seventeen-store pool append.
    std::array<NativeTu15CommandEmitStore,35> rows_{};
    std::size_t size_{};
public:
    void append(NativeTu15CommandEmitStore);
    std::span<const NativeTu15CommandEmitStore> rows() const {return std::span(rows_).first(size_);}
};
// Whole ordinary 41030 and its ordinary callees. Stage only non-stack stores,
// in source order; the caller owns publication into its current command banks.
// Rejection publishes neither stores nor arithmetic state.
NativeTu15CommandEmitStores native_tu15_command_emit_41030(const SourcePhysicsContactMemory&,
    const NativeTu15CommandEmitArgs&,NativeTu15PhysicsFrameFp&);
}
