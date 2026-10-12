#pragma once
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_weather_wind.hpp"

namespace halo2 {
// weat+0 records are immediate particles; weat+8 records are background
// plates. Neither block is an ordinary render model or material shader.
struct SourceWeatherParticleRender {
    Vec3 position;
    float size{};
    std::uint32_t color{};
};
struct SourceWeatherParticleState {
    Vec3 velocity, noise;
    float mass{}, age{};
};
static_assert(sizeof(SourceWeatherParticleRender)==0x14);
static_assert(sizeof(SourceWeatherParticleState)==0x20);
struct SourceWeatherResource {
    std::array<std::byte,16> descriptor{};
    std::vector<std::byte> bytes;
};
struct SourceWeatherParticles {
    std::array<std::byte,0x8c> authored{};
    std::uint32_t sprites=0xffffffffu;
    Vec3 view_box, initial_velocity;
    float exclusion_radius{},maximum_velocity{},animation_speed{};
    std::array<float,2> mass{},size{},opacity{};
    std::uint32_t maximum_particles{};
    std::uint16_t type{};
    float streak_scale{},line_width{};
    // Literal decompressed resource arrays before 39A80 overwrites them.
    std::vector<SourceWeatherParticleRender> render;
    std::vector<SourceWeatherParticleState> state;
    std::vector<SourceWeatherResource> resources;
    // Complete literal hklb/fklb allocation, including the section header.
    // The runtime resource owner applies223C70 fixups to its private copy.
    std::vector<std::byte> resource_storage;
};
struct SourceWeatherPlate {
    std::array<std::byte,0x3a8> authored{};
    std::array<std::uint32_t,3> textures{};
    std::array<float,3> positions{},texture_scale{},opacity{},mass{};
    std::array<Vec3,3> velocity{},jitter{},tint{};
    std::array<float,2> depth{},depth_blend{};
    unsigned flags{};
};
struct SourceWeatherDefinition {
    std::uint32_t datum{};
    std::string name;
    std::vector<SourceWeatherParticles> particles;
    std::vector<SourceWeatherPlate> plates;
    SourceWeatherWind wind;
    float fade_radius{};
    // Original matg rasterizer global vertex shaders 21/22, class0.
    std::array<std::uint32_t,2> vertex_datums{};
    std::array<std::vector<std::byte>,2> vertex_programs;
    // Complete literal weat root. Runtime wind owns a separate mutable copy;
    // particle/plate callers consume fields through +AC from that same root.
    std::array<std::byte,0xb0> authored{};
};
struct SourceWeatherBsp {
    struct Palette {
        std::array<std::byte,0x88> authored{};
        std::string name;
        std::uint32_t datum=0xffffffffu;
    };
    struct Polyhedron {Vec3 center;float radius{};std::vector<ShaderVector> planes;};
    std::vector<Palette> palette;
    std::vector<std::int16_t> cluster_palette;
    std::vector<Polyhedron> polyhedra;
};
struct SourceWeatherAdmission {
    bool visible{};
    float portal_distance=32767.f;
};
// 36B50 consumes the caller's actual clipped visible-cluster list. A PVS
// upper bound is not interchangeable with that list. Distance uses only
// portals of the camera's containing cluster and the original plane metric.
std::vector<SourceWeatherAdmission> source_weather_admission(
    const SourceWeatherBsp &,const BspAtmosphere &,int current_cluster,
    std::span<const unsigned> visible_clusters,Vec3 eye);
struct SourceWeatherVertex {ShaderVector position;std::uint32_t color{};};
struct SourceWeatherSubmission {
    bool lines{};
    float line_width{};
    std::vector<SourceWeatherVertex> vertices;
};
// 391F0's immediate input arithmetic only. Residency, clocks, simulation,
// current camera constants, point rasterization and viewport scaling are
// deliberately supplied by their separate source producers.
SourceWeatherSubmission source_weather_submission(const SourceWeatherParticles &,
    float source_elapsed,std::int16_t source_view_count,std::int16_t source_view);
// Same391F0 immediate arithmetic over current published runtime rows. The
// system supplies current type/opacity/streak/width; decoded cold copies are
// not consulted. Resource readiness remains the actual caller's query.
SourceWeatherSubmission source_weather_submission(std::span<const std::byte> system,
    std::span<const std::byte> render,std::span<const std::byte> state,
    float source_elapsed,std::int16_t source_view_count,std::int16_t source_view);
PixelProgram source_weather_pixel_program(bool rain);
// 12FA0's c10/c11 for the exact391F0 caller (resolution-scaling flag0).
// Rectangle coordinates address the source raster target. Its output point
// diameter is in that target's pixel units, separate from display resampling.
std::array<ShaderVector,2> source_weather_viewport_constants(
    std::int16_t left,std::int16_t top,std::int16_t right,std::int16_t bottom,
    int source_render_mode=0);
}
