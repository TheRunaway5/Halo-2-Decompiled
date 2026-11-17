#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "gameplay/objects/native_tu15_object_lookup_copies_source_owner.hpp"
#include "rendering/effects/native_tu15_particle_type_filter_source_frame.hpp"
#include "native_tu15_object_property_source_test_harness.hpp"

namespace {
using namespace halo2;
using namespace halo2::object_property_source_test;
using halo2::native_particle_type_filter_detail::run_source_frame;
#include "particle_type_filter_source_owners.inc"

struct Profile {
    using Current=current::WorldState;
    using Fresh=fresh::WorldState;
    static constexpr auto label="17DE60";
    static inline bool ordered{};

    static std::string fixture_name(std::uint32_t) {
        throw std::runtime_error("Unexpected 17DE60 named key request");
    }
    static std::uint32_t fixture_key(std::uint32_t key) {return key;}
    static void after_instruction(WorldState& state,std::uint32_t pc,int feedback) {
        if(feedback==1&&pc==0x17deaeu)state.gp[1]^=1u;
    }
    static void after_read(auto& memory,std::uint32_t,std::span<std::byte>) {
        if(memory.feedback==3&&*memory.cursor==0x17dea9u)memory.state->gp[1]^=1u;
    }
    static void after_write(auto&,std::uint32_t,std::span<const std::byte>) {}
    static void after_observer(auto& memory,const SourcePhysicsContactRecordStore& event) {
        if(memory.feedback==2&&event.instruction==0x17de78u)memory.state->gp[3]^=0x100u;
    }
    template<class State> static void run_reference(State& state,unsigned v) {
        retained(state,bool(v&2u),ordered);
    }
    template<class State,bool Literal,bool Ordered> static void run_native_typed(State& state) {
        run_source_frame<Literal,Ordered>(state,[](auto& child){lookup_child(child,Literal);});
    }
    template<class State> static void run_native(State& state,unsigned v) {
        if(v&2u){
            if(ordered)run_native_typed<State,true,true>(state);
            else run_native_typed<State,true,false>(state);
        }else if(ordered)run_native_typed<State,false,true>(state);
        else run_native_typed<State,false,false>(state);
    }
};
} // namespace

int main(int argc,char** argv)try {
    check(argc==2,"Supply genuine 17DE60 source inputs");
    std::ifstream in(argv[1]);const auto rows=J::parse(in);
    const std::array modes{Mode{},Mode{true},Mode{false,false,false,true},
        Mode{false,false,false,false,1},Mode{false,false,false,false,2},Mode{false,false,false,false,3}};
    unsigned comparisons{},stops{},packets{};
    for(const bool ordered:{false,true}){
        Profile::ordered=ordered;
        for(unsigned n=0;n<rows.size();++n){
            const Seed seed{rows[n]};
            for(unsigned context=0;context<4;++context)
                for(unsigned mode=0;mode<modes.size();++mode) {
                    const auto name=std::to_string(n)+"/context"+std::to_string(context)+
                        "/mode"+std::to_string(mode)+(ordered?"/ordered":"/modeled");
                    const auto expected=run<Profile>(seed,modes[mode],context,false);
                    equal<Profile>(run<Profile>(seed,modes[mode],context,true),expected,name);
                    ++comparisons;packets+=expected.trace.size();
                    if(n<9&&(mode==0||mode==1||mode>=3)){
                        for(unsigned at=0;at<expected.trace.size();++at){
                            const auto prefix=run<Profile>(seed,modes[mode],context,false,int(at));
                            check(prefix.stopped,"17DE60 expected stop did not occur");
                            equal<Profile>(run<Profile>(seed,modes[mode],context,true,int(at)),prefix,
                                          name+"/stop"+std::to_string(at));
                            ++stops;packets+=prefix.trace.size();
                        }
                    }
                }
        }
    }
    const auto& sites=Coverage<Profile>::sites;
    check(std::count_if(sites.begin(),sites.end(),[](auto pc){return 0x17de60u<=pc&&pc<=0x17debau;})==27,
          "17DE60 retained owner coverage incomplete");
    std::cout<<J{{"passed",true},{"source_returns",comparisons},{"source_failure_prefixes",stops},
        {"complete_source_packets",packets},{"parent_sites",27},{"real_child_sites",sites.size()-27},
        {"production_bound",false}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
