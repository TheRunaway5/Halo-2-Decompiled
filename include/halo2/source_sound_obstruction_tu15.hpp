#pragma once
#include "halo2/math.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
// What a 3D voice's obstruction pass reads: the original's resident data
// (the structure BSP at [0x4ECC18] with its audibility block, and the door
// scales in the loop-sound globals at [0x4F9B58]+0x28), and a line test
// against the structure collision BSP standing in for 1DE630.
struct SourceSoundObstructionWorldTu15 {
  std::function<std::span<const std::byte>(std::uint32_t address, std::size_t size)> memory;
  // The hit fraction (0..1) along from -> to, if the segment hits.
  std::function<std::optional<float>(Vec3 from, Vec3 to)> line;
};

// A listener or a source as the pass sees it: a point, its structure BSP
// cluster and leaf (-1 when outside).
struct SourceSoundObstructionPointTu15 {
  Vec3 position;
  std::int32_t cluster = -1, leaf = -1;
};

// 221E30's three results for a voice: obstruction (the line test's
// thickness through geometry, or a portal-distance estimate), a room gain
// loss with distance from the source's environment portal, and occlusion by
// the doors between the clusters. 129790 scales each by the sound class's
// transmission multiplier (+0x50).
struct SourceSoundObstructionTu15 {
  float obstruction = 0, gain = 0, occlusion = 0;
};

// The per-listener half of 221A70: each cluster's door value from the
// listener's cluster, the largest of the doors that separate them (18CAC0's
// open/closed scale). Empty when the structure has no audibility data.
std::vector<float> source_sound_door_values_tu15(const SourceSoundObstructionWorldTu15 &, std::int32_t listener_cluster);

// 221E30's line test: the share of the listener-to-source segment inside
// the structure (both ends tested), or all of it past the first hit when the
// source is enclosed.
float source_sound_line_obstruction_tu15(const SourceSoundObstructionWorldTu15 &, Vec3 listener, Vec3 source);

// 221E30 for one voice. skip_line_test is 129100's flag47: the class
// (+0xA bit 6) or sound (flag 0x20) asks for the portal-distance estimate.
// A known line obstruction (an earlier source_sound_line_obstruction_tu15
// result) stands in for the line tests.
SourceSoundObstructionTu15 source_sound_obstruction_tu15(const SourceSoundObstructionWorldTu15 &,
                                                         std::span<const float> door_values,
                                                         const SourceSoundObstructionPointTu15 &listener,
                                                         const SourceSoundObstructionPointTu15 &source, bool skip_line_test,
                                                         std::optional<float> known_line_obstruction = {});

// 249D60: the distance sound travels between two clusters.
float source_sound_cluster_distance_tu15(const SourceSoundObstructionWorldTu15 &, std::int32_t a, std::int32_t b);
} // namespace halo2
