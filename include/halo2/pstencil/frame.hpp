#pragma once
#include "halo2/pstencil/render.hpp"
#include "halo2/effect_light_draw.hpp"
#include "halo2/pstencil/lightmap.hpp"
#include "halo2/pstencil/point_caster.hpp"

namespace halo2::pstencil {
MenuWorldRasterPacket receiver_raster(MenuWorldRasterPacket);
struct LightBatch {
    std::uint32_t identity;
    Vec3 position;
    std::vector<ShadowVertex> volume;
    std::vector<std::size_t> receivers;
    std::size_t upload{};
    std::vector<const MenuWorldSourceBinding*> source_ranges;
    std::vector<std::size_t> specular_receivers;
    std::size_t dynamic_triangles{};
    std::uint32_t domain{};
};
// Per-view, per-light native adapter over the actual already admitted light
// receiver bank. All source geometry and palettes are borrowed for one draw.
class Frame {
public:
    explicit Frame(std::size_t views):views_(views),lightmaps_(views){}
    void add(std::size_t view,const EffectLightRawDraw&,std::size_t receiver,
        std::span<const std::size_t> specular={},bool collect_caster=true);
    void add_caster(std::size_t view,const PointCaster&);
    void add_casters(std::size_t view,std::span<const Vertex>,std::span<const std::uint8_t> caster_materials);
    void add_lightmap(std::size_t view,const LightmapCaster&,std::span<const std::uint8_t> caster_materials);
    bool empty() const;
    std::string describe() const;
    void prepare(Render&);
    std::span<const LightBatch> lights(std::size_t view) const{return views_.at(view);}
    std::span<const LightBatch> lightmaps(std::size_t view) const{return lightmaps_.at(view);}
private:
    LightBatch& light(std::size_t view,std::uint32_t identity,std::uint32_t domain,Vec3 position);
    void add_source(LightBatch&,const MenuWorldSourceBinding&,unsigned stipple_level=16);
    std::vector<std::vector<LightBatch>> views_;
    std::vector<std::vector<LightBatch>> lightmaps_;
    std::size_t shadow_only_{},without_color_{},fractional_{};
};
} // namespace halo2::pstencil
