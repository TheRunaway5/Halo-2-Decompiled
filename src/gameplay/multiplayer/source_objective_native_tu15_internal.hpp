#pragma once
#include "halo2/source_objective_native_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cerrno>
#include <cstdlib>

namespace halo2::source_objective_native_tu15_detail {
using namespace physics_world_runtime_detail;
inline const bool reference=[] {
    const auto saved=errno;
    const auto requested=[](const char* name){const auto* value=std::getenv(name);return value && *value;};
    const bool enabled=requested("HALO2_REFERENCE_OBJECTIVE_NATIVE_KERNELS") || requested("HALO2_REFERENCE_LITERAL_INSTRUCTIONS");
    errno=saved;return enabled;
}();
inline bool admitted(const WorldState& s) noexcept {
    if(reference)return false;
    const auto *cursor=s.instruction_cursor;
    return !s.services.instruction || (s.literal_service_sites && cursor &&
        !(cursor->first_trace && *cursor->first_trace) &&
        !(cursor->second_trace && *cursor->second_trace));
}
inline void cursor(WorldState& s,std::uint32_t pc) {
    if(s.services.instruction)*s.instruction_cursor->position=pc;
}
inline std::int32_t signed_half(std::uint32_t value) {
    return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
inline std::uint32_t extend(std::uint32_t value,unsigned width) {
    return std::uint32_t(width==1?std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))):signed_half(value));
}
inline std::uint32_t single_spill(long double value,std::uint16_t& status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    float result;std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        :"=m"(result),"=m"(actual):"m"(value):"st");
    status=(status&~0x200u)|(actual&0x200u);return std::bit_cast<std::uint32_t>(result);
#else
    throw std::runtime_error("Required objective extended-to-single backend unavailable");
#endif
}
inline std::uint32_t read(WorldState& s,std::uint32_t pc,std::uint32_t at,unsigned width=4) {
    cursor(s,pc);return r(s.memory,at,width);
}
inline void store(WorldState& s,std::uint32_t pc,std::uint32_t at,unsigned width,std::uint32_t value) {
    cursor(s,pc);w(s.memory,s.observe,pc,at,width,value);
}
inline void push(WorldState& s,std::uint32_t pc,std::uint32_t value) {
    cursor(s,pc);s.push(pc,value);
}
template<class State,class Dispatch>void call(State& s,Dispatch&& dispatch,
    std::uint32_t entry,std::uint32_t pc,std::uint32_t rejoin) {
    push(s,pc,rejoin);dispatch(s,entry,pc,false);
}
// Keep the compatibility boundary's read/fault order, complete volatile ABI,
// flags at its observable store and original write notification. Rule math
// executes as ordinary native integers, without a per-instruction flag loop.
inline bool arm(WorldState& s) {
    if(!admitted(s))return false;
    cursor(s,0x249866u);s.gp[1]=r(s.memory,0x51df54u,4);
    cursor(s,0x24986cu);s.gp[2]=std::uint32_t(signed_half(r(s.memory,s.gp[1]+2u,2)));
    cursor(s,0x249870u);s.gp[0]=r(s.memory,0x4f3218u,4);
    cursor(s,0x249875u);s.gp[0]=std::uint32_t(signed_half(r(s.memory,s.gp[0]+0x244u,2)));
    cursor(s,0x24987cu);s.gp[1]=std::uint32_t(signed_half(r(s.memory,s.gp[5]+0x1a0u+s.gp[7]*2u,2)));
    const auto previous=signed_half(s.gp[1]);
    const auto step=source_assault_arming_tick(std::int16_t(previous),
        std::int16_t(signed_half(s.gp[0])),std::int16_t(signed_half(s.gp[2])));
    s.gp[0]=std::uint32_t(step.threshold);s.gp[1]=std::uint32_t(step.progress);
    s.flags.minus(std::uint32_t(previous+1),s.gp[0],4);
    cursor(s,0x24988du);w(s.memory,s.observe,0x24988du,s.gp[5]+0x1a0u+s.gp[7]*2u,2,s.gp[1]);
    s.gp[1]=std::uint32_t(signed_half(s.gp[1]));
    s.flags.minus(s.gp[1],s.gp[0],4);cursor(s,0x249898u);
    ++source_objective_native_tu15_stats.arming;return true;
}
inline bool decay(WorldState& s) {
    if(!admitted(s) || s.gp[6]!=0xffffffffu)return false;
    cursor(s,0x24991cu);s.gp[0]=std::uint32_t(signed_half(r(s.memory,s.gp[5]+0x1a0u+s.gp[7]*2u,2)));
    const auto previous=signed_half(s.gp[0]);
    s.gp[0]=std::uint32_t(source_assault_arming_decay(std::int16_t(previous)));
    s.flags.minus(std::uint32_t(previous-3),s.gp[6],4);
    cursor(s,0x24992cu);w(s.memory,s.observe,0x24992cu,s.gp[5]+0x1a0u+s.gp[7]*2u,2,s.gp[0]);
    ++source_objective_native_tu15_stats.decay;return true;
}
}
#include "objective_native/configuration.hpp"
#include "objective_native/queries.hpp"
#include "objective_native/initialization.hpp"
#include "objective_native/traits.hpp"
#include "objective_native/pickup.hpp"
#include "objective_native/activation.hpp"
#include "objective_native/objects.hpp"
#include "objective_native/entity.hpp"
#include "objective_native/overtime.hpp"
#include "objective_native/carrier.hpp"
#include "objective_native/geometry.hpp"
#include "objective_native/spawn.hpp"
#include "objective_native/markers.hpp"
#include "objective_native/player_tick.hpp"
#include "objective_native/events.hpp"
#include "objective_native/allied_carrier.hpp"
#include "objective_native/regions.hpp"
#include "objective_native/registry.hpp"
#include "objective_native/item_events.hpp"
#include "objective_native/round_tick.hpp"
#include "objective_native/goal_queries.hpp"
#include "objective_native/goal_extents.hpp"
#include "objective_native/plant.hpp"
#include "objective_native/touch_return.hpp"
#include "objective_native/return_reset.hpp"
#include "objective_native/object_tick.hpp"
#include "objective_native/indicators.hpp"
