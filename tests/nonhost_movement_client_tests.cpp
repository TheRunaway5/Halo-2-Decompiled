#include "halo2/nonhost_movement_client.hpp"
#include "halo2/math.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value,const char *message) {
  if(!value)throw std::runtime_error(message);
}
bool same(Vec3 a,Vec3 b) {return a.x==b.x && a.y==b.y && a.z==b.z;}
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
MultiplayerState::Player walking() {
  MultiplayerState::Player p;p.connected=true;p.source_unit=0x20002u;
  p.native_physical_state=1;p.position={0,0,tu15_biped_origin_height+.0001f};
  p.native_grounded=true;return p;
}
LanClient::InputReceipt receipt(std::uint32_t sequence,bool known=true) {
  LanClient::InputReceipt r;r.known=known;r.consumed=sequence;return r;
}
void activate(NonHostMovementClient &client,const MultiplayerState::Player &p,
    const BipedDefinition &b,const CollisionWorld &collision) {
  Controls run;run.forward=1;
  check(!client.predict(0,10,run,0,0,p,b,collision),"Prediction started before an authority baseline");
  client.reconcile(0,p,receipt(10),collision);
  for(unsigned sequence=11;sequence<=15;++sequence)
    check(client.predict(0,sequence,run,0,0,p,b,collision).has_value(),"Ordinary native state1 did not predict");
  MultiplayerState shown;shown.players[0]=p;const std::array<unsigned,1> slots{0};
  client.present(shown,slots,1);
  check(shown.players[0].position.x>p.position.x,"Ordinary input did not move the presented local player");
}
// A declared source mode6 receipt is a native Sword/flying trajectory, not
// another walking baseline. This uses the production client interface, with
// intentionally outstanding walking inputs on both sides of the cut.
void native_motion_transition(std::uint8_t physical_state,bool known_receipt) {
  const auto collision=floor();const auto b=biped();auto p=walking();
  NonHostMovementClient client(NetcodeMode::balanced,movement(),std::bit_cast<std::uint32_t>(.01f));
  activate(client,p,b,collision);
  p.native_physical_state=physical_state;p.position={40,-12,3};p.velocity={18,-4,2};p.native_grounded=false;
  client.reconcile(0,p,receipt(12,known_receipt),collision);
  const std::array<unsigned,1> slots{0};
  for(float fraction:{0.f,.5f,1.f}) {
    MultiplayerState shown;shown.players[0]=p;client.present(shown,slots,fraction);
    check(same(shown.players[0].position,p.position) && same(shown.players[0].velocity,p.velocity),
        "Walking presentation overwrote received native special motion");
  }
  Controls attack;attack.fire=attack.melee=attack.reload=attack.switch_weapon=attack.jump=true;
  attack.forward=.75f;attack.right=-.5f;
  check(!client.predict(0,16,attack,0,0,p,b,collision),"Native special motion emitted a walking report");
  check(attack.fire && attack.melee && attack.reload && attack.switch_weapon && attack.jump &&
      attack.forward==.75f && attack.right==-.5f,"Movement gating changed outgoing gameplay controls");

  // Same unit, no teleport: returning to mode1 must use this current source
  // position, not replay the old queued forward inputs13..15 from before the
  // native motion cut. No new local input has occurred yet.
  p.native_physical_state=1;p.position={50,20,tu15_biped_origin_height+.0001f};
  p.velocity={};p.native_grounded=true;
  client.reconcile(0,p,receipt(12),collision);
  MultiplayerState resumed;resumed.players[0]=p;client.present(resumed,slots,1);
  check(same(resumed.players[0].position,p.position) && same(resumed.players[0].velocity,p.velocity),
      "Returning to native state1 replayed movement from before the special-motion cut");
  Controls neutral;
  check(client.predict(0,17,neutral,0,0,p,b,collision).has_value(),"Ordinary prediction did not resume from fresh authority");
  client.present(resumed,slots,1);
  check(std::abs(resumed.players[0].position.x-p.position.x)<1e-5f &&
      std::abs(resumed.players[0].position.y-p.position.y)<1e-5f,
      "Resumed neutral prediction snapped back into stale walking history");
}
void displayed_native_frame() {
  const auto collision=floor();const auto b=biped();const auto p=walking();
  NonHostMovementClient client(NetcodeMode::balanced,movement(),std::bit_cast<std::uint32_t>(.01f));
  activate(client,p,b,collision);
  // Adaptive playout can display an older special-motion endpoint after the
  // newest received endpoint has already resumed ordinary prediction.
  MultiplayerState shown;shown.players[0]=p;shown.players[0].native_physical_state=6;
  shown.players[0].position={8,9,2};shown.players[0].velocity={12,3,1};
  const auto native=shown.players[0];const std::array<unsigned,1> slots{0};client.present(shown,slots,1);
  check(same(shown.players[0].position,native.position) && same(shown.players[0].velocity,native.velocity),
      "New walking prediction overwrote an older displayed native special-motion frame");
}
}
int main()try {
  native_motion_transition(6,true);
  native_motion_transition(6,false);
  native_motion_transition(2,true);
  native_motion_transition(0xff,true);
  displayed_native_frame();
  std::puts("Non-host client preserves native special motion and reseeds ordinary prediction");return 0;
}catch(const std::exception &e){std::fprintf(stderr,"Non-host movement client: %s\n",e.what());return 1;}
