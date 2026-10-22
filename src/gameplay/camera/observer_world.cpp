#include "halo2/observer_world.hpp"
#include "halo2/source_camera.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
float add(float a,float b){volatile float v=a+b;return v;}
float sub(float a,float b){volatile float v=a-b;return v;}
float mul(float a,float b){volatile float v=a*b;return v;}
float div(float a,float b){volatile float v=a/b;return v;}
float f(const SourceObserverState&s,unsigned at){return std::bit_cast<float>(s.at(at/4));}
void f(SourceObserverState&s,unsigned at,float v){s.at(at/4)=std::bit_cast<std::uint32_t>(v);}
Vec3 vec(const SourceObserverState&s,unsigned at){return {f(s,at),f(s,at+4),f(s,at+8)};}
void vec(SourceObserverState&s,unsigned at,Vec3 v){f(s,at,v.x);f(s,at+4,v.y);f(s,at+8,v.z);}
float pf(const DeathCameraPacket&p,unsigned at){return std::bit_cast<float>(p.at(at/4));}
void pf(DeathCameraPacket&p,unsigned at,float v){p.at(at/4)=std::bit_cast<std::uint32_t>(v);}
Vec3 pv(const DeathCameraPacket&p,unsigned at){return {pf(p,at),pf(p,at+4),pf(p,at+8)};}
void pv(DeathCameraPacket&p,unsigned at,Vec3 v){pf(p,at,v.x);pf(p,at+4,v.y);pf(p,at+8,v.z);}
bool finite(float x){return (std::bit_cast<std::uint32_t>(x)&0x7f800000u)!=0x7f800000u;}
bool finite(Vec3 v){return finite(v.x)&&finite(v.y)&&finite(v.z);}
bool unit(Vec3 v,const SourceObserverConstants&c){
  const auto d=sub(add(add(mul(v.x,v.x),mul(v.y,v.y)),mul(v.z,v.z)),c.scalar.one);
  return finite(d)&&std::abs(d)<c.orthogonal_tolerance;
}
bool basis(Vec3 forward,Vec3 up,const SourceObserverConstants&c){
  if(!unit(forward,c)||!unit(up,c))return false;
  const auto d=add(add(mul(up.z,forward.z),mul(up.y,forward.y)),mul(forward.x,up.x));
  return finite(d)&&std::abs(d)<c.orthogonal_tolerance;
}
bool bounded(float x,float lo,float hi){return finite(x)&&x>=lo&&x<=hi;}
bool bounded(Vec3 v,float lo,float hi){return bounded(v.x,lo,hi)&&bounded(v.y,lo,hi)&&bounded(v.z,lo,hi);}
// COMISS/JA/JBE preserve an unordered value here. The final validity owner
// subsequently replaces the complete packet if a NaN survived the repair.
float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
Vec3 clamp(Vec3 v,float lo,float hi){return {clamp(v.x,lo,hi),clamp(v.y,lo,hi),clamp(v.z,lo,hi)};}
Vec3 parent(const SourceObserverState&s,Vec3 v,bool point){
  const auto scale=f(s,0x14c);
  if(scale!=1.f)v={mul(scale,v.x),mul(scale,v.y),mul(scale,v.z)};
  Vec3 out{add(add(mul(f(s,0x168),v.z),mul(f(s,0x15c),v.y)),mul(f(s,0x150),v.x)),
    add(add(mul(f(s,0x16c),v.z),mul(f(s,0x160),v.y)),mul(f(s,0x154),v.x)),
    add(add(mul(f(s,0x170),v.z),mul(f(s,0x164),v.y)),mul(f(s,0x158),v.x))};
  if(point)out={add(out.x,f(s,0x174)),add(out.y,f(s,0x178)),add(out.z,f(s,0x17c))};
  return out;
}
}
std::int16_t SourceObserverLocation::cluster()const{return std::bit_cast<std::int16_t>(std::uint16_t(words[1]));}
bool source_observer_packet_valid(const DeathCameraPacket&p,const SourceObserverConstants&c,
    const DeathCameraConstants&d,const SourceObserverWorldConstants&w){
  if(!(p[0]&1u))return true;
  return basis(pv(p,0x2c),pv(p,0x38),c)&&
    bounded(pv(p,4),d.minimum_coordinate,d.maximum_coordinate)&&
    bounded(pv(p,0x10),d.minimum_coordinate,d.maximum_coordinate)&&finite(pv(p,0x44))&&
    bounded(pf(p,0x24),0.f,w.maximum_distance)&&
    bounded(pf(p,0x28),c.scalar.packet_minimum,c.scalar.packet_maximum)&&
    bounded(pf(p,0x88),0.f,w.packet_maximum_seconds);
}
void source_observer_repair_packet(DeathCameraPacket&p,const SourceObserverConstants&c,
    const DeathCameraConstants&d,const SourceObserverWorldConstants&w){
  if(source_observer_packet_valid(p,c,d,w))return;
  if(!basis(pv(p,0x2c),pv(p,0x38),c)){pv(p,0x2c,w.default_forward);pv(p,0x38,w.default_up);}
  pv(p,4,clamp(pv(p,4),d.minimum_coordinate,d.maximum_coordinate));
  pv(p,0x10,clamp(pv(p,0x10),d.minimum_coordinate,d.maximum_coordinate));
  if(!finite(pv(p,0x44)))pv(p,0x44,w.zero);
  pf(p,0x28,clamp(pf(p,0x28),c.scalar.packet_minimum,c.scalar.packet_maximum));
  pf(p,0x24,clamp(pf(p,0x24),0.f,w.maximum_distance));
  if(!source_observer_packet_valid(p,c,d,w)){
    p={};pv(p,0x2c,w.default_forward);pv(p,0x38,w.default_up);pf(p,0x28,c.scalar.default_fov);
  }
}
void source_observer_reset(SourceObserverState&s,std::int16_t bsp,
    const SourceObserverConstants&c,const SourceObserverWorldConstants&w){
  vec(s,0x134,w.default_forward);vec(s,0x140,w.default_up);f(s,0x130,c.scalar.default_fov);
  vec(s,0xb8,w.zero);s[0xc4/4]=0xffffffffu;
  s[0xc8/4]=std::uint16_t(0xffffu)|(std::uint32_t(std::uint16_t(bsp))<<16);
  vec(s,0xcc,w.zero);vec(s,0xd8,w.default_forward);vec(s,0xe4,w.default_up);
  f(s,0xf0,c.scalar.default_fov);f(s,0xf8,0);f(s,0xfc,0);
  for(unsigned i=2;i<2+43;++i)s[i]=0;
  vec(s,0x34,w.default_forward);vec(s,0x40,w.default_up);f(s,0x30,c.scalar.default_fov);
  s[0]=s[0x354/4]=0x72616421u;
  s[0xb4/4]=(s[0xb4/4]&0xff000000u)|1u;
}
void source_observer_world(SourceObserverState&s,const SourceObserverWorldFrame&frame,
    const SourceObserverWorldServices&services,const SourceObserverConstants&c,
    const DeathCameraConstants&d,const SourceObserverWorldConstants&w){
  float distance=clamp(f(s,0x12c),0.f,w.maximum_initial_distance);
  if(!basis(vec(s,0x134),vec(s,0x140),c)){vec(s,0x134,w.default_forward);vec(s,0x140,w.default_up);}
  const auto fov=clamp(f(s,0x130),c.scalar.render_minimum,c.scalar.render_maximum);f(s,0x130,fov);
  const auto position=clamp(vec(s,0x10c),d.minimum_coordinate,d.maximum_coordinate);
  distance=clamp(distance,0.f,w.maximum_distance);
  auto forward=vec(s,0x134);
  const auto magnitude=float(std::sqrt(double(forward.x)*double(forward.x)+double(forward.y)*double(forward.y)));
  float x=forward.x,y=forward.y;
  if(std::abs(magnitude)>=c.epsilon){const auto inverse=div(c.scalar.one,magnitude);x=mul(inverse,x);y=mul(inverse,y);}
  Vec3 target{add(add(mul(f(s,0x11c),y),mul(f(s,0x118),x)),position.x),
    add(sub(mul(f(s,0x118),y),mul(f(s,0x11c),x)),position.y),add(f(s,0x120),position.z)};
  vec(s,0xd8,forward);vec(s,0xe4,vec(s,0x140));
  const auto velocity=vec(s,0x180);vec(s,0xcc,{sub(0.f,velocity.x),sub(0.f,velocity.y),sub(0.f,velocity.z)});
  f(s,0xf0,fov);f(s,0xf4,w.aspect);f(s,0xf8,f(s,0x124));f(s,0xfc,f(s,0x128));
  const auto aim=source_observer_aim(forward,vec(s,0xe4),fov,{f(s,0xf8),f(s,0xfc)},w.aspect);
  vec(s,0xd8,aim.forward);vec(s,0xe4,aim.up);f(s,0x100,aim.displacement);
  //173810 stores the observer vertical angle before the separate2F800 factor.
  f(s,0x104,float(2*std::atan(std::tan(double(fov)*double(c.scalar.half))/double(w.aspect))));
  f(s,0x108,div(fov,c.scalar.default_fov));
  if((s[0xb4/4]>>16)&255u){
    target=parent(s,target,true);
    auto v=parent(s,vec(s,0xcc),false);const auto extra=vec(s,0x4c);
    vec(s,0xcc,{add(v.x,extra.x),add(v.y,extra.y),add(v.z,extra.z)});
    vec(s,0xd8,parent(s,vec(s,0xd8),false));vec(s,0xe4,parent(s,vec(s,0xe4),false));
  }
  if(!(s[2]&0x10u)&&distance!=0.f){
    if(!services.fog_side)throw std::runtime_error("Required original observer fog owner unavailable");
    distance=source_death_camera_distance(vec(s,0xd8),vec(s,0xe4),target,distance,w.margin,services.fog_side(target),services.collision,d);
  }
  forward=vec(s,0xd8);
  vec(s,0xb8,clamp({sub(target.x,mul(forward.x,distance)),sub(target.y,mul(forward.y,distance)),
      sub(target.z,mul(forward.z,distance))},d.minimum_coordinate,d.maximum_coordinate));
  if(!services.locate)throw std::runtime_error("Required original observer location owner unavailable");
  const auto location=services.locate(vec(s,0xb8));
  const auto request=[&](std::int32_t cluster){
    if(!services.request_visibility)throw std::runtime_error("Required original observer PVS owner unavailable");
    services.request_visibility(cluster);
  };
  const auto cluster=location.cluster();
  if(cluster!=-1){
    if(cluster!=std::bit_cast<std::int16_t>(std::uint16_t(s[0xc8/4]))&&cluster>=0&&cluster<frame.cluster_count)request(cluster);
    s[0xc4/4]=location.words[0];s[0xc8/4]=location.words[1];
  }
  if(frame.view<0||frame.view>=4)throw std::runtime_error("Original observer requires its actual local view");
  const auto saved=frame.saved_clusters.clusters[unsigned(frame.view)];
  if(frame.current_unit==0xffffffffu&&!(frame.player_flags&8u)&&frame.saved_clusters.bsp==frame.bsp&&saved>=0&&saved<frame.cluster_count)request(saved);
  if(std::abs(w.vertical_bias_test)<double(w.vertical_bias_threshold))f(s,0xc0,sub(f(s,0xc0),w.maximum_initial_distance));
}
} // namespace halo2
