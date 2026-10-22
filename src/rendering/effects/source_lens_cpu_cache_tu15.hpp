#pragma once
#include "halo2/source_runtime_memory.hpp"
#include <vector>
namespace halo2::private_lens_cpu {
struct Region {
    unsigned address;
    SourceRuntimeMemory::Access access;
    const char* role;
    std::vector<std::byte> bytes;
};
// CPU adapter inputs only. The frame must already retain its real renderer
// device/images; this plan neither issues a renderer lease nor creates a GPU
// resource, SDK device, readiness result or initialized original hardware.
std::vector<Region> mcc_cache_plan();
}
