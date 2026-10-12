#pragma once
#include "halo2/source_shadow.hpp"
#include "halo2/source_tag_metadata.hpp"

namespace halo2 {
// Original1DC8F0/1DC940 authored marker selection. The metadata must be a
// render-model tag; object/hlmt resolution remains the caller's choice.
std::vector<ObjectAttachmentMarker> source_model_markers(
    const SourceTagMetadata &,std::uint32_t name);
struct PosedModelMarker {
  unsigned node{};
  Transform local,world;
  float authored_scale{};
};
// Original1DC940 optional region and node-remap inputs. Empty spans mean the
// original null pointers, not fabricated permutation or node states.
std::vector<PosedModelMarker> pose_model_markers(
    std::span<const ObjectAttachmentMarker>,std::span<const Transform> current_nodes,
    std::span<const std::uint8_t> permutations={},
    std::span<const std::uint32_t> node_remap={},unsigned maximum=16);
} // namespace halo2
