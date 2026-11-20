// Private 4BAE0 physical-original comparison harness (not a production
// target). Fixtures come from tools/validate_native_object_fade_tu15.py.
#include "rendering/native_tu15_object_fade.hpp"
#include "native_tu15_shadow_frame_harness.hpp"
#ifndef HALO2_SHADOW_FRAME_MUTATION_CORE_ONLY
namespace copies {
using namespace physics_world_runtime_detail;
using WorldState=InstrumentedState;
#include "object_fade_bodies.inc"
}
#endif
struct Owner {
#ifdef HALO2_SHADOW_FRAME_MUTATION_CORE_ONLY
    static constexpr bool core_only=true;
#else
    static constexpr bool core_only=false;
    static void retained(InstrumentedState&s){copies::retained::run_4bae0(s);}
    static void bound(InstrumentedState&s){copies::bound::run_4bae0(s);}
    static void prepared(InstrumentedState&s){copies::prepared::run_4bae0(s);}
#endif
    static constexpr const char*name="object fade";
    static constexpr const char*reference_test_variable="HALO2_OBJECT_FADE_REFERENCE_TEST";
    static constexpr const char*census_test_variable="HALO2_OBJECT_FADE_CENSUS_TEST";
    static bool reference(){return native_object_fade_detail::reference();}
    template<class S>static bool policy_admitted(const S&s){return native_object_fade_detail::policy_admitted(s);}
    template<class S,class Call,class Before>static bool native(S&s,Call&call,Before&before){return native_object_fade_detail::bridge(s,call,before);}
    template<class S,class Call>static bool production(S&s,Call&call){return native_object_fade_detail::bridge(s,call);}
    // Both callees (BCFB0 and _ftol2) are stubbed required owners here.
    template<class S,class Observe>static bool child(S&,std::uint32_t,std::uint32_t,Observe&){return false;}
};
int main(int argc,char**argv){return shadow_frame_harness::run<Owner>(argc,argv);}
