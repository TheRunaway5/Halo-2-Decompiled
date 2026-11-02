#include "halo2/native_tu15_render_lighting.hpp"
#include "halo2/native_tu15_render_prepare.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <atomic>
#include <cstdlib>
#include <iostream>

namespace halo2 {
namespace {
struct Stats {
    std::atomic<std::uint64_t> sections{},lighting{},geometry{},requests{};
    ~Stats(){if(const auto* v=std::getenv("HALO2_NATIVE_RENDER_ADMISSION_STATS");v&&*v)
        std::cerr<<"Native render admission stats: sections="<<sections<<" lighting="<<lighting<<
            " geometry="<<geometry<<" requests="<<requests<<'\n';}
};
Stats& stats(){static Stats s;return s;}
NativeTu15RenderUnitReader reader(const SourceCullDrawUnit& unit) {
    return [&unit](std::uint32_t offset){return unit.words.at(offset/4u);};
}
std::uint16_t half(const SourcePhysicsContactMemory& memory,std::uint32_t at) {
    std::array<std::byte,2> bytes{};memory.read(at,bytes);
    return std::uint16_t(std::to_integer<unsigned>(bytes[0])|(std::to_integer<unsigned>(bytes[1])<<8));
}
}
SourceDrawUnitServices source_draw_resource_services_tu15(PhysicsLiveWorldTu15& world,NativeTu15PhysicsFrameFp& fp) {
    SourceDrawUnitServices services;
    services.geometry=[&world](std::uint16_t,std::uint32_t selector,const SourceCullDrawUnit& unit) {
        ++stats().sections;const auto& memory=world.memory();
        const auto section=native_tu15_render_section_44930(memory,reader(unit),std::uint8_t(selector));
        const auto classification=half(memory,section+0x14u),subparts=half(memory,section+0x24u);
        return SourceDrawUnitGeometry{classification,subparts};
    };
    services.lighting=[&world,&fp](auto owner,auto model,auto instance) {
        ++stats().lighting;return world.classify_render_lighting_tu15(owner,model,instance,fp);
    };
    services.resident=[&world,&fp](std::uint16_t,std::uint32_t selector,bool request,const SourceCullDrawUnit& unit) {
        ++stats().geometry;
        return native_tu15_render_geometry_449d0(world.memory(),reader(unit),std::uint8_t(request),std::uint8_t(selector),fp,
            [&](auto header,auto& arithmetic) {++stats().requests;return world.request_render_geometry_tu15(3u,header,arithmetic);})!=0;
    };
    return services;
}
}
