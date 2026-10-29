#include "halo2/source_native_render_timeline.hpp"
#include "halo2/model_node_interpolation.hpp"
#include "halo2/math.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <type_traits>

using namespace halo2;
static_assert(!std::is_default_constructible_v<SourceNativeCutPermit>);
static_assert(!std::is_constructible_v<SourceNativeCutPermit,std::uint64_t,std::uint64_t>);
static_assert(!std::is_default_constructible_v<SourceNativeRenderFrameContext>);
static_assert(std::is_same_v<decltype(std::declval<SourceNativeRenderFrameContext>().match()),const MultiplayerMatch&>);
static_assert(std::is_same_v<decltype(std::declval<SourceNativeRenderFrameContext>().simulation()),const Simulation&>);
namespace {
void require(bool value,const char *why){if(!value)throw FormatError(why);}
void near(float a,float b,const char *why){require(std::abs(a-b)<0.0001f,why);}
Transform at(float x,float angle=0) {
    Transform value;value.position={x,0,0};value.basis={Vec3{std::cos(angle),std::sin(angle),0},
        Vec3{-std::sin(angle),std::cos(angle),0},Vec3{0,0,1}};return value;
}
void continuity() {
    const SourceNativeRenderContinuity base{true,true,4,5,100,130,77,90,1,2};
    require(source_native_render_continuous(base),"Exact progressing sparse native cuts did not pair");
    auto p=base;p.current_tick=101;require(source_native_render_continuous(p),"Native consecutive cut rejected");
    p=base;p.current_tick=220;require(source_native_render_continuous(p),"Native120tick boundary rejected");
    p=base;p.current_tick=221;require(!source_native_render_continuous(p),"Unbounded native sparse gap admitted");
    p=base;p.current_tick=100;require(!source_native_render_continuous(p),"Same native tick invented interpolation time");
    p=base;p.current_tick=99;require(!source_native_render_continuous(p),"Backward native tick admitted");
    p=base;p.same_generation=false;require(!source_native_render_continuous(p),"Foreign generation paired");
    p=base;p.same_world=false;require(!source_native_render_continuous(p),"Foreign world paired");
    p=base;p.previous_serial=0;require(!source_native_render_continuous(p),"Absent cut serial paired");
    p=base;p.current_serial=p.previous_serial;require(!source_native_render_continuous(p),"Duplicate serial paired");
    p=base;p.current_serial=3;require(!source_native_render_continuous(p),"Backward serial paired");
    p=base;p.current_sequence=p.previous_sequence;require(!source_native_render_continuous(p),"Duplicate sequence paired");
    p=base;p.current_sequence=76;require(!source_native_render_continuous(p),"Backward sequence paired");
    p=base;p.previous_sequence=0xffffffffu;p.current_sequence=0;
    require(source_native_render_continuous(p),"Genuine progressing snapshot sequence wrap rejected");
    p=base;p.current_sequence=p.previous_sequence+0x80000000u;
    require(!source_native_render_continuous(p),"Ambiguous half-range sequence paired");
    p=base;p.current_seconds=0.5;require(!source_native_render_continuous(p),"Backward render clock paired");
    p=base;p.previous_seconds=-1;require(!source_native_render_continuous(p),"Negative render clock paired");
    p=base;p.current_seconds=std::numeric_limits<double>::quiet_NaN();
    require(!source_native_render_continuous(p),"NaN render clock paired");
    p=base;p.previous_seconds=std::numeric_limits<double>::infinity();
    require(!source_native_render_continuous(p),"Infinite render clock paired");
    p=base;p.current_seconds=p.previous_seconds;
    require(source_native_render_continuous(p),"Two actual cuts in one foreground service were rejected");
}
void freshness() {
    require(source_native_render_fresh(100,100,100),"Exact native/ordinary frame rejected");
    require(source_native_render_fresh(100,220,220),"Native120tick lag boundary rejected");
    require(!source_native_render_fresh(100,221,221),"Native121tick stale frame admitted");
    require(!source_native_render_fresh(100,150,221),"Buffered ordinary frame hid stale native front");
    require(!source_native_render_fresh(100,221,150),"Supplied stale ordinary/native pair admitted");
    require(!source_native_render_fresh(100,99,100),"Native ahead of ordinary frame admitted");
    require(!source_native_render_fresh(100,100,99),"Native ahead of actual client admitted");
    require(!source_native_render_fresh(0,std::numeric_limits<std::uint64_t>::max(),100),
        "Unbounded native age wrapped into freshness");
    require(source_native_render_fresh(std::numeric_limits<std::uint64_t>::max(),
        std::numeric_limits<std::uint64_t>::max(),std::numeric_limits<std::uint64_t>::max()),
        "Freshness arithmetic overflowed at equal maximum source ticks");
}
void ordinary_policy_unchanged() {
    SourceWorldPose pose{0x10001,{at(0)}, {},SourceWorldPoseAttachment{null_datum,0xff,{}}};
    SourceWorldInterpolation world;world.reset(10,std::span(&pose,1));
    pose.nodes[0]=at(10);world.capture(11,std::span(&pose,1));
    near(world.sample(pose.object,.5f)[0].position.x,5,"Ordinary consecutive WORLD pair changed");
    pose.nodes[0]=at(30);world.capture(14,std::span(&pose,1));
    near(world.sample(pose.object,0)[0].position.x,30,"Ordinary sparse WORLD gap invented missing states");
    SceneObjects::Object object;object.handle=0x10002;object.visible=true;object.position={0,0,0};
    SceneInterpolation scene;scene.reset(10,std::span(&object,1));object.position.x=10;
    scene.capture(11,std::span(&object,1));near(scene.sample(object,.5f).position.x,5,"Ordinary consecutive scene pair changed");
    object.position.x=30;scene.capture(14,std::span(&object,1));
    near(scene.sample(object,0).position.x,30,"Ordinary sparse scene gap invented missing states");
}
void actual_model_ticks() {
    RenderModel model;model.bones.resize(2);model.bones[0].parent=-1;model.bones[1].parent=0;
    const std::array before{at(0),at(1)};
    const auto root=at(30,1.5707963267948966f);
    const std::array after{root,compose_transform(root,at(1))};
    ModelNodeInterpolation native;native.capture_render(20,before);native.capture_render(50,after);
    require(native.paired(),"Actual sparse collector ticks were discarded");
    const auto middle=native.sample(model,.5f);
    near(middle[0].position.x,15,"Native sparse pair used a fictitious one-tick fraction");
    near(length(middle[1].position-middle[0].position),1,"Native sparse hierarchy shrank a rotating limb");
    for(float f:{0.f,1.f}) {
        const auto nodes=native.sample(model,f);const auto &expected=f==0?before:after;
        require(!std::memcmp(nodes.data(),expected.data(),sizeof(before)),"Native sparse endpoint bits changed");
    }
    // Ordinary capture retains its stricter policy even for identical banks.
    ModelNodeInterpolation ordinary;ordinary.capture(20,before);ordinary.capture(50,after);
    require(!ordinary.paired(),"Ordinary model capture accepted a sparse native-only interval");
    near(ordinary.sample(model,.5f)[0].position.x,30,"Ordinary missing-step model policy changed");
}
}
int main(){try{continuity();freshness();ordinary_policy_unchanged();actual_model_ticks();
    std::cout<<"PASS native-cut numeric continuity controls, private permit boundary, actual model ticks and unchanged ordinary WORLD/scene/model policies\n";
    return 0;}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
