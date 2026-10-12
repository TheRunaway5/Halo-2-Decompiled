#pragma once
#include "halo2/source_lift_contact.hpp"

namespace halo2 {
struct SourceLiftMembershipInput {
  std::uint32_t entity{}, own_component_datum{}, own_component_address{};
  std::int32_t own_body{}, phantom_shape{};
};
struct SourceLiftMembershipServices {
  // Original allocator, array growth and free owners. These services must
  // retain real source-address backing; there is no fabricated host contact.
  std::function<std::uint32_t(std::uint32_t size, std::uint32_t category)>
      allocate;
  std::function<void(std::uint32_t descriptor, std::uint32_t stride)> grow;
  std::function<void(std::uint32_t address, std::uint32_t size,
                     std::uint32_t category)>
      release;
};
// Complete1D5830/1DA0F0 and1D5A60/1DA320 direct contact-list owners.
// The input entity must come from the actual collision producer. These
// functions neither test overlap nor create a world body or apply a force.
void source_lift_contact_add(SourcePhysicsContactMemory &,
                             const SourceLiftMembershipInput &,
                             SourcePhysicsContactImage,
                             const SourceLiftMembershipServices &);
void source_lift_contact_remove(SourcePhysicsContactMemory &,
                                const SourceLiftMembershipInput &,
                                SourcePhysicsContactImage,
                                const SourceLiftMembershipServices &);
} // namespace halo2
