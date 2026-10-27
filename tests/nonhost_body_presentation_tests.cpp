// Non-host movement prediction draws the own third-person body (other local
// views, shadows) where the predicted camera is (NETCODE_ENHANCED_GOAL item 1).
// A production NonHostMovementClient runs against a lagging host whose
// snapshots carry a correction (45%/35% decay), a lost snapshot (unpaired
// body bank) and a teleport cut; every presented frame samples the body bank
// exactly as the runtime does, at independent pose and animation blends.
#include "halo2/multiplayer_presentation.hpp"
#include "halo2/nonhost_movement_client.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <utility>

using namespace halo2;
namespace {
void check(bool value,const char *message) {
  if(!value)throw std::runtime_error(message);
}
bool same_bytes(const std::vector<Transform> &a,const std::vector<Transform> &b) {
  return a.size()==b.size() && (a.empty() || !std::memcmp(a.data(),b.data(),a.size()*sizeof(Transform)));
}
bool same_basis(const Transform &a,const Transform &b) {
  return !std::memcmp(a.basis.data(),b.basis.data(),sizeof(a.basis)) &&
      std::bit_cast<std::uint32_t>(a.scale)==std::bit_cast<std::uint32_t>(b.scale);
}
float distance(Vec3 a,Vec3 b) {return length(a-b);}
// A child recomposed into a displaced shown parent frame keeps its rotation
// to rounding (compose(to, inverse(from)) is not bitwise the identity).
float basis_error(const Transform &a,const Transform &b) {
  float error=std::abs(a.scale-b.scale);
  for(unsigned axis=0;axis<3;++axis)error=std::max(error,distance(a.basis[axis],b.basis[axis]));
  return error;
}
CollisionWorld floor() {
  std::vector<CollisionTriangle> triangles(2);
  triangles[0].a={-200,-200,0};triangles[0].b={200,-200,0};triangles[0].c={200,200,0};
  triangles[1].a={-200,-200,0};triangles[1].b={200,200,0};triangles[1].c={-200,200,0};
  for(auto &t:triangles)t.source_normal={0,0,1};
  return CollisionWorld(std::move(triangles));
}
MovementDefinition movement() {return {2.25f,2.f,2.f,12.f,1.f,.9f,.9f,8.f,3.f,1.f};}
BipedDefinition biped() {
  BipedDefinition b;b.datum=0x10001u;b.height=.7f;b.crouch_height=.5f;b.radius=.2f;
  b.crouch_velocity=4.f;b.jump_velocity=1.2f;b.camera_height=.6f;b.crouch_camera_height=.4f;
  b.landing={.2f,.5f,1.2f,2.f,3.f};return b;
}
LanClient::InputReceipt receipt(std::uint32_t sequence) {
  LanClient::InputReceipt r;r.known=true;r.consumed=sequence;return r;
}

constexpr std::uint32_t own_unit=0x20002u,own_weapon=0x20003u,own_grenade=0x20004u;
constexpr std::uint32_t seated_unit=0x30002u,vehicle=0x30005u,remote_unit=0x40002u,remote_weapon=0x40003u;
constexpr std::array objects{own_unit,own_weapon,own_grenade,seated_unit,vehicle,remote_unit,remote_weapon};
constexpr std::array others{seated_unit,vehicle,remote_unit,remote_weapon};
constexpr float ground=tu15_biped_origin_height+.0001f;

Transform frame(Vec3 position,float yaw) {
  Transform t;t.position=position;
  t.basis={Vec3{std::cos(yaw),std::sin(yaw),0},Vec3{-std::sin(yaw),std::cos(yaw),0},Vec3{0,0,1}};return t;
}
RenderModel skeleton(unsigned nodes) {
  RenderModel model;model.bones.resize(nodes);
  for(unsigned n=0;n<nodes;++n)model.bones[n].parent=n?0:-1;
  return model;
}
SourceWorldPoseAttachment unparented() {return {0xffffffffu,0xffu,{}};}
// WORLD node0 is the object origin; the other nodes animate every tick.
std::vector<Transform> body_nodes(Vec3 origin,float yaw,std::uint32_t tick) {
  const auto root=frame(origin,yaw);const float phase=float(tick)*.21f;
  return {root,compose_transform(root,frame({.25f*std::cos(phase),-.2f,.5f+.05f*std::sin(phase)},.5f*phase)),
      compose_transform(root,frame({0,0,.65f},.1f*std::sin(phase)))};
}
std::vector<Transform> weapon_nodes(const Transform &hand,std::uint32_t tick) {
  const auto grip=compose_transform(hand,frame({.1f,0,0},.2f+.03f*float(tick%5)));
  return {grip,compose_transform(grip,frame({.4f,0,0},0))};
}
MultiplayerState snapshot(std::uint32_t tick,Vec3 own,Vec3 velocity,std::uint32_t teleport) {
  MultiplayerState s;s.tick=tick;s.source_body_poses_authoritative=true;
  auto &a=s.players[0];
  a.connected=true;a.source_unit=own_unit;a.source_weapon=own_weapon;a.native_physical_state=1;
  a.native_grounded=true;a.position=own;a.velocity=velocity;a.source_teleport_serial=teleport;
  auto &b=s.players[1]; // Second local view, seated: never predicted.
  const Vec3 seat{-4.f+.05f*float(tick),6,.5f};
  b.connected=true;b.source_unit=seated_unit;b.source_parent=vehicle;b.source_seat=0;
  b.native_physical_state=1;b.position=seat+Vec3{0,0,.4f};
  auto &c=s.players[2]; // Remote player.
  c.connected=true;c.source_unit=remote_unit;c.source_weapon=remote_weapon;c.native_physical_state=1;
  c.native_grounded=true;c.position={5.f-.07f*float(tick),3,ground};c.yaw=3.1f;
  const auto own_body=body_nodes(a.position,0,tick),remote_body=body_nodes(c.position,c.yaw,tick);
  const auto own_held=weapon_nodes(own_body[1],tick),remote_held=weapon_nodes(remote_body[1],tick);
  const auto vehicle_nodes=std::vector<Transform>{frame(seat,.3f)};
  const auto seated_body=body_nodes(b.position,.3f,tick);
  const auto stuck=compose_transform(own_held[1],frame({.05f,0,.02f},0));
  const auto sphere=[](const Transform &node){return ShadowCasterSphere{node.position+Vec3{0,0,.3f},.6f};};
  s.source_body_poses={
      {own_unit,own_body,sphere(own_body[0]),unparented()},
      {own_weapon,own_held,sphere(own_held[0]),SourceWorldPoseAttachment{own_unit,1,own_body[1]}},
      {own_grenade,{stuck},sphere(stuck),SourceWorldPoseAttachment{own_weapon,1,own_held[1]}},
      {vehicle,vehicle_nodes,sphere(vehicle_nodes[0]),unparented()},
      {seated_unit,seated_body,sphere(seated_body[0]),SourceWorldPoseAttachment{vehicle,0,vehicle_nodes[0]}},
      {remote_unit,remote_body,sphere(remote_body[0]),unparented()},
      {remote_weapon,remote_held,sphere(remote_held[0]),SourceWorldPoseAttachment{remote_unit,1,remote_body[1]}}};
  return s;
}
const ShadowCasterSphere &pose_sphere(const MultiplayerState &s,std::uint32_t object) {
  const auto at=std::find_if(s.source_body_poses.begin(),s.source_body_poses.end(),
      [&](const auto &pose){return pose.object==object;});
  if(at==s.source_body_poses.end())throw std::runtime_error("Fixture lost a body pose");
  return at->sphere;
}
bool same_bits(Vec3 a,Vec3 b) {
  return std::bit_cast<std::uint32_t>(a.x)==std::bit_cast<std::uint32_t>(b.x) &&
      std::bit_cast<std::uint32_t>(a.y)==std::bit_cast<std::uint32_t>(b.y) &&
      std::bit_cast<std::uint32_t>(a.z)==std::bit_cast<std::uint32_t>(b.z);
}

struct Totals {
  unsigned frames{},predicted_frames{},correction_frames{},unpaired_frames{},cut_frames{};
  float root_error{},attachment_error{},pose_error{},lag{};
};

void predicted_own_body() {
  const auto collision=floor();const auto b=biped();
  NonHostMovementClient client(NetcodeMode::balanced,movement(),std::bit_cast<std::uint32_t>(.01f));
  const std::array<unsigned,2> slots{0,1};
  const auto body_model=skeleton(3),weapon_model=skeleton(2),single=skeleton(1);
  const SourceWorldInterpolation::ModelProvider models=[&](std::uint32_t owner)->const RenderModel* {
    if(owner==own_unit || owner==seated_unit || owner==remote_unit)return &body_model;
    if(owner==own_weapon || owner==remote_weapon)return &weapon_model;
    return &single;
  };
  // Three identical banks: predicted (the runtime's), ordinary (before this
  // change) and prediction off (no movement client: no roots).
  SourceWorldInterpolation predicted_bank,ordinary_bank,off_bank;
  MultiplayerState previous=snapshot(10,{0,0,ground},{},0),current=previous;
  for(auto *bank:{&predicted_bank,&ordinary_bank,&off_bank})
    capture_multiplayer_body_poses(*bank,previous,current,true);
  Controls run;run.forward=1;run.right=.25f;
  check(!client.predict(0,10,run,0,0,current.players[0],b,collision),"Prediction started before a baseline");
  check(!client.predict(1,10,run,0,0,current.players[1],b,collision),"Seated view predicted");
  client.reconcile(0,current.players[0],receipt(10),collision);
  client.reconcile(1,current.players[1],receipt(10),collision);
  std::uint32_t teleport=0;
  Totals totals;
  const std::array<std::pair<float,float>,6> fractions{{{0,0},{.2f,.35f},{.55f,.8f},{.9f,1},{1,.5f},{.37f,.13f}}};
  for(std::uint32_t t=11;t<=70;++t) {
    check(client.predict(0,t,run,0,0,current.players[0],b,collision).has_value(),"Own view stopped predicting");
    check(!client.predict(1,t,run,0,0,current.players[1],b,collision),"Seated view predicted");
    const bool lost=t==40,gap=t==41,cut=t==55;
    if(!lost) {
      // The host consumes each input three ticks late; its motion is the
      // client's own prediction of that input, except for an injected
      // correction (0.4 wu, replayed and decayed) and a 21 wu teleport.
      const std::uint32_t consumed=t<13?10:t-3;
      Vec3 position{0,0,ground},velocity;
      if(consumed!=10) {
        const auto *motion=client.predictor(0)->prediction(consumed);
        check(motion!=nullptr,"Host input left the prediction history");
        position=nonhost_authority_position(*client.definition(0),motion->body.position);
        velocity=motion->body.velocity;
      }
      if(consumed==25)position.y+=.4f;
      if(cut){++teleport;position=position+Vec3{21,0,0};}
      previous=current;current=snapshot(t,position,velocity,teleport);
      for(auto *bank:{&predicted_bank,&ordinary_bank,&off_bank})capture_multiplayer_body_poses(*bank,previous,current);
      client.reconcile(0,current.players[0],receipt(consumed),collision);
      client.reconcile(1,current.players[1],receipt(consumed),collision);
    }
    for(const auto &[blend,animation]:fractions) {
      ++totals.frames;
      auto shown=multiplayer_presentation(previous,current,blend);
      const auto host_shown=shown;
      client.present(shown,slots,animation);
      const std::array presented{client.presented(0,animation),client.presented(1,animation)};
      const auto roots=predicted_body_roots(previous,current,shown,slots,presented);
      predicted_bank.predict(roots);
      // Prediction off: the runtime has no movement client, so no position.
      const std::array<std::optional<Vec3>,2> none{};
      const auto off_roots=predicted_body_roots(previous,current,host_shown,slots,none);
      check(off_roots.empty(),"Prediction off produced a predicted body");
      off_bank.predict(off_roots);
      check(presented[0] && !presented[1],"Only the on-foot view is predicted");
      check(same_bits(shown.players[0].position,*presented[0]),"Camera position is not the prediction");
      check(roots.size()==1 && roots[0].object==own_unit,"Own predicted body was not selected");
      ++totals.predicted_frames;
      const auto camera=shown.players[0].position;
      totals.lag=std::max(totals.lag,distance(camera,host_shown.players[0].position));
      if(length(client.predictor(0)->position_offset())>1e-4f)++totals.correction_frames;
      if(gap)++totals.unpaired_frames;
      if(cut)++totals.cut_frames;

      // Model-order sampling (bodies and held weapons in the runtime).
      const auto body=predicted_bank.sample(own_unit,blend,models),plain=ordinary_bank.sample(own_unit,blend,models);
      const float root_error=distance(body[0].position,camera);
      totals.root_error=std::max(totals.root_error,root_error);
      check(root_error<1e-4f,"Own body root is not at the predicted camera origin");
      const auto moved=body[0].position-plain[0].position;
      for(unsigned n=0;n<body.size();++n) {
        check(same_basis(body[n],plain[n]),"Own body pose rotated");
        const float pose_error=distance(body[n].position-body[0].position,plain[n].position-plain[0].position);
        totals.pose_error=std::max(totals.pose_error,pose_error);
        check(pose_error<1e-4f,"Own body animation pose changed");
      }
      if(gap && blend<1)check(distance(plain[0].position,host_shown.players[0].position)>1e-3f,
          "Lost snapshot fixture did not unpair the body bank");
      for(const auto object:{own_weapon,own_grenade}) {
        const auto held=predicted_bank.sample(object,blend,models),still=ordinary_bank.sample(object,blend,models);
        check(held.size()==still.size(),"Attachment bank changed size");
        for(unsigned n=0;n<held.size();++n) {
          check(basis_error(held[n],still[n])<1e-5f,"Attachment rotated");
          const float error=distance(held[n].position-still[n].position,moved);
          totals.attachment_error=std::max(totals.attachment_error,error);
          check(error<1e-4f,"Attachment did not move with its predicted body");
        }
      }
      // Independent WORLD sampling (dynamic objects, glows): same origin.
      const auto loose=predicted_bank.sample(own_unit,blend),loose_still=ordinary_bank.sample(own_unit,blend);
      check(distance(loose[0].position,camera)<1e-4f,"Independent own body sample is not at the prediction");
      const auto loose_moved=loose[0].position-loose_still[0].position;
      for(const auto object:{own_weapon,own_grenade}) {
        const auto held=predicted_bank.sample(object,blend),still=ordinary_bank.sample(object,blend);
        for(unsigned n=0;n<held.size();++n) {
          check(same_basis(held[n],still[n]),"Independent attachment rotated");
          check(distance(held[n].position-still[n].position,loose_moved)<1e-4f,
              "Independent attachment did not move with its predicted body");
        }
      }
      // Shadow/lighting sphere follows the same root displacement.
      const auto &sphere=pose_sphere(current,own_unit);
      const auto shown_sphere=predicted_body_sphere(roots,own_unit,sphere);
      check(distance(shown_sphere.center-sphere.center,camera-current.players[0].position)<1e-4f &&
          shown_sphere.radius==sphere.radius,"Own body sphere did not follow the prediction");
      // Other players (seated local view, vehicle, remote) are untouched.
      for(const auto object:others) {
        check(same_bytes(predicted_bank.sample(object,blend,models),ordinary_bank.sample(object,blend,models)) &&
            same_bytes(predicted_bank.sample(object,blend),ordinary_bank.sample(object,blend)),
            "Another player's body moved");
        const auto &other=pose_sphere(current,object);const auto kept=predicted_body_sphere(roots,object,other);
        check(!std::memcmp(&kept,&other,sizeof(kept)),"Another player's sphere moved");
      }
      // Prediction off: byte-identical to the ordinary presentation.
      for(const auto object:objects)
        check(same_bytes(off_bank.sample(object,blend,models),ordinary_bank.sample(object,blend,models)) &&
            same_bytes(off_bank.sample(object,blend),ordinary_bank.sample(object,blend)),
            "Prediction off changed the presentation");
    }
  }
  const auto &stats=client.stats(0);
  check(stats.replayed>=1 && totals.correction_frames>=6,"Fixture did not exercise the decaying correction");
  check(stats.cuts>=2 && totals.cut_frames>0,"Fixture did not exercise a cut");
  check(totals.unpaired_frames>0 && totals.lag>.1f,"Fixture did not exercise lag and an unpaired bank");
  // Clearing the roots restores ordinary sampling exactly.
  predicted_bank.predict({});
  for(const float blend:{0.f,.4f,1.f})for(const auto object:objects)
    check(same_bytes(predicted_bank.sample(object,blend,models),ordinary_bank.sample(object,blend,models)) &&
        same_bytes(predicted_bank.sample(object,blend),ordinary_bank.sample(object,blend)),
        "Cleared prediction left a displaced body");
  std::printf("own body: %u frames (%u predicted, %u correcting, %u unpaired, %u cut), max lag %.4f wu, "
      "max root error %.3g wu, attachment error %.3g wu, pose error %.3g wu; replayed %llu, cuts %llu\n",
      totals.frames,totals.predicted_frames,totals.correction_frames,totals.unpaired_frames,totals.cut_frames,
      totals.lag,totals.root_error,totals.attachment_error,totals.pose_error,
      (unsigned long long)stats.replayed,(unsigned long long)stats.cuts);
}
// Only a shown, living, on-foot prediction selects its body.
void selection() {
  auto state=snapshot(20,{1,2,ground},{},0);
  const std::array<unsigned,3> slots{0,1,2};
  const Vec3 target{1.5f,2,ground};
  auto shown=state;shown.players[0].position=target;
  const std::array<std::optional<Vec3>,3> predicted{target,shown.players[1].position,shown.players[2].position};
  // Seated (slot 1) is never selected even when its shown position matches.
  auto roots=predicted_body_roots(state,state,shown,slots,predicted);
  check(roots.size()==2 && roots[0].object==own_unit && roots[1].object==remote_unit,"Seated body was selected");
  check(same_bits(roots[0].predicted,target) && same_bits(roots[0].current,state.players[0].position),
      "Predicted body lost its endpoints");
  // A displayed frame the client did not overwrite keeps its host body.
  auto native=shown;native.players[0].position={8,9,2};
  roots=predicted_body_roots(state,state,native,std::span(slots).first(1),std::span(predicted).first(1));
  check(roots.empty(),"An unpredicted displayed frame moved its body");
  auto dead=shown;dead.players[0].dead=true;
  check(predicted_body_roots(state,state,dead,std::span(slots).first(1),std::span(predicted).first(1)).empty(),
      "A dead view moved its body");
  // Repeated owners and non-finite origins are rejected by the bank.
  SourceWorldInterpolation bank;bank.reset(20,state.source_body_poses);
  bool rejected{};
  const std::array twice{SourceWorldPredictedRoot{own_unit,{},{},{}},SourceWorldPredictedRoot{own_unit,{},{},{}}};
  try{bank.predict(twice);}catch(const FormatError&){rejected=true;}
  check(rejected,"Repeated predicted body was admitted");
  rejected=false;
  const std::array invalid{SourceWorldPredictedRoot{own_unit,{},{},{std::nanf(""),0,0}}};
  try{bank.predict(invalid);}catch(const FormatError&){rejected=true;}
  check(rejected,"Non-finite predicted body was admitted");
}
}
int main()try {
  predicted_own_body();
  selection();
  std::puts("PASS own third-person body at the predicted origin; attachments follow; others and prediction-off unchanged");
  return 0;
}catch(const std::exception &e){std::fprintf(stderr,"Non-host body presentation: %s\n",e.what());return 1;}
