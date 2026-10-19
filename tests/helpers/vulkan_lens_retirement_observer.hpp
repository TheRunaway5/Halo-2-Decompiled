#pragma once
#include <cstdint>

// Test executable link wrappers observe real Vulkan entry points, without
// replacing their results or adding a production renderer callback. These
// probes own no files or devices; the ordinary CTest scratch/process lifecycle
// owns all actual resources. Observation is local to the caller's executor.
struct VulkanLensRetirementCalls {
    std::uint64_t waits{},buffers{},memory{},pipelines{},layouts{},pools{},query_reads{};
    std::uint64_t total()const noexcept{return waits+buffers+memory+pipelines+layouts+pools+query_reads;}
};
void begin_vulkan_lens_retirement_observation() noexcept;
VulkanLensRetirementCalls end_vulkan_lens_retirement_observation() noexcept;
