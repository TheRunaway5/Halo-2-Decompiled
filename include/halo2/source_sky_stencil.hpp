#pragma once
#include "halo2/source_weather_depth_alias.hpp"
#include <array>
#include <span>

namespace halo2 {
// Typed subset of captured VSOut: gl_Position at byte0 and v_vtxPos0 at
// byte176 in the retained 240-byte rows. No reconstructed world vertices.
struct SourceSkyStencilVertex {std::array<float,4> clip,window;};
// Exact retained GSOut subset. RenderDoc expands each captured three-vertex
// output strip to a triangle list; all flat lanes must agree within each tri.
struct SourceSkyStencilPostGeometryVertex {
    std::array<float,4> clip,window0,window1,window2;
    float tri_mz;
};
struct alignas(16) SourceSkyStencilUniform {
    std::array<float,4> clip_range;
    std::array<std::array<std::int32_t,4>,8> regions;
    std::array<float,2> depth_factor_offset;
    std::array<std::int32_t,2> surface_scale;
};
static_assert(sizeof(SourceSkyStencilVertex)==32);
static_assert(sizeof(SourceSkyStencilPostGeometryVertex)==68);
static_assert(sizeof(SourceSkyStencilUniform)==160);
static_assert(offsetof(SourceSkyStencilPostGeometryVertex,window0)==16);
static_assert(offsetof(SourceSkyStencilPostGeometryVertex,tri_mz)==64);
static_assert(offsetof(SourceSkyStencilUniform,regions)==16);
static_assert(offsetof(SourceSkyStencilUniform,depth_factor_offset)==144);
static_assert(offsetof(SourceSkyStencilUniform,surface_scale)==152);
struct SourceSkyStencilDraw {
    std::span<const SourceSkyStencilVertex> vertices;
    std::span<const std::uint32_t> indices;
    SourceSkyStencilUniform uniform;
    // Captured GS cyclic rotations for even/odd strip primitives. Values
    // 0/1/2 mean (0,1,2)/(2,0,1)/(1,2,0), respectively; never inferred from
    // a mesh label or from expected stencil pixels.
    std::array<std::uint32_t,2> rotations;
};
// CPU-only guards, shared by fixture and prepare().
void validate_source_sky_stencil_draw(const SourceSkyStencilDraw &);
struct SourceSkyStencilPostGeometryDraw {
    std::span<const SourceSkyStencilPostGeometryVertex> vertices;
    SourceSkyStencilUniform uniform;
};
void validate_source_sky_stencil_draw(const SourceSkyStencilPostGeometryDraw &);
struct SourceSkyStencilContext {
    VkPhysicalDevice physical{};
    VkDevice device{};
    std::uint32_t queue_family{};
    std::filesystem::path shader_directory;
    // Caller facts about enabled device features; physical support alone
    // does not prove vkCreateDevice enabled them.
    bool geometry_shader_enabled{},depth_clamp_enabled{};
    // Native D24 remains the default. D32S8 requires the canonical source
    // integer storage declaration on both the context and actual image.
    SourceDepthStorageEncoding depth_encoding=SourceDepthStorageEncoding::native_d24;
};
struct SourceSkyStencilRecord {
    VkDevice command_owner{};
    VkCommandBuffer command{};
    VkFence completion{};
    std::uint32_t command_queue_family{};
    bool outside_render_pass{},exclusive_access{};
};
// Isolated stencil-only retained-geometry diagnostic. Fixed triangle strip,
// cull NONE, depth clamp, late LEQUAL/no depth writes, ALWAYS/KEEP/KEEP/
// REPLACE FF with both masks FF. Owns no color attachment and no sky textures.
// prepare() copies typed retained geometry and source uniforms, and borrows
// an actual current D24S8 or canonical D32S8 image. record() runs outside a caller render pass,
// restores its supplied initialized layout, and makes stencil writes visible.
// One caller-owned reset/unsignaled fence MUST cover the recorded command
// and all consumers; keep image, device and fence alive until retire(). A
// successful record must be submitted exactly once, not discarded. No other
// image accesses, queue transfers, prepares or records before retirement.
// Destruction waits for a pending fence; the submission contract makes that
// wait valid. No live Renderer activation or complete sky appearance claim.
// When geometryShader is disabled, retained strip indices are assembled with
// integer operations into ordered per-instance triangles. The vertex shader
// executes the same source plane arithmetic; no CPU floating reconstruction.
class SourceSkyStencil {
public:
    explicit SourceSkyStencil(SourceSkyStencilContext);
    ~SourceSkyStencil();
    SourceSkyStencil(const SourceSkyStencil &)=delete;
    SourceSkyStencil &operator=(const SourceSkyStencil &)=delete;
    void prepare(const SourceWeatherDepthAliasImage &,const SourceSkyStencilDraw &);
    // Independent downstream control: copied actual GSOut clip/flat lanes,
    // with no authored or retained VS arithmetic regenerated.
    void prepare(const SourceWeatherDepthAliasImage &,const SourceSkyStencilPostGeometryDraw &);
    void record(const SourceSkyStencilRecord &);
    void retire(VkFence);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
