#pragma once
#include "halo2/cache.hpp"
#include "halo2/source_contrail_state.hpp"
#include "halo2/source_contrail_snapshot.hpp"
#include "halo2/source_transparent_sort.hpp"
#include <memory>
namespace halo2 {
class SourceRuntimeMemory;
struct SourceContrailVertex {Vec3 position;Vec2 uv;std::uint32_t color;};
static_assert(sizeof(SourceContrailVertex)==24);
struct SourceContrailDraw {
    std::uint32_t datum{},definition{};unsigned chain{},blend{};
    const Texture *texture{};
    PixelProgram pixel;ShaderTextureState sampler;
    std::array<std::uint32_t,3> blend_operands{};
    std::vector<SourceContrailVertex> vertices;
    std::array<ShaderVector,192> constants{};
    ShaderVector viewport{},clip_range{};
    SourceTransparentCommand command;
};
// Original source point/contrail pools own births, movement and retirement.
// This reader runs42CE0 on caller-owned output and cannot mutate those banks.
class SourceContrailScene {
public:
    explicit SourceContrailScene(const Cache &);
    ~SourceContrailScene();
    std::vector<SourceContrailDraw> draw(const SourcePhysicsContactMemory &,
        Vec3 eye,Vec3 forward,const std::array<ShaderVector,192> &,
        ShaderVector viewport,ShaderVector clip_range)const;
    std::vector<SourceContrailDraw> draw(const SourceContrailSnapshot &,
        Vec3 eye,Vec3 forward,const std::array<ShaderVector,192> &,
        ShaderVector viewport,ShaderVector clip_range)const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
