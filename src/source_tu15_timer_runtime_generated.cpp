// Generated complete owned modified Complex4627 timer bodies; no runtime decoder.
#include "physics_world_runtime_internal.hpp"
#include "halo2/source_tu15_timer.hpp"
namespace halo2 { namespace {
using namespace physics_world_runtime_detail;
struct TimerState:WorldState {
 const SourceTu15TimerDataOwner &data_owner;
 TimerState(M &m,const O &o,const SourcePhysicsContactRuntimeServices &v,const SourceTu15TimerDataOwner &d)
  :WorldState(m,o,v,SourcePhysicsContactImage::TitleUpdate15),data_owner(d) {}
};
std::uint32_t kernel_read(TimerState &s,std::uint32_t pc,std::uint32_t p,unsigned n) {
 if(!s.data_owner)throw SourceTu15TimerDataUnavailable({pc,p,n});
 s.data_owner(s.memory,{pc,p,n});return r(s.memory,p,n);
}
std::uint32_t carry_add(TimerState &s,std::uint32_t a,std::uint32_t b,unsigned n) {
 const auto carry=unsigned(s.flags.cf),mask=Flags::mask(n),sign=1u<<(n*8-1);a&=mask;b&=mask;
 const auto full=std::uint64_t(a)+b+carry;const auto x=std::uint32_t(full)&mask;
 s.flags.cf=full>mask;s.flags.of=(~(a^b)&(a^x)&sign)!=0;s.flags.af=((a^b^x)&16u)!=0;s.flags.result(x,n);return x;
}
std::uint32_t double_right(TimerState &s,std::uint32_t a,std::uint32_t b,unsigned count) {
 count&=31;if(!count)return a;
 const auto x=(a>>count)|(b<<(32-count));s.flags.cf=((a>>(count-1))&1)!=0;
 if(count==1)s.flags.of=((a^x)>>31)!=0;s.flags.result(x,4);return x;
}
void run_800195d6(TimerState &);
void run_8001951c(TimerState &);
void run_80014348(TimerState &);
void run_8001cd34(TimerState &);
void run_8001a384(TimerState &);
void run_800194c0(TimerState &);
void run_8001c4cf(TimerState &);
void run_8001c3cf(TimerState &);
void run_8001ccac(TimerState &);
void run_800195d6(TimerState &s) {
L800195d6: if(s.services.instruction)s.services.instruction(0x800195d6u);s.push(0x800195d6u,r(s.memory,(0x10u+s.gp[4]*1u),4));
L800195da: if(s.services.instruction)s.services.instruction(0x800195dau);s.push(0x800195dau,0x0u);
L800195dc: if(s.services.instruction)s.services.instruction(0x800195dcu);s.push(0x800195dcu,r(s.memory,(0x14u+s.gp[4]*1u),4));
L800195e0: if(s.services.instruction)s.services.instruction(0x800195e0u);s.push(0x800195e0u,r(s.memory,(0x14u+s.gp[4]*1u),4));
L800195e4: if(s.services.instruction)s.services.instruction(0x800195e4u);s.push(0x800195e4u,r(s.memory,(0x14u+s.gp[4]*1u),4));
L800195e8: if(s.services.instruction)s.services.instruction(0x800195e8u);s.push(0x800195e8u,0x800195edu);run_8001951c(s);
L800195ed: if(s.services.instruction)s.services.instruction(0x800195edu);s.pop();s.gp[4]+=16u;return;
}
void run_8001951c(TimerState &s) {
L8001951c: if(s.services.instruction)s.services.instruction(0x8001951cu);s.push(0x8001951cu,s.reg(5,4));
L8001951d: if(s.services.instruction)s.services.instruction(0x8001951du);s.set(5,s.reg(4,4),4);
L8001951f: if(s.services.instruction)s.services.instruction(0x8001951fu);s.push(0x8001951fu,s.reg(1,4));
L80019520: if(s.services.instruction)s.services.instruction(0x80019520u);s.push(0x80019520u,s.reg(3,4));
L80019521: if(s.services.instruction)s.services.instruction(0x80019521u);s.push(0x80019521u,s.reg(6,4));
L80019522: if(s.services.instruction)s.services.instruction(0x80019522u);s.push(0x80019522u,s.reg(7,4));
L80019523: if(s.services.instruction)s.services.instruction(0x80019523u);s.push(0x80019523u,0x80019528u);run_80014348(s);
L80019528: if(s.services.instruction)s.services.instruction(0x80019528u);s.set(6,r(s.memory,(0x8u+s.gp[5]*1u),4),4);
L8001952b: if(s.services.instruction)s.services.instruction(0x8001952bu);w(s.memory,s.observe,0x8001952bu,(0xffffffffu+s.gp[5]*1u),1,s.reg(0,1));
L8001952e: if(s.services.instruction)s.services.instruction(0x8001952eu);s.set(0,r(s.memory,(0x3u+s.gp[6]*1u),1),1);
L80019531: if(s.services.instruction)s.services.instruction(0x80019531u);s.flags.logic((s.reg(0,1))&(s.reg(0,1)),1);
L80019533: if(s.services.instruction)s.services.instruction(0x80019533u);w(s.memory,s.observe,0x80019533u,(0xbu+s.gp[5]*1u),1,s.reg(0,1));
L80019536: if(s.services.instruction)s.services.instruction(0x80019536u);if(s.flags.zf)goto L80019547;
L80019538: if(s.services.instruction)s.services.instruction(0x80019538u);s.set(0,r(s.memory,(0x18u+s.gp[6]*1u),4),4);
L8001953b: if(s.services.instruction)s.services.instruction(0x8001953bu);s.set(1,r(s.memory,(0x1cu+s.gp[6]*1u),4),4);
L8001953e: if(s.services.instruction)s.services.instruction(0x8001953eu);w(s.memory,s.observe,0x8001953eu,(0x3u+s.gp[6]*1u),1,s.flags.logic((r(s.memory,(0x3u+s.gp[6]*1u),1))&(0x0u),1));
L80019542: if(s.services.instruction)s.services.instruction(0x80019542u);w(s.memory,s.observe,0x80019542u,(0x0u+s.gp[1]*1u),4,s.reg(0,4));
L80019544: if(s.services.instruction)s.services.instruction(0x80019544u);w(s.memory,s.observe,0x80019544u,(0x4u+s.gp[0]*1u),4,s.reg(1,4));
L80019547: if(s.services.instruction)s.services.instruction(0x80019547u);s.push(0x80019547u,r(s.memory,(0x10u+s.gp[5]*1u),4));
L8001954a: if(s.services.instruction)s.services.instruction(0x8001954au);s.set(7,r(s.memory,(0x18u+s.gp[5]*1u),4),4);
L8001954d: if(s.services.instruction)s.services.instruction(0x8001954du);s.set(3,r(s.memory,(0x14u+s.gp[5]*1u),4),4);
L80019550: if(s.services.instruction)s.services.instruction(0x80019550u);s.push(0x80019550u,r(s.memory,(0xcu+s.gp[5]*1u),4));
L80019553: if(s.services.instruction)s.services.instruction(0x80019553u);w(s.memory,s.observe,0x80019553u,(0x4u+s.gp[6]*1u),4,s.flags.logic((r(s.memory,(0x4u+s.gp[6]*1u),4))&(0x0u),4));
L80019557: if(s.services.instruction)s.services.instruction(0x80019557u);s.set(1,s.reg(6,4),4);
L80019559: if(s.services.instruction)s.services.instruction(0x80019559u);w(s.memory,s.observe,0x80019559u,(0x20u+s.gp[6]*1u),4,s.reg(7,4));
L8001955c: if(s.services.instruction)s.services.instruction(0x8001955cu);w(s.memory,s.observe,0x8001955cu,(0x24u+s.gp[6]*1u),4,s.reg(3,4));
L8001955f: if(s.services.instruction)s.services.instruction(0x8001955fu);s.push(0x8001955fu,0x80019564u);run_8001c4cf(s);
L80019564: if(s.services.instruction)s.services.instruction(0x80019564u);s.flags.logic((s.reg(0,4))&(s.reg(0,4)),4);
L80019566: if(s.services.instruction)s.services.instruction(0x80019566u);if(!s.flags.zf)goto L800195c4;
L80019568: if(s.services.instruction)s.services.instruction(0x80019568u);s.set(0,(0x8u+s.gp[6]*1u),4);
L8001956b: if(s.services.instruction)s.services.instruction(0x8001956bu);s.flags.minus(r(s.memory,(0x0u+s.gp[0]*1u),4),s.reg(0,4),4);
L8001956d: if(s.services.instruction)s.services.instruction(0x8001956du);if(s.flags.zf)goto L80019578;
L8001956f: if(s.services.instruction)s.services.instruction(0x8001956fu);s.set(2,s.flags.logic((s.reg(2,4))^(s.reg(2,4)),4),4);
L80019571: if(s.services.instruction)s.services.instruction(0x80019571u);s.set(1,s.reg(6,4),4);
L80019573: if(s.services.instruction)s.services.instruction(0x80019573u);s.push(0x80019573u,0x80019578u);s.required(0x80019573u,0x8001c033u,false);
L80019578: if(s.services.instruction)s.services.instruction(0x80019578u);s.flags.logic((s.reg(7,4))&(s.reg(7,4)),4);
L8001957a: if(s.services.instruction)s.services.instruction(0x8001957au);if(s.flags.zf)goto L8001959d;
L8001957c: if(s.services.instruction)s.services.instruction(0x8001957cu);goto L80019580;
L8001957e: if(s.services.instruction)s.services.instruction(0x8001957eu);;
L80019580: if(s.services.instruction)s.services.instruction(0x80019580u);s.set(0,kernel_read(s,0x80019580u,(0x8003a884u),4),4);
L80019585: if(s.services.instruction)s.services.instruction(0x80019585u);s.set(1,kernel_read(s,0x80019585u,(0x8003a880u),4),4);
L8001958b: if(s.services.instruction)s.services.instruction(0x8001958bu);s.flags.minus(s.reg(0,4),kernel_read(s,0x8001958bu,(0x8003a888u),4),4);
L80019591: if(s.services.instruction)s.services.instruction(0x80019591u);if(!s.flags.zf)goto L8001957e;
L80019593: if(s.services.instruction)s.services.instruction(0x80019593u);s.push(0x80019593u,s.reg(0,4));
L80019594: if(s.services.instruction)s.services.instruction(0x80019594u);s.push(0x80019594u,s.reg(1,4));
L80019595: if(s.services.instruction)s.services.instruction(0x80019595u);s.push(0x80019595u,r(s.memory,(0x20u+s.gp[6]*1u),4));
L80019598: if(s.services.instruction)s.services.instruction(0x80019598u);s.push(0x80019598u,0x8001959du);s.required(0x80019598u,0x8001a3c3u,false);
L8001959d: if(s.services.instruction)s.services.instruction(0x8001959du);s.flags.logic((s.reg(3,4))&(s.reg(3,4)),4);
L8001959f: if(s.services.instruction)s.services.instruction(0x8001959fu);if(s.flags.zf)goto L800195c4;
L800195a1: if(s.services.instruction)s.services.instruction(0x800195a1u);s.set(0,r(s.memory,(0x24u+s.gp[6]*1u),4),4);
L800195a4: if(s.services.instruction)s.services.instruction(0x800195a4u);s.push(0x800195a4u,0xffffffffu);
L800195a6: if(s.services.instruction)s.services.instruction(0x800195a6u);s.gp[2]=si(s.gp[0])<0?0xffffffffu:0u;
L800195a7: if(s.services.instruction)s.services.instruction(0x800195a7u);s.push(0x800195a7u,0xffffd8f0u);
L800195ac: if(s.services.instruction)s.services.instruction(0x800195acu);s.push(0x800195acu,s.reg(2,4));
L800195ad: if(s.services.instruction)s.services.instruction(0x800195adu);s.push(0x800195adu,s.reg(0,4));
L800195ae: if(s.services.instruction)s.services.instruction(0x800195aeu);s.push(0x800195aeu,0x800195b3u);s.required(0x800195aeu,0x8002dab0u,false);
L800195b3: if(s.services.instruction)s.services.instruction(0x800195b3u);s.set(7,s.reg(0,4),4);
L800195b5: if(s.services.instruction)s.services.instruction(0x800195b5u);s.set(3,s.reg(2,4),4);
L800195b7: if(s.services.instruction)s.services.instruction(0x800195b7u);s.push(0x800195b7u,s.reg(3,4));
L800195b8: if(s.services.instruction)s.services.instruction(0x800195b8u);s.push(0x800195b8u,s.reg(7,4));
L800195b9: if(s.services.instruction)s.services.instruction(0x800195b9u);s.set(1,s.reg(6,4),4);
L800195bb: if(s.services.instruction)s.services.instruction(0x800195bbu);s.push(0x800195bbu,0x800195c0u);run_8001c4cf(s);
L800195c0: if(s.services.instruction)s.services.instruction(0x800195c0u);s.flags.logic((s.reg(0,4))&(s.reg(0,4)),4);
L800195c2: if(s.services.instruction)s.services.instruction(0x800195c2u);if(s.flags.zf)goto L800195b7;
L800195c4: if(s.services.instruction)s.services.instruction(0x800195c4u);s.set(1,r(s.memory,(0xffffffffu+s.gp[5]*1u),1),1);
L800195c7: if(s.services.instruction)s.services.instruction(0x800195c7u);s.push(0x800195c7u,0x800195ccu);s.required(0x800195c7u,0x80018ba0u,false);
L800195cc: if(s.services.instruction)s.services.instruction(0x800195ccu);s.set(0,r(s.memory,(0xbu+s.gp[5]*1u),1),1);
L800195cf: if(s.services.instruction)s.services.instruction(0x800195cfu);s.set(7,s.pop(),4);
L800195d0: if(s.services.instruction)s.services.instruction(0x800195d0u);s.set(6,s.pop(),4);
L800195d1: if(s.services.instruction)s.services.instruction(0x800195d1u);s.set(3,s.pop(),4);
L800195d2: if(s.services.instruction)s.services.instruction(0x800195d2u);s.gp[4]=s.gp[5];s.gp[5]=s.pop();
L800195d3: if(s.services.instruction)s.services.instruction(0x800195d3u);s.pop();s.gp[4]+=20u;return;
}
void run_80014348(TimerState &s) {
L80014348: if(s.services.instruction)s.services.instruction(0x80014348u);s.set(0,s.flags.logic((s.reg(0,4))^(s.reg(0,4)),4),4);
L8001434a: if(s.services.instruction)s.services.instruction(0x8001434au);s.set(0,kernel_read(s,0x8001434au,(0x80035c00u),1),1);
L8001434f: if(s.services.instruction)s.services.instruction(0x8001434fu);w(s.memory,s.observe,0x8001434fu,(0x80035c00u),1,0x2u);
L80014356: if(s.services.instruction)s.services.instruction(0x80014356u);s.pop();s.gp[4]+=0u;return;
}
void run_8001cd34(TimerState &s) {
L8001cd34: if(s.services.instruction)s.services.instruction(0x8001cd34u);s.push(0x8001cd34u,0x20u);
L8001cd36: if(s.services.instruction)s.services.instruction(0x8001cd36u);s.set(0,0x8003aa20u,4);
L8001cd3b: if(s.services.instruction)s.services.instruction(0x8001cd3bu);s.set(1,s.pop(),4);
L8001cd3c: if(s.services.instruction)s.services.instruction(0x8001cd3cu);w(s.memory,s.observe,0x8001cd3cu,(0x4u+s.gp[0]*1u),4,s.reg(0,4));
L8001cd3f: if(s.services.instruction)s.services.instruction(0x8001cd3fu);w(s.memory,s.observe,0x8001cd3fu,(0x0u+s.gp[0]*1u),4,s.reg(0,4));
L8001cd41: if(s.services.instruction)s.services.instruction(0x8001cd41u);s.set(0,s.flags.plus(s.reg(0,4),0x8u,4),4);
L8001cd44: if(s.services.instruction)s.services.instruction(0x8001cd44u);s.set(1,s.flags.increment(s.reg(1,4),4,true),4);
L8001cd45: if(s.services.instruction)s.services.instruction(0x8001cd45u);if(!s.flags.zf)goto L8001cd3c;
L8001cd47: if(s.services.instruction)s.services.instruction(0x8001cd47u);s.push(0x8001cd47u,0x0u);
L8001cd49: if(s.services.instruction)s.services.instruction(0x8001cd49u);s.push(0x8001cd49u,0x8001c29bu);
L8001cd4e: if(s.services.instruction)s.services.instruction(0x8001cd4eu);s.push(0x8001cd4eu,0x8003a8c0u);
L8001cd53: if(s.services.instruction)s.services.instruction(0x8001cd53u);s.push(0x8001cd53u,0x8001cd58u);run_8001a384(s);
L8001cd58: if(s.services.instruction)s.services.instruction(0x8001cd58u);s.push(0x8001cd58u,0x20u);
L8001cd5a: if(s.services.instruction)s.services.instruction(0x8001cd5au);s.set(0,0x8003a900u,4);
L8001cd5f: if(s.services.instruction)s.services.instruction(0x8001cd5fu);s.set(1,s.pop(),4);
L8001cd60: if(s.services.instruction)s.services.instruction(0x8001cd60u);w(s.memory,s.observe,0x8001cd60u,(0x4u+s.gp[0]*1u),4,s.reg(0,4));
L8001cd63: if(s.services.instruction)s.services.instruction(0x8001cd63u);w(s.memory,s.observe,0x8001cd63u,(0x0u+s.gp[0]*1u),4,s.reg(0,4));
L8001cd65: if(s.services.instruction)s.services.instruction(0x8001cd65u);s.set(0,s.flags.plus(s.reg(0,4),0x8u,4),4);
L8001cd68: if(s.services.instruction)s.services.instruction(0x8001cd68u);s.set(1,s.flags.increment(s.reg(1,4),4,true),4);
L8001cd69: if(s.services.instruction)s.services.instruction(0x8001cd69u);if(!s.flags.zf)goto L8001cd60;
L8001cd6b: if(s.services.instruction)s.services.instruction(0x8001cd6bu);s.set(0,0x8003a8a8u,4);
L8001cd70: if(s.services.instruction)s.services.instruction(0x8001cd70u);w(s.memory,s.observe,0x8001cd70u,(0x8003a8acu),4,s.reg(0,4));
L8001cd75: if(s.services.instruction)s.services.instruction(0x8001cd75u);w(s.memory,s.observe,0x8001cd75u,(0x8003a8a8u),4,s.reg(0,4));
L8001cd7a: if(s.services.instruction)s.services.instruction(0x8001cd7au);s.pop();s.gp[4]+=0u;return;
}
void run_8001a384(TimerState &s) {
L8001a384: if(s.services.instruction)s.services.instruction(0x8001a384u);s.set(0,r(s.memory,(0x4u+s.gp[4]*1u),4),4);
L8001a388: if(s.services.instruction)s.services.instruction(0x8001a388u);s.set(1,r(s.memory,(0x8u+s.gp[4]*1u),4),4);
L8001a38c: if(s.services.instruction)s.services.instruction(0x8001a38cu);w(s.memory,s.observe,0x8001a38cu,(0x2u+s.gp[0]*1u),1,s.flags.logic((r(s.memory,(0x2u+s.gp[0]*1u),1))&(0x0u),1));
L8001a390: if(s.services.instruction)s.services.instruction(0x8001a390u);w(s.memory,s.observe,0x8001a390u,(0xcu+s.gp[0]*1u),4,s.reg(1,4));
L8001a393: if(s.services.instruction)s.services.instruction(0x8001a393u);s.set(1,r(s.memory,(0xcu+s.gp[4]*1u),4),4);
L8001a397: if(s.services.instruction)s.services.instruction(0x8001a397u);w(s.memory,s.observe,0x8001a397u,(0x0u+s.gp[0]*1u),2,0x13u);
L8001a39c: if(s.services.instruction)s.services.instruction(0x8001a39cu);w(s.memory,s.observe,0x8001a39cu,(0x10u+s.gp[0]*1u),4,s.reg(1,4));
L8001a39f: if(s.services.instruction)s.services.instruction(0x8001a39fu);s.pop();s.gp[4]+=12u;return;
}
void run_800194c0(TimerState &s) {
L800194c0: if(s.services.instruction)s.services.instruction(0x800194c0u);s.set(0,r(s.memory,(0x4u+s.gp[4]*1u),4),4);
L800194c4: if(s.services.instruction)s.services.instruction(0x800194c4u);s.set(1,r(s.memory,(0x8u+s.gp[4]*1u),1),1);
L800194c8: if(s.services.instruction)s.services.instruction(0x800194c8u);s.set(1,s.flags.plus(s.reg(1,1),0x8u,1),1);
L800194cb: if(s.services.instruction)s.services.instruction(0x800194cbu);s.set(2,s.flags.logic((s.reg(2,4))^(s.reg(2,4)),4),4);
L800194cd: if(s.services.instruction)s.services.instruction(0x800194cdu);w(s.memory,s.observe,0x800194cdu,(0x0u+s.gp[0]*1u),1,s.reg(1,1));
L800194cf: if(s.services.instruction)s.services.instruction(0x800194cfu);s.set(1,(0x8u+s.gp[0]*1u),4);
L800194d2: if(s.services.instruction)s.services.instruction(0x800194d2u);w(s.memory,s.observe,0x800194d2u,(0x3u+s.gp[0]*1u),1,s.reg(2,1));
L800194d5: if(s.services.instruction)s.services.instruction(0x800194d5u);w(s.memory,s.observe,0x800194d5u,(0x2u+s.gp[0]*1u),1,0xau);
L800194d9: if(s.services.instruction)s.services.instruction(0x800194d9u);w(s.memory,s.observe,0x800194d9u,(0x4u+s.gp[0]*1u),4,s.reg(2,4));
L800194dc: if(s.services.instruction)s.services.instruction(0x800194dcu);w(s.memory,s.observe,0x800194dcu,(0xcu+s.gp[0]*1u),4,s.reg(1,4));
L800194df: if(s.services.instruction)s.services.instruction(0x800194dfu);w(s.memory,s.observe,0x800194dfu,(0x0u+s.gp[1]*1u),4,s.reg(1,4));
L800194e1: if(s.services.instruction)s.services.instruction(0x800194e1u);w(s.memory,s.observe,0x800194e1u,(0x10u+s.gp[0]*1u),4,s.reg(2,4));
L800194e4: if(s.services.instruction)s.services.instruction(0x800194e4u);w(s.memory,s.observe,0x800194e4u,(0x14u+s.gp[0]*1u),4,s.reg(2,4));
L800194e7: if(s.services.instruction)s.services.instruction(0x800194e7u);w(s.memory,s.observe,0x800194e7u,(0x24u+s.gp[0]*1u),4,s.reg(2,4));
L800194ea: if(s.services.instruction)s.services.instruction(0x800194eau);s.pop();s.gp[4]+=8u;return;
}
void run_8001c4cf(TimerState &s) {
L8001c4cf: if(s.services.instruction)s.services.instruction(0x8001c4cfu);s.push(0x8001c4cfu,s.reg(5,4));
L8001c4d0: if(s.services.instruction)s.services.instruction(0x8001c4d0u);s.set(5,s.reg(4,4),4);
L8001c4d2: if(s.services.instruction)s.services.instruction(0x8001c4d2u);s.push(0x8001c4d2u,s.reg(1,4));
L8001c4d3: if(s.services.instruction)s.services.instruction(0x8001c4d3u);w(s.memory,s.observe,0x8001c4d3u,(0xfffffffcu+s.gp[5]*1u),4,s.reg(1,4));
L8001c4d6: if(s.services.instruction)s.services.instruction(0x8001c4d6u);w(s.memory,s.observe,0x8001c4d6u,(0x3u+s.gp[1]*1u),1,0x1u);
L8001c4da: if(s.services.instruction)s.services.instruction(0x8001c4dau);s.set(0,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c4dd: if(s.services.instruction)s.services.instruction(0x8001c4ddu);s.set(2,s.flags.logic((s.reg(2,4))^(s.reg(2,4)),4),4);
L8001c4df: if(s.services.instruction)s.services.instruction(0x8001c4dfu);w(s.memory,s.observe,0x8001c4dfu,(0x1u+s.gp[0]*1u),1,s.reg(2,1));
L8001c4e2: if(s.services.instruction)s.services.instruction(0x8001c4e2u);s.set(0,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c4e5: if(s.services.instruction)s.services.instruction(0x8001c4e5u);s.flags.minus(r(s.memory,(0x24u+s.gp[0]*1u),4),s.reg(2,4),4);
L8001c4e8: if(s.services.instruction)s.services.instruction(0x8001c4e8u);if(!s.flags.zf)goto L8001c4ed;
L8001c4ea: if(s.services.instruction)s.services.instruction(0x8001c4eau);w(s.memory,s.observe,0x8001c4eau,(0x4u+s.gp[0]*1u),4,s.reg(2,4));
L8001c4ed: if(s.services.instruction)s.services.instruction(0x8001c4edu);s.flags.minus(r(s.memory,(0xcu+s.gp[5]*1u),4),s.reg(2,4),4);
L8001c4f0: if(s.services.instruction)s.services.instruction(0x8001c4f0u);if(s.flags.sf!=s.flags.of)goto L8001c536;
L8001c4f2: if(s.services.instruction)s.services.instruction(0x8001c4f2u);goto L8001c4f6;
L8001c4f4: if(s.services.instruction)s.services.instruction(0x8001c4f4u);;
L8001c4f6: if(s.services.instruction)s.services.instruction(0x8001c4f6u);s.set(0,kernel_read(s,0x8001c4f6u,(0x8003a884u),4),4);
L8001c4fb: if(s.services.instruction)s.services.instruction(0x8001c4fbu);s.set(1,kernel_read(s,0x8001c4fbu,(0x8003a880u),4),4);
L8001c501: if(s.services.instruction)s.services.instruction(0x8001c501u);s.flags.minus(s.reg(0,4),kernel_read(s,0x8001c501u,(0x8003a888u),4),4);
L8001c507: if(s.services.instruction)s.services.instruction(0x8001c507u);if(!s.flags.zf)goto L8001c4f4;
L8001c509: if(s.services.instruction)s.services.instruction(0x8001c509u);s.set(1,s.flags.minus(s.reg(1,4),r(s.memory,(0x8u+s.gp[5]*1u),4),4),4);
L8001c50c: if(s.services.instruction)s.services.instruction(0x8001c50cu);s.set(0,s.flags.borrow(s.reg(0,4),r(s.memory,(0xcu+s.gp[5]*1u),4),4),4);
L8001c50f: if(s.services.instruction)s.services.instruction(0x8001c50fu);if(s.flags.sf)goto L8001c525;
L8001c511: if(s.services.instruction)s.services.instruction(0x8001c511u);s.set(0,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c514: if(s.services.instruction)s.services.instruction(0x8001c514u);w(s.memory,s.observe,0x8001c514u,(0x4u+s.gp[0]*1u),4,0x1u);
L8001c51b: if(s.services.instruction)s.services.instruction(0x8001c51bu);s.set(0,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c51e: if(s.services.instruction)s.services.instruction(0x8001c51eu);w(s.memory,s.observe,0x8001c51eu,(0x3u+s.gp[0]*1u),1,s.reg(2,1));
L8001c521: if(s.services.instruction)s.services.instruction(0x8001c521u);s.set(0,s.flags.logic((s.reg(0,4))^(s.reg(0,4)),4),4);
L8001c523: if(s.services.instruction)s.services.instruction(0x8001c523u);goto L8001c559;
L8001c525: if(s.services.instruction)s.services.instruction(0x8001c525u);w(s.memory,s.observe,0x8001c525u,(0xcu+s.gp[5]*1u),4,s.reg(0,4));
L8001c528: if(s.services.instruction)s.services.instruction(0x8001c528u);s.set(0,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c52b: if(s.services.instruction)s.services.instruction(0x8001c52bu);w(s.memory,s.observe,0x8001c52bu,(0x8u+s.gp[5]*1u),4,s.reg(1,4));
L8001c52e: if(s.services.instruction)s.services.instruction(0x8001c52eu);w(s.memory,s.observe,0x8001c52eu,(0x1u+s.gp[0]*1u),1,0x1u);
L8001c532: if(s.services.instruction)s.services.instruction(0x8001c532u);goto L8001c536;
L8001c534: if(s.services.instruction)s.services.instruction(0x8001c534u);;
L8001c536: if(s.services.instruction)s.services.instruction(0x8001c536u);s.set(0,kernel_read(s,0x8001c536u,(0x8003a894u),4),4);
L8001c53b: if(s.services.instruction)s.services.instruction(0x8001c53bu);s.set(1,kernel_read(s,0x8001c53bu,(0x8003a890u),4),4);
L8001c541: if(s.services.instruction)s.services.instruction(0x8001c541u);s.flags.minus(s.reg(0,4),kernel_read(s,0x8001c541u,(0x8003a898u),4),4);
L8001c547: if(s.services.instruction)s.services.instruction(0x8001c547u);if(!s.flags.zf)goto L8001c534;
L8001c549: if(s.services.instruction)s.services.instruction(0x8001c549u);s.push(0x8001c549u,s.reg(0,4));
L8001c54a: if(s.services.instruction)s.services.instruction(0x8001c54au);s.push(0x8001c54au,s.reg(1,4));
L8001c54b: if(s.services.instruction)s.services.instruction(0x8001c54bu);s.push(0x8001c54bu,r(s.memory,(0xcu+s.gp[5]*1u),4));
L8001c54e: if(s.services.instruction)s.services.instruction(0x8001c54eu);s.set(1,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c551: if(s.services.instruction)s.services.instruction(0x8001c551u);s.push(0x8001c551u,r(s.memory,(0x8u+s.gp[5]*1u),4));
L8001c554: if(s.services.instruction)s.services.instruction(0x8001c554u);s.push(0x8001c554u,0x8001c559u);run_8001c3cf(s);
L8001c559: if(s.services.instruction)s.services.instruction(0x8001c559u);s.gp[4]=s.gp[5];s.gp[5]=s.pop();
L8001c55a: if(s.services.instruction)s.services.instruction(0x8001c55au);s.pop();s.gp[4]+=8u;return;
}
void run_8001c3cf(TimerState &s) {
L8001c3cf: if(s.services.instruction)s.services.instruction(0x8001c3cfu);s.push(0x8001c3cfu,s.reg(5,4));
L8001c3d0: if(s.services.instruction)s.services.instruction(0x8001c3d0u);s.set(5,s.reg(4,4),4);
L8001c3d2: if(s.services.instruction)s.services.instruction(0x8001c3d2u);s.push(0x8001c3d2u,s.reg(1,4));
L8001c3d3: if(s.services.instruction)s.services.instruction(0x8001c3d3u);s.push(0x8001c3d3u,s.reg(3,4));
L8001c3d4: if(s.services.instruction)s.services.instruction(0x8001c3d4u);s.push(0x8001c3d4u,s.reg(6,4));
L8001c3d5: if(s.services.instruction)s.services.instruction(0x8001c3d5u);s.push(0x8001c3d5u,s.reg(7,4));
L8001c3d6: if(s.services.instruction)s.services.instruction(0x8001c3d6u);s.push(0x8001c3d6u,s.reg(1,4));
L8001c3d7: if(s.services.instruction)s.services.instruction(0x8001c3d7u);s.push(0x8001c3d7u,r(s.memory,(0x14u+s.gp[5]*1u),4));
L8001c3da: if(s.services.instruction)s.services.instruction(0x8001c3dau);w(s.memory,s.observe,0x8001c3dau,(0xfffffffcu+s.gp[5]*1u),4,s.reg(1,4));
L8001c3dd: if(s.services.instruction)s.services.instruction(0x8001c3ddu);s.push(0x8001c3ddu,r(s.memory,(0x10u+s.gp[5]*1u),4));
L8001c3e0: if(s.services.instruction)s.services.instruction(0x8001c3e0u);s.push(0x8001c3e0u,r(s.memory,(0xcu+s.gp[5]*1u),4));
L8001c3e3: if(s.services.instruction)s.services.instruction(0x8001c3e3u);s.push(0x8001c3e3u,r(s.memory,(0x8u+s.gp[5]*1u),4));
L8001c3e6: if(s.services.instruction)s.services.instruction(0x8001c3e6u);s.push(0x8001c3e6u,0x8001c3ebu);run_8001ccac(s);
L8001c3eb: if(s.services.instruction)s.services.instruction(0x8001c3ebu);s.set(2,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c3ee: if(s.services.instruction)s.services.instruction(0x8001c3eeu);s.set(1,(0x8003a900u+s.gp[0]*8u),4);
L8001c3f5: if(s.services.instruction)s.services.instruction(0x8001c3f5u);s.set(0,r(s.memory,(0x4u+s.gp[1]*1u),4),4);
L8001c3f8: if(s.services.instruction)s.services.instruction(0x8001c3f8u);s.flags.minus(s.reg(0,4),s.reg(1,4),4);
L8001c3fa: if(s.services.instruction)s.services.instruction(0x8001c3fau);if(s.flags.zf)goto L8001c421;
L8001c3fc: if(s.services.instruction)s.services.instruction(0x8001c3fcu);s.set(7,r(s.memory,(0x14u+s.gp[2]*1u),4),4);
L8001c3ff: if(s.services.instruction)s.services.instruction(0x8001c3ffu);s.set(6,r(s.memory,(0xfffffffcu+s.gp[0]*1u),4),4);
L8001c402: if(s.services.instruction)s.services.instruction(0x8001c402u);s.flags.minus(s.reg(7,4),s.reg(6,4),4);
L8001c404: if(s.services.instruction)s.services.instruction(0x8001c404u);if(!s.flags.zf)goto L8001c414;
L8001c406: if(s.services.instruction)s.services.instruction(0x8001c406u);s.set(3,r(s.memory,(0x10u+s.gp[2]*1u),4),4);
L8001c409: if(s.services.instruction)s.services.instruction(0x8001c409u);s.flags.minus(s.reg(3,4),r(s.memory,(0xfffffff8u+s.gp[0]*1u),4),4);
L8001c40c: if(s.services.instruction)s.services.instruction(0x8001c40cu);if(!s.flags.cf)goto L8001c4ae;
L8001c412: if(s.services.instruction)s.services.instruction(0x8001c412u);s.flags.minus(s.reg(7,4),s.reg(6,4),4);
L8001c414: if(s.services.instruction)s.services.instruction(0x8001c414u);if(!s.flags.cf&&!s.flags.zf)goto L8001c4ae;
L8001c41a: if(s.services.instruction)s.services.instruction(0x8001c41au);s.set(0,r(s.memory,(0x4u+s.gp[0]*1u),4),4);
L8001c41d: if(s.services.instruction)s.services.instruction(0x8001c41du);s.flags.minus(s.reg(0,4),s.reg(1,4),4);
L8001c41f: if(s.services.instruction)s.services.instruction(0x8001c41fu);if(!s.flags.zf)goto L8001c3ff;
L8001c421: if(s.services.instruction)s.services.instruction(0x8001c421u);s.set(0,r(s.memory,(0x0u+s.gp[1]*1u),4),4);
L8001c423: if(s.services.instruction)s.services.instruction(0x8001c423u);w(s.memory,s.observe,0x8001c423u,(0x18u+s.gp[2]*1u),4,s.reg(0,4));
L8001c426: if(s.services.instruction)s.services.instruction(0x8001c426u);s.set(2,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c429: if(s.services.instruction)s.services.instruction(0x8001c429u);w(s.memory,s.observe,0x8001c429u,(0x1cu+s.gp[2]*1u),4,s.reg(1,4));
L8001c42c: if(s.services.instruction)s.services.instruction(0x8001c42cu);s.set(2,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c42f: if(s.services.instruction)s.services.instruction(0x8001c42fu);s.set(2,s.flags.plus(s.reg(2,4),0x18u,4),4);
L8001c432: if(s.services.instruction)s.services.instruction(0x8001c432u);w(s.memory,s.observe,0x8001c432u,(0x4u+s.gp[0]*1u),4,s.reg(2,4));
L8001c435: if(s.services.instruction)s.services.instruction(0x8001c435u);s.set(2,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c438: if(s.services.instruction)s.services.instruction(0x8001c438u);s.set(0,(0x18u+s.gp[2]*1u),4);
L8001c43b: if(s.services.instruction)s.services.instruction(0x8001c43bu);w(s.memory,s.observe,0x8001c43bu,(0x0u+s.gp[1]*1u),4,s.reg(0,4));
L8001c43d: if(s.services.instruction)s.services.instruction(0x8001c43du);s.set(0,kernel_read(s,0x8001c43du,(0x8003a894u),4),4);
L8001c442: if(s.services.instruction)s.services.instruction(0x8001c442u);s.set(1,kernel_read(s,0x8001c442u,(0x8003a890u),4),4);
L8001c448: if(s.services.instruction)s.services.instruction(0x8001c448u);s.flags.minus(s.reg(0,4),kernel_read(s,0x8001c448u,(0x8003a898u),4),4);
L8001c44e: if(s.services.instruction)s.services.instruction(0x8001c44eu);w(s.memory,s.observe,0x8001c44eu,(0x14u+s.gp[5]*1u),4,s.reg(0,4));
L8001c451: if(s.services.instruction)s.services.instruction(0x8001c451u);w(s.memory,s.observe,0x8001c451u,(0x10u+s.gp[5]*1u),4,s.reg(1,4));
L8001c454: if(s.services.instruction)s.services.instruction(0x8001c454u);if(s.flags.zf)goto L8001c474;
L8001c456: if(s.services.instruction)s.services.instruction(0x8001c456u);;
L8001c458: if(s.services.instruction)s.services.instruction(0x8001c458u);s.set(0,kernel_read(s,0x8001c458u,(0x8003a894u),4),4);
L8001c45d: if(s.services.instruction)s.services.instruction(0x8001c45du);s.set(1,kernel_read(s,0x8001c45du,(0x8003a890u),4),4);
L8001c463: if(s.services.instruction)s.services.instruction(0x8001c463u);s.flags.minus(s.reg(0,4),kernel_read(s,0x8001c463u,(0x8003a898u),4),4);
L8001c469: if(s.services.instruction)s.services.instruction(0x8001c469u);w(s.memory,s.observe,0x8001c469u,(0x14u+s.gp[5]*1u),4,s.reg(0,4));
L8001c46c: if(s.services.instruction)s.services.instruction(0x8001c46cu);w(s.memory,s.observe,0x8001c46cu,(0x10u+s.gp[5]*1u),4,s.reg(1,4));
L8001c46f: if(s.services.instruction)s.services.instruction(0x8001c46fu);if(!s.flags.zf)goto L8001c456;
L8001c471: if(s.services.instruction)s.services.instruction(0x8001c471u);s.set(2,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c474: if(s.services.instruction)s.services.instruction(0x8001c474u);s.set(6,r(s.memory,(0x14u+s.gp[2]*1u),4),4);
L8001c477: if(s.services.instruction)s.services.instruction(0x8001c477u);s.flags.minus(s.reg(6,4),s.reg(0,4),4);
L8001c479: if(s.services.instruction)s.services.instruction(0x8001c479u);if(!s.flags.zf)goto L8001c482;
L8001c47b: if(s.services.instruction)s.services.instruction(0x8001c47bu);s.flags.minus(r(s.memory,(0x10u+s.gp[2]*1u),4),s.reg(1,4),4);
L8001c47e: if(s.services.instruction)s.services.instruction(0x8001c47eu);if(s.flags.cf||s.flags.zf)goto L8001c484;
L8001c480: if(s.services.instruction)s.services.instruction(0x8001c480u);s.flags.minus(s.reg(6,4),s.reg(0,4),4);
L8001c482: if(s.services.instruction)s.services.instruction(0x8001c482u);if(!s.flags.cf)goto L8001c4a3;
L8001c484: if(s.services.instruction)s.services.instruction(0x8001c484u);w(s.memory,s.observe,0x8001c484u,(0x3u+s.gp[2]*1u),1,s.flags.logic((r(s.memory,(0x3u+s.gp[2]*1u),1))&(0x0u),1));
L8001c488: if(s.services.instruction)s.services.instruction(0x8001c488u);s.set(1,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c48b: if(s.services.instruction)s.services.instruction(0x8001c48bu);s.set(0,r(s.memory,(0x18u+s.gp[1]*1u),4),4);
L8001c48e: if(s.services.instruction)s.services.instruction(0x8001c48eu);s.set(1,r(s.memory,(0x1cu+s.gp[1]*1u),4),4);
L8001c491: if(s.services.instruction)s.services.instruction(0x8001c491u);w(s.memory,s.observe,0x8001c491u,(0x0u+s.gp[1]*1u),4,s.reg(0,4));
L8001c493: if(s.services.instruction)s.services.instruction(0x8001c493u);w(s.memory,s.observe,0x8001c493u,(0x4u+s.gp[0]*1u),4,s.reg(1,4));
L8001c496: if(s.services.instruction)s.services.instruction(0x8001c496u);s.set(0,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c499: if(s.services.instruction)s.services.instruction(0x8001c499u);w(s.memory,s.observe,0x8001c499u,(0x4u+s.gp[0]*1u),4,0x1u);
L8001c4a0: if(s.services.instruction)s.services.instruction(0x8001c4a0u);s.set(2,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c4a3: if(s.services.instruction)s.services.instruction(0x8001c4a3u);s.set(0,r(s.memory,(0x3u+s.gp[2]*1u),1),4);
L8001c4a7: if(s.services.instruction)s.services.instruction(0x8001c4a7u);s.set(7,s.pop(),4);
L8001c4a8: if(s.services.instruction)s.services.instruction(0x8001c4a8u);s.set(6,s.pop(),4);
L8001c4a9: if(s.services.instruction)s.services.instruction(0x8001c4a9u);s.set(3,s.pop(),4);
L8001c4aa: if(s.services.instruction)s.services.instruction(0x8001c4aau);s.gp[4]=s.gp[5];s.gp[5]=s.pop();
L8001c4ab: if(s.services.instruction)s.services.instruction(0x8001c4abu);s.pop();s.gp[4]+=16u;return;
L8001c4ae: if(s.services.instruction)s.services.instruction(0x8001c4aeu);s.set(1,r(s.memory,(0x0u+s.gp[0]*1u),4),4);
L8001c4b0: if(s.services.instruction)s.services.instruction(0x8001c4b0u);w(s.memory,s.observe,0x8001c4b0u,(0x18u+s.gp[2]*1u),4,s.reg(1,4));
L8001c4b3: if(s.services.instruction)s.services.instruction(0x8001c4b3u);s.set(2,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c4b6: if(s.services.instruction)s.services.instruction(0x8001c4b6u);w(s.memory,s.observe,0x8001c4b6u,(0x1cu+s.gp[2]*1u),4,s.reg(0,4));
L8001c4b9: if(s.services.instruction)s.services.instruction(0x8001c4b9u);s.set(2,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c4bc: if(s.services.instruction)s.services.instruction(0x8001c4bcu);s.set(2,s.flags.plus(s.reg(2,4),0x18u,4),4);
L8001c4bf: if(s.services.instruction)s.services.instruction(0x8001c4bfu);w(s.memory,s.observe,0x8001c4bfu,(0x4u+s.gp[1]*1u),4,s.reg(2,4));
L8001c4c2: if(s.services.instruction)s.services.instruction(0x8001c4c2u);s.set(1,r(s.memory,(0xfffffffcu+s.gp[5]*1u),4),4);
L8001c4c5: if(s.services.instruction)s.services.instruction(0x8001c4c5u);s.set(1,s.flags.plus(s.reg(1,4),0x18u,4),4);
L8001c4c8: if(s.services.instruction)s.services.instruction(0x8001c4c8u);w(s.memory,s.observe,0x8001c4c8u,(0x0u+s.gp[0]*1u),4,s.reg(1,4));
L8001c4ca: if(s.services.instruction)s.services.instruction(0x8001c4cau);s.set(0,s.flags.logic((s.reg(0,4))^(s.reg(0,4)),4),4);
L8001c4cc: if(s.services.instruction)s.services.instruction(0x8001c4ccu);s.set(0,s.flags.increment(s.reg(0,4),4,false),4);
L8001c4cd: if(s.services.instruction)s.services.instruction(0x8001c4cdu);goto L8001c4a7;
}
void run_8001ccac(TimerState &s) {
L8001ccac: if(s.services.instruction)s.services.instruction(0x8001ccacu);s.set(4,s.flags.minus(s.reg(4,4),0x14u,4),4);
L8001ccaf: if(s.services.instruction)s.services.instruction(0x8001ccafu);w(s.memory,s.observe,0x8001ccafu,(0x10u+s.gp[4]*1u),4,s.reg(3,4));
L8001ccb3: if(s.services.instruction)s.services.instruction(0x8001ccb3u);s.set(3,r(s.memory,(0x20u+s.gp[4]*1u),4),4);
L8001ccb7: if(s.services.instruction)s.services.instruction(0x8001ccb7u);s.set(1,r(s.memory,(0x24u+s.gp[4]*1u),4),4);
L8001ccbb: if(s.services.instruction)s.services.instruction(0x8001ccbbu);s.set(3,s.flags.minus(s.reg(3,4),r(s.memory,(0x18u+s.gp[4]*1u),4),4),4);
L8001ccbf: if(s.services.instruction)s.services.instruction(0x8001ccbfu);s.set(1,s.flags.borrow(s.reg(1,4),r(s.memory,(0x1cu+s.gp[4]*1u),4),4),4);
L8001ccc3: if(s.services.instruction)s.services.instruction(0x8001ccc3u);s.set(0,r(s.memory,(0x28u+s.gp[4]*1u),4),4);
L8001ccc7: if(s.services.instruction)s.services.instruction(0x8001ccc7u);w(s.memory,s.observe,0x8001ccc7u,(0x10u+s.gp[0]*1u),4,s.reg(3,4));
L8001ccca: if(s.services.instruction)s.services.instruction(0x8001cccau);w(s.memory,s.observe,0x8001cccau,(0x14u+s.gp[0]*1u),4,s.reg(1,4));
L8001cccd: if(s.services.instruction)s.services.instruction(0x8001cccdu);s.set(0,s.reg(3,4),4);
L8001cccf: if(s.services.instruction)s.services.instruction(0x8001cccfu);{const auto product=std::uint64_t(s.gp[0])*kernel_read(s,0x8001cccfu,(0x80010cc8u),4);s.gp[0]=std::uint32_t(product);s.gp[2]=std::uint32_t(product>>32u);s.flags.cf=s.flags.of=s.gp[2]!=0;}
L8001ccd5: if(s.services.instruction)s.services.instruction(0x8001ccd5u);w(s.memory,s.observe,0x8001ccd5u,(0xcu+s.gp[4]*1u),4,s.reg(2,4));
L8001ccd9: if(s.services.instruction)s.services.instruction(0x8001ccd9u);s.set(0,s.reg(3,4),4);
L8001ccdb: if(s.services.instruction)s.services.instruction(0x8001ccdbu);{const auto product=std::uint64_t(s.gp[0])*kernel_read(s,0x8001ccdbu,(0x80010cccu),4);s.gp[0]=std::uint32_t(product);s.gp[2]=std::uint32_t(product>>32u);s.flags.cf=s.flags.of=s.gp[2]!=0;}
L8001cce1: if(s.services.instruction)s.services.instruction(0x8001cce1u);w(s.memory,s.observe,0x8001cce1u,(0x8u+s.gp[4]*1u),4,s.reg(0,4));
L8001cce5: if(s.services.instruction)s.services.instruction(0x8001cce5u);w(s.memory,s.observe,0x8001cce5u,(0x4u+s.gp[4]*1u),4,s.reg(2,4));
L8001cce9: if(s.services.instruction)s.services.instruction(0x8001cce9u);s.set(0,s.reg(1,4),4);
L8001cceb: if(s.services.instruction)s.services.instruction(0x8001ccebu);{const auto product=std::uint64_t(s.gp[0])*kernel_read(s,0x8001ccebu,(0x80010cc8u),4);s.gp[0]=std::uint32_t(product);s.gp[2]=std::uint32_t(product>>32u);s.flags.cf=s.flags.of=s.gp[2]!=0;}
L8001ccf1: if(s.services.instruction)s.services.instruction(0x8001ccf1u);w(s.memory,s.observe,0x8001ccf1u,(0x0u+s.gp[4]*1u),4,s.reg(2,4));
L8001ccf4: if(s.services.instruction)s.services.instruction(0x8001ccf4u);s.set(3,s.flags.logic((s.reg(3,4))^(s.reg(3,4)),4),4);
L8001ccf6: if(s.services.instruction)s.services.instruction(0x8001ccf6u);s.set(0,s.flags.plus(s.reg(0,4),r(s.memory,(0x8u+s.gp[4]*1u),4),4),4);
L8001ccfa: if(s.services.instruction)s.services.instruction(0x8001ccfau);s.set(3,carry_add(s,s.reg(3,4),0x0u,4),4);
L8001ccfd: if(s.services.instruction)s.services.instruction(0x8001ccfdu);s.set(0,s.flags.plus(s.reg(0,4),r(s.memory,(0xcu+s.gp[4]*1u),4),4),4);
L8001cd01: if(s.services.instruction)s.services.instruction(0x8001cd01u);s.set(3,carry_add(s,s.reg(3,4),0x0u,4),4);
L8001cd04: if(s.services.instruction)s.services.instruction(0x8001cd04u);s.set(0,s.reg(1,4),4);
L8001cd06: if(s.services.instruction)s.services.instruction(0x8001cd06u);{const auto product=std::uint64_t(s.gp[0])*kernel_read(s,0x8001cd06u,(0x80010cccu),4);s.gp[0]=std::uint32_t(product);s.gp[2]=std::uint32_t(product>>32u);s.flags.cf=s.flags.of=s.gp[2]!=0;}
L8001cd0c: if(s.services.instruction)s.services.instruction(0x8001cd0cu);s.set(0,s.flags.plus(s.reg(0,4),s.reg(3,4),4),4);
L8001cd0e: if(s.services.instruction)s.services.instruction(0x8001cd0eu);s.set(2,carry_add(s,s.reg(2,4),0x0u,4),4);
L8001cd11: if(s.services.instruction)s.services.instruction(0x8001cd11u);s.set(0,s.flags.plus(s.reg(0,4),r(s.memory,(0x0u+s.gp[4]*1u),4),4),4);
L8001cd14: if(s.services.instruction)s.services.instruction(0x8001cd14u);s.set(2,carry_add(s,s.reg(2,4),0x0u,4),4);
L8001cd17: if(s.services.instruction)s.services.instruction(0x8001cd17u);s.set(0,s.flags.plus(s.reg(0,4),r(s.memory,(0x4u+s.gp[4]*1u),4),4),4);
L8001cd1b: if(s.services.instruction)s.services.instruction(0x8001cd1bu);s.set(2,carry_add(s,s.reg(2,4),0x0u,4),4);
L8001cd1e: if(s.services.instruction)s.services.instruction(0x8001cd1eu);s.set(1,kernel_read(s,0x8001cd1eu,(0x80010cd0u),1),1);
L8001cd24: if(s.services.instruction)s.services.instruction(0x8001cd24u);s.set(0,double_right(s,s.reg(0,4),s.reg(2,4),s.reg(1,1)),4);
L8001cd27: if(s.services.instruction)s.services.instruction(0x8001cd27u);s.set(3,r(s.memory,(0x10u+s.gp[4]*1u),4),4);
L8001cd2b: if(s.services.instruction)s.services.instruction(0x8001cd2bu);s.set(4,s.flags.plus(s.reg(4,4),0x14u,4),4);
L8001cd2e: if(s.services.instruction)s.services.instruction(0x8001cd2eu);s.set(0,s.flags.logic((s.reg(0,4))&(0x1fu),4),4);
L8001cd31: if(s.services.instruction)s.services.instruction(0x8001cd31u);s.pop();s.gp[4]+=20u;return;
}
}
void source_run_tu15_timer_runtime(SourcePhysicsContactMemory &m,std::uint32_t entry,
 SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
 const SourcePhysicsContactRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe,
 const SourceTu15TimerDataOwner &data_owner) {
 FpScope scope(fp);TimerState s{m,observe,services,data_owner};s.load(abi);
 try {switch(entry) {
case 0x800195d6u:run_800195d6(s);break;
case 0x8001951cu:run_8001951c(s);break;
case 0x80014348u:run_80014348(s);break;
case 0x8001cd34u:run_8001cd34(s);break;
case 0x8001a384u:run_8001a384(s);break;
case 0x800194c0u:run_800194c0(s);break;
case 0x8001c4cfu:run_8001c4cf(s);break;
case 0x8001c3cfu:run_8001c3cf(s);break;
case 0x8001ccacu:run_8001ccac(s);break;
default:throw std::runtime_error("Unverified original timer entry");}}
 catch(...) {abi=s.save();throw;}abi=s.save();
}
} // namespace halo2
