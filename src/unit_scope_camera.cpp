#include "halo2/unit_scope_camera.hpp"

namespace halo2 {
UnitScopeCameraState source_scope_camera_initial(const UnitScopeCameraConstants &c) {
  UnitScopeCameraState s;
  s.current=c.default_fov;
  s.target=c.default_fov;
  s.rendered_fov=c.default_fov;
  return s;
}

float source_scope_camera_fov(std::optional<float> authored,float magnification,
                            const UnitScopeCameraConstants &c) {
  if(!authored)return c.default_fov;
  if(magnification==c.one)return *authored;
  const float candidate=*authored/magnification;
  if(candidate>c.magnified_minimum && c.magnified_maximum>candidate)return candidate;
  return *authored;
}

void source_scope_camera_target(UnitScopeCameraProducer &producer,
    UnitScopeCameraPacket &packet,float fov,const UnitScopeCameraConstants &c) {
  float target=fov*c.packet_multiplier;
  if(c.packet_minimum>target)target=c.packet_minimum;
  else if(target>c.packet_maximum)target=c.packet_maximum;
  packet.fov=target;
  if(producer.persistent_fov!=target) {
    packet.fov_seconds=c.transition_seconds;
    packet.fov_flags=1;
    producer.persistent_fov=target;
  }
}

void source_scope_camera_register(UnitScopeCameraState &s,UnitScopeCameraPacket &p) {
  if(!s.registered) {
    s.registered=true;
    p.global_seconds=0.0f;
    p.flags|=8;
    p.fov_seconds=0.0f;
  }
  s.registered_flags=p.flags;
}

bool source_scope_camera_adopt(UnitScopeCameraState &s,UnitScopeCameraPacket &p,
                              const UnitScopeCameraConstants &c) {
  if(!(p.flags&1u))return false;
  if(p.fov_flags&1u) {
    if(!(p.fov_flags&2u) && s.remaining>p.fov_seconds)
      p.fov_seconds=s.remaining>c.two?c.two:s.remaining;
  }else {
    if(s.remaining>p.global_seconds && !(p.flags&8u))
      p.fov_seconds=s.remaining>c.two?c.two:s.remaining;
    else p.fov_seconds=p.global_seconds;
  }
  s.flags=p.flags;
  s.fov_flags=p.fov_flags;
  s.target=p.fov;
  s.remaining=p.fov_seconds;
  return true;
}

float source_scope_camera_dt(float frame_seconds,const UnitScopeCameraConstants &c) {
  return frame_seconds*c.frame_dt_multiplier;
}

bool source_scope_camera_motion(UnitScopeCameraState &s,float dt,
                               const UnitScopeCameraConstants &c) {
  if(s.registered_flags&0x20u)return false;
  s.delta=s.target-s.current;
  auto &a=s.coefficients;
  if((s.flags&1u) && s.remaining>dt) {
    const float r=c.one/s.remaining;
    const float r2=r*r;
    const float r3=r2*r;
    const float r4=r3*r;
    const float r5=r4*r;
    float x=s.delta*r5;
    x=x*c.six;
    float y=s.velocity*r4;
    y=y*c.three;
    x=x+y;
    y=s.acceleration*r3;
    y=y*c.half;
    a[0]=y-x;
    x=s.delta*r4;
    x=x*c.fifteen;
    y=s.velocity*r3;
    y=y*c.seven;
    x=x+y;
    y=s.acceleration*r2;
    a[1]=x-y;
    x=s.acceleration*r;
    x=x*c.half;
    y=s.delta*r3;
    y=y*c.ten;
    float z=s.velocity*r2;
    z=z*c.four;
    y=y+z;
    a[2]=x-y;
    a[3]=0.0f;
    a[4]=0.0f;
    a[5]=s.delta;
  }
  bool cancelled=false;
  float t=s.remaining-dt;
  if(t>0.0f) {
    const float t2=t*t;
    const float t3=t2*t;
    float x=a[0]*t3;
    x=x*c.twenty;
    float y=a[1]*t2;
    y=y*c.twelve;
    x=x+y;
    y=a[2]*t;
    y=y*c.six;
    x=x+y;
    y=a[3]*c.two;
    s.acceleration=x+y;
    if(s.acceleration>c.acceleration_cap || (0.0f-c.acceleration_cap)>s.acceleration) {
      s.remaining=0.0f;
      cancelled=true;
    }
  }else s.acceleration=0.0f;
  t=s.remaining-dt;
  if(t>0.0f) {
    const float t2=t*t;
    const float t3=t2*t;
    const float t4=t3*t;
    float x=a[0]*t4;
    x=x*c.five;
    float y=a[1]*t3;
    y=y*c.four;
    x=x+y;
    y=a[2]*t2;
    y=y*c.three;
    x=x+y;
    y=a[3]*t;
    y=y*c.two;
    x=x+y;
    s.velocity=x+a[4];
  }else if(s.flags&1u) {
    if((s.fov_flags&2u) || (s.flags&8u))s.velocity=0.0f;
    else {
      // Original x87 FDIVR of the immutable double, stored to a float before
      // SSE multiplication. Subtract AFTER multiplying, preserving rounding.
      const float reciprocal=static_cast<float>(c.reciprocal_numerator/static_cast<double>(dt));
      const float product=s.delta*reciprocal;
      s.velocity=0.0f-product;
    }
  }
  if(!(t>0.0f) && (s.flags&1u))s.current=s.target;
  else {
    float displacement;
    if(t>0.0f) {
      const float t2=t*t;
      const float t3=t2*t;
      const float t4=t3*t;
      const float t5=t4*t;
      float x=a[0]*t5;
      float y=a[1]*t4;
      x=x+y;
      y=a[2]*t3;
      x=x+y;
      y=a[3]*t2;
      x=x+y;
      y=a[4]*t;
      x=x+y;
      displacement=x+a[5];
    }else {
      const float product=s.velocity*dt;
      displacement=0.0f-product;
    }
    s.current=displacement+s.current;
  }
  t=s.remaining-dt;
  s.remaining=t>0.0f?t:0.0f;
  return cancelled;
}

float source_scope_camera_render_fov(UnitScopeCameraState &s,
                                    const UnitScopeCameraConstants &c) {
  float fov=c.render_minimum;
  if(!(fov>s.current)) {
    fov=c.render_maximum;
    if(!(s.current>fov))fov=s.current;
  }
  s.current=fov;
  s.rendered_fov=fov;
  return fov;
}
} // namespace halo2
