// Generated complete modified Complex4627 interrupt bodies; no runtime decoder.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_tu15_interrupt.hpp"
namespace halo2 { namespace {
using namespace physics_world_runtime_detail;
struct InterruptState:WorldState {
 const SourceTu15TimerDataOwner &data;
 const SourceTu15InterruptPortOwner &ports;
 InterruptState(M &m,const O &o,const SourcePhysicsContactRuntimeServices &v,
  const SourceTu15TimerDataOwner &d,const SourceTu15InterruptPortOwner &p)
  :WorldState(m,o,v,SourcePhysicsContactImage::TitleUpdate15),data(d),ports(p) {}
};
std::uint32_t owned_read(InterruptState &s,std::uint32_t pc,std::uint32_t p,unsigned n) {
 if(!s.data)throw SourceTu15TimerDataUnavailable({pc,p,n});
 s.data(s.memory,{pc,p,n});return r(s.memory,p,n);
}
void run_80018ba0(InterruptState &);
void run_80014368(InterruptState &);
void run_80018ba0(InterruptState &s) {
L80018ba0: if(s.services.instruction)s.services.instruction(0x80018ba0u);s.flags.minus(owned_read(s,0x80018ba0u,(0x80035c08u),4),0x0u,4);
L80018ba7: if(s.services.instruction)s.services.instruction(0x80018ba7u);if(!s.flags.zf)goto L80018bc5;
L80018ba9: if(s.services.instruction)s.services.instruction(0x80018ba9u);run_80014368(s);return;
L80018bae: if(s.services.instruction)s.services.instruction(0x80018baeu);s.flags.minus(owned_read(s,0x80018baeu,(0x80035c34u),4),0x0u,4);
L80018bb5: if(s.services.instruction)s.services.instruction(0x80018bb5u);if(!s.flags.zf)goto L80018ba9;
L80018bb7: if(s.services.instruction)s.services.instruction(0x80018bb7u);s.push(0x80018bb7u,s.reg(1,4));
L80018bb8: if(s.services.instruction)s.services.instruction(0x80018bb8u);s.set(1,0x2u,1);
L80018bba: if(s.services.instruction)s.services.instruction(0x80018bbau);s.push(0x80018bbau,0x80018bbfu);s.required(0x80018bbau,0x80014128u,false);
L80018bbf: if(s.services.instruction)s.services.instruction(0x80018bbfu);s.set(1,s.pop(),4);
L80018bc0: if(s.services.instruction)s.services.instruction(0x80018bc0u);run_80014368(s);return;
L80018bc5: if(s.services.instruction)s.services.instruction(0x80018bc5u);s.flags.minus(s.reg(1,1),0x2u,1);
L80018bc8: if(s.services.instruction)s.services.instruction(0x80018bc8u);if(s.flags.sf==s.flags.of)goto L80018bae;
L80018bca: if(s.services.instruction)s.services.instruction(0x80018bcau);s.set(4,s.flags.minus(s.reg(4,4),0x10u,4),4);
L80018bcd: if(s.services.instruction)s.services.instruction(0x80018bcdu);w(s.memory,s.observe,0x80018bcdu,(0xcu+s.gp[4]*1u),4,s.reg(3,4));
L80018bd1: if(s.services.instruction)s.services.instruction(0x80018bd1u);w(s.memory,s.observe,0x80018bd1u,(0x8u+s.gp[4]*1u),4,s.reg(6,4));
L80018bd5: if(s.services.instruction)s.services.instruction(0x80018bd5u);w(s.memory,s.observe,0x80018bd5u,(0x4u+s.gp[4]*1u),4,s.reg(7,4));
L80018bd9: if(s.services.instruction)s.services.instruction(0x80018bd9u);w(s.memory,s.observe,0x80018bd9u,(0x0u+s.gp[4]*1u),4,s.reg(5,4));
L80018bdc: if(s.services.instruction)s.services.instruction(0x80018bdcu);s.set(3,(0x80035bdcu),4);
L80018be2: if(s.services.instruction)s.services.instruction(0x80018be2u);s.set(6,r(s.memory,(0x2cu+s.gp[3]*1u),4),4);
L80018be5: if(s.services.instruction)s.services.instruction(0x80018be5u);s.set(7,r(s.memory,(0x28u+s.gp[3]*1u),4),4);
L80018be8: if(s.services.instruction)s.services.instruction(0x80018be8u);w(s.memory,s.observe,0x80018be8u,(0x2cu+s.gp[3]*1u),4,0x0u);
L80018bef: if(s.services.instruction)s.services.instruction(0x80018befu);w(s.memory,s.observe,0x80018befu,(0x28u+s.gp[3]*1u),4,s.reg(6,4));
L80018bf2: if(s.services.instruction)s.services.instruction(0x80018bf2u);w(s.memory,s.observe,0x80018bf2u,(0x54u+s.gp[7]*1u),1,s.reg(1,1));
L80018bf5: if(s.services.instruction)s.services.instruction(0x80018bf5u);s.set(1,s.reg(7,4),4);
L80018bf7: if(s.services.instruction)s.services.instruction(0x80018bf7u);s.push(0x80018bf7u,0x80018bfcu);s.required(0x80018bf7u,0x8001bd68u,false);
L80018bfc: if(s.services.instruction)s.services.instruction(0x80018bfcu);s.set(1,r(s.memory,(0x54u+s.gp[7]*1u),1),1);
L80018bff: if(s.services.instruction)s.services.instruction(0x80018bffu);s.push(0x80018bffu,0x80018c04u);s.required(0x80018bffu,0x80018dc0u,false);
L80018c04: if(s.services.instruction)s.services.instruction(0x80018c04u);s.set(0,s.flags.logic((s.reg(0,1))|(s.reg(0,1)),1),1);
L80018c06: if(s.services.instruction)s.services.instruction(0x80018c06u);s.set(1,r(s.memory,(0x54u+s.gp[6]*1u),1),1);
L80018c09: if(s.services.instruction)s.services.instruction(0x80018c09u);if(!s.flags.zf)goto L80018c22;
L80018c0b: if(s.services.instruction)s.services.instruction(0x80018c0bu);s.set(5,r(s.memory,(0x0u+s.gp[4]*1u),4),4);
L80018c0e: if(s.services.instruction)s.services.instruction(0x80018c0eu);s.set(7,r(s.memory,(0x4u+s.gp[4]*1u),4),4);
L80018c12: if(s.services.instruction)s.services.instruction(0x80018c12u);s.set(6,r(s.memory,(0x8u+s.gp[4]*1u),4),4);
L80018c16: if(s.services.instruction)s.services.instruction(0x80018c16u);s.set(3,r(s.memory,(0xcu+s.gp[4]*1u),4),4);
L80018c1a: if(s.services.instruction)s.services.instruction(0x80018c1au);s.set(4,s.flags.plus(s.reg(4,4),0x10u,4),4);
L80018c1d: if(s.services.instruction)s.services.instruction(0x80018c1du);run_80014368(s);return;
L80018c22: if(s.services.instruction)s.services.instruction(0x80018c22u);s.set(1,0x1u,1);
L80018c24: if(s.services.instruction)s.services.instruction(0x80018c24u);s.push(0x80018c24u,0x80018c29u);run_80014368(s);
L80018c29: if(s.services.instruction)s.services.instruction(0x80018c29u);s.set(0,s.flags.logic((s.reg(0,4))^(s.reg(0,4)),4),4);
L80018c2b: if(s.services.instruction)s.services.instruction(0x80018c2bu);s.push(0x80018c2bu,0x80018c30u);s.required(0x80018c2bu,0x8001bb3du,false);
L80018c30: if(s.services.instruction)s.services.instruction(0x80018c30u);s.set(1,s.flags.logic((s.reg(1,4))^(s.reg(1,4)),4),4);
L80018c32: if(s.services.instruction)s.services.instruction(0x80018c32u);goto L80018c0b;
}
void run_80014368(InterruptState &s) {
L80014368: if(s.services.instruction)s.services.instruction(0x80014368u);s.set(1,s.flags.logic((s.reg(1,4))&(0xffu),4),4);
L8001436e: if(s.services.instruction)s.services.instruction(0x8001436eu);s.push(0x8001436eu,s.other_flags|s.flags.bits());
L8001436f: if(s.services.instruction)s.services.instruction(0x8001436fu);s.other_flags&=~0x200u;
L80014370: if(s.services.instruction)s.services.instruction(0x80014370u);w(s.memory,s.observe,0x80014370u,(0x80035c00u),4,s.reg(1,4));
L80014376: if(s.services.instruction)s.services.instruction(0x80014376u);s.set(2,owned_read(s,0x80014376u,(0x80035a1cu),4),4);
L8001437c: if(s.services.instruction)s.services.instruction(0x8001437cu);s.set(2,s.flags.logic((s.reg(2,4))&(owned_read(s,0x8001437cu,(0x800142a4u+s.gp[1]*4u),4)),4),4);
L80014383: if(s.services.instruction)s.services.instruction(0x80014383u);if(!s.flags.zf)goto L80014388;
L80014385: if(s.services.instruction)s.services.instruction(0x80014385u);{const auto f=s.pop();s.other_flags=f&~0x8d5u;s.flags.load(f);}
L80014386: if(s.services.instruction)s.services.instruction(0x80014386u);s.pop();return;
L80014388: if(s.services.instruction)s.services.instruction(0x80014388u);{const auto x=s.reg(2,4);s.flags.zf=x==0;if(x)s.set(1,31u-std::countl_zero(x),4);}
L8001438b: if(s.services.instruction)s.services.instruction(0x8001438bu);s.flags.minus(s.reg(1,4),0x2u,4);
L8001438e: if(s.services.instruction)s.services.instruction(0x8001438eu);if(!s.flags.cf&&!s.flags.zf)goto L80014399;
L80014390: if(s.services.instruction)s.services.instruction(0x80014390u);s.push(0x80014390u,0x80014397u);s.required(0x80014390u,owned_read(s,0x80014390u,(0x80035938u+s.gp[1]*4u),4),false);
L80014397: if(s.services.instruction)s.services.instruction(0x80014397u);{const auto f=s.pop();s.other_flags=f&~0x8d5u;s.flags.load(f);}
L80014398: if(s.services.instruction)s.services.instruction(0x80014398u);s.pop();return;
L80014399: if(s.services.instruction)s.services.instruction(0x80014399u);s.set(0,owned_read(s,0x80014399u,(0x80035a24u),4),4);
L8001439e: if(s.services.instruction)s.services.instruction(0x8001439eu);if(!s.ports)throw std::runtime_error("Original PIC output requires its port owner");s.ports({0x8001439eu,0x21u,std::uint8_t(s.reg(0,1))});
L800143a0: if(s.services.instruction)s.services.instruction(0x800143a0u);s.set(0,s.flags.right(s.reg(0,4),0x8u,4,false),4);
L800143a3: if(s.services.instruction)s.services.instruction(0x800143a3u);if(!s.ports)throw std::runtime_error("Original PIC output requires its port owner");s.ports({0x800143a3u,0xa1u,std::uint8_t(s.reg(0,1))});
L800143a5: if(s.services.instruction)s.services.instruction(0x800143a5u);s.set(2,0x1u,4);
L800143aa: if(s.services.instruction)s.services.instruction(0x800143aau);s.set(2,s.flags.shift(s.reg(2,4),s.reg(1,1),4),4);
L800143ac: if(s.services.instruction)s.services.instruction(0x800143acu);w(s.memory,s.observe,0x800143acu,(0x80035a1cu),4,s.flags.logic((owned_read(s,0x800143acu,(0x80035a1cu),4))^(s.reg(2,4)),4));
L800143b2: if(s.services.instruction)s.services.instruction(0x800143b2u);s.push(0x800143b2u,0x800143b9u);s.required(0x800143b2u,owned_read(s,0x800143b2u,(0x80035938u+s.gp[1]*4u),4),false);
L800143b9: if(s.services.instruction)s.services.instruction(0x800143b9u);{const auto f=s.pop();s.other_flags=f&~0x8d5u;s.flags.load(f);}
L800143ba: if(s.services.instruction)s.services.instruction(0x800143bau);s.pop();return;
}
}
void source_run_tu15_interrupt_runtime(SourcePhysicsContactMemory &m,std::uint32_t entry,
 SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
 const SourcePhysicsContactRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe,
 const SourceTu15TimerDataOwner &data,const SourceTu15InterruptPortOwner &ports) {
 FpScope scope(fp);InterruptState s{m,observe,services,data,ports};s.load(abi);
 try {switch(entry) {
 case 0x80018ba0:run_80018ba0(s);break;
 case 0x80014368:run_80014368(s);break;
 default:throw std::runtime_error("Unverified original interrupt entry");}}
 catch(...) {abi=s.save();throw;}abi=s.save();
}
} // namespace halo2
