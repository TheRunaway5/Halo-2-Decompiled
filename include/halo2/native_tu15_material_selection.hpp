#pragma once

#include "halo2/physics_contact_record.hpp"

namespace halo2 {
struct NativeTu15MaterialSelectionArgs {
    std::uint32_t shader,lod,layer,pass,token,descriptor,stack;
};
struct NativeTu15MaterialSelection {
    struct Store {std::uint32_t instruction,address,value;unsigned width;};
    std::array<Store,10> stores{};
    unsigned count{};
    bool accepted{},class_rejected{};
    std::span<const Store> rows() const {return std::span(stores).first(count);}
};
// Plan original caller-argument and descriptor stores for the integer shad
// paths. Unmapped or unowned input leaves the genuine callback available.
NativeTu15MaterialSelection native_tu15_material_selection_4cbe0(
    const SourcePhysicsContactMemory&,const NativeTu15MaterialSelectionArgs&);
}
