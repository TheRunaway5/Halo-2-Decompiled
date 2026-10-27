#include "halo2/replica_motion.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/math.hpp"
#include <glm/gtc/quaternion.hpp>
#include <bit>
#include <set>

namespace halo2 {
namespace {
using Quat=glm::quat;
constexpr double tick_interval=1.0/30.0;
Quat quat(const std::array<float,4> &q){return Quat(q[3],q[0],q[1],q[2]);}
std::array<float,4> array(const Quat &q){return {q.x,q.y,q.z,q.w};}
Vec3 rotate(const Quat &q,Vec3 v){const auto r=q*glm::vec3(v.x,v.y,v.z);return {r.x,r.y,r.z};}
bool finite(Vec3 v){return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);}
bool newer(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a-b)>0;}
// Orthonormal right-handed unit-scale columns (forward, left, up).
bool rigid(const Transform &t) {
  const auto &b=t.basis;
  for(const auto &axis:b)if(!finite(axis) || std::abs(length(axis)-1.f)>1e-3f)return false;
  return std::abs(dot(b[0],b[1]))<1e-3f && std::abs(dot(b[0],b[2]))<1e-3f && std::abs(dot(b[1],b[2]))<1e-3f &&
      dot(cross(b[0],b[1]),b[2])>.5f;
}
Quat orientation(const Transform &t) {
  return glm::normalize(glm::quat_cast(glm::mat3(glm::vec3(t.basis[0].x,t.basis[0].y,t.basis[0].z),
      glm::vec3(t.basis[1].x,t.basis[1].y,t.basis[1].z),glm::vec3(t.basis[2].x,t.basis[2].y,t.basis[2].z))));
}
// World-space rotation by angular velocity w over t seconds.
Quat spin(Vec3 w,double t) {
  const double angle=double(length(w))*t;
  if(!(angle>1e-9))return Quat(1,0,0,0);
  const auto axis=normalized(w);
  return glm::angleAxis(float(angle),glm::vec3(axis.x,axis.y,axis.z));
}
// Axis * angle of the shortest rotation.
Vec3 rotation_vector(Quat q) {
  if(q.w<0)q=-q;
  const double sine=std::sqrt(double(q.x)*q.x+double(q.y)*q.y+double(q.z)*q.z);
  if(!(sine>1e-9))return {};
  const double angle=2*std::atan2(sine,double(q.w));
  const auto scale=float(angle/sine);
  return {q.x*scale,q.y*scale,q.z*scale};
}
float angle_of(Quat q){return length(rotation_vector(q));}
bool interval(std::uint32_t newer_tick,std::uint32_t older_tick) {
  return newer_tick!=older_tick && !newer(older_tick,newer_tick) && newer_tick-older_tick<=4;
}
}
ReplicaMotion::ReplicaMotion(ReplicaMotionSettings settings,ProjectileGravity gravity,SceneRootAdmission admission)
    :settings_(settings),projectile_gravity_(std::move(gravity)),scene_root_admission_(std::move(admission)) {
  if(!(settings_.extrapolation_seconds>=0) || !(settings_.position_blend>0 && settings_.position_blend<=1) ||
      !(settings_.orientation_blend>0 && settings_.orientation_blend<=1) || !(settings_.snap_distance>0) ||
      !(settings_.snap_angle>0) || !std::isfinite(settings_.gravity))
    throw FormatError("Invalid replica motion settings");
}
void ReplicaMotion::reset() {
  tracks_.clear();have_tick_=have_serial_=false;last_tick_=impulse_serial_=0;
}
ReplicaMotion::Pose ReplicaMotion::evaluate(const Track &track,double seconds)const {
  const double elapsed=std::clamp(seconds-track.base_time,0.0,double(settings_.extrapolation_seconds));
  const auto e=float(elapsed);
  // Projectile flight (F8EB0) is the exact ballistic segment. Havok adds
  // gravity to the velocity before integrating position (symplectic Euler),
  // which this matches at every tick boundary.
  const bool projectile=track.kind==Kind::weapon_projectile || track.kind==Kind::grenade;
  const float fall=projectile?.5f*e*e:.5f*e*(e+float(tick_interval));
  Pose pose{track.position+track.velocity*e+track.acceleration*fall,track.rotation};
  if(track.rotates)pose.rotation=array(glm::normalize(spin(track.angular,elapsed)*quat(track.rotation)));
  if(track.error) {
    const auto [position,orientation]=correction_weights(seconds-track.base_time);
    pose.position=pose.position+track.error_position*position;
    if(track.rotates)pose.rotation=array(glm::normalize(
        glm::slerp(Quat(1,0,0,0),quat(track.error_rotation),orientation)*quat(pose.rotation)));
  }
  return pose;
}
// Remaining share of the receipt error: (1-blend)^ticks, continuous in time.
// Restarting it at every snapshot keeps the presented path continuous; a
// newer receipt simply measures what remains (95% of a position error is
// gone after 5 ticks, of an orientation error after 7).
std::pair<float,float> ReplicaMotion::correction_weights(double elapsed)const {
  const double ticks=std::max(elapsed,0.0)/tick_interval;
  return {float(std::pow(1.0-settings_.position_blend,ticks)),float(std::pow(1.0-settings_.orientation_blend,ticks))};
}
std::optional<Transform> ReplicaMotion::motion(const Track &track,double seconds)const {
  const auto [position_left,orientation_left]=track.error?correction_weights(seconds-track.base_time):std::pair{0.f,0.f};
  const bool correcting=position_left*length(track.error_position)>1e-5f ||
      orientation_left*angle_of(quat(track.error_rotation))>1e-5f;
  const auto zero=[](Vec3 v){return v.x==0 && v.y==0 && v.z==0;};
  if(!correcting && zero(track.velocity) && zero(track.acceleration) && (!track.rotates || zero(track.angular)))
    return std::nullopt; // Exactly the newest authoritative root.
  const auto pose=evaluate(track,seconds);
  // Rigid motion carrying the authoritative root (p0, q0) onto (P, Q).
  Transform result;
  if(track.rotates) {
    const auto delta=glm::normalize(quat(pose.rotation)*glm::inverse(quat(track.rotation)));
    result.basis={rotate(delta,{1,0,0}),rotate(delta,{0,1,0}),rotate(delta,{0,0,1})};
    result.position=pose.position-rotate(delta,track.position);
  }else result.position=pose.position-track.position;
  if(!finite(result.position))return std::nullopt;
  return result;
}
void ReplicaMotion::observe(Kind kind,std::uint32_t object,std::uint32_t definition,const Transform &root,
    std::optional<Vec3> velocity,std::uint32_t tick,double seconds) {
  if(object==null_datum || !finite(root.position) || (velocity && !finite(*velocity)))return;
  auto [at,created]=tracks_.try_emplace(key(kind,object));
  auto &track=at->second;
  std::optional<Pose> shown;
  if(!created && track.samples)shown=evaluate(track,seconds);
  track.kind=kind;track.object=object;track.definition=definition;track.generation=generation_;
  track.root=root;
  const bool projectile=kind==Kind::weapon_projectile || kind==Kind::grenade;
  track.rotates=!projectile && rigid(root);
  Sample sample{tick,root.position,track.rotates?array(orientation(root)):std::array<float,4>{0,0,0,1},velocity};
  if(track.samples && track.history[0].tick==tick)track.history[0]=sample;
  else {
    for(unsigned i=unsigned(track.history.size())-1;i>0;--i)track.history[i]=track.history[i-1];
    track.history[0]=sample;track.samples=std::min<unsigned>(track.samples+1,unsigned(track.history.size()));
  }
  const auto &h=track.history;
  const auto seconds_between=[&](unsigned a,unsigned b){return float(double(h[a].tick-h[b].tick)*tick_interval);};
  const auto paired=[&](unsigned a){return track.samples>a+1 && interval(h[a].tick,h[a+1].tick);};
  // Velocity at sample i: replicated, or the displacement over the interval
  // ending at it. Havok integrates position with its post-step velocity, so
  // that displacement is the body's velocity at sample i. (Projectiles always
  // replicate theirs.)
  const auto velocity_at=[&](unsigned i)->std::optional<Vec3> {
    if(i<track.samples && h[i].velocity)return h[i].velocity;
    if(paired(i))return (h[i].position-h[i+1].position)/seconds_between(i,i+1);
    return std::nullopt;
  };
  track.position=sample.position;track.rotation=sample.rotation;track.base_time=seconds;
  track.moving=paired(0) && (length(h[0].position-h[1].position)>0 || h[0].rotation!=h[1].rotation);
  track.acceleration={};track.angular={};
  // Gravity where the original applies it.
  if(projectile) {
    std::optional<float> gravity=projectile_gravity_?projectile_gravity_(definition):std::nullopt;
    if(gravity && std::isfinite(*gravity))track.acceleration={0,0,0.f-*gravity};
    else if(paired(0) && h[0].velocity && h[1].velocity) {
      const auto a=(*h[0].velocity-*h[1].velocity)/seconds_between(0,1);
      const float g=settings_.gravity;
      if(finite(a) && a.z<=0 && a.z>=-4*g && std::abs(a.x)<=.25f*g && std::abs(a.y)<=.25f*g)track.acceleration={0,0,a.z};
    }
  }else {
    // A Havok body in free flight gains exactly -g*dt between snapshots;
    // contact (resting, sliding, bouncing) does not, and gets no gravity.
    const auto v0=velocity_at(0),v1=velocity_at(1);
    if(v0 && v1 && paired(0)) {
      const auto a=(*v0-*v1)/seconds_between(0,1);const float g=settings_.gravity;
      if(finite(a) && std::abs(a.z+g)<=.3f*g && std::abs(a.x)<=.3f*g && std::abs(a.y)<=.3f*g)
        track.acceleration={0,0,0.f-g};
    }
  }
  if(velocity)track.velocity=*velocity;
  else if(const auto derived=velocity_at(0))track.velocity=*derived;
  else track.velocity={};
  if(track.rotates && paired(0)) {
    const auto w=rotation_vector(quat(h[0].rotation)*glm::inverse(quat(h[1].rotation)));
    // More than ~86 degrees per 33 ms is a cut, not a spin; below 1e-4
    // radians is the rounding of an unchanged basis.
    if(length(w)>1e-4f && length(w)<1.5f*float(h[0].tick-h[1].tick))track.angular=w/seconds_between(0,1);
  }
  track.error=false;track.error_position={};track.error_rotation={0,0,0,1};
  if(shown) {
    const auto error=shown->position-track.position;
    const auto turn=track.rotates?glm::normalize(quat(shown->rotation)*glm::inverse(quat(track.rotation))):Quat(1,0,0,0);
    const bool position_snap=!finite(error) || length(error)>settings_.snap_distance;
    const bool rotation_snap=!(angle_of(turn)<=settings_.snap_angle);
    if(position_snap || rotation_snap)++statistics_.snaps;
    // Below 1e-4 world units/radians is rounding, not a disagreement.
    const bool position_error=!position_snap && length(error)>1e-4f;
    const bool rotation_error=!rotation_snap && angle_of(turn)>1e-4f;
    if(position_error)track.error_position=error;
    if(rotation_error)track.error_rotation=array(turn);
    track.error=position_error || rotation_error;
    if(track.error)++statistics_.corrections;
  }
}
void ReplicaMotion::receive(const MultiplayerState &state,double seconds) {
  if(!std::isfinite(seconds))throw FormatError("Replica motion requires a finite receipt clock");
  // A rollback or a long gap is a new presentation history.
  if(have_tick_ && (newer(last_tick_,state.tick) || state.tick-last_tick_>30))reset();
  const auto previous_tick=have_tick_?last_tick_:state.tick-1;
  ++generation_;++statistics_.receipts;
  std::set<std::uint32_t> occupied;
  for(const auto &player:state.players)
    if(player.connected && !player.dead && player.source_parent!=null_datum)occupied.insert(player.source_parent);
  for(const auto &p:state.weapon_projectiles)
    observe(Kind::weapon_projectile,p.datum,p.definition,source_object_transform(p.position,p.forward,p.up,1,false),
        p.velocity,state.tick,seconds);
  for(const auto &g:state.grenade_projectiles)if(g.parent==null_datum) {
    Transform root;root.position=g.position;
    observe(Kind::grenade,g.datum,g.definition,root,g.velocity,state.tick,seconds);
  }
  for(const auto &w:state.world_weapons)
    observe(Kind::world_weapon,w.handle,w.definition,source_object_transform(w.position,w.forward,w.up,1,false),
        std::nullopt,state.tick,seconds);
  for(const auto &v:state.world_vehicles)if(!occupied.contains(v.handle))
    observe(Kind::vehicle,v.handle,v.placement,v.transform,
        v.lighting_owner==v.handle?std::optional{v.lighting_velocity}:std::nullopt,state.tick,seconds);
  for(const auto &o:state.source_dynamic_objects)if(o.parent==null_datum && !o.frozen)
    observe(Kind::dynamic_object,o.object,o.definition,o.transform,o.velocity,state.tick,seconds);
  for(const auto &r:state.source_scene_roots)if(r.visible && !r.frozen && (!scene_root_admission_ || scene_root_admission_(r)))
    observe(Kind::scene_root,r.object,r.definition,r.transform,r.velocity,state.tick,seconds);
  std::erase_if(tracks_,[&](const auto &entry){return entry.second.generation!=generation_;});
  // Completed impulses after the previous snapshot replace the body's motion
  // now: this snapshot's root plus the velocity the impulse produced.
  for(const auto &event:state.physics_impulses) {
    if(have_serial_ && !newer(event.serial,impulse_serial_))continue;
    impulse_serial_=event.serial;have_serial_=true;
    if(!newer(event.tick,previous_tick))continue;
    for(const auto kind:{Kind::world_weapon,Kind::vehicle,Kind::dynamic_object,Kind::scene_root}) {
      const auto found=tracks_.find(key(kind,event.object));
      if(found==tracks_.end())continue;
      auto &track=found->second;
      track.velocity=event.linear_velocity;
      if(track.rotates)track.angular=event.angular_velocity;
      // Launched upward means airborne; the original then applies gravity.
      if(track.acceleration.z==0 && event.linear_velocity.z>.05f)track.acceleration={0,0,0.f-settings_.gravity};
      ++statistics_.impulses;
    }
  }
  last_tick_=state.tick;have_tick_=true;
}
std::optional<Transform> ReplicaMotion::root(Kind kind,std::uint32_t object,double seconds)const {
  const auto found=tracks_.find(key(kind,object));
  if(found==tracks_.end())return std::nullopt;
  if(const auto rigid=motion(found->second,seconds))return compose_transform(*rigid,found->second.root);
  return found->second.root;
}
void ReplicaMotion::present(MultiplayerState &shown,double seconds)const {
  const auto presented=[&](Kind kind,std::uint32_t object)->std::optional<Transform> {
    if(!tracks_.contains(key(kind,object)))return std::nullopt;
    return root(kind,object,seconds);
  };
  for(auto &p:shown.weapon_projectiles)if(const auto t=presented(Kind::weapon_projectile,p.datum))p.position=t->position;
  for(auto &g:shown.grenade_projectiles)if(g.parent==null_datum)
    if(const auto t=presented(Kind::grenade,g.datum))g.position=t->position;
  for(auto &w:shown.world_weapons)if(const auto t=presented(Kind::world_weapon,w.handle)) {
    w.position=t->position;w.forward=t->basis[0];w.up=t->basis[2];
  }
  for(auto &v:shown.world_vehicles)if(const auto t=presented(Kind::vehicle,v.handle))v.transform=*t;
  for(auto &o:shown.source_dynamic_objects)if(const auto t=presented(Kind::dynamic_object,o.object))o.transform=*t;
  for(auto &r:shown.source_scene_roots)if(const auto t=presented(Kind::scene_root,r.object))r.transform=*t;
}
std::vector<std::pair<std::uint32_t,Transform>> ReplicaMotion::node_motions(double seconds)const {
  std::vector<std::pair<std::uint32_t,Transform>> result;
  for(const auto &[_,track]:tracks_) {
    if(track.kind==Kind::weapon_projectile || track.kind==Kind::grenade)continue;
    // Moving bodies present their newest nodes, like their roots, even when
    // the dead-reckoned motion is momentarily exactly zero.
    if(const auto rigid=motion(track,seconds))result.emplace_back(track.object,*rigid);
    else if(track.moving)result.emplace_back(track.object,Transform{});
  }
  return result;
}
}
