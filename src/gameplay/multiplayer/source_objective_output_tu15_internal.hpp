#pragma once
#include "halo2/source_objective_waypoint_tu15.hpp"

namespace halo2::source_objective_output_tu15_detail {
// Scoped to the real synchronous presentation query on this executor.
// The output owns values; it never retains a caller/world memory borrow.
inline thread_local SourceObjectivePresentationTu15* output=nullptr;
inline thread_local std::vector<SourceObjectiveQuadTu15>* quads=nullptr;
inline thread_local std::uint32_t emblem_player=0xffffffffu;
inline thread_local std::array<std::uint32_t,2> font_headers{};
template<class State>bool font(State& state) {
    if(!font_headers[0])return false;
    if(state.gp[1]<5 || state.gp[1]>6)throw std::runtime_error("Objective font lookup requested an unbound original selector");
    state.gp[0]=font_headers[state.gp[1]-5];state.pop();return true;
}
template<class State>bool goal(State& state) {
    if(!output)return false;
    const auto r=[&](std::uint32_t at,unsigned width=4){return physics_world_runtime_detail::r(state.memory,at,width);};
    const auto sp=state.gp[4];
    if(r(sp+12)!=0x44200000u || r(sp+16)!=0xd7100u || r(sp+20)!=0x24d760u || r(sp+24)!=0x24d900u ||
        r(sp+8)!=0xffffffffu || r(sp+4)!=0 || output->goals.size()>=18)
        throw std::runtime_error("Objective goal output has a different original render contract");
    const auto designator=std::bit_cast<std::int32_t>(r(sp+28));
    const auto bank=r(0x52d710u),scenario=r(0x4ecc20u);
    const auto index=designator>=0?designator:~designator;
    if(index<0 || index>=9)throw std::runtime_error("Objective goal designator differs");
    const auto marker=std::bit_cast<std::int16_t>(std::uint16_t(r(bank+(designator>=0?0x14u:0x26u)+unsigned(index)*2,2)));
    if(marker<0 || unsigned(marker)>=r(scenario+0x118))throw std::runtime_error("Objective goal has no authored marker");
    const auto point=[&](std::uint32_t at){return Vec3{std::bit_cast<float>(r(at)),std::bit_cast<float>(r(at+4)),std::bit_cast<float>(r(at+8))};};
    output->goals.push_back({state.gp[0],designator,point(r(scenario+0x11c)+unsigned(marker)*32),point(r(0x474c70u)),
        std::bit_cast<float>(r(bank+(designator>=0?0x3cu:0xa8u)+unsigned(index)*12)),std::bit_cast<float>(r(0x48c7b4u))});
    state.pop();state.gp[4]+=32;return true;
}
template<class State>bool emblem(State& state) {
    if(!quads)return false;
    emblem_player=state.gp[0];state.pop();return true;
}
template<class State>bool quad(State& state) {
    if(!quads)return false;
    if(quads->size()>=512)throw std::runtime_error("Original objective UI quad capacity differs");
    SourceObjectiveQuadTu15 value;
    state.memory.read(state.gp[1],std::as_writable_bytes(std::span(value.positions)));
    state.memory.read(state.gp[0],std::as_writable_bytes(std::span(value.uv)));
    const auto r=[&](std::uint32_t at){return physics_world_runtime_detail::r(state.memory,at,4);};
    value.bitmap=r(state.gp[4]+4);value.frame=r(state.gp[4]+8);value.shader=r(state.gp[4]+12);
    value.emblem_player=emblem_player;
    state.memory.read(0x4f3220u,std::as_writable_bytes(std::span(value.colors)));
    state.memory.read(0x4f3238u,std::as_writable_bytes(std::span(value.inputs)));
    quads->push_back(value);state.pop();state.gp[4]+=12;return true;
}
struct Scope {
    SourceObjectivePresentationTu15* previous;
    std::array<std::uint32_t,2> previous_fonts;
    explicit Scope(SourceObjectivePresentationTu15& current,std::array<std::uint32_t,2> fonts={}):previous(output),previous_fonts(font_headers){output=&current;font_headers=fonts;}
    ~Scope(){output=previous;font_headers=previous_fonts;}
};
template<class State>bool waypoint(State& state) {
    if(!output)return false;
    const auto address=physics_world_runtime_detail::r(state.memory,state.gp[4]+4u,4);
    SourceObjectiveWaypointTu15 value;
    state.memory.read(address,std::span(value.bytes).first(2));
    state.memory.read(address+4u,std::span(value.bytes).subspan(4,0x44));
    const auto count=physics_world_runtime_detail::r(state.memory,address+0x44u,4);
    if(count>2 || output->waypoints.size()>=64)throw std::runtime_error("Original objective waypoint region/caller capacity differs");
    state.memory.read(address+0x48u,std::span(value.bytes).subspan(0x48,std::size_t(count)*0x24));
    output->waypoints.push_back(std::move(value));
    state.pop();state.gp[4]+=4u;return true;
}
} // namespace halo2::source_objective_output_tu15_detail
