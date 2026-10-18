#pragma once
#include "halo2/source_runtime_memory.hpp"
namespace halo2 {
// Exact245400 camera-packet multiplier and382360/382380 destinations.
// The original CRT owners still compute/publish both limits; no completed
// limit, packet, observer or GPU readiness is prepopulated.
void mount_native_tu15_camera_packet_data(SourceRuntimeMemory&);
}
