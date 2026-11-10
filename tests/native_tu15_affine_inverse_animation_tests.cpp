// Reuse the independent complete packet/bank/fault harness with the genuine
// fixed-1.1 animation State, including its distinct Flags and software-MMX ABI.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_affine_inverse.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"
namespace halo2::animation_inverse_proof {
using namespace physics_contact_runtime_detail;
#include "inverse_animation_state.inc"
}
namespace halo2 {
class AnimationInverseProofState:public animation_inverse_proof::State {
    static const SourceUnitAnimationRuntimeServices&owners(){static const SourceUnitAnimationRuntimeServices value;return value;}
    static std::uint64_t preserved(unsigned index){return 0xfedcba9876543210ull^(0x1111111111111111ull*index);}
public:
    AnimationInverseProofState(SourcePhysicsContactMemory&memory,const SourcePhysicsContactRecordObserver&observer,
        const SourcePhysicsWorldRuntimeServices&services,SourcePhysicsContactImage image)
        :animation_inverse_proof::State{memory,observer,services,owners()}{
        if(image!=SourcePhysicsContactImage::MapPack11)throw std::runtime_error("Animation inverse owns only its fixed1.1 image");
    }
    void load(const SourcePhysicsWorldRuntimeAbi&incoming){
        SourceUnitAnimationRuntimeAbi value{};
        static_cast<SourcePhysicsContactRuntimeAbi&>(value)=incoming;
        for(unsigned n=0;n<8;++n)value.mmx[n]=preserved(n);
        animation_inverse_proof::State::load(value);
    }
    SourceUnitAnimationRuntimeAbi save()const{
        auto value=animation_inverse_proof::State::save();
        for(unsigned n=0;n<8;++n)if(value.mmx[n]!=preserved(n))throw std::runtime_error("Animation inverse modified its separate softwareMMX ABI");
        return value;
    }
};
}
namespace halo2::native_affine_inverse_detail {
inline unsigned animation_bound_calls{};
template<class State,class Before>void animation_route(State&s,Before before,Observation observation){
    if(reference()){
        ++animation_bound_calls;
        animation_inverse_proof::bound::run_142d00(s);return;
    }
    if constexpr(std::is_same_v<std::remove_cvref_t<Before>,Noop>){
        ++animation_bound_calls;
        animation_inverse_proof::bound::run_142d00(s);
    }
    else run_private(s,before,observation);
}
}
// All source/backend headers above were parsed with their real names. Only the
// harness's local State alias and two candidate call sites are redirected.
#define WorldState AnimationInverseProofState
#define run_private animation_route
#define main inverse_animation_fixture_main
#define HALO2_AFFINE_INVERSE_ANIMATION_PROOF 1
#include "native_tu15_affine_inverse_tests.cpp"
#undef main
#undef run_private
#undef WorldState
int main(int argc,char**argv){
    const auto result=inverse_animation_fixture_main(argc,argv);if(result)return result;
    if(argc==3){
        std::ifstream input(argv[2]);auto report=J::parse(input);
        const auto calls=halo2::native_affine_inverse_detail::animation_bound_calls;
        const auto factor=halo2::native_affine_inverse_detail::reference()?1u:2u;
        check(calls>0&&calls*factor==report.at("comparisons").get<unsigned>(),"Missing actual animation source-route comparisons");
        report["actual_animation_bound_calls"]=calls;report["separate_software_mmx_preserved"]=true;
        std::ofstream output(argv[2]);output<<report.dump()<<'\n';
    }
    return 0;
}
