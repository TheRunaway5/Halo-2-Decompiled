#include "halo2/death_camera.hpp"
#include "halo2/cache.hpp"
#include "halo2/observer_world.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
float add(float a,float b){volatile float x=a+b;return x;}
float sub(float a,float b){volatile float x=a-b;return x;}
float mul(float a,float b){volatile float x=a*b;return x;}
float div(float a,float b){volatile float x=a/b;return x;}
const DeathCameraPlayer* player(std::span<const DeathCameraPlayer> p,std::uint32_t d){
  const auto i=std::find_if(p.begin(),p.end(),[&](const auto&v){return v.datum==d;});
  return i==p.end()?nullptr:&*i;
}
std::uint16_t draw(std::uint32_t&seed){seed=seed*0x19660du+0x3c6ef35fu;return std::uint16_t(seed>>16);}
std::uint32_t bits(float x){return std::bit_cast<std::uint32_t>(x);}
void vector(DeathCameraPacket&p,unsigned i,Vec3 v){p[i]=bits(v.x);p[i+1]=bits(v.y);p[i+2]=bits(v.z);}
Vec3 cross_source(Vec3 a,Vec3 b){return {sub(mul(a.y,b.z),mul(b.y,a.z)),
    sub(mul(b.x,a.z),mul(a.x,b.z)),sub(mul(a.x,b.y),mul(b.x,a.y))};}
