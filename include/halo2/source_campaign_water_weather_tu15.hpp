#pragma once
#include "halo2/water.hpp"
#include "halo2/source_weather_world.hpp"
#include "halo2/source_weather_point_render.hpp"
#include "halo2/source_weather_line_render.hpp"
#include "halo2/source_weather_plate_render.hpp"
#include <optional>
#include <utility>

namespace halo2 {
class PhysicsLiveWorldTu15;
using SourceCampaignWaterWeatherStateTu15 = SourceWeatherRenderState;
// The real2B9F0 ->132B0 packet remains separate from the local user. Its
// first short is the weather viewport ordinal; NONE is never mapped to zero.
struct SourceCampaignWaterWeatherViewTu15 {
    std::span<const std::byte,0x298> submitting_packet;
    // Whole2B9F0 publishes camera1 globally and camera2 in this packet.
    // Environment clamping can change camera1 far while camera2 stays raw.
    const SourceViewProjectionBank &collection_rendering;
    const SourceViewProjectionBank &submitting_rendering;
    const SourcePortalVisibilitySet &visibility;
    const BspAtmosphere &atmosphere;
    const BspFogSample &fog;
};
struct SourceCampaignWaterWeatherFrameTu15 {
    std::vector<WaterFrame> water;
    std::vector<Vertex> water_vertices;
    std::optional<Vec3> reflection_eye;
    std::vector<SourceWeatherPointDraw> weather_points;
    std::vector<SourceWeatherLineDraw> weather_lines;
    std::vector<SourceWeatherPlateDraw> weather_plates;
    SourceWeatherWorldStep weather_step;
    std::int16_t source_view{};
    std::int32_t source_user{};
    double source_realtime{};
    float source_elapsed{};
    std::uint32_t auxiliary_before{},auxiliary_after{};
};
class SourceCampaignWaterWeatherTu15;
// Scene material indices belong to this registration, never the retained
// weather resource lifetime. A new/restored scene binds a fresh material table.
class SourceCampaignWaterMaterialBindingsTu15 {
public:
    SourceCampaignWaterMaterialBindingsTu15(const SourceCampaignWaterMaterialBindingsTu15 &)=default;
    SourceCampaignWaterMaterialBindingsTu15(SourceCampaignWaterMaterialBindingsTu15 &&)=default;
    SourceCampaignWaterMaterialBindingsTu15 &operator=(const SourceCampaignWaterMaterialBindingsTu15 &)=default;
    SourceCampaignWaterMaterialBindingsTu15 &operator=(SourceCampaignWaterMaterialBindingsTu15 &&)=default;
    std::size_t material_count()const{return material_count_;}
private:
    friend class SourceCampaignWaterWeatherTu15;
    SourceCampaignWaterMaterialBindingsTu15(const SourceCampaignWaterWeatherTu15 *owner,
        std::vector<unsigned> bases,std::size_t count):owner_(owner),bases_(std::move(bases)),material_count_(count){}
    const SourceCampaignWaterWeatherTu15 *owner_;
    std::vector<unsigned> bases_;
    std::size_t material_count_;
};
// One retained owner per actual Cache/BSP, shared by ordinary and secondary
// campaign views. It owns no simulation clock, intensity, command record or
// random stream. Texture/program spans in completed packets borrow this owner.
class SourceCampaignWaterWeatherTu15 {
public:
    SourceCampaignWaterWeatherTu15(const Cache &,std::size_t bsp);
    // Append before the renderer captures the scene's full material table.
    // Bind pools once all ordinary object/scenery material roots are present.
    SourceCampaignWaterMaterialBindingsTu15 append_water_materials(Mesh &)const;
    void bind_water_pools(std::span<Material>) const;
    const BspWater &water_definitions() const{return water_;}
    const SourceWeatherWorld &weather_world() const{return weather_;}
    // Rendering BSP constructors consume the actual shared auxiliary word
    // before scenario factories. Duplicate initialization is rejected.
    void initialize(PhysicsLiveWorldTu15 &);
    // Called only by the genuine activation notification, before new scene
    // factories. It reruns36AA0 constructors and resets its render globals.
    void activate_after_bsp_switch(PhysicsLiveWorldTu15 &,SourceCampaignWaterWeatherStateTu15);
    // Checkpoint restoration is not activation. Reuse retained mutable banks
    // without RNG, clock/global reset or a duplicate constructor.
    void restore_binding(PhysicsLiveWorldTu15 &)const;
    // Resource age/admission resets once for the whole application frame.
    // This does not advance Simulation or the shared source weather clock.
    void begin_frame(PhysicsLiveWorldTu15 &,std::uint64_t frame);
    SourceCampaignWaterWeatherFrameTu15 collect_view(PhysicsLiveWorldTu15 &,
        SourceCampaignWaterWeatherStateTu15,const SourceCampaignWaterWeatherViewTu15 &,
        const SourceCampaignWaterMaterialBindingsTu15 &);
private:
    const Cache &cache_;
    std::size_t bsp_{};
    BspWater water_;
    SourceWeatherWorld weather_;
    std::map<std::uint32_t,Texture> sprites_,layers_;
    const SourceRuntimeMemory *memory_owner_{};
    std::optional<std::uint64_t> frame_;
    void require_bsp(PhysicsLiveWorldTu15 &) const;
};
}
