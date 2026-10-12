#pragma once
#include "halo2/vertex_program.hpp"
#include "halo2/source_weather_plate_submission.hpp"
#include <filesystem>
#include <memory>
#include <vulkan/vulkan.h>

namespace halo2 {
struct SourceWeatherPlateCallerInput;
struct SourceWeatherPlateVertex {
    // Original immediate v0..v4. v0 is projected screen XYZ/W, v1 is the
    // pixel-coordinate depth alias UV, and v2..4 are authored layer UVs.
    std::array<ShaderVector,5> registers;
};
static_assert(sizeof(SourceWeatherPlateVertex)==80);
struct SourceWeatherPlateTexture {
    // Caller must retain source sampler state: alias stage0 uses nearest
    // min/mag, no mip filtering and clamp-to-edge; stages1..3 use repeat,
    // linear min/mag/mip and zero bias. Opaque VkSampler handles cannot be
    // inspected here. The matched fixture creates exactly these samplers.
    VkImageView view{};
    VkSampler sampler{};
    VkFormat format{};
    VkExtent2D extent{};
    unsigned mips{};
};
struct SourceWeatherPlateDraw {
    std::span<const std::byte> vertex_program;
    SourceWeatherPlateSubmission submission;
    std::array<const Texture*,3> layers{};
    ShaderVector clip_range{};
    std::array<std::int16_t,4> rectangle{};
};
struct SourceWeatherPlatePacket {
    std::span<const std::byte> vertex_program;
    PixelProgram pixel;
    std::array<SourceWeatherPlateVertex,4> vertices;
    std::array<ShaderVector,3> scroll;
    ShaderVector clip_range{},surface{};
    std::array<std::array<std::int32_t,4>,8> clip_regions;
    // Stage0 is the original packed RGBA depth alias, not a float sampler.
    // Resource ownership, its current bytes, and all layer mips are inputs.
    std::array<SourceWeatherPlateTexture,4> textures;
    bool depth_clamp{},depth_clipping{};
};
struct SourceWeatherPlateRenderContext {
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkRenderPass pass{};
    std::filesystem::path shader_directory;
    bool depth_clamp_enabled{};
};
// Isolated observed VS21/3A5C0 mode3 packet. It records the source four-vertex
// strip with ONE/INV_SRC_ALPHA, rounded-byte alpha GREATER0, RGB-only writes,
// LEQUAL W depth and no depth writes. Caller synchronization and the matching
// color/depth pass are required. It provides no live producer/loading.
// Legacy preparation admits only the retained captured pixel definition.
// Typed preparation derives the original caller profile/program/projection
// from separate explicit camera banks before admitting its matching packet.
class SourceWeatherPlateRender {
public:
    explicit SourceWeatherPlateRender(SourceWeatherPlateRenderContext);
    ~SourceWeatherPlateRender();
    SourceWeatherPlateRender(const SourceWeatherPlateRender&)=delete;
    SourceWeatherPlateRender &operator=(const SourceWeatherPlateRender&)=delete;
    void prepare(const SourceWeatherPlatePacket &);
    void prepare(const SourceWeatherPlateCallerInput &,const SourceWeatherPlatePacket &);
    void record(VkCommandBuffer,VkViewport,VkRect2D) const;
private:
    // Only the captured legacy definition or the internally derived original
    // caller result can reach this seam. Arbitrary expected words are private.
    void prepare_admitted(const SourceWeatherPlatePacket &,const std::array<std::uint32_t,60> &);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
