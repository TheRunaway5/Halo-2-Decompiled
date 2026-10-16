#pragma once
#include "halo2/render_view.hpp"
#include <vulkan/vulkan.h>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace halo2 {
// Original511 request records, each retaining three hardware query identities.
inline constexpr unsigned source_lens_query_capacity=511*3;
struct SourceLensQueryDraw {
    std::uint32_t id{};
    std::uint64_t generation{};
    std::array<ShaderVector,4> vertices{};
    bool has_quad{};
};
struct SourceLensQueryResult {
    std::uint32_t id{};
    std::uint64_t generation{},samples{},timestamp{};
};
struct SourceLensQueryGpuContext {
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkRenderPass render_pass{};
    std::uint32_t queue_family{};
    bool precise_queries_enabled{};
    std::filesystem::path shader_directory;
};
// Actual depth-tested GPU queries. Caller owns a one-sample D24S8 world pass,
// excludes submissions while preparing, and keeps this owner/device alive
// until submitted work completes. No visibility or timestamp is synthesized.
class SourceLensQueryGpu {
public:
    explicit SourceLensQueryGpu(SourceLensQueryGpuContext);
    ~SourceLensQueryGpu();
    void prepare(std::span<const SourceLensQueryDraw>);
    // Outside render pass, after earlier submissions using these IDs retire.
    void reset(VkCommandBuffer);
    // Inside the retained world-depth pass, before foreground depth changes.
    void record(VkCommandBuffer,std::span<const SourceLensQueryDraw>,VkExtent2D,VkRect2D);
    void submitted();
    void discard_unsubmitted();
    std::vector<SourceLensQueryResult> completed();
    SourceLensQueryGpu(const SourceLensQueryGpu&)=delete;
    SourceLensQueryGpu& operator=(const SourceLensQueryGpu&)=delete;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
