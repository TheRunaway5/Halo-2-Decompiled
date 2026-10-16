#include "halo2/source_object_functions_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <bit>
namespace halo2 {
void source_object_function_set_tu15(PhysicsLiveWorldTu15 &w,std::uint32_t object,
    std::uint32_t name,float value,float frames) {
    auto a=w.prepare_source_call(std::array{object,name,std::bit_cast<std::uint32_t>(value)});
    a.xmm[4][0]=std::bit_cast<std::uint32_t>(frames);w.run_actual(0x10fc30u,a);
}
void source_object_function_clear_tu15(PhysicsLiveWorldTu15 &w,std::uint32_t object,std::uint32_t name) {
    auto a=w.prepare_source_call();a.gp[7]=object;a.gp[6]=name;w.run_actual(0x10fd50u,a);
}
std::optional<float> source_object_function_value_tu15(PhysicsLiveWorldTu15 &w,
    std::uint32_t object,std::uint32_t name) {
    const auto scratch=w.prepare_source_call().gp[4]+0x100u;
    auto a=w.prepare_source_call(std::array{name,scratch,scratch+4u});a.gp[3]=object;
    w.run_actual(0x10fb70u,a);if(!(a.gp[0]&255u))return {};
    return Bytes(w.memory().view(scratch,4)).f32(0);
}
std::map<std::pair<std::uint32_t,std::uint32_t>,float> source_object_function_samples_tu15(PhysicsLiveWorldTu15 &w) {
    auto &m=w.memory();const auto p=Bytes(m.view(0x51daf4u,4)).u32(0);
    if(!p)throw FormatError("Source shader functions lack their original slot-bank constructor");
    const Bytes bank(m.view(p,0x300u));std::map<std::pair<std::uint32_t,std::uint32_t>,float> out;
    for(unsigned slot=0;slot<32;++slot) {
        const auto object=bank.u32(0x280u+slot*4u);if(object==0xffffffffu)continue;
        const auto name=bank.u32(slot*0x14u);
        const auto value=source_object_function_value_tu15(w,object,name);
        if(!value)throw FormatError("Original occupied shader-function slot did not resolve");
        out.emplace(std::pair{object,name},*value);
    }
    return out;
}
}
