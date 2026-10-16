#include "halo2/multiplayer_presentation.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/particle_presentation.hpp"
#include "halo2/model_interpolation.hpp"
#include "halo2/model_node_interpolation.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <iostream>
#include <limits>
#include <set>
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
void body_lifecycle() {
    MultiplayerState before;before.tick=100;before.source_body_poses_authoritative=true;
    auto &player=before.players[0];player.connected=true;player.source_player=0x10000;
    player.source_unit=0x20001;player.source_weapon=0x30002;player.secondary_weapon.datum=0x30003;
    before.source_body_poses={{player.source_unit,{Transform{}}},{player.source_weapon,{Transform{}}},
        {player.secondary_weapon.datum,{Transform{}}}};
    const auto transition=[&](const MultiplayerState &old,const MultiplayerState &after,const char *message) {
        SourceWorldInterpolation history;
        capture_multiplayer_body_poses(history,old,old,true);
        capture_multiplayer_body_poses(history,old,after);
        for(float f:{0.f,.25f,.5f,.75f,1.f})for(const auto &pose:after.source_body_poses)
            near(history.sample(pose.object,f).front().position,pose.nodes.front().position,message);
    };
    auto seated=before;++seated.tick;seated.players[0].source_parent=0x40003;seated.players[0].source_seat=0;
    seated.players[0].position={4,2,1};
    for(auto &pose:seated.source_body_poses)pose.nodes.front().position={4,2,1};
    transition(before,seated,"Vehicle entry retained the standing body or held weapon at the pre-entry position");
    auto exited=seated;++exited.tick;exited.players[0].source_parent=null_datum;exited.players[0].source_seat=-1;
    for(auto &pose:exited.source_body_poses)pose.nodes.front().position={5,3,0};
    transition(seated,exited,"Vehicle exit retained the attached body or held weapon pose");
    auto other_seat=seated;++other_seat.tick;other_seat.players[0].source_seat=1;other_seat.players[0].position={7,2,1};
    for(auto &pose:other_seat.source_body_poses)pose.nodes.front().position={7,2,1};
    transition(seated,other_seat,"Seat transfer blended through the previous seat body/weapon pose");
    near(multiplayer_presentation(seated,other_seat,0).players[0].position,other_seat.players[0].position,
        "Seat transfer retained its previous seat root");
    auto dead=before;++dead.tick;dead.players[0].dead=true;
    dead.players[0].source_previous_unit=player.source_unit;dead.players[0].source_unit=null_datum;
    dead.source_body_poses.resize(1);dead.source_body_poses.front().nodes.front().position={1,2,0};
    dead.corpses.emplace_back();dead.corpses.back().object=player.source_unit;
    transition(before,dead,"Death retained the standing pose instead of the completed corpse pose");
    auto seated_death=dead;seated_death.tick=seated.tick+1;
    transition(seated,seated_death,"Seated death retained the old attached body pose");
    auto respawn=dead;++respawn.tick;respawn.players[0].dead=false;respawn.players[0].source_unit=0x30001;
    respawn.source_body_poses.front().nodes.front().position={3,2,0};
    respawn.source_body_poses.push_back({respawn.players[0].source_unit,{Transform{}}});
    respawn.source_body_poses.back().nodes.front().position={10,2,0};
    SourceWorldInterpolation history;capture_multiplayer_body_poses(history,dead,dead,true);
    capture_multiplayer_body_poses(history,dead,respawn);
    near(history.sample(player.source_unit,.5f).front().position,{2,2,0},"Respawn froze the retained corpse's motion");
    near(history.sample(respawn.players[0].source_unit,0).front().position,{10,2,0},"Respawn retained the old live model");
    auto moving=seated;++moving.tick;
    for(auto &pose:moving.source_body_poses)pose.nodes.front().position={6,2,1};
    capture_multiplayer_body_poses(history,seated,seated,true);capture_multiplayer_body_poses(history,seated,moving);
    near(history.sample(player.source_unit,.5f).front().position,{5,2,1},"Ordinary seated movement stopped interpolating");
    auto world_before=before,world_seated=seated;
    world_before.source_body_poses.push_back({0x40003,{Transform{}}});
    world_seated.source_body_poses.push_back({0x40003,{Transform{}}});
    world_seated.source_body_poses.back().nodes.front().position={2,0,0};
    capture_multiplayer_body_poses(history,world_before,world_before,true);
    capture_multiplayer_body_poses(history,world_before,world_seated);
    near(history.sample(0x40003,.5f).front().position,{1,0,0},"Player entry discarded the vehicle's independent motion history");
    auto teleported=before;++teleported.tick;teleported.players[0].position={20,0,0};
    for(auto &pose:teleported.source_body_poses)pose.nodes.front().position={20,0,0};
    transition(before,teleported,"Teleport blended the old body/weapon across the map");
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
void completed_model_animations() {
    RenderModel model;model.bones.resize(3);
    model.bones[1].parent=0;model.bones[2].parent=1;
    Transform first,last;last.position={2,0,0};
    last.basis={Vec3{0,1,0},Vec3{-1,0,0},Vec3{0,0,1}};
    Transform child;child.position={1,0,0};
    const std::vector<Transform> a{first,compose_transform(first,child),
        compose_transform(compose_transform(first,child),child)};
    const std::vector<Transform> b{last,compose_transform(last,child),
        compose_transform(compose_transform(last,child),child)};
    ModelNodeInterpolation history;history.capture(10,a);history.capture(11,b);
    const auto words=[](const Transform &t){return std::bit_cast<std::array<std::uint32_t,13>>(t);};
    for(unsigned hz:{60u,120u,144u,240u}) {
        std::set<std::array<std::uint32_t,13>> samples;
        for(unsigned frame=0;frame<=hz;++frame) {
            const float f=float(frame)/hz;
            const auto nodes=history.sample(model,f);
            near(nodes[0].position.x,2*f,"completed source pose snapped between ticks");
            near(length(nodes[1].position-nodes[0].position),1,"completed pose shortened a child bone");
            near(length(nodes[2].position-nodes[1].position),1,"completed pose shortened a grandchild bone");
            samples.insert(words(nodes[0]));
        }
        check(samples.size()==hz+1,"high-refresh samples did not produce distinct poses");
        // One second of actual 30 Hz updates and this render cadence. History
        // must survive frames with no game step and exact tick boundaries.
        ModelNodeInterpolation timed;timed.capture(0,a);unsigned tick{},fractional{};
        std::set<float> positions;
        for(unsigned frame=0;frame<=hz;++frame) {
            const auto completed=frame*30/hz;
            const float f=float(frame*30%hz)/hz;
            while(tick<completed) {
                ++tick;auto next=a;for(auto &node:next)node.position.x+=2*tick;
                timed.capture(tick,next);
            }
            const auto nodes=timed.sample(model,f);
            const float expected=completed?2*(float(completed-1)+f):0;
            near(nodes[0].position.x,expected,"30 Hz history skipped a high-refresh render fraction");
            near(length(nodes[1].position-nodes[0].position),1,"High-refresh bone hierarchy changed length");
            positions.insert(nodes[0].position.x);fractional+=completed && f>0;
        }
        check(fractional>10 && positions.size()>hz-10,"High-refresh render clock stayed on whole source ticks");
    }
    for(unsigned n=0;n<a.size();++n) {
        check(words(history.sample(model,0)[n])==words(a[n]),"completed pose changed previous endpoint words");
        check(words(history.sample(model,1)[n])==words(b[n]),"completed pose changed current endpoint words");
    }
    // Multiple views/frames in one game tick must not turn current into previous.
    history.capture(11,b);near(history.sample(model,.5f)[0].position.x,1,"same-tick render destroyed pose history");
    history.capture(12,a);near(history.sample(model,0)[0].position,b[0].position,"completed pose broke tick continuity");
    history.capture(14,b);near(history.sample(model,0)[0].position,b[0].position,"missing ticks invented completed poses");
    history.capture(3,a);check(!history.paired(),"checkpoint rewind retained future animation");
    history.capture(4,a);
    for(float f:{.125f,.5f,.875f})for(unsigned n=0;n<a.size();++n)
        check(words(history.sample(model,f)[n])==words(a[n]),"Stationary source idle acquired interpolation jitter");
    // Camera movement is immediate; both arms and weapon remain camera-relative.
    const auto camera_a=camera_transform({4,5,6},{1,0,0},{0,0,1});
    const auto camera_b=camera_transform({20,30,40},{0,1,0},{0,0,1});
    auto fp_a=a,fp_b=b;
    for(auto &node:fp_a)node=compose_transform(camera_a,node);
    for(auto &node:fp_b)node=compose_transform(camera_b,node);
    ModelNodeInterpolation fp;fp.capture(20,fp_a,camera_a);fp.capture(21,fp_b,camera_b);
    const auto half=fp.sample(model,.5f,camera_b);
    const auto relative=compose_transform(inverse_transform(camera_b),half[0]);
    near(relative.position,{1,0,0},"FP interpolation lagged behind the current camera");
    for(unsigned n=0;n<b.size();++n)
        check(words(fp.sample(model,1,camera_b)[n])==words(fp_b[n]),"FP current source endpoint changed words");
    RenderModel held;held.bones.resize(1);Transform grip;grip.position={.3f,0,0};
    const std::array held_a{compose_transform(first,grip)},held_b{compose_transform(last,grip)};
    ModelNodeInterpolation attached;attached.capture(30,held_a,first);attached.capture(31,held_b,last);
    for(float f:{0.f,.25f,.5f,.75f,1.f}) {
        const auto hand=render_transform(first,last,f);
        const auto gun=attached.sample(held,f,hand);
        near(compose_transform(inverse_transform(hand),gun.front()).position,grip.position,
            "Held weapon drifted out of the interpolated hand marker");
    }
    for(float bad:{-1.f,2.f,std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected=false;try{fp.sample(model,bad,camera_b);}catch(const FormatError&){rejected=true;}
        check(rejected,"completed pose admitted an invalid fraction");
    }
}
}
int main(){try{
    snapshots();scene_history();body_lifecycle();particles_and_phase();completed_model_animations();
    for(float bad:{-1.f,2.f,std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected=false;try{multiplayer_presentation({}, {},bad);}catch(const FormatError&){rejected=true;}
        check(rejected,"invalid fraction was admitted");
    }
    std::cout<<"PASS world interpolation, vehicle entry/exit/seat transfer, death, respawn and retained corpse motion\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
