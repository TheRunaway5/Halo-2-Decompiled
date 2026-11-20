// Private 1365C0 physical-original comparison harness (not a production
// target). Fixtures come from tools/validate_native_model_hierarchy_tu15.py.
#include "rendering/native_tu15_model_hierarchy.hpp"
#include "native_tu15_shadow_frame_harness.hpp"
#ifndef HALO2_SHADOW_FRAME_MUTATION_CORE_ONLY
namespace copies {
using namespace physics_world_runtime_detail;
using WorldState=InstrumentedState;
#include "model_hierarchy_bodies.inc"
}
#endif
struct Owner {
#ifdef HALO2_SHADOW_FRAME_MUTATION_CORE_ONLY
    static constexpr bool core_only=true;
#else
    static constexpr bool core_only=false;
    static void retained(InstrumentedState&s){copies::retained::run_1365c0(s);}
    static void bound(InstrumentedState&s){copies::bound::run_1365c0(s);}
    static void prepared(InstrumentedState&s){copies::prepared::run_1365c0(s);}
#endif
    static constexpr const char*name="model hierarchy";
    static constexpr const char*reference_test_variable="HALO2_MODEL_HIERARCHY_REFERENCE_TEST";
    static constexpr const char*census_test_variable="HALO2_MODEL_HIERARCHY_CENSUS_TEST";
    static bool reference(){return native_model_hierarchy_detail::reference();}
    template<class S>static bool policy_admitted(const S&s){return native_model_hierarchy_detail::policy_admitted(s);}
    template<class S,class Call,class Before>static bool native(S&s,Call&call,Before&before){return native_model_hierarchy_detail::bridge(s,call,before);}
    template<class S,class Call>static bool production(S&s,Call&call){return native_model_hierarchy_detail::bridge(s,call);}
    // Recursive 1365C0 calls run through the native bridge; 136A80 and 4B5C0
    // are separately owned stubbed callees.
    template<class S,class Observe>static bool child(S&s,std::uint32_t target,std::uint32_t,Observe&observe){
        if(target!=0x1365c0u)return false;
        auto recurse=[&](auto&c,std::uint32_t t,std::uint32_t site,auto&obs){
            if(!child(c,t,site,obs))c.required(site,t,false);
        };
        if(!native_model_hierarchy_detail::bridge(s,recurse,observe))
            throw std::runtime_error("Recursive 1365C0 bridge refused");
        return true;
    }
};
int main(int argc,char**argv){return shadow_frame_harness::run<Owner>(argc,argv);}
