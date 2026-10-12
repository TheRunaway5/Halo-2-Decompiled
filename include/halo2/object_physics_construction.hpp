#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Mutable global pointer slots, never initial values or readiness overrides.
struct SourceObjectPhysicsConstructionState {
  std::uint32_t object_pool_slot,component_pool_slot,context_flag,counter_slot;
};
struct ObjectPhysicsConstructionStore {
  std::uint32_t instruction,address,before,after;
  unsigned bytes;
  friend bool operator==(const ObjectPhysicsConstructionStore &,
                         const ObjectPhysicsConstructionStore &)=default;
};
using ObjectPhysicsConstructionObserver=std::function<void(const ObjectPhysicsConstructionStore &)>;
struct ObjectPhysicsConstructionCall {
  std::uint32_t instruction,entry,object,component;
  friend bool operator==(const ObjectPhysicsConstructionCall &,
                         const ObjectPhysicsConstructionCall &)=default;
};
struct SourceObjectPhysicsConstructionServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Complete148360,27F190,1D2AA0,1D2B10 and1D4F30 owners, when reached.
  // Only1D2AA0's defined EAX is consumed. Required side effects must occur in
  // the actual retained memory. Missing owners stop rather than returning NONE
  // or constructing an unrelated host body. No fixture service is installed.
  std::function<std::uint32_t(const ObjectPhysicsConstructionCall &)> invoke;
};
// CompleteBBE70 parent-chain query. Header salt validation is not added.
std::uint32_t source_object_physics_root(const SourceObjectPhysicsConstructionState &,
    std::uint32_t object,const SourceObjectPhysicsConstructionServices &);
// CompleteC0D10 word-bit leaf: x86 shift count masks to31 and the request is
// tested as a low byte. Bits outside the word truncate exactly as in Source.
void source_object_physics_word_bit(std::uint32_t address,std::uint32_t bit,
    std::uint32_t request,const SourceObjectPhysicsConstructionServices &,
    const ObjectPhysicsConstructionObserver & = {});
// Complete1C72E0 dirty-bit/counter owner on actual object/pool storage.
void source_object_physics_mark(const SourceObjectPhysicsConstructionState &,
    std::uint32_t object,const SourceObjectPhysicsConstructionServices &,
    const ObjectPhysicsConstructionObserver & = {});
// Complete1C6F50 direct owner, composed with the three leaves above. Physical
// component allocation/initialization/world rebuilding remain mandatory.
void source_object_physics_construct(const SourceObjectPhysicsConstructionState &,
    std::uint32_t object,const SourceObjectPhysicsConstructionServices &,
    const ObjectPhysicsConstructionObserver & = {});
// Complete1C7050 recursive owner. Actual child head is retained before lower
// construction; child body is read before the source root-sentinel comparison.
void source_object_physics_construct_tree(const SourceObjectPhysicsConstructionState &,
    std::uint32_t root,std::uint32_t object,const SourceObjectPhysicsConstructionServices &,
    const ObjectPhysicsConstructionObserver & = {});
} // namespace halo2
