#include "halo2/multiplayer_presentation.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/particle_presentation.hpp"
#include "halo2/model_interpolation.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <iostream>
#include <limits>
using namespace halo2;
namespace {
void check(bool b,const char *m){if(!b)throw FormatError(m);}
void near(float a,float b,const char *m){check(std::abs(a-b)<.0001f,m);}
void near(Vec3 a,Vec3 b,const char *m){check(length(a-b)<.0001f,m);}
void snapshots() {
    MultiplayerState a,b;a.tick=10;b.tick=11;
    auto &p=a.players[0];p.connected=true;p.source_player=0x10000;p.source_unit=0x10001;
    p.position={0,2,4};p.yaw=3.1f;p.crouch=0;
    p.grenade_action.graph=1;p.grenade_action.clip=2;p.grenade_action.activity=3;
    p.grenade_action.animation_active=true;p.grenade_action.frame=4;
    b.players[0]=p;b.players[0].position={4,2,4};b.players[0].yaw=-3.1f;b.players[0].crouch=1;
    b.players[0].grenade_action.frame=5;b.players[0].health=.5f;
    a.world_weapons.push_back({0x20001,2,{0,0,0},{1,0,0},{0,0,1},6,12});
    b.world_weapons=a.world_weapons;b.world_weapons[0].position={8,0,0};b.world_weapons[0].loaded=3;
    a.weapon_projectiles.push_back({0x30002,3,{0,0,0},{},{1,0,0},{0,0,1}});
    b.weapon_projectiles=a.weapon_projectiles;b.weapon_projectiles[0].position={12,0,0};
    NativeGrenadeProjectile g;g.datum=0x40003;g.definition=4;g.type=0;g.position={0,0,0};
    a.grenade_projectiles.push_back(g);g.position={16,0,0};b.grenade_projectiles.push_back(g);
    for(unsigned i=0;i<=4;++i) {
        const float f=float(i)/4;const auto shown=multiplayer_presentation(a,b,f);
        near(shown.players[0].position,{float(i),2,4},"player missed fractional render sample");
        near(shown.world_weapons[0].position.x,float(i)*2,"dropped weapon missed fractional render sample");
        near(shown.weapon_projectiles[0].position.x,float(i)*3,"weapon projectile missed fractional render sample");
        near(shown.grenade_projectiles[0].position.x,float(i)*4,"grenade missed fractional render sample");
        near(shown.players[0].crouch,f,"crouch did not interpolate");
        near(shown.players[0].grenade_action.frame,4+f,"body throw animation did not interpolate");
        check(shown.tick==b.tick && shown.world_weapons[0].loaded==3 && shown.players[0].health==.5f,
            "presentation blended discrete gameplay state");
    }
    const auto half=multiplayer_presentation(a,b,.5f);
    near(std::abs(half.players[0].yaw),3.14159265f,"yaw took the long path across angle wrap");
    check(a.players[0].position.x==0 && b.players[0].position.x==4 && a.world_weapons[0].loaded==6,
        "sampling changed authoritative endpoints");
    auto cut=b;cut.players[0].source_unit+=0x10000;cut.world_weapons[0].handle+=0x10000;
    cut.weapon_projectiles[0].datum+=0x10000;cut.grenade_projectiles[0].datum+=0x10000;
    const auto replaced=multiplayer_presentation(a,cut,0);
    near(replaced.players[0].position,cut.players[0].position,"respawn retained previous unit root");
    near(replaced.world_weapons[0].position,cut.world_weapons[0].position,"recycled weapon blended");
    near(replaced.weapon_projectiles[0].position,cut.weapon_projectiles[0].position,"recycled projectile blended");
    near(replaced.grenade_projectiles[0].position,cut.grenade_projectiles[0].position,"recycled grenade blended");
    cut=b;cut.players[0].dead=true;cut.grenade_projectiles[0].parent=0x123;
    const auto dead=multiplayer_presentation(a,cut,0);
    near(dead.players[0].position,cut.players[0].position,"death interpolated across visibility transition");
    near(dead.grenade_projectiles[0].position,cut.grenade_projectiles[0].position,"attachment interpolated across transition");
    cut=b;cut.world_weapons.clear();cut.weapon_projectiles.clear();cut.grenade_projectiles.clear();
    const auto gone=multiplayer_presentation(a,cut,.5f);
    check(gone.world_weapons.empty() && gone.weapon_projectiles.empty() && gone.grenade_projectiles.empty(),
        "removed object remained in presentation");
    cut=b;cut.tick=a.tick;
    near(multiplayer_presentation(a,cut,0).players[0].position,cut.players[0].position,"same tick acquired motion history");
}
void scene_history() {
    SceneObjects::Object o;o.handle=0x10002;o.created_tick=1;o.placement=3;
    SceneInterpolation history;history.reset(5,std::span{&o,1});
    o.position={4,2,0};o.rotation.x=3.14159265f;o.scale=2;history.capture(6,std::span{&o,1});
    const auto half=history.sample(o,.5f);
    near(half.position,{2,1,0},"scene root did not interpolate");near(half.scale,1.5f,"scene scale did not interpolate");
    near(length(half.basis[0]),1,"rotation interpolation shrank basis");
    near(dot(half.basis[0],half.basis[1]),0,"rotation interpolation sheared basis");
    const auto endpoint=history.sample(o,1);o.position={8,2,0};history.capture(7,std::span{&o,1});
    near(history.sample(o,0).position,endpoint.position,"scene tick boundary discontinuity");
    o.position={12,2,0};history.capture(8,std::span{&o,1});
    near(history.sample(o,.5f).position,{10,2,0},"multiple steps between draws retained stale endpoint");
    auto born=o;born.handle+=0x10000;near(history.sample(born,0).position,born.position,"scene lifetime salt ignored");
    born=o;born.bsp=2;near(history.sample(born,0).position,born.position,"scene BSP cut ignored");
    history.reset(8,std::span{&o,1});near(history.sample(o,0).position,o.position,"restore kept interpolation history");
    o.position.x=20;history.capture(12,std::span{&o,1});near(history.sample(o,0).position,o.position,"missing scene steps invented motion");
}
void particles_and_phase() {
    ParticleSimulationState a,b;a.salt=b.salt=5;a.position={0,0,0};b.position={4,0,0};
    a.velocity={120,0,0};b.velocity={-120,0,0};a.age=.2f;b.age=.4f;a.phase=1;b.phase=2;
    const auto p=particle_presentation(&a,b,.5f);
    near(p.position,{2,0,0},"particle extrapolated past a collision");near(p.age,.3f,"particle age was not sampled");
    near(p.phase,1.5f,"particle phase was not sampled");
    b.salt++;near(particle_presentation(&a,b,0).position,b.position,"recycled particle blended");
    near(particle_presentation(nullptr,b,0).position,b.position,"new particle invented a previous position");
    RenderModel model;ModelBone root,child;root.name=1;child.name=2;child.parent=0;
    child.bind.translation={1,0,0};model.bones={root,child};
    AnimationClip clip;clip.frame_count=2;clip.nodes={1,2};clip.parents={-1,0};
    clip.components={1,0};clip.frames.resize(4);clip.channels.resize(2);
    clip.channels[0][0].times={0,1};clip.channels[0][0].keys={{{0,0,0,1}},{{0,0,1,0}}};
    const Transform frame;
    const auto nodes=pose_model_nodes_world_between_frames(model,frame,clip,{1,0,.5f});
    near(length(nodes[1].position-nodes[0].position),1,"interpolation shortened a child bone across loop wrap");
    near(std::abs(nodes[1].position.y),1,"loop pose held the final key instead of moving between completed poses");
    for(unsigned endpoint=0;endpoint<=1;++endpoint) {
        const auto actual=pose_model_nodes_world_between_frames(model,frame,clip,{1,0,float(endpoint)});
        const auto expected=pose_model_nodes_world_frame(model,frame,&clip,float(1-endpoint));
        check(actual.size()==expected.size(),"pose interpolation changed node inventory");
        for(unsigned n=0;n<actual.size();++n)
            check(std::bit_cast<std::array<std::uint32_t,13>>(actual[n])==
                  std::bit_cast<std::array<std::uint32_t,13>>(expected[n]),"pose endpoint changed original node words");
    }
}
}
int main(){try{
    snapshots();scene_history();particles_and_phase();
    for(float bad:{-1.f,2.f,std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected=false;try{multiplayer_presentation({}, {},bad);}catch(const FormatError&){rejected=true;}
        check(rejected,"invalid fraction was admitted");
    }
    std::cout<<"PASS world objects at 120 Hz samples over 30 Hz states, rotations, lifetimes, scene steps, particle collisions and source scenery loops\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
