#pragma once
#include "halo2/effect_light_lifetime.hpp"
namespace halo2 {
class SourceRuntimeMemory;
// Read-only copy of the completed native light and spatial data arrays.
// Full generations, occupied order, partial lists and retained record bytes
// are simulation-owned. Presentation may update only its private view fields;
// it must not allocate, tick, register, retire or republish this snapshot.
struct SourceNativeLightSnapshotTu15 {
    SourceEffectLightDatumPool lights;
    SourceEffectLightSpatialPool spatial;
    explicit SourceNativeLightSnapshotTu15(std::size_t clusters):spatial(clusters){}
};
SourceNativeLightSnapshotTu15 source_native_light_snapshot_tu15(
    const SourceRuntimeMemory&,std::size_t clusters);
}
