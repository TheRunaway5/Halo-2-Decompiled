#pragma once
#include "halo2/native_upscaler.hpp"
#include "halo2/cache.hpp"
#include <glm/glm.hpp>
#include <span>
#include <vector>
namespace halo2 {
struct TemporalView {
    glm::mat4 matrix{1};
    VkRect2D rectangle{};
    bool w_depth{};
    float near_plane=.05f,far_plane=10000.f,vertical_fov=1.2f;
    std::span<const Vertex> vertices;
    std::span<const Vertex> foreground;
    glm::mat4 foreground_matrix{1};
};
class TemporalUpscaler {
public:
    TemporalUpscaler(VkInstance,VkPhysicalDevice,VkDevice,VkQueue,unsigned,VkExtent2D input,VkExtent2D output,
        Upscaler,UpscalingQuality,const std::filesystem::path& shaders,VkImage depth,VkImageView depth_view,std::unique_ptr<NativeUpscaler> prepared={});
    ~TemporalUpscaler();
    glm::vec2 jitter() const;
    void begin(VkCommandBuffer);
    void reset_history();
    // Called outside render passes, after each view's world/foreground and
    // before UI cameras can clear its depth. View indices own independent history.
    void view(VkCommandBuffer,unsigned,const TemporalView&);
    void present(VkCommandBuffer,VkImage color,VkImageView,VkImage destination,VkImageLayout color_layout);
    std::uint64_t frames() const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
