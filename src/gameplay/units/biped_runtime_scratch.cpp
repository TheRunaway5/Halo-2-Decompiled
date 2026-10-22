#include "halo2/biped_controller.hpp"
#include "halo2/biped_ragdoll_admission.hpp"
#include "halo2/physics_body_grouping.hpp"

namespace halo2 {
SourceBipedPlacementScratch
source_biped_placement_scratch_1_1(std::uint32_t entry) {
  // E3190 owns110h locals plus EBP; its E3270 and E3A10 entries both
  // follow two more saved registers, seven arguments and CALL.
  const auto recursive = entry - 0x13c;
  return {entry - 0x108, entry - 0x110, entry - 0x10c,
          recursive - 0xc, recursive - 8, recursive - 4, entry};
}
SourceBipedRagdollRebuildScratch
source_biped_ragdoll_rebuild_scratch_1_1(std::uint32_t entry) {
  // E2CE0 has18h locals and four saved registers. Its E3190 entry
  // follows two arguments/CALL. After ESI is popped, E2430 owns a
  // separate3190E0 requirement packet54h below its entry (entry-28h).
  return {entry - 12, source_biped_placement_scratch_1_1(entry - 0x34),
          entry - 0x7c};
}
SourceBipedDeathAdmissionScratch
source_biped_death_admission_scratch_1_1(std::uint32_t entry) {
  // E2520 has4Ch locals and three saved registers; its contact branch
  // additionally saves EDI. E2430->1D6FC0->3190E0 owns the16-byte
  // requirement packet. Rebuild E2CE0 owns its separate nested packet.
  return {entry - 0xb0, entry - 0x44, entry - 0x24, entry - 0xc};
}
namespace {
std::uint32_t aligned_frame(std::uint32_t entry, std::uint32_t locals) {
  return ((entry - 4u) & ~15u) - locals;
}
} // namespace
SourcePhysicsBodyFactoryScratch
source_physics_body_factory_scratch_1_1(std::uint32_t entry) {
  // 1DAAC0: PUSH EBP, align,124h locals, three saved registers. The
  // 318100->317BF0->31AC90->31ABD0 AABB and317D10 radius AABB have
  //  distinct lifetimes and distinct original aligned caller frames.
  const auto frame = aligned_frame(entry, 0x130);
  return {frame + 0x20, frame + 0x10, frame - 0xa0, frame - 0x50};
}
SourcePhysicsShapeWrapperScratch
source_physics_shape_wrapper_scratch_1_1(std::uint32_t entry) {
  // 1DACF0's saved registers/three arguments/CALL put1DADF0 at entry-1C.
  // 1DADF0 aligns and owns1A4h locals plus three saved registers.184510
  // owns its aligned50h matrix frame;1DAD80 owns the configuration packet.
  const auto frame = aligned_frame(entry - 0x1c, 0x1b0);
  return {frame + 0x28, frame + 0x2c, frame - 0x50, frame - 0x20};
}
SourcePhysicsBodyGroupingScratch
source_physics_body_grouping_scratch_1_1(std::uint32_t entry) {
  SourcePhysicsBodyGroupingScratch out{};
  // 326AF0 probes4E84h locals;1D9330 then saves four registers.
  out.grouping_frame = entry - 0x4e94;
  // Eight arguments and CALL enter1D95E0 at grouping_frame-24h.
  out.selection_frame = aligned_frame(out.grouping_frame - 0x24, 0x100);
  out.collection_frame = aligned_frame(out.selection_frame - 0x40, 0x460);
  // 183500/183770 output packets are below the grouping locals;1DA4C0
  // later reuses the same call area for its1DB1C0 constraint construction.
  out.region_result = out.grouping_frame - 0x14;
  out.permutation_result = out.grouping_frame - 0x10;
  out.constraints_frame = aligned_frame(out.grouping_frame - 0x14, 0x460);
  out.iterator_stride = out.constraints_frame - 0xc;
  out.factory =
      source_physics_body_factory_scratch_1_1(out.selection_frame - 0x3c);
  out.wrapper =
      source_physics_shape_wrapper_scratch_1_1(out.selection_frame - 0xc);
  return out;
}
SourceBipedControllerScratch
source_biped_controller_scratch_1_1(std::uint32_t entry) {
  // 1DA6F0 owns D4h locals and three saved registers. Its factory and
  // wrapper calls marshal the same argument packets as1D95E0.
  const auto frame = aligned_frame(entry, 0xe0);
  return {entry, source_physics_body_factory_scratch_1_1(frame - 0x3c),
          source_physics_shape_wrapper_scratch_1_1(frame - 0xc)};
}
} // namespace halo2
