// Complete owned modified Complex4627 kernel301, statically translated.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_tu15_status.hpp"
namespace halo2 { namespace {
using namespace physics_world_runtime_detail;
void run_status(WorldState &s) {
L80023653: if(s.services.instruction)s.services.instruction(0x80023653u);s.set(0,r(s.memory,(0x4u+s.gp[4]*1u),4),4);
L80023657: if(s.services.instruction)s.services.instruction(0x80023657u);s.flags.logic((s.reg(0,4))&(0x20000000u),4);
L8002365c: if(s.services.instruction)s.services.instruction(0x8002365cu);if(!s.flags.zf)goto L80023720;
L80023662: if(s.services.instruction)s.services.instruction(0x80023662u);s.push(0x80023662u,s.reg(3,4));
L80023663: if(s.services.instruction)s.services.instruction(0x80023663u);s.push(0x80023663u,s.reg(6,4));
L80023664: if(s.services.instruction)s.services.instruction(0x80023664u);s.push(0x80023664u,s.reg(7,4));
L80023665: if(s.services.instruction)s.services.instruction(0x80023665u);s.set(1,s.reg(0,4),4);
L80023667: if(s.services.instruction)s.services.instruction(0x80023667u);s.set(7,0xffff0000u,4);
L8002366c: if(s.services.instruction)s.services.instruction(0x8002366cu);s.set(1,s.flags.logic((s.reg(1,4))&(s.reg(7,4)),4),4);
L8002366e: if(s.services.instruction)s.services.instruction(0x8002366eu);s.flags.minus(s.reg(1,4),0x80070000u,4);
L80023674: if(s.services.instruction)s.services.instruction(0x80023674u);if(!s.flags.zf)goto L80023680;
L80023676: if(s.services.instruction)s.services.instruction(0x80023676u);s.set(0,s.flags.logic((s.reg(0,4))&(0xffffu),4),4);
L8002367b: if(s.services.instruction)s.services.instruction(0x8002367bu);goto L8002371d;
L80023680: if(s.services.instruction)s.services.instruction(0x80023680u);s.set(1,s.reg(0,4),4);
L80023682: if(s.services.instruction)s.services.instruction(0x80023682u);s.set(1,s.flags.logic((s.reg(1,4))&(0xf0000000u),4),4);
L80023688: if(s.services.instruction)s.services.instruction(0x80023688u);s.flags.minus(s.reg(1,4),0xd0000000u,4);
L8002368e: if(s.services.instruction)s.services.instruction(0x8002368eu);if(!s.flags.zf)goto L80023695;
L80023690: if(s.services.instruction)s.services.instruction(0x80023690u);s.set(0,s.flags.logic((s.reg(0,4))&(0xcfffffffu),4),4);
L80023695: if(s.services.instruction)s.services.instruction(0x80023695u);s.set(1,s.flags.logic((s.reg(1,4))^(s.reg(1,4)),4),4);
L80023697: if(s.services.instruction)s.services.instruction(0x80023697u);s.set(6,s.flags.logic((s.reg(6,4))^(s.reg(6,4)),4),4);
L80023699: if(s.services.instruction)s.services.instruction(0x80023699u);s.flags.minus(s.reg(0,4),r(s.memory,(0x80011f90u+s.gp[1]*8u),4),4);
L800236a0: if(s.services.instruction)s.services.instruction(0x800236a0u);if(s.flags.cf)goto L800236bf;
L800236a2: if(s.services.instruction)s.services.instruction(0x800236a2u);s.set(2,r(s.memory,(0x80011f8eu+s.gp[1]*8u),2),4);
L800236aa: if(s.services.instruction)s.services.instruction(0x800236aau);s.set(3,r(s.memory,(0x80011f8cu+s.gp[1]*8u),2),4);
L800236b2: if(s.services.instruction)s.services.instruction(0x800236b2u);s.set(2,s.flags.product(s.reg(2,4),s.reg(3,4)),4);
L800236b5: if(s.services.instruction)s.services.instruction(0x800236b5u);s.set(6,s.flags.plus(s.reg(6,4),s.reg(2,4),4),4);
L800236b7: if(s.services.instruction)s.services.instruction(0x800236b7u);s.set(1,s.flags.increment(s.reg(1,4),4,false),4);
L800236b8: if(s.services.instruction)s.services.instruction(0x800236b8u);s.flags.minus(s.reg(1,4),0x67u,4);
L800236bb: if(s.services.instruction)s.services.instruction(0x800236bbu);if(s.flags.cf)goto L80023699;
L800236bd: if(s.services.instruction)s.services.instruction(0x800236bdu);goto L800236d5;
L800236bf: if(s.services.instruction)s.services.instruction(0x800236bfu);s.set(1,s.flags.shift(s.reg(1,4),0x3u,4),4);
L800236c2: if(s.services.instruction)s.services.instruction(0x800236c2u);s.set(3,r(s.memory,(0x80011f8cu+s.gp[1]*1u),2),4);
L800236c9: if(s.services.instruction)s.services.instruction(0x800236c9u);s.set(2,s.reg(0,4),4);
L800236cb: if(s.services.instruction)s.services.instruction(0x800236cbu);s.set(2,s.flags.minus(s.reg(2,4),r(s.memory,(0x80011f88u+s.gp[1]*1u),4),4),4);
L800236d1: if(s.services.instruction)s.services.instruction(0x800236d1u);s.flags.minus(s.reg(2,4),s.reg(3,4),4);
L800236d3: if(s.services.instruction)s.services.instruction(0x800236d3u);if(s.flags.cf)goto L800236e8;
L800236d5: if(s.services.instruction)s.services.instruction(0x800236d5u);s.set(1,s.reg(0,4),4);
L800236d7: if(s.services.instruction)s.services.instruction(0x800236d7u);s.set(1,s.flags.logic((s.reg(1,4))&(s.reg(7,4)),4),4);
L800236d9: if(s.services.instruction)s.services.instruction(0x800236d9u);s.flags.minus(s.reg(1,4),0xc0010000u,4);
L800236df: if(s.services.instruction)s.services.instruction(0x800236dfu);if(s.flags.zf)goto L80023676;
L800236e1: if(s.services.instruction)s.services.instruction(0x800236e1u);s.set(0,0x13du,4);
L800236e6: if(s.services.instruction)s.services.instruction(0x800236e6u);goto L8002371d;
L800236e8: if(s.services.instruction)s.services.instruction(0x800236e8u);s.set(1,r(s.memory,(0x80011f8eu+s.gp[1]*1u),2),2);
L800236ef: if(s.services.instruction)s.services.instruction(0x800236efu);s.set(0,s.reg(1,2),4);
L800236f2: if(s.services.instruction)s.services.instruction(0x800236f2u);s.set(0,s.flags.product(s.reg(0,4),s.reg(2,4)),4);
L800236f5: if(s.services.instruction)s.services.instruction(0x800236f5u);s.set(6,s.flags.plus(s.reg(6,4),s.reg(0,4),4),4);
L800236f7: if(s.services.instruction)s.services.instruction(0x800236f7u);s.flags.minus(s.reg(1,2),0x1u,2);
L800236fb: if(s.services.instruction)s.services.instruction(0x800236fbu);if(!s.flags.zf)goto L80023707;
L800236fd: if(s.services.instruction)s.services.instruction(0x800236fdu);s.set(0,r(s.memory,(0x80011988u+s.gp[6]*2u),2),4);
L80023705: if(s.services.instruction)s.services.instruction(0x80023705u);goto L8002371d;
L80023707: if(s.services.instruction)s.services.instruction(0x80023707u);s.set(1,(0x0u+s.gp[6]*1u+s.gp[6]*1u),4);
L8002370a: if(s.services.instruction)s.services.instruction(0x8002370au);s.set(0,r(s.memory,(0x8001198au+s.gp[1]*1u),2),4);
L80023711: if(s.services.instruction)s.services.instruction(0x80023711u);s.set(1,r(s.memory,(0x80011988u+s.gp[1]*1u),2),4);
L80023718: if(s.services.instruction)s.services.instruction(0x80023718u);s.set(0,s.flags.shift(s.reg(0,4),0x10u,4),4);
L8002371b: if(s.services.instruction)s.services.instruction(0x8002371bu);s.set(0,s.flags.logic((s.reg(0,4))|(s.reg(1,4)),4),4);
L8002371d: if(s.services.instruction)s.services.instruction(0x8002371du);s.set(7,s.pop(),4);
L8002371e: if(s.services.instruction)s.services.instruction(0x8002371eu);s.set(6,s.pop(),4);
L8002371f: if(s.services.instruction)s.services.instruction(0x8002371fu);s.set(3,s.pop(),4);
L80023720: if(s.services.instruction)s.services.instruction(0x80023720u);s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("Original owned kernel301 left its complete CFG");
}
}
void source_run_tu15_status_runtime(SourcePhysicsContactMemory &m,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe) {
    FpScope scope(fp);WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {run_status(s);} catch(...) {abi=s.save();throw;}abi=s.save();
}
}
