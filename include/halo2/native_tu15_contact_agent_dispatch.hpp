#pragma once
#include "halo2/native_tu15_contact_producer.hpp"
namespace halo2 {
struct NativeTu15ContactAgent {std::uint32_t identity{};};
struct NativeTu15ContactAgentDispatchServices {
  // Source shape vtable+14 (RET0). The actual method determines the class;
  // callers must not replace it with a fabricated classification outcome.
  std::function<std::uint32_t(NativeTu15ContactChild,NativeTu15ContactVirtualOwner,
    NativeTu15PhysicsFrameFp&)>classify_shape;
  // Actual selected cdecl4 factory. Its returned agent identity and mutable
  // memory must come from its genuine construction work.
  std::function<NativeTu15ContactAgent(NativeTu15ContactVirtualOwner,
    NativeTu15ContactPayload,NativeTu15ContactPayload,NativeTu15ContactContext,
    std::uint32_t user_data,NativeTu15PhysicsFrameFp&)>create_agent;
};
// Actual2F3140. Cache the dispatch table before the first real class call,
// reread the second shape afterward, and preserve32-bit index wrapping.
NativeTu15ContactAgent native_tu15_dispatch_contact_agent(SourcePhysicsContactMemory&,
  NativeTu15ContactPayload,NativeTu15ContactPayload,NativeTu15ContactContext,
  std::uint32_t user_data,NativeTu15PhysicsFrameFp&,
  const NativeTu15ContactAgentDispatchServices&);
// Actual30BAD0's authenticated constant sentinel DATA identity.
constexpr NativeTu15ContactAgent native_tu15_invalid_contact_agent(){return{0x48cdc4u};}
} // namespace halo2
