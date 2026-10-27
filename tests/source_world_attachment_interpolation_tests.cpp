#include "halo2/render_interpolation.hpp"
#include "halo2/math.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
using namespace halo2;
namespace {
constexpr std::uint32_t parent=0x10001u,child=0x10002u;
void need(bool value,const char *message){if(!value)throw FormatError(message);}
void near(float actual,float expected,const char *message){need(std::abs(actual-expected)<1e-5f,message);}
Transform frame(float x,float angle=0) {
    Transform t;t.position={x,0,0};t.basis={Vec3{std::cos(angle),std::sin(angle),0},
        Vec3{-std::sin(angle),std::cos(angle),0},Vec3{0,0,1}};return t;
}
RenderModel skeleton(unsigned nodes) {
    RenderModel model;model.bones.resize(nodes);
    for(unsigned n=0;n<nodes;++n)model.bones[n].parent=n?0:-1;return model;
}
SourceWorldPose root(std::uint32_t owner,std::vector<Transform> nodes) {
    return {owner,std::move(nodes),{},SourceWorldPoseAttachment{0xffffffffu,0xffu,{}}};
}
SourceWorldPose attached(Transform node,Transform anchor,unsigned selected=0,std::uint32_t owner=parent) {
    return {child,{node},{},SourceWorldPoseAttachment{owner,std::uint8_t(selected),anchor}};
}
SourceWorldInterpolation history(std::span<const SourceWorldPose> before,std::span<const SourceWorldPose> after) {
    SourceWorldInterpolation result;result.reset(10,before);result.capture(11,after);return result;
}
void hierarchy_and_hand() {
    const auto body=skeleton(2),gun=skeleton(1);const auto start=frame(0),finish=frame(0,1.5707963267948966f);
    auto hand=frame(.5f),gun_node=frame(.7f);
    const std::array before{root(parent,{start,hand}),attached(gun_node,hand,1)};
    const std::array after{root(parent,{finish,compose_transform(finish,hand)}),
        attached(compose_transform(finish,gun_node),compose_transform(finish,hand),1)};
    const auto samples=history(before,after);
    const SourceWorldInterpolation::ModelProvider models=[&](auto owner){return owner==parent?&body:&gun;};
    for(float fraction:{0.f,.25f,.5f,.75f,1.f}) {
        const auto shown_body=samples.sample(parent,fraction,models),shown_gun=samples.sample(child,fraction,models);
        near(length(shown_body[1].position),.5f,"WORLD hierarchy shortened a rotating limb");
        near(length(shown_gun[0].position-shown_body[1].position),.2f,"Carried model left its shown native hand frame");
        if(fraction==0 || fraction==1) {
            const auto &original=fraction==0?before:after;
            need(!std::memcmp(shown_body.data(),original[0].nodes.data(),sizeof(Transform)*2),"Body endpoint words changed");
            need(!std::memcmp(shown_gun.data(),original[1].nodes.data(),sizeof(Transform)),"Held endpoint words changed");
        }
    }
    const auto old_body=samples.sample(parent,.5f),old_gun=samples.sample(child,.5f);
    need(std::abs(length(old_gun[0].position-old_body[1].position)-.2f)>.01f,
        "Independent WORLD policy negative control did not detach the carried model");
}
void fresh_frame_and_budget() {
    const auto body=skeleton(1),gun=skeleton(1);
    const SourceWorldInterpolation::ModelProvider models=[&](auto owner){return owner==parent?&body:&gun;};
    const std::array before{root(parent,{frame(0)}),attached(frame(1),frame(0))};
    const std::array full{root(parent,{frame(10)}),attached(frame(12),frame(10))};
    const auto full_history=history(before,full);
    near(full_history.sample(child,.5f,models)[0].position.x,6.5f,"Fresh native frame discarded new child-local root motion");
    near(full_history.sample(child,1.f,models)[0].position.x,12.f,"Fresh completed WORLD root changed");
    // A body budget can retain another represented pose. Decompose the child
    // with its fresh native source frame, then use the ACTUALLY SHOWN hand.
    const std::array budgeted{root(parent,{frame(7)}),attached(frame(12),frame(10))};
    const auto budget_history=history(before,budgeted);
    near(budget_history.sample(child,1.f)[0].position.x,12.f,"Presentation rewrote the accepted completed WORLD bank");
    near(budget_history.sample(child,1.f,models)[0].position.x,9.f,"Budgeted body used a stale source frame as its shown hand");
    near(budget_history.sample(child,.5f,models)[0].position.x,5.f,"Parent/child budget mismatch changed the retained local pose");
    const std::array held{root(parent,{frame(10)}),attached(frame(1),frame(0))};
    near(history(before,held).sample(child,1.f,models)[0].position.x,11.f,"Held bank lost its matching older source frame");
}
void ownership_cuts_and_legacy() {
    const auto body=skeleton(3),gun=skeleton(1);
    const std::vector<Transform> nodes{frame(0),frame(1),frame(2)};
    const std::array before{root(parent,nodes),attached(frame(1.2f),frame(1),1)};
    const std::array after{root(parent,nodes),attached(frame(2.5f),frame(2),2)};
    const SourceWorldInterpolation::ModelProvider models=[&](auto owner){return owner==child?&gun:&body;};
    for(float fraction:{0.f,.5f,1.f})near(history(before,after).sample(child,fraction,models)[0].position.x,2.5f,
        "Native selected-node transition retained the old hand pose");
    const std::array salted{root(parent+0x10000u,nodes),attached(frame(2.5f),frame(2),2,parent+0x10000u)};
    near(history(before,salted).sample(child,0.f,models)[0].position.x,2.5f,"Recycled parent salt retained the previous attachment");
    const std::array detached{root(parent,nodes),root(child,{frame(8)})};
    near(history(before,detached).sample(child,0.f,models)[0].position.x,8.f,"Detach blended the previous carried pose");
    auto legacy=detached;legacy[1].attachment.reset();
    const auto unknown=history(before,legacy);
    near(unknown.sample(child,0.f,models)[0].position.x,8.f,"Legacy unknown binding invented native attachment interpolation");
    auto moving=unknown;Transform motion;motion.position.x=3;
    const std::array motions{std::pair{child,motion}};moving.present(motions);
    near(moving.sample(child,.5f,models)[0].position.x,11.f,"Native sampling changed legacy rigid dead reckoning");
    bool rejected{};
    try{(void)history(before,after).sample(child,.5f,[&](auto owner)->const RenderModel* {return owner==child?&gun:nullptr;});}
    catch(const FormatError&){rejected=true;}
    need(rejected,"Missing native parent hierarchy guessed a skeleton");
    rejected=false;
    try{(void)history(before,after).sample(child,gun,.5f);}catch(const FormatError&){rejected=true;}
    need(rejected,"Visible native parent sampling silently omitted its shown hierarchy");
    auto invalid=after;invalid[1].attachment->parent_frame->position.x=std::numeric_limits<float>::quiet_NaN();rejected=false;
    try{(void)history(before,invalid);}catch(const FormatError&){rejected=true;}
    need(rejected,"Nonfinite native parent frame was admitted");
}
}
int main()try {
    hierarchy_and_hand();fresh_frame_and_budget();ownership_cuts_and_legacy();
    std::cout<<"PASS source WORLD hierarchy, native parent frames, budget tuples, cuts and legacy motion\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
