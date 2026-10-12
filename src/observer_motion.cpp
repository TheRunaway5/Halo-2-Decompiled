#include "halo2/observer_motion.hpp"
#include "halo2/transform.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace halo2 {
namespace {
float add(float a,float b){volatile float v=a+b;return v;}
float sub(float a,float b){volatile float v=a-b;return v;}
float mul(float a,float b){volatile float v=a*b;return v;}
float div(float a,float b){volatile float v=a/b;return v;}
float f(const SourceObserverState&s,unsigned at){return std::bit_cast<float>(s.at(at/4));}
void f(SourceObserverState&s,unsigned at,float v){s.at(at/4)=std::bit_cast<std::uint32_t>(v);}
unsigned byte(const SourceObserverState&s,unsigned at){return (s.at(at/4)>>((at&3)*8))&255u;}
void byte(SourceObserverState&s,unsigned at,unsigned v){auto&w=s.at(at/4);const auto shift=(at&3)*8;
  w=(w&~(255u<<shift))|((v&255u)<<shift);}
float pf(const DeathCameraPacket&p,unsigned at){return std::bit_cast<float>(p.at(at/4));}
void pf(DeathCameraPacket&p,unsigned at,float v){p.at(at/4)=std::bit_cast<std::uint32_t>(v);}
unsigned pb(const DeathCameraPacket&p,unsigned at){return (p.at(at/4)>>((at&3)*8))&255u;}
Vec3 vec(const SourceObserverState&s,unsigned at){return {f(s,at),f(s,at+4),f(s,at+8)};}
void vec(SourceObserverState&s,unsigned at,Vec3 v){f(s,at,v.x);f(s,at+4,v.y);f(s,at+8,v.z);}
Vec3 cross_source(Vec3 a,Vec3 b){return {sub(mul(a.y,b.z),mul(b.y,a.z)),
    sub(mul(b.x,a.z),mul(a.x,b.z)),sub(mul(a.x,b.y),mul(b.x,a.y))};}
float normalize(Vec3&v,const SourceObserverConstants&c){
  const auto n=float(std::sqrt(double(add(add(mul(v.x,v.x),mul(v.y,v.y)),mul(v.z,v.z)))));
  if(std::abs(n)>=c.epsilon){const auto inverse=div(c.scalar.one,n);
    v={mul(v.x,inverse),mul(v.y,inverse),mul(v.z,inverse)};}
  return n;
}
std::array<float,4> quaternion(const Transform&t,const SourceObserverConstants&c){
  const auto a=std::bit_cast<std::array<float,9>>(t.basis);
  const auto trace=add(add(a[4],a[0]),a[8]);std::array<float,4>q{};
  if(trace>0.f){const auto s=float(std::sqrt(double(trace)+double(c.scalar.one))*double(c.scalar.half));
    const auto inverse=div(c.quarter,s);
    q={mul(sub(a[5],a[7]),inverse),mul(sub(a[6],a[2]),inverse),mul(sub(a[1],a[3]),inverse),s};
  }else{unsigned i=a[4]>a[0]?1:0;if(a[8]>a[i*4])i=2;const auto j=(i+1)%3,k=(i+2)%3;
    const auto s=float(std::sqrt(((double(a[i*4])-double(a[j*4]))-double(a[k*4]))+double(c.scalar.one))*double(c.scalar.half));
    const auto inverse=div(c.quarter,s);q[i]=s;q[3]=mul(sub(a[j*3+k],a[k*3+j]),inverse);
    q[j]=mul(add(a[i*3+j],a[j*3+i]),inverse);q[k]=mul(add(a[i*3+k],a[k*3+i]),inverse);}
  if(q[3]<0.f)for(auto&v:q)v=sub(0.f,v);return q;
}
Vec3 rotation_delta(Vec3 forward,Vec3 up,Vec3 target_forward,Vec3 target_up,const SourceObserverConstants&c){
  const Transform current{c.scalar.one,{forward,cross_source(up,forward),up},{}};
  const Transform target{c.scalar.one,{target_forward,cross_source(target_up,target_forward),target_up},{}};
  const auto q=quaternion(compose_transform(target,inverse_transform(current)),c);
  Vec3 axis{q[0],q[1],q[2]};const float magnitude=normalize(axis,c);
  const double wide=std::atan2(double(magnitude),double(q[3]));float angle=float(wide+wide);
  if(double(c.pi)<wide+wide){axis={sub(0.f,axis.x),sub(0.f,axis.y),sub(0.f,axis.z)};angle=sub(c.full_turn,angle);}
  return {mul(axis.x,angle),mul(axis.y,angle),mul(axis.z,angle)};
}
void rotate(Vec3 delta,Vec3&forward,Vec3&up,const SourceObserverConstants&c){
  const auto angle=normalize(delta,c);if(!(std::abs(angle)>=c.epsilon)||angle==0.f)return;
  const auto sine=float(std::sin(double(angle))),cosine=float(std::cos(double(angle))),rest=sub(c.scalar.one,cosine);
  const auto&axis=delta;
  const auto dot=mul(add(add(mul(forward.x,axis.x),mul(axis.z,forward.z)),mul(axis.y,forward.y)),rest);
  const auto old=forward;
  forward.x=sub(add(mul(old.x,cosine),mul(dot,axis.x)),mul(sub(mul(axis.z,old.y),mul(axis.y,old.z)),sine));
  forward.z=sub(add(mul(dot,axis.z),mul(cosine,old.z)),mul(sub(mul(old.x,axis.y),mul(axis.x,old.y)),sine));
  forward.y=sub(add(mul(dot,axis.y),mul(cosine,old.y)),mul(sub(mul(axis.x,old.z),mul(old.x,axis.z)),sine));
  const auto d=mul(add(add(mul(up.z,axis.z),mul(up.y,axis.y)),mul(up.x,axis.x)),rest);const auto u=up;
  up.x=sub(add(mul(u.x,cosine),mul(d,axis.x)),mul(sub(mul(u.y,axis.z),mul(u.z,axis.y)),sine));
  up.y=sub(add(mul(u.y,cosine),mul(d,axis.y)),mul(sub(mul(u.z,axis.x),mul(u.x,axis.z)),sine));
  up.z=sub(add(mul(u.z,cosine),mul(d,axis.z)),mul(sub(mul(u.x,axis.y),mul(u.y,axis.x)),sine));
}
Vec3 transformed(const Transform&t,Vec3 p,bool point){
  if(t.scale!=1.f)p={mul(p.x,t.scale),mul(p.y,t.scale),mul(p.z,t.scale)};
  Vec3 v{add(add(mul(t.basis[2].x,p.z),mul(t.basis[1].x,p.y)),mul(t.basis[0].x,p.x)),
      add(add(mul(t.basis[2].y,p.z),mul(t.basis[1].y,p.y)),mul(t.basis[0].y,p.x)),
      add(add(mul(t.basis[2].z,p.z),mul(t.basis[1].z,p.y)),mul(t.basis[0].z,p.x))};
  if(point)v={add(v.x,t.position.x),add(v.y,t.position.y),add(v.z,t.position.z)};return v;
}
}
void source_observer_register_adopt(SourceObserverState&s,DeathCameraPacket&p,const SourceObserverConstants&c){
  byte(s,0xb4,0);
  if(!byte(s,0xb5)){byte(s,0xb5,1);pf(p,0x88,0);p[0]|=8u;for(unsigned i=0;i<6;++i)pf(p,0x94+i*4,0);}
  if(!(p[0]&1u))return;
  for(unsigned group=0;group<6;++group){const auto remaining=f(s,0x9c+group*4);const auto flags=pb(p,0x8c+group);
    if(!(flags&1u)){if(!(pf(p,0x88)<remaining)||p[0]&8u)pf(p,0x94+group*4,pf(p,0x88));
      else pf(p,0x94+group*4,std::min(c.scalar.two,remaining));}
    else if(!(flags&2u)&&pf(p,0x94+group*4)<remaining)pf(p,0x94+group*4,std::min(c.scalar.two,remaining));
  }
  const auto previous=byte(s,0xb6);const auto next=(p[0]>>6)&1u;
  if((previous||next)&&(previous!=next||s[0x8c/4]!=p[0x84/4])){
    Transform change;
    if(next){Transform parent;std::memcpy(&parent,p.data()+0x50/4,sizeof(parent));change=inverse_transform(parent);}
    if(previous){Transform old;std::memcpy(&old,s.data()+0x14c/4,sizeof(old));change=compose_transform(change,old);}
    vec(s,0x10c,transformed(change,vec(s,0x10c),true));
    for(const auto at:{0x134u,0x140u,0x180u,0x1a8u,0x1b4u,0x1dcu,0x320u,0x348u})
      vec(s,at,transformed(change,vec(s,at),false));
  }
  byte(s,0xb6,next);
  if(next)std::copy_n(p.begin()+0x50/4,13,s.begin()+0x14c/4);
  std::copy(p.begin(),p.end(),s.begin()+2);
}
void source_observer_motion(SourceObserverState&s,float dt,const SourceObserverConstants&c){
  const auto&k=c.scalar;if(s[2]&0x20u)return;
  for(unsigned n=0;n<10;++n)f(s,0x320+n*4,sub(f(s,0xc+n*4),f(s,0x10c+n*4)));
  vec(s,0x348,rotation_delta(vec(s,0x134),vec(s,0x140),vec(s,0x34),vec(s,0x40),c));
  unsigned index=0;
  //171990 calculates all coefficients before any duration cancellation.
  for(unsigned group=0;group<6;++group){const auto time=f(s,0x9c+group*4);
    if((s[2]&1u)&&time>dt){const auto r=div(k.one,time),r2=mul(r,r),r3=mul(r2,r),r4=mul(r3,r),r5=mul(r4,r);
      for(unsigned n=0;n<unsigned(c.motion_width[group]);++n){const auto at=(index+n)*4;
        const auto delta=f(s,0x320+at),velocity=f(s,0x180+at),acceleration=f(s,0x1b4+at);
        auto a=sub(mul(mul(acceleration,r3),k.half),add(mul(mul(delta,r5),k.six),mul(mul(velocity,r4),k.three)));
        auto b=sub(add(mul(mul(delta,r4),k.fifteen),mul(mul(velocity,r3),k.seven)),mul(acceleration,r2));
        auto d=sub(mul(mul(acceleration,r),k.half),add(mul(mul(delta,r3),k.ten),mul(mul(velocity,r2),k.four)));
        f(s,0x284+at,0);f(s,0x2b8+at,0);f(s,0x2ec+at,delta);
        if(group==0&&!byte(s,0xb6)){const auto speed=f(s,0x4c+n*4);
          a=sub(a,mul(mul(speed,r4),k.three));b=add(mul(mul(speed,r3),c.velocity_cubic),b);
          d=sub(d,mul(mul(speed,r2),k.six));f(s,0x2b8+at,add(f(s,0x2b8+at),speed));}
        f(s,0x1e8+at,a);f(s,0x21c+at,b);f(s,0x250+at,d);
      }
    }index+=c.motion_width[group];
  }
  index=0;
  for(unsigned group=0;group<6;++group){const auto remaining_at=0x9c+group*4;const auto time=sub(f(s,remaining_at),dt);
    for(unsigned n=0;n<unsigned(c.motion_width[group]);++n){const auto at=(index+n)*4;
      if(time<=0.f)f(s,0x1b4+at,0);
      else{const auto t2=mul(time,time),t3=mul(t2,time);
        const auto acceleration=add(add(add(mul(mul(f(s,0x1e8+at),t3),k.twenty),mul(mul(f(s,0x21c+at),t2),k.twelve)),
            mul(mul(f(s,0x250+at),time),k.six)),mul(f(s,0x284+at),k.two));f(s,0x1b4+at,acceleration);
        if(acceleration>c.acceleration_caps[group]||acceleration<sub(0.f,c.acceleration_caps[group])){
          const auto old=f(s,remaining_at);for(unsigned other=0;other<6;++other)
            if(other!=group&&f(s,0x9c+other*4)==old)f(s,0x9c+other*4,0);f(s,remaining_at,0);}
      }
    }index+=c.motion_width[group];
  }
  const float reciprocal=float(k.reciprocal_numerator/double(dt));index=0;
  for(unsigned group=0;group<6;++group){const auto time=sub(f(s,0x9c+group*4),dt);
    for(unsigned n=0;n<unsigned(c.motion_width[group]);++n){const auto at=(index+n)*4;
      if(time>0.f){const auto t2=mul(time,time),t3=mul(t2,time),t4=mul(t3,time);
        f(s,0x180+at,add(add(add(add(mul(mul(f(s,0x1e8+at),t4),k.five),mul(mul(f(s,0x21c+at),t3),k.four)),
            mul(mul(f(s,0x250+at),t2),k.three)),mul(mul(f(s,0x284+at),time),k.two)),f(s,0x2b8+at)));
      }else if(s[2]&1u){if((byte(s,0x94+group)&2u)||(s[2]&8u))f(s,0x180+at,0);
        else f(s,0x180+at,sub(0.f,mul(f(s,0x320+at),reciprocal)));}
    }index+=c.motion_width[group];
  }
  index=0;unsigned packet_index=0;
  for(unsigned group=0;group<6;++group){const auto time=sub(f(s,0x9c+group*4),dt);
    if(time<=0.f&&(s[2]&1u))for(unsigned n=0;n<unsigned(c.packet_width[group]);++n)s[(0x10c+packet_index*4)/4+n]=s[(0xc+packet_index*4)/4+n];
    else{std::array<float,3>delta{};
      for(unsigned n=0;n<unsigned(c.motion_width[group]);++n){const auto at=(index+n)*4;
        if(time>0.f){const auto t2=mul(time,time),t3=mul(t2,time),t4=mul(t3,time),t5=mul(t4,time);
          delta[n]=add(add(add(add(add(mul(f(s,0x1e8+at),t5),mul(f(s,0x21c+at),t4)),mul(f(s,0x250+at),t3)),
              mul(f(s,0x284+at),t2)),mul(f(s,0x2b8+at),time)),f(s,0x2ec+at));}
        else delta[n]=sub(0.f,mul(f(s,0x180+at),dt));
        if(group<5)f(s,0x10c+(packet_index+n)*4,add(delta[n],f(s,0x10c+(packet_index+n)*4)));
      }
      if(group==5){auto forward=vec(s,0x134),up=vec(s,0x140);rotate({delta[0],delta[1],delta[2]},forward,up,c);vec(s,0x134,forward);vec(s,0x140,up);}
    }index+=c.motion_width[group];packet_index+=c.packet_width[group];
  }
  auto forward=vec(s,0x134),up=vec(s,0x140);
  const auto squared=[](Vec3 v){return add(add(mul(v.x,v.x),mul(v.y,v.y)),mul(v.z,v.z));};
  const auto dot=add(add(mul(up.z,forward.z),mul(forward.x,up.x)),mul(up.y,forward.y));
  if(!(std::abs(sub(squared(forward),k.one))<c.orthogonal_tolerance&&
      std::abs(sub(squared(up),k.one))<c.orthogonal_tolerance&&std::abs(dot)<c.orthogonal_tolerance)){
    up=cross_source(forward,cross_source(up,forward));normalize(forward,c);normalize(up,c);vec(s,0x134,forward);vec(s,0x140,up);}
  for(unsigned group=0;group<6;++group){const auto time=sub(f(s,0x9c+group*4),dt);f(s,0x9c+group*4,time>0.f?time:0.f);}
}
} // namespace halo2
