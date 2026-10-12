#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// Complete1D56F0/1D9FB0. Reads the actual component/object/tag/phantom
// banks through their selected-image global slots, including signed shape-rule
// index, size limits, player binding and original type/control exclusion bits.
// This admits a counterpart only; it does not create phantom contacts, apply
// forces, activate a body or run a world step.
bool source_lift_contact_allowed(const SourcePhysicsContactMemory &,
                                 std::uint32_t phantom_shape,
                                 std::uint32_t own_component,
                                 std::uint32_t other_component,
                                 SourcePhysicsContactImage);
} // namespace halo2
