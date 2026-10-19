#include "halo2/model_node_interpolation.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <iostream>
#include <string_view>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
constexpr float maximum=std::bit_cast<float>(0x4415fffdu);
constexpr float hidden=std::bit_cast<float>(0x3bc49b9fu);
RenderModel hierarchy(){RenderModel m;m.bones.resize(3);m.bones[0].name=1;
    m.bones[1].name=2;m.bones[1].parent=0;m.bones[2].name=3;m.bones[2].parent=0;return m;}
AnimationClip authored(){AnimationClip c;c.frame_count=195;c.nodes={1,2,3};c.components={0,4,0};c.channels.resize(3);
    auto &s=c.channels[1][2];s.times={0,60,150,151,193,194};
    s.keys={{{std::bit_cast<float>(0x3f7ffffdu),0,0,0}},{{std::bit_cast<float>(0x3f7ffffcu),0,0,0}},
        {{maximum,0,0,0}},{{hidden,0,0,0}},{{hidden,0,0,0}},{{hidden,0,0,0}}};return c;}
std::array<Transform,3> pose(float scale,float x,float other_scale) {
    Transform root,blast,other;root.position.x=x;blast.scale=scale;blast.position.x=2;
    other.scale=other_scale;other.position.y=3;
    return {root,compose_transform(root,blast),compose_transform(root,other)};
}
void reset(bool old) {
    const auto model=hierarchy();const auto cuts=model_terminal_scale_resets(model,authored());
    check(cuts.size()==1 && cuts[0].node==1 && cuts[0].hidden==hidden,"Authored terminal scale reset not found");
    ModelNodeInterpolation history;history.capture_render(2839,pose(maximum,100,1.25f));
    history.capture_render(2840,pose(hidden,102,1.5f));
    for(float f:{.001f,.125f,.25f,.5f,.75f,.999f}) {
        const auto p=history.sample_render(model,f,{},old?std::span<const ModelScaleReset>{}:cuts);
        check(p[1].scale==hidden,"Blast interpolation swept the completed hidden reset back through the camera");
        check(std::abs(p[0].position.x-(100+2*f))<.0001f,"Scale cut stopped ordinary root motion");
        check(std::abs(p[2].scale-(1.25f+.25f*f))<.0001f,"Scale cut stopped another node's interpolation");
    }
    history=ModelNodeInterpolation{};history.capture_render(20,pose(1,0,1));history.capture_render(21,pose(maximum,0,1));
    check(history.sample_render(model,.25f,{},cuts)[1].scale==std::lerp(1.f,maximum,.25f),"Blast expansion stopped interpolating");
    history=ModelNodeInterpolation{};history.capture_render(30,pose(maximum,0,1));history.capture_render(33,pose(hidden,0,1));
    check(history.sample_render(model,.25f,{},cuts)[1].scale==hidden,"Slow render gap reintroduced a scale reset pulse");
    auto gradual=authored();gradual.channels[1][2].times[3]=152;
    check(model_terminal_scale_resets(model,gradual).empty(),"Gradual scale contraction became a cut");
    auto ordinary=authored();for(auto &v:ordinary.channels[1][2].keys)v[0]=1;
    check(model_terminal_scale_resets(model,ordinary).empty(),"Constant NPC scale became a cut");
    auto cycle=authored();cycle.channels[1][2].keys[0][0]=hidden;
    check(model_terminal_scale_resets(model,cycle).empty(),"Normal scale cycle became a terminal new-minimum reset");
}
#ifndef HALO2_SCALE_RESET_STANDALONE
void asset(const char *path) {
    Cache cache(path);const auto model=cache.load_model_hierarchy(0xe201008cu);
    const auto cuts=cache.load_model_scale_resets(model);
    check(cuts.size()==1 && cuts[0].node==1 && cuts[0].visible_max==maximum && cuts[0].hidden==hidden,
        "Original ring asset did not supply its exact terminal scale reset");
}
#endif
}
int main(int argc,char **argv){try {
    const bool old=argc>1 && std::string_view(argv[1])=="--old-interpolation";
    reset(old);
#ifndef HALO2_SCALE_RESET_STANDALONE
    if(argc>1 && !old)asset(argv[1]);
#endif
    std::cout<<"Authored terminal scale reset preserves interpolation without a reverse blast\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
