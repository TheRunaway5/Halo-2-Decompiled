#pragma once
#include "halo2/vertex_program.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <vulkan/vulkan.h>

namespace halo2 {
struct SourceWeatherPointInput {
    // Immediate source registers: v0 includes the authored point-size word;
    // v1 is the logical ARGB color, before the SDK swaps its red/blue bytes.
    ShaderVector position;
    std::uint32_t color{};
};
static_assert(sizeof(SourceWeatherPointInput)==20);
enum class SourceWeatherPointSpriteCoordinates {
    // Saved original GL framebuffer/texture rows, with the actual point
    // coordinate origin at upper-left. No presentation image flip is applied.
    OriginalGlRawRowsUpperLeft,
};
struct SourceWeatherPointRaster {
    // Original D24 range: [0,16777215,nearW,farW]. These are source/device
    // inputs, not inferred from the prototype's clear depth.
    ShaderVector clip_range{},surface{};
    float depth_factor{},depth_offset{};
    std::uint32_t color_write_mask{}; // Vulkan RGBA channel bits.
    bool depth_clamp{},depth_clipping{};
    unsigned point_parameters_enabled{},point_sprite_enabled{};
    //391F0 inherits stencil. Explicit false is admitted for the observed
    // SDK4076D8/4B843C state; enabled stencil has no prototype implementation.
    std::optional<bool> stencil_enabled;
    std::optional<SourceWeatherPointSpriteCoordinates> sprite_coordinates;
};
struct SourceWeatherPointPacket {
    std::span<const std::byte> vertex_program;
    std::array<ShaderVector,12> camera;
    PixelProgram pixel;
    std::span<const SourceWeatherPointInput> points;
    // Explicit current2D sprite binding; its caller retains the actual BC3
    // levels32/16/8/4 (snow) or64/32/16/8/4 (flood spores), and source sampler.
    // This module never selects mips, manufactures readiness or substitutes
    // an authored full chain. The resource owner validates one2D layer.
    VkImageView sprite_view{};
    VkSampler sprite_sampler{};
    VkFormat sprite_format{};
    VkExtent2D sprite_extent{};
    unsigned sprite_mips{};
    SourceWeatherPointRaster raster;
};
// Ordinary-match CPU submission. Programs, sprite and ordered immediate rows
// come from the current map/resource owner; the renderer supplies GPU handles.
struct SourceWeatherPointDraw {
    std::span<const std::byte> vertex_program;
    std::array<ShaderVector,12> camera;
    PixelProgram pixel;
    std::vector<SourceWeatherPointInput> points;
    const Texture *sprite{};
    ShaderVector clip_range{};
};
struct SourceWeatherPointRenderContext {
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkRenderPass pass{}; // Compatible scene color + explicit depth target.
    std::filesystem::path shader_directory;
    // The logical device must actually enable these features. Querying the
    // physical device alone cannot establish logical-device admission.
    bool large_points_enabled{},depth_clamp_enabled{};
};
// Isolated original391F0 non-rain/VS22 point path. It does not publish weather,
// run simulation or decide visibility, clocks, resource readiness or residency.
// Its owner waits for earlier use before prepare() or destruction, then records
// the supplied ordered points inside the compatible scene pass. Known source
// states are SRC_ALPHA/INV_SRC_ALPHA, rounded-byte alpha GEQUAL0,
// LEQUAL/W/no depth writes.
// Color-write, clip range, depth offset/clamp and sampler are explicit inputs.
// Only positive finite sourceW/oPts within the physical point-size range are
// admitted; GPU rounding at range boundaries is not established by CPU checks.
// record() uses the prepared full-surface viewport and an explicit subset
// scissor. An arbitrary second split viewport is deliberately rejected.
class SourceWeatherPointRender {
public:
    explicit SourceWeatherPointRender(SourceWeatherPointRenderContext);
    ~SourceWeatherPointRender();
    SourceWeatherPointRender(const SourceWeatherPointRender&)=delete;
    SourceWeatherPointRender &operator=(const SourceWeatherPointRender&)=delete;
    void prepare(const SourceWeatherPointPacket &);
    // Remove point centers rejected by positive homogeneous W and the source
    // W clip planes. Visible points stay in source order. The programmable
    // output is clamped to this device's range, as in reference OpenGL point
    // rasterization; that native limit is not an inferred Xbox SDK maximum.
    void prepare_world(const SourceWeatherPointPacket &);
    void record(VkCommandBuffer,VkViewport,VkRect2D) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
