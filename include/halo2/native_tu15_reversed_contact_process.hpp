#pragma once
#include "halo2/native_tu15_compound_process.hpp"
namespace halo2 {
enum class NativeTu15ReversedContactKind {wrapped,compound};
struct NativeTu15ReversedContactProcessStats {
  std::uint64_t calls{},child_calls{},child_returns{},normals_reversed{};
};
// The ordinary first signed contact index comes from the PUBLIC parent's
// pre-child header read. This suffix changes only newly appended16B normals;
// it rereads the actual header/count/data and source16B mask at every phase.
void native_tu15_reverse_appended_contact_normals(SourcePhysicsContactMemory&,
  NativeTu15IslandContactOutput,std::uint32_t first_index,NativeTu15ReversedContactKind,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15ReversedContactProcessStats* = nullptr);
// Complete PUBLIC2F7480 /2F7FB0, including prologue DATA reads and the actual
// reversed first/second static child call. Typed services execute current child
// work, never supply contact outcomes or serialized GP/SIMD/private postimages.
void native_tu15_process_reversed_wrapped_contact_agent(SourcePhysicsContactMemory&,
  NativeTu15IslandCollisionAgent,NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
  NativeTu15PhysicsStepConfiguration,NativeTu15IslandContactOutput,NativeTu15PhysicsFrameFp&,
  const NativeTu15CompoundProcessServices&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15ReversedContactProcessStats* = nullptr);
void native_tu15_process_reversed_compound_contact_agent(SourcePhysicsContactMemory&,
  NativeTu15IslandCollisionAgent,NativeTu15PhysicsBody first,NativeTu15PhysicsBody second,
  NativeTu15PhysicsStepConfiguration,NativeTu15IslandContactOutput,NativeTu15PhysicsFrameFp&,
  const NativeTu15CompoundProcessServices&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15ReversedContactProcessStats* = nullptr);
} // namespace halo2
