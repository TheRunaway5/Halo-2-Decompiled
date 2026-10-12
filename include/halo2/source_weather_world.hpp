#pragma once
#include "halo2/source_weather_assets.hpp"
#include "halo2/source_weather.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_portal_visibility.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/source_weather_particle_simulation.hpp"
#include <map>
#include <optional>

namespace halo2 {
// A synchronous native geometry allocation. The literal cache block and its
// descriptor bank are owned here. Publication validates weather's type0
// inputs then executes complete shared223C70; request retains12DE70's age touch.
// Tokens identify this private native address space, not recovered Xbox heap
// addresses. Its one registered block has native index0, with no Xbox salt or
// async/eviction claim. No published pointer addresses decoded-copy vectors.
class SourceWeatherParticleResource {
public:
    explicit SourceWeatherParticleResource(const SourceWeatherParticles &,
                                           std::uint32_t native_base=0x2100000);
    bool request(std::int32_t cache_clock);
    bool published()const;
    std::optional<std::int32_t> last_use()const{return last_use_;}
    void rebase_age(); //13D230 for this registered block at INT_MAX.
    std::span<std::byte> system();
    std::span<const std::byte> system()const;
    std::span<std::byte> render_rows();
    std::span<std::byte> state_rows();
    std::span<const std::byte> render_rows()const;
    std::span<const std::byte> state_rows()const;
    SourceWeatherSubmission submission(float elapsed,std::int16_t view_count,std::int16_t slot)const;
    std::span<const std::byte> raw_storage()const;
    std::uint32_t system_token()const{return base_;}
    std::uint32_t raw_token()const{return raw_;}
private:
    SourceRuntimeMemory memory_;
    std::uint32_t base_{},raw_{},raw_size_{};
    std::optional<std::int32_t> last_use_;
    void publish();
    std::span<std::byte> rows(unsigned locator,unsigned stride);
    std::span<const std::byte> rows(unsigned locator,unsigned stride)const;
};
struct SourceWeatherWorldDefinition {
    SourceWeatherDefinition definition;
    std::vector<SourceWeatherParticleResource> particles;
    std::array<std::byte,0xb0> weather{};
};
// The fields actually read by37260/38450, produced from this native view's
// submitting camera and the existing original2FD90 transform producer.
// Unconsumed storage is zero; this is not a complete original0x298 packet.
std::array<std::byte,0xf4> source_weather_view_inputs(std::int16_t slot,Vec3 eye,
    Vec3 forward,float projection_angle,const SourceViewProjectionBank &);
struct SourceWeatherWorldSimulation {
    std::int16_t view_count{};
    float elapsed{}; // Existing shared weather clock, not an independent tick.
    std::uint16_t x87_control{};
    std::uint32_t mxcsr{};
    SourceWeatherParticleRsqrt rsqrt;
};
struct SourceWeatherWorldDrawServices {
    std::function<bool(std::uint32_t datum,std::size_t index,
        std::span<const std::byte> plate,std::span<const std::byte> weather,unsigned slot)> plate;
    std::function<bool(std::uint32_t datum,std::size_t index,
        SourceWeatherParticleResource &,float elapsed,std::int16_t view_count,std::int16_t slot)> particle;
};
struct SourceWeatherWorldStep {
    std::size_t palettes{},wind_steps{},plate_steps{},particle_steps{},particle_rows{},
        rsqrt_calls{},plate_draws{},particle_draws{};
};
// Match/BSP lifetime owner, shared by every local view. Construction publishes
// genuinely loaded native resources; it does not choose an auxiliary RNG seed
// or call initialization on a private stream. An actual stream owner must call
// initialize() in36AB0/39880's palette order before simulation/submission
// admission. Intensity, retained commands and the shared render clock remain
// with their existing simulation owner; this class keeps no duplicate state.
class SourceWeatherWorld {
public:
    SourceWeatherWorld(const Cache &,std::size_t bsp);
    const SourceWeatherBsp &bsp()const{return bsp_;}
    const std::map<std::uint32_t,SourceWeatherWorldDefinition> &definitions()const{return definitions_;}
    SourceWeatherWorldDefinition &definition(std::uint32_t);
    void initialize(std::uint32_t &actual_auxiliary);
    bool initialized()const{return initialized_;}
    void begin_frame();
    std::int32_t cache_clock()const{return cache_clock_;}
    std::span<const SourceWeatherAdmission> admit_view(unsigned slot,const BspAtmosphere &,
        int containing_cluster,const SourcePortalVisibilitySet &,Vec3 eye,const SourceWeatherState &);
    std::span<const SourceWeatherAdmission> admission(unsigned slot)const;
    // Publish actual current view inputs and admit with their same eye/slot.
    std::span<const SourceWeatherAdmission> admit_view(std::span<const std::byte> view,
        const BspAtmosphere &,int containing_cluster,const SourcePortalVisibilitySet &,
        const SourceWeatherState &);
    std::span<const std::byte> view_inputs(unsigned slot)const;
    //36B50's palette loop, after the existing shared clock/state owner and
    // actual clipped-view admission. Wind only runs on view0. Each palette's
    // plate/particle update chain precedes its independent draw chain. Draw
    // services must execute actual preparation/submission and return their AL;
    // no readiness, RNG, elapsed time or ISA answer is manufactured here.
    // Completed earlier phases remain completed if an external service fails.
    SourceWeatherWorldStep visit_view(unsigned slot,const SourceWeatherState &,
        const SourceWeatherWorldSimulation &,std::uint32_t &actual_auxiliary,
        const SourceWeatherWorldDrawServices &);
    // Same original producer prefix, ending at the draw boundary. A native
    // caller can advance actual owned rows before its GPU consumer is ready,
    // without manufacturing successful plate/particle draw-service returns.
    SourceWeatherWorldStep simulate_view(unsigned slot,const SourceWeatherState &,
        const SourceWeatherWorldSimulation &,std::uint32_t &actual_auxiliary);
private:
    SourceWeatherWorldStep visit_view_impl(unsigned,const SourceWeatherState &,
        const SourceWeatherWorldSimulation &,std::uint32_t &,
        const SourceWeatherWorldDrawServices *);
    SourceWeatherBsp bsp_;
    std::map<std::uint32_t,SourceWeatherWorldDefinition> definitions_;
    std::array<std::vector<SourceWeatherAdmission>,4> admissions_;
    std::array<std::array<std::byte,0xf4>,4> views_{};
    std::array<bool,4> view_valid_{};
    std::int32_t cache_clock_{};
    bool initialized_{};
};
}
