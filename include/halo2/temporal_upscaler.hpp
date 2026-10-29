#pragma once
#include "halo2/native_upscaler.hpp"
#include "halo2/cache.hpp"
#include <glm/glm.hpp>
#include <functional>
#include <span>
#include <vector>
namespace halo2 {
// Salted source object/model and original expanded vertex ordinal. Material
// slots are shared between live owners and cannot establish correspondence.
struct TemporalVertexIdentity {
    std::uint32_t owner=0xffffffffu,definition=0xffffffffu,render_model=0xffffffffu;
    std::uint32_t lod=0xffffffffu,source_vertex=0xffffffffu;
    bool valid()const{return owner!=0xffffffffu && render_model!=0xffffffffu && source_vertex!=0xffffffffu;}
    bool operator==(const TemporalVertexIdentity&)const=default;
};
struct TemporalView {
    glm::mat4 matrix{1};
    VkRect2D rectangle{};
    bool w_depth{};
    float near_plane=.05f,far_plane=10000.f,vertical_fov=1.2f;
    std::span<const Vertex> vertices;
    std::span<const Vertex> foreground;
    glm::mat4 foreground_matrix{1};
    // Empty means no retained source owner. Such records may reuse history
    // only when their WORLD position is unchanged; moving unknown owners reject it.
    std::span<const TemporalVertexIdentity> identities,foreground_identities;
};
class TemporalUpscaler {
public:
    // Optional diagnostics at the actual vendor boundary. The observer may
    // record readback commands, restoring every input's shader-read layout.
    using InputObserver=std::function<void(const UpscalingDispatch&)>;
    TemporalUpscaler(VkInstance,VkPhysicalDevice,VkDevice,VkQueue,unsigned,VkExtent2D input,VkExtent2D output,
        Upscaler,UpscalingQuality,const std::filesystem::path& shaders,VkImage depth,VkImageView depth_view,std::unique_ptr<NativeUpscaler> prepared={},InputObserver observer={});
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
