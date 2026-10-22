// Generated from complete owned TU1.5 instructions. No runtime decoder.
#include "halo2/physics_vehicle_contact_state.hpp"
#include "physics/vehicles/physics_vehicle_motion_math_internal.hpp"
namespace halo2 {
namespace {
using namespace physics_vehicle_motion_math_detail;
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
 if(n==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x))));
 if(n==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));
 throw std::runtime_error("Unverified TU1.5 sign-extension width");
}
void run_20fcb0(MathState &);
void run_20fd70(MathState &);
void run_22e1b0(MathState &);
void run_20fcb0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x20fcb0u); s.set(0,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcb2u); s.set(2,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcb8u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcbdu); s.set(1,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x20fcc0u); s.set(0,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcc3u); s.set(1,r(s.memory,(0x8u+s.gp[0]+s.gp[1]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcc7u); s.set(0,r(s.memory,(0xb4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fccdu); s.set(2,r(s.memory,(0x52be5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcd3u); s.set(1,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcd6u); s.push(0x20fcd6u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x20fcd7u); s.set(5,r(s.memory,(0x8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcdbu); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fce0u); s.push(0x20fce0u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x20fce1u); s.set(7,(0x0u+s.gp[0]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x20fce4u); s.set(0,r(s.memory,(0x10u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x20fce8u); s.set(7,s.flags.shift(s.reg(7,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcebu); s.set(7,s.flags.plus(s.reg(7,4,0),s.reg(1,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcedu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x20fcefu); if(!s.flags.zf)goto L20fd69;
if(s.services.instruction)s.services.instruction(0x20fcf1u); s.set(1,r(s.memory,(0x52d690u),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcf7u); s.push(0x20fcf7u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x20fcf8u); s.set(3,s.flags.logic((s.reg(3,4,0))^(s.reg(3,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fcfau); s.push(0x20fcfau,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x20fcfbu); goto L20fd00;
L20fd00: if(s.services.instruction)s.services.instruction(0x20fd00u); s.set(0,r(s.memory,(0x20u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd03u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x20fd06u); if(!s.flags.zf)goto L20fd0c;
if(s.services.instruction)s.services.instruction(0x20fd08u); s.set(2,s.flags.logic((s.reg(2,4,0))^(s.reg(2,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd0au); goto L20fd1f;
L20fd0c: if(s.services.instruction)s.services.instruction(0x20fd0cu); s.set(6,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd0fu); s.set(2,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x20fd11u); s.set(2,s.flags.logic((s.reg(2,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd17u); s.set(2,s.flags.shift(s.reg(2,4,0),0x6u,4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd1au); s.set(2,sign_extend(r(s.memory,(0x2u+s.gp[2]+s.gp[6]*1u),2),2),4,0);
L20fd1f: if(s.services.instruction)s.services.instruction(0x20fd1fu); s.flags.minus(s.reg(3,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x20fd21u); if(s.flags.sf==s.flags.of)goto L20fd67;
if(s.services.instruction)s.services.instruction(0x20fd23u); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd26u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd2bu); s.set(0,s.flags.shift(s.reg(0,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd2eu); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(3,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd30u); s.set(6,r(s.memory,(0x4u+s.gp[2]+s.gp[0]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd34u); s.set(2,r(s.memory,(0x52d68cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd3au); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x20fd3cu); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd41u); s.set(0,(0x0u+s.gp[0]+s.gp[0]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x20fd44u); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd47u); s.set(0,s.flags.plus(s.reg(0,4,0),r(s.memory,(0x44u+s.gp[2]),4),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd4au); s.flags.minus(r(s.memory,(0x8u+s.gp[0]),2),0x0u,2);
if(s.services.instruction)s.services.instruction(0x20fd4fu); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L20fd64;
if(s.services.instruction)s.services.instruction(0x20fd51u); s.set(2,sign_extend(r(s.memory,(0xdu+s.gp[0]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x20fd55u); s.flags.minus(s.reg(2,4,0),s.reg(5,4,0),4);
if(s.services.instruction)s.services.instruction(0x20fd57u); if(!s.flags.zf)goto L20fd64;
if(s.services.instruction)s.services.instruction(0x20fd59u); s.push(0x20fd59u,0x20fd5eu);run_22e1b0(s);
if(s.services.instruction)s.services.instruction(0x20fd5eu); s.set(1,r(s.memory,(0x52d690u),4),4,0);
L20fd64: if(s.services.instruction)s.services.instruction(0x20fd64u); s.set(3,s.flags.increment(s.reg(3,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x20fd65u); goto L20fd00;
L20fd67: if(s.services.instruction)s.services.instruction(0x20fd67u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x20fd68u); s.set(3,s.pop(),4,0);
L20fd69: if(s.services.instruction)s.services.instruction(0x20fd69u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x20fd6au); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x20fd6bu); s.pop();s.gp[4]+=8u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_20fd70(MathState &s) {
if(s.services.instruction)s.services.instruction(0x20fd70u); s.push(0x20fd70u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x20fd71u); s.push(0x20fd71u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x20fd72u); s.push(0x20fd72u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x20fd73u); s.set(6,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x20fd75u); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x20fd77u); s.set(0,s.flags.product(s.reg(0,4,0),0x4cu),4,0);
if(s.services.instruction)s.services.instruction(0x20fd7au); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x20fd7du); s.set(1,s.flags.product(s.reg(1,4,0),0xd8u),4,0);
if(s.services.instruction)s.services.instruction(0x20fd83u); s.set(1,(0xb10u+s.gp[1]+s.gp[2]*1u),4,0);
if(s.services.instruction)s.services.instruction(0x20fd8au); s.set(2,r(s.memory,(0x4u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd8du); s.set(5,r(s.memory,(0x48u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd90u); s.push(0x20fd90u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x20fd91u); s.set(7,r(s.memory,(0x474d10u),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd97u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(5,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd99u); s.set(5,r(s.memory,(0x0u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fd9bu); s.set(2,(0xa0u+s.gp[1]),4,0);
if(s.services.instruction)s.services.instruction(0x20fda1u); s.set(3,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x20fda3u); w(s.memory,s.observe,0x20fda3u,(0x0u+s.gp[3]),4,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x20fda5u); s.set(5,r(s.memory,(0x4u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fda8u); w(s.memory,s.observe,0x20fda8u,(0x4u+s.gp[3]),4,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x20fdabu); s.set(7,r(s.memory,(0x8u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fdaeu); w(s.memory,s.observe,0x20fdaeu,(0x8u+s.gp[3]),4,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x20fdb1u); s.set(7,r(s.memory,(0x55d728u),2),2,0);
if(s.services.instruction)s.services.instruction(0x20fdb8u); w(s.memory,s.observe,0x20fdb8u,(0xb0u+s.gp[1]),2,s.reg(7,2,0));
if(s.services.instruction)s.services.instruction(0x20fdbfu); w(s.memory,s.observe,0x20fdbfu,(0x84u+s.gp[1]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x20fdc7u); s.flags.minus(r(s.memory,(0x0u+s.gp[6]),1),0x0u,1);
if(s.services.instruction)s.services.instruction(0x20fdcau); if(s.flags.zf)goto L20fe49;
if(s.services.instruction)s.services.instruction(0x20fdccu); s.set(7,r(s.memory,(0x8u+s.gp[6]),2),2,0);
if(s.services.instruction)s.services.instruction(0x20fdd0u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x46a180u),4)));
if(s.services.instruction)s.services.instruction(0x20fdd8u); w(s.memory,s.observe,0x20fdd8u,(0xb0u+s.gp[1]),2,s.reg(7,2,0));
if(s.services.instruction)s.services.instruction(0x20fddfu); s.set(7,(0x18u+s.gp[6]),4,0);
if(s.services.instruction)s.services.instruction(0x20fde2u); s.set(3,r(s.memory,(0x0u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fde4u); w(s.memory,s.observe,0x20fde4u,(0x0u+s.gp[2]),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x20fde6u); s.set(3,r(s.memory,(0x4u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fde9u); w(s.memory,s.observe,0x20fde9u,(0x4u+s.gp[2]),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x20fdecu); s.set(7,r(s.memory,(0x8u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fdefu); w(s.memory,s.observe,0x20fdefu,(0x8u+s.gp[2]),4,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x20fdf2u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0xcu+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x20fdf7u); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[6]),4))));
if(s.services.instruction)s.services.instruction(0x20fdfcu); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x20fdffu); if(s.flags.cf||s.flags.zf)goto L20fe06;
if(s.services.instruction)s.services.instruction(0x20fe01u); s.xmm[0]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x20fe04u); goto L20fe1b;
L20fe06: if(s.services.instruction)s.services.instruction(0x20fe06u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0xcu+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x20fe0bu); s.xmm[1]=_mm_sub_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[6]),4))));
if(s.services.instruction)s.services.instruction(0x20fe10u); s.flags.scalar_compare(s.xmm[1],_mm_set_ss(f(r(s.memory,(0xcu+s.gp[0]),4))),false);
if(s.services.instruction)s.services.instruction(0x20fe14u); if(s.flags.cf||s.flags.zf)goto L20fe1b;
if(s.services.instruction)s.services.instruction(0x20fe16u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0xcu+s.gp[0]),4)));
L20fe1b: if(s.services.instruction)s.services.instruction(0x20fe1bu); w(s.memory,s.observe,0x20fe1bu,(0x84u+s.gp[1]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x20fe23u); w(s.memory,s.observe,0x20fe23u,(0xacu+s.gp[1]),4,0xffffffffu);
if(s.services.instruction)s.services.instruction(0x20fe2du); s.set(0,r(s.memory,(0x2cu+s.gp[6]),1),1,0);
if(s.services.instruction)s.services.instruction(0x20fe30u); w(s.memory,s.observe,0x20fe30u,(0x70u+s.gp[1]),1,s.reg(0,1,0));
if(s.services.instruction)s.services.instruction(0x20fe33u); s.set(6,s.flags.plus(s.reg(6,4,0),0x30u,4),4,0);
if(s.services.instruction)s.services.instruction(0x20fe36u); s.set(2,r(s.memory,(0x0u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fe38u); s.set(1,s.flags.plus(s.reg(1,4,0),0x74u,4),4,0);
if(s.services.instruction)s.services.instruction(0x20fe3bu); w(s.memory,s.observe,0x20fe3bu,(0x0u+s.gp[1]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x20fe3du); s.set(0,r(s.memory,(0x4u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fe40u); w(s.memory,s.observe,0x20fe40u,(0x4u+s.gp[1]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x20fe43u); s.set(2,r(s.memory,(0x8u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x20fe46u); w(s.memory,s.observe,0x20fe46u,(0x8u+s.gp[1]),4,s.reg(2,4,0));
L20fe49: if(s.services.instruction)s.services.instruction(0x20fe49u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x20fe4au); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x20fe4bu); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x20fe4cu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x20fe4du); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_22e1b0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x22e1b0u); s.set(4,s.flags.minus(s.reg(4,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e1b3u); s.push(0x22e1b3u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22e1b4u); s.set(3,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e1b6u); s.set(1,0x1u,4,0);
if(s.services.instruction)s.services.instruction(0x22e1bbu); s.flags.minus(r(s.memory,(0x8u+s.gp[3]),2),s.reg(1,2,0),2);
if(s.services.instruction)s.services.instruction(0x22e1bfu); if(!s.flags.zf)goto L22e201;
if(s.services.instruction)s.services.instruction(0x22e1c1u); s.set(0,r(s.memory,(0x10u+s.gp[3]),1),1,0);
if(s.services.instruction)s.services.instruction(0x22e1c4u); s.flags.minus(s.reg(0,1,0),s.reg(1,1,0),1);
if(s.services.instruction)s.services.instruction(0x22e1c6u); if(s.flags.sf!=s.flags.of)goto L22e1cd;
if(s.services.instruction)s.services.instruction(0x22e1c8u); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L22e1cd;
if(s.services.instruction)s.services.instruction(0x22e1cau); s.set(1,sign_extend(s.reg(0,1,0),1),4,0);
L22e1cd: if(s.services.instruction)s.services.instruction(0x22e1cdu); s.set(0,sign_extend(s.reg(0,1,0),1),4,0);
if(s.services.instruction)s.services.instruction(0x22e1d0u); s.flags.minus(s.reg(1,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x22e1d2u); if(!s.flags.zf)goto L22e201;
if(s.services.instruction)s.services.instruction(0x22e1d4u); s.set(1,(0x4u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x22e1d8u); s.push(0x22e1d8u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22e1d9u); s.set(2,(0xcu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x22e1ddu); s.push(0x22e1ddu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x22e1deu); s.push(0x22e1deu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22e1dfu); s.push(0x22e1dfu,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x22e1e0u); s.push(0x22e1e0u,0x22e1e5u);s.required(0x22e1e0u,0x22e390u,false);
if(s.services.instruction)s.services.instruction(0x22e1e5u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x14u+s.gp[3]),4)));
if(s.services.instruction)s.services.instruction(0x22e1eau); s.set(1,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x22e1ecu); s.push(0x22e1ecu,0x22e1f1u);s.required(0x22e1ecu,0x22fe30u,false);
if(s.services.instruction)s.services.instruction(0x22e1f1u); s.set(0,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e1f5u); s.set(1,r(s.memory,(0x8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22e1f9u); s.push(0x22e1f9u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x22e1fau); s.push(0x22e1fau,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x22e1fbu); s.push(0x22e1fbu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x22e1fcu); s.push(0x22e1fcu,0x22e201u);s.required(0x22e1fcu,0x22fea0u,false);
L22e201: if(s.services.instruction)s.services.instruction(0x22e201u); w(s.memory,s.observe,0x22e201u,(0x8u+s.gp[3]),2,s.flags.increment(r(s.memory,(0x8u+s.gp[3]),2),2,true));
if(s.services.instruction)s.services.instruction(0x22e205u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x22e206u); s.set(4,s.flags.plus(s.reg(4,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22e209u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}
} // namespace
void source_run_physics_vehicle_contact_state_runtime(M &m,std::uint32_t entry,
 SourcePhysicsContactRuntimeAbi &abi,FP &fp,const SourcePhysicsContactRuntimeServices &services,const O &observe) {
 auto bound=services;
 bound.required_owner=[&](M &memory,const SourcePhysicsContactRuntimeCall &call,
  SourcePhysicsContactRuntimeAbi &state,const O &observer) {
if(call.target==0x20fcb0u){source_run_physics_vehicle_contact_state_runtime(memory,0x20b3b0u,state,fp,services,observer);return;}
if(call.target==0x20fd70u){source_run_physics_vehicle_contact_state_runtime(memory,0x20b470u,state,fp,services,observer);return;}
if(call.target==0x22e1b0u){source_run_physics_vehicle_contact_state_runtime(memory,0x22e1b0u,state,fp,services,observer);return;}

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

  try {source_run_physics_vehicle_motion_math_runtime(m,entry,abi,SourcePhysicsContactImage::TitleUpdate15,fp,bound,observe);}
  catch(...) {if(abi.floating_point)fp=*abi.floating_point;throw;}
  if(abi.floating_point)fp=*abi.floating_point;return;
case 0x20b3b0u:break;
case 0x20b470u:break;
case 0x22e1b0u:break;

 default:throw std::runtime_error("Required TU1.5 vehicle entry unavailable");
 }
 FpScope scope(fp);MathState s{m,observe,bound,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
 try {switch(entry) {
case 0x20b3b0u:run_20fcb0(s);break;
case 0x20b470u:run_20fd70(s);break;
case 0x22e1b0u:run_22e1b0(s);break;

 }}catch(...) {abi=s.save();fp={control_word(),_mm_getcsr()};throw;}
 abi=s.save();fp={control_word(),_mm_getcsr()};
}
} // namespace halo2