float normalize(Vec3&v,float epsilon){
  //30BD0 keeps the products, sum and FSQRT at the original53-bit x87
  //precision. The epsilon comparison precedes the float reciprocal; its
  //below-epsilon branch returns zero and preserves the input vector.
  const double x=v.x,y=v.y,z=v.z;
  volatile double xx=x*x,yy=y*y,zz=z*z;
  volatile double xy=xx+yy,squared=xy+zz;
  const double magnitude=std::sqrt(squared);
  if(std::abs(magnitude)<double(epsilon))return 0;
  const float n=float(magnitude),r=div(1.f,n);
  v={mul(v.x,r),mul(r,v.y),mul(r,v.z)};
  return n;
}
}
std::uint32_t source_death_camera_next_player(std::span<const DeathCameraPlayer> bank,
    std::uint32_t own,std::uint32_t watched,bool team_only){
  const auto*local=player(bank,own);
  if(team_only&&!local)throw FormatError("Death camera requires its actual team owner");
  auto result=none;
  for(const auto&p:bank){
    if(p.datum==none||p.datum==own||p.current_unit==none||(team_only&&p.team!=local->team))continue;
    if(result!=none&&(watched&65535u)<(p.datum&65535u)){result=p.datum;break;}
    if(result==none)result=p.datum;
  }
  return result==none?watched:result;
}
bool source_death_camera_team_restriction(std::span<const DeathCameraPlayer> bank,
    std::uint32_t own) {
  const auto*local=player(bank,own);
  if(!local)throw FormatError("Campaign death camera requires its actual player owner");
  return std::any_of(bank.begin(),bank.end(),[&](const auto&p) {
    return p.datum!=none && p.datum!=own && p.team==local->team;
  });
}
void source_death_camera_replace_target(DeathCameraState&s,std::span<const DeathCameraPlayer> bank,
    bool team_only,const DeathCameraConstants&c,const DeathCameraSession&session){
  if(!session.multiplayer)team_only=source_death_camera_team_restriction(bank,s.player);
  s.watched_player=source_death_camera_next_player(bank,s.player,s.watched_player,team_only);
  const auto*p=player(bank,s.watched_player);const auto target=p?p->current_unit:none;
  if(target!=s.target&&target!=none){s.remaining=c.transition_seconds;s.target=target;s.attached_ticks=0;}
  s.watch_remaining=session.multiplayer?c.watch_seconds:c.transition_seconds;
}
DeathCameraState source_death_camera_create(Vec3 position,std::int8_t view,std::uint32_t own,
    std::span<const DeathCameraPlayer> bank,std::uint32_t&rng,const DeathCameraConstants&c,bool team_only,
    const DeathCameraSession&session){
  DeathCameraState s;s.initial_position=position;s.fov=c.default_fov;
  // Original x87 chains retain 53-bit intermediates until each FSTP32.
  s.distance=float((double(draw(rng))*double(c.random_scale))*double(c.distance_scale)+double(c.distance_bias));
  s.yaw=float((double(draw(rng))*double(c.random_scale))*double(c.full_turn));
  s.pitch=float(-((double(draw(rng))*double(c.random_scale))*double(c.pitch_scale)+double(c.pitch_bias)));
  s.remaining=c.transition_seconds;s.watch_remaining=session.multiplayer?c.watch_seconds:c.transition_seconds;
  s.player=own;s.watched_player=own;s.view=view;
  if(const auto*p=player(bank,own)){
    s.target=p->current_unit!=none?p->current_unit:p->previous_unit;
    if(s.target==none)source_death_camera_replace_target(s,bank,team_only,c,session);
  }
  return s;
}
DeathCameraPacket source_death_camera_packet(DeathCameraState&s,std::span<const DeathCameraPlayer> bank,
    const DeathCameraTargetLookup&lookup,const DeathCameraFrame&f,const DeathCameraConstants&c,
    const SourceObserverConstants&observer,const SourceObserverWorldConstants&world,
    const DeathCameraSession&session){
  if(!lookup)throw FormatError("Death camera requires actual current WORLD nodes");
  if(s.target!=none&&!lookup(s.target))s.target=none;
  if(s.target==none){source_death_camera_replace_target(s,bank,f.team_only,c,session);
    if(s.target==none) {
      if(session.multiplayer)source_death_camera_replace_target(s,bank,f.team_only,c,session);
      else {
        if(!session.fallback_target)throw FormatError("Campaign death camera requires original23FE30 object owner");
        s.target=session.fallback_target();
      }
    }}
  auto target=s.initial_position;
  const auto threshold=std::int32_t(std::nearbyint(mul(float(f.tick_rate),c.attached_seconds)));
  if(s.target!=none){
    if(const auto t=lookup(s.target)){
      target=t->node0;
      if((t->type&31u)==0){
        if(((t->biped_flags&0x8000u)||s.attached_ticks)&&s.attached_ticks<threshold)++s.attached_ticks;
        if(s.attached_ticks==threshold)s.anchor=f.observer_position;
      }
    }else s.attached_ticks=0;
  }
  DeathCameraPacket p{};vector(p,1,target);p[10]=bits(s.fov);
  if(s.attached_ticks==threshold){
    auto forward=target-s.anchor;const auto distance=normalize(forward,c.epsilon);
    const auto side=cross_source(f.observer_up,forward);
    auto up=cross_source(forward,side);normalize(up,c.epsilon);
    p[0]=0x19;p[9]=bits(distance);vector(p,11,forward);vector(p,14,up);
  }else{
    const double cp=std::cos(double(s.pitch));
    const Vec3 forward{float(std::cos(double(s.yaw))*cp),float(std::sin(double(s.yaw))*cp),float(std::sin(double(s.pitch)))};
    Vec3 side{forward.y,0.f-forward.x,0};
    if(normalize(side,c.epsilon)==0.f)side={c.one,0,0}; //11E7B0/46FC48.
    auto up=cross_source(side,forward);
    p[0]=1;p[9]=bits(s.distance);vector(p,11,forward);vector(p,14,up);
    p[34]=bits(s.remaining>=0.f?s.remaining:0.f);
  }
  p[35]=3; // group0 explicit duration/override flags.
  if(s.remaining==c.transition_seconds){p[9]=bits(c.half);p[40]=0;p[35]|=3u<<24;}
  s.remaining=sub(s.remaining,f.seconds);
  const auto watch=sub(s.watch_remaining,f.seconds);s.watch_remaining=watch>=0.f?watch:0.f;
  const auto watch_seconds=session.multiplayer?c.watch_seconds:c.transition_seconds;
  const bool cycle=s.player!=none&&f.cycle_pressed&&add(s.watch_remaining,c.one)<watch_seconds;
  if((s.watch_remaining==0.f&&(!f.clock_initialized||!f.clock_paused))||cycle)
    source_death_camera_replace_target(s,bank,f.team_only,c,session);
  source_observer_repair_packet(p,observer,c,world);
  return p;
}
float source_death_camera_distance(Vec3 forward,Vec3 up,Vec3 target,float distance,float margin,
    bool fog_side,const DeathCameraCollision&collision,const DeathCameraConstants&c){
  if(!collision)throw FormatError("Death camera requires its original collision query owner");
  const auto flags=fog_side?0x808c0du:0x808c0fu;
  const auto cast=[&](Vec3 point){return collision(flags,target,point-target);};
  const auto span=add(distance,margin);
  const Vec3 end{add(sub(0.f,mul(span,forward.x)),target.x),
      add(sub(0.f,mul(span,forward.y)),target.y),add(sub(0.f,mul(span,forward.z)),target.z)};
  const auto initial=cast(end);const float base=initial?*initial:c.one;
  float closest=base;const float spread=mul(distance,c.probe_scale);
  const Vec3 a{mul(up.x,spread),mul(up.y,spread),mul(up.z,spread)};
  const auto cross=cross_source(up,forward);
  const Vec3 b{mul(cross.x,spread),mul(cross.y,spread),mul(cross.z,spread)};
  const std::array offsets{a,b};std::optional<Vec3> selected;float sign{};
  for(unsigned i=0;i<4;++i){const float s=i&2?1.f:-1.f;const auto v=offsets[i&1];
    const auto hit=cast({add(mul(v.x,s),end.x),add(mul(v.y,s),end.y),add(mul(v.z,s),end.z)});
    if(hit&&*hit<closest){closest=*hit;sign=s;selected=v;}}
  float fraction=base;
  if(selected){float lo=sign,hi=0.f,lo_fraction=closest,hi_fraction=base;
    for(unsigned i=0;i<10;++i){const float mid=mul(add(lo,hi),c.half);const auto v=*selected;
      const auto hit=cast({add(mul(v.x,mid),end.x),add(mul(v.y,mid),end.y),add(mul(v.z,mid),end.z)});
      if(hit&&std::abs(sub(*hit,lo_fraction))<c.probe_tolerance){lo=mid;lo_fraction=*hit;}
      else{hi=mid;hi_fraction=hit?*hit:c.one;}}
    float blend=lo_fraction<=hi_fraction?lo:hi;
    // Original final sign fold retains negative-side magnitude.
    if(blend<0.f)blend=0.f-blend;
    fraction=add(mul(sub(c.one,blend),closest),mul(blend,base));
  }
  return mul(distance,fraction);
}
void source_death_camera_save_clusters(DeathCameraSavedClusters&s,std::uint32_t flags,
    std::int16_t bsp,std::span<const std::uint32_t,4> local,std::span<const Vec3,4> positions,
    const std::function<std::int16_t(Vec3)>&locate){
  if(flags&0x10u)return;
  if(!locate)throw FormatError("Saved death camera clusters require actual11D4A0 location owner");
  s.bsp=bsp;
  for(unsigned i=0;i<4;++i)s.clusters[i]=local[i]==none?-1:locate(positions[i]);
}
} // namespace halo2
