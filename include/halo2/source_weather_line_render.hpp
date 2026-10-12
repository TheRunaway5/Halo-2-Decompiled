#pragma once
#include "halo2/vertex_program.hpp"
#include <filesystem>
#include <memory>
#include <vulkan/vulkan.h>

namespace halo2 {
struct SourceWeatherLineInput {ShaderVector position;std::uint32_t color{};};
static_assert(sizeof(SourceWeatherLineInput)==20);
struct SourceWeatherLineRaster {
    ShaderVector clip_range{},surface{};
    float depth_factor{},depth_offset{},line_width=1;
    std::uint32_t color_write_mask=7;
    bool depth_clamp{},depth_clipping{},stencil_enabled{};
};
struct SourceWeatherLinePacket {
    std::span<const std::byte> vertex_program;
    std::array<ShaderVector,12> camera;
    PixelProgram pixel;
    std::span<const SourceWeatherLineInput> vertices;
    SourceWeatherLineRaster raster;
};
struct SourceWeatherLineDraw {
    std::span<const std::byte> vertex_program;
    std::array<ShaderVector,12> camera;
    PixelProgram pixel;
    std::vector<SourceWeatherLineInput> vertices;
    ShaderVector clip_range{};
    float line_width=1;
};
struct SourceWeatherLineRenderContext {
    VkPhysicalDevice physical{};VkDevice device{};VkRenderPass pass{};
    std::filesystem::path shader_directory;
    bool geometry_shader_enabled{},wide_lines_enabled{},depth_clamp_enabled{};
};
// Original391F0 rain immediate pairs, VS22 and39780's rain combiner. No
// bitmap is selected. The retained xemu geometry profile supplies the source
// perpendicular third vertex for W-depth interpolation. Native line coverage
// and width use the actual logical device, not a claimed NV2A raster match.
class SourceWeatherLineRender {
public:
    explicit SourceWeatherLineRender(SourceWeatherLineRenderContext);
    ~SourceWeatherLineRender();
    SourceWeatherLineRender(const SourceWeatherLineRender&)=delete;
    void prepare(const SourceWeatherLinePacket &);
    void record(VkCommandBuffer,VkViewport,VkRect2D)const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
