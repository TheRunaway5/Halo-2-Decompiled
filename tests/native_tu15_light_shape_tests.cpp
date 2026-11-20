// Private C3AB0 physical-original comparison harness (not a production
// target). Fixtures come from tools/validate_native_light_shape_tu15.py.
#include "rendering/native_tu15_light_shape.hpp"
#include "native_tu15_shadow_frame_harness.hpp"
#ifndef HALO2_SHADOW_FRAME_MUTATION_CORE_ONLY
namespace copies {
using namespace physics_world_runtime_detail;
using WorldState=InstrumentedState;
#include "light_shape_bodies.inc"
}
#endif
struct Owner {
#ifdef HALO2_SHADOW_FRAME_MUTATION_CORE_ONLY
    static constexpr bool core_only=true;
#else
    static constexpr bool core_only=false;
    static void retained(InstrumentedState&s){copies::retained::run_c3ab0(s);}
    static void bound(InstrumentedState&s){copies::bound::run_c3ab0(s);}
    static void prepared(InstrumentedState&s){copies::prepared::run_c3ab0(s);}
#endif
    static constexpr const char*name="light shape";
    static constexpr const char*reference_test_variable="HALO2_LIGHT_SHAPE_REFERENCE_TEST";
    static constexpr const char*census_test_variable="HALO2_LIGHT_SHAPE_CENSUS_TEST";
    static bool reference(){return native_light_shape_detail::reference();}
    template<class S>static bool policy_admitted(const S&s){return native_light_shape_detail::policy_admitted(s);}
    template<class S,class Call,class Before>static bool native(S&s,Call&,Before&before){return native_light_shape_detail::bridge(s,before);}
    template<class S,class Call>static bool production(S&s,Call&){return native_light_shape_detail::bridge(s);}
    // C3AB0 is a pure leaf with no callees.
    template<class S,class Observe>static bool child(S&,std::uint32_t,std::uint32_t,Observe&){return false;}
};
int main(int argc,char**argv){return shadow_frame_harness::run<Owner>(argc,argv);}
