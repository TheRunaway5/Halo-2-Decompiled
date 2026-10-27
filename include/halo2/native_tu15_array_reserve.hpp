#pragma once
#include "halo2/native_tu15_array_growth.hpp"
namespace halo2 {
// Actual cdecl input cells may alias allocator-owned storage. Read stride and
// capacity after the allocator/vtable input, and header only after allocation.
// These callbacks read source inputs; they never provide allocation outcomes.
struct NativeTu15ArrayReserveArguments {
  std::function<std::uint32_t()> stride;
  std::function<std::uint32_t()> capacity;
  std::function<NativeTu15ArrayHeader()> header;
};
void native_tu15_reserve_array(SourcePhysicsContactMemory&,
  const NativeTu15ArrayReserveArguments&,NativeTu15PhysicsFrameFp&,
  const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15ArrayGrowthActivity* = nullptr);
// Actual2E31E0 cdecl(header,capacity,stride). The supplied capacity is retained
// across real allocator callbacks; count/data/ownership are reread at their
// original phases. It allocates even if the existing capacity would suffice.
// Shared byte-copy and allocator owners retain their independently proved
// native implementations. This API supplies no allocation outcome.
void native_tu15_reserve_array(SourcePhysicsContactMemory&,NativeTu15ArrayHeader,
  std::uint32_t capacity,std::uint32_t stride,NativeTu15PhysicsFrameFp&,
  const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15ArrayGrowthActivity* = nullptr);
} // namespace halo2
