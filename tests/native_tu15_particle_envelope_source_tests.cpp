#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "rendering/effects/native_tu15_particle_envelope_source_frame.hpp"
#include "native_tu15_object_property_source_test_harness.hpp"

namespace {
using namespace halo2;
using namespace halo2::object_property_source_test;
using halo2::native_particle_envelope_detail::run_source_frame;
#include "particle_envelope_source_owners.inc"

struct Profile {
    using Current=current::WorldState;
    using Fresh=fresh::WorldState;
    static constexpr auto label="252EC0";

    static std::string fixture_name(std::uint32_t) {
        throw std::runtime_error("Unexpected 252EC0 named key request");
    }
    static std::uint32_t fixture_key(std::uint32_t key) {return key;}
    static void after_instruction(WorldState& state,std::uint32_t pc,int feedback) {
        // A raw callback may change the live time before its comparison.
        if(feedback==1&&pc==0x252ed4u)state.xmm[1]=_mm_xor_ps(state.xmm[1],_mm_set_ss(-0.0f));
    }
    static void after_read(auto& memory,std::uint32_t,std::span<std::byte>) {
        if(memory.feedback==3&&*memory.cursor==0x252ee9u)
            memory.state->xmm[1]=_mm_add_ss(memory.state->xmm[1],_mm_set_ss(1.0f));
    }
    static void after_write(auto&,std::uint32_t,std::span<const std::byte>) {}
    static void after_observer(auto&,const SourcePhysicsContactRecordStore&) {}
    template<class State> static void run_reference(State& state,unsigned v) {retained(state,bool(v&2u));}
    template<class State> static void run_native(State& state,unsigned v) {
        if(v&2u)run_source_frame<true>(state);else run_source_frame<false>(state);
    }
};
} // namespace

int main(int argc,char** argv)try {
    check(argc==2,"Supply genuine 252EC0 source inputs");
    std::ifstream in(argv[1]);const auto rows=J::parse(in);
    const std::array modes{Mode{},Mode{true},Mode{false,false,false,true},
        Mode{false,false,false,false,1},Mode{false,false,false,false,3}};
    unsigned comparisons{},stops{},packets{};
    for(unsigned n=0;n<rows.size();++n){
        const Seed seed{rows[n]};
        for(unsigned context=0;context<4;++context)
            for(unsigned mode=0;mode<modes.size();++mode) {
                const auto name=std::to_string(n)+"/context"+std::to_string(context)+"/mode"+std::to_string(mode);
                const auto expected=run<Profile>(seed,modes[mode],context,false);
                equal<Profile>(run<Profile>(seed,modes[mode],context,true),expected,name);
                ++comparisons;packets+=expected.trace.size();
                if(n<32&&mode!=2){
                    for(unsigned at=0;at<expected.trace.size();++at){
                        const auto prefix=run<Profile>(seed,modes[mode],context,false,int(at));
                        check(prefix.stopped,"252EC0 expected stop did not occur");
                        equal<Profile>(run<Profile>(seed,modes[mode],context,true,int(at)),prefix,
                                      name+"/stop"+std::to_string(at));
                        ++stops;packets+=prefix.trace.size();
                    }
                }
            }
    }
    const auto& sites=Coverage<Profile>::sites;
    check(sites.size()==30,"252EC0 retained owner coverage incomplete");
    std::cout<<J{{"passed",true},{"source_returns",comparisons},{"source_failure_prefixes",stops},
        {"complete_source_packets",packets},{"parent_sites",30},{"production_bound",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
