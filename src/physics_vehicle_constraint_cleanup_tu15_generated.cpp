// Generated from complete owned TU1.5 instructions. No runtime decoder.
#include "halo2/physics_vehicle_constraint_cleanup.hpp"
#include "physics_vehicle_motion_math_internal.hpp"
namespace halo2 {
namespace {
using namespace physics_vehicle_motion_math_detail;
std::uint32_t integer_spill_observed(MathState &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 const auto input=s.fp.st(0);std::uint32_t result;std::uint16_t status;
 asm volatile("fldt %2\n\tfistpl %0\n\tfnstsw %1":"=m"(result),"=m"(status):"m"(input):"st");
 s.fp.status=std::uint16_t((s.fp.status&~0x200u)|(status&0x200u));return result;
#else
 throw std::runtime_error("Required TU1.5 FISTP environment backend unavailable");
#endif
}
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
 if(n==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x))));
 if(n==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));
 throw std::runtime_error("Unverified TU1.5 sign-extension width");
}
void run_de8c0(MathState &);
void run_16e6a0(MathState &);
void run_178e30(MathState &);
void run_17a0f0(MathState &);
void run_17dec0(MathState &);
void run_18bbc0(MathState &);
void run_18c440(MathState &);
void run_18cdd0(MathState &);
void run_18d4b0(MathState &);
void run_18d510(MathState &);
void run_1903a0(MathState &);
void run_1d8b90(MathState &);
void run_22e210(MathState &);
void run_22e390(MathState &);
void run_22e4f0(MathState &);
void run_22eec0(MathState &);
void run_22efd0(MathState &);
void run_22fe30(MathState &);
void run_22fea0(MathState &);
void run_230a60(MathState &);
void run_230ae0(MathState &);
void run_de8c0(MathState &s) {
if(s.services.instruction)s.services.instruction(0xde8c0u); s.set(2,r(s.memory,(0x7cu+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0xde8c3u); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0xde8c6u); s.set(0,(0x0u+s.gp[2]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0xde8c9u); s.set(0,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0xde8ccu); s.set(2,r(s.memory,(0xcu+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0xde8cfu); s.set(1,r(s.memory,(0x34u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0xde8d2u); s.set(0,r(s.memory,(0x10u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0xde8d5u); s.push(0xde8d5u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0xde8d6u); s.set(6,s.flags.logic((s.reg(6,4,0))^(s.reg(6,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0xde8d8u); s.flags.logic((s.reg(1,4,0))&(s.reg(1,4,0)),4);
if(s.services.instruction)s.services.instruction(0xde8dau); s.push(0xde8dau,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0xde8dbu); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto Lde8f2;
if(s.services.instruction)s.services.instruction(0xde8ddu); s.set(2,r(s.memory,(0x30u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0xde8e0u); s.set(7,s.reg(2,4,0),4,0);
Lde8e2: if(s.services.instruction)s.services.instruction(0xde8e2u); s.flags.minus(r(s.memory,(0x0u+s.gp[7]),4),0x2002u,4);
if(s.services.instruction)s.services.instruction(0xde8e8u); if(s.flags.zf)goto Lde91a;
if(s.services.instruction)s.services.instruction(0xde8eau); s.set(6,s.flags.increment(s.reg(6,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0xde8ebu); s.set(7,s.flags.plus(s.reg(7,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0xde8eeu); s.flags.minus(s.reg(6,4,0),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0xde8f0u); if(s.flags.sf!=s.flags.of)goto Lde8e2;
Lde8f2: if(s.services.instruction)s.services.instruction(0xde8f2u); s.set(6,s.flags.logic((s.reg(6,4,0))^(s.reg(6,4,0)),4),4,0);
Lde8f4: if(s.services.instruction)s.services.instruction(0xde8f4u); s.set(1,r(s.memory,(0xcu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0xde8f8u); w(s.memory,s.observe,0xde8f8u,(0x0u+s.gp[1]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0xde8fau); s.set(2,r(s.memory,(0x34u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0xde8fdu); s.set(1,s.flags.logic((s.reg(1,4,0))^(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0xde8ffu); s.flags.logic((s.reg(2,4,0))&(s.reg(2,4,0)),4);
if(s.services.instruction)s.services.instruction(0xde901u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto Lde929;
if(s.services.instruction)s.services.instruction(0xde903u); s.set(7,r(s.memory,(0x30u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0xde906u); s.set(6,s.reg(7,4,0),4,0);
Lde908: if(s.services.instruction)s.services.instruction(0xde908u); s.flags.minus(r(s.memory,(0x0u+s.gp[6]),4),0x2001u,4);
if(s.services.instruction)s.services.instruction(0xde90eu); if(s.flags.zf)goto Lde920;
if(s.services.instruction)s.services.instruction(0xde910u); s.set(1,s.flags.increment(s.reg(1,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0xde911u); s.set(6,s.flags.plus(s.reg(6,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0xde914u); s.flags.minus(s.reg(1,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0xde916u); if(s.flags.sf!=s.flags.of)goto Lde908;
if(s.services.instruction)s.services.instruction(0xde918u); goto Lde929;
Lde91a: if(s.services.instruction)s.services.instruction(0xde91au); s.set(6,r(s.memory,(0x4u+s.gp[2]+s.gp[6]*8u),4),4,0);
if(s.services.instruction)s.services.instruction(0xde91eu); goto Lde8f4;
Lde920: if(s.services.instruction)s.services.instruction(0xde920u); s.set(1,r(s.memory,(0x4u+s.gp[7]+s.gp[1]*8u),4),4,0);
if(s.services.instruction)s.services.instruction(0xde924u); s.flags.minus(s.reg(1,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0xde927u); if(s.flags.zf)goto Lde958;
Lde929: if(s.services.instruction)s.services.instruction(0xde929u); s.set(1,s.flags.logic((s.reg(1,4,0))^(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0xde92bu); s.flags.logic((s.reg(2,4,0))&(s.reg(2,4,0)),4);
if(s.services.instruction)s.services.instruction(0xde92du); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto Lde944;
if(s.services.instruction)s.services.instruction(0xde92fu); s.set(6,r(s.memory,(0x30u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0xde932u); s.set(0,s.reg(6,4,0),4,0);
Lde934: if(s.services.instruction)s.services.instruction(0xde934u); s.flags.minus(r(s.memory,(0x0u+s.gp[0]),4),0x2002u,4);
if(s.services.instruction)s.services.instruction(0xde93au); if(s.flags.zf)goto Lde94d;
if(s.services.instruction)s.services.instruction(0xde93cu); s.set(1,s.flags.increment(s.reg(1,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0xde93du); s.set(0,s.flags.plus(s.reg(0,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0xde940u); s.flags.minus(s.reg(1,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0xde942u); if(s.flags.sf!=s.flags.of)goto Lde934;
Lde944: if(s.services.instruction)s.services.instruction(0xde944u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0xde945u); s.set(1,s.flags.logic((s.reg(1,4,0))^(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0xde947u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0xde948u); w(s.memory,s.observe,0xde948u,(0x0u+s.gp[3]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0xde94au); s.pop();s.gp[4]+=4u;return;
Lde94d: if(s.services.instruction)s.services.instruction(0xde94du); s.set(1,r(s.memory,(0x4u+s.gp[6]+s.gp[1]*8u),4),4,0);
if(s.services.instruction)s.services.instruction(0xde951u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0xde952u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0xde953u); w(s.memory,s.observe,0xde953u,(0x0u+s.gp[3]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0xde955u); s.pop();s.gp[4]+=4u;return;
Lde958: if(s.services.instruction)s.services.instruction(0xde958u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0xde959u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0xde95au); w(s.memory,s.observe,0xde95au,(0x0u+s.gp[3]),4,0xffffffffu);
if(s.services.instruction)s.services.instruction(0xde960u); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_16e6a0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x16e6a0u); s.push(0x16e6a0u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x16e6a1u); s.push(0x16e6a1u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x16e6a2u); s.push(0x16e6a2u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x16e6a3u); s.set(6,r(s.memory,(0x34u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e6a6u); s.push(0x16e6a6u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x16e6a7u); s.set(7,r(s.memory,(0x38u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e6aau); s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e6adu); s.flags.minus(s.reg(6,4,0),s.reg(7,4,0),4);
if(s.services.instruction)s.services.instruction(0x16e6afu); if(s.flags.sf==s.flags.of)goto L16e6db;
if(s.services.instruction)s.services.instruction(0x16e6b1u); s.set(3,r(s.memory,(0x48u+s.gp[2]),4),4,0);
L16e6b4: if(s.services.instruction)s.services.instruction(0x16e6b4u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e6b6u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0x1fu),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e6b9u); s.set(5,0x1u,4,0);
if(s.services.instruction)s.services.instruction(0x16e6beu); s.set(5,s.flags.shift(s.reg(5,4,0),s.reg(1,1,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e6c0u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e6c2u); s.set(1,s.flags.right(s.reg(1,4,0),0x5u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x16e6c5u); s.flags.logic((r(s.memory,(0x0u+s.gp[3]+s.gp[1]*4u),4))&(s.reg(5,4,0)),4);
if(s.services.instruction)s.services.instruction(0x16e6c8u); if(s.flags.zf)goto L16e6d4;
if(s.services.instruction)s.services.instruction(0x16e6cau); s.set(1,r(s.memory,(0x38u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e6cdu); s.set(6,s.flags.increment(s.reg(6,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x16e6ceu); s.flags.minus(s.reg(6,4,0),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x16e6d0u); if(s.flags.sf!=s.flags.of)goto L16e6b4;
if(s.services.instruction)s.services.instruction(0x16e6d2u); goto L16e6db;
L16e6d4: if(s.services.instruction)s.services.instruction(0x16e6d4u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x16e6d7u); s.set(3,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e6d9u); if(!s.flags.zf)goto L16e6e7;
L16e6db: if(s.services.instruction)s.services.instruction(0x16e6dbu); s.flags.minus(s.reg(7,4,0),r(s.memory,(0x20u+s.gp[2]),4),4);
if(s.services.instruction)s.services.instruction(0x16e6deu); if(s.flags.sf==s.flags.of)goto L16e75c;
if(s.services.instruction)s.services.instruction(0x16e6e0u); s.flags.minus(s.reg(7,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x16e6e3u); s.set(3,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e6e5u); if(s.flags.zf)goto L16e75c;
L16e6e7: if(s.services.instruction)s.services.instruction(0x16e6e7u); s.set(6,r(s.memory,(0x24u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e6eau); s.set(0,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e6edu); s.set(6,s.flags.product(s.reg(6,4,0),s.reg(3,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0x16e6f0u); s.set(1,r(s.memory,(0x48u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e6f3u); s.set(6,s.flags.plus(s.reg(6,4,0),s.reg(0,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e6f5u); s.set(0,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e6f7u); s.set(0,s.flags.right(s.reg(0,4,0),0x5u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x16e6fau); s.set(5,r(s.memory,(0x0u+s.gp[1]+s.gp[0]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e6fdu); s.set(0,(0x0u+s.gp[1]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x16e700u); s.set(1,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e702u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0x1fu),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e705u); s.set(7,0x1u,4,0);
if(s.services.instruction)s.services.instruction(0x16e70au); s.set(7,s.flags.shift(s.reg(7,4,0),s.reg(1,1,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e70cu); s.set(5,s.flags.logic((s.reg(5,4,0))|(s.reg(7,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e70eu); w(s.memory,s.observe,0x16e70eu,(0x0u+s.gp[0]),4,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x16e710u); s.set(7,r(s.memory,(0x3cu+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e713u); s.set(1,r(s.memory,(0x38u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e716u); s.set(7,s.flags.increment(s.reg(7,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x16e717u); s.flags.minus(s.reg(1,4,0),s.reg(3,4,0),4);
if(s.services.instruction)s.services.instruction(0x16e719u); s.set(0,(0x1u+s.gp[3]),4,0);
if(s.services.instruction)s.services.instruction(0x16e71cu); w(s.memory,s.observe,0x16e71cu,(0x3cu+s.gp[2]),4,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x16e71fu); w(s.memory,s.observe,0x16e71fu,(0x34u+s.gp[2]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x16e722u); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L16e727;
if(s.services.instruction)s.services.instruction(0x16e724u); w(s.memory,s.observe,0x16e724u,(0x38u+s.gp[2]),4,s.reg(0,4,0));
L16e727: if(s.services.instruction)s.services.instruction(0x16e727u); s.set(1,r(s.memory,(0x24u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e72au); s.set(5,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e72cu); s.set(1,s.flags.right(s.reg(1,4,0),0x2u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x16e72fu); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e731u); s.set(7,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e733u); while(s.gp[1]){const auto x=s.gp[0];w(s.memory,s.observe,0x16e733u,s.gp[7],4,x);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x16e733u);}
if(s.services.instruction)s.services.instruction(0x16e735u); s.set(1,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e737u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0x3u),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e73au); while(s.gp[1]){const auto x=s.gp[0];w(s.memory,s.observe,0x16e73au,s.gp[7],1,x);const auto delta=(s.other_flags&1024u)?4294967295u:1u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x16e73au);}
if(s.services.instruction)s.services.instruction(0x16e73cu); s.set(0,r(s.memory,(0x40u+s.gp[2]),2),2,0);
if(s.services.instruction)s.services.instruction(0x16e740u); w(s.memory,s.observe,0x16e740u,(0x0u+s.gp[6]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x16e743u); w(s.memory,s.observe,0x16e743u,(0x40u+s.gp[2]),2,s.flags.increment(r(s.memory,(0x40u+s.gp[2]),2),2,false));
if(s.services.instruction)s.services.instruction(0x16e747u); s.flags.minus(r(s.memory,(0x40u+s.gp[2]),2),0xffffffffu,2);
if(s.services.instruction)s.services.instruction(0x16e74cu); if(!s.flags.zf)goto L16e754;
if(s.services.instruction)s.services.instruction(0x16e74eu); w(s.memory,s.observe,0x16e74eu,(0x40u+s.gp[2]),2,0x8000u);
L16e754: if(s.services.instruction)s.services.instruction(0x16e754u); s.set(0,sign_extend(r(s.memory,(0x0u+s.gp[6]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x16e757u); s.set(0,s.flags.shift(s.reg(0,4,0),0x10u,4),4,0);
if(s.services.instruction)s.services.instruction(0x16e75au); s.set(0,s.flags.logic((s.reg(0,4,0))|(s.reg(3,4,0)),4),4,0);
L16e75c: if(s.services.instruction)s.services.instruction(0x16e75cu); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x16e75du); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x16e75eu); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x16e75fu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x16e760u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_178e30(MathState &s) {
if(s.services.instruction)s.services.instruction(0x178e30u); s.push(0x178e30u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x178e31u); s.push(0x178e31u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x178e32u); s.push(0x178e32u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x178e33u); s.set(7,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e37u); s.set(0,r(s.memory,(0x0u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e39u); s.set(0,s.flags.right(s.reg(0,4,0),0x2u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x178e3cu); s.set(6,s.flags.logic((s.reg(6,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e3fu); s.set(0,s.flags.logic((s.reg(0,1,0))&(0x1u),1),1,0);
if(s.services.instruction)s.services.instruction(0x178e41u); w(s.memory,s.observe,0x178e41u,(0x10u+s.gp[4]),1,s.reg(0,1,0));
if(s.services.instruction)s.services.instruction(0x178e45u); if(!s.flags.zf)goto L178e54;
if(s.services.instruction)s.services.instruction(0x178e47u); s.push(0x178e47u,0x178e4cu);s.required(0x178e47u,0x1790a0u,false);
if(s.services.instruction)s.services.instruction(0x178e4cu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x178e4eu); if(s.flags.zf)goto L17908d;
L178e54: if(s.services.instruction)s.services.instruction(0x178e54u); s.set(1,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e58u); s.set(0,r(s.memory,(0x4u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e5bu); s.push(0x178e5bu,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x178e5cu); s.push(0x178e5cu,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x178e5du); s.set(2,(0xcu+s.gp[7]),4,0);
if(s.services.instruction)s.services.instruction(0x178e60u); s.push(0x178e60u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x178e61u); s.push(0x178e61u,0x178e66u);s.required(0x178e61u,0x17afb0u,false);
if(s.services.instruction)s.services.instruction(0x178e66u); s.set(5,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x178e68u); s.flags.minus(s.reg(5,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x178e6bu); w(s.memory,s.observe,0x178e6bu,(0x14u+s.gp[4]),4,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x178e6fu); if(s.flags.zf)goto L179095;
if(s.services.instruction)s.services.instruction(0x178e75u); s.set(0,r(s.memory,(0x4f720cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e7au); s.set(1,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e7du); s.set(0,r(s.memory,(0x30u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e80u); s.set(6,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x178e82u); s.set(6,s.flags.logic((s.reg(6,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e88u); s.set(6,s.flags.product(s.reg(6,4,0),0x190u),4,0);
if(s.services.instruction)s.services.instruction(0x178e8eu); s.set(6,s.flags.plus(s.reg(6,4,0),s.reg(1,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e90u); s.set(1,r(s.memory,(0x4u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e93u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x178e99u); s.set(1,s.flags.shift(s.reg(1,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x178e9cu); s.push(0x178e9cu,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x178e9du); s.set(3,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x178ea3u); s.set(2,r(s.memory,(0x8u+s.gp[1]+s.gp[3]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x178ea7u); s.set(1,r(s.memory,(0x4f3218u),4),4,0);
if(s.services.instruction)s.services.instruction(0x178eadu); w(s.memory,s.observe,0x178eadu,(0xcu+s.gp[6]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x178eb0u); s.flags.minus(r(s.memory,(0x8u+s.gp[1]),4),0x2u,4);
if(s.services.instruction)s.services.instruction(0x178eb4u); w(s.memory,s.observe,0x178eb4u,(0x10u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x178eb8u); if(!s.flags.zf)goto L178f05;
if(s.services.instruction)s.services.instruction(0x178ebau); s.push(0x178ebau,0x178ebfu);s.required(0x178ebau,0x165b00u,false);
if(s.services.instruction)s.services.instruction(0x178ebfu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x178ec1u); if(s.flags.zf)goto L178f05;
if(s.services.instruction)s.services.instruction(0x178ec3u); s.set(0,r(s.memory,(0x8u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178ec6u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x178ec9u); if(s.flags.zf)goto L178f05;
if(s.services.instruction)s.services.instruction(0x178ecbu); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x178ed0u); s.set(2,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x178ed3u); s.set(0,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x178ed8u); s.set(1,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178edbu); s.set(2,r(s.memory,(0x8u+s.gp[1]+s.gp[2]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x178edfu); s.set(0,r(s.memory,(0x0u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178ee1u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x178ee6u); s.set(0,s.flags.shift(s.reg(0,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x178ee9u); s.set(1,r(s.memory,(0x8u+s.gp[0]+s.gp[3]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x178eedu); s.set(2,s.flags.logic((s.reg(2,4,0))^(s.reg(2,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x178eefu); s.set(2,r(s.memory,(0x2u+s.gp[1]),2),2,0);
if(s.services.instruction)s.services.instruction(0x178ef3u); s.set(2,s.flags.right(s.reg(2,4,0),0xdu,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x178ef6u); s.flags.logic((s.reg(2,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x178ef9u); if(s.flags.zf)goto L178f01;
if(s.services.instruction)s.services.instruction(0x178efbu); w(s.memory,s.observe,0x178efbu,(0x3u+s.gp[6]),1,s.flags.logic((r(s.memory,(0x3u+s.gp[6]),1))|(0x4u),1));
if(s.services.instruction)s.services.instruction(0x178effu); goto L178f05;
L178f01: if(s.services.instruction)s.services.instruction(0x178f01u); w(s.memory,s.observe,0x178f01u,(0x3u+s.gp[6]),1,s.flags.logic((r(s.memory,(0x3u+s.gp[6]),1))&(0xfbu),1));
L178f05: if(s.services.instruction)s.services.instruction(0x178f05u); s.flags.logic((r(s.memory,(0x0u+s.gp[7]),1))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x178f08u); if(s.flags.zf)goto L178f46;
if(s.services.instruction)s.services.instruction(0x178f0au); s.set(3,r(s.memory,(0x8u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f0du); s.push(0x178f0du,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x178f0eu); s.push(0x178f0eu,0x178f13u);s.required(0x178f0eu,0x169112u,false);
if(s.services.instruction)s.services.instruction(0x178f13u); s.set(5,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x178f15u); s.push(0x178f15u,0xffffffffu);
if(s.services.instruction)s.services.instruction(0x178f17u); s.set(0,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x178f19u); s.push(0x178f19u,0x178f1eu);s.required(0x178f19u,0xbcdf0u,false);
if(s.services.instruction)s.services.instruction(0x178f1eu); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x178f20u); if(s.flags.zf)goto L178f25;
if(s.services.instruction)s.services.instruction(0x178f22u); w(s.memory,s.observe,0x178f22u,(0x48u+s.gp[6]),4,s.reg(3,4,0));
L178f25: if(s.services.instruction)s.services.instruction(0x178f25u); s.flags.minus(s.reg(5,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x178f28u); if(s.flags.zf)goto L178f3c;
if(s.services.instruction)s.services.instruction(0x178f2au); s.set(0,r(s.memory,(0x8u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f2du); s.push(0x178f2du,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x178f2eu); s.set(1,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x178f30u); s.push(0x178f30u,0x178f35u);s.required(0x178f30u,0x169151u,false);
if(s.services.instruction)s.services.instruction(0x178f35u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x178f37u); if(s.flags.zf)goto L178f3c;
if(s.services.instruction)s.services.instruction(0x178f39u); w(s.memory,s.observe,0x178f39u,(0x58u+s.gp[6]),4,s.reg(5,4,0));
L178f3c: if(s.services.instruction)s.services.instruction(0x178f3cu); w(s.memory,s.observe,0x178f3cu,(0x2u+s.gp[6]),1,s.flags.logic((r(s.memory,(0x2u+s.gp[6]),1))|(0x80u),1));
if(s.services.instruction)s.services.instruction(0x178f40u); s.set(5,r(s.memory,(0x18u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f44u); goto L178f63;
L178f46: if(s.services.instruction)s.services.instruction(0x178f46u); w(s.memory,s.observe,0x178f46u,(0x48u+s.gp[6]),4,0xffffffffu);
if(s.services.instruction)s.services.instruction(0x178f4du); s.set(1,(0x24u+s.gp[7]),4,0);
if(s.services.instruction)s.services.instruction(0x178f50u); s.set(0,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f52u); s.set(2,(0x30u+s.gp[6]),4,0);
if(s.services.instruction)s.services.instruction(0x178f55u); w(s.memory,s.observe,0x178f55u,(0x0u+s.gp[2]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x178f57u); s.set(0,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f5au); w(s.memory,s.observe,0x178f5au,(0x4u+s.gp[2]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x178f5du); s.set(1,r(s.memory,(0x8u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f60u); w(s.memory,s.observe,0x178f60u,(0x8u+s.gp[2]),4,s.reg(1,4,0));
L178f63: if(s.services.instruction)s.services.instruction(0x178f63u); s.set(2,r(s.memory,(0x0u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f65u); s.set(2,s.flags.right(s.reg(2,4,0),0x1u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x178f67u); s.flags.logic((s.reg(2,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x178f6au); if(s.flags.zf)goto L178f8e;
if(s.services.instruction)s.services.instruction(0x178f6cu); w(s.memory,s.observe,0x178f6cu,(0x2u+s.gp[6]),1,s.flags.logic((r(s.memory,(0x2u+s.gp[6]),1))|(0x2u),1));
if(s.services.instruction)s.services.instruction(0x178f70u); s.set(0,r(s.memory,(0x34u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f73u); w(s.memory,s.observe,0x178f73u,(0x10u+s.gp[6]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x178f76u); s.set(1,r(s.memory,(0x38u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f79u); w(s.memory,s.observe,0x178f79u,(0x14u+s.gp[6]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x178f7cu); s.set(2,r(s.memory,(0x3cu+s.gp[7]),2),2,0);
if(s.services.instruction)s.services.instruction(0x178f80u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f82u); w(s.memory,s.observe,0x178f82u,(0x18u+s.gp[6]),2,s.reg(2,2,0));
if(s.services.instruction)s.services.instruction(0x178f86u); w(s.memory,s.observe,0x178f86u,(0x40u+s.gp[6]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x178f89u); w(s.memory,s.observe,0x178f89u,(0x44u+s.gp[6]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x178f8cu); goto L178fa4;
L178f8e: if(s.services.instruction)s.services.instruction(0x178f8eu); s.set(0,r(s.memory,(0x44u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f91u); s.set(2,r(s.memory,(0x4cu+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f94u); s.set(1,r(s.memory,(0x48u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178f97u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x40u+s.gp[7]),4)));
if(s.services.instruction)s.services.instruction(0x178f9cu); s.push(0x178f9cu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x178f9du); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x178f9fu); s.push(0x178f9fu,0x178fa4u);s.required(0x178f9fu,0x17b0d0u,false);
L178fa4: if(s.services.instruction)s.services.instruction(0x178fa4u); s.set(0,r(s.memory,(0x48u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178fa7u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x178faau); if(s.flags.zf)goto L178fc3;
if(s.services.instruction)s.services.instruction(0x178facu); s.set(1,r(s.memory,(0x51df7cu),1),1,0);
if(s.services.instruction)s.services.instruction(0x178fb2u); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x178fb4u); if(s.flags.zf)goto L178fc3;
if(s.services.instruction)s.services.instruction(0x178fb6u); s.push(0x178fb6u,0x178fbbu);s.required(0x178fb6u,0x178de0u,false);
if(s.services.instruction)s.services.instruction(0x178fbbu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x178fbdu); if(s.flags.zf)goto L178fc3;
if(s.services.instruction)s.services.instruction(0x178fbfu); w(s.memory,s.observe,0x178fbfu,(0x2u+s.gp[6]),1,s.flags.logic((r(s.memory,(0x2u+s.gp[6]),1))|(0x20u),1));
L178fc3: if(s.services.instruction)s.services.instruction(0x178fc3u); s.set(1,(0x7cu+s.gp[6]),4,0);
if(s.services.instruction)s.services.instruction(0x178fc6u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x178fc9u); s.push(0x178fc9u,0x178fceu);s.required(0x178fc9u,0x17e5e0u,false);
if(s.services.instruction)s.services.instruction(0x178fceu); s.set(2,r(s.memory,(0x20u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178fd1u); s.set(0,r(s.memory,(0x1cu+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178fd4u); s.set(1,sign_extend(r(s.memory,(0x18u+s.gp[7]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x178fd8u); s.push(0x178fd8u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x178fd9u); s.set(2,r(s.memory,(0x58u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178fdcu); s.push(0x178fdcu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x178fddu); s.set(0,r(s.memory,(0x48u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178fe0u); s.push(0x178fe0u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x178fe1u); s.push(0x178fe1u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x178fe2u); s.set(0,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x178fe4u); s.push(0x178fe4u,0x178fe9u);s.required(0x178fe4u,0x17b1f0u,false);
if(s.services.instruction)s.services.instruction(0x178fe9u); s.flags.logic((r(s.memory,(0x0u+s.gp[7]),1))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x178fecu); if(!s.flags.zf)goto L178ff3;
if(s.services.instruction)s.services.instruction(0x178feeu); s.push(0x178feeu,0x178ff3u);s.required(0x178feeu,0x17b880u,false);
L178ff3: if(s.services.instruction)s.services.instruction(0x178ff3u); s.set(3,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178ff7u); s.set(0,r(s.memory,(0x0u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178ff9u); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x178ffbu); s.set(1,s.flags.right(s.reg(1,4,0),0x3u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x178ffeu); s.flags.logic((s.reg(1,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x179001u); if(!s.flags.zf)goto L17900a;
if(s.services.instruction)s.services.instruction(0x179003u); s.set(0,s.flags.right(s.reg(0,4,0),0x4u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x179006u); s.flags.logic((s.reg(0,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x179008u); if(s.flags.zf)goto L179017;
L17900a: if(s.services.instruction)s.services.instruction(0x17900au); s.set(0,r(s.memory,(0x8u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17900du); s.push(0x17900du,0x1u);
if(s.services.instruction)s.services.instruction(0x17900fu); s.push(0x17900fu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x179010u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x179012u); s.push(0x179012u,0x179017u);s.required(0x179012u,0x17e460u,false);
L179017: if(s.services.instruction)s.services.instruction(0x179017u); s.set(0,r(s.memory,(0x50u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17901au); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x17901cu); if(s.flags.zf)goto L179029;
if(s.services.instruction)s.services.instruction(0x17901eu); s.set(2,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x179020u); w(s.memory,s.observe,0x179020u,(0x74u+s.gp[6]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x179023u); s.set(0,r(s.memory,(0x4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x179026u); w(s.memory,s.observe,0x179026u,(0x78u+s.gp[6]),4,s.reg(0,4,0));
L179029: if(s.services.instruction)s.services.instruction(0x179029u); s.push(0x179029u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x17902au); s.push(0x17902au,0x17902fu);s.required(0x17902au,0x17a1a0u,false);
if(s.services.instruction)s.services.instruction(0x17902fu); s.set(2,r(s.memory,(0x0u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x179031u); s.set(1,r(s.memory,(0x5cu+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x179034u); s.set(2,s.flags.right(s.reg(2,4,0),0x1u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x179036u); s.flags.logic((s.reg(2,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x179039u); w(s.memory,s.observe,0x179039u,(0x51df78u),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x17903fu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x179040u); if(s.flags.zf)goto L17905f;
if(s.services.instruction)s.services.instruction(0x179042u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x179044u); s.set(0,r(s.memory,(0x2u+s.gp[6]),2),2,0);
if(s.services.instruction)s.services.instruction(0x179048u); s.set(1,s.reg(0,1,0),1,0);
if(s.services.instruction)s.services.instruction(0x17904au); s.set(1,s.flags.right(s.reg(1,1,0),0x1u,1,false),1,0);
if(s.services.instruction)s.services.instruction(0x17904cu); s.flags.logic((s.reg(1,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x17904fu); if(!s.flags.zf)goto L17905f;
if(s.services.instruction)s.services.instruction(0x179051u); s.set(0,s.flags.right(s.reg(0,4,0),0x9u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x179054u); s.flags.logic((s.reg(0,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x179056u); if(!s.flags.zf)goto L17905f;
if(s.services.instruction)s.services.instruction(0x179058u); s.push(0x179058u,0x3f800000u);
if(s.services.instruction)s.services.instruction(0x17905du); goto L179061;
L17905f: if(s.services.instruction)s.services.instruction(0x17905fu); s.push(0x17905fu,0x0u);
L179061: if(s.services.instruction)s.services.instruction(0x179061u); s.set(0,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x179063u); s.push(0x179063u,0x179068u);s.required(0x179063u,0x17c6e0u,false);
if(s.services.instruction)s.services.instruction(0x179068u); s.set(6,r(s.memory,(0x4f720cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17906eu); s.set(0,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x179070u); w(s.memory,s.observe,0x179070u,(0x51df78u),4,0x0u);
if(s.services.instruction)s.services.instruction(0x17907au); s.push(0x17907au,0x17907fu);s.required(0x17907au,0x16e940u,false);
if(s.services.instruction)s.services.instruction(0x17907fu); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x179081u); if(!s.flags.zf)goto L179095;
if(s.services.instruction)s.services.instruction(0x179083u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x179084u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x179085u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x179088u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x179089u); s.set(1,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17908au); s.pop();s.gp[4]+=4u;return;
L17908d: if(s.services.instruction)s.services.instruction(0x17908du); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17908eu); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x179090u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x179091u); s.set(1,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x179092u); s.pop();s.gp[4]+=4u;return;
L179095: if(s.services.instruction)s.services.instruction(0x179095u); s.set(0,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x179097u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x179098u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x179099u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17909au); s.set(1,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17909bu); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_17a0f0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x17a0f0u); s.flags.minus(s.reg(7,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a0f3u); if(s.flags.zf)goto L17a192;
if(s.services.instruction)s.services.instruction(0x17a0f9u); s.set(1,r(s.memory,(0x4f720cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a0ffu); s.set(2,r(s.memory,(0x38u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a102u); s.set(0,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a104u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a109u); s.flags.minus(s.reg(0,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a10bu); if(s.flags.sf==s.flags.of)goto L17a192;
if(s.services.instruction)s.services.instruction(0x17a111u); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a114u); s.push(0x17a114u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17a115u); s.set(6,r(s.memory,(0x24u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a118u); s.set(6,s.flags.product(s.reg(6,4,0),s.reg(0,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0x17a11bu); s.set(0,r(s.memory,(0x0u+s.gp[6]+s.gp[2]*1u),2),2,0);
if(s.services.instruction)s.services.instruction(0x17a11fu); s.set(6,s.flags.plus(s.reg(6,4,0),s.reg(2,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a121u); s.flags.logic((s.reg(0,2,0))&(s.reg(0,2,0)),2);
if(s.services.instruction)s.services.instruction(0x17a124u); if(s.flags.zf)goto L17a191;
if(s.services.instruction)s.services.instruction(0x17a126u); s.set(1,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a128u); s.set(2,sign_extend(s.reg(0,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x17a12bu); s.set(1,s.flags.right(s.reg(1,4,0),0x10u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x17a12eu); s.flags.minus(s.reg(2,4,0),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a130u); if(!s.flags.zf)goto L17a191;
if(s.services.instruction)s.services.instruction(0x17a132u); s.set(0,r(s.memory,(0x8u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a135u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a138u); if(s.flags.zf)goto L17a15b;
if(s.services.instruction)s.services.instruction(0x17a13au); s.set(1,r(s.memory,(0x4f9b5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a140u); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a143u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a148u); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x17a14bu); w(s.memory,s.observe,0x17a14bu,(0x4u+s.gp[2]+s.gp[0]*8u),1,s.flags.logic((r(s.memory,(0x4u+s.gp[2]+s.gp[0]*8u),1))|(0x2u),1));
if(s.services.instruction)s.services.instruction(0x17a150u); s.set(0,(0x4u+s.gp[2]+s.gp[0]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x17a154u); w(s.memory,s.observe,0x17a154u,(0x8u+s.gp[6]),4,0xffffffffu);
L17a15b: if(s.services.instruction)s.services.instruction(0x17a15bu); s.push(0x17a15bu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17a15cu); s.push(0x17a15cu,0x17a161u);s.required(0x17a15cu,0x17a030u,false);
if(s.services.instruction)s.services.instruction(0x17a161u); s.set(0,r(s.memory,(0x2u+s.gp[6]),1),1,0);
if(s.services.instruction)s.services.instruction(0x17a164u); s.set(0,s.flags.right(s.reg(0,1,0),0x1u,1,false),1,0);
if(s.services.instruction)s.services.instruction(0x17a166u); s.flags.logic((s.reg(0,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x17a168u); if(s.flags.zf)goto L17a18a;
if(s.services.instruction)s.services.instruction(0x17a16au); s.set(0,r(s.memory,(0x8u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x17a16eu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x17a170u); if(s.flags.zf)goto L17a17e;
if(s.services.instruction)s.services.instruction(0x17a172u); w(s.memory,s.observe,0x17a172u,(0x2u+s.gp[6]),1,s.flags.logic((r(s.memory,(0x2u+s.gp[6]),1))|(0x10u),1));
if(s.services.instruction)s.services.instruction(0x17a176u); w(s.memory,s.observe,0x17a176u,(0x2u+s.gp[6]),1,s.flags.logic((r(s.memory,(0x2u+s.gp[6]),1))|(0x4u),1));
if(s.services.instruction)s.services.instruction(0x17a17au); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a17bu); s.pop();s.gp[4]+=4u;return;
L17a17e: if(s.services.instruction)s.services.instruction(0x17a17eu); w(s.memory,s.observe,0x17a17eu,(0x2u+s.gp[6]),1,s.flags.logic((r(s.memory,(0x2u+s.gp[6]),1))&(0xefu),1));
if(s.services.instruction)s.services.instruction(0x17a182u); w(s.memory,s.observe,0x17a182u,(0x2u+s.gp[6]),1,s.flags.logic((r(s.memory,(0x2u+s.gp[6]),1))|(0x4u),1));
if(s.services.instruction)s.services.instruction(0x17a186u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a187u); s.pop();s.gp[4]+=4u;return;
L17a18a: if(s.services.instruction)s.services.instruction(0x17a18au); s.set(0,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a18cu); s.push(0x17a18cu,0x17a191u);s.required(0x17a18cu,0x17a4a0u,false);
L17a191: if(s.services.instruction)s.services.instruction(0x17a191u); s.set(6,s.pop(),4,0);
L17a192: if(s.services.instruction)s.services.instruction(0x17a192u); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_17dec0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x17dec0u); s.push(0x17dec0u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x17dec1u); s.push(0x17dec1u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x17dec2u); s.push(0x17dec2u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x17dec3u); s.set(5,r(s.memory,(0x14u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17dec7u); s.push(0x17dec7u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17dec8u); s.set(6,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17decau); s.set(1,s.flags.logic((s.reg(1,4,0))^(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x17deccu); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17decfu); s.push(0x17decfu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x17ded0u); s.set(3,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17ded2u); if(s.flags.zf)goto L17df06;
if(s.services.instruction)s.services.instruction(0x17ded4u); s.set(7,r(s.memory,(0x4f720cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17dedau); s.set(0,r(s.memory,(0x38u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17deddu); s.set(2,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17dedfu); s.set(2,s.flags.logic((s.reg(2,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17dee5u); s.flags.minus(s.reg(2,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x17dee7u); if(s.flags.sf==s.flags.of)goto L17df06;
if(s.services.instruction)s.services.instruction(0x17dee9u); s.set(0,r(s.memory,(0x24u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17deecu); s.set(0,s.flags.product(s.reg(0,4,0),s.reg(2,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0x17deefu); s.set(0,s.flags.plus(s.reg(0,4,0),r(s.memory,(0x44u+s.gp[7]),4),4),4,0);
if(s.services.instruction)s.services.instruction(0x17def2u); s.set(2,r(s.memory,(0x0u+s.gp[0]),2),2,0);
if(s.services.instruction)s.services.instruction(0x17def5u); s.flags.logic((s.reg(2,2,0))&(s.reg(2,2,0)),2);
if(s.services.instruction)s.services.instruction(0x17def8u); if(s.flags.zf)goto L17df06;
if(s.services.instruction)s.services.instruction(0x17defau); s.set(2,sign_extend(s.reg(2,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x17defdu); s.set(6,s.flags.right(s.reg(6,4,0),0x10u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x17df00u); s.flags.minus(s.reg(2,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x17df02u); if(!s.flags.zf)goto L17df06;
if(s.services.instruction)s.services.instruction(0x17df04u); s.set(1,s.reg(0,4,0),4,0);
L17df06: if(s.services.instruction)s.services.instruction(0x17df06u); s.set(0,s.flags.logic((s.reg(0,1,0))^(s.reg(0,1,0)),1),1,0);
if(s.services.instruction)s.services.instruction(0x17df08u); s.flags.logic((s.reg(1,4,0))&(s.reg(1,4,0)),4);
if(s.services.instruction)s.services.instruction(0x17df0au); if(s.flags.zf)goto L17dfe8;
if(s.services.instruction)s.services.instruction(0x17df10u); s.set(0,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df13u); s.set(2,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df19u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df1eu); s.set(0,s.flags.shift(s.reg(0,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17df21u); s.set(2,r(s.memory,(0x8u+s.gp[0]+s.gp[2]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df25u); w(s.memory,s.observe,0x17df25u,(0x3u+s.gp[1]),1,s.flags.logic((r(s.memory,(0x3u+s.gp[1]),1))|(0x2u),1));
if(s.services.instruction)s.services.instruction(0x17df29u); s.flags.logic((s.reg(3,4,0))&(s.reg(3,4,0)),4);
if(s.services.instruction)s.services.instruction(0x17df2bu); w(s.memory,s.observe,0x17df2bu,(0x10u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x17df2fu); if(s.flags.zf)goto L17df44;
if(s.services.instruction)s.services.instruction(0x17df31u); s.set(6,r(s.memory,(0x0u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df33u); s.set(0,(0x30u+s.gp[1]),4,0);
if(s.services.instruction)s.services.instruction(0x17df36u); w(s.memory,s.observe,0x17df36u,(0x0u+s.gp[0]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17df38u); s.set(6,r(s.memory,(0x4u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df3bu); w(s.memory,s.observe,0x17df3bu,(0x4u+s.gp[0]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17df3eu); s.set(6,r(s.memory,(0x8u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df41u); w(s.memory,s.observe,0x17df41u,(0x8u+s.gp[0]),4,s.reg(6,4,0));
L17df44: if(s.services.instruction)s.services.instruction(0x17df44u); s.set(0,r(s.memory,(0x20u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df48u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x17df4au); if(s.flags.zf)goto L17df57;
if(s.services.instruction)s.services.instruction(0x17df4cu); s.set(6,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df4eu); w(s.memory,s.observe,0x17df4eu,(0x74u+s.gp[1]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17df51u); s.set(0,r(s.memory,(0x4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df54u); w(s.memory,s.observe,0x17df54u,(0x78u+s.gp[1]),4,s.reg(0,4,0));
L17df57: if(s.services.instruction)s.services.instruction(0x17df57u); s.flags.logic((s.reg(5,4,0))&(s.reg(5,4,0)),4);
if(s.services.instruction)s.services.instruction(0x17df59u); w(s.memory,s.observe,0x17df59u,(0x6cu+s.gp[1]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x17df5eu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x18u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x17df64u); w(s.memory,s.observe,0x17df64u,(0x70u+s.gp[1]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x17df69u); if(s.flags.zf)goto L17dfe6;
if(s.services.instruction)s.services.instruction(0x17df6bu); s.set(0,r(s.memory,(0xcu+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df6eu); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x17df70u); w(s.memory,s.observe,0x17df70u,(0x1cu+s.gp[4]),4,0x0u);
if(s.services.instruction)s.services.instruction(0x17df78u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L17dfe6;
if(s.services.instruction)s.services.instruction(0x17df7au); s.set(3,r(s.memory,(0x4f7208u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df80u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x17df88u); s.set(1,s.flags.plus(s.reg(1,4,0),0x7cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x17df8bu); w(s.memory,s.observe,0x17df8bu,(0x20u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x17df8fu); ;
L17df90: if(s.services.instruction)s.services.instruction(0x17df90u); s.set(0,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17df92u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17df95u); if(s.flags.zf)goto L17dfcb;
if(s.services.instruction)s.services.instruction(0x17df97u); goto L17dfa0;
L17dfa0: if(s.services.instruction)s.services.instruction(0x17dfa0u); s.set(2,r(s.memory,(0x44u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17dfa3u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17dfa8u); s.set(0,s.flags.product(s.reg(0,4,0),0x3cu),4,0);
if(s.services.instruction)s.services.instruction(0x17dfabu); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(2,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x17dfadu); s.set(2,(0x8u+s.gp[0]),4,0);
if(s.services.instruction)s.services.instruction(0x17dfb0u); s.set(0,r(s.memory,(0x4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17dfb3u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17dfb6u); s.set(1,0xdu,4,0);
if(s.services.instruction)s.services.instruction(0x17dfbbu); s.set(6,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17dfbdu); s.set(7,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17dfbfu); while(s.gp[1]){const auto x=r(s.memory,s.gp[6],4);w(s.memory,s.observe,0x17dfbfu,s.gp[7],4,x);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;s.gp[6]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x17dfbfu);}
if(s.services.instruction)s.services.instruction(0x17dfc1u); w(s.memory,s.observe,0x17dfc1u,(0x0u+s.gp[2]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x17dfc5u); if(!s.flags.zf)goto L17dfa0;
if(s.services.instruction)s.services.instruction(0x17dfc7u); s.set(2,r(s.memory,(0x10u+s.gp[4]),4),4,0);
L17dfcb: if(s.services.instruction)s.services.instruction(0x17dfcbu); s.set(0,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17dfcfu); s.set(1,r(s.memory,(0x20u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17dfd3u); s.set(6,r(s.memory,(0xcu+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17dfd6u); s.set(0,s.flags.increment(s.reg(0,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x17dfd7u); s.set(1,s.flags.plus(s.reg(1,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17dfdau); s.flags.minus(s.reg(0,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x17dfdcu); w(s.memory,s.observe,0x17dfdcu,(0x1cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x17dfe0u); w(s.memory,s.observe,0x17dfe0u,(0x20u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x17dfe4u); if(s.flags.sf!=s.flags.of)goto L17df90;
L17dfe6: if(s.services.instruction)s.services.instruction(0x17dfe6u); s.set(0,0x1u,1,0);
L17dfe8: if(s.services.instruction)s.services.instruction(0x17dfe8u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17dfe9u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17dfeau); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17dfebu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17dfecu); s.set(1,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17dfedu); s.pop();s.gp[4]+=12u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_18bbc0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x18bbc0u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x46a0dcu),4)));
if(s.services.instruction)s.services.instruction(0x18bbc8u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x18bbd0u); s.set(4,s.flags.minus(s.reg(4,4,0),0x10u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18bbd3u); s.flags.scalar_compare(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4))),false);
if(s.services.instruction)s.services.instruction(0x18bbd7u); if(s.flags.cf||s.flags.zf)goto L18bbde;
if(s.services.instruction)s.services.instruction(0x18bbd9u); s.xmm[1]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x18bbdcu); goto L18bbf0;
L18bbde: if(s.services.instruction)s.services.instruction(0x18bbdeu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x18bbe3u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x18bbe6u); s.xmm[1]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x18bbe9u); if(!s.flags.cf&&!s.flags.zf)goto L18bbf0;
if(s.services.instruction)s.services.instruction(0x18bbebu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4)));
L18bbf0: if(s.services.instruction)s.services.instruction(0x18bbf0u); s.flags.scalar_compare(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4))),false);
if(s.services.instruction)s.services.instruction(0x18bbf4u); if(s.flags.cf||s.flags.zf)goto L18bbfb;
if(s.services.instruction)s.services.instruction(0x18bbf6u); s.xmm[2]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x18bbf9u); goto L18bc0d;
L18bbfb: if(s.services.instruction)s.services.instruction(0x18bbfbu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x18bc00u); s.flags.scalar_compare(s.xmm[2],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x18bc03u); s.xmm[2]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x18bc06u); if(!s.flags.cf&&!s.flags.zf)goto L18bc0d;
if(s.services.instruction)s.services.instruction(0x18bc08u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
L18bc0d: if(s.services.instruction)s.services.instruction(0x18bc0du); s.flags.scalar_compare(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4))),false);
if(s.services.instruction)s.services.instruction(0x18bc10u); if(s.flags.cf||s.flags.zf)goto L18bc17;
if(s.services.instruction)s.services.instruction(0x18bc12u); s.xmm[0]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x18bc15u); goto L18bc23;
L18bc17: if(s.services.instruction)s.services.instruction(0x18bc17u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x18bc1bu); s.flags.scalar_compare(s.xmm[4],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x18bc1eu); if(!s.flags.cf&&!s.flags.zf)goto L18bc23;
if(s.services.instruction)s.services.instruction(0x18bc20u); s.xmm[0]=s.xmm[4];
L18bc23: if(s.services.instruction)s.services.instruction(0x18bc23u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x18bc27u); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x46a338u),4))));
if(s.services.instruction)s.services.instruction(0x18bc2fu); w(s.memory,s.observe,0x18bc2fu,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x18bc34u); s.fp.push(ext(f(r(s.memory,(0x0u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x18bc37u); w(s.memory,s.observe,0x18bc37u,(0x4u+s.gp[4]),4,integer_spill_observed(s));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x18bc3bu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x46a33cu),4)));
if(s.services.instruction)s.services.instruction(0x18bc43u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x18bc47u); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x18bc4bu); w(s.memory,s.observe,0x18bc4bu,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x18bc50u); s.fp.push(ext(f(r(s.memory,(0x0u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x18bc53u); w(s.memory,s.observe,0x18bc53u,(0x8u+s.gp[4]),4,integer_spill_observed(s));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x18bc57u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x18bc5bu); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x18bc5fu); w(s.memory,s.observe,0x18bc5fu,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x18bc64u); s.fp.push(ext(f(r(s.memory,(0x0u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x18bc67u); w(s.memory,s.observe,0x18bc67u,(0xcu+s.gp[4]),4,integer_spill_observed(s));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x18bc6bu); s.set(0,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18bc6fu); s.set(1,r(s.memory,(0x8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18bc73u); s.set(0,s.flags.shift(s.reg(0,4,0),0xbu,4),4,0);
if(s.services.instruction)s.services.instruction(0x18bc76u); s.set(0,s.flags.logic((s.reg(0,4,0))|(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18bc78u); s.set(1,r(s.memory,(0xcu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18bc7cu); s.set(0,s.flags.shift(s.reg(0,4,0),0xbu,4),4,0);
if(s.services.instruction)s.services.instruction(0x18bc7fu); s.set(0,s.flags.logic((s.reg(0,4,0))|(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18bc81u); s.set(4,s.flags.plus(s.reg(4,4,0),0x10u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18bc84u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_18c440(MathState &s) {
if(s.services.instruction)s.services.instruction(0x18c440u); s.set(4,s.flags.minus(s.reg(4,4,0),0x58u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18c443u); s.push(0x18c443u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x18c444u); s.push(0x18c444u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x18c445u); s.set(5,r(s.memory,(0x64u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18c449u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x18c44eu); s.push(0x18c44eu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18c44fu); s.push(0x18c44fu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x18c450u); s.set(3,s.flags.logic((s.reg(3,4,0))^(s.reg(3,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18c452u); s.set(6,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18c454u); s.set(0,r(s.memory,(0x0u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18c457u); s.set(1,0x9u,4,0);
if(s.services.instruction)s.services.instruction(0x18c45cu); s.set(7,(0x1cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x18c460u); w(s.memory,s.observe,0x18c460u,(0x12u+s.gp[4]),1,s.reg(3,1,0));
if(s.services.instruction)s.services.instruction(0x18c464u); w(s.memory,s.observe,0x18c464u,(0x10u+s.gp[4]),2,s.reg(3,2,0));
if(s.services.instruction)s.services.instruction(0x18c469u); while(s.gp[1]){const auto x=r(s.memory,s.gp[6],4);w(s.memory,s.observe,0x18c469u,s.gp[7],4,x);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;s.gp[6]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x18c469u);}
if(s.services.instruction)s.services.instruction(0x18c46bu); w(s.memory,s.observe,0x18c46bu,(0x14u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x18c471u); w(s.memory,s.observe,0x18c471u,(0x13u+s.gp[4]),1,0x11u);
if(s.services.instruction)s.services.instruction(0x18c476u); w(s.memory,s.observe,0x18c476u,(0x18u+s.gp[4]),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x18c47au); w(s.memory,s.observe,0x18c47au,(0x58u+s.gp[4]),4,0xffffffffu);
if(s.services.instruction)s.services.instruction(0x18c482u); s.push(0x18c482u,0x18c487u);run_1903a0(s);
if(s.services.instruction)s.services.instruction(0x18c487u); w(s.memory,s.observe,0x18c487u,(0x54u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x18c48bu); s.set(0,r(s.memory,(0x4u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18c48eu); s.push(0x18c48eu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x18c48fu); s.set(0,(0x14u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x18c493u); w(s.memory,s.observe,0x18c493u,(0x64u+s.gp[4]),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x18c497u); w(s.memory,s.observe,0x18c497u,(0x60u+s.gp[4]),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x18c49bu); w(s.memory,s.observe,0x18c49bu,(0x68u+s.gp[4]),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x18c49fu); s.push(0x18c49fu,0x18c4a4u);run_18cdd0(s);
if(s.services.instruction)s.services.instruction(0x18c4a4u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18c4a5u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18c4a6u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18c4a7u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18c4a8u); s.set(4,s.flags.plus(s.reg(4,4,0),0x58u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18c4abu); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_18cdd0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x18cdd0u); s.set(1,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cdd4u); s.set(4,s.flags.minus(s.reg(4,4,0),0xa8u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18cddau); s.push(0x18cddau,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x18cddbu); s.set(3,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18cdddu); s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cde0u); s.flags.minus(s.reg(1,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18cde3u); if(s.flags.zf)goto L18cf9c;
if(s.services.instruction)s.services.instruction(0x18cde9u); s.set(0,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cdeeu); s.set(2,r(s.memory,(0x4cu+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cdf1u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cdf7u); s.set(1,s.flags.shift(s.reg(1,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18cdfau); s.set(1,r(s.memory,(0x8u+s.gp[1]+s.gp[0]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cdfeu); w(s.memory,s.observe,0x18cdfeu,(0x4u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x18ce02u); s.set(1,r(s.memory,(0x3u+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x18ce05u); s.set(1,s.flags.logic((s.reg(1,1,0))&(0xfu),1),1,0);
if(s.services.instruction)s.services.instruction(0x18ce08u); s.set(1,s.flags.minus(0,s.reg(1,1,0),1),1,0);
if(s.services.instruction)s.services.instruction(0x18ce0au); s.set(0,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18ce0cu); s.push(0x18ce0cu,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x18ce0du); s.set(5,r(s.memory,(0x48u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce10u); s.set(1,s.flags.borrow(s.reg(1,4,0),s.reg(1,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce12u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0x2u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce15u); s.set(1,s.flags.increment(s.reg(1,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x18ce16u); s.set(0,s.flags.minus(0,s.reg(0,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce18u); s.set(0,s.flags.borrow(s.reg(0,4,0),s.reg(0,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce1au); s.set(0,s.flags.logic((s.reg(0,4,0))&(0x20u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce1du); s.set(0,s.flags.plus(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce20u); s.flags.minus(s.reg(5,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18ce23u); if(s.flags.zf)goto L18ce2a;
if(s.services.instruction)s.services.instruction(0x18ce25u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0x8u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce28u); goto L18ce2d;
L18ce2a: if(s.services.instruction)s.services.instruction(0x18ce2au); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xfffffff7u),4),4,0);
L18ce2d: if(s.services.instruction)s.services.instruction(0x18ce2du); s.push(0x18ce2du,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18ce2eu); s.flags.minus(r(s.memory,(0x44u+s.gp[3]),4),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18ce32u); if(s.flags.zf)goto L18ce3b;
if(s.services.instruction)s.services.instruction(0x18ce34u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0x100u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce39u); goto L18ce40;
L18ce3b: if(s.services.instruction)s.services.instruction(0x18ce3bu); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xfffffeffu),4),4,0);
L18ce40: if(s.services.instruction)s.services.instruction(0x18ce40u); s.push(0x18ce40u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x18ce41u); w(s.memory,s.observe,0x18ce41u,(0x1cu+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x18ce45u); w(s.memory,s.observe,0x18ce45u,(0x18u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x18ce49u); s.set(0,r(s.memory,(0x44u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce4cu); s.set(1,0x11u,4,0);
if(s.services.instruction)s.services.instruction(0x18ce51u); s.set(6,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18ce53u); s.set(7,(0x24u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x18ce57u); while(s.gp[1]){const auto x=r(s.memory,s.gp[6],4);w(s.memory,s.observe,0x18ce57u,s.gp[7],4,x);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;s.gp[6]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x18ce57u);}
if(s.services.instruction)s.services.instruction(0x18ce59u); w(s.memory,s.observe,0x18ce59u,(0x68u+s.gp[4]),4,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x18ce5du); s.set(5,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce61u); w(s.memory,s.observe,0x18ce61u,(0x70u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18ce65u); s.set(2,sign_extend(r(s.memory,(0x2u+s.gp[5]),1),1),2,0);
if(s.services.instruction)s.services.instruction(0x18ce6au); w(s.memory,s.observe,0x18ce6au,(0xb0u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x18ce71u); s.push(0x18ce71u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18ce72u); s.push(0x18ce72u,0x18ce77u);s.required(0x18ce72u,0x228220u,false);
if(s.services.instruction)s.services.instruction(0x18ce77u); s.set(0,r(s.memory,(0xau+s.gp[0]),1),1,0);
if(s.services.instruction)s.services.instruction(0x18ce7au); s.set(0,s.flags.right(s.reg(0,1,0),0x7u,1,false),1,0);
if(s.services.instruction)s.services.instruction(0x18ce7du); s.flags.logic((s.reg(0,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x18ce7fu); if(s.flags.zf)goto L18ce8c;
if(s.services.instruction)s.services.instruction(0x18ce81u); s.flags.logic((r(s.memory,(0x1u+s.gp[5]),1))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x18ce85u); if(!s.flags.zf)goto L18ce8c;
if(s.services.instruction)s.services.instruction(0x18ce87u); w(s.memory,s.observe,0x18ce87u,(0x25u+s.gp[4]),1,s.flags.logic((r(s.memory,(0x25u+s.gp[4]),1))|(0x8u),1));
L18ce8c: if(s.services.instruction)s.services.instruction(0x18ce8cu); s.set(6,r(s.memory,(0x50u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ce8fu); s.flags.logic((s.reg(6,4,0))&(s.reg(6,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18ce91u); if(s.flags.zf)goto L18ceaa;
if(s.services.instruction)s.services.instruction(0x18ce93u); s.set(1,0xau,4,0);
if(s.services.instruction)s.services.instruction(0x18ce98u); s.set(7,(0x74u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x18ce9cu); while(s.gp[1]){const auto x=r(s.memory,s.gp[6],4);w(s.memory,s.observe,0x18ce9cu,s.gp[7],4,x);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;s.gp[6]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x18ce9cu);}
if(s.services.instruction)s.services.instruction(0x18ce9eu); w(s.memory,s.observe,0x18ce9eu,(0xa4u+s.gp[4]),2,0x28u);
if(s.services.instruction)s.services.instruction(0x18cea8u); goto L18ceb4;
L18ceaa: if(s.services.instruction)s.services.instruction(0x18ceaau); w(s.memory,s.observe,0x18ceaau,(0xa4u+s.gp[4]),2,0x0u);
L18ceb4: if(s.services.instruction)s.services.instruction(0x18ceb4u); s.set(0,r(s.memory,(0x54u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ceb7u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18ceb9u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18cebau); if(s.flags.zf)goto L18cef0;
if(s.services.instruction)s.services.instruction(0x18cebcu); s.flags.minus(r(s.memory,(0x1u+s.gp[0]),1),0xffu,1);
if(s.services.instruction)s.services.instruction(0x18cec0u); if(s.flags.zf)goto L18cef0;
if(s.services.instruction)s.services.instruction(0x18cec2u); s.flags.minus(r(s.memory,(0x0u+s.gp[0]),1),0xffu,1);
if(s.services.instruction)s.services.instruction(0x18cec5u); if(s.flags.zf)goto L18cef0;
if(s.services.instruction)s.services.instruction(0x18cec7u); s.set(3,r(s.memory,(0x54u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cecau); s.set(0,r(s.memory,(0x14u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ceceu); s.set(1,sign_extend(r(s.memory,(0x0u+s.gp[3]),1),1),2,0);
if(s.services.instruction)s.services.instruction(0x18ced2u); s.set(2,sign_extend(r(s.memory,(0x1u+s.gp[3]),1),1),2,0);
if(s.services.instruction)s.services.instruction(0x18ced7u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0x400u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cedcu); w(s.memory,s.observe,0x18cedcu,(0x14u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x18cee0u); w(s.memory,s.observe,0x18cee0u,(0xb0u+s.gp[4]),2,s.reg(1,2,0));
if(s.services.instruction)s.services.instruction(0x18cee8u); w(s.memory,s.observe,0x18cee8u,(0xb2u+s.gp[4]),2,s.reg(2,2,0));
L18cef0: if(s.services.instruction)s.services.instruction(0x18cef0u); s.set(0,r(s.memory,(0xb8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cef7u); s.push(0x18cef7u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x18cef8u); s.set(0,(0x18u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x18cefcu); s.push(0x18cefcu,0x18cf01u);s.required(0x18cefcu,0x223b20u,false);
if(s.services.instruction)s.services.instruction(0x18cf01u); s.set(6,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18cf03u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18cf06u); if(s.flags.zf)goto L18cf98;
if(s.services.instruction)s.services.instruction(0x18cf0cu); s.set(1,sign_extend(r(s.memory,(0x2u+s.gp[5]),1),1),2,0);
if(s.services.instruction)s.services.instruction(0x18cf11u); s.push(0x18cf11u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x18cf12u); s.push(0x18cf12u,0x18cf17u);s.required(0x18cf12u,0x228220u,false);
if(s.services.instruction)s.services.instruction(0x18cf17u); s.set(0,r(s.memory,(0x8u+s.gp[0]),2),2,0);
if(s.services.instruction)s.services.instruction(0x18cf1bu); s.set(2,s.reg(0,1,0),1,0);
if(s.services.instruction)s.services.instruction(0x18cf1du); s.set(2,s.flags.right(s.reg(2,1,0),0x1u,1,false),1,0);
if(s.services.instruction)s.services.instruction(0x18cf1fu); s.flags.logic((s.reg(2,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x18cf22u); if(s.flags.zf)goto L18cf98;
if(s.services.instruction)s.services.instruction(0x18cf24u); s.set(0,s.flags.right(s.reg(0,1,0),0x2u,1,false),1,0);
if(s.services.instruction)s.services.instruction(0x18cf27u); s.flags.logic((s.reg(0,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x18cf29u); if(s.flags.zf)goto L18cf98;
if(s.services.instruction)s.services.instruction(0x18cf2bu); s.set(0,r(s.memory,(0x51df54u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cf30u); s.set(1,sign_extend(r(s.memory,(0x2u+s.gp[0]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x18cf34u); s.xmm[0]=_mm_cvtsi32_ss(s.xmm[0],si(r(s.memory,(0x10u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x18cf39u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x46a180u),4))));
if(s.services.instruction)s.services.instruction(0x18cf41u); s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],si(s.reg(1,4,0)));
if(s.services.instruction)s.services.instruction(0x18cf45u); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x18cf49u); w(s.memory,s.observe,0x18cf49u,(0xcu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x18cf4fu); s.fp.push(ext(f(r(s.memory,(0xcu+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x18cf53u); w(s.memory,s.observe,0x18cf53u,(0x10u+s.gp[4]),4,integer_spill_observed(s));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x18cf57u); s.set(2,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cf5au); s.set(0,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cf5eu); s.set(0,(0xau+s.gp[2]+s.gp[0]*1u),4,0);
if(s.services.instruction)s.services.instruction(0x18cf62u); s.set(2,r(s.memory,(0x4f9b58u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cf68u); s.set(1,r(s.memory,(0x20u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cf6bu); s.flags.minus(s.reg(0,4,0),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x18cf6du); {const auto x=s.reg(0,4,0);if(!s.flags.zf&&(s.flags.sf==s.flags.of)){s.set(1,x,4,0);}}
if(s.services.instruction)s.services.instruction(0x18cf70u); w(s.memory,s.observe,0x18cf70u,(0x20u+s.gp[2]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x18cf73u); s.flags.logic((r(s.memory,(0x21u+s.gp[4]),1))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x18cf78u); if(!s.flags.zf)goto L18cf8d;
if(s.services.instruction)s.services.instruction(0x18cf7au); s.set(1,r(s.memory,(0x20u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cf7eu); s.set(1,s.flags.right(s.reg(1,4,0),0xbu,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x18cf81u); s.flags.logic((s.reg(1,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x18cf84u); if(!s.flags.zf)goto L18cf8d;
if(s.services.instruction)s.services.instruction(0x18cf86u); s.flags.logic((r(s.memory,(0x23u+s.gp[4]),1))&(0xfu),1);
if(s.services.instruction)s.services.instruction(0x18cf8bu); if(!s.flags.zf)goto L18cf98;
L18cf8d: if(s.services.instruction)s.services.instruction(0x18cf8du); s.set(1,r(s.memory,(0x24u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18cf90u); s.flags.minus(s.reg(0,4,0),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x18cf92u); {const auto x=s.reg(1,4,0);if(s.flags.zf||(s.flags.sf!=s.flags.of)){s.set(0,x,4,0);}}
if(s.services.instruction)s.services.instruction(0x18cf95u); w(s.memory,s.observe,0x18cf95u,(0x24u+s.gp[2]),4,s.reg(0,4,0));
L18cf98: if(s.services.instruction)s.services.instruction(0x18cf98u); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18cf9au); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18cf9bu); s.set(5,s.pop(),4,0);
L18cf9c: if(s.services.instruction)s.services.instruction(0x18cf9cu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18cf9du); s.set(4,s.flags.plus(s.reg(4,4,0),0xa8u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18cfa3u); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_18d4b0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x18d4b0u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18d4b3u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18d4b6u); if(s.flags.zf)goto L18d507;
if(s.services.instruction)s.services.instruction(0x18d4b8u); s.set(2,r(s.memory,(0x4f9b5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18d4beu); s.push(0x18d4beu,0x18d4c3u);run_16e6a0(s);
if(s.services.instruction)s.services.instruction(0x18d4c3u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18d4c6u); if(s.flags.zf)goto L18d507;
if(s.services.instruction)s.services.instruction(0x18d4c8u); s.set(2,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18d4cbu); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x18d4ceu); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18d4d0u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18d4d6u); s.set(1,(0x0u+s.gp[1]+s.gp[1]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x18d4d9u); s.set(1,(0x0u+s.gp[2]+s.gp[1]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x18d4dcu); s.set(2,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18d4e0u); w(s.memory,s.observe,0x18d4e0u,(0x3u+s.gp[1]),1,0x2u);
if(s.services.instruction)s.services.instruction(0x18d4e4u); w(s.memory,s.observe,0x18d4e4u,(0x2u+s.gp[1]),1,0x3u);
if(s.services.instruction)s.services.instruction(0x18d4e8u); w(s.memory,s.observe,0x18d4e8u,(0x10u+s.gp[1]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18d4ebu); w(s.memory,s.observe,0x18d4ebu,(0x4u+s.gp[1]),2,0x0u);
if(s.services.instruction)s.services.instruction(0x18d4f1u); w(s.memory,s.observe,0x18d4f1u,(0x5u+s.gp[1]),1,s.flags.logic((r(s.memory,(0x5u+s.gp[1]),1))|(0x1u),1));
if(s.services.instruction)s.services.instruction(0x18d4f5u); w(s.memory,s.observe,0x18d4f5u,(0x8u+s.gp[1]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x18d4fau); w(s.memory,s.observe,0x18d4fau,(0xcu+s.gp[1]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18d4fdu); w(s.memory,s.observe,0x18d4fdu,(0x6u+s.gp[1]),2,0xffffu);
if(s.services.instruction)s.services.instruction(0x18d503u); w(s.memory,s.observe,0x18d503u,(0x4u+s.gp[1]),1,s.flags.logic((r(s.memory,(0x4u+s.gp[1]),1))|(0x1u),1));
L18d507: if(s.services.instruction)s.services.instruction(0x18d507u); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_18d510(MathState &s) {
if(s.services.instruction)s.services.instruction(0x18d510u); s.set(1,r(s.memory,(0x4f9b5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18d516u); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18d519u); s.set(1,r(s.memory,(0x4u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x18d51du); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18d522u); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x18d524u); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x18d527u); s.set(0,(0x4u+s.gp[2]+s.gp[0]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x18d52bu); if(s.flags.zf)goto L18d533;
if(s.services.instruction)s.services.instruction(0x18d52du); w(s.memory,s.observe,0x18d52du,(0x0u+s.gp[0]),1,s.flags.logic((r(s.memory,(0x0u+s.gp[0]),1))|(0x10u),1));
if(s.services.instruction)s.services.instruction(0x18d530u); s.pop();s.gp[4]+=4u;return;
L18d533: if(s.services.instruction)s.services.instruction(0x18d533u); w(s.memory,s.observe,0x18d533u,(0x0u+s.gp[0]),1,s.flags.logic((r(s.memory,(0x0u+s.gp[0]),1))&(0xefu),1));
if(s.services.instruction)s.services.instruction(0x18d536u); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_1903a0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x1903a0u); s.push(0x1903a0u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x1903a1u); s.push(0x1903a1u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x1903a2u); s.set(7,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1903a4u); s.set(6,s.flags.logic((s.reg(6,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1903a7u); s.flags.logic((s.reg(7,4,0))&(s.reg(7,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1903a9u); if(s.flags.zf)goto L190419;
if(s.services.instruction)s.services.instruction(0x1903abu); s.flags.minus(s.reg(7,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x1903aeu); if(s.flags.zf)goto L190419;
if(s.services.instruction)s.services.instruction(0x1903b0u); s.flags.minus(s.reg(7,4,0),0x300012au,4);
if(s.services.instruction)s.services.instruction(0x1903b6u); if(!s.flags.zf)goto L1903c0;
if(s.services.instruction)s.services.instruction(0x1903b8u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1903b9u); s.set(0,0xc0000000u,4,0);
if(s.services.instruction)s.services.instruction(0x1903beu); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1903bfu); s.pop();s.gp[4]+=0u;return;
L1903c0: if(s.services.instruction)s.services.instruction(0x1903c0u); s.set(0,r(s.memory,(0x4ecc20u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1903c5u); s.set(1,r(s.memory,(0x324u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1903cbu); s.push(0x1903cbu,0x1903d0u);s.required(0x1903cbu,0x190420u,false);
if(s.services.instruction)s.services.instruction(0x1903d0u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x1903d3u); if(s.flags.zf)goto L1903e6;
if(s.services.instruction)s.services.instruction(0x1903d5u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0x3fffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1903dau); s.set(0,s.flags.logic((s.reg(0,4,0))|(0x80000000u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1903dfu); s.set(6,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1903e1u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x1903e4u); if(!s.flags.zf)goto L190419;
L1903e6: if(s.services.instruction)s.services.instruction(0x1903e6u); s.set(0,r(s.memory,(0x4ecc1cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1903ebu); s.set(1,r(s.memory,(0xc0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1903f1u); s.flags.logic((s.reg(1,4,0))&(s.reg(1,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1903f3u); if(s.flags.zf)goto L190419;
if(s.services.instruction)s.services.instruction(0x1903f5u); s.set(0,r(s.memory,(0xc4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1903fbu); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1903fdu); if(s.flags.zf)goto L190419;
if(s.services.instruction)s.services.instruction(0x1903ffu); s.set(1,r(s.memory,(0xcu+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x190402u); s.push(0x190402u,0x190407u);s.required(0x190402u,0x190420u,false);
if(s.services.instruction)s.services.instruction(0x190407u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x19040au); if(s.flags.zf)goto L190419;
if(s.services.instruction)s.services.instruction(0x19040cu); s.set(0,s.flags.logic((s.reg(0,4,0))&(0x3fffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x190411u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x190412u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0x40000000u),4),4,0);
if(s.services.instruction)s.services.instruction(0x190417u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x190418u); s.pop();s.gp[4]+=0u;return;
L190419: if(s.services.instruction)s.services.instruction(0x190419u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x19041au); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x19041cu); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x19041du); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_1d8b90(MathState &s) {
if(s.services.instruction)s.services.instruction(0x1d8b90u); s.push(0x1d8b90u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x1d8b91u); s.set(5,s.reg(4,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1d8b93u); s.set(4,s.flags.logic((s.reg(4,4,0))&(0xfffffff0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d8b96u); s.set(4,s.flags.minus(s.reg(4,4,0),0x40u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d8b99u); s.set(2,r(s.memory,(0x70u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d8b9cu); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x1d8b9fu); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d8ba2u); s.set(0,r(s.memory,(0x40u+s.gp[0]+s.gp[2]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d8ba6u); s.set(0,r(s.memory,(0x3cu+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d8ba9u); s.xmm[0]=v(s.memory,(0x80u+s.gp[0]));
if(s.services.instruction)s.services.instruction(0x1d8bb0u); s.set(2,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d8bb3u); s.set(0,s.flags.plus(s.reg(0,4,0),0x80u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d8bb8u); vw(s.memory,s.observe,0x1d8bb8u,(0x0u+s.gp[4]),s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1d8bbcu); s.xmm[0]=v(s.memory,(0x10u+s.gp[0]));
if(s.services.instruction)s.services.instruction(0x1d8bc0u); vw(s.memory,s.observe,0x1d8bc0u,(0x10u+s.gp[4]),s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1d8bc5u); s.xmm[0]=v(s.memory,(0x20u+s.gp[0]));
if(s.services.instruction)s.services.instruction(0x1d8bc9u); s.set(2,s.flags.right(s.reg(2,4,0),0x6u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x1d8bccu); s.flags.logic((s.reg(2,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x1d8bcfu); vw(s.memory,s.observe,0x1d8bcfu,(0x20u+s.gp[4]),s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1d8bd4u); s.xmm[0]=v(s.memory,(0x30u+s.gp[0]));
if(s.services.instruction)s.services.instruction(0x1d8bd8u); vw(s.memory,s.observe,0x1d8bd8u,(0x30u+s.gp[4]),s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1d8bddu); if(s.flags.zf)goto L1d8bec;
if(s.services.instruction)s.services.instruction(0x1d8bdfu); s.set(1,s.flags.plus(s.reg(1,4,0),0x30u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d8be2u); s.push(0x1d8be2u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1d8be3u); s.set(1,(0x4u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1d8be7u); s.push(0x1d8be7u,0x1d8becu);s.required(0x1d8be7u,0x2e4650u,false);
L1d8bec: if(s.services.instruction)s.services.instruction(0x1d8becu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x1d8bf4u); w(s.memory,s.observe,0x1d8bf4u,(0x0u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8bf8u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8bfdu); w(s.memory,s.observe,0x1d8bfdu,(0x4u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c02u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8c08u); w(s.memory,s.observe,0x1d8c08u,(0x8u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c0du); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8c13u); w(s.memory,s.observe,0x1d8c13u,(0xcu+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c18u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x10u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8c1eu); w(s.memory,s.observe,0x1d8c1eu,(0x10u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c23u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8c29u); w(s.memory,s.observe,0x1d8c29u,(0x14u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c2eu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x18u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8c34u); w(s.memory,s.observe,0x1d8c34u,(0x18u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c39u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x20u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8c3fu); w(s.memory,s.observe,0x1d8c3fu,(0x1cu+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c44u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x24u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8c4au); w(s.memory,s.observe,0x1d8c4au,(0x20u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c4fu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x28u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8c55u); w(s.memory,s.observe,0x1d8c55u,(0x24u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c5au); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x30u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8c60u); w(s.memory,s.observe,0x1d8c60u,(0x28u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c65u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x34u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8c6bu); w(s.memory,s.observe,0x1d8c6bu,(0x2cu+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c70u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x38u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d8c76u); w(s.memory,s.observe,0x1d8c76u,(0x30u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d8c7bu); s.set(4,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1d8c7du); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1d8c7eu); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_22e210(MathState &s) {
if(s.services.instruction)s.services.instruction(0x22e210u); s.set(1,r(s.memory,(0x52be5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e216u); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e219u); s.set(1,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e21fu); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e224u); s.set(0,(0x0u+s.gp[0]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x22e227u); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e22au); s.set(0,r(s.memory,(0x8u+s.gp[0]+s.gp[2]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e22eu); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e231u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e236u); s.push(0x22e236u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22e237u); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x22e23au); s.set(2,r(s.memory,(0x8u+s.gp[2]+s.gp[0]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e23eu); s.set(0,r(s.memory,(0x8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e242u); s.set(0,s.flags.plus(s.reg(0,4,0),0x5cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e245u); s.push(0x22e245u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x22e246u); s.push(0x22e246u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x22e247u); s.set(7,(0x18u+s.gp[6]),4,0);
if(s.services.instruction)s.services.instruction(0x22e24au); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e24cu); s.set(5,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e24eu); s.set(3,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e250u); w(s.memory,s.observe,0x22e250u,(0x0u+s.gp[3]),4,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x22e252u); s.set(5,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e255u); w(s.memory,s.observe,0x22e255u,(0x4u+s.gp[3]),4,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x22e258u); s.set(1,r(s.memory,(0x8u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e25bu); w(s.memory,s.observe,0x22e25bu,(0x8u+s.gp[3]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22e25eu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x78u+s.gp[2]),4)));
if(s.services.instruction)s.services.instruction(0x22e263u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x22e268u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x74u+s.gp[2]),4)));
if(s.services.instruction)s.services.instruction(0x22e26du); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x22e272u); s.set(1,(0x70u+s.gp[2]),4,0);
if(s.services.instruction)s.services.instruction(0x22e275u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e279u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[1]),4)));
if(s.services.instruction)s.services.instruction(0x22e27du); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x22e281u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e285u); s.xmm[1]=_mm_xor_ps(s.xmm[1],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e288u); s.flags.scalar_compare(s.xmm[0],s.xmm[1],false);
if(s.services.instruction)s.services.instruction(0x22e28bu); if(!s.flags.cf)goto L22e294;
if(s.services.instruction)s.services.instruction(0x22e28du); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22e291u); s.xmm[0]=s.xmm[1];
L22e294: if(s.services.instruction)s.services.instruction(0x22e294u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x46a194u),4)));
if(s.services.instruction)s.services.instruction(0x22e29cu); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x22e29fu); if(!s.flags.cf&&!s.flags.zf)goto L22e2a4;
if(s.services.instruction)s.services.instruction(0x22e2a1u); s.set(1,(0x7cu+s.gp[2]),4,0);
L22e2a4: if(s.services.instruction)s.services.instruction(0x22e2a4u); s.set(0,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e2a6u); s.set(2,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e2a8u); w(s.memory,s.observe,0x22e2a8u,(0x0u+s.gp[2]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22e2aau); s.set(0,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e2adu); w(s.memory,s.observe,0x22e2adu,(0x4u+s.gp[2]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22e2b0u); s.set(1,r(s.memory,(0x8u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e2b3u); w(s.memory,s.observe,0x22e2b3u,(0x8u+s.gp[2]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22e2b6u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[7]),4)));
if(s.services.instruction)s.services.instruction(0x22e2bbu); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[6]),4))));
if(s.services.instruction)s.services.instruction(0x22e2c0u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[7]),4)));
if(s.services.instruction)s.services.instruction(0x22e2c5u); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[6]),4))));
if(s.services.instruction)s.services.instruction(0x22e2cau); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[7]),4)));
if(s.services.instruction)s.services.instruction(0x22e2cfu); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[7]),4)));
if(s.services.instruction)s.services.instruction(0x22e2d4u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e2d8u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[7]),4)));
if(s.services.instruction)s.services.instruction(0x22e2dcu); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[6]),4))));
if(s.services.instruction)s.services.instruction(0x22e2e0u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e2e4u); s.xmm[1]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x22e2e7u); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22e2ebu); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22e2efu); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22e2f3u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[6]),4)));
if(s.services.instruction)s.services.instruction(0x22e2f7u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e2fbu); w(s.memory,s.observe,0x22e2fbu,(0x0u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e2ffu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[6]),4)));
if(s.services.instruction)s.services.instruction(0x22e304u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22e308u); w(s.memory,s.observe,0x22e308u,(0x4u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e30du); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[6]),4)));
if(s.services.instruction)s.services.instruction(0x22e312u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22e316u); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e318u); w(s.memory,s.observe,0x22e318u,(0x8u+s.gp[6]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e31du); s.push(0x22e31du,0x22e322u);s.required(0x22e31du,0x30bd0u,false);
if(s.services.instruction)s.services.instruction(0x22e322u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[7]),4)));
if(s.services.instruction)s.services.instruction(0x22e326u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[7]),4)));
if(s.services.instruction)s.services.instruction(0x22e32bu); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[6]),4))));
if(s.services.instruction)s.services.instruction(0x22e32fu); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[6]),4))));
if(s.services.instruction)s.services.instruction(0x22e334u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[7]),4)));
if(s.services.instruction)s.services.instruction(0x22e338u); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[6]),4))));
if(s.services.instruction)s.services.instruction(0x22e33du); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[7]),4)));
if(s.services.instruction)s.services.instruction(0x22e342u); s.xmm[3]=_mm_mul_ss(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[6]),4))));
if(s.services.instruction)s.services.instruction(0x22e347u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e34bu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[6]),4)));
if(s.services.instruction)s.services.instruction(0x22e34fu); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22e354u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22e358u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[7]),4)));
if(s.services.instruction)s.services.instruction(0x22e35du); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[6]),4))));
if(s.services.instruction)s.services.instruction(0x22e362u); s.set(0,(0xcu+s.gp[6]),4,0);
if(s.services.instruction)s.services.instruction(0x22e365u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22e369u); w(s.memory,s.observe,0x22e369u,(0x0u+s.gp[0]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x22e36du); w(s.memory,s.observe,0x22e36du,(0x4u+s.gp[0]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22e372u); w(s.memory,s.observe,0x22e372u,(0x8u+s.gp[0]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e377u); s.push(0x22e377u,0x22e37cu);s.required(0x22e377u,0x30bd0u,false);
if(s.services.instruction)s.services.instruction(0x22e37cu); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e37du); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e37eu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e37fu); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_22e390(MathState &s) {
if(s.services.instruction)s.services.instruction(0x22e390u); s.set(2,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e394u); s.set(0,r(s.memory,(0x24u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e397u); s.push(0x22e397u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22e398u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e39du); s.push(0x22e39du,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x22e39eu); s.set(1,(0x0u+s.gp[0]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x22e3a1u); s.set(0,r(s.memory,(0x52be5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e3a6u); s.push(0x22e3a6u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22e3a7u); s.push(0x22e3a7u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x22e3a8u); s.set(7,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e3abu); s.set(0,r(s.memory,(0x9cu+s.gp[2]),2),2,0);
if(s.services.instruction)s.services.instruction(0x22e3b2u); s.set(1,s.flags.shift(s.reg(1,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e3b5u); s.set(3,s.flags.logic((s.reg(3,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e3b8u); s.set(1,s.flags.plus(s.reg(1,4,0),s.reg(7,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e3bau); s.flags.minus(s.reg(0,2,0),s.reg(3,2,0),2);
if(s.services.instruction)s.services.instruction(0x22e3bdu); if(s.flags.zf)goto L22e410;
if(s.services.instruction)s.services.instruction(0x22e3bfu); w(s.memory,s.observe,0x22e3bfu,(0x18u+s.gp[4]),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22e3c3u); s.set(3,r(s.memory,(0x80u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e3c9u); s.set(6,s.flags.logic((s.reg(6,4,0))^(s.reg(6,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e3cbu); s.flags.logic((s.reg(3,4,0))&(s.reg(3,4,0)),4);
if(s.services.instruction)s.services.instruction(0x22e3cdu); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L22e3f7;
if(s.services.instruction)s.services.instruction(0x22e3cfu); s.set(2,r(s.memory,(0x7cu+s.gp[1]),4),4,0);
L22e3d2: if(s.services.instruction)s.services.instruction(0x22e3d2u); s.set(7,r(s.memory,(0x0u+s.gp[2]),2),2,0);
if(s.services.instruction)s.services.instruction(0x22e3d5u); s.flags.minus(s.reg(7,2,0),s.reg(0,2,0),2);
if(s.services.instruction)s.services.instruction(0x22e3d8u); s.set(5,r(s.memory,(0x2u+s.gp[2]),2),2,0);
if(s.services.instruction)s.services.instruction(0x22e3dcu); if(!s.flags.zf)goto L22e3ef;
if(s.services.instruction)s.services.instruction(0x22e3deu); s.set(7,r(s.memory,(0x14u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e3e2u); s.flags.minus(s.reg(5,2,0),r(s.memory,(0x9eu+s.gp[7]),2),2);
if(s.services.instruction)s.services.instruction(0x22e3e9u); if(!s.flags.zf)goto L22e3ef;
if(s.services.instruction)s.services.instruction(0x22e3ebu); w(s.memory,s.observe,0x22e3ebu,(0x18u+s.gp[4]),4,s.reg(6,4,0));
L22e3ef: if(s.services.instruction)s.services.instruction(0x22e3efu); s.set(6,s.flags.increment(s.reg(6,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x22e3f0u); s.set(2,s.flags.plus(s.reg(2,4,0),0xcu,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e3f3u); s.flags.minus(s.reg(6,4,0),s.reg(3,4,0),4);
if(s.services.instruction)s.services.instruction(0x22e3f5u); if(s.flags.sf!=s.flags.of)goto L22e3d2;
L22e3f7: if(s.services.instruction)s.services.instruction(0x22e3f7u); s.set(2,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e3fbu); s.set(3,r(s.memory,(0x20u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e3ffu); s.set(0,r(s.memory,(0x18u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e403u); s.push(0x22e403u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22e404u); s.push(0x22e404u,0x22e409u);run_de8c0(s);
if(s.services.instruction)s.services.instruction(0x22e409u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e40au); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e40bu); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e40cu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e40du); s.pop();s.gp[4]+=16u;return;
L22e410: if(s.services.instruction)s.services.instruction(0x22e410u); s.flags.minus(r(s.memory,(0xdu+s.gp[2]),1),s.reg(3,1,0),1);
if(s.services.instruction)s.services.instruction(0x22e413u); if(s.flags.zf)goto L22e458;
if(s.services.instruction)s.services.instruction(0x22e415u); s.set(0,r(s.memory,(0x18u+s.gp[1]),1),1,0);
if(s.services.instruction)s.services.instruction(0x22e418u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x22e41au); if(s.flags.sf!=s.flags.of)goto L22e440;
if(s.services.instruction)s.services.instruction(0x22e41cu); s.set(6,r(s.memory,(0x74u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e41fu); s.set(2,sign_extend(s.reg(0,1,0),1),4,0);
if(s.services.instruction)s.services.instruction(0x22e422u); s.flags.minus(s.reg(2,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x22e424u); if(s.flags.sf==s.flags.of)goto L22e440;
if(s.services.instruction)s.services.instruction(0x22e426u); s.set(1,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e42au); s.set(2,r(s.memory,(0x20u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e42eu); s.set(0,sign_extend(s.reg(0,1,0),1),2,0);
if(s.services.instruction)s.services.instruction(0x22e432u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e433u); s.set(0,sign_extend(s.reg(0,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x22e436u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e437u); w(s.memory,s.observe,0x22e437u,(0x0u+s.gp[1]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22e439u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e43au); w(s.memory,s.observe,0x22e43au,(0x0u+s.gp[2]),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22e43cu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e43du); s.pop();s.gp[4]+=16u;return;
L22e440: if(s.services.instruction)s.services.instruction(0x22e440u); s.set(1,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e444u); s.set(2,r(s.memory,(0x20u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e448u); s.set(0,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e44au); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e44bu); s.set(0,sign_extend(s.reg(0,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x22e44eu); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e44fu); w(s.memory,s.observe,0x22e44fu,(0x0u+s.gp[1]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22e451u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e452u); w(s.memory,s.observe,0x22e452u,(0x0u+s.gp[2]),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22e454u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e455u); s.pop();s.gp[4]+=16u;return;
L22e458: if(s.services.instruction)s.services.instruction(0x22e458u); s.set(0,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e45cu); s.set(5,r(s.memory,(0x20u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e460u); w(s.memory,s.observe,0x22e460u,(0x0u+s.gp[0]),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22e462u); w(s.memory,s.observe,0x22e462u,(0x0u+s.gp[5]),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22e465u); s.set(0,r(s.memory,(0x8cu+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e46bu); s.set(7,s.flags.logic((s.reg(7,4,0))^(s.reg(7,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e46du); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x22e46fu); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L22e4dc;
if(s.services.instruction)s.services.instruction(0x22e471u); s.set(6,s.flags.logic((s.reg(6,4,0))^(s.reg(6,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e473u); goto L22e480;
L22e480: if(s.services.instruction)s.services.instruction(0x22e480u); s.set(0,r(s.memory,(0x88u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e486u); s.set(2,r(s.memory,(0x8u+s.gp[0]+s.gp[6]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e48au); s.set(3,r(s.memory,(0x18u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e48eu); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(6,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e490u); s.flags.minus(s.reg(2,4,0),s.reg(3,4,0),4);
if(s.services.instruction)s.services.instruction(0x22e492u); if(!s.flags.zf)goto L22e4ce;
if(s.services.instruction)s.services.instruction(0x22e494u); s.set(2,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e498u); s.set(2,r(s.memory,(0x0u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e49au); s.flags.minus(s.reg(2,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x22e49du); if(s.flags.zf)goto L22e4a7;
if(s.services.instruction)s.services.instruction(0x22e49fu); s.set(3,sign_extend(r(s.memory,(0x45u+s.gp[0]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x22e4a3u); s.flags.minus(s.reg(3,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x22e4a5u); if(s.flags.sf==s.flags.of)goto L22e4b1;
L22e4a7: if(s.services.instruction)s.services.instruction(0x22e4a7u); s.set(2,sign_extend(r(s.memory,(0x45u+s.gp[0]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x22e4abu); s.set(3,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e4afu); w(s.memory,s.observe,0x22e4afu,(0x0u+s.gp[3]),4,s.reg(2,4,0));
L22e4b1: if(s.services.instruction)s.services.instruction(0x22e4b1u); s.flags.minus(r(s.memory,(0x14u+s.gp[0]),4),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x22e4b5u); if(s.flags.zf)goto L22e4ce;
if(s.services.instruction)s.services.instruction(0x22e4b7u); s.set(2,r(s.memory,(0x0u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e4bau); s.flags.minus(s.reg(2,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x22e4bdu); if(s.flags.zf)goto L22e4c7;
if(s.services.instruction)s.services.instruction(0x22e4bfu); s.set(3,sign_extend(r(s.memory,(0x46u+s.gp[0]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x22e4c3u); s.flags.minus(s.reg(3,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x22e4c5u); if(s.flags.sf==s.flags.of)goto L22e4ce;
L22e4c7: if(s.services.instruction)s.services.instruction(0x22e4c7u); s.set(0,sign_extend(r(s.memory,(0x46u+s.gp[0]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x22e4cbu); w(s.memory,s.observe,0x22e4cbu,(0x0u+s.gp[5]),4,s.reg(0,4,0));
L22e4ce: if(s.services.instruction)s.services.instruction(0x22e4ceu); s.set(0,r(s.memory,(0x8cu+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e4d4u); s.set(7,s.flags.increment(s.reg(7,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x22e4d5u); s.set(6,s.flags.plus(s.reg(6,4,0),0x48u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e4d8u); s.flags.minus(s.reg(7,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x22e4dau); if(s.flags.sf!=s.flags.of)goto L22e480;
L22e4dc: if(s.services.instruction)s.services.instruction(0x22e4dcu); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e4ddu); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e4deu); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e4dfu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e4e0u); s.pop();s.gp[4]+=16u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_22e4f0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x22e4f0u); s.set(1,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e4f4u); s.set(4,s.flags.minus(s.reg(4,4,0),0x7cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e4f7u); s.push(0x22e4f7u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22e4f8u); s.set(3,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e4fau); s.push(0x22e4fau,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x22e4fbu); s.set(5,r(s.memory,(0x94u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e502u); s.push(0x22e502u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22e503u); s.set(0,(0x5cu+s.gp[1]),4,0);
if(s.services.instruction)s.services.instruction(0x22e506u); s.set(6,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e508u); s.set(2,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e50au); w(s.memory,s.observe,0x22e50au,(0x0u+s.gp[2]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22e50cu); s.set(6,r(s.memory,(0x4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e50fu); w(s.memory,s.observe,0x22e50fu,(0x4u+s.gp[2]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22e512u); s.set(0,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e515u); w(s.memory,s.observe,0x22e515u,(0x8u+s.gp[2]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22e518u); s.set(2,(0x68u+s.gp[1]),4,0);
if(s.services.instruction)s.services.instruction(0x22e51bu); s.set(6,r(s.memory,(0x0u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e51du); s.set(0,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e51fu); w(s.memory,s.observe,0x22e51fu,(0x0u+s.gp[0]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22e521u); s.set(6,r(s.memory,(0x4u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e524u); w(s.memory,s.observe,0x22e524u,(0x4u+s.gp[0]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22e527u); s.set(2,r(s.memory,(0x8u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e52au); s.set(6,s.flags.logic((s.reg(6,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e52du); w(s.memory,s.observe,0x22e52du,(0x8u+s.gp[0]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22e530u); s.flags.minus(r(s.memory,(0x9cu+s.gp[1]),2),s.reg(6,2,0),2);
if(s.services.instruction)s.services.instruction(0x22e537u); if(s.flags.zf)goto L22e5e5;
if(s.services.instruction)s.services.instruction(0x22e53du); s.set(0,r(s.memory,(0x24u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e540u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e545u); s.set(1,(0x0u+s.gp[0]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x22e548u); s.set(0,r(s.memory,(0x52be5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e54du); s.set(2,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e550u); s.set(0,r(s.memory,(0x94u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e557u); s.set(1,s.flags.shift(s.reg(1,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e55au); s.set(1,s.flags.plus(s.reg(1,4,0),s.reg(2,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e55cu); s.set(6,(0x54u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x22e560u); s.push(0x22e560u,0x22e565u);run_1d8b90(s);
if(s.services.instruction)s.services.instruction(0x22e565u); s.set(1,r(s.memory,(0x7cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e569u); s.set(2,r(s.memory,(0x80u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e570u); s.set(0,r(s.memory,(0x84u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e577u); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22e57au); w(s.memory,s.observe,0x22e57au,(0x0u+s.gp[7]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22e57cu); s.set(1,r(s.memory,(0x58u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e580u); w(s.memory,s.observe,0x22e580u,(0x4u+s.gp[7]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22e583u); s.set(2,r(s.memory,(0x5cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e587u); w(s.memory,s.observe,0x22e587u,(0x8u+s.gp[7]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22e58au); s.set(0,r(s.memory,(0x60u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e58eu); w(s.memory,s.observe,0x22e58eu,(0x0u+s.gp[5]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22e591u); s.set(1,r(s.memory,(0x70u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e595u); w(s.memory,s.observe,0x22e595u,(0x4u+s.gp[5]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22e598u); s.set(2,r(s.memory,(0x74u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e59cu); w(s.memory,s.observe,0x22e59cu,(0x8u+s.gp[5]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22e59fu); s.set(0,r(s.memory,(0x78u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e5a3u); w(s.memory,s.observe,0x22e5a3u,(0x0u+s.gp[3]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22e5a5u); s.set(1,r(s.memory,(0x64u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e5a9u); w(s.memory,s.observe,0x22e5a9u,(0x4u+s.gp[3]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22e5acu); s.set(2,r(s.memory,(0x68u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e5b0u); w(s.memory,s.observe,0x22e5b0u,(0x8u+s.gp[3]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22e5b3u); s.set(0,r(s.memory,(0x9cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e5bau); w(s.memory,s.observe,0x22e5bau,(0x0u+s.gp[0]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22e5bcu); s.set(1,r(s.memory,(0x6cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e5c0u); w(s.memory,s.observe,0x22e5c0u,(0x4u+s.gp[0]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22e5c3u); s.set(2,r(s.memory,(0xa0u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e5cau); w(s.memory,s.observe,0x22e5cau,(0x8u+s.gp[0]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22e5cdu); s.set(0,r(s.memory,(0xa4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e5d4u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e5d5u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e5d6u); w(s.memory,s.observe,0x22e5d6u,(0x0u+s.gp[2]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e5dau); w(s.memory,s.observe,0x22e5dau,(0x0u+s.gp[0]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e5deu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e5dfu); s.set(4,s.flags.plus(s.reg(4,4,0),0x7cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e5e2u); s.pop();s.gp[4]+=32u;return;
L22e5e5: if(s.services.instruction)s.services.instruction(0x22e5e5u); s.flags.minus(r(s.memory,(0x8u+s.gp[1]),2),0x1u,2);
if(s.services.instruction)s.services.instruction(0x22e5eau); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L22e753;
if(s.services.instruction)s.services.instruction(0x22e5f0u); s.set(0,r(s.memory,(0xeu+s.gp[1]),1),1,0);
if(s.services.instruction)s.services.instruction(0x22e5f3u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x22e5f5u); if(s.flags.zf)goto L22e606;
if(s.services.instruction)s.services.instruction(0x22e5f7u); s.set(0,r(s.memory,(0xa8u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x22e5feu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x22e600u); if(!s.flags.zf)goto L22e753;
L22e606: if(s.services.instruction)s.services.instruction(0x22e606u); s.set(0,r(s.memory,(0x474d10u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e60bu); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22e60eu); s.set(1,r(s.memory,(0xa0u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e615u); s.set(2,r(s.memory,(0xa4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e61cu); w(s.memory,s.observe,0x22e61cu,(0x0u+s.gp[1]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e620u); w(s.memory,s.observe,0x22e620u,(0x0u+s.gp[2]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e624u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22e629u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e62eu); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x22e632u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[3]),4))));
if(s.services.instruction)s.services.instruction(0x22e636u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e63au); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x22e63fu); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22e644u); s.xmm[3]=_mm_mul_ss(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[3]),4))));
if(s.services.instruction)s.services.instruction(0x22e649u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e64du); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e652u); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x22e656u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22e65au); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22e65fu); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[3]),4))));
if(s.services.instruction)s.services.instruction(0x22e664u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22e668u); s.set(0,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e66au); w(s.memory,s.observe,0x22e66au,(0x0u+s.gp[5]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x22e66fu); w(s.memory,s.observe,0x22e66fu,(0x4u+s.gp[5]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22e674u); w(s.memory,s.observe,0x22e674u,(0x8u+s.gp[5]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e679u); s.push(0x22e679u,0x22e67eu);s.required(0x22e679u,0x30bd0u,false);
if(s.services.instruction)s.services.instruction(0x22e67eu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x46a180u),4)));
if(s.services.instruction)s.services.instruction(0x22e686u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x22e689u); if(s.flags.cf||s.flags.zf)goto L22e6ea;
if(s.services.instruction)s.services.instruction(0x22e68bu); s.set(0,r(s.memory,(0x474d08u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e690u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22e695u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e69au); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x22e69eu); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[3]),4))));
if(s.services.instruction)s.services.instruction(0x22e6a2u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e6a6u); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x22e6abu); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22e6b0u); s.xmm[3]=_mm_mul_ss(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[3]),4))));
if(s.services.instruction)s.services.instruction(0x22e6b5u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e6b9u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e6beu); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x22e6c2u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22e6c6u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22e6cbu); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[3]),4))));
if(s.services.instruction)s.services.instruction(0x22e6d0u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22e6d4u); s.set(0,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e6d6u); w(s.memory,s.observe,0x22e6d6u,(0x0u+s.gp[5]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x22e6dbu); w(s.memory,s.observe,0x22e6dbu,(0x4u+s.gp[5]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22e6e0u); w(s.memory,s.observe,0x22e6e0u,(0x8u+s.gp[5]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e6e5u); s.push(0x22e6e5u,0x22e6eau);s.required(0x22e6e5u,0x30bd0u,false);
L22e6ea: if(s.services.instruction)s.services.instruction(0x22e6eau); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e6eeu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x22e6f3u); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[3]),4))));
if(s.services.instruction)s.services.instruction(0x22e6f8u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[5]),4))));
if(s.services.instruction)s.services.instruction(0x22e6fdu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e701u); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[5]),4))));
if(s.services.instruction)s.services.instruction(0x22e706u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x22e70bu); s.xmm[3]=_mm_mul_ss(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[3]),4))));
if(s.services.instruction)s.services.instruction(0x22e710u); s.set(0,r(s.memory,(0x9cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e717u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e71bu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x22e720u); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[3]),4))));
if(s.services.instruction)s.services.instruction(0x22e725u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22e729u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e72eu); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[5]),4))));
if(s.services.instruction)s.services.instruction(0x22e733u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22e737u); w(s.memory,s.observe,0x22e737u,(0x0u+s.gp[0]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x22e73bu); w(s.memory,s.observe,0x22e73bu,(0x4u+s.gp[0]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22e740u); w(s.memory,s.observe,0x22e740u,(0x8u+s.gp[0]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e745u); s.push(0x22e745u,0x22e74au);s.required(0x22e745u,0x30bd0u,false);
if(s.services.instruction)s.services.instruction(0x22e74au); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e74bu); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e74cu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e74du); s.set(4,s.flags.plus(s.reg(4,4,0),0x7cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e750u); s.pop();s.gp[4]+=32u;return;
L22e753: if(s.services.instruction)s.services.instruction(0x22e753u); s.set(0,r(s.memory,(0x24u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e756u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x45695cu),4)));
if(s.services.instruction)s.services.instruction(0x22e75eu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x44aac8u),4)));
if(s.services.instruction)s.services.instruction(0x22e766u); s.set(2,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e768u); s.set(2,s.flags.logic((s.reg(2,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e76eu); s.set(3,(0x0u+s.gp[2]+s.gp[2]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x22e771u); s.set(2,r(s.memory,(0x52be5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e777u); s.set(3,s.flags.shift(s.reg(3,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e77au); s.set(3,s.flags.plus(s.reg(3,4,0),r(s.memory,(0x44u+s.gp[2]),4),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e77du); s.flags.minus(r(s.memory,(0x28u+s.gp[1]),4),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x22e780u); s.push(0x22e780u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22e781u); s.set(6,(0x58u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x22e785u); w(s.memory,s.observe,0x22e785u,(0x13u+s.gp[4]),1,!s.flags.zf);
if(s.services.instruction)s.services.instruction(0x22e78au); w(s.memory,s.observe,0x22e78au,(0x24u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e790u); w(s.memory,s.observe,0x22e790u,(0x14u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22e796u); w(s.memory,s.observe,0x22e796u,(0x28u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e79cu); w(s.memory,s.observe,0x22e79cu,(0x20u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22e7a2u); w(s.memory,s.observe,0x22e7a2u,(0x18u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e7a8u); w(s.memory,s.observe,0x22e7a8u,(0x30u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22e7aeu); w(s.memory,s.observe,0x22e7aeu,(0x1cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e7b4u); w(s.memory,s.observe,0x22e7b4u,(0x2cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22e7bau); s.push(0x22e7bau,0x22e7bfu);run_22e210(s);
if(s.services.instruction)s.services.instruction(0x22e7bfu); s.set(1,r(s.memory,(0xfu+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x22e7c3u); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x22e7c5u); if(s.flags.zf)goto L22e7df;
if(s.services.instruction)s.services.instruction(0x22e7c7u); s.set(0,r(s.memory,(0x8cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e7ceu); s.push(0x22e7ceu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22e7cfu); s.set(0,r(s.memory,(0x28u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e7d2u); s.set(6,(0x34u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x22e7d6u); s.push(0x22e7d6u,0x22e7dbu);run_22e210(s);
if(s.services.instruction)s.services.instruction(0x22e7dbu); s.set(1,r(s.memory,(0xfu+s.gp[4]),1),1,0);
L22e7df: if(s.services.instruction)s.services.instruction(0x22e7dfu); s.set(0,r(s.memory,(0x8cu+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e7e5u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x22e7e7u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L22e968;
if(s.services.instruction)s.services.instruction(0x22e7edu); s.set(3,r(s.memory,(0x88u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e7f3u); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x60u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e7f9u); s.xmm[6]=_mm_set_ss(f(r(s.memory,(0x64u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e7ffu); s.set(3,s.flags.plus(s.reg(3,4,0),0x24u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e802u); goto L22e810;
L22e810: if(s.services.instruction)s.services.instruction(0x22e810u); s.set(2,r(s.memory,(0x90u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e817u); s.flags.minus(r(s.memory,(0xffffffe4u+s.gp[3]),4),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x22e81au); if(!s.flags.zf)goto L22e95e;
if(s.services.instruction)s.services.instruction(0x22e820u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0xfffffffcu+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e825u); s.xmm[2]=_mm_sub_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22e82au); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e82eu); s.xmm[3]=_mm_sub_ss(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22e833u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x5cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e839u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x58u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e83fu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0xfffffff8u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e844u); s.xmm[1]=_mm_sub_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22e848u); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22e84cu); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22e850u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x22e854u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x54u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e85au); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e85eu); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x22e862u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x68u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e868u); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22e86cu); s.xmm[7]=s.xmm[6];
if(s.services.instruction)s.services.instruction(0x22e86fu); s.xmm[7]=_mm_mul_ss(s.xmm[7],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22e873u); s.xmm[4]=_mm_add_ss(s.xmm[4],s.xmm[7]);
if(s.services.instruction)s.services.instruction(0x22e877u); s.xmm[7]=s.xmm[5];
if(s.services.instruction)s.services.instruction(0x22e87au); s.xmm[7]=_mm_mul_ss(s.xmm[7],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e87eu); s.xmm[4]=_mm_add_ss(s.xmm[4],s.xmm[7]);
if(s.services.instruction)s.services.instruction(0x22e882u); s.xmm[7]=_mm_set_ss(f(r(s.memory,(0x10u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e888u); s.flags.scalar_compare(s.xmm[7],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x22e88bu); if(!s.flags.cf&&!s.flags.zf)goto L22e893;
if(s.services.instruction)s.services.instruction(0x22e88du); w(s.memory,s.observe,0x22e88du,(0x10u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
L22e893: if(s.services.instruction)s.services.instruction(0x22e893u); s.xmm[7]=_mm_set_ss(f(r(s.memory,(0x20u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e899u); s.flags.scalar_compare(s.xmm[7],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x22e89cu); if(s.flags.cf||s.flags.zf)goto L22e8a4;
if(s.services.instruction)s.services.instruction(0x22e89eu); w(s.memory,s.observe,0x22e89eu,(0x20u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
L22e8a4: if(s.services.instruction)s.services.instruction(0x22e8a4u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x1cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e8aau); s.flags.scalar_compare(s.xmm[0],s.xmm[4],false);
if(s.services.instruction)s.services.instruction(0x22e8adu); if(!s.flags.cf&&!s.flags.zf)goto L22e8b5;
if(s.services.instruction)s.services.instruction(0x22e8afu); w(s.memory,s.observe,0x22e8afu,(0x1cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[4])));
L22e8b5: if(s.services.instruction)s.services.instruction(0x22e8b5u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x24u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e8bbu); s.flags.scalar_compare(s.xmm[0],s.xmm[4],false);
if(s.services.instruction)s.services.instruction(0x22e8beu); if(s.flags.cf||s.flags.zf)goto L22e8c6;
if(s.services.instruction)s.services.instruction(0x22e8c0u); w(s.memory,s.observe,0x22e8c0u,(0x24u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[4])));
L22e8c6: if(s.services.instruction)s.services.instruction(0x22e8c6u); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x22e8c8u); if(s.flags.zf)goto L22e95e;
if(s.services.instruction)s.services.instruction(0x22e8ceu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x48u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e8d4u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x3cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e8dau); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22e8deu); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22e8e2u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x22e8e6u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x30u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e8ecu); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e8f0u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x22e8f4u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x4cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e8fau); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22e8feu); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x40u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e904u); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22e908u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x34u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e90eu); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22e912u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x2cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e918u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x22e91bu); s.xmm[4]=_mm_add_ss(s.xmm[4],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22e91fu); s.xmm[4]=_mm_add_ss(s.xmm[4],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22e923u); if(!s.flags.cf&&!s.flags.zf)goto L22e92b;
if(s.services.instruction)s.services.instruction(0x22e925u); w(s.memory,s.observe,0x22e925u,(0x2cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
L22e92b: if(s.services.instruction)s.services.instruction(0x22e92bu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e931u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x22e934u); if(s.flags.cf||s.flags.zf)goto L22e93c;
if(s.services.instruction)s.services.instruction(0x22e936u); w(s.memory,s.observe,0x22e936u,(0x14u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
L22e93c: if(s.services.instruction)s.services.instruction(0x22e93cu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x28u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e942u); s.flags.scalar_compare(s.xmm[0],s.xmm[4],false);
if(s.services.instruction)s.services.instruction(0x22e945u); if(!s.flags.cf&&!s.flags.zf)goto L22e94d;
if(s.services.instruction)s.services.instruction(0x22e947u); w(s.memory,s.observe,0x22e947u,(0x28u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[4])));
L22e94d: if(s.services.instruction)s.services.instruction(0x22e94du); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x18u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e953u); s.flags.scalar_compare(s.xmm[0],s.xmm[4],false);
if(s.services.instruction)s.services.instruction(0x22e956u); if(s.flags.cf||s.flags.zf)goto L22e95e;
if(s.services.instruction)s.services.instruction(0x22e958u); w(s.memory,s.observe,0x22e958u,(0x18u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[4])));
L22e95e: if(s.services.instruction)s.services.instruction(0x22e95eu); s.set(3,s.flags.plus(s.reg(3,4,0),0x48u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e961u); s.set(0,s.flags.increment(s.reg(0,4,0),4,true),4,0);
if(s.services.instruction)s.services.instruction(0x22e962u); if(!s.flags.zf)goto L22e810;
L22e968: if(s.services.instruction)s.services.instruction(0x22e968u); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x22e96au); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x20u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e970u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x1cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e976u); if(s.flags.zf)goto L22eaa4;
if(s.services.instruction)s.services.instruction(0x22e97cu); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x2cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e982u); s.xmm[7]=_mm_set_ss(f(r(s.memory,(0x10u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e988u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x28u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22e98eu); s.xmm[7]=_mm_sub_ss(s.xmm[7],s.xmm[5]);
if(s.services.instruction)s.services.instruction(0x22e992u); s.xmm[6]=s.xmm[4];
if(s.services.instruction)s.services.instruction(0x22e995u); s.xmm[6]=_mm_sub_ss(s.xmm[6],_mm_set_ss(f(r(s.memory,(0x24u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x22e99bu); s.xmm[6]=_mm_mul_ss(s.xmm[6],s.xmm[7]);
if(s.services.instruction)s.services.instruction(0x22e99fu); s.xmm[1]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x22e9a2u); s.xmm[1]=_mm_sub_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x22e9a8u); s.xmm[0]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x22e9abu); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x18u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x22e9b1u); s.xmm[7]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x22e9b4u); s.xmm[7]=_mm_mul_ss(s.xmm[7],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22e9b8u); s.flags.scalar_compare(s.xmm[6],s.xmm[7],false);
if(s.services.instruction)s.services.instruction(0x22e9bbu); if(!s.flags.cf&&!s.flags.zf)goto L22eaa4;
if(s.services.instruction)s.services.instruction(0x22e9c1u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x46a0ccu),4)));
if(s.services.instruction)s.services.instruction(0x22e9c9u); s.set(0,r(s.memory,(0xa0u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e9d0u); s.set(1,r(s.memory,(0xa4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e9d7u); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x22e9dbu); s.xmm[3]=_mm_add_ss(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x22e9e1u); s.xmm[2]=_mm_add_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x18u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x22e9e7u); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x22e9ebu); w(s.memory,s.observe,0x22e9ebu,(0x0u+s.gp[0]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22e9efu); s.set(0,r(s.memory,(0x30u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e9f3u); w(s.memory,s.observe,0x22e9f3u,(0x0u+s.gp[1]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22e9f7u); s.set(1,r(s.memory,(0x34u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e9fbu); s.set(2,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e9fdu); w(s.memory,s.observe,0x22e9fdu,(0x0u+s.gp[2]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22e9ffu); s.set(0,r(s.memory,(0x38u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ea03u); w(s.memory,s.observe,0x22ea03u,(0x4u+s.gp[2]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22ea06u); w(s.memory,s.observe,0x22ea06u,(0x8u+s.gp[2]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22ea09u); s.set(0,r(s.memory,(0x9cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ea10u); s.set(2,r(s.memory,(0x3cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ea14u); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22ea16u); w(s.memory,s.observe,0x22ea16u,(0x0u+s.gp[1]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22ea18u); s.set(2,r(s.memory,(0x40u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ea1cu); w(s.memory,s.observe,0x22ea1cu,(0x4u+s.gp[1]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22ea1fu); s.set(2,r(s.memory,(0x44u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ea23u); w(s.memory,s.observe,0x22ea23u,(0x8u+s.gp[1]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22ea26u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x22ea2bu); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x22ea2fu); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22ea33u); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22ea37u); w(s.memory,s.observe,0x22ea37u,(0x0u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22ea3bu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x22ea40u); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22ea44u); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22ea49u); w(s.memory,s.observe,0x22ea49u,(0x4u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22ea4eu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x22ea53u); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22ea57u); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22ea5cu); w(s.memory,s.observe,0x22ea5cu,(0x8u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22ea61u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22ea65u); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x22ea69u); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22ea6du); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22ea71u); w(s.memory,s.observe,0x22ea71u,(0x0u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22ea75u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22ea7au); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22ea7eu); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22ea83u); w(s.memory,s.observe,0x22ea83u,(0x4u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22ea88u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22ea8du); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22ea8eu); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22ea92u); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22ea97u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22ea98u); w(s.memory,s.observe,0x22ea98u,(0x8u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22ea9du); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22ea9eu); s.set(4,s.flags.plus(s.reg(4,4,0),0x7cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x22eaa1u); s.pop();s.gp[4]+=32u;return;
L22eaa4: if(s.services.instruction)s.services.instruction(0x22eaa4u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x10u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22eaaau); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x46a0ccu),4)));
if(s.services.instruction)s.services.instruction(0x22eab2u); s.set(0,r(s.memory,(0xa0u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eab9u); s.set(1,r(s.memory,(0xa4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eac0u); s.xmm[2]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x22eac3u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[5]);
if(s.services.instruction)s.services.instruction(0x22eac7u); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22eacbu); w(s.memory,s.observe,0x22eacbu,(0x0u+s.gp[0]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x22eacfu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x24u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x22ead5u); s.set(0,r(s.memory,(0x54u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ead9u); s.xmm[3]=s.xmm[4];
if(s.services.instruction)s.services.instruction(0x22eadcu); s.xmm[3]=_mm_sub_ss(s.xmm[3],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22eae0u); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22eae4u); w(s.memory,s.observe,0x22eae4u,(0x0u+s.gp[1]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x22eae8u); s.set(1,r(s.memory,(0x58u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eaecu); s.set(2,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22eaeeu); w(s.memory,s.observe,0x22eaeeu,(0x0u+s.gp[2]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22eaf0u); s.set(0,r(s.memory,(0x5cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eaf4u); w(s.memory,s.observe,0x22eaf4u,(0x4u+s.gp[2]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22eaf7u); w(s.memory,s.observe,0x22eaf7u,(0x8u+s.gp[2]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22eafau); s.set(0,r(s.memory,(0x9cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eb01u); s.set(2,r(s.memory,(0x60u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eb05u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[5]);
if(s.services.instruction)s.services.instruction(0x22eb09u); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22eb0du); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22eb0fu); w(s.memory,s.observe,0x22eb0fu,(0x0u+s.gp[1]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22eb11u); s.set(2,r(s.memory,(0x64u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eb15u); w(s.memory,s.observe,0x22eb15u,(0x4u+s.gp[1]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22eb18u); s.set(2,r(s.memory,(0x68u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eb1cu); w(s.memory,s.observe,0x22eb1cu,(0x8u+s.gp[1]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22eb1fu); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x22eb24u); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22eb28u); s.xmm[3]=_mm_add_ss(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22eb2cu); w(s.memory,s.observe,0x22eb2cu,(0x0u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x22eb30u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x22eb35u); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22eb39u); s.xmm[3]=_mm_add_ss(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22eb3eu); w(s.memory,s.observe,0x22eb3eu,(0x4u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x22eb43u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x22eb48u); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22eb4cu); s.xmm[3]=_mm_add_ss(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22eb51u); w(s.memory,s.observe,0x22eb51u,(0x8u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x22eb56u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22eb5au); s.xmm[4]=_mm_add_ss(s.xmm[4],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x22eb5eu); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x22eb62u); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x22eb66u); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22eb6au); w(s.memory,s.observe,0x22eb6au,(0x0u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22eb6eu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22eb73u); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x22eb77u); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x22eb7cu); w(s.memory,s.observe,0x22eb7cu,(0x4u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22eb81u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22eb86u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22eb87u); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x22eb8bu); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22eb8cu); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x22eb90u); w(s.memory,s.observe,0x22eb90u,(0x8u+s.gp[7]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22eb95u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22eb96u); s.set(4,s.flags.plus(s.reg(4,4,0),0x7cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x22eb99u); s.pop();s.gp[4]+=32u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_22eec0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x22eec0u); s.push(0x22eec0u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22eec1u); s.set(2,r(s.memory,(0x28u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eec4u); s.flags.minus(s.reg(2,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x22eec7u); s.push(0x22eec7u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22eec8u); s.push(0x22eec8u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22eec9u); s.push(0x22eec9u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x22eecau); if(s.flags.zf)goto L22ef61;
if(s.services.instruction)s.services.instruction(0x22eed0u); s.set(0,r(s.memory,(0x52be5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eed5u); s.set(6,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eed8u); s.set(0,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eeddu); s.set(7,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eee0u); s.set(0,r(s.memory,(0x24u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eee3u); s.set(3,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eee9u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eeeeu); s.set(0,(0x0u+s.gp[0]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x22eef1u); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22eef4u); s.set(0,r(s.memory,(0x8u+s.gp[0]+s.gp[6]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eef8u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eefdu); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x22ef00u); s.set(0,r(s.memory,(0x8u+s.gp[7]+s.gp[0]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef04u); s.set(0,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef06u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef0bu); s.set(0,s.flags.shift(s.reg(0,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef0eu); s.set(0,r(s.memory,(0x8u+s.gp[0]+s.gp[3]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef12u); s.flags.minus(r(s.memory,(0x58u+s.gp[0]),4),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x22ef16u); if(!s.flags.zf)goto L22ef61;
if(s.services.instruction)s.services.instruction(0x22ef18u); s.set(0,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22ef1au); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef1fu); s.set(0,(0x0u+s.gp[0]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x22ef22u); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef25u); s.set(0,r(s.memory,(0x8u+s.gp[0]+s.gp[6]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef29u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef2eu); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x22ef31u); s.set(0,r(s.memory,(0x8u+s.gp[7]+s.gp[0]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef35u); s.set(0,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef37u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef3cu); s.set(0,s.flags.shift(s.reg(0,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef3fu); s.set(0,r(s.memory,(0x8u+s.gp[0]+s.gp[3]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef43u); s.flags.minus(r(s.memory,(0x58u+s.gp[0]),4),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x22ef47u); if(s.flags.zf)goto L22ef61;
if(s.services.instruction)s.services.instruction(0x22ef49u); s.set(0,r(s.memory,(0x18u+s.gp[4]),2),2,0);
if(s.services.instruction)s.services.instruction(0x22ef4eu); w(s.memory,s.observe,0x22ef4eu,(0xcu+s.gp[4]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x22ef53u); s.set(0,r(s.memory,(0x1cu+s.gp[4]),2),2,0);
if(s.services.instruction)s.services.instruction(0x22ef58u); w(s.memory,s.observe,0x22ef58u,(0x1cu+s.gp[4]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x22ef5du); s.set(6,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22ef5fu); goto L22ef78;
L22ef61: if(s.services.instruction)s.services.instruction(0x22ef61u); s.set(0,r(s.memory,(0x1cu+s.gp[4]),2),2,0);
if(s.services.instruction)s.services.instruction(0x22ef66u); s.set(6,r(s.memory,(0x24u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef69u); w(s.memory,s.observe,0x22ef69u,(0xcu+s.gp[4]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x22ef6eu); s.set(0,r(s.memory,(0x18u+s.gp[4]),2),2,0);
if(s.services.instruction)s.services.instruction(0x22ef73u); w(s.memory,s.observe,0x22ef73u,(0x1cu+s.gp[4]),2,s.reg(0,2,0));
L22ef78: if(s.services.instruction)s.services.instruction(0x22ef78u); s.set(0,r(s.memory,(0x38u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef7cu); s.push(0x22ef7cu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22ef7du); s.set(0,r(s.memory,(0x38u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef81u); s.push(0x22ef81u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22ef82u); s.set(0,r(s.memory,(0x38u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef86u); s.push(0x22ef86u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22ef87u); s.set(0,r(s.memory,(0x38u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef8bu); s.push(0x22ef8bu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22ef8cu); s.set(0,r(s.memory,(0x38u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef90u); s.push(0x22ef90u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22ef91u); s.set(0,r(s.memory,(0x38u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef95u); s.push(0x22ef95u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22ef96u); s.set(0,r(s.memory,(0x38u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef9au); s.push(0x22ef9au,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22ef9bu); s.set(0,r(s.memory,(0x28u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ef9fu); s.push(0x22ef9fu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22efa0u); s.set(0,r(s.memory,(0x3cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22efa4u); s.push(0x22efa4u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22efa5u); s.set(0,r(s.memory,(0x38u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22efa9u); s.flags.minus(s.reg(2,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x22efacu); s.push(0x22efacu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22efadu); s.set(0,sign_extend(r(s.memory,(0xbu+s.gp[1]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x22efb1u); s.set(2,!s.flags.zf,1,0);
if(s.services.instruction)s.services.instruction(0x22efb4u); s.push(0x22efb4u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22efb5u); s.push(0x22efb5u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22efb6u); s.set(0,(0x5cu+s.gp[1]),4,0);
if(s.services.instruction)s.services.instruction(0x22efb9u); s.set(1,sign_extend(r(s.memory,(0xdu+s.gp[1]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x22efbdu); s.push(0x22efbdu,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22efbeu); s.push(0x22efbeu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22efbfu); s.push(0x22efbfu,0x22efc4u);run_22efd0(s);
if(s.services.instruction)s.services.instruction(0x22efc4u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22efc5u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22efc6u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22efc7u); s.set(1,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22efc8u); s.pop();s.gp[4]+=40u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_22efd0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x22efd0u); s.set(4,s.flags.minus(s.reg(4,4,0),0x14u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22efd3u); s.set(1,r(s.memory,(0x28u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22efd7u); s.set(2,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22efd9u); w(s.memory,s.observe,0x22efd9u,(0x8u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22efddu); s.set(2,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22efe0u); s.set(1,r(s.memory,(0x8u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22efe3u); s.push(0x22efe3u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22efe4u); w(s.memory,s.observe,0x22efe4u,(0x14u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22efe8u); s.set(1,r(s.memory,(0x28u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x22efecu); s.set(3,s.flags.logic((s.reg(3,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22efefu); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x22eff1u); s.push(0x22eff1u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x22eff2u); s.set(5,r(s.memory,(0x20u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22eff6u); w(s.memory,s.observe,0x22eff6u,(0x14u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22effau); w(s.memory,s.observe,0x22effau,(0x20u+s.gp[4]),2,s.reg(3,2,0));
if(s.services.instruction)s.services.instruction(0x22efffu); if(!s.flags.zf)goto L22f047;
if(s.services.instruction)s.services.instruction(0x22f001u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x48b874u),4)));
if(s.services.instruction)s.services.instruction(0x22f009u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22f00du); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22f011u); s.xmm[1]=_mm_add_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x10u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x22f017u); w(s.memory,s.observe,0x22f017u,(0x10u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22f01du); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22f022u); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22f026u); s.xmm[1]=_mm_add_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x22f02cu); w(s.memory,s.observe,0x22f02cu,(0x14u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22f032u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x22f037u); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x22f03bu); s.xmm[1]=_mm_add_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x18u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x22f041u); w(s.memory,s.observe,0x22f041u,(0x18u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
L22f047: if(s.services.instruction)s.services.instruction(0x22f047u); s.push(0x22f047u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22f048u); s.push(0x22f048u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x22f049u); s.set(2,(0x18u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x22f04du); s.set(6,(0x10u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x22f051u); s.push(0x22f051u,0x22f056u);s.required(0x22f051u,0x11e600u,false);
if(s.services.instruction)s.services.instruction(0x22f056u); s.flags.minus(r(s.memory,(0x14u+s.gp[4]),2),s.reg(3,2,0),2);
if(s.services.instruction)s.services.instruction(0x22f05bu); s.set(6,r(s.memory,(0x52be5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f061u); if(!s.flags.zf)goto L22f09a;
if(s.services.instruction)s.services.instruction(0x22f063u); s.set(2,r(s.memory,(0x44u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f066u); s.set(0,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22f068u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f06du); s.set(0,(0x0u+s.gp[0]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x22f070u); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22f073u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(2,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f075u); s.set(0,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f078u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f07du); s.set(2,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x22f080u); s.set(0,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f085u); s.set(1,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f088u); s.set(0,r(s.memory,(0x8u+s.gp[1]+s.gp[2]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f08cu); s.set(2,r(s.memory,(0x28u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f08fu); s.set(0,r(s.memory,(0x2cu+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f092u); w(s.memory,s.observe,0x22f092u,(0x10u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22f096u); w(s.memory,s.observe,0x22f096u,(0x14u+s.gp[4]),4,s.reg(0,4,0));
L22f09a: if(s.services.instruction)s.services.instruction(0x22f09au); s.set(1,(0x28u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x22f09eu); s.push(0x22f09eu,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22f09fu); s.set(7,(0x1cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x22f0a3u); s.set(1,(0x14u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x22f0a7u); s.push(0x22f0a7u,0x22f0acu);s.required(0x22f0a7u,0x11e850u,false);
if(s.services.instruction)s.services.instruction(0x22f0acu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x22f0aeu); if(s.flags.zf)goto L22f0ba;
if(s.services.instruction)s.services.instruction(0x22f0b0u); s.set(2,r(s.memory,(0x28u+s.gp[4]),2),2,0);
if(s.services.instruction)s.services.instruction(0x22f0b5u); w(s.memory,s.observe,0x22f0b5u,(0x40u+s.gp[4]),2,s.reg(2,2,0));
L22f0ba: if(s.services.instruction)s.services.instruction(0x22f0bau); s.flags.minus(s.reg(5,4,0),s.reg(3,4,0),4);
if(s.services.instruction)s.services.instruction(0x22f0bcu); if(s.flags.zf)goto L22f105;
if(s.services.instruction)s.services.instruction(0x22f0beu); s.flags.minus(r(s.memory,(0x2cu+s.gp[4]),4),s.reg(3,4,0),4);
if(s.services.instruction)s.services.instruction(0x22f0c2u); if(!s.flags.zf)goto L22f105;
if(s.services.instruction)s.services.instruction(0x22f0c4u); s.set(7,r(s.memory,(0x44u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0c7u); s.set(1,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0cdu); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0d0u); s.set(1,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0d6u); s.set(5,s.flags.logic((s.reg(5,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0dcu); s.set(0,(0x0u+s.gp[5]+s.gp[5]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x22f0e0u); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0e3u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(7,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0e5u); s.set(0,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0e8u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0edu); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x22f0f0u); s.set(0,r(s.memory,(0x8u+s.gp[2]+s.gp[0]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0f4u); s.set(0,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0f6u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0fbu); s.set(0,s.flags.shift(s.reg(0,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22f0feu); s.set(0,r(s.memory,(0x8u+s.gp[0]+s.gp[1]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f102u); s.set(3,r(s.memory,(0x58u+s.gp[0]),4),4,0);
L22f105: if(s.services.instruction)s.services.instruction(0x22f105u); s.set(0,r(s.memory,(0x44u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f109u); s.set(0,s.flags.minus(s.reg(0,4,0),0x0u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22f10cu); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22f10du); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22f10eu); if(s.flags.zf)goto L22f123;
if(s.services.instruction)s.services.instruction(0x22f110u); s.set(0,s.flags.minus(s.reg(0,4,0),0x2u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22f113u); if(s.flags.zf)goto L22f11c;
if(s.services.instruction)s.services.instruction(0x22f115u); s.set(0,0xdu,4,0);
if(s.services.instruction)s.services.instruction(0x22f11au); goto L22f12a;
L22f11c: if(s.services.instruction)s.services.instruction(0x22f11cu); s.set(0,0xeu,4,0);
if(s.services.instruction)s.services.instruction(0x22f121u); goto L22f12a;
L22f123: if(s.services.instruction)s.services.instruction(0x22f123u); s.set(0,r(s.memory,(0x28u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f127u); s.set(0,s.flags.plus(s.reg(0,4,0),0xau,4),4,0);
L22f12a: if(s.services.instruction)s.services.instruction(0x22f12au); s.set(2,r(s.memory,(0x54u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f12eu); s.set(1,r(s.memory,(0x50u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f132u); s.push(0x22f132u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22f133u); s.set(2,r(s.memory,(0x50u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f137u); s.push(0x22f137u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22f138u); s.set(1,r(s.memory,(0x50u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f13cu); s.push(0x22f13cu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22f13du); s.set(2,r(s.memory,(0x50u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f141u); s.push(0x22f141u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22f142u); s.set(1,r(s.memory,(0x50u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f146u); s.push(0x22f146u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22f147u); s.set(2,r(s.memory,(0x3cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f14bu); s.push(0x22f14bu,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22f14cu); s.set(1,r(s.memory,(0x4cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f150u); s.push(0x22f150u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22f151u); s.set(2,r(s.memory,(0x4cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f155u); s.push(0x22f155u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22f156u); s.set(0,r(s.memory,(0x58u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22f15au); s.push(0x22f15au,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22f15bu); s.push(0x22f15bu,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22f15cu); s.push(0x22f15cu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22f15du); s.push(0x22f15du,0x0u);
if(s.services.instruction)s.services.instruction(0x22f15fu); s.push(0x22f15fu,0x22f164u);s.required(0x22f15fu,0x18b1d0u,false);
if(s.services.instruction)s.services.instruction(0x22f164u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22f165u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22f166u); s.set(4,s.flags.plus(s.reg(4,4,0),0x14u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22f169u); s.pop();s.gp[4]+=56u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_22fe30(MathState &s) {
if(s.services.instruction)s.services.instruction(0x22fe30u); s.set(0,r(s.memory,(0x11u+s.gp[1]),1),1,0);
if(s.services.instruction)s.services.instruction(0x22fe33u); s.set(4,s.flags.minus(s.reg(4,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22fe36u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x22fe38u); if(s.flags.zf)goto L22fe40;
if(s.services.instruction)s.services.instruction(0x22fe3au); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x18u+s.gp[1]),4))),false);
if(s.services.instruction)s.services.instruction(0x22fe3eu); if(s.flags.cf||s.flags.zf)goto L22fe98;
L22fe40: if(s.services.instruction)s.services.instruction(0x22fe40u); s.flags.minus(r(s.memory,(0x94u+s.gp[1]),4),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x22fe47u); if(s.flags.zf)goto L22fe8f;
if(s.services.instruction)s.services.instruction(0x22fe49u); s.set(2,r(s.memory,(0x51df54u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fe4fu); s.set(0,r(s.memory,(0x8u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fe52u); s.set(2,sign_extend(r(s.memory,(0x2u+s.gp[2]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x22fe56u); s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],si(s.reg(2,4,0)));
if(s.services.instruction)s.services.instruction(0x22fe5au); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x46a16cu),4))));
if(s.services.instruction)s.services.instruction(0x22fe62u); w(s.memory,s.observe,0x22fe62u,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x22fe67u); s.fp.push(ext(f(r(s.memory,(0x0u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x22fe6au); w(s.memory,s.observe,0x22fe6au,(0x4u+s.gp[4]),4,integer_spill_observed(s));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x22fe6eu); s.set(0,s.flags.minus(s.reg(0,4,0),r(s.memory,(0x94u+s.gp[1]),4),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fe74u); s.flags.minus(s.reg(0,4,0),r(s.memory,(0x4u+s.gp[4]),4),4);
if(s.services.instruction)s.services.instruction(0x22fe78u); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L22fe8f;
if(s.services.instruction)s.services.instruction(0x22fe7au); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x98u+s.gp[1]),4)));
if(s.services.instruction)s.services.instruction(0x22fe82u); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x46a35cu),4))));
if(s.services.instruction)s.services.instruction(0x22fe8au); s.flags.scalar_compare(s.xmm[0],s.xmm[1],false);
if(s.services.instruction)s.services.instruction(0x22fe8du); if(s.flags.cf||s.flags.zf)goto L22fe98;
L22fe8f: if(s.services.instruction)s.services.instruction(0x22fe8fu); w(s.memory,s.observe,0x22fe8fu,(0x11u+s.gp[1]),1,0x1u);
if(s.services.instruction)s.services.instruction(0x22fe93u); w(s.memory,s.observe,0x22fe93u,(0x18u+s.gp[1]),4,u(_mm_cvtss_f32(s.xmm[0])));
L22fe98: if(s.services.instruction)s.services.instruction(0x22fe98u); s.set(4,s.flags.plus(s.reg(4,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22fe9bu); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_22fea0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x22fea0u); s.push(0x22fea0u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x22fea1u); s.set(5,s.reg(4,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22fea3u); s.set(4,s.flags.logic((s.reg(4,4,0))&(0xfffffff8u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fea6u); s.set(4,s.flags.minus(s.reg(4,4,0),0xd0u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22feacu); s.push(0x22feacu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22feadu); s.push(0x22feadu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x22feaeu); s.set(0,r(s.memory,(0x24u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22feb1u); s.set(1,r(s.memory,(0x52be5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22feb7u); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22febau); s.set(1,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fec0u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fec5u); s.set(0,(0x0u+s.gp[0]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x22fec8u); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22fecbu); s.set(0,r(s.memory,(0x8u+s.gp[0]+s.gp[2]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fecfu); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fed2u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fed7u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x46a0ccu),4)));
if(s.services.instruction)s.services.instruction(0x22fedfu); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x22fee2u); s.set(2,r(s.memory,(0x8u+s.gp[2]+s.gp[0]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fee6u); s.set(0,(0x68u+s.gp[3]),4,0);
if(s.services.instruction)s.services.instruction(0x22fee9u); s.set(1,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22feebu); w(s.memory,s.observe,0x22feebu,(0xbcu+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22fef2u); s.set(1,r(s.memory,(0x4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fef5u); s.set(0,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fef8u); w(s.memory,s.observe,0x22fef8u,(0xc0u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22feffu); w(s.memory,s.observe,0x22feffu,(0xc4u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22ff06u); s.set(1,(0x5cu+s.gp[3]),4,0);
if(s.services.instruction)s.services.instruction(0x22ff09u); s.set(0,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ff0bu); w(s.memory,s.observe,0x22ff0bu,(0xc8u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22ff12u); s.set(0,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ff15u); s.set(1,r(s.memory,(0x8u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ff18u); s.set(6,s.flags.logic((s.reg(6,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ff1bu); w(s.memory,s.observe,0x22ff1bu,(0xccu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22ff22u); s.set(0,r(s.memory,(0x11u+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x22ff25u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x22ff27u); w(s.memory,s.observe,0x22ff27u,(0xb0u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22ff2eu); w(s.memory,s.observe,0x22ff2eu,(0x9cu+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22ff35u); w(s.memory,s.observe,0x22ff35u,(0xd4u+s.gp[4]),4,0x600022bu);
if(s.services.instruction)s.services.instruction(0x22ff40u); w(s.memory,s.observe,0x22ff40u,(0xd0u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22ff47u); if(s.flags.zf)goto L230355;
if(s.services.instruction)s.services.instruction(0x22ff4du); s.set(0,r(s.memory,(0x48b892u),1),1,0);
if(s.services.instruction)s.services.instruction(0x22ff52u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x22ff54u); if(s.flags.zf)goto L230355;
if(s.services.instruction)s.services.instruction(0x22ff5au); s.flags.minus(r(s.memory,(0x1cu+s.gp[3]),1),0x2u,1);
if(s.services.instruction)s.services.instruction(0x22ff5eu); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L230355;
if(s.services.instruction)s.services.instruction(0x22ff64u); s.set(0,r(s.memory,(0x1du+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x22ff67u); s.flags.minus(s.reg(0,1,0),0x2u,1);
if(s.services.instruction)s.services.instruction(0x22ff69u); if(s.flags.sf==s.flags.of)goto L22ff72;
if(s.services.instruction)s.services.instruction(0x22ff6bu); s.set(1,0x2u,4,0);
if(s.services.instruction)s.services.instruction(0x22ff70u); goto L22ff7e;
L22ff72: if(s.services.instruction)s.services.instruction(0x22ff72u); s.flags.minus(s.reg(0,1,0),0x3u,1);
if(s.services.instruction)s.services.instruction(0x22ff74u); s.set(1,0x3u,4,0);
if(s.services.instruction)s.services.instruction(0x22ff79u); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L22ff7e;
if(s.services.instruction)s.services.instruction(0x22ff7bu); s.set(1,sign_extend(s.reg(0,1,0),1),4,0);
L22ff7e: if(s.services.instruction)s.services.instruction(0x22ff7eu); s.set(7,sign_extend(s.reg(0,1,0),1),4,0);
if(s.services.instruction)s.services.instruction(0x22ff81u); s.flags.minus(s.reg(1,4,0),s.reg(7,4,0),4);
if(s.services.instruction)s.services.instruction(0x22ff83u); if(!s.flags.zf)goto L22ffb4;
if(s.services.instruction)s.services.instruction(0x22ff85u); s.flags.minus(s.reg(0,1,0),0x3u,1);
if(s.services.instruction)s.services.instruction(0x22ff87u); if(!s.flags.zf)goto L22ff90;
if(s.services.instruction)s.services.instruction(0x22ff89u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x48u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22ff8eu); goto L22ff95;
L22ff90: if(s.services.instruction)s.services.instruction(0x22ff90u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4cu+s.gp[3]),4)));
L22ff95: if(s.services.instruction)s.services.instruction(0x22ff95u); s.flags.minus(s.reg(0,1,0),0x2u,1);
if(s.services.instruction)s.services.instruction(0x22ff97u); if(!s.flags.zf)goto L230355;
if(s.services.instruction)s.services.instruction(0x22ff9du); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x22ffa0u); if(!s.flags.cf&&!s.flags.zf)goto L22ffb4;
if(s.services.instruction)s.services.instruction(0x22ffa2u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x18u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22ffa7u); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x46a130u),4))),false);
if(s.services.instruction)s.services.instruction(0x22ffaeu); if(s.flags.cf||s.flags.zf)goto L230355;
L22ffb4: if(s.services.instruction)s.services.instruction(0x22ffb4u); s.xmm[5]=_mm_xor_ps(s.xmm[5],s.xmm[5]);
if(s.services.instruction)s.services.instruction(0x22ffb7u); s.flags.scalar_compare(s.xmm[5],_mm_set_ss(f(r(s.memory,(0x18u+s.gp[3]),4))),false);
if(s.services.instruction)s.services.instruction(0x22ffbbu); if(s.flags.cf||s.flags.zf)goto L22ffc5;
if(s.services.instruction)s.services.instruction(0x22ffbdu); w(s.memory,s.observe,0x22ffbdu,(0x8u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x22ffc3u); goto L22ffe3;
L22ffc5: if(s.services.instruction)s.services.instruction(0x22ffc5u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x18u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22ffcau); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x22ffd2u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x22ffd5u); if(!s.flags.cf&&!s.flags.zf)goto L22ffda;
if(s.services.instruction)s.services.instruction(0x22ffd7u); s.xmm[0]=s.xmm[1];
L22ffda: if(s.services.instruction)s.services.instruction(0x22ffdau); w(s.memory,s.observe,0x22ffdau,(0x8u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x22ffe0u); s.xmm[5]=s.xmm[0];
L22ffe3: if(s.services.instruction)s.services.instruction(0x22ffe3u); s.set(7,(0x68u+s.gp[3]),4,0);
if(s.services.instruction)s.services.instruction(0x22ffe6u); s.set(0,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22ffe8u); s.set(1,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22ffeau); w(s.memory,s.observe,0x22ffeau,(0x30u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22ffeeu); s.set(1,r(s.memory,(0x4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fff1u); s.set(0,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22fff4u); w(s.memory,s.observe,0x22fff4u,(0x38u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22fff8u); s.set(0,(0x50u+s.gp[3]),4,0);
if(s.services.instruction)s.services.instruction(0x22fffbu); w(s.memory,s.observe,0x22fffbu,(0xb8u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230002u); w(s.memory,s.observe,0x230002u,(0x1cu+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230006u); w(s.memory,s.observe,0x230006u,(0xb4u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x23000du); w(s.memory,s.observe,0x23000du,(0x18u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230011u); w(s.memory,s.observe,0x230011u,(0x10u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230015u); w(s.memory,s.observe,0x230015u,(0x14u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230019u); w(s.memory,s.observe,0x230019u,(0x34u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x23001du); s.push(0x23001du,0x230022u);run_18bbc0(s);
if(s.services.instruction)s.services.instruction(0x230022u); s.set(1,r(s.memory,(0x474d04u),4),4,0);
if(s.services.instruction)s.services.instruction(0x230028u); w(s.memory,s.observe,0x230028u,(0x3cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23002cu); s.set(0,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23002eu); w(s.memory,s.observe,0x23002eu,(0x40u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230032u); s.set(0,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230035u); s.set(1,r(s.memory,(0x8u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230038u); w(s.memory,s.observe,0x230038u,(0x48u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x23003cu); s.set(1,r(s.memory,(0x2cu+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23003fu); w(s.memory,s.observe,0x23003fu,(0x44u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230043u); s.set(0,r(s.memory,(0x28u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230046u); s.set(2,r(s.memory,(0x2eu+s.gp[3]),2),2,0);
if(s.services.instruction)s.services.instruction(0x23004au); w(s.memory,s.observe,0x23004au,(0x50u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x23004eu); w(s.memory,s.observe,0x23004eu,(0x4cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230052u); s.set(0,r(s.memory,(0x2cu+s.gp[3]),2),2,0);
if(s.services.instruction)s.services.instruction(0x230056u); w(s.memory,s.observe,0x230056u,(0x94u+s.gp[4]),2,s.reg(2,2,0));
if(s.services.instruction)s.services.instruction(0x23005eu); s.set(1,(0x14u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230062u); s.push(0x230062u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230063u); w(s.memory,s.observe,0x230063u,(0x24u+s.gp[4]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x230068u); s.set(2,(0x14u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23006cu); s.push(0x23006cu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x23006du); s.set(0,(0x20u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230071u); s.push(0x230071u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230072u); s.set(1,(0xc0u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230079u); s.push(0x230079u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x23007au); s.set(1,r(s.memory,(0xa4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230081u); s.set(2,(0x2cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230085u); s.push(0x230085u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x230086u); s.set(2,r(s.memory,(0x34u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23008au); s.set(0,(0xccu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230091u); s.push(0x230091u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230092u); s.push(0x230092u,0x0u);
if(s.services.instruction)s.services.instruction(0x230094u); s.push(0x230094u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230095u); s.push(0x230095u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x230096u); s.push(0x230096u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x230097u); s.set(1,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x230099u); s.push(0x230099u,0x23009eu);run_22eec0(s);
if(s.services.instruction)s.services.instruction(0x23009eu); s.set(0,r(s.memory,(0xb8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2300a5u); s.set(7,s.flags.logic((s.reg(7,4,0))^(s.reg(7,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x2300a7u); s.flags.minus(s.reg(0,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2300a9u); if(s.flags.zf)goto L2300e0;
if(s.services.instruction)s.services.instruction(0x2300abu); w(s.memory,s.observe,0x2300abu,(0xa4u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2300b2u); s.set(0,(0xa0u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2300b9u); s.push(0x2300b9u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2300bau); s.set(0,(0x34u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2300beu); w(s.memory,s.observe,0x2300beu,(0xa4u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x2300c5u); w(s.memory,s.observe,0x2300c5u,(0xacu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x2300ceu); w(s.memory,s.observe,0x2300ceu,(0xb0u+s.gp[4]),4,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x2300d5u); s.push(0x2300d5u,0x2300dau);run_18c440(s);
if(s.services.instruction)s.services.instruction(0x2300dau); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
L2300e0: if(s.services.instruction)s.services.instruction(0x2300e0u); s.set(0,r(s.memory,(0xb4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2300e7u); s.flags.minus(s.reg(0,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2300e9u); if(s.flags.zf)goto L230120;
if(s.services.instruction)s.services.instruction(0x2300ebu); s.set(1,(0xa0u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2300f2u); w(s.memory,s.observe,0x2300f2u,(0xa4u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2300f9u); s.push(0x2300f9u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x2300fau); s.set(0,(0x34u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2300feu); w(s.memory,s.observe,0x2300feu,(0xa4u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230105u); w(s.memory,s.observe,0x230105u,(0xacu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x23010eu); w(s.memory,s.observe,0x23010eu,(0xb0u+s.gp[4]),4,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x230115u); s.push(0x230115u,0x23011au);run_18c440(s);
if(s.services.instruction)s.services.instruction(0x23011au); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
L230120: if(s.services.instruction)s.services.instruction(0x230120u); s.set(0,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230124u); s.flags.minus(s.reg(0,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x230126u); if(s.flags.zf)goto L23015d;
if(s.services.instruction)s.services.instruction(0x230128u); s.set(2,(0xa0u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23012fu); w(s.memory,s.observe,0x23012fu,(0xa4u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230136u); s.push(0x230136u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x230137u); s.set(0,(0x34u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23013bu); w(s.memory,s.observe,0x23013bu,(0xa4u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230142u); w(s.memory,s.observe,0x230142u,(0xacu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x23014bu); w(s.memory,s.observe,0x23014bu,(0xb0u+s.gp[4]),4,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x230152u); s.push(0x230152u,0x230157u);run_18c440(s);
if(s.services.instruction)s.services.instruction(0x230157u); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
L23015d: if(s.services.instruction)s.services.instruction(0x23015du); s.flags.minus(r(s.memory,(0x9cu+s.gp[3]),2),s.reg(6,2,0),2);
if(s.services.instruction)s.services.instruction(0x230164u); if(!s.flags.zf)goto L230332;
if(s.services.instruction)s.services.instruction(0x23016au); s.set(2,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23016eu); s.flags.minus(s.reg(2,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x230170u); if(s.flags.zf)goto L230202;
if(s.services.instruction)s.services.instruction(0x230176u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x230178u); s.set(1,0x18u,4,0);
if(s.services.instruction)s.services.instruction(0x23017du); s.set(7,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230181u); while(s.gp[1]){const auto x=s.gp[0];w(s.memory,s.observe,0x230181u,s.gp[7],4,x);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x230181u);}
if(s.services.instruction)s.services.instruction(0x230183u); s.set(1,0xff808080u,4,0);
if(s.services.instruction)s.services.instruction(0x230188u); w(s.memory,s.observe,0x230188u,(0x84u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x23018fu); w(s.memory,s.observe,0x23018fu,(0x88u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230196u); w(s.memory,s.observe,0x230196u,(0x64u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23019au); w(s.memory,s.observe,0x23019au,(0x68u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23019eu); w(s.memory,s.observe,0x23019eu,(0x6cu+s.gp[4]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x2301a3u); w(s.memory,s.observe,0x2301a3u,(0x60u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2301a7u); w(s.memory,s.observe,0x2301a7u,(0x8cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2301aeu); w(s.memory,s.observe,0x2301aeu,(0x30u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2301b2u); s.set(1,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2301b6u); s.set(0,(0xbcu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2301bdu); s.push(0x2301bdu,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x2301beu); w(s.memory,s.observe,0x2301beu,(0x4cu+s.gp[4]),2,s.reg(6,2,0));
if(s.services.instruction)s.services.instruction(0x2301c3u); w(s.memory,s.observe,0x2301c3u,(0x3cu+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x2301c7u); w(s.memory,s.observe,0x2301c7u,(0x44u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x2301cbu); w(s.memory,s.observe,0x2301cbu,(0x40u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x2301cfu); w(s.memory,s.observe,0x2301cfu,(0x48u+s.gp[4]),2,s.reg(6,2,0));
if(s.services.instruction)s.services.instruction(0x2301d4u); w(s.memory,s.observe,0x2301d4u,(0x38u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x2301d8u); w(s.memory,s.observe,0x2301d8u,(0x54u+s.gp[4]),4,0x1u);
if(s.services.instruction)s.services.instruction(0x2301e0u); w(s.memory,s.observe,0x2301e0u,(0x50u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2301e4u); w(s.memory,s.observe,0x2301e4u,(0x74u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x2301eau); w(s.memory,s.observe,0x2301eau,(0x78u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x2301f0u); s.push(0x2301f0u,0x2301f5u);run_178e30(s);
if(s.services.instruction)s.services.instruction(0x2301f5u); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x2301fbu); w(s.memory,s.observe,0x2301fbu,(0xb0u+s.gp[4]),4,s.reg(0,4,0));
L230202: if(s.services.instruction)s.services.instruction(0x230202u); s.set(2,r(s.memory,(0x18u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230206u); s.flags.minus(s.reg(2,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x230208u); if(s.flags.zf)goto L23029a;
if(s.services.instruction)s.services.instruction(0x23020eu); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x230210u); s.set(1,0x18u,4,0);
if(s.services.instruction)s.services.instruction(0x230215u); s.set(7,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230219u); while(s.gp[1]){const auto x=s.gp[0];w(s.memory,s.observe,0x230219u,s.gp[7],4,x);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x230219u);}
if(s.services.instruction)s.services.instruction(0x23021bu); w(s.memory,s.observe,0x23021bu,(0x64u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23021fu); w(s.memory,s.observe,0x23021fu,(0x68u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230223u); w(s.memory,s.observe,0x230223u,(0x6cu+s.gp[4]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x230228u); w(s.memory,s.observe,0x230228u,(0x60u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23022cu); w(s.memory,s.observe,0x23022cu,(0x8cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230233u); w(s.memory,s.observe,0x230233u,(0x30u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230237u); s.set(1,0xff808080u,4,0);
if(s.services.instruction)s.services.instruction(0x23023cu); w(s.memory,s.observe,0x23023cu,(0x34u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x230240u); s.set(0,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230244u); s.set(2,(0xbcu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23024bu); s.push(0x23024bu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23024cu); w(s.memory,s.observe,0x23024cu,(0x4cu+s.gp[4]),2,s.reg(6,2,0));
if(s.services.instruction)s.services.instruction(0x230251u); w(s.memory,s.observe,0x230251u,(0x3cu+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230255u); w(s.memory,s.observe,0x230255u,(0x44u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230259u); w(s.memory,s.observe,0x230259u,(0x40u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x23025du); w(s.memory,s.observe,0x23025du,(0x48u+s.gp[4]),2,s.reg(6,2,0));
if(s.services.instruction)s.services.instruction(0x230262u); w(s.memory,s.observe,0x230262u,(0x88u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230269u); w(s.memory,s.observe,0x230269u,(0x8cu+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230270u); w(s.memory,s.observe,0x230270u,(0x54u+s.gp[4]),4,0x1u);
if(s.services.instruction)s.services.instruction(0x230278u); w(s.memory,s.observe,0x230278u,(0x50u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x23027cu); w(s.memory,s.observe,0x23027cu,(0x74u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x230282u); w(s.memory,s.observe,0x230282u,(0x78u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x230288u); s.push(0x230288u,0x23028du);run_178e30(s);
if(s.services.instruction)s.services.instruction(0x23028du); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x230293u); w(s.memory,s.observe,0x230293u,(0x9cu+s.gp[4]),4,s.reg(0,4,0));
L23029a: if(s.services.instruction)s.services.instruction(0x23029au); s.set(2,r(s.memory,(0x14u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23029eu); s.flags.minus(s.reg(2,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2302a0u); if(s.flags.zf)goto L230332;
if(s.services.instruction)s.services.instruction(0x2302a6u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x2302a8u); s.set(1,0x18u,4,0);
if(s.services.instruction)s.services.instruction(0x2302adu); s.set(7,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2302b1u); while(s.gp[1]){const auto x=s.gp[0];w(s.memory,s.observe,0x2302b1u,s.gp[7],4,x);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x2302b1u);}
if(s.services.instruction)s.services.instruction(0x2302b3u); s.set(1,0xff808080u,4,0);
if(s.services.instruction)s.services.instruction(0x2302b8u); w(s.memory,s.observe,0x2302b8u,(0x34u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x2302bcu); w(s.memory,s.observe,0x2302bcu,(0x84u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x2302c3u); w(s.memory,s.observe,0x2302c3u,(0x88u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x2302cau); s.set(2,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2302ceu); s.set(1,(0xbcu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2302d5u); s.push(0x2302d5u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x2302d6u); w(s.memory,s.observe,0x2302d6u,(0x4cu+s.gp[4]),2,s.reg(6,2,0));
if(s.services.instruction)s.services.instruction(0x2302dbu); w(s.memory,s.observe,0x2302dbu,(0x3cu+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x2302dfu); w(s.memory,s.observe,0x2302dfu,(0x68u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2302e3u); w(s.memory,s.observe,0x2302e3u,(0x6cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2302e7u); w(s.memory,s.observe,0x2302e7u,(0x70u+s.gp[4]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x2302ecu); w(s.memory,s.observe,0x2302ecu,(0x44u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x2302f0u); w(s.memory,s.observe,0x2302f0u,(0x40u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x2302f4u); w(s.memory,s.observe,0x2302f4u,(0x48u+s.gp[4]),2,s.reg(6,2,0));
if(s.services.instruction)s.services.instruction(0x2302f9u); w(s.memory,s.observe,0x2302f9u,(0x64u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2302fdu); w(s.memory,s.observe,0x2302fdu,(0x90u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230304u); w(s.memory,s.observe,0x230304u,(0x34u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230308u); w(s.memory,s.observe,0x230308u,(0x54u+s.gp[4]),4,0x1u);
if(s.services.instruction)s.services.instruction(0x230310u); w(s.memory,s.observe,0x230310u,(0x50u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230314u); w(s.memory,s.observe,0x230314u,(0x74u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x23031au); w(s.memory,s.observe,0x23031au,(0x78u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x230320u); s.push(0x230320u,0x230325u);run_178e30(s);
if(s.services.instruction)s.services.instruction(0x230325u); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x23032bu); w(s.memory,s.observe,0x23032bu,(0x9cu+s.gp[4]),4,s.reg(0,4,0));
L230332: if(s.services.instruction)s.services.instruction(0x230332u); s.set(0,r(s.memory,(0x51df54u),4),4,0);
if(s.services.instruction)s.services.instruction(0x230337u); s.set(1,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23033au); w(s.memory,s.observe,0x23033au,(0x94u+s.gp[3]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230340u); w(s.memory,s.observe,0x230340u,(0x98u+s.gp[3]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x230348u); w(s.memory,s.observe,0x230348u,(0x1cu+s.gp[3]),1,0x0u);
if(s.services.instruction)s.services.instruction(0x23034cu); w(s.memory,s.observe,0x23034cu,(0x10u+s.gp[3]),1,0x0u);
if(s.services.instruction)s.services.instruction(0x230350u); goto L23076e;
L230355: if(s.services.instruction)s.services.instruction(0x230355u); s.set(0,r(s.memory,(0xcu+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x230358u); s.flags.minus(s.reg(0,1,0),0x1u,1);
if(s.services.instruction)s.services.instruction(0x23035au); s.set(2,r(s.memory,(0x51df54u),4),4,0);
if(s.services.instruction)s.services.instruction(0x230360u); if(!s.flags.zf)goto L23039f;
if(s.services.instruction)s.services.instruction(0x230362u); s.set(1,sign_extend(r(s.memory,(0x1eu+s.gp[3]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x230366u); s.xmm[0]=_mm_cvtsi32_ss(s.xmm[0],si(s.reg(1,4,0)));
if(s.services.instruction)s.services.instruction(0x23036au); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[2]),4))));
if(s.services.instruction)s.services.instruction(0x23036fu); s.flags.scalar_compare(s.xmm[0],s.xmm[1],false);
if(s.services.instruction)s.services.instruction(0x230372u); if(s.flags.cf||s.flags.zf)goto L23039f;
if(s.services.instruction)s.services.instruction(0x230374u); s.flags.minus(r(s.memory,(0x30u+s.gp[3]),4),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x230377u); if(!s.flags.zf)goto L230388;
if(s.services.instruction)s.services.instruction(0x230379u); s.flags.minus(r(s.memory,(0x34u+s.gp[3]),4),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x23037cu); if(!s.flags.zf)goto L230388;
if(s.services.instruction)s.services.instruction(0x23037eu); s.flags.minus(r(s.memory,(0x38u+s.gp[3]),4),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x230381u); if(!s.flags.zf)goto L230388;
if(s.services.instruction)s.services.instruction(0x230383u); s.flags.minus(r(s.memory,(0x3cu+s.gp[3]),4),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x230386u); if(s.flags.zf)goto L23039f;
L230388: if(s.services.instruction)s.services.instruction(0x230388u); s.push(0x230388u,0x1u);
if(s.services.instruction)s.services.instruction(0x23038au); s.set(0,0x1u,1,0);
if(s.services.instruction)s.services.instruction(0x23038cu); s.set(6,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x23038eu); s.push(0x23038eu,0x230393u);run_230a60(s);
if(s.services.instruction)s.services.instruction(0x230393u); w(s.memory,s.observe,0x230393u,(0x1du+s.gp[3]),1,0xffu);
if(s.services.instruction)s.services.instruction(0x230397u); s.set(6,s.flags.logic((s.reg(6,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x23039au); goto L23076e;
L23039f: if(s.services.instruction)s.services.instruction(0x23039fu); s.flags.minus(s.reg(0,1,0),0x2u,1);
if(s.services.instruction)s.services.instruction(0x2303a1u); if(s.flags.sf==s.flags.of)goto L2303aa;
if(s.services.instruction)s.services.instruction(0x2303a3u); s.set(1,0x2u,4,0);
if(s.services.instruction)s.services.instruction(0x2303a8u); goto L2303b6;
L2303aa: if(s.services.instruction)s.services.instruction(0x2303aau); s.flags.minus(s.reg(0,1,0),0x3u,1);
if(s.services.instruction)s.services.instruction(0x2303acu); s.set(1,0x3u,4,0);
if(s.services.instruction)s.services.instruction(0x2303b1u); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L2303b6;
if(s.services.instruction)s.services.instruction(0x2303b3u); s.set(1,sign_extend(s.reg(0,1,0),1),4,0);
L2303b6: if(s.services.instruction)s.services.instruction(0x2303b6u); s.set(7,sign_extend(s.reg(0,1,0),1),4,0);
if(s.services.instruction)s.services.instruction(0x2303b9u); s.flags.minus(s.reg(1,4,0),s.reg(7,4,0),4);
if(s.services.instruction)s.services.instruction(0x2303bbu); if(!s.flags.zf)goto L23076e;
if(s.services.instruction)s.services.instruction(0x2303c1u); s.flags.minus(r(s.memory,(0x91u+s.gp[3]),1),0x2u,1);
if(s.services.instruction)s.services.instruction(0x2303c8u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L23076e;
if(s.services.instruction)s.services.instruction(0x2303ceu); s.set(1,r(s.memory,(0x1du+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x2303d1u); s.flags.minus(s.reg(0,1,0),s.reg(1,1,0),1);
if(s.services.instruction)s.services.instruction(0x2303d3u); w(s.memory,s.observe,0x2303d3u,(0xeu+s.gp[4]),1,s.reg(1,1,0));
if(s.services.instruction)s.services.instruction(0x2303d7u); if(s.flags.zf)goto L23076e;
if(s.services.instruction)s.services.instruction(0x2303ddu); s.set(1,sign_extend(r(s.memory,(0x1eu+s.gp[3]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x2303e1u); s.xmm[0]=_mm_cvtsi32_ss(s.xmm[0],si(s.reg(1,4,0)));
if(s.services.instruction)s.services.instruction(0x2303e5u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[2]),4))));
if(s.services.instruction)s.services.instruction(0x2303eau); s.flags.scalar_compare(s.xmm[0],s.xmm[1],false);
if(s.services.instruction)s.services.instruction(0x2303edu); if(s.flags.cf||s.flags.zf)goto L23076e;
if(s.services.instruction)s.services.instruction(0x2303f3u); s.flags.minus(r(s.memory,(0x1fu+s.gp[3]),1),s.reg(0,1,0),1);
if(s.services.instruction)s.services.instruction(0x2303f6u); w(s.memory,s.observe,0x2303f6u,(0xdu+s.gp[4]),1,0x0u);
if(s.services.instruction)s.services.instruction(0x2303fbu); if(s.flags.zf)goto L23040e;
if(s.services.instruction)s.services.instruction(0x2303fdu); s.set(1,s.reg(0,1,0),1,0);
if(s.services.instruction)s.services.instruction(0x2303ffu); s.flags.minus(s.reg(1,1,0),s.reg(0,1,0),1);
if(s.services.instruction)s.services.instruction(0x230401u); w(s.memory,s.observe,0x230401u,(0x20u+s.gp[3]),1,0x0u);
if(s.services.instruction)s.services.instruction(0x230405u); w(s.memory,s.observe,0x230405u,(0x1fu+s.gp[3]),1,s.reg(0,1,0));
if(s.services.instruction)s.services.instruction(0x230408u); if(!s.flags.zf)goto L2304a4;
L23040e: if(s.services.instruction)s.services.instruction(0x23040eu); s.set(0,sign_extend(r(s.memory,(0x20u+s.gp[3]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x230412u); s.xmm[0]=_mm_cvtsi32_ss(s.xmm[0],si(s.reg(0,4,0)));
if(s.services.instruction)s.services.instruction(0x230416u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[2]),4))));
if(s.services.instruction)s.services.instruction(0x23041bu); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x456918u),4))),false);
if(s.services.instruction)s.services.instruction(0x230422u); if(!s.flags.cf&&!s.flags.zf)goto L23042f;
if(s.services.instruction)s.services.instruction(0x230424u); s.set(0,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x230426u); s.push(0x230426u,0x23042bu);run_230ae0(s);
if(s.services.instruction)s.services.instruction(0x23042bu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x23042du); if(!s.flags.zf)goto L2304a4;
L23042f: if(s.services.instruction)s.services.instruction(0x23042fu); s.set(0,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x230431u); s.push(0x230431u,0x230436u);run_230ae0(s);
if(s.services.instruction)s.services.instruction(0x230436u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x230438u); if(s.flags.zf)goto L2304a0;
if(s.services.instruction)s.services.instruction(0x23043au); s.set(1,r(s.memory,(0xeu+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x23043eu); s.flags.minus(s.reg(1,1,0),0x2u,1);
if(s.services.instruction)s.services.instruction(0x230441u); if(s.flags.sf==s.flags.of)goto L23044a;
if(s.services.instruction)s.services.instruction(0x230443u); s.set(0,0x2u,4,0);
if(s.services.instruction)s.services.instruction(0x230448u); goto L230457;
L23044a: if(s.services.instruction)s.services.instruction(0x23044au); s.flags.minus(s.reg(1,1,0),0x3u,1);
if(s.services.instruction)s.services.instruction(0x23044du); s.set(0,0x3u,4,0);
if(s.services.instruction)s.services.instruction(0x230452u); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L230457;
if(s.services.instruction)s.services.instruction(0x230454u); s.set(0,sign_extend(s.reg(1,1,0),1),4,0);
L230457: if(s.services.instruction)s.services.instruction(0x230457u); s.set(1,sign_extend(s.reg(1,1,0),1),4,0);
if(s.services.instruction)s.services.instruction(0x23045au); s.flags.minus(s.reg(0,4,0),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x23045cu); if(!s.flags.zf)goto L230486;
if(s.services.instruction)s.services.instruction(0x23045eu); s.set(1,r(s.memory,(0x1fu+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x230461u); s.flags.minus(s.reg(1,1,0),0x2u,1);
if(s.services.instruction)s.services.instruction(0x230464u); if(s.flags.sf==s.flags.of)goto L23046d;
if(s.services.instruction)s.services.instruction(0x230466u); s.set(0,0x2u,4,0);
if(s.services.instruction)s.services.instruction(0x23046bu); goto L23047a;
L23046d: if(s.services.instruction)s.services.instruction(0x23046du); s.flags.minus(s.reg(1,1,0),0x3u,1);
if(s.services.instruction)s.services.instruction(0x230470u); s.set(0,0x3u,4,0);
if(s.services.instruction)s.services.instruction(0x230475u); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L23047a;
if(s.services.instruction)s.services.instruction(0x230477u); s.set(0,sign_extend(s.reg(1,1,0),1),4,0);
L23047a: if(s.services.instruction)s.services.instruction(0x23047au); s.set(2,sign_extend(s.reg(1,1,0),1),4,0);
if(s.services.instruction)s.services.instruction(0x23047du); s.flags.minus(s.reg(0,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x23047fu); w(s.memory,s.observe,0x23047fu,(0xdu+s.gp[4]),1,0x1u);
if(s.services.instruction)s.services.instruction(0x230484u); if(s.flags.zf)goto L23048b;
L230486: if(s.services.instruction)s.services.instruction(0x230486u); w(s.memory,s.observe,0x230486u,(0xdu+s.gp[4]),1,0x0u);
L23048b: if(s.services.instruction)s.services.instruction(0x23048bu); s.set(2,r(s.memory,(0xdu+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x23048fu); s.flags.logic((s.reg(2,1,0))&(s.reg(2,1,0)),1);
if(s.services.instruction)s.services.instruction(0x230491u); s.set(0,s.flags.zf,1,0);
if(s.services.instruction)s.services.instruction(0x230494u); s.push(0x230494u,0x1u);
if(s.services.instruction)s.services.instruction(0x230496u); s.set(6,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x230498u); s.push(0x230498u,0x23049du);run_230a60(s);
if(s.services.instruction)s.services.instruction(0x23049du); s.set(6,s.flags.logic((s.reg(6,4,0))|(0xffffffffu),4),4,0);
L2304a0: if(s.services.instruction)s.services.instruction(0x2304a0u); w(s.memory,s.observe,0x2304a0u,(0x1du+s.gp[3]),1,0xffu);
L2304a4: if(s.services.instruction)s.services.instruction(0x2304a4u); s.set(0,r(s.memory,(0x48b892u),1),1,0);
if(s.services.instruction)s.services.instruction(0x2304a9u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x2304abu); if(s.flags.zf)goto L23076e;
if(s.services.instruction)s.services.instruction(0x2304b1u); s.set(0,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x2304b3u); s.push(0x2304b3u,0x2304b8u);run_230ae0(s);
if(s.services.instruction)s.services.instruction(0x2304b8u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x2304bau); if(s.flags.zf)goto L2304c8;
if(s.services.instruction)s.services.instruction(0x2304bcu); s.set(0,r(s.memory,(0xdu+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x2304c0u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x2304c2u); if(s.flags.zf)goto L23076e;
L2304c8: if(s.services.instruction)s.services.instruction(0x2304c8u); s.set(0,r(s.memory,(0xdu+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x2304cbu); s.flags.minus(s.reg(0,1,0),0xffu,1);
if(s.services.instruction)s.services.instruction(0x2304cdu); if(s.flags.zf)goto L2304db;
if(s.services.instruction)s.services.instruction(0x2304cfu); s.set(1,r(s.memory,(0xeu+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x2304d2u); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x2304d4u); w(s.memory,s.observe,0x2304d4u,(0xfu+s.gp[4]),1,0x0u);
if(s.services.instruction)s.services.instruction(0x2304d9u); if(s.flags.zf)goto L2304e0;
L2304db: if(s.services.instruction)s.services.instruction(0x2304dbu); w(s.memory,s.observe,0x2304dbu,(0xfu+s.gp[4]),1,0x1u);
L2304e0: if(s.services.instruction)s.services.instruction(0x2304e0u); s.flags.minus(s.reg(0,1,0),0xffu,1);
if(s.services.instruction)s.services.instruction(0x2304e2u); if(s.flags.zf)goto L2304eb;
if(s.services.instruction)s.services.instruction(0x2304e4u); s.set(0,r(s.memory,(0xeu+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x2304e7u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x2304e9u); if(!s.flags.zf)goto L2304f9;
L2304eb: if(s.services.instruction)s.services.instruction(0x2304ebu); s.flags.minus(r(s.memory,(0x9cu+s.gp[3]),2),s.reg(6,2,0),2);
if(s.services.instruction)s.services.instruction(0x2304f2u); w(s.memory,s.observe,0x2304f2u,(0xeu+s.gp[4]),1,0x1u);
if(s.services.instruction)s.services.instruction(0x2304f7u); if(s.flags.zf)goto L2304fe;
L2304f9: if(s.services.instruction)s.services.instruction(0x2304f9u); w(s.memory,s.observe,0x2304f9u,(0xeu+s.gp[4]),1,0x0u);
L2304fe: if(s.services.instruction)s.services.instruction(0x2304feu); s.set(0,r(s.memory,(0xcu+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x230501u); s.flags.minus(s.reg(0,1,0),0x3u,1);
if(s.services.instruction)s.services.instruction(0x230503u); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x230506u); if(!s.flags.zf)goto L230525;
if(s.services.instruction)s.services.instruction(0x230508u); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x48u+s.gp[3]),4))),false);
if(s.services.instruction)s.services.instruction(0x23050cu); if(!s.flags.cf&&!s.flags.zf)goto L230540;
if(s.services.instruction)s.services.instruction(0x23050eu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x48u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x230513u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x23051bu); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x23051eu); if(!s.flags.cf&&!s.flags.zf)goto L230540;
if(s.services.instruction)s.services.instruction(0x230520u); s.xmm[0]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x230523u); goto L230540;
L230525: if(s.services.instruction)s.services.instruction(0x230525u); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4cu+s.gp[3]),4))),false);
if(s.services.instruction)s.services.instruction(0x230529u); if(!s.flags.cf&&!s.flags.zf)goto L230540;
if(s.services.instruction)s.services.instruction(0x23052bu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x4cu+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x230530u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x230538u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x23053bu); if(!s.flags.cf&&!s.flags.zf)goto L230540;
if(s.services.instruction)s.services.instruction(0x23053du); s.xmm[0]=s.xmm[1];
L230540: if(s.services.instruction)s.services.instruction(0x230540u); s.set(1,r(s.memory,(0x2eu+s.gp[3]),2),2,0);
if(s.services.instruction)s.services.instruction(0x230544u); s.set(2,r(s.memory,(0x2cu+s.gp[3]),2),2,0);
if(s.services.instruction)s.services.instruction(0x230548u); s.push(0x230548u,0x0u);
if(s.services.instruction)s.services.instruction(0x23054au); s.push(0x23054au,0x0u);
if(s.services.instruction)s.services.instruction(0x23054cu); w(s.memory,s.observe,0x23054cu,(0x28u+s.gp[4]),2,s.reg(1,2,0));
if(s.services.instruction)s.services.instruction(0x230551u); w(s.memory,s.observe,0x230551u,(0x9cu+s.gp[4]),2,s.reg(2,2,0));
if(s.services.instruction)s.services.instruction(0x230559u); s.set(1,(0x18u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23055du); s.push(0x23055du,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x23055eu); s.set(2,(0x24u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230562u); s.push(0x230562u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x230563u); s.set(1,(0x2cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230567u); s.push(0x230567u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230568u); s.set(1,r(s.memory,(0x34u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23056cu); s.set(0,sign_extend(s.reg(0,1,0),1),4,0);
if(s.services.instruction)s.services.instruction(0x23056fu); s.set(2,(0x28u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230573u); s.push(0x230573u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x230574u); s.set(2,r(s.memory,(0xacu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23057bu); s.push(0x23057bu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23057cu); s.push(0x23057cu,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x23057du); s.push(0x23057du,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x23057eu); s.set(0,(0x68u+s.gp[3]),4,0);
if(s.services.instruction)s.services.instruction(0x230581u); s.push(0x230581u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230582u); s.set(1,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x230584u); w(s.memory,s.observe,0x230584u,(0x30u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x23058au); w(s.memory,s.observe,0x23058au,(0x3cu+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x23058eu); w(s.memory,s.observe,0x23058eu,(0x40u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230592u); w(s.memory,s.observe,0x230592u,(0x44u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230596u); w(s.memory,s.observe,0x230596u,(0x38u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x23059au); s.push(0x23059au,0x23059fu);run_22eec0(s);
if(s.services.instruction)s.services.instruction(0x23059fu); s.set(0,r(s.memory,(0xfu+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x2305a3u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x2305a5u); if(s.flags.zf)goto L230625;
if(s.services.instruction)s.services.instruction(0x2305a7u); s.set(0,r(s.memory,(0xdu+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x2305abu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x2305adu); if(s.flags.zf)goto L2305d9;
if(s.services.instruction)s.services.instruction(0x2305afu); s.set(0,r(s.memory,(0x30u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2305b2u); s.flags.minus(s.reg(0,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2305b4u); if(s.flags.zf)goto L2305c3;
if(s.services.instruction)s.services.instruction(0x2305b6u); s.flags.minus(r(s.memory,(0xcu+s.gp[3]),1),0x2u,1);
if(s.services.instruction)s.services.instruction(0x2305bau); s.set(1,!s.flags.zf,1,0);
if(s.services.instruction)s.services.instruction(0x2305bdu); s.push(0x2305bdu,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x2305beu); s.push(0x2305beu,0x2305c3u);run_18d510(s);
L2305c3: if(s.services.instruction)s.services.instruction(0x2305c3u); s.set(0,r(s.memory,(0x34u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2305c6u); s.flags.minus(s.reg(0,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2305c8u); if(s.flags.zf)goto L230625;
if(s.services.instruction)s.services.instruction(0x2305cau); s.flags.minus(r(s.memory,(0xcu+s.gp[3]),1),0x2u,1);
if(s.services.instruction)s.services.instruction(0x2305ceu); s.set(2,!s.flags.zf,1,0);
if(s.services.instruction)s.services.instruction(0x2305d1u); s.push(0x2305d1u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x2305d2u); s.push(0x2305d2u,0x2305d7u);run_18d510(s);
if(s.services.instruction)s.services.instruction(0x2305d7u); goto L230625;
L2305d9: if(s.services.instruction)s.services.instruction(0x2305d9u); s.set(0,r(s.memory,(0x14u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2305ddu); s.flags.minus(s.reg(0,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2305dfu); if(s.flags.zf)goto L2305ff;
if(s.services.instruction)s.services.instruction(0x2305e1u); s.set(1,r(s.memory,(0x8u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2305e4u); s.push(0x2305e4u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x2305e5u); s.set(6,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x2305e7u); s.push(0x2305e7u,0x2305ecu);run_18d4b0(s);
if(s.services.instruction)s.services.instruction(0x2305ecu); s.flags.minus(r(s.memory,(0xcu+s.gp[3]),1),0x2u,1);
if(s.services.instruction)s.services.instruction(0x2305f0u); s.set(2,!s.flags.zf,1,0);
if(s.services.instruction)s.services.instruction(0x2305f3u); w(s.memory,s.observe,0x2305f3u,(0x30u+s.gp[3]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2305f6u); s.push(0x2305f6u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x2305f7u); s.push(0x2305f7u,0x2305fcu);run_18d510(s);
if(s.services.instruction)s.services.instruction(0x2305fcu); s.set(6,s.flags.logic((s.reg(6,4,0))|(0xffffffffu),4),4,0);
L2305ff: if(s.services.instruction)s.services.instruction(0x2305ffu); s.set(0,r(s.memory,(0x18u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230603u); s.flags.minus(s.reg(0,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x230605u); if(s.flags.zf)goto L230625;
if(s.services.instruction)s.services.instruction(0x230607u); s.set(1,r(s.memory,(0x8u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23060au); s.push(0x23060au,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x23060bu); s.set(6,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x23060du); s.push(0x23060du,0x230612u);run_18d4b0(s);
if(s.services.instruction)s.services.instruction(0x230612u); s.flags.minus(r(s.memory,(0xcu+s.gp[3]),1),0x2u,1);
if(s.services.instruction)s.services.instruction(0x230616u); s.set(2,!s.flags.zf,1,0);
if(s.services.instruction)s.services.instruction(0x230619u); w(s.memory,s.observe,0x230619u,(0x34u+s.gp[3]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23061cu); s.push(0x23061cu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x23061du); s.push(0x23061du,0x230622u);run_18d510(s);
if(s.services.instruction)s.services.instruction(0x230622u); s.set(6,s.flags.logic((s.reg(6,4,0))|(0xffffffffu),4),4,0);
L230625: if(s.services.instruction)s.services.instruction(0x230625u); s.set(0,r(s.memory,(0xeu+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x230629u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x23062bu); if(s.flags.zf)goto L230759;
if(s.services.instruction)s.services.instruction(0x230631u); s.set(2,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230635u); s.flags.minus(s.reg(2,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x230637u); if(s.flags.zf)goto L2306c5;
if(s.services.instruction)s.services.instruction(0x23063du); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x230643u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x230645u); s.set(1,0x18u,4,0);
if(s.services.instruction)s.services.instruction(0x23064au); s.set(7,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23064eu); while(s.gp[1]){const auto x=s.gp[0];w(s.memory,s.observe,0x23064eu,s.gp[7],4,x);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x23064eu);}
if(s.services.instruction)s.services.instruction(0x230650u); s.set(1,0xff808080u,4,0);
if(s.services.instruction)s.services.instruction(0x230655u); w(s.memory,s.observe,0x230655u,(0x84u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x23065cu); w(s.memory,s.observe,0x23065cu,(0x88u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230663u); w(s.memory,s.observe,0x230663u,(0x64u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230667u); w(s.memory,s.observe,0x230667u,(0x68u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23066bu); w(s.memory,s.observe,0x23066bu,(0x6cu+s.gp[4]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x230670u); w(s.memory,s.observe,0x230670u,(0x60u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230674u); w(s.memory,s.observe,0x230674u,(0x8cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23067bu); w(s.memory,s.observe,0x23067bu,(0x30u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23067fu); s.set(1,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230683u); s.set(0,(0xbcu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23068au); s.push(0x23068au,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x23068bu); w(s.memory,s.observe,0x23068bu,(0x4cu+s.gp[4]),2,s.reg(6,2,0));
if(s.services.instruction)s.services.instruction(0x230690u); w(s.memory,s.observe,0x230690u,(0x3cu+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230694u); w(s.memory,s.observe,0x230694u,(0x44u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230698u); w(s.memory,s.observe,0x230698u,(0x40u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x23069cu); w(s.memory,s.observe,0x23069cu,(0x48u+s.gp[4]),2,s.reg(6,2,0));
if(s.services.instruction)s.services.instruction(0x2306a1u); w(s.memory,s.observe,0x2306a1u,(0x38u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x2306a5u); w(s.memory,s.observe,0x2306a5u,(0x54u+s.gp[4]),4,0x1u);
if(s.services.instruction)s.services.instruction(0x2306adu); w(s.memory,s.observe,0x2306adu,(0x50u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2306b1u); w(s.memory,s.observe,0x2306b1u,(0x74u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x2306b7u); w(s.memory,s.observe,0x2306b7u,(0x78u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x2306bdu); s.push(0x2306bdu,0x2306c2u);run_178e30(s);
if(s.services.instruction)s.services.instruction(0x2306c2u); w(s.memory,s.observe,0x2306c2u,(0x38u+s.gp[3]),4,s.reg(0,4,0));
L2306c5: if(s.services.instruction)s.services.instruction(0x2306c5u); s.set(2,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2306c9u); s.flags.minus(s.reg(2,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2306cbu); if(s.flags.zf)goto L230759;
if(s.services.instruction)s.services.instruction(0x2306d1u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x2306d7u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x2306d9u); s.set(1,0x18u,4,0);
if(s.services.instruction)s.services.instruction(0x2306deu); s.set(7,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2306e2u); while(s.gp[1]){const auto x=s.gp[0];w(s.memory,s.observe,0x2306e2u,s.gp[7],4,x);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x2306e2u);}
if(s.services.instruction)s.services.instruction(0x2306e4u); w(s.memory,s.observe,0x2306e4u,(0x64u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2306e8u); w(s.memory,s.observe,0x2306e8u,(0x68u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2306ecu); w(s.memory,s.observe,0x2306ecu,(0x6cu+s.gp[4]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x2306f1u); w(s.memory,s.observe,0x2306f1u,(0x60u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2306f5u); w(s.memory,s.observe,0x2306f5u,(0x8cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2306fcu); w(s.memory,s.observe,0x2306fcu,(0x30u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230700u); s.set(1,0xff808080u,4,0);
if(s.services.instruction)s.services.instruction(0x230705u); w(s.memory,s.observe,0x230705u,(0x34u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x230709u); s.set(0,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23070du); s.set(2,(0xbcu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230714u); s.push(0x230714u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230715u); w(s.memory,s.observe,0x230715u,(0x4cu+s.gp[4]),2,s.reg(6,2,0));
if(s.services.instruction)s.services.instruction(0x23071au); w(s.memory,s.observe,0x23071au,(0x3cu+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x23071eu); w(s.memory,s.observe,0x23071eu,(0x44u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230722u); w(s.memory,s.observe,0x230722u,(0x40u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x230726u); w(s.memory,s.observe,0x230726u,(0x48u+s.gp[4]),2,s.reg(6,2,0));
if(s.services.instruction)s.services.instruction(0x23072bu); w(s.memory,s.observe,0x23072bu,(0x88u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230732u); w(s.memory,s.observe,0x230732u,(0x8cu+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230739u); w(s.memory,s.observe,0x230739u,(0x54u+s.gp[4]),4,0x1u);
if(s.services.instruction)s.services.instruction(0x230741u); w(s.memory,s.observe,0x230741u,(0x50u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x230745u); w(s.memory,s.observe,0x230745u,(0x74u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x23074bu); w(s.memory,s.observe,0x23074bu,(0x78u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x230751u); s.push(0x230751u,0x230756u);run_178e30(s);
if(s.services.instruction)s.services.instruction(0x230756u); w(s.memory,s.observe,0x230756u,(0x3cu+s.gp[3]),4,s.reg(0,4,0));
L230759: if(s.services.instruction)s.services.instruction(0x230759u); s.set(0,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x23075bu); s.push(0x23075bu,0x230760u);run_230ae0(s);
if(s.services.instruction)s.services.instruction(0x230760u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x230762u); if(s.flags.zf)goto L23076e;
if(s.services.instruction)s.services.instruction(0x230764u); s.set(1,r(s.memory,(0xcu+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x230767u); w(s.memory,s.observe,0x230767u,(0x1eu+s.gp[3]),1,0x0u);
if(s.services.instruction)s.services.instruction(0x23076bu); w(s.memory,s.observe,0x23076bu,(0x1du+s.gp[3]),1,s.reg(1,1,0));
L23076e: if(s.services.instruction)s.services.instruction(0x23076eu); s.set(0,sign_extend(r(s.memory,(0x20u+s.gp[3]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x230772u); s.set(0,s.flags.increment(s.reg(0,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x230773u); s.flags.minus(s.reg(0,4,0),0x7eu,4);
if(s.services.instruction)s.services.instruction(0x230776u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L23077d;
if(s.services.instruction)s.services.instruction(0x230778u); s.set(0,0x7eu,4,0);
L23077d: if(s.services.instruction)s.services.instruction(0x23077du); w(s.memory,s.observe,0x23077du,(0x20u+s.gp[3]),1,s.reg(0,1,0));
if(s.services.instruction)s.services.instruction(0x230780u); s.set(0,sign_extend(r(s.memory,(0x1cu+s.gp[3]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x230784u); s.set(0,s.flags.increment(s.reg(0,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x230785u); s.flags.minus(s.reg(0,4,0),0x7eu,4);
if(s.services.instruction)s.services.instruction(0x230788u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L23078f;
if(s.services.instruction)s.services.instruction(0x23078au); s.set(0,0x7eu,4,0);
L23078f: if(s.services.instruction)s.services.instruction(0x23078fu); w(s.memory,s.observe,0x23078fu,(0x1cu+s.gp[3]),1,s.reg(0,1,0));
if(s.services.instruction)s.services.instruction(0x230792u); s.set(0,sign_extend(r(s.memory,(0x1eu+s.gp[3]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x230796u); s.set(0,s.flags.increment(s.reg(0,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x230797u); s.flags.minus(s.reg(0,4,0),0x7eu,4);
if(s.services.instruction)s.services.instruction(0x23079au); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L2307a1;
if(s.services.instruction)s.services.instruction(0x23079cu); s.set(0,0x7eu,4,0);
L2307a1: if(s.services.instruction)s.services.instruction(0x2307a1u); w(s.memory,s.observe,0x2307a1u,(0x1eu+s.gp[3]),1,s.reg(0,1,0));
if(s.services.instruction)s.services.instruction(0x2307a4u); s.flags.minus(r(s.memory,(0x38u+s.gp[3]),4),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2307a7u); if(!s.flags.zf)goto L2307c4;
if(s.services.instruction)s.services.instruction(0x2307a9u); s.flags.minus(r(s.memory,(0x3cu+s.gp[3]),4),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2307acu); if(!s.flags.zf)goto L2307c4;
if(s.services.instruction)s.services.instruction(0x2307aeu); s.flags.minus(r(s.memory,(0xb0u+s.gp[4]),4),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2307b5u); if(!s.flags.zf)goto L2307c4;
if(s.services.instruction)s.services.instruction(0x2307b7u); s.flags.minus(r(s.memory,(0x9cu+s.gp[4]),4),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2307beu); if(s.flags.zf)goto L230a46;
L2307c4: if(s.services.instruction)s.services.instruction(0x2307c4u); s.flags.minus(r(s.memory,(0x1du+s.gp[3]),1),0x3u,1);
if(s.services.instruction)s.services.instruction(0x2307c8u); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x2307cbu); if(!s.flags.zf)goto L2307ea;
if(s.services.instruction)s.services.instruction(0x2307cdu); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x48u+s.gp[3]),4))),false);
if(s.services.instruction)s.services.instruction(0x2307d1u); if(!s.flags.cf&&!s.flags.zf)goto L230805;
if(s.services.instruction)s.services.instruction(0x2307d3u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x48u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x2307d8u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x2307e0u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x2307e3u); if(!s.flags.cf&&!s.flags.zf)goto L230805;
if(s.services.instruction)s.services.instruction(0x2307e5u); s.xmm[0]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x2307e8u); goto L230805;
L2307ea: if(s.services.instruction)s.services.instruction(0x2307eau); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4cu+s.gp[3]),4))),false);
if(s.services.instruction)s.services.instruction(0x2307eeu); if(!s.flags.cf&&!s.flags.zf)goto L230805;
if(s.services.instruction)s.services.instruction(0x2307f0u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x4cu+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x2307f5u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x2307fdu); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x230800u); if(!s.flags.cf&&!s.flags.zf)goto L230805;
if(s.services.instruction)s.services.instruction(0x230802u); s.xmm[0]=s.xmm[1];
L230805: if(s.services.instruction)s.services.instruction(0x230805u); s.set(0,r(s.memory,(0x24u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230808u); s.set(2,r(s.memory,(0x52be5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x23080eu); s.set(1,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230811u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x230816u); s.set(0,(0x0u+s.gp[0]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x230819u); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x23081cu); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(1,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x23081eu); s.push(0x23081eu,0x0u);
if(s.services.instruction)s.services.instruction(0x230820u); w(s.memory,s.observe,0x230820u,(0x24u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230824u); s.set(0,(0x9cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23082bu); s.push(0x23082bu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23082cu); s.set(1,(0x9cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230833u); s.push(0x230833u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230834u); s.set(1,r(s.memory,(0xcu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230837u); s.set(2,(0x4cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23083bu); s.push(0x23083bu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x23083cu); s.set(2,r(s.memory,(0x8u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23083fu); s.set(0,(0x5cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230843u); s.push(0x230843u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230844u); s.push(0x230844u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x230845u); s.push(0x230845u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x230846u); s.push(0x230846u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x230847u); s.set(0,(0x54u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23084bu); s.set(7,(0x78u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23084fu); w(s.memory,s.observe,0x23084fu,(0x28u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x230855u); s.push(0x230855u,0x23085au);run_22e4f0(s);
if(s.services.instruction)s.services.instruction(0x23085au); s.set(0,r(s.memory,(0xcu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23085du); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0dcu),4)));
if(s.services.instruction)s.services.instruction(0x230865u); s.set(1,r(s.memory,(0x20u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230869u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x40u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x23086fu); s.set(2,r(s.memory,(0x70u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230872u); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x230876u); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x230879u); w(s.memory,s.observe,0x230879u,(0x40u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x23087fu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x44u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x230885u); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x230888u); s.set(0,r(s.memory,(0x40u+s.gp[2]+s.gp[0]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x23088cu); s.set(1,r(s.memory,(0x40u+s.gp[0]),1),1,0);
if(s.services.instruction)s.services.instruction(0x23088fu); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x230891u); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x230895u); w(s.memory,s.observe,0x230895u,(0x44u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x23089bu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x48u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x2308a1u); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x2308a5u); w(s.memory,s.observe,0x2308a5u,(0x48u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x2308abu); if(!s.flags.zf)goto L2308d6;
if(s.services.instruction)s.services.instruction(0x2308adu); s.set(0,r(s.memory,(0x3cu+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2308b0u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x40u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x2308b5u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x44u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x2308bau); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x48u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x2308bfu); s.set(0,s.flags.plus(s.reg(0,4,0),0x40u,4),4,0);
if(s.services.instruction)s.services.instruction(0x2308c2u); w(s.memory,s.observe,0x2308c2u,(0x24u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[4])));
if(s.services.instruction)s.services.instruction(0x2308c8u); w(s.memory,s.observe,0x2308c8u,(0x28u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x2308ceu); w(s.memory,s.observe,0x2308ceu,(0x2cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x2308d4u); goto L230901;
L2308d6: if(s.services.instruction)s.services.instruction(0x2308d6u); s.set(0,r(s.memory,(0x474d04u),4),4,0);
if(s.services.instruction)s.services.instruction(0x2308dbu); s.set(1,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2308ddu); s.set(2,r(s.memory,(0x4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2308e0u); s.set(0,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2308e3u); w(s.memory,s.observe,0x2308e3u,(0x24u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x2308e7u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x24u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x2308edu); w(s.memory,s.observe,0x2308edu,(0x28u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x2308f1u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x28u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x2308f7u); w(s.memory,s.observe,0x2308f7u,(0x2cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2308fbu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x2cu+s.gp[4]),4)));
L230901: if(s.services.instruction)s.services.instruction(0x230901u); s.flags.minus(r(s.memory,(0x8u+s.gp[3]),2),0x0u,2);
if(s.services.instruction)s.services.instruction(0x230906u); if(s.flags.zf)goto L230961;
if(s.services.instruction)s.services.instruction(0x230908u); s.set(0,r(s.memory,(0x28u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x23090bu); s.flags.minus(s.reg(0,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x23090du); if(s.flags.zf)goto L230961;
if(s.services.instruction)s.services.instruction(0x23090fu); s.set(1,r(s.memory,(0x52be5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x230915u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x23091au); s.set(2,(0x0u+s.gp[0]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x23091du); s.set(0,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230920u); s.set(2,s.flags.shift(s.reg(2,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x230923u); s.set(2,s.flags.plus(s.reg(2,4,0),s.reg(0,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x230925u); s.set(0,r(s.memory,(0x10u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230928u); s.set(1,(0xa0u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23092fu); s.push(0x23092fu,0x230934u);s.required(0x23092fu,0x1d8c80u,false);
if(s.services.instruction)s.services.instruction(0x230934u); s.xmm[4]=_mm_sub_ss(s.xmm[4],_mm_set_ss(f(r(s.memory,(0xa0u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x23093du); s.xmm[3]=_mm_sub_ss(s.xmm[3],_mm_set_ss(f(r(s.memory,(0xa4u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x230946u); s.xmm[2]=_mm_sub_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0xa8u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x23094fu); w(s.memory,s.observe,0x23094fu,(0x24u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[4])));
if(s.services.instruction)s.services.instruction(0x230955u); w(s.memory,s.observe,0x230955u,(0x28u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x23095bu); w(s.memory,s.observe,0x23095bu,(0x2cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
L230961: if(s.services.instruction)s.services.instruction(0x230961u); s.set(1,r(s.memory,(0x38u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230964u); s.flags.minus(s.reg(1,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x230966u); if(s.flags.zf)goto L23098d;
if(s.services.instruction)s.services.instruction(0x230968u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x23096eu); s.set(2,(0x94u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230975u); s.push(0x230975u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x230976u); s.set(0,(0x34u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x23097au); s.push(0x23097au,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x23097bu); s.push(0x23097bu,0x0u);
if(s.services.instruction)s.services.instruction(0x23097du); s.set(0,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230981u); s.push(0x230981u,0x230986u);run_17dec0(s);
if(s.services.instruction)s.services.instruction(0x230986u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x230988u); if(!s.flags.zf)goto L23098d;
if(s.services.instruction)s.services.instruction(0x23098au); w(s.memory,s.observe,0x23098au,(0x38u+s.gp[3]),4,s.reg(6,4,0));
L23098d: if(s.services.instruction)s.services.instruction(0x23098du); s.set(1,r(s.memory,(0x3cu+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230990u); s.flags.minus(s.reg(1,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x230992u); if(s.flags.zf)goto L2309b9;
if(s.services.instruction)s.services.instruction(0x230994u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x23099au); s.set(2,(0x94u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2309a1u); s.push(0x2309a1u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x2309a2u); s.set(0,(0x34u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2309a6u); s.push(0x2309a6u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2309a7u); s.push(0x2309a7u,0x0u);
if(s.services.instruction)s.services.instruction(0x2309a9u); s.set(0,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2309adu); s.push(0x2309adu,0x2309b2u);run_17dec0(s);
if(s.services.instruction)s.services.instruction(0x2309b2u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x2309b4u); if(!s.flags.zf)goto L2309b9;
if(s.services.instruction)s.services.instruction(0x2309b6u); w(s.memory,s.observe,0x2309b6u,(0x3cu+s.gp[3]),4,s.reg(6,4,0));
L2309b9: if(s.services.instruction)s.services.instruction(0x2309b9u); s.set(1,r(s.memory,(0xb0u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2309c0u); s.flags.minus(s.reg(1,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2309c2u); s.set(7,r(s.memory,(0x8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2309c6u); s.xmm[1]=_mm_xor_ps(s.xmm[1],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x2309c9u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x2309d1u); if(s.flags.zf)goto L230a07;
if(s.services.instruction)s.services.instruction(0x2309d3u); s.flags.scalar_compare(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x18u+s.gp[3]),4))),false);
if(s.services.instruction)s.services.instruction(0x2309d7u); if(s.flags.cf||s.flags.zf)goto L2309de;
if(s.services.instruction)s.services.instruction(0x2309d9u); s.xmm[0]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x2309dcu); goto L2309f0;
L2309de: if(s.services.instruction)s.services.instruction(0x2309deu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x18u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x2309e3u); s.flags.scalar_compare(s.xmm[0],s.xmm[2],false);
if(s.services.instruction)s.services.instruction(0x2309e6u); s.xmm[0]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x2309e9u); if(!s.flags.cf&&!s.flags.zf)goto L2309f0;
if(s.services.instruction)s.services.instruction(0x2309ebu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x18u+s.gp[3]),4)));
L2309f0: if(s.services.instruction)s.services.instruction(0x2309f0u); s.set(2,(0x94u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2309f7u); s.push(0x2309f7u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x2309f8u); s.set(0,(0x34u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2309fcu); s.push(0x2309fcu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2309fdu); s.push(0x2309fdu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x2309feu); s.set(0,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230a02u); s.push(0x230a02u,0x230a07u);run_17dec0(s);
L230a07: if(s.services.instruction)s.services.instruction(0x230a07u); s.set(1,r(s.memory,(0x9cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230a0eu); s.flags.minus(s.reg(1,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x230a10u); if(s.flags.zf)goto L230a46;
if(s.services.instruction)s.services.instruction(0x230a12u); s.flags.scalar_compare(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x18u+s.gp[3]),4))),false);
if(s.services.instruction)s.services.instruction(0x230a16u); if(s.flags.cf||s.flags.zf)goto L230a1d;
if(s.services.instruction)s.services.instruction(0x230a18u); s.xmm[0]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x230a1bu); goto L230a2f;
L230a1d: if(s.services.instruction)s.services.instruction(0x230a1du); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x18u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x230a22u); s.flags.scalar_compare(s.xmm[0],s.xmm[2],false);
if(s.services.instruction)s.services.instruction(0x230a25u); s.xmm[0]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x230a28u); if(!s.flags.cf&&!s.flags.zf)goto L230a2f;
if(s.services.instruction)s.services.instruction(0x230a2au); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x18u+s.gp[3]),4)));
L230a2f: if(s.services.instruction)s.services.instruction(0x230a2fu); s.set(2,(0x94u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230a36u); s.push(0x230a36u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x230a37u); s.set(0,(0x34u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230a3bu); s.push(0x230a3bu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x230a3cu); s.push(0x230a3cu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x230a3du); s.set(0,(0x30u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x230a41u); s.push(0x230a41u,0x230a46u);run_17dec0(s);
L230a46: if(s.services.instruction)s.services.instruction(0x230a46u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x230a47u); w(s.memory,s.observe,0x230a47u,(0x11u+s.gp[3]),1,0x0u);
if(s.services.instruction)s.services.instruction(0x230a4bu); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x230a4cu); s.set(4,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x230a4eu); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x230a4fu); s.pop();s.gp[4]+=12u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_230a60(MathState &s) {
if(s.services.instruction)s.services.instruction(0x230a60u); s.push(0x230a60u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x230a61u); s.set(3,s.flags.logic((s.reg(3,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x230a64u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x230a66u); if(s.flags.zf)goto L230aaa;
if(s.services.instruction)s.services.instruction(0x230a68u); s.set(0,r(s.memory,(0x30u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230a6bu); s.flags.minus(s.reg(0,4,0),s.reg(3,4,0),4);
if(s.services.instruction)s.services.instruction(0x230a6du); s.set(1,r(s.memory,(0x4f9b5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x230a73u); if(s.flags.zf)goto L230a8c;
if(s.services.instruction)s.services.instruction(0x230a75u); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230a78u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x230a7du); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x230a80u); w(s.memory,s.observe,0x230a80u,(0x4u+s.gp[2]+s.gp[0]*8u),1,s.flags.logic((r(s.memory,(0x4u+s.gp[2]+s.gp[0]*8u),1))|(0x2u),1));
if(s.services.instruction)s.services.instruction(0x230a85u); s.set(0,(0x4u+s.gp[2]+s.gp[0]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x230a89u); w(s.memory,s.observe,0x230a89u,(0x30u+s.gp[6]),4,s.reg(3,4,0));
L230a8c: if(s.services.instruction)s.services.instruction(0x230a8cu); s.set(0,r(s.memory,(0x34u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230a8fu); s.flags.minus(s.reg(0,4,0),s.reg(3,4,0),4);
if(s.services.instruction)s.services.instruction(0x230a91u); if(s.flags.zf)goto L230aaa;
if(s.services.instruction)s.services.instruction(0x230a93u); s.set(1,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230a96u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x230a9bu); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x230a9eu); w(s.memory,s.observe,0x230a9eu,(0x4u+s.gp[1]+s.gp[0]*8u),1,s.flags.logic((r(s.memory,(0x4u+s.gp[1]+s.gp[0]*8u),1))|(0x2u),1));
if(s.services.instruction)s.services.instruction(0x230aa3u); s.set(0,(0x4u+s.gp[1]+s.gp[0]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x230aa7u); w(s.memory,s.observe,0x230aa7u,(0x34u+s.gp[6]),4,s.reg(3,4,0));
L230aaa: if(s.services.instruction)s.services.instruction(0x230aaau); s.set(0,r(s.memory,(0x8u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x230aaeu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x230ab0u); if(s.flags.zf)goto L230ad6;
if(s.services.instruction)s.services.instruction(0x230ab2u); s.push(0x230ab2u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x230ab3u); s.set(7,r(s.memory,(0x38u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230ab6u); s.flags.minus(s.reg(7,4,0),s.reg(3,4,0),4);
if(s.services.instruction)s.services.instruction(0x230ab8u); if(s.flags.zf)goto L230ac4;
if(s.services.instruction)s.services.instruction(0x230abau); s.push(0x230abau,0x1u);
if(s.services.instruction)s.services.instruction(0x230abcu); s.push(0x230abcu,0x230ac1u);run_17a0f0(s);
if(s.services.instruction)s.services.instruction(0x230ac1u); w(s.memory,s.observe,0x230ac1u,(0x38u+s.gp[6]),4,s.reg(3,4,0));
L230ac4: if(s.services.instruction)s.services.instruction(0x230ac4u); s.set(7,r(s.memory,(0x3cu+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230ac7u); s.flags.minus(s.reg(7,4,0),s.reg(3,4,0),4);
if(s.services.instruction)s.services.instruction(0x230ac9u); if(s.flags.zf)goto L230ad5;
if(s.services.instruction)s.services.instruction(0x230acbu); s.push(0x230acbu,0x1u);
if(s.services.instruction)s.services.instruction(0x230acdu); s.push(0x230acdu,0x230ad2u);run_17a0f0(s);
if(s.services.instruction)s.services.instruction(0x230ad2u); w(s.memory,s.observe,0x230ad2u,(0x3cu+s.gp[6]),4,s.reg(3,4,0));
L230ad5: if(s.services.instruction)s.services.instruction(0x230ad5u); s.set(7,s.pop(),4,0);
L230ad6: if(s.services.instruction)s.services.instruction(0x230ad6u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x230ad7u); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_230ae0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x230ae0u); s.set(2,r(s.memory,(0x30u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x230ae3u); s.set(1,s.flags.logic((s.reg(1,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x230ae6u); s.flags.minus(s.reg(2,4,0),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x230ae8u); if(!s.flags.zf)goto L230afc;
if(s.services.instruction)s.services.instruction(0x230aeau); s.flags.minus(r(s.memory,(0x34u+s.gp[0]),4),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x230aedu); if(!s.flags.zf)goto L230afc;
if(s.services.instruction)s.services.instruction(0x230aefu); s.flags.minus(r(s.memory,(0x38u+s.gp[0]),4),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x230af2u); if(!s.flags.zf)goto L230afc;
if(s.services.instruction)s.services.instruction(0x230af4u); s.flags.minus(r(s.memory,(0x3cu+s.gp[0]),4),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x230af7u); if(!s.flags.zf)goto L230afc;
if(s.services.instruction)s.services.instruction(0x230af9u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x230afbu); s.pop();s.gp[4]+=0u;return;
L230afc: if(s.services.instruction)s.services.instruction(0x230afcu); s.set(0,0x1u,4,0);
if(s.services.instruction)s.services.instruction(0x230b01u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}
} // namespace
void source_run_physics_vehicle_constraint_cleanup_runtime(M &m,std::uint32_t entry,
 SourcePhysicsContactRuntimeAbi &abi,FP &fp,const SourcePhysicsContactRuntimeServices &services,const O &observe) {
 auto bound=services;
 bound.required_owner=[&](M &memory,const SourcePhysicsContactRuntimeCall &call,
  SourcePhysicsContactRuntimeAbi &state,const O &observer) {
if(call.target==0xde8c0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xde8c0u,state,fp,services,observer);return;}
if(call.target==0x16e6a0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x16e6a0u,state,fp,services,observer);return;}
if(call.target==0x178e30u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x178e30u,state,fp,services,observer);return;}
if(call.target==0x17a0f0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x17a0f0u,state,fp,services,observer);return;}
if(call.target==0x17dec0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x17dec0u,state,fp,services,observer);return;}
if(call.target==0x18bbc0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x18bbc0u,state,fp,services,observer);return;}
if(call.target==0x18c440u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x18c440u,state,fp,services,observer);return;}
if(call.target==0x18cdd0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x18cdd0u,state,fp,services,observer);return;}
if(call.target==0x18d4b0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x18d4b0u,state,fp,services,observer);return;}
if(call.target==0x18d510u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x18d510u,state,fp,services,observer);return;}
if(call.target==0x1903a0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1903a0u,state,fp,services,observer);return;}
if(call.target==0x1d8b90u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1d8b90u,state,fp,services,observer);return;}
if(call.target==0x22e210u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22e210u,state,fp,services,observer);return;}
if(call.target==0x22e390u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22e390u,state,fp,services,observer);return;}
if(call.target==0x22e4f0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22e4f0u,state,fp,services,observer);return;}
if(call.target==0x22eec0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22eec0u,state,fp,services,observer);return;}
if(call.target==0x22efd0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22efd0u,state,fp,services,observer);return;}
if(call.target==0x22fe30u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22fe30u,state,fp,services,observer);return;}
if(call.target==0x22fea0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22fea0u,state,fp,services,observer);return;}
if(call.target==0x230a60u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x230a60u,state,fp,services,observer);return;}
if(call.target==0x230ae0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x230ae0u,state,fp,services,observer);return;}
if(call.target==0x2b3e0u){source_run_physics_vehicle_contact_state_runtime(memory,0x2b3e0u,state,fp,bound,observer);return;}
if(call.target==0x30bd0u){source_run_physics_vehicle_contact_state_runtime(memory,0x30bd0u,state,fp,bound,observer);return;}
if(call.target==0xb17d0u){source_run_physics_vehicle_contact_state_runtime(memory,0xa8380u,state,fp,bound,observer);return;}
if(call.target==0xb9390u){source_run_physics_vehicle_contact_state_runtime(memory,0xb8250u,state,fp,bound,observer);return;}
if(call.target==0xb9770u){source_run_physics_vehicle_contact_state_runtime(memory,0xb8630u,state,fp,bound,observer);return;}
if(call.target==0xb9800u){source_run_physics_vehicle_contact_state_runtime(memory,0xb86c0u,state,fp,bound,observer);return;}
if(call.target==0xbacd0u){source_run_physics_vehicle_contact_state_runtime(memory,0xb9b90u,state,fp,bound,observer);return;}
if(call.target==0xbad60u){source_run_physics_vehicle_contact_state_runtime(memory,0xb9c20u,state,fp,bound,observer);return;}
if(call.target==0xbbbc0u){source_run_physics_vehicle_contact_state_runtime(memory,0xbaa80u,state,fp,bound,observer);return;}
if(call.target==0xbc190u){source_run_physics_vehicle_contact_state_runtime(memory,0xbb050u,state,fp,bound,observer);return;}
if(call.target==0xbc200u){source_run_physics_vehicle_contact_state_runtime(memory,0xbb0c0u,state,fp,bound,observer);return;}
if(call.target==0xbcfb0u){source_run_physics_vehicle_contact_state_runtime(memory,0xbbe70u,state,fp,bound,observer);return;}
if(call.target==0xbda50u){source_run_physics_vehicle_contact_state_runtime(memory,0xbc910u,state,fp,bound,observer);return;}
if(call.target==0xbe7e0u){source_run_physics_vehicle_contact_state_runtime(memory,0xbd6a0u,state,fp,bound,observer);return;}
if(call.target==0xd3750u){source_run_physics_vehicle_contact_state_runtime(memory,0xd26c0u,state,fp,bound,observer);return;}
if(call.target==0xe8d50u){source_run_physics_vehicle_contact_state_runtime(memory,0xe7e30u,state,fp,bound,observer);return;}
if(call.target==0xe8d80u){source_run_physics_vehicle_contact_state_runtime(memory,0xe7e60u,state,fp,bound,observer);return;}
if(call.target==0xf11f0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf02d0u,state,fp,bound,observer);return;}
if(call.target==0xf15d0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf06b0u,state,fp,bound,observer);return;}
if(call.target==0xf1a50u){source_run_physics_vehicle_contact_state_runtime(memory,0xf0b30u,state,fp,bound,observer);return;}
if(call.target==0xf1dd0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf0eb0u,state,fp,bound,observer);return;}
if(call.target==0xf20c0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf11a0u,state,fp,bound,observer);return;}
if(call.target==0xf2190u){source_run_physics_vehicle_contact_state_runtime(memory,0xf1270u,state,fp,bound,observer);return;}
if(call.target==0xf2340u){source_run_physics_vehicle_contact_state_runtime(memory,0xf1420u,state,fp,bound,observer);return;}
if(call.target==0xf2c20u){source_run_physics_vehicle_contact_state_runtime(memory,0xf1d00u,state,fp,bound,observer);return;}
if(call.target==0xf3880u){source_run_physics_vehicle_contact_state_runtime(memory,0xf2960u,state,fp,bound,observer);return;}
if(call.target==0xf38e0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf29c0u,state,fp,bound,observer);return;}
if(call.target==0xf3a50u){source_run_physics_vehicle_contact_state_runtime(memory,0xf2b30u,state,fp,bound,observer);return;}
if(call.target==0xf3aa0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf2b80u,state,fp,bound,observer);return;}
if(call.target==0xf3b30u){source_run_physics_vehicle_contact_state_runtime(memory,0xf2c10u,state,fp,bound,observer);return;}
if(call.target==0xf3ca0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf2d80u,state,fp,bound,observer);return;}
if(call.target==0xf3eb0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf2f90u,state,fp,bound,observer);return;}
if(call.target==0xf41e0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf32c0u,state,fp,bound,observer);return;}
if(call.target==0xf4230u){source_run_physics_vehicle_contact_state_runtime(memory,0xf3310u,state,fp,bound,observer);return;}
if(call.target==0xf44e0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf35c0u,state,fp,bound,observer);return;}
if(call.target==0xf48c0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf39a0u,state,fp,bound,observer);return;}
if(call.target==0xf5570u){source_run_physics_vehicle_contact_state_runtime(memory,0xf4650u,state,fp,bound,observer);return;}
if(call.target==0xf67b0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf5890u,state,fp,bound,observer);return;}
if(call.target==0xf6850u){source_run_physics_vehicle_contact_state_runtime(memory,0xf5930u,state,fp,bound,observer);return;}
if(call.target==0xf6a00u){source_run_physics_vehicle_contact_state_runtime(memory,0xf5ae0u,state,fp,bound,observer);return;}
if(call.target==0xf6d60u){source_run_physics_vehicle_contact_state_runtime(memory,0xf5e40u,state,fp,bound,observer);return;}
if(call.target==0xf8150u){source_run_physics_vehicle_contact_state_runtime(memory,0xf7230u,state,fp,bound,observer);return;}
if(call.target==0xf82d0u){source_run_physics_vehicle_contact_state_runtime(memory,0xf73b0u,state,fp,bound,observer);return;}
if(call.target==0xf8360u){source_run_physics_vehicle_contact_state_runtime(memory,0xf7440u,state,fp,bound,observer);return;}
if(call.target==0xf8440u){source_run_physics_vehicle_contact_state_runtime(memory,0xf7520u,state,fp,bound,observer);return;}
if(call.target==0xf8570u){source_run_physics_vehicle_contact_state_runtime(memory,0xf7650u,state,fp,bound,observer);return;}
if(call.target==0xf9130u){source_run_physics_vehicle_contact_state_runtime(memory,0xf8210u,state,fp,bound,observer);return;}
if(call.target==0x111500u){source_run_physics_vehicle_contact_state_runtime(memory,0x1105e0u,state,fp,bound,observer);return;}
if(call.target==0x116520u){source_run_physics_vehicle_contact_state_runtime(memory,0x115600u,state,fp,bound,observer);return;}
if(call.target==0x120150u){source_run_physics_vehicle_contact_state_runtime(memory,0x11eeb0u,state,fp,bound,observer);return;}
if(call.target==0x144090u){source_run_physics_vehicle_contact_state_runtime(memory,0x142d00u,state,fp,bound,observer);return;}
if(call.target==0x144910u){source_run_physics_vehicle_contact_state_runtime(memory,0x143580u,state,fp,bound,observer);return;}
if(call.target==0x144a60u){source_run_physics_vehicle_contact_state_runtime(memory,0x1436d0u,state,fp,bound,observer);return;}
if(call.target==0x144bf0u){source_run_physics_vehicle_contact_state_runtime(memory,0x143860u,state,fp,bound,observer);return;}
if(call.target==0x145560u){source_run_physics_vehicle_contact_state_runtime(memory,0x1441d0u,state,fp,bound,observer);return;}
if(call.target==0x145680u){source_run_physics_vehicle_contact_state_runtime(memory,0x1442f0u,state,fp,bound,observer);return;}
if(call.target==0x1459b0u){source_run_physics_vehicle_contact_state_runtime(memory,0x144620u,state,fp,bound,observer);return;}
if(call.target==0x185070u){source_run_physics_vehicle_contact_state_runtime(memory,0x183c50u,state,fp,bound,observer);return;}
if(call.target==0x1cd6a0u){source_run_physics_vehicle_contact_state_runtime(memory,0x1c8430u,state,fp,bound,observer);return;}
if(call.target==0x1d64c0u){source_run_physics_vehicle_contact_state_runtime(memory,0x1d1c00u,state,fp,bound,observer);return;}
if(call.target==0x1d7d70u){source_run_physics_vehicle_contact_state_runtime(memory,0x1d34b0u,state,fp,bound,observer);return;}
if(call.target==0x1d7e40u){source_run_physics_vehicle_contact_state_runtime(memory,0x1d3580u,state,fp,bound,observer);return;}
if(call.target==0x1d8c80u){source_run_physics_vehicle_contact_state_runtime(memory,0x1d43c0u,state,fp,bound,observer);return;}
if(call.target==0x1d8d80u){source_run_physics_vehicle_contact_state_runtime(memory,0x1d44c0u,state,fp,bound,observer);return;}
if(call.target==0x1d8dd0u){source_run_physics_vehicle_contact_state_runtime(memory,0x1d4510u,state,fp,bound,observer);return;}
if(call.target==0x1d8e60u){source_run_physics_vehicle_contact_state_runtime(memory,0x1d45a0u,state,fp,bound,observer);return;}
if(call.target==0x1e11b0u){source_run_physics_vehicle_contact_state_runtime(memory,0x1dc8f0u,state,fp,bound,observer);return;}
if(call.target==0x1e1200u){source_run_physics_vehicle_contact_state_runtime(memory,0x1dc940u,state,fp,bound,observer);return;}
if(call.target==0x20d680u){source_run_physics_vehicle_contact_state_runtime(memory,0x208d80u,state,fp,bound,observer);return;}
if(call.target==0x20d770u){source_run_physics_vehicle_contact_state_runtime(memory,0x208e70u,state,fp,bound,observer);return;}
if(call.target==0x20d7d0u){source_run_physics_vehicle_contact_state_runtime(memory,0x208ed0u,state,fp,bound,observer);return;}
if(call.target==0x20d9a0u){source_run_physics_vehicle_contact_state_runtime(memory,0x2090a0u,state,fp,bound,observer);return;}
if(call.target==0x20dea0u){source_run_physics_vehicle_contact_state_runtime(memory,0x2095a0u,state,fp,bound,observer);return;}
if(call.target==0x20e350u){source_run_physics_vehicle_contact_state_runtime(memory,0x209a50u,state,fp,bound,observer);return;}
if(call.target==0x20fcb0u){source_run_physics_vehicle_contact_state_runtime(memory,0x20b3b0u,state,fp,bound,observer);return;}
if(call.target==0x20fd70u){source_run_physics_vehicle_contact_state_runtime(memory,0x20b470u,state,fp,bound,observer);return;}
if(call.target==0x22e1b0u){source_run_physics_vehicle_contact_state_runtime(memory,0x22e1b0u,state,fp,bound,observer);return;}
if(call.target==0x3150e0u){source_run_physics_vehicle_contact_state_runtime(memory,0x3115f0u,state,fp,bound,observer);return;}
if(call.target==0x3248b0u){source_run_physics_vehicle_contact_state_runtime(memory,0x31f9e0u,state,fp,bound,observer);return;}
if(call.target==0x324be0u){source_run_physics_vehicle_contact_state_runtime(memory,0x321070u,state,fp,bound,observer);return;}
if(call.target==0x324c40u){source_run_physics_vehicle_contact_state_runtime(memory,0x3210d0u,state,fp,bound,observer);return;}
if(call.target==0x32a660u){source_run_physics_vehicle_contact_state_runtime(memory,0x326af0u,state,fp,bound,observer);return;}
if(call.target==0x32b26cu){source_run_physics_vehicle_contact_state_runtime(memory,0x3276fcu,state,fp,bound,observer);return;}
if(call.target==0x32b2feu){source_run_physics_vehicle_contact_state_runtime(memory,0x32778eu,state,fp,bound,observer);return;}
if(call.target==0x32b38cu){source_run_physics_vehicle_contact_state_runtime(memory,0x32781cu,state,fp,bound,observer);return;}
if(call.target==0x32b3c5u){source_run_physics_vehicle_contact_state_runtime(memory,0x327855u,state,fp,bound,observer);return;}
if(call.target==0x32b3d6u){source_run_physics_vehicle_contact_state_runtime(memory,0x327866u,state,fp,bound,observer);return;}
if(call.target==0x331ecbu){source_run_physics_vehicle_contact_state_runtime(memory,0x32e35bu,state,fp,bound,observer);return;}
if(call.target==0x331f68u){source_run_physics_vehicle_contact_state_runtime(memory,0x32e3f8u,state,fp,bound,observer);return;}
if(call.target==0x331fa1u){source_run_physics_vehicle_contact_state_runtime(memory,0x32e431u,state,fp,bound,observer);return;}
if(call.target==0x331fa3u){source_run_physics_vehicle_contact_state_runtime(memory,0x32e433u,state,fp,bound,observer);return;}
if(call.target==0x331fcbu){source_run_physics_vehicle_contact_state_runtime(memory,0x32e45bu,state,fp,bound,observer);return;}
if(call.target==0x33200au){source_run_physics_vehicle_contact_state_runtime(memory,0x32e49au,state,fp,bound,observer);return;}
if(call.target==0x37cde8u){source_run_physics_vehicle_contact_state_runtime(memory,0x379338u,state,fp,bound,observer);return;}
if(call.target==0x37cdeau){source_run_physics_vehicle_contact_state_runtime(memory,0x37933au,state,fp,bound,observer);return;}
if(call.target==0x37d46cu){source_run_physics_vehicle_contact_state_runtime(memory,0x3799bcu,state,fp,bound,observer);return;}
if(call.target==0x37d536u){source_run_physics_vehicle_contact_state_runtime(memory,0x379a86u,state,fp,bound,observer);return;}
if(call.target==0x37dc0au){source_run_physics_vehicle_contact_state_runtime(memory,0x37a15au,state,fp,bound,observer);return;}
if(call.target==0x37dc14u){source_run_physics_vehicle_contact_state_runtime(memory,0x37a164u,state,fp,bound,observer);return;}

  if(!services.required_owner)throw std::runtime_error("Required original contact runtime callee unavailable");
  services.required_owner(memory,call,state,observer);
 };
 switch(entry) {
case 0x2b3e0u:
case 0x30bd0u:
case 0xa8380u:
case 0xb8250u:
case 0xb8630u:
case 0xb86c0u:
case 0xb9b90u:
case 0xb9c20u:
case 0xbaa80u:
case 0xbb050u:
case 0xbb0c0u:
case 0xbbe70u:
case 0xbc910u:
case 0xbd6a0u:
case 0xd26c0u:
case 0xe7e30u:
case 0xe7e60u:
case 0xf02d0u:
case 0xf06b0u:
case 0xf0b30u:
case 0xf0eb0u:
case 0xf11a0u:
case 0xf1270u:
case 0xf1420u:
case 0xf1d00u:
case 0xf2960u:
case 0xf29c0u:
case 0xf2b30u:
case 0xf2b80u:
case 0xf2c10u:
case 0xf2d80u:
case 0xf2f90u:
case 0xf32c0u:
case 0xf3310u:
case 0xf35c0u:
case 0xf39a0u:
case 0xf4650u:
case 0xf5890u:
case 0xf5930u:
case 0xf5ae0u:
case 0xf5e40u:
case 0xf7230u:
case 0xf73b0u:
case 0xf7440u:
case 0xf7520u:
case 0xf7650u:
case 0xf8210u:
case 0x1105e0u:
case 0x115600u:
case 0x11eeb0u:
case 0x142d00u:
case 0x143580u:
case 0x1436d0u:
case 0x143860u:
case 0x1441d0u:
case 0x1442f0u:
case 0x144620u:
case 0x183c50u:
case 0x1c8430u:
case 0x1d1c00u:
case 0x1d34b0u:
case 0x1d3580u:
case 0x1d43c0u:
case 0x1d44c0u:
case 0x1d4510u:
case 0x1d45a0u:
case 0x1dc8f0u:
case 0x1dc940u:
case 0x208d80u:
case 0x208e70u:
case 0x208ed0u:
case 0x2090a0u:
case 0x2095a0u:
case 0x209a50u:
case 0x20b3b0u:
case 0x20b470u:
case 0x22e1b0u:
case 0x3115f0u:
case 0x31f9e0u:
case 0x321070u:
case 0x3210d0u:
case 0x326af0u:
case 0x3276fcu:
case 0x32778eu:
case 0x32781cu:
case 0x327855u:
case 0x327866u:
case 0x32e35bu:
case 0x32e3f8u:
case 0x32e431u:
case 0x32e433u:
case 0x32e45bu:
case 0x32e49au:
case 0x379338u:
case 0x37933au:
case 0x3799bcu:
case 0x379a86u:
case 0x37a15au:
case 0x37a164u:

  try {source_run_physics_vehicle_contact_state_runtime(m,entry,abi,fp,bound,observe);}
  catch(...) {if(abi.floating_point)fp=*abi.floating_point;throw;}
  if(abi.floating_point)fp=*abi.floating_point;return;
case 0xde8c0u:break;
case 0x16e6a0u:break;
case 0x178e30u:break;
case 0x17a0f0u:break;
case 0x17dec0u:break;
case 0x18bbc0u:break;
case 0x18c440u:break;
case 0x18cdd0u:break;
case 0x18d4b0u:break;
case 0x18d510u:break;
case 0x1903a0u:break;
case 0x1d8b90u:break;
case 0x22e210u:break;
case 0x22e390u:break;
case 0x22e4f0u:break;
case 0x22eec0u:break;
case 0x22efd0u:break;
case 0x22fe30u:break;
case 0x22fea0u:break;
case 0x230a60u:break;
case 0x230ae0u:break;

 default:throw std::runtime_error("Required TU1.5 vehicle entry unavailable");
 }
 FpScope scope(fp);MathState s{m,observe,bound,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
 try {switch(entry) {
case 0xde8c0u:run_de8c0(s);break;
case 0x16e6a0u:run_16e6a0(s);break;
case 0x178e30u:run_178e30(s);break;
case 0x17a0f0u:run_17a0f0(s);break;
case 0x17dec0u:run_17dec0(s);break;
case 0x18bbc0u:run_18bbc0(s);break;
case 0x18c440u:run_18c440(s);break;
case 0x18cdd0u:run_18cdd0(s);break;
case 0x18d4b0u:run_18d4b0(s);break;
case 0x18d510u:run_18d510(s);break;
case 0x1903a0u:run_1903a0(s);break;
case 0x1d8b90u:run_1d8b90(s);break;
case 0x22e210u:run_22e210(s);break;
case 0x22e390u:run_22e390(s);break;
case 0x22e4f0u:run_22e4f0(s);break;
case 0x22eec0u:run_22eec0(s);break;
case 0x22efd0u:run_22efd0(s);break;
case 0x22fe30u:run_22fe30(s);break;
case 0x22fea0u:run_22fea0(s);break;
case 0x230a60u:run_230a60(s);break;
case 0x230ae0u:run_230ae0(s);break;

 }}catch(...) {abi=s.save();fp={control_word(),_mm_getcsr()};throw;}
 abi=s.save();fp={control_word(),_mm_getcsr()};
}
} // namespace halo2
