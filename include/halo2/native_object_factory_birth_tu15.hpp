#pragma once
#include "halo2/physics_contact_record.hpp"
#include <cstdint>
#include <functional>
#include <map>

// Native TU1.5 object_new (B9B70), track "birth". Owners ported by the other
// factory tracks are injected through ObjectFactoryServices; an empty service
// is a named refusal (never original code). See docs/NATIVE_OBJECT_FACTORY.md.
namespace halo2::native_object_factory_tu15 {
using FactoryMemory=SourcePhysicsContactMemory;
struct ObjectFactoryServices {
  // Live MXCSR of the world (SSE compares/conversions in the factory).
  std::uint32_t mxcsr=0x1f80u;
  // Live x87 control word (C0270's position hash and fmod, BC5C0's draws).
  std::uint16_t x87_control=0x027fu;
  // The original's stack pointer at B9B70 entry, when known (oracle). A
  // relocating body-block allocation records BE3B0's frame slot (entry -
  // 0xF8) as the moved block's provisional owner before overwriting it;
  // zero records zero. The final world state does not depend on it.
  std::uint32_t entry_stack=0;
  // 1D3380(record, graph tag, model tag, 1): animation manager setup on
  // `record` (also probed on a private 0x84-byte local). Returns AL.
  std::function<bool(FactoryMemory&,std::uint32_t record,std::uint32_t graph_tag,std::uint32_t model_tag)> animation_setup;
  std::function<void(FactoryMemory&,std::uint32_t object,std::uint32_t region_mask)> regions;            // BC5C0
  // C0270: initial change colors from request +74 (mask) and +78 (colors).
  std::function<void(FactoryMemory&,std::uint32_t object,std::uint32_t mask,std::uint32_t colors)> change_colors;
  std::function<void(FactoryMemory&,std::uint32_t object)> function_colors;                               // C0910
  std::function<void(FactoryMemory&,std::uint32_t object)> type_finish;                                   // C06C0
  std::function<void(FactoryMemory&,std::uint32_t object)> node_rebuild;                                  // BF050
  std::function<bool(FactoryMemory&,std::uint32_t object,std::uint32_t location)> cluster_location;       // C1540
  std::function<void(FactoryMemory&,std::uint32_t object,std::uint32_t location)> connect;                // BA630
  std::function<void(FactoryMemory&,std::uint32_t object)> activate;                                      // BA8C0
  std::function<void(FactoryMemory&,std::uint32_t object)> deferred_delete;                               // BA570
  std::function<void(FactoryMemory&,std::uint32_t object,std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d)> disconnect; // C0F60
  // 1157D0 (unit +74, from C0200): node count and matrices.
  std::function<void(FactoryMemory&,std::uint32_t object,std::uint32_t count,std::uint32_t matrices)> unit_nodes;
  // BAF10(parent, marker, child) with EAX = marker index word (entry +4).
  std::function<void(FactoryMemory&,std::uint32_t parent,std::uint32_t marker,std::uint32_t child,std::uint32_t child_marker)> attach;
  // Type chain callbacks by original address (0x474B90 slots):
  //   validate (+28): void(object=request)   new (+2C): bool(object, request, failed byte address)
  //   place (+38): void(object)               new failed (+3C): void(object)
  std::map<std::uint32_t,std::function<void(FactoryMemory&,std::uint32_t request)>> validate;
  std::map<std::uint32_t,std::function<bool(FactoryMemory&,std::uint32_t object,std::uint32_t request,std::uint32_t failed)>> construct;
  std::map<std::uint32_t,std::function<void(FactoryMemory&,std::uint32_t object)>> place;
  std::map<std::uint32_t,std::function<void(FactoryMemory&,std::uint32_t object)>> construct_failed;
};
// B9B70: create the object described by the 0xC4-byte placement request at
// `request` (B9960 layout). Returns the object datum or NONE. The original's
// stack locals (the failure byte handed to type constructors, the probe
// record, C19D0 outputs) live in a private overlay and never reach the world.
std::uint32_t object_new(FactoryMemory&,std::uint32_t request,const ObjectFactoryServices&);
// C0790 (object place slot +38): births the object's model-variant children
// (variant body +B1; rows of 0x10: +0 marker, +4 child marker, +C tag) at
// the parent's position (BBE00) through object_new, attaches each (BAF10)
// and flags its body +4 bit 26. Requests live in a private local frame.
void object_place_variant_children(FactoryMemory&,std::uint32_t object,const ObjectFactoryServices&);
}
