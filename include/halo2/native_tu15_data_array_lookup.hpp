#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Complete16E940. Match the original signed high-water and nonzero salt;
// retain the original lazy reads and 32-bit row-address arithmetic.
std::uint32_t native_tu15_data_array_try_lookup(const SourcePhysicsContactMemory &,
    std::uint32_t pool,std::uint32_t datum);
}
