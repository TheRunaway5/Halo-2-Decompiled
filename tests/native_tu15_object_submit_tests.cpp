// Private 1362A0 physical-original comparison harness (not a production
// target). Fixtures come from tools/validate_native_object_submit_tu15.py.
#include "rendering/native_tu15_object_submit.hpp"
#include "native_tu15_shadow_frame_harness.hpp"
#ifndef HALO2_SHADOW_FRAME_MUTATION_CORE_ONLY
namespace copies {
using namespace physics_world_runtime_detail;
using WorldState=InstrumentedState;
#include "object_submit_bodies.inc"
}
#endif
struct Owner {
#ifdef HALO2_SHADOW_FRAME_MUTATION_CORE_ONLY
    static constexpr bool core_only=true;
#else
    static constexpr bool core_only=false;
    static void retained(InstrumentedState&s){copies::retained::run_1362a0(s);}
    static void bound(InstrumentedState&s){copies::bound::run_1362a0(s);}
    static void prepared(InstrumentedState&s){copies::prepared::run_1362a0(s);}
#endif
    static constexpr const char*name="object submit";
    static constexpr const char*reference_test_variable="HALO2_OBJECT_SUBMIT_REFERENCE_TEST";
    static constexpr const char*census_test_variable="HALO2_OBJECT_SUBMIT_CENSUS_TEST";
    static bool reference(){return native_object_submit_detail::reference();}
    template<class S>static bool policy_admitted(const S&s){return native_object_submit_detail::policy_admitted(s);}
    template<class S,class Call,class Before>static bool native(S&s,Call&call,Before&before){return native_object_submit_detail::bridge(s,call,before);}
    template<class S,class Call>static bool production(S&s,Call&call){return native_object_submit_detail::bridge(s,call);}
    // 4BAE0, 135E00, 4B5C0 and 1365C0 are all separately owned stubbed callees.
    template<class S,class Observe>static bool child(S&,std::uint32_t,std::uint32_t,Observe&){return false;}
};
int main(int argc,char**argv){return shadow_frame_harness::run<Owner>(argc,argv);}
