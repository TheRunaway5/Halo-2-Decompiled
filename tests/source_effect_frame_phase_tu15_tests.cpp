#include "halo2/source_effect_frame_tu15.hpp"
#include "halo2/campaign_frame_clock.hpp"
#include <iostream>
#include <limits>
#include <vector>
using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
struct Call {std::uint32_t entry;std::vector<std::uint32_t> arguments;bool operator==(const Call &)const=default;};
struct World {
    std::vector<Call> calls;
    unsigned call_actual(unsigned entry,std::span<const unsigned> args) {calls.push_back({entry,{args.begin(),args.end()}});return 0;}
    void advance_effect_particles(float seconds,std::nullptr_t) {calls.push_back({0x179df0u,{std::bit_cast<unsigned>(seconds)}});}
};
}
int main()try {
    struct Case {std::uint32_t seconds;int requested,completed;std::uint32_t accepted;};
    // Retained accepted-budget fixtures include zero-tick positive elapsed,
    // a partially completed batch, and stopping before the first game tick.
    constexpr std::array cases{Case{0,0,0,0},Case{1007192201u,0,0,1007192201u},
        Case{1023969417u,1,1,1023969417u},Case{1036831949u,3,3,1036831949u},
        Case{1039516303u,3,3,1039516303u},Case{1039516303u,3,1,1029338124u},
        Case{1039516303u,3,0,0}};
    for(const auto &input:cases) {
        const auto accepted=accepted_campaign_frame_elapsed(std::bit_cast<float>(input.seconds),input.requested,input.completed);
        check(std::bit_cast<std::uint32_t>(accepted)==input.accepted,"Accepted source interval differs from retained budget fixture");
        World world;source_effect_frame_tu15(world,accepted,nullptr);
        const std::vector<std::uint32_t> delta{input.accepted};
        const auto expected=input.accepted?std::vector<Call>{{0x17ead0u,delta},{0x180520u,{}},{0x1378e0u,delta},{0x179df0u,delta}}:
            std::vector<Call>{{0x179df0u,delta}};
        check(world.calls==expected,"Positive-only environment mixer changed original selected-owner order/accepted argument");
    }
    for(const float invalid:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        World world;bool rejected{};try{source_effect_frame_tu15(world,invalid,nullptr);}catch(const FormatError &){rejected=true;}
        check(rejected && world.calls.empty(),"Rejected source interval reached a native frame owner");
    }
    std::cout<<"PASS7 accepted-budget fake-owner phases:positive1378E0 exactly once, zero-frame179DF0 only,3invalid deltas rejected before calls\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
