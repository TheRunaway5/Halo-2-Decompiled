// Generated from complete owned TU1.5 instructions. No runtime decoder.
#include "halo2/physics_vehicle_environment_state.hpp"
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
void run_bcdf0(MathState &);
void run_c6120(MathState &);
void run_11e600(MathState &);
void run_11e850(MathState &);
void run_14cdb0(MathState &);
void run_16e8a0(MathState &);
void run_16ea60(MathState &);
void run_1755b0(MathState &);
void run_177020(MathState &);
void run_177fa0(MathState &);
void run_17a030(MathState &);
void run_17a230(MathState &);
void run_17a2f0(MathState &);
void run_17a380(MathState &);
void run_17a420(MathState &);
void run_17a4a0(MathState &);
void run_18add0(MathState &);
void run_18ae80(MathState &);
void run_18af20(MathState &);
void run_18b0c0(MathState &);
void run_18b160(MathState &);
void run_18b1d0(MathState &);
void run_190420(MathState &);
void run_228220(MathState &);
void run_2e44d0(MathState &);
void run_2e4650(MathState &);
void run_bcdf0(MathState &s) {
if(s.services.instruction)s.services.instruction(0xbcdf0u); s.push(0xbcdf0u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0xbcdf1u); s.push(0xbcdf1u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0xbcdf2u); s.set(6,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0xbcdf4u); s.set(7,s.flags.logic((s.reg(7,4,0))^(s.reg(7,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0xbcdf6u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0xbcdf9u); if(s.flags.zf)goto Lbce2d;
if(s.services.instruction)s.services.instruction(0xbcdfbu); s.set(2,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0xbce01u); s.set(0,r(s.memory,(0x38u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0xbce04u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0xbce06u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0xbce0cu); s.flags.minus(s.reg(1,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0xbce0eu); if(s.flags.sf==s.flags.of)goto Lbce2d;
if(s.services.instruction)s.services.instruction(0xbce10u); s.set(0,r(s.memory,(0x24u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0xbce13u); s.set(0,s.flags.product(s.reg(0,4,0),s.reg(1,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0xbce16u); s.set(0,s.flags.plus(s.reg(0,4,0),r(s.memory,(0x44u+s.gp[2]),4),4),4,0);
if(s.services.instruction)s.services.instruction(0xbce19u); s.set(1,r(s.memory,(0x0u+s.gp[0]),2),2,0);
if(s.services.instruction)s.services.instruction(0xbce1cu); s.flags.logic((s.reg(1,2,0))&(s.reg(1,2,0)),2);
if(s.services.instruction)s.services.instruction(0xbce1fu); if(s.flags.zf)goto Lbce2d;
if(s.services.instruction)s.services.instruction(0xbce21u); s.set(1,sign_extend(s.reg(1,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0xbce24u); s.set(6,s.flags.right(s.reg(6,4,0),0x10u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0xbce27u); s.flags.minus(s.reg(1,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0xbce29u); if(!s.flags.zf)goto Lbce2d;
if(s.services.instruction)s.services.instruction(0xbce2bu); s.set(7,s.reg(0,4,0),4,0);
Lbce2d: if(s.services.instruction)s.services.instruction(0xbce2du); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0xbce2fu); s.flags.logic((s.reg(7,4,0))&(s.reg(7,4,0)),4);
if(s.services.instruction)s.services.instruction(0xbce31u); if(s.flags.zf)goto Lbce46;
if(s.services.instruction)s.services.instruction(0xbce33u); s.set(1,r(s.memory,(0x3u+s.gp[7]),1),1,0);
if(s.services.instruction)s.services.instruction(0xbce36u); s.set(2,0x1u,4,0);
if(s.services.instruction)s.services.instruction(0xbce3bu); s.set(2,s.flags.shift(s.reg(2,4,0),s.reg(1,1,0),4),4,0);
if(s.services.instruction)s.services.instruction(0xbce3du); s.flags.logic((r(s.memory,(0xcu+s.gp[4]),4))&(s.reg(2,4,0)),4);
if(s.services.instruction)s.services.instruction(0xbce41u); if(s.flags.zf)goto Lbce46;
if(s.services.instruction)s.services.instruction(0xbce43u); s.set(0,r(s.memory,(0x8u+s.gp[7]),4),4,0);
Lbce46: if(s.services.instruction)s.services.instruction(0xbce46u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0xbce47u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0xbce48u); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_c6120(MathState &s) {
if(s.services.instruction)s.services.instruction(0xc6120u); s.set(0,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0xc6124u); s.push(0xc6124u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0xc6125u); s.set(6,r(s.memory,(0x4ecbecu),4),4,0);
if(s.services.instruction)s.services.instruction(0xc612bu); s.flags.logic((s.reg(6,4,0))&(s.reg(6,4,0)),4);
if(s.services.instruction)s.services.instruction(0xc612du); if(s.flags.zf)goto Lc6235;
if(s.services.instruction)s.services.instruction(0xc6133u); s.set(1,r(s.memory,(0x29u+s.gp[6]),1),1,0);
if(s.services.instruction)s.services.instruction(0xc6136u); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0xc6138u); if(s.flags.zf)goto Lc6235;
if(s.services.instruction)s.services.instruction(0xc613eu); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0xc6141u); if(s.flags.zf)goto Lc6235;
if(s.services.instruction)s.services.instruction(0xc6147u); s.flags.minus(r(s.memory,(0xcu+s.gp[4]),4),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0xc614cu); if(s.flags.zf)goto Lc6235;
if(s.services.instruction)s.services.instruction(0xc6152u); s.push(0xc6152u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0xc6153u); s.set(3,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0xc6159u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0xc615eu); s.set(0,s.flags.shift(s.reg(0,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0xc6161u); s.push(0xc6161u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0xc6162u); s.set(5,r(s.memory,(0x8u+s.gp[0]+s.gp[3]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0xc6166u); s.set(2,s.flags.logic((s.reg(2,4,0))^(s.reg(2,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0xc6168u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0xc616au); s.push(0xc616au,0xc616fu);run_16ea60(s);
if(s.services.instruction)s.services.instruction(0xc616fu); s.set(1,s.flags.logic((s.reg(1,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0xc6172u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0xc6175u); if(s.flags.zf)goto Lc6189;
if(s.services.instruction)s.services.instruction(0xc6177u); s.set(1,r(s.memory,(0x24u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0xc617au); s.set(2,r(s.memory,(0x44u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0xc617du); s.set(1,s.flags.product(s.reg(1,4,0),s.reg(0,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0xc6180u); s.set(1,sign_extend(r(s.memory,(0x0u+s.gp[1]+s.gp[2]*1u),2),2),4,0);
if(s.services.instruction)s.services.instruction(0xc6184u); s.set(1,s.flags.shift(s.reg(1,4,0),0x10u,4),4,0);
if(s.services.instruction)s.services.instruction(0xc6187u); s.set(1,s.flags.logic((s.reg(1,4,0))|(s.reg(0,4,0)),4),4,0);
Lc6189: if(s.services.instruction)s.services.instruction(0xc6189u); s.flags.minus(s.reg(1,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0xc618cu); s.push(0xc618cu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0xc618du); s.set(7,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0xc618fu); if(s.flags.zf)goto Lc6232;
if(s.services.instruction)s.services.instruction(0xc6195u); s.xmm[2]=_mm_xor_ps(s.xmm[2],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0xc6198u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
Lc61a0: if(s.services.instruction)s.services.instruction(0xc61a0u); s.set(1,r(s.memory,(0x44u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0xc61a3u); s.set(2,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0xc61a5u); s.set(2,s.flags.logic((s.reg(2,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0xc61abu); s.set(0,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0xc61adu); s.set(0,s.flags.product(s.reg(0,4,0),0x110u),4,0);
if(s.services.instruction)s.services.instruction(0xc61b3u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(1,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0xc61b5u); s.set(1,r(s.memory,(0x18u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0xc61b9u); s.flags.minus(r(s.memory,(0xcu+s.gp[0]),4),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0xc61bcu); if(!s.flags.zf)goto Lc61fc;
if(s.services.instruction)s.services.instruction(0xc61beu); s.set(1,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0xc61c1u); s.flags.minus(s.reg(1,4,0),r(s.memory,(0x14u+s.gp[4]),4),4);
if(s.services.instruction)s.services.instruction(0xc61c5u); if(!s.flags.zf)goto Lc61fc;
if(s.services.instruction)s.services.instruction(0xc61c7u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0xc61cdu); s.set(1,s.flags.shift(s.reg(1,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0xc61d0u); s.set(1,r(s.memory,(0x8u+s.gp[1]+s.gp[3]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0xc61d4u); s.flags.minus(r(s.memory,(0x2u+s.gp[1]),2),0x0u,2);
if(s.services.instruction)s.services.instruction(0xc61d9u); if(!s.flags.zf)goto Lc61fc;
if(s.services.instruction)s.services.instruction(0xc61dbu); s.flags.minus(r(s.memory,(0x2u+s.gp[5]),2),0x0u,2);
if(s.services.instruction)s.services.instruction(0xc61e0u); if(!s.flags.zf)goto Lc61fc;
if(s.services.instruction)s.services.instruction(0xc61e2u); s.flags.scalar_compare(s.xmm[2],s.xmm[1],false);
if(s.services.instruction)s.services.instruction(0xc61e5u); if(s.flags.cf||s.flags.zf)goto Lc61ec;
if(s.services.instruction)s.services.instruction(0xc61e7u); s.xmm[0]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0xc61eau); goto Lc61f7;
Lc61ec: if(s.services.instruction)s.services.instruction(0xc61ecu); s.flags.scalar_compare(s.xmm[1],s.xmm[3],false);
if(s.services.instruction)s.services.instruction(0xc61efu); s.xmm[0]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0xc61f2u); if(!s.flags.cf&&!s.flags.zf)goto Lc61f7;
if(s.services.instruction)s.services.instruction(0xc61f4u); s.xmm[0]=s.xmm[1];
Lc61f7: if(s.services.instruction)s.services.instruction(0xc61f7u); w(s.memory,s.observe,0xc61f7u,(0x10u+s.gp[0]),4,u(_mm_cvtss_f32(s.xmm[0])));
Lc61fc: if(s.services.instruction)s.services.instruction(0xc61fcu); s.flags.minus(s.reg(7,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0xc61ffu); if(!s.flags.zf)goto Lc6205;
if(s.services.instruction)s.services.instruction(0xc6201u); s.set(2,s.flags.logic((s.reg(2,4,0))^(s.reg(2,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0xc6203u); goto Lc6206;
Lc6205: if(s.services.instruction)s.services.instruction(0xc6205u); s.set(2,s.flags.increment(s.reg(2,4,0),4,false),4,0);
Lc6206: if(s.services.instruction)s.services.instruction(0xc6206u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0xc6208u); s.push(0xc6208u,0xc620du);run_16ea60(s);
if(s.services.instruction)s.services.instruction(0xc620du); s.set(1,s.flags.logic((s.reg(1,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0xc6210u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0xc6213u); if(s.flags.zf)goto Lc6227;
if(s.services.instruction)s.services.instruction(0xc6215u); s.set(2,r(s.memory,(0x24u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0xc6218u); s.set(1,r(s.memory,(0x44u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0xc621bu); s.set(2,s.flags.product(s.reg(2,4,0),s.reg(0,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0xc621eu); s.set(1,sign_extend(r(s.memory,(0x0u+s.gp[2]+s.gp[1]*1u),2),2),4,0);
if(s.services.instruction)s.services.instruction(0xc6222u); s.set(1,s.flags.shift(s.reg(1,4,0),0x10u,4),4,0);
if(s.services.instruction)s.services.instruction(0xc6225u); s.set(1,s.flags.logic((s.reg(1,4,0))|(s.reg(0,4,0)),4),4,0);
Lc6227: if(s.services.instruction)s.services.instruction(0xc6227u); s.flags.minus(s.reg(1,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0xc622au); s.set(7,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0xc622cu); if(!s.flags.zf)goto Lc61a0;
Lc6232: if(s.services.instruction)s.services.instruction(0xc6232u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0xc6233u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0xc6234u); s.set(3,s.pop(),4,0);
Lc6235: if(s.services.instruction)s.services.instruction(0xc6235u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0xc6236u); s.pop();s.gp[4]+=8u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_11e600(MathState &s) {
if(s.services.instruction)s.services.instruction(0x11e600u); s.push(0x11e600u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x11e601u); s.set(7,r(s.memory,(0x474c24u),2),2,0);
if(s.services.instruction)s.services.instruction(0x11e608u); s.flags.minus(s.reg(7,2,0),0xffffffffu,2);
if(s.services.instruction)s.services.instruction(0x11e60cu); if(!s.flags.zf)goto L11e61e;
if(s.services.instruction)s.services.instruction(0x11e60eu); w(s.memory,s.observe,0x11e60eu,(0x4u+s.gp[6]),2,s.reg(7,2,0));
if(s.services.instruction)s.services.instruction(0x11e612u); w(s.memory,s.observe,0x11e612u,(0x6u+s.gp[6]),2,s.reg(7,2,0));
if(s.services.instruction)s.services.instruction(0x11e616u); w(s.memory,s.observe,0x11e616u,(0x0u+s.gp[6]),4,0xffffffffu);
if(s.services.instruction)s.services.instruction(0x11e61cu); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x11e61du); s.pop();s.gp[4]+=0u;return;
L11e61e: if(s.services.instruction)s.services.instruction(0x11e61eu); s.set(1,r(s.memory,(0x4ecc0cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x11e624u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x11e626u); s.push(0x11e626u,0x11e62bu);run_14cdb0(s);
if(s.services.instruction)s.services.instruction(0x11e62bu); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x11e62eu); w(s.memory,s.observe,0x11e62eu,(0x0u+s.gp[6]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x11e630u); if(s.flags.zf)goto L11e649;
if(s.services.instruction)s.services.instruction(0x11e632u); s.set(1,r(s.memory,(0x4ecc18u),4),4,0);
if(s.services.instruction)s.services.instruction(0x11e638u); s.set(2,r(s.memory,(0x30u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x11e63bu); s.set(0,sign_extend(r(s.memory,(0x0u+s.gp[2]+s.gp[0]*8u),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x11e63fu); w(s.memory,s.observe,0x11e63fu,(0x6u+s.gp[6]),2,s.reg(7,2,0));
if(s.services.instruction)s.services.instruction(0x11e643u); w(s.memory,s.observe,0x11e643u,(0x4u+s.gp[6]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x11e647u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x11e648u); s.pop();s.gp[4]+=0u;return;
L11e649: if(s.services.instruction)s.services.instruction(0x11e649u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x11e64cu); w(s.memory,s.observe,0x11e64cu,(0x6u+s.gp[6]),2,s.reg(7,2,0));
if(s.services.instruction)s.services.instruction(0x11e650u); w(s.memory,s.observe,0x11e650u,(0x4u+s.gp[6]),2,s.reg(0,2,0));
if(s.services.instruction)s.services.instruction(0x11e654u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x11e655u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_11e850(MathState &s) {
if(s.services.instruction)s.services.instruction(0x11e850u); s.set(1,r(s.memory,(0x4u+s.gp[1]),2),2,0);
if(s.services.instruction)s.services.instruction(0x11e854u); s.set(0,s.flags.logic((s.reg(0,1,0))^(s.reg(0,1,0)),1),1,0);
if(s.services.instruction)s.services.instruction(0x11e856u); s.flags.minus(s.reg(1,2,0),0xffffffffu,2);
if(s.services.instruction)s.services.instruction(0x11e85au); s.push(0x11e85au,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x11e85bu); s.set(5,r(s.memory,(0x8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x11e85fu); if(s.flags.zf)goto L11e8da;
if(s.services.instruction)s.services.instruction(0x11e861u); s.set(1,sign_extend(s.reg(1,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x11e864u); s.set(1,s.flags.product(s.reg(1,4,0),0xb0u),4,0);
if(s.services.instruction)s.services.instruction(0x11e86au); s.push(0x11e86au,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x11e86bu); s.set(6,r(s.memory,(0x4ecc18u),4),4,0);
if(s.services.instruction)s.services.instruction(0x11e871u); s.set(1,s.flags.plus(s.reg(1,4,0),r(s.memory,(0xa0u+s.gp[6]),4),4),4,0);
if(s.services.instruction)s.services.instruction(0x11e877u); s.set(1,r(s.memory,(0x70u+s.gp[1]),1),1,0);
if(s.services.instruction)s.services.instruction(0x11e87au); s.flags.minus(s.reg(1,1,0),0xffu,1);
if(s.services.instruction)s.services.instruction(0x11e87du); if(s.flags.zf)goto L11e8d9;
if(s.services.instruction)s.services.instruction(0x11e87fu); s.set(6,r(s.memory,(0x68u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x11e882u); s.set(2,s.flags.logic((s.reg(2,4,0))^(s.reg(2,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x11e884u); s.set(2,s.reg(1,1,0),1,0);
if(s.services.instruction)s.services.instruction(0x11e886u); s.set(2,s.flags.logic((s.reg(2,4,0))&(0x7fu),4),4,0);
if(s.services.instruction)s.services.instruction(0x11e889u); s.set(2,(0x0u+s.gp[2]+s.gp[2]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x11e88cu); s.set(2,(0x0u+s.gp[6]+s.gp[2]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x11e88fu); s.set(6,r(s.memory,(0x2u+s.gp[2]),2),2,0);
if(s.services.instruction)s.services.instruction(0x11e893u); s.flags.minus(s.reg(6,2,0),0xffffffffu,2);
if(s.services.instruction)s.services.instruction(0x11e897u); if(s.flags.zf)goto L11e8d9;
if(s.services.instruction)s.services.instruction(0x11e899u); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x11e89bu); if(!s.flags.sf)goto L11e8cf;
if(s.services.instruction)s.services.instruction(0x11e89du); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0xcu+s.gp[2]),4)));
if(s.services.instruction)s.services.instruction(0x11e8a2u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x11e8a7u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[2]),4)));
if(s.services.instruction)s.services.instruction(0x11e8acu); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x11e8b1u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x11e8b5u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[2]),4)));
if(s.services.instruction)s.services.instruction(0x11e8bau); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x11e8beu); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x11e8c2u); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x10u+s.gp[2]),4))));
if(s.services.instruction)s.services.instruction(0x11e8c7u); s.xmm[1]=_mm_xor_ps(s.xmm[1],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x11e8cau); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x11e8cdu); if(s.flags.cf||s.flags.zf)goto L11e8d9;
L11e8cf: if(s.services.instruction)s.services.instruction(0x11e8cfu); s.flags.logic((s.reg(5,4,0))&(s.reg(5,4,0)),4);
if(s.services.instruction)s.services.instruction(0x11e8d1u); s.set(0,0x1u,1,0);
if(s.services.instruction)s.services.instruction(0x11e8d3u); if(s.flags.zf)goto L11e8d9;
if(s.services.instruction)s.services.instruction(0x11e8d5u); w(s.memory,s.observe,0x11e8d5u,(0x0u+s.gp[5]),2,s.reg(6,2,0));
L11e8d9: if(s.services.instruction)s.services.instruction(0x11e8d9u); s.set(6,s.pop(),4,0);
L11e8da: if(s.services.instruction)s.services.instruction(0x11e8dau); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x11e8dbu); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_14cdb0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x14cdb0u); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x14cdb3u); s.push(0x14cdb3u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x14cdb4u); s.set(6,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x14cdb7u); s.push(0x14cdb7u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x14cdb8u); s.set(7,r(s.memory,(0xcu+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x14cdbbu); goto L14cdc0;
L14cdc0: if(s.services.instruction)s.services.instruction(0x14cdc0u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0x7fffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x14cdc5u); s.set(1,(0x0u+s.gp[6]+s.gp[0]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x14cdc8u); s.set(0,sign_extend(r(s.memory,(0x0u+s.gp[1]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x14cdcbu); s.set(0,s.flags.shift(s.reg(0,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x14cdceu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]+s.gp[7]*1u),4)));
if(s.services.instruction)s.services.instruction(0x14cdd4u); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[2]),4))));
if(s.services.instruction)s.services.instruction(0x14cdd9u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]+s.gp[7]*1u),4)));
if(s.services.instruction)s.services.instruction(0x14cddfu); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[2]),4))));
if(s.services.instruction)s.services.instruction(0x14cde4u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(7,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x14cde6u); s.xmm[1]=_mm_add_ss(s.xmm[1],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x14cdeau); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x14cdeeu); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[2]),4))));
if(s.services.instruction)s.services.instruction(0x14cdf2u); s.xmm[1]=_mm_add_ss(s.xmm[1],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x14cdf6u); s.xmm[1]=_mm_sub_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0xcu+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x14cdfbu); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x14cdfeu); if(s.flags.cf)goto L14ce04;
if(s.services.instruction)s.services.instruction(0x14ce00u); s.set(0,0x1u,1,0);
if(s.services.instruction)s.services.instruction(0x14ce02u); goto L14ce06;
L14ce04: if(s.services.instruction)s.services.instruction(0x14ce04u); s.set(0,s.flags.logic((s.reg(0,1,0))^(s.reg(0,1,0)),1),1,0);
L14ce06: if(s.services.instruction)s.services.instruction(0x14ce06u); s.set(0,s.reg(0,1,0),4,0);
if(s.services.instruction)s.services.instruction(0x14ce09u); s.set(1,(0x0u+s.gp[1]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x14ce0cu); s.set(0,r(s.memory,(0x1u+s.gp[0]+s.gp[1]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x14ce10u); s.set(0,s.flags.right(s.reg(0,4,0),0x8u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x14ce13u); s.flags.logic((s.reg(0,4,0))&(0x800000u),4);
if(s.services.instruction)s.services.instruction(0x14ce18u); if(s.flags.zf)goto L14cdc0;
if(s.services.instruction)s.services.instruction(0x14ce1au); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x14ce1du); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x14ce1eu); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x14ce1fu); if(s.flags.zf)goto L14ce27;
if(s.services.instruction)s.services.instruction(0x14ce21u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0x7fffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x14ce26u); s.pop();s.gp[4]+=0u;return;
L14ce27: if(s.services.instruction)s.services.instruction(0x14ce27u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x14ce2au); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_16e8a0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x16e8a0u); s.push(0x16e8a0u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x16e8a1u); s.push(0x16e8a1u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x16e8a2u); s.set(3,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e8a4u); s.set(0,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e8a7u); s.push(0x16e8a7u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x16e8a8u); s.set(7,r(s.memory,(0x24u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e8abu); s.set(3,s.flags.logic((s.reg(3,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e8b1u); s.set(6,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e8b3u); s.set(6,s.flags.product(s.reg(6,4,0),s.reg(3,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0x16e8b6u); s.set(6,s.flags.plus(s.reg(6,4,0),s.reg(0,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e8b8u); s.set(0,r(s.memory,(0x2au+s.gp[2]),1),1,0);
if(s.services.instruction)s.services.instruction(0x16e8bbu); s.set(0,s.flags.right(s.reg(0,1,0),0x3u,1,false),1,0);
if(s.services.instruction)s.services.instruction(0x16e8beu); s.flags.logic((s.reg(0,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x16e8c0u); if(s.flags.zf)goto L16e8db;
if(s.services.instruction)s.services.instruction(0x16e8c2u); s.set(1,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e8c4u); s.push(0x16e8c4u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x16e8c5u); s.set(5,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e8c7u); s.set(1,s.flags.right(s.reg(1,4,0),0x2u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x16e8cau); s.set(0,0xbabababau,4,0);
if(s.services.instruction)s.services.instruction(0x16e8cfu); s.set(7,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e8d1u); while(s.gp[1]){const auto x=s.gp[0];w(s.memory,s.observe,0x16e8d1u,s.gp[7],4,x);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x16e8d1u);}
if(s.services.instruction)s.services.instruction(0x16e8d3u); s.set(1,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e8d5u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0x3u),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e8d8u); while(s.gp[1]){const auto x=s.gp[0];w(s.memory,s.observe,0x16e8d8u,s.gp[7],1,x);const auto delta=(s.other_flags&1024u)?4294967295u:1u;s.gp[7]+=delta;--s.gp[1];if(s.services.instruction)s.services.instruction(0x16e8d8u);}
if(s.services.instruction)s.services.instruction(0x16e8dau); s.set(5,s.pop(),4,0);
L16e8db: if(s.services.instruction)s.services.instruction(0x16e8dbu); s.set(0,r(s.memory,(0x48u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e8deu); s.set(1,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e8e0u); s.set(1,s.flags.right(s.reg(1,4,0),0x5u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x16e8e3u); s.set(0,(0x0u+s.gp[0]+s.gp[1]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x16e8e6u); s.set(1,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e8e8u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0x1fu),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e8ebu); s.set(7,0x1u,4,0);
if(s.services.instruction)s.services.instruction(0x16e8f0u); s.set(7,s.flags.shift(s.reg(7,4,0),s.reg(1,1,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e8f2u); s.set(1,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e8f4u); s.set(7,~(s.reg(7,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0x16e8f6u); s.set(1,s.flags.logic((s.reg(1,4,0))&(s.reg(7,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e8f8u); w(s.memory,s.observe,0x16e8f8u,(0x0u+s.gp[0]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x16e8fau); w(s.memory,s.observe,0x16e8fau,(0x0u+s.gp[6]),2,0x0u);
if(s.services.instruction)s.services.instruction(0x16e8ffu); s.flags.minus(s.reg(3,4,0),r(s.memory,(0x34u+s.gp[2]),4),4);
if(s.services.instruction)s.services.instruction(0x16e902u); if(s.flags.sf==s.flags.of)goto L16e907;
if(s.services.instruction)s.services.instruction(0x16e904u); w(s.memory,s.observe,0x16e904u,(0x34u+s.gp[2]),4,s.reg(3,4,0));
L16e907: if(s.services.instruction)s.services.instruction(0x16e907u); s.set(0,r(s.memory,(0x38u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e90au); s.set(3,s.flags.increment(s.reg(3,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x16e90bu); s.flags.minus(s.reg(3,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x16e90du); if(!s.flags.zf)goto L16e927;
if(s.services.instruction)s.services.instruction(0x16e90fu); s.set(0,r(s.memory,(0x24u+s.gp[2]),4),4,0);
L16e912: if(s.services.instruction)s.services.instruction(0x16e912u); s.set(7,r(s.memory,(0x38u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e915u); s.set(6,s.flags.minus(s.reg(6,4,0),s.reg(0,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e917u); s.set(7,s.flags.increment(s.reg(7,4,0),4,true),4,0);
if(s.services.instruction)s.services.instruction(0x16e918u); s.set(1,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16e91au); s.flags.logic((s.reg(1,4,0))&(s.reg(1,4,0)),4);
if(s.services.instruction)s.services.instruction(0x16e91cu); w(s.memory,s.observe,0x16e91cu,(0x38u+s.gp[2]),4,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x16e91fu); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L16e927;
if(s.services.instruction)s.services.instruction(0x16e921u); s.flags.minus(r(s.memory,(0x0u+s.gp[6]),2),0x0u,2);
if(s.services.instruction)s.services.instruction(0x16e925u); if(s.flags.zf)goto L16e912;
L16e927: if(s.services.instruction)s.services.instruction(0x16e927u); s.set(0,r(s.memory,(0x3cu+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16e92au); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x16e92bu); s.set(0,s.flags.increment(s.reg(0,4,0),4,true),4,0);
if(s.services.instruction)s.services.instruction(0x16e92cu); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x16e92du); w(s.memory,s.observe,0x16e92du,(0x3cu+s.gp[2]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x16e930u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x16e931u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_16ea60(MathState &s) {
if(s.services.instruction)s.services.instruction(0x16ea60u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x16ea63u); s.flags.logic((s.reg(2,4,0))&(s.reg(2,4,0)),4);
if(s.services.instruction)s.services.instruction(0x16ea65u); if(s.flags.sf!=s.flags.of)goto L16ea98;
if(s.services.instruction)s.services.instruction(0x16ea67u); s.push(0x16ea67u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x16ea68u); s.set(7,r(s.memory,(0x38u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16ea6bu); s.flags.minus(s.reg(2,4,0),s.reg(7,4,0),4);
if(s.services.instruction)s.services.instruction(0x16ea6du); if(s.flags.sf==s.flags.of)goto L16ea97;
if(s.services.instruction)s.services.instruction(0x16ea6fu); s.push(0x16ea6fu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x16ea70u); s.set(6,r(s.memory,(0x48u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x16ea73u); s.push(0x16ea73u,s.reg(3,4,0));
L16ea74: if(s.services.instruction)s.services.instruction(0x16ea74u); s.set(1,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16ea76u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0x1fu),4),4,0);
if(s.services.instruction)s.services.instruction(0x16ea79u); s.set(3,0x1u,4,0);
if(s.services.instruction)s.services.instruction(0x16ea7eu); s.set(3,s.flags.shift(s.reg(3,4,0),s.reg(1,1,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x16ea80u); s.set(1,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16ea82u); s.set(1,s.flags.right(s.reg(1,4,0),0x5u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x16ea85u); s.flags.logic((r(s.memory,(0x0u+s.gp[6]+s.gp[1]*4u),4))&(s.reg(3,4,0)),4);
if(s.services.instruction)s.services.instruction(0x16ea88u); if(!s.flags.zf)goto L16ea93;
if(s.services.instruction)s.services.instruction(0x16ea8au); s.set(2,s.flags.increment(s.reg(2,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x16ea8bu); s.flags.minus(s.reg(2,4,0),s.reg(7,4,0),4);
if(s.services.instruction)s.services.instruction(0x16ea8du); if(s.flags.sf!=s.flags.of)goto L16ea74;
if(s.services.instruction)s.services.instruction(0x16ea8fu); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x16ea90u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x16ea91u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x16ea92u); s.pop();s.gp[4]+=0u;return;
L16ea93: if(s.services.instruction)s.services.instruction(0x16ea93u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x16ea94u); s.set(0,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x16ea96u); s.set(6,s.pop(),4,0);
L16ea97: if(s.services.instruction)s.services.instruction(0x16ea97u); s.set(7,s.pop(),4,0);
L16ea98: if(s.services.instruction)s.services.instruction(0x16ea98u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_1755b0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x1755b0u); s.push(0x1755b0u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x1755b1u); s.set(3,r(s.memory,(0x4f54f0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1755b7u); s.push(0x1755b7u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x1755b8u); s.push(0x1755b8u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x1755b9u); s.xmm[3]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1755bcu); s.set(2,s.flags.logic((s.reg(2,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1755bfu); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1755c3u); s.set(1,s.flags.logic((s.reg(1,4,0))^(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x1755c5u); s.set(6,(0xcu+s.gp[3]),4,0);
L1755c8: if(s.services.instruction)s.services.instruction(0x1755c8u); s.flags.minus(r(s.memory,(0x0u+s.gp[6]),4),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x1755cbu); if(!s.flags.zf)goto L1755d8;
if(s.services.instruction)s.services.instruction(0x1755cdu); s.set(1,s.flags.increment(s.reg(1,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x1755ceu); s.set(6,s.flags.plus(s.reg(6,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1755d1u); s.flags.minus(s.reg(1,4,0),0x4u,4);
if(s.services.instruction)s.services.instruction(0x1755d4u); if(s.flags.sf!=s.flags.of)goto L1755c8;
if(s.services.instruction)s.services.instruction(0x1755d6u); goto L1755da;
L1755d8: if(s.services.instruction)s.services.instruction(0x1755d8u); s.set(2,s.reg(1,4,0),4,0);
L1755da: if(s.services.instruction)s.services.instruction(0x1755dau); s.flags.minus(s.reg(2,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x1755ddu); if(s.flags.zf)goto L175693;
if(s.services.instruction)s.services.instruction(0x1755e3u); s.set(5,r(s.memory,(0x474c24u),2),2,0);
if(s.services.instruction)s.services.instruction(0x1755eau); s.set(3,(0x0u+s.gp[3]),4,0);
L1755f0: if(s.services.instruction)s.services.instruction(0x1755f0u); s.set(1,s.flags.logic((s.reg(1,4,0))^(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x1755f2u); s.flags.minus(s.reg(2,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x1755f5u); if(s.flags.zf)goto L17560b;
if(s.services.instruction)s.services.instruction(0x1755f7u); s.flags.minus(s.reg(5,2,0),0xffffffffu,2);
if(s.services.instruction)s.services.instruction(0x1755fbu); if(s.flags.zf)goto L17560b;
if(s.services.instruction)s.services.instruction(0x1755fdu); s.set(1,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1755ffu); s.set(1,s.flags.product(s.reg(1,4,0),0x358u),4,0);
if(s.services.instruction)s.services.instruction(0x175605u); s.set(1,s.flags.plus(s.reg(1,4,0),0x4f655cu,4),4,0);
L17560b: if(s.services.instruction)s.services.instruction(0x17560bu); s.flags.minus(s.reg(0,4,0),0x1u,4);
if(s.services.instruction)s.services.instruction(0x17560eu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[1]),4)));
if(s.services.instruction)s.services.instruction(0x175613u); s.xmm[2]=_mm_sub_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x8u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x175618u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[1]),4)));
if(s.services.instruction)s.services.instruction(0x17561cu); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x175620u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[1]),4)));
if(s.services.instruction)s.services.instruction(0x175625u); s.xmm[1]=_mm_sub_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[7]),4))));
if(s.services.instruction)s.services.instruction(0x17562au); s.xmm[4]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x17562du); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x175631u); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x175635u); s.xmm[2]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x175638u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x17563cu); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x175640u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x175644u); if(!s.flags.zf)goto L175659;
if(s.services.instruction)s.services.instruction(0x175646u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x50u+s.gp[1]),4)));
if(s.services.instruction)s.services.instruction(0x17564bu); s.xmm[2]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x17564eu); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x175652u); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x175656u); s.xmm[0]=s.xmm[2];
L175659: if(s.services.instruction)s.services.instruction(0x175659u); s.flags.scalar_compare(s.xmm[3],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x17565cu); if(!s.flags.cf&&!s.flags.zf)goto L175699;
if(s.services.instruction)s.services.instruction(0x17565eu); s.set(6,s.flags.logic((s.reg(6,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x175661u); s.flags.minus(s.reg(2,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x175664u); if(!s.flags.zf)goto L17566a;
if(s.services.instruction)s.services.instruction(0x175666u); s.set(1,s.flags.logic((s.reg(1,4,0))^(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x175668u); goto L17566d;
L17566a: if(s.services.instruction)s.services.instruction(0x17566au); s.set(1,(0x1u+s.gp[2]),4,0);
L17566d: if(s.services.instruction)s.services.instruction(0x17566du); s.flags.minus(s.reg(1,4,0),0x4u,4);
if(s.services.instruction)s.services.instruction(0x175670u); if(s.flags.sf==s.flags.of)goto L175688;
if(s.services.instruction)s.services.instruction(0x175672u); s.set(2,(0xcu+s.gp[3]+s.gp[1]*4u),4,0);
L175676: if(s.services.instruction)s.services.instruction(0x175676u); s.flags.minus(r(s.memory,(0x0u+s.gp[2]),4),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x175679u); if(!s.flags.zf)goto L175686;
if(s.services.instruction)s.services.instruction(0x17567bu); s.set(1,s.flags.increment(s.reg(1,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x17567cu); s.set(2,s.flags.plus(s.reg(2,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17567fu); s.flags.minus(s.reg(1,4,0),0x4u,4);
if(s.services.instruction)s.services.instruction(0x175682u); if(s.flags.sf!=s.flags.of)goto L175676;
if(s.services.instruction)s.services.instruction(0x175684u); goto L175688;
L175686: if(s.services.instruction)s.services.instruction(0x175686u); s.set(6,s.reg(1,4,0),4,0);
L175688: if(s.services.instruction)s.services.instruction(0x175688u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17568bu); s.set(2,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17568du); if(!s.flags.zf)goto L1755f0;
L175693: if(s.services.instruction)s.services.instruction(0x175693u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x175694u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x175695u); s.set(0,s.flags.logic((s.reg(0,1,0))^(s.reg(0,1,0)),1),1,0);
if(s.services.instruction)s.services.instruction(0x175697u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x175698u); s.pop();s.gp[4]+=0u;return;
L175699: if(s.services.instruction)s.services.instruction(0x175699u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17569au); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17569bu); s.set(0,0x1u,1,0);
if(s.services.instruction)s.services.instruction(0x17569du); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17569eu); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_177020(MathState &s) {
if(s.services.instruction)s.services.instruction(0x177020u); s.set(0,r(s.memory,(0x51df74u),4),4,0);
if(s.services.instruction)s.services.instruction(0x177025u); s.set(1,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x177028u); s.push(0x177028u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x177029u); s.push(0x177029u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x17702au); s.set(5,r(s.memory,(0xcu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17702eu); s.push(0x17702eu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17702fu); s.push(0x17702fu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x177030u); s.set(7,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x177032u); s.set(7,s.flags.logic((s.reg(7,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x177038u); s.set(7,s.flags.product(s.reg(7,4,0),0x54u),4,0);
if(s.services.instruction)s.services.instruction(0x17703bu); s.set(7,s.flags.plus(s.reg(7,4,0),s.reg(1,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x17703du); s.set(3,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17703fu); s.push(0x17703fu,0x177044u);s.required(0x17703fu,0x178290u,false);
if(s.services.instruction)s.services.instruction(0x177044u); s.set(6,r(s.memory,(0x40u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x177047u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17704au); s.set(3,(0x40u+s.gp[7]),4,0);
if(s.services.instruction)s.services.instruction(0x17704du); if(s.flags.zf)goto L177083;
if(s.services.instruction)s.services.instruction(0x17704fu); ;
L177050: if(s.services.instruction)s.services.instruction(0x177050u); s.set(2,r(s.memory,(0x51df74u),4),4,0);
if(s.services.instruction)s.services.instruction(0x177056u); s.set(5,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x177059u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17705bu); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x177061u); s.set(1,s.flags.product(s.reg(1,4,0),0x54u),4,0);
if(s.services.instruction)s.services.instruction(0x177064u); s.set(0,(0x44u+s.gp[7]),4,0);
if(s.services.instruction)s.services.instruction(0x177067u); s.set(1,s.flags.plus(s.reg(1,4,0),s.reg(5,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x177069u); s.set(5,r(s.memory,(0x38u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17706cu); s.push(0x17706cu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x17706du); s.push(0x17706du,0x177072u);run_177fa0(s);
if(s.services.instruction)s.services.instruction(0x177072u); s.push(0x177072u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x177073u); s.push(0x177073u,0x177078u);run_177020(s);
if(s.services.instruction)s.services.instruction(0x177078u); s.flags.minus(s.reg(5,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17707bu); s.set(6,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17707du); if(!s.flags.zf)goto L177050;
if(s.services.instruction)s.services.instruction(0x17707fu); s.set(5,r(s.memory,(0x14u+s.gp[4]),4),4,0);
L177083: if(s.services.instruction)s.services.instruction(0x177083u); s.set(0,r(s.memory,(0x18u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x177086u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x177089u); if(s.flags.zf)goto L1770a8;
if(s.services.instruction)s.services.instruction(0x17708bu); s.set(1,sign_extend(r(s.memory,(0x10u+s.gp[7]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x17708fu); s.set(2,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x177095u); s.set(1,s.flags.shift(s.reg(1,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x177098u); s.flags.minus(r(s.memory,(0x0u+s.gp[1]+s.gp[2]*1u),4),0x65666665u,4);
if(s.services.instruction)s.services.instruction(0x17709fu); if(!s.flags.zf)goto L1770a8;
if(s.services.instruction)s.services.instruction(0x1770a1u); s.set(2,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1770a3u); s.push(0x1770a3u,0x1770a8u);s.required(0x1770a3u,0x17ba10u,false);
L1770a8: if(s.services.instruction)s.services.instruction(0x1770a8u); s.set(2,r(s.memory,(0x51df74u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1770aeu); s.set(0,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1770b0u); s.push(0x1770b0u,0x1770b5u);run_16e8a0(s);
if(s.services.instruction)s.services.instruction(0x1770b5u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1770b6u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1770b7u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1770b8u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1770b9u); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_177fa0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x177fa0u); s.push(0x177fa0u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x177fa1u); s.push(0x177fa1u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x177fa2u); s.push(0x177fa2u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x177fa3u); s.set(7,r(s.memory,(0x51df74u),4),4,0);
if(s.services.instruction)s.services.instruction(0x177fa9u); s.set(6,r(s.memory,(0x44u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x177facu); s.set(2,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x177faeu); s.set(2,s.flags.minus(s.reg(2,4,0),s.reg(6,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x177fb0u); s.set(0,0x30c30c31u,4,0);
if(s.services.instruction)s.services.instruction(0x177fb5u); {const auto x=std::int64_t(si(s.gp[0]))*si(s.reg(2,4,0));s.gp[0]=std::uint32_t(x);s.gp[2]=std::uint32_t(std::uint64_t(x)>>32u);s.flags.cf=s.flags.of=x!=std::int64_t(si(s.gp[0]));}
if(s.services.instruction)s.services.instruction(0x177fb7u); s.set(2,s.flags.right(s.reg(2,4,0),0x4u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x177fbau); s.set(0,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x177fbcu); s.set(0,s.flags.right(s.reg(0,4,0),0x1fu,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x177fbfu); s.set(2,s.flags.plus(s.reg(2,4,0),s.reg(0,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x177fc1u); s.set(0,sign_extend(r(s.memory,(0x0u+s.gp[1]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x177fc4u); s.set(0,s.flags.shift(s.reg(0,4,0),0x10u,4),4,0);
if(s.services.instruction)s.services.instruction(0x177fc7u); s.set(2,s.flags.logic((s.reg(2,4,0))|(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x177fc9u); s.set(0,r(s.memory,(0x38u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x177fccu); s.set(5,s.flags.logic((s.reg(5,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x177fcfu); s.flags.minus(s.reg(0,4,0),s.reg(5,4,0),4);
if(s.services.instruction)s.services.instruction(0x177fd1u); if(s.flags.zf)goto L177fe5;
if(s.services.instruction)s.services.instruction(0x177fd3u); s.set(5,r(s.memory,(0x3cu+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x177fd6u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x177fdbu); s.set(0,s.flags.product(s.reg(0,4,0),0x54u),4,0);
if(s.services.instruction)s.services.instruction(0x177fdeu); w(s.memory,s.observe,0x177fdeu,(0x3cu+s.gp[0]+s.gp[6]*1u),4,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x177fe2u); s.set(5,s.flags.logic((s.reg(5,4,0))|(0xffffffffu),4),4,0);
L177fe5: if(s.services.instruction)s.services.instruction(0x177fe5u); s.set(0,r(s.memory,(0x3cu+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x177fe8u); s.flags.minus(s.reg(0,4,0),s.reg(5,4,0),4);
if(s.services.instruction)s.services.instruction(0x177feau); if(s.flags.zf)goto L177ffe;
if(s.services.instruction)s.services.instruction(0x177fecu); s.set(6,r(s.memory,(0x44u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x177fefu); s.set(7,r(s.memory,(0x38u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x177ff2u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x177ff7u); s.set(0,s.flags.product(s.reg(0,4,0),0x54u),4,0);
if(s.services.instruction)s.services.instruction(0x177ffau); w(s.memory,s.observe,0x177ffau,(0x38u+s.gp[0]+s.gp[6]*1u),4,s.reg(7,4,0));
L177ffe: if(s.services.instruction)s.services.instruction(0x177ffeu); s.flags.minus(r(s.memory,(0x0u+s.gp[3]),4),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x178000u); if(!s.flags.zf)goto L178007;
if(s.services.instruction)s.services.instruction(0x178002u); s.set(0,r(s.memory,(0x38u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178005u); w(s.memory,s.observe,0x178005u,(0x0u+s.gp[3]),4,s.reg(0,4,0));
L178007: if(s.services.instruction)s.services.instruction(0x178007u); s.set(0,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17800bu); s.flags.minus(r(s.memory,(0x0u+s.gp[0]),4),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x17800du); if(!s.flags.zf)goto L178014;
if(s.services.instruction)s.services.instruction(0x17800fu); s.set(2,r(s.memory,(0x3cu+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x178012u); w(s.memory,s.observe,0x178012u,(0x0u+s.gp[0]),4,s.reg(2,4,0));
L178014: if(s.services.instruction)s.services.instruction(0x178014u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x178015u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x178016u); w(s.memory,s.observe,0x178016u,(0x38u+s.gp[1]),4,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x178019u); w(s.memory,s.observe,0x178019u,(0x3cu+s.gp[1]),4,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x17801cu); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17801du); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_17a030(MathState &s) {
if(s.services.instruction)s.services.instruction(0x17a030u); s.set(0,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a034u); s.set(1,r(s.memory,(0x4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a037u); s.set(2,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a03du); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a043u); s.set(4,s.flags.minus(s.reg(4,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17a046u); s.set(1,s.flags.shift(s.reg(1,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17a049u); s.push(0x17a049u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x17a04au); s.set(5,r(s.memory,(0x8u+s.gp[1]+s.gp[2]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a04eu); s.set(0,r(s.memory,(0x0u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a051u); s.set(0,s.flags.right(s.reg(0,4,0),0x5u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x17a054u); s.flags.logic((s.reg(0,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x17a056u); if(s.flags.zf)goto L17a0e3;
if(s.services.instruction)s.services.instruction(0x17a05cu); s.set(1,r(s.memory,(0x14u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a05fu); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a061u); s.flags.minus(s.reg(1,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a063u); w(s.memory,s.observe,0x17a063u,(0x8u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x17a067u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L17a0e3;
if(s.services.instruction)s.services.instruction(0x17a069u); s.xmm[4]=_mm_xor_ps(s.xmm[4],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x17a06cu); s.push(0x17a06cu,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x17a06du); s.push(0x17a06du,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17a06eu); w(s.memory,s.observe,0x17a06eu,(0xcu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x17a072u); s.push(0x17a072u,s.reg(7,4,0));
L17a073: if(s.services.instruction)s.services.instruction(0x17a073u); s.set(6,r(s.memory,(0x18u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a076u); s.set(1,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a07au); s.set(0,r(s.memory,(0x18u+s.gp[6]+s.gp[1]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a07eu); s.set(6,s.flags.plus(s.reg(6,4,0),s.reg(1,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a080u); s.set(3,s.flags.logic((s.reg(3,4,0))^(s.reg(3,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a082u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x17a084u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L17a0c5;
if(s.services.instruction)s.services.instruction(0x17a086u); s.set(7,s.flags.logic((s.reg(7,4,0))^(s.reg(7,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a088u); goto L17a090;
L17a090: if(s.services.instruction)s.services.instruction(0x17a090u); s.set(0,r(s.memory,(0x1cu+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a093u); s.set(1,r(s.memory,(0xcu+s.gp[0]+s.gp[7]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a097u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(7,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a099u); s.flags.minus(s.reg(1,4,0),0x7464746cu,4);
if(s.services.instruction)s.services.instruction(0x17a09fu); if(!s.flags.zf)goto L17a0ba;
if(s.services.instruction)s.services.instruction(0x17a0a1u); s.set(0,r(s.memory,(0x10u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a0a4u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a0a7u); if(s.flags.zf)goto L17a0ba;
if(s.services.instruction)s.services.instruction(0x17a0a9u); s.set(1,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a0adu); s.set(2,r(s.memory,(0x48u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a0b0u); s.push(0x17a0b0u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x17a0b1u); s.push(0x17a0b1u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x17a0b2u); s.xmm[1]=s.xmm[4];
if(s.services.instruction)s.services.instruction(0x17a0b5u); s.push(0x17a0b5u,0x17a0bau);run_c6120(s);
L17a0ba: if(s.services.instruction)s.services.instruction(0x17a0bau); s.set(0,r(s.memory,(0x18u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a0bdu); s.set(3,s.flags.increment(s.reg(3,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x17a0beu); s.set(7,s.flags.plus(s.reg(7,4,0),0x38u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17a0c1u); s.flags.minus(s.reg(3,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a0c3u); if(s.flags.sf!=s.flags.of)goto L17a090;
L17a0c5: if(s.services.instruction)s.services.instruction(0x17a0c5u); s.set(0,r(s.memory,(0x14u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a0c9u); s.set(2,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a0cdu); s.set(1,r(s.memory,(0x14u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a0d0u); s.set(0,s.flags.increment(s.reg(0,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x17a0d1u); s.set(2,s.flags.plus(s.reg(2,4,0),0x38u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17a0d4u); s.flags.minus(s.reg(0,4,0),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a0d6u); w(s.memory,s.observe,0x17a0d6u,(0x14u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x17a0dau); w(s.memory,s.observe,0x17a0dau,(0x10u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x17a0deu); if(s.flags.sf!=s.flags.of)goto L17a073;
if(s.services.instruction)s.services.instruction(0x17a0e0u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a0e1u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a0e2u); s.set(3,s.pop(),4,0);
L17a0e3: if(s.services.instruction)s.services.instruction(0x17a0e3u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a0e4u); s.set(4,s.flags.plus(s.reg(4,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17a0e7u); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_17a230(MathState &s) {
if(s.services.instruction)s.services.instruction(0x17a230u); s.push(0x17a230u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17a231u); s.set(6,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a233u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a236u); if(s.flags.zf)goto L17a2e2;
if(s.services.instruction)s.services.instruction(0x17a23cu); s.set(2,r(s.memory,(0x4f720cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a242u); s.set(0,r(s.memory,(0x38u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a245u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a247u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a24du); s.flags.minus(s.reg(1,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a24fu); if(s.flags.sf==s.flags.of)goto L17a2e2;
if(s.services.instruction)s.services.instruction(0x17a255u); s.set(0,r(s.memory,(0x24u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a258u); s.set(0,s.flags.product(s.reg(0,4,0),s.reg(1,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0x17a25bu); s.push(0x17a25bu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x17a25cu); s.set(7,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a25fu); s.set(1,r(s.memory,(0x0u+s.gp[0]+s.gp[7]*1u),2),2,0);
if(s.services.instruction)s.services.instruction(0x17a263u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(7,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a265u); s.flags.logic((s.reg(1,2,0))&(s.reg(1,2,0)),2);
if(s.services.instruction)s.services.instruction(0x17a268u); if(s.flags.zf)goto L17a2e1;
if(s.services.instruction)s.services.instruction(0x17a26au); s.set(1,sign_extend(s.reg(1,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x17a26du); s.set(6,s.flags.right(s.reg(6,4,0),0x10u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x17a270u); s.flags.minus(s.reg(1,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a272u); if(!s.flags.zf)goto L17a2e1;
if(s.services.instruction)s.services.instruction(0x17a274u); s.set(2,r(s.memory,(0x4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a277u); s.set(1,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a27du); s.set(2,s.flags.logic((s.reg(2,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a283u); s.push(0x17a283u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x17a284u); s.set(2,s.flags.shift(s.reg(2,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17a287u); s.push(0x17a287u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x17a288u); s.set(5,r(s.memory,(0x8u+s.gp[2]+s.gp[1]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a28cu); s.set(1,r(s.memory,(0xcu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a28fu); s.set(3,s.flags.logic((s.reg(3,4,0))^(s.reg(3,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a291u); s.flags.logic((s.reg(1,4,0))&(s.reg(1,4,0)),4);
if(s.services.instruction)s.services.instruction(0x17a293u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L17a2df;
if(s.services.instruction)s.services.instruction(0x17a295u); s.set(2,r(s.memory,(0x4f7208u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a29bu); s.set(7,(0x7cu+s.gp[0]),4,0);
if(s.services.instruction)s.services.instruction(0x17a29eu); s.set(7,s.reg(7,4,0),4,0);
L17a2a0: if(s.services.instruction)s.services.instruction(0x17a2a0u); s.set(0,r(s.memory,(0x0u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a2a2u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a2a5u); if(s.flags.zf)goto L17a2ce;
if(s.services.instruction)s.services.instruction(0x17a2a7u); goto L17a2b0;
L17a2b0: if(s.services.instruction)s.services.instruction(0x17a2b0u); s.set(6,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a2b3u); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a2b5u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a2bbu); s.set(1,s.flags.product(s.reg(1,4,0),0x3cu),4,0);
if(s.services.instruction)s.services.instruction(0x17a2beu); s.set(6,r(s.memory,(0x4u+s.gp[1]+s.gp[6]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a2c2u); s.push(0x17a2c2u,0x17a2c7u);run_16e8a0(s);
if(s.services.instruction)s.services.instruction(0x17a2c7u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a2cau); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a2ccu); if(!s.flags.zf)goto L17a2b0;
L17a2ce: if(s.services.instruction)s.services.instruction(0x17a2ceu); w(s.memory,s.observe,0x17a2ceu,(0x0u+s.gp[7]),4,0xffffffffu);
if(s.services.instruction)s.services.instruction(0x17a2d4u); s.set(0,r(s.memory,(0xcu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a2d7u); s.set(3,s.flags.increment(s.reg(3,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x17a2d8u); s.set(7,s.flags.plus(s.reg(7,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17a2dbu); s.flags.minus(s.reg(3,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a2ddu); if(s.flags.sf!=s.flags.of)goto L17a2a0;
L17a2df: if(s.services.instruction)s.services.instruction(0x17a2dfu); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a2e0u); s.set(3,s.pop(),4,0);
L17a2e1: if(s.services.instruction)s.services.instruction(0x17a2e1u); s.set(7,s.pop(),4,0);
L17a2e2: if(s.services.instruction)s.services.instruction(0x17a2e2u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a2e3u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_17a2f0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x17a2f0u); s.set(0,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a2f4u); s.push(0x17a2f4u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x17a2f5u); s.set(5,(0x100u+s.gp[0]),4,0);
if(s.services.instruction)s.services.instruction(0x17a2fbu); s.push(0x17a2fbu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17a2fcu); s.set(6,r(s.memory,(0x0u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a2ffu); s.set(2,s.flags.logic((s.reg(2,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a302u); s.flags.minus(s.reg(6,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a304u); if(s.flags.zf)goto L17a343;
if(s.services.instruction)s.services.instruction(0x17a306u); s.push(0x17a306u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x17a307u); s.push(0x17a307u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x17a308u); s.set(3,(0xfcu+s.gp[0]),4,0);
if(s.services.instruction)s.services.instruction(0x17a30eu); s.set(7,s.reg(7,4,0),4,0);
L17a310: if(s.services.instruction)s.services.instruction(0x17a310u); s.set(0,r(s.memory,(0x51df74u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a315u); s.set(7,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a318u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a31au); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a320u); s.set(1,s.flags.product(s.reg(1,4,0),0x54u),4,0);
if(s.services.instruction)s.services.instruction(0x17a323u); s.set(1,s.flags.plus(s.reg(1,4,0),s.reg(7,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a325u); s.set(7,r(s.memory,(0x3cu+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a328u); s.push(0x17a328u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x17a329u); s.push(0x17a329u,0x17a32eu);run_177fa0(s);
if(s.services.instruction)s.services.instruction(0x17a32eu); s.push(0x17a32eu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17a32fu); s.push(0x17a32fu,0x17a334u);run_177020(s);
if(s.services.instruction)s.services.instruction(0x17a334u); s.flags.minus(s.reg(7,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a337u); s.set(6,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a339u); if(!s.flags.zf)goto L17a310;
if(s.services.instruction)s.services.instruction(0x17a33bu); s.set(0,r(s.memory,(0x14u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a33fu); s.set(2,s.flags.logic((s.reg(2,4,0))|(s.reg(7,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a341u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a342u); s.set(3,s.pop(),4,0);
L17a343: if(s.services.instruction)s.services.instruction(0x17a343u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a344u); w(s.memory,s.observe,0x17a344u,(0xfcu+s.gp[0]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x17a34au); w(s.memory,s.observe,0x17a34au,(0x0u+s.gp[5]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x17a34du); s.set(0,s.flags.plus(s.reg(0,4,0),0x110u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17a352u); s.set(1,0x10u,4,0);
if(s.services.instruction)s.services.instruction(0x17a357u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a358u); goto L17a360;
L17a360: if(s.services.instruction)s.services.instruction(0x17a360u); w(s.memory,s.observe,0x17a360u,(0x0u+s.gp[0]),4,0x0u);
if(s.services.instruction)s.services.instruction(0x17a366u); w(s.memory,s.observe,0x17a366u,(0x4u+s.gp[0]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x17a369u); s.set(0,s.flags.plus(s.reg(0,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0x17a36cu); s.set(1,s.flags.increment(s.reg(1,4,0),4,true),4,0);
if(s.services.instruction)s.services.instruction(0x17a36du); if(!s.flags.zf)goto L17a360;
if(s.services.instruction)s.services.instruction(0x17a36fu); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_17a380(MathState &s) {
if(s.services.instruction)s.services.instruction(0x17a380u); s.push(0x17a380u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17a381u); s.set(6,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a383u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a386u); if(s.flags.zf)goto L17a40f;
if(s.services.instruction)s.services.instruction(0x17a38cu); s.push(0x17a38cu,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x17a38du); s.set(3,r(s.memory,(0x4f720cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a393u); s.set(1,r(s.memory,(0x38u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a396u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a39bu); s.flags.minus(s.reg(0,4,0),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a39du); if(s.flags.sf==s.flags.of)goto L17a40e;
if(s.services.instruction)s.services.instruction(0x17a39fu); s.set(2,r(s.memory,(0x44u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a3a2u); s.push(0x17a3a2u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x17a3a3u); s.set(7,r(s.memory,(0x24u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a3a6u); s.set(7,s.flags.product(s.reg(7,4,0),s.reg(0,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0x17a3a9u); s.set(0,r(s.memory,(0x0u+s.gp[7]+s.gp[2]*1u),2),2,0);
if(s.services.instruction)s.services.instruction(0x17a3adu); s.set(7,s.flags.plus(s.reg(7,4,0),s.reg(2,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a3afu); s.flags.logic((s.reg(0,2,0))&(s.reg(0,2,0)),2);
if(s.services.instruction)s.services.instruction(0x17a3b2u); if(s.flags.zf)goto L17a40d;
if(s.services.instruction)s.services.instruction(0x17a3b4u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a3b6u); s.set(2,sign_extend(s.reg(0,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x17a3b9u); s.set(1,s.flags.right(s.reg(1,4,0),0x10u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x17a3bcu); s.flags.minus(s.reg(2,4,0),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a3beu); if(!s.flags.zf)goto L17a40d;
if(s.services.instruction)s.services.instruction(0x17a3c0u); s.push(0x17a3c0u,0x17a3c5u);run_17a420(s);
if(s.services.instruction)s.services.instruction(0x17a3c5u); s.set(0,r(s.memory,(0x8u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a3c8u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a3cbu); if(s.flags.zf)goto L17a3ee;
if(s.services.instruction)s.services.instruction(0x17a3cdu); s.set(1,r(s.memory,(0x4f9b5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a3d3u); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a3d6u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a3dbu); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x17a3deu); w(s.memory,s.observe,0x17a3deu,(0x4u+s.gp[2]+s.gp[0]*8u),1,s.flags.logic((r(s.memory,(0x4u+s.gp[2]+s.gp[0]*8u),1))|(0x2u),1));
if(s.services.instruction)s.services.instruction(0x17a3e3u); s.set(0,(0x4u+s.gp[2]+s.gp[0]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x17a3e7u); w(s.memory,s.observe,0x17a3e7u,(0x8u+s.gp[7]),4,0xffffffffu);
L17a3ee: if(s.services.instruction)s.services.instruction(0x17a3eeu); s.push(0x17a3eeu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x17a3efu); s.push(0x17a3efu,0x17a3f4u);run_17a030(s);
if(s.services.instruction)s.services.instruction(0x17a3f4u); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a3f6u); s.push(0x17a3f6u,0x17a3fbu);run_17a230(s);
if(s.services.instruction)s.services.instruction(0x17a3fbu); s.push(0x17a3fbu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x17a3fcu); s.push(0x17a3fcu,0x17a401u);run_17a2f0(s);
if(s.services.instruction)s.services.instruction(0x17a401u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a402u); s.set(2,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a404u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a405u); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a407u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a408u); run_16e8a0(s);return;
L17a40d: if(s.services.instruction)s.services.instruction(0x17a40du); s.set(7,s.pop(),4,0);
L17a40e: if(s.services.instruction)s.services.instruction(0x17a40eu); s.set(3,s.pop(),4,0);
L17a40f: if(s.services.instruction)s.services.instruction(0x17a40fu); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a410u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_17a420(MathState &s) {
if(s.services.instruction)s.services.instruction(0x17a420u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a423u); if(s.flags.zf)goto L17a49a;
if(s.services.instruction)s.services.instruction(0x17a425u); s.set(2,r(s.memory,(0x4f720cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a42bu); s.set(0,r(s.memory,(0x38u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a42eu); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a430u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a436u); s.flags.minus(s.reg(1,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a438u); if(s.flags.sf==s.flags.of)goto L17a49a;
if(s.services.instruction)s.services.instruction(0x17a43au); s.set(0,r(s.memory,(0x24u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a43du); s.set(0,s.flags.product(s.reg(0,4,0),s.reg(1,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0x17a440u); s.set(0,s.flags.plus(s.reg(0,4,0),r(s.memory,(0x44u+s.gp[2]),4),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a443u); s.set(1,r(s.memory,(0x0u+s.gp[0]),2),2,0);
if(s.services.instruction)s.services.instruction(0x17a446u); s.flags.logic((s.reg(1,2,0))&(s.reg(1,2,0)),2);
if(s.services.instruction)s.services.instruction(0x17a449u); if(s.flags.zf)goto L17a49a;
if(s.services.instruction)s.services.instruction(0x17a44bu); s.set(2,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a44du); s.set(1,sign_extend(s.reg(1,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x17a450u); s.set(2,s.flags.right(s.reg(2,4,0),0x10u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x17a453u); s.flags.minus(s.reg(1,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a455u); if(!s.flags.zf)goto L17a49a;
if(s.services.instruction)s.services.instruction(0x17a457u); s.set(0,r(s.memory,(0x48u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a45au); s.push(0x17a45au,0xffffffffu);
if(s.services.instruction)s.services.instruction(0x17a45cu); s.push(0x17a45cu,0x17a461u);run_bcdf0(s);
if(s.services.instruction)s.services.instruction(0x17a461u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x17a463u); if(s.flags.zf)goto L17a49a;
if(s.services.instruction)s.services.instruction(0x17a465u); s.set(2,sign_extend(r(s.memory,(0x11cu+s.gp[0]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x17a46cu); s.set(1,sign_extend(r(s.memory,(0x11eu+s.gp[0]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x17a473u); s.set(1,s.flags.plus(s.reg(1,4,0),s.reg(0,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a475u); s.set(2,s.flags.right(s.reg(2,4,0),0x3u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x17a478u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a47au); s.flags.logic((s.reg(2,4,0))&(s.reg(2,4,0)),4);
if(s.services.instruction)s.services.instruction(0x17a47cu); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L17a49a;
if(s.services.instruction)s.services.instruction(0x17a47eu); s.set(7,s.reg(7,4,0),4,0);
L17a480: if(s.services.instruction)s.services.instruction(0x17a480u); s.flags.minus(r(s.memory,(0x4u+s.gp[1]+s.gp[0]*8u),4),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a484u); if(!s.flags.zf)goto L17a48c;
if(s.services.instruction)s.services.instruction(0x17a486u); s.flags.minus(r(s.memory,(0x0u+s.gp[1]+s.gp[0]*8u),1),0x2u,1);
if(s.services.instruction)s.services.instruction(0x17a48au); if(s.flags.zf)goto L17a492;
L17a48c: if(s.services.instruction)s.services.instruction(0x17a48cu); s.set(0,s.flags.increment(s.reg(0,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x17a48du); s.flags.minus(s.reg(0,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a48fu); if(s.flags.sf!=s.flags.of)goto L17a480;
if(s.services.instruction)s.services.instruction(0x17a491u); s.pop();s.gp[4]+=0u;return;
L17a492: if(s.services.instruction)s.services.instruction(0x17a492u); w(s.memory,s.observe,0x17a492u,(0x4u+s.gp[1]+s.gp[0]*8u),4,0xffffffffu);
L17a49a: if(s.services.instruction)s.services.instruction(0x17a49au); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_17a4a0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x17a4a0u); s.push(0x17a4a0u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x17a4a1u); s.push(0x17a4a1u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x17a4a2u); s.push(0x17a4a2u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x17a4a3u); s.set(6,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a4a5u); s.set(7,s.flags.logic((s.reg(7,4,0))^(s.reg(7,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a4a7u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a4aau); if(s.flags.zf)goto L17a4e3;
if(s.services.instruction)s.services.instruction(0x17a4acu); s.set(2,r(s.memory,(0x4f720cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a4b2u); s.set(0,r(s.memory,(0x38u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a4b5u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a4b7u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a4bdu); s.flags.minus(s.reg(1,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a4bfu); if(s.flags.sf==s.flags.of)goto L17a4e3;
if(s.services.instruction)s.services.instruction(0x17a4c1u); s.set(0,r(s.memory,(0x24u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a4c4u); s.set(3,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a4c7u); s.set(0,s.flags.product(s.reg(0,4,0),s.reg(1,4,0)),4,0);
if(s.services.instruction)s.services.instruction(0x17a4cau); s.set(1,r(s.memory,(0x0u+s.gp[0]+s.gp[3]*1u),2),2,0);
if(s.services.instruction)s.services.instruction(0x17a4ceu); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(3,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a4d0u); s.flags.logic((s.reg(1,2,0))&(s.reg(1,2,0)),2);
if(s.services.instruction)s.services.instruction(0x17a4d3u); if(s.flags.zf)goto L17a4e3;
if(s.services.instruction)s.services.instruction(0x17a4d5u); s.set(2,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a4d7u); s.set(1,sign_extend(s.reg(1,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x17a4dau); s.set(2,s.flags.right(s.reg(2,4,0),0x10u,4,true),4,0);
if(s.services.instruction)s.services.instruction(0x17a4ddu); s.flags.minus(s.reg(1,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x17a4dfu); if(!s.flags.zf)goto L17a4e3;
if(s.services.instruction)s.services.instruction(0x17a4e1u); s.set(7,s.reg(0,4,0),4,0);
L17a4e3: if(s.services.instruction)s.services.instruction(0x17a4e3u); s.flags.logic((s.reg(7,4,0))&(s.reg(7,4,0)),4);
if(s.services.instruction)s.services.instruction(0x17a4e5u); s.set(3,0x1u,1,0);
if(s.services.instruction)s.services.instruction(0x17a4e7u); if(s.flags.zf)goto L17a526;
if(s.services.instruction)s.services.instruction(0x17a4e9u); s.push(0x17a4e9u,0x17a4eeu);run_17a420(s);
if(s.services.instruction)s.services.instruction(0x17a4eeu); s.set(0,r(s.memory,(0x8u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a4f1u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a4f4u); if(s.flags.zf)goto L17a516;
if(s.services.instruction)s.services.instruction(0x17a4f6u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a4fbu); s.set(2,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x17a4feu); s.set(0,r(s.memory,(0x4f9b5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a503u); s.set(1,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a506u); w(s.memory,s.observe,0x17a506u,(0x4u+s.gp[1]+s.gp[2]*8u),1,s.flags.logic((r(s.memory,(0x4u+s.gp[1]+s.gp[2]*8u),1))|(0x2u),1));
if(s.services.instruction)s.services.instruction(0x17a50bu); s.set(0,(0x4u+s.gp[1]+s.gp[2]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x17a50fu); w(s.memory,s.observe,0x17a50fu,(0x8u+s.gp[7]),4,0xffffffffu);
L17a516: if(s.services.instruction)s.services.instruction(0x17a516u); s.flags.minus(r(s.memory,(0xfcu+s.gp[7]),4),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a51du); if(!s.flags.zf)goto L17a52c;
if(s.services.instruction)s.services.instruction(0x17a51fu); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x17a521u); s.push(0x17a521u,0x17a526u);run_17a380(s);
L17a526: if(s.services.instruction)s.services.instruction(0x17a526u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a527u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a528u); s.set(0,s.reg(3,1,0),1,0);
if(s.services.instruction)s.services.instruction(0x17a52au); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a52bu); s.pop();s.gp[4]+=0u;return;
L17a52c: if(s.services.instruction)s.services.instruction(0x17a52cu); s.set(0,r(s.memory,(0x100u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a532u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a535u); if(s.flags.zf)goto L17a55c;
if(s.services.instruction)s.services.instruction(0x17a537u); s.set(1,r(s.memory,(0x51df74u),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a53du); s.set(2,0xfffeu,4,0);
L17a542: if(s.services.instruction)s.services.instruction(0x17a542u); s.set(6,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a545u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a54au); s.set(0,s.flags.product(s.reg(0,4,0),0x54u),4,0);
if(s.services.instruction)s.services.instruction(0x17a54du); w(s.memory,s.observe,0x17a54du,(0xcu+s.gp[0]+s.gp[6]*1u),2,s.flags.logic((r(s.memory,(0xcu+s.gp[0]+s.gp[6]*1u),2))&(s.reg(2,2,0)),2));
if(s.services.instruction)s.services.instruction(0x17a552u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(6,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a554u); s.set(0,r(s.memory,(0x3cu+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x17a557u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x17a55au); if(!s.flags.zf)goto L17a542;
L17a55c: if(s.services.instruction)s.services.instruction(0x17a55cu); w(s.memory,s.observe,0x17a55cu,(0x2u+s.gp[7]),1,s.flags.logic((r(s.memory,(0x2u+s.gp[7]),1))|(0x44u),1));
if(s.services.instruction)s.services.instruction(0x17a560u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a561u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a562u); s.set(0,s.flags.logic((s.reg(0,1,0))^(s.reg(0,1,0)),1),1,0);
if(s.services.instruction)s.services.instruction(0x17a564u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x17a565u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_18add0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x18add0u); s.push(0x18add0u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x18add1u); s.push(0x18add1u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x18add2u); s.flags.logic((s.reg(2,4,0))&(s.reg(2,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18add4u); s.push(0x18add4u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x18add5u); s.set(5,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18add9u); s.set(0,s.flags.zf,1,0);
if(s.services.instruction)s.services.instruction(0x18addcu); s.set(2,s.flags.logic((s.reg(2,4,0))^(s.reg(2,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18addeu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x18ade0u); s.push(0x18ade0u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18ade1u); s.push(0x18ade1u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x18ade2u); w(s.memory,s.observe,0x18ade2u,(0x18u+s.gp[4]),1,s.reg(0,1,0));
if(s.services.instruction)s.services.instruction(0x18ade6u); w(s.memory,s.observe,0x18ade6u,(0x10u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18adeau); if(s.flags.zf)goto L18adf1;
if(s.services.instruction)s.services.instruction(0x18adecu); s.set(0,r(s.memory,(0x8u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18adefu); goto L18adf4;
L18adf1: if(s.services.instruction)s.services.instruction(0x18adf1u); s.set(0,r(s.memory,(0x10u+s.gp[5]),4),4,0);
L18adf4: if(s.services.instruction)s.services.instruction(0x18adf4u); s.set(7,r(s.memory,(0x1cu+s.gp[4]),2),2,0);
if(s.services.instruction)s.services.instruction(0x18adf9u); s.set(4,(0x0u+s.gp[4]),4,0);
L18ae00: if(s.services.instruction)s.services.instruction(0x18ae00u); s.flags.minus(s.reg(7,2,0),0xffffffffu,2);
if(s.services.instruction)s.services.instruction(0x18ae04u); if(s.flags.zf)goto L18ae72;
if(s.services.instruction)s.services.instruction(0x18ae06u); s.flags.logic((s.reg(7,2,0))&(s.reg(7,2,0)),2);
if(s.services.instruction)s.services.instruction(0x18ae09u); if(s.flags.sf!=s.flags.of)goto L18ae72;
if(s.services.instruction)s.services.instruction(0x18ae0bu); s.set(6,r(s.memory,(0x4ecc1cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae11u); s.set(3,r(s.memory,(0x150u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae17u); s.set(1,sign_extend(s.reg(7,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x18ae1au); s.flags.minus(s.reg(1,4,0),s.reg(3,4,0),4);
if(s.services.instruction)s.services.instruction(0x18ae1cu); if(s.flags.sf==s.flags.of)goto L18ae72;
if(s.services.instruction)s.services.instruction(0x18ae1eu); s.set(6,r(s.memory,(0x154u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae24u); s.set(1,s.flags.product(s.reg(1,4,0),0xb4u),4,0);
if(s.services.instruction)s.services.instruction(0x18ae2au); s.set(3,(0x0u+s.gp[1]+s.gp[6]*1u),4,0);
if(s.services.instruction)s.services.instruction(0x18ae2du); s.flags.logic((s.reg(3,4,0))&(s.reg(3,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18ae2fu); if(s.flags.zf)goto L18ae72;
if(s.services.instruction)s.services.instruction(0x18ae31u); s.set(6,s.flags.logic((s.reg(6,4,0))^(s.reg(6,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae33u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18ae35u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L18ae6a;
if(s.services.instruction)s.services.instruction(0x18ae37u); s.set(2,s.flags.logic((s.reg(2,4,0))^(s.reg(2,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae39u); s.set(4,(0x0u+s.gp[4]),4,0);
L18ae40: if(s.services.instruction)s.services.instruction(0x18ae40u); s.set(1,r(s.memory,(0x18u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x18ae44u); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x18ae46u); if(s.flags.zf)goto L18ae4d;
if(s.services.instruction)s.services.instruction(0x18ae48u); s.set(1,r(s.memory,(0xcu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae4bu); goto L18ae50;
L18ae4d: if(s.services.instruction)s.services.instruction(0x18ae4du); s.set(1,r(s.memory,(0x14u+s.gp[5]),4),4,0);
L18ae50: if(s.services.instruction)s.services.instruction(0x18ae50u); s.set(1,s.flags.plus(s.reg(1,4,0),s.reg(2,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae52u); s.flags.minus(r(s.memory,(0x14u+s.gp[1]),2),s.reg(7,2,0),2);
if(s.services.instruction)s.services.instruction(0x18ae56u); if(s.flags.zf)goto L18ae62;
if(s.services.instruction)s.services.instruction(0x18ae58u); s.set(6,s.flags.increment(s.reg(6,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x18ae59u); s.set(2,s.flags.plus(s.reg(2,4,0),0x18u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae5cu); s.flags.minus(s.reg(6,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x18ae5eu); if(s.flags.sf!=s.flags.of)goto L18ae40;
if(s.services.instruction)s.services.instruction(0x18ae60u); goto L18ae66;
L18ae62: if(s.services.instruction)s.services.instruction(0x18ae62u); w(s.memory,s.observe,0x18ae62u,(0x10u+s.gp[4]),4,s.reg(1,4,0));
L18ae66: if(s.services.instruction)s.services.instruction(0x18ae66u); s.set(2,r(s.memory,(0x10u+s.gp[4]),4),4,0);
L18ae6a: if(s.services.instruction)s.services.instruction(0x18ae6au); s.flags.logic((s.reg(2,4,0))&(s.reg(2,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18ae6cu); s.set(7,r(s.memory,(0x8u+s.gp[3]),2),2,0);
if(s.services.instruction)s.services.instruction(0x18ae70u); if(s.flags.zf)goto L18ae00;
L18ae72: if(s.services.instruction)s.services.instruction(0x18ae72u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18ae73u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18ae74u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18ae75u); s.set(0,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18ae77u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18ae78u); s.set(1,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18ae79u); s.pop();s.gp[4]+=8u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_18ae80(MathState &s) {
if(s.services.instruction)s.services.instruction(0x18ae80u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae83u); s.flags.minus(s.reg(2,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18ae86u); if(s.flags.zf)goto L18aefa;
if(s.services.instruction)s.services.instruction(0x18ae88u); s.push(0x18ae88u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x18ae89u); s.push(0x18ae89u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18ae8au); s.set(6,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae90u); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae96u); s.set(1,s.flags.shift(s.reg(1,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae99u); s.set(6,r(s.memory,(0x8u+s.gp[1]+s.gp[6]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae9du); s.set(3,r(s.memory,(0x0u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18ae9fu); s.push(0x18ae9fu,s.reg(5,4,0));
L18aea0: if(s.services.instruction)s.services.instruction(0x18aea0u); s.flags.minus(s.reg(2,4,0),s.reg(3,4,0),4);
if(s.services.instruction)s.services.instruction(0x18aea2u); if(s.flags.sf==s.flags.of)goto L18aebe;
if(s.services.instruction)s.services.instruction(0x18aea4u); s.flags.logic((s.reg(7,4,0))&(s.reg(7,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18aea6u); s.set(5,r(s.memory,(0x4u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18aea9u); s.set(1,(0x0u+s.gp[2]+s.gp[2]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x18aeacu); s.set(1,(0x0u+s.gp[5]+s.gp[1]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x18aeb0u); if(!s.flags.zf)goto L18aeb7;
if(s.services.instruction)s.services.instruction(0x18aeb2u); s.set(5,r(s.memory,(0x8u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18aeb5u); goto L18aeba;
L18aeb7: if(s.services.instruction)s.services.instruction(0x18aeb7u); s.set(5,r(s.memory,(0x10u+s.gp[1]),4),4,0);
L18aeba: if(s.services.instruction)s.services.instruction(0x18aebau); s.flags.logic((s.reg(5,4,0))&(s.reg(5,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18aebcu); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L18aef5;
L18aebe: if(s.services.instruction)s.services.instruction(0x18aebeu); s.set(2,s.flags.plus(s.reg(2,4,0),0xfffffff3u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18aec1u); s.set(1,s.flags.logic((s.reg(1,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18aec4u); s.flags.minus(s.reg(2,4,0),0x7u,4);
if(s.services.instruction)s.services.instruction(0x18aec7u); if(!s.flags.cf&&!s.flags.zf)goto L18aeea;
if(s.services.instruction)s.services.instruction(0x18aec9u); switch(r(s.memory,(0x18aefcu+s.gp[2]*4u),4)){case 0x18aed0u:goto L18aed0;case 0x18aed7u:goto L18aed7;case 0x18aedeu:goto L18aede;case 0x18aee5u:goto L18aee5;case 0x18aeeau:goto L18aeea;default:throw std::runtime_error("Required TU1.5 environment dispatch target unavailable");}
L18aed0: if(s.services.instruction)s.services.instruction(0x18aed0u); s.set(1,0xfu,4,0);
if(s.services.instruction)s.services.instruction(0x18aed5u); goto L18aeea;
L18aed7: if(s.services.instruction)s.services.instruction(0x18aed7u); s.set(1,0x10u,4,0);
if(s.services.instruction)s.services.instruction(0x18aedcu); goto L18aeea;
L18aede: if(s.services.instruction)s.services.instruction(0x18aedeu); s.set(1,0x13u,4,0);
if(s.services.instruction)s.services.instruction(0x18aee3u); goto L18aeea;
L18aee5: if(s.services.instruction)s.services.instruction(0x18aee5u); s.set(1,0xeu,4,0);
L18aeea: if(s.services.instruction)s.services.instruction(0x18aeeau); s.flags.minus(s.reg(1,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18aeedu); s.set(2,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18aeefu); if(!s.flags.zf)goto L18aea0;
if(s.services.instruction)s.services.instruction(0x18aef1u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18aef2u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18aef3u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18aef4u); s.pop();s.gp[4]+=0u;return;
L18aef5: if(s.services.instruction)s.services.instruction(0x18aef5u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18aef6u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18aef7u); s.set(0,s.reg(2,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18aef9u); s.set(3,s.pop(),4,0);
L18aefa: if(s.services.instruction)s.services.instruction(0x18aefau); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_18af20(MathState &s) {
if(s.services.instruction)s.services.instruction(0x18af20u); s.push(0x18af20u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18af21u); s.push(0x18af21u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x18af22u); s.set(7,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18af24u); s.set(6,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18af26u); s.push(0x18af26u,0x18af2bu);run_18ae80(s);
if(s.services.instruction)s.services.instruction(0x18af2bu); s.set(1,s.flags.logic((s.reg(1,4,0))^(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18af2du); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18af30u); if(s.flags.zf)goto L18af60;
if(s.services.instruction)s.services.instruction(0x18af32u); s.set(1,r(s.memory,(0xcu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18af36u); s.set(2,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18af3cu); s.set(6,s.flags.logic((s.reg(6,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18af42u); s.push(0x18af42u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x18af43u); s.set(6,s.flags.shift(s.reg(6,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18af46u); s.set(1,r(s.memory,(0x8u+s.gp[6]+s.gp[2]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18af4au); s.set(2,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x18af4du); s.set(0,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18af50u); s.set(1,(0x0u+s.gp[0]+s.gp[2]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x18af53u); s.push(0x18af53u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x18af54u); s.set(2,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18af56u); s.push(0x18af56u,0x18af5bu);run_18add0(s);
if(s.services.instruction)s.services.instruction(0x18af5bu); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18af5cu); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18af5du); s.pop();s.gp[4]+=4u;return;
L18af60: if(s.services.instruction)s.services.instruction(0x18af60u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18af61u); s.set(0,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18af63u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18af64u); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_18b0c0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x18b0c0u); s.set(1,r(s.memory,(0x4u+s.gp[4]),2),2,0);
if(s.services.instruction)s.services.instruction(0x18b0c5u); s.push(0x18b0c5u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x18b0c6u); s.push(0x18b0c6u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x18b0c7u); s.set(5,r(s.memory,(0x14u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b0cbu); s.push(0x18b0cbu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18b0ccu); s.push(0x18b0ccu,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x18b0cdu); s.set(7,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18b0cfu); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
L18b0d1: if(s.services.instruction)s.services.instruction(0x18b0d1u); s.flags.minus(s.reg(1,2,0),0xffffffffu,2);
if(s.services.instruction)s.services.instruction(0x18b0d5u); if(s.flags.zf)goto L18b150;
if(s.services.instruction)s.services.instruction(0x18b0d7u); s.flags.logic((s.reg(1,2,0))&(s.reg(1,2,0)),2);
if(s.services.instruction)s.services.instruction(0x18b0dau); if(s.flags.sf!=s.flags.of)goto L18b150;
if(s.services.instruction)s.services.instruction(0x18b0dcu); s.set(2,r(s.memory,(0x4ecc1cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b0e2u); s.set(6,r(s.memory,(0x150u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b0e8u); s.set(1,sign_extend(s.reg(1,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x18b0ebu); s.flags.minus(s.reg(1,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x18b0edu); if(s.flags.sf==s.flags.of)goto L18b150;
if(s.services.instruction)s.services.instruction(0x18b0efu); s.set(2,r(s.memory,(0x154u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b0f5u); s.set(1,s.flags.product(s.reg(1,4,0),0xb4u),4,0);
if(s.services.instruction)s.services.instruction(0x18b0fbu); s.set(3,(0x0u+s.gp[1]+s.gp[2]*1u),4,0);
if(s.services.instruction)s.services.instruction(0x18b0feu); s.flags.logic((s.reg(3,4,0))&(s.reg(3,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b100u); if(s.flags.zf)goto L18b150;
if(s.services.instruction)s.services.instruction(0x18b102u); s.set(6,r(s.memory,(0xb0u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b108u); s.flags.minus(s.reg(6,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18b10bu); if(s.flags.zf)goto L18b148;
if(s.services.instruction)s.services.instruction(0x18b10du); s.set(2,r(s.memory,(0x18u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b111u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18b113u); s.push(0x18b113u,0x18b118u);run_18ae80(s);
if(s.services.instruction)s.services.instruction(0x18b118u); s.set(1,s.flags.logic((s.reg(1,4,0))^(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b11au); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18b11du); if(s.flags.zf)goto L18b146;
if(s.services.instruction)s.services.instruction(0x18b11fu); s.set(1,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b125u); s.set(6,s.flags.logic((s.reg(6,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b12bu); s.set(6,s.flags.shift(s.reg(6,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x18b12eu); s.set(2,r(s.memory,(0x8u+s.gp[6]+s.gp[1]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b132u); s.set(1,r(s.memory,(0x4u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b135u); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x18b138u); s.set(2,(0x0u+s.gp[1]+s.gp[0]*8u),4,0);
if(s.services.instruction)s.services.instruction(0x18b13bu); s.push(0x18b13bu,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x18b13cu); s.push(0x18b13cu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18b13du); s.set(2,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18b13fu); s.push(0x18b13fu,0x18b144u);run_18add0(s);
if(s.services.instruction)s.services.instruction(0x18b144u); s.set(1,s.reg(0,4,0),4,0);
L18b146: if(s.services.instruction)s.services.instruction(0x18b146u); s.set(0,s.reg(1,4,0),4,0);
L18b148: if(s.services.instruction)s.services.instruction(0x18b148u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b14au); s.set(1,r(s.memory,(0x8u+s.gp[3]),2),2,0);
if(s.services.instruction)s.services.instruction(0x18b14eu); if(s.flags.zf)goto L18b0d1;
L18b150: if(s.services.instruction)s.services.instruction(0x18b150u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18b151u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18b152u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18b153u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18b154u); s.pop();s.gp[4]+=12u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_18b160(MathState &s) {
if(s.services.instruction)s.services.instruction(0x18b160u); s.flags.minus(s.reg(0,4,0),0x14u,4);
if(s.services.instruction)s.services.instruction(0x18b163u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a110u),4)));
if(s.services.instruction)s.services.instruction(0x18b16bu); if(!s.flags.cf&&!s.flags.zf)goto L18b19e;
if(s.services.instruction)s.services.instruction(0x18b16du); s.set(0,r(s.memory,(0x18b1b4u+s.gp[0]),1),4,0);
if(s.services.instruction)s.services.instruction(0x18b174u); switch(r(s.memory,(0x18b1a0u+s.gp[0]*4u),4)){case 0x18b17bu:goto L18b17b;case 0x18b184u:goto L18b184;case 0x18b18du:goto L18b18d;case 0x18b196u:goto L18b196;case 0x18b19eu:goto L18b19e;default:throw std::runtime_error("Required TU1.5 environment dispatch target unavailable");}
L18b17b: if(s.services.instruction)s.services.instruction(0x18b17bu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a1a8u),4)));
if(s.services.instruction)s.services.instruction(0x18b183u); s.pop();s.gp[4]+=0u;return;
L18b184: if(s.services.instruction)s.services.instruction(0x18b184u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a108u),4)));
if(s.services.instruction)s.services.instruction(0x18b18cu); s.pop();s.gp[4]+=0u;return;
L18b18d: if(s.services.instruction)s.services.instruction(0x18b18du); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a1a4u),4)));
if(s.services.instruction)s.services.instruction(0x18b195u); s.pop();s.gp[4]+=0u;return;
L18b196: if(s.services.instruction)s.services.instruction(0x18b196u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a118u),4)));
L18b19e: if(s.services.instruction)s.services.instruction(0x18b19eu); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_18b1d0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x18b1d0u); s.set(4,s.flags.minus(s.reg(4,4,0),0x1cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x18b1d3u); s.push(0x18b1d3u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x18b1d4u); s.push(0x18b1d4u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18b1d5u); s.set(6,r(s.memory,(0x34u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b1d9u); s.set(5,s.flags.logic((s.reg(5,4,0))^(s.reg(5,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b1dbu); s.set(2,s.flags.logic((s.reg(2,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b1deu); s.flags.minus(s.reg(6,2,0),s.reg(2,2,0),2);
if(s.services.instruction)s.services.instruction(0x18b1e1u); s.push(0x18b1e1u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x18b1e2u); w(s.memory,s.observe,0x18b1e2u,(0xcu+s.gp[4]),4,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x18b1e6u); if(s.flags.zf)goto L18b212;
if(s.services.instruction)s.services.instruction(0x18b1e8u); s.flags.minus(s.reg(6,2,0),s.reg(5,2,0),2);
if(s.services.instruction)s.services.instruction(0x18b1ebu); if(s.flags.sf!=s.flags.of)goto L18b212;
if(s.services.instruction)s.services.instruction(0x18b1edu); s.set(1,r(s.memory,(0x4ecc1cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b1f3u); s.set(7,r(s.memory,(0x150u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b1f9u); s.set(0,sign_extend(s.reg(6,2,0),2),4,0);
if(s.services.instruction)s.services.instruction(0x18b1fcu); s.flags.minus(s.reg(0,4,0),s.reg(7,4,0),4);
if(s.services.instruction)s.services.instruction(0x18b1feu); if(s.flags.sf==s.flags.of)goto L18b212;
if(s.services.instruction)s.services.instruction(0x18b200u); s.set(7,r(s.memory,(0x154u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b206u); s.set(0,s.flags.product(s.reg(0,4,0),0xb4u),4,0);
if(s.services.instruction)s.services.instruction(0x18b20cu); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(7,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b20eu); w(s.memory,s.observe,0x18b20eu,(0xcu+s.gp[4]),4,s.reg(0,4,0));
L18b212: if(s.services.instruction)s.services.instruction(0x18b212u); s.set(0,r(s.memory,(0x3cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b216u); s.flags.minus(s.reg(0,4,0),0xdu,4);
if(s.services.instruction)s.services.instruction(0x18b219u); w(s.memory,s.observe,0x18b219u,(0x10u+s.gp[4]),4,0xeu);
if(s.services.instruction)s.services.instruction(0x18b221u); if(s.flags.zf)goto L18b227;
if(s.services.instruction)s.services.instruction(0x18b223u); w(s.memory,s.observe,0x18b223u,(0x10u+s.gp[4]),4,s.reg(0,4,0));
L18b227: if(s.services.instruction)s.services.instruction(0x18b227u); s.set(1,r(s.memory,(0x2cu+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x18b22bu); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x18b22du); w(s.memory,s.observe,0x18b22du,(0x18u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18b231u); w(s.memory,s.observe,0x18b231u,(0x1cu+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18b235u); w(s.memory,s.observe,0x18b235u,(0x14u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18b239u); w(s.memory,s.observe,0x18b239u,(0x38u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18b23du); w(s.memory,s.observe,0x18b23du,(0x20u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18b241u); w(s.memory,s.observe,0x18b241u,(0x24u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18b245u); if(!s.flags.zf)goto L18b25f;
if(s.services.instruction)s.services.instruction(0x18b247u); s.push(0x18b247u,0x18b24cu);run_18b160(s);
if(s.services.instruction)s.services.instruction(0x18b24cu); s.set(7,r(s.memory,(0x30u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b250u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b252u); s.push(0x18b252u,0x18b257u);run_1755b0(s);
if(s.services.instruction)s.services.instruction(0x18b257u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x18b259u); if(s.flags.zf)goto L18b375;
L18b25f: if(s.services.instruction)s.services.instruction(0x18b25fu); s.flags.minus(s.reg(3,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18b262u); if(s.flags.zf)goto L18b28b;
if(s.services.instruction)s.services.instruction(0x18b264u); s.set(2,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b268u); s.push(0x18b268u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18b269u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b26bu); s.set(1,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18b26du); s.push(0x18b26du,0x18b272u);run_18af20(s);
if(s.services.instruction)s.services.instruction(0x18b272u); s.set(2,r(s.memory,(0x3cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b276u); s.set(7,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18b278u); s.push(0x18b278u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18b279u); s.set(0,0x1u,4,0);
if(s.services.instruction)s.services.instruction(0x18b27eu); s.set(1,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18b280u); s.push(0x18b280u,0x18b285u);run_18af20(s);
if(s.services.instruction)s.services.instruction(0x18b285u); s.flags.logic((s.reg(7,4,0))&(s.reg(7,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b287u); s.set(5,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18b289u); if(!s.flags.zf)goto L18b29f;
L18b28b: if(s.services.instruction)s.services.instruction(0x18b28bu); s.set(0,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b28fu); s.set(1,r(s.memory,(0x34u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b293u); s.push(0x18b293u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18b294u); s.push(0x18b294u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x18b295u); s.push(0x18b295u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x18b296u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b298u); s.push(0x18b298u,0x18b29du);run_18b0c0(s);
if(s.services.instruction)s.services.instruction(0x18b29du); s.set(7,s.reg(0,4,0),4,0);
L18b29f: if(s.services.instruction)s.services.instruction(0x18b29fu); s.flags.logic((s.reg(5,4,0))&(s.reg(5,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b2a1u); if(!s.flags.zf)goto L18b2bc;
if(s.services.instruction)s.services.instruction(0x18b2a3u); s.set(2,r(s.memory,(0x34u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b2a7u); s.push(0x18b2a7u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18b2a8u); s.set(6,r(s.memory,(0x40u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b2acu); s.push(0x18b2acu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x18b2adu); s.push(0x18b2adu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18b2aeu); s.set(0,0x1u,4,0);
if(s.services.instruction)s.services.instruction(0x18b2b3u); s.push(0x18b2b3u,0x18b2b8u);run_18b0c0(s);
if(s.services.instruction)s.services.instruction(0x18b2b8u); s.set(5,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18b2bau); goto L18b2c0;
L18b2bc: if(s.services.instruction)s.services.instruction(0x18b2bcu); s.set(6,r(s.memory,(0x3cu+s.gp[4]),4),4,0);
L18b2c0: if(s.services.instruction)s.services.instruction(0x18b2c0u); s.flags.logic((s.reg(7,4,0))&(s.reg(7,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b2c2u); if(s.flags.zf)goto L18b314;
if(s.services.instruction)s.services.instruction(0x18b2c4u); s.flags.minus(r(s.memory,(0x16u+s.gp[7]),1),0x1u,1);
if(s.services.instruction)s.services.instruction(0x18b2c8u); s.set(0,r(s.memory,(0x40u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b2ccu); if(!s.flags.zf)goto L18b304;
if(s.services.instruction)s.services.instruction(0x18b2ceu); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18b2d1u); if(s.flags.zf)goto L18b304;
if(s.services.instruction)s.services.instruction(0x18b2d3u); s.flags.minus(s.reg(6,4,0),0xdu,4);
if(s.services.instruction)s.services.instruction(0x18b2d6u); if(s.flags.sf!=s.flags.of)goto L18b2f8;
if(s.services.instruction)s.services.instruction(0x18b2d8u); s.flags.minus(s.reg(6,4,0),0xeu,4);
if(s.services.instruction)s.services.instruction(0x18b2dbu); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L18b2f8;
if(s.services.instruction)s.services.instruction(0x18b2ddu); s.set(1,r(s.memory,(0xcu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b2e1u); s.set(2,r(s.memory,(0x54u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b2e4u); s.set(1,r(s.memory,(0x4u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b2e7u); w(s.memory,s.observe,0x18b2e7u,(0x14u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18b2ebu); s.set(2,r(s.memory,(0xcu+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b2eeu); w(s.memory,s.observe,0x18b2eeu,(0x18u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x18b2f2u); w(s.memory,s.observe,0x18b2f2u,(0x20u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18b2f6u); goto L18b318;
L18b2f8: if(s.services.instruction)s.services.instruction(0x18b2f8u); s.set(1,r(s.memory,(0xcu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b2fcu); s.set(2,r(s.memory,(0x3cu+s.gp[1]+s.gp[0]*8u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b300u); w(s.memory,s.observe,0x18b300u,(0x14u+s.gp[4]),4,s.reg(2,4,0));
L18b304: if(s.services.instruction)s.services.instruction(0x18b304u); s.set(1,r(s.memory,(0x4u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b307u); s.set(2,r(s.memory,(0xcu+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b30au); w(s.memory,s.observe,0x18b30au,(0x18u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x18b30eu); w(s.memory,s.observe,0x18b30eu,(0x20u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x18b312u); goto L18b318;
L18b314: if(s.services.instruction)s.services.instruction(0x18b314u); s.set(0,r(s.memory,(0x40u+s.gp[4]),4),4,0);
L18b318: if(s.services.instruction)s.services.instruction(0x18b318u); s.flags.logic((s.reg(5,4,0))&(s.reg(5,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b31au); if(s.flags.zf)goto L18b375;
if(s.services.instruction)s.services.instruction(0x18b31cu); s.flags.minus(r(s.memory,(0x16u+s.gp[5]),1),0x1u,1);
if(s.services.instruction)s.services.instruction(0x18b320u); if(!s.flags.zf)goto L18b367;
if(s.services.instruction)s.services.instruction(0x18b322u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x18b325u); if(s.flags.zf)goto L18b367;
if(s.services.instruction)s.services.instruction(0x18b327u); s.flags.minus(s.reg(6,4,0),0xdu,4);
if(s.services.instruction)s.services.instruction(0x18b32au); if(s.flags.sf!=s.flags.of)goto L18b35b;
if(s.services.instruction)s.services.instruction(0x18b32cu); s.flags.minus(s.reg(6,4,0),0xeu,4);
if(s.services.instruction)s.services.instruction(0x18b32fu); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L18b35b;
if(s.services.instruction)s.services.instruction(0x18b331u); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x18b333u); s.set(0,s.flags.minus(s.reg(0,4,0),0xdu,4),4,0);
if(s.services.instruction)s.services.instruction(0x18b336u); if(s.flags.zf)goto L18b34b;
if(s.services.instruction)s.services.instruction(0x18b338u); s.set(0,s.flags.increment(s.reg(0,4,0),4,true),4,0);
if(s.services.instruction)s.services.instruction(0x18b339u); if(!s.flags.zf)goto L18b367;
if(s.services.instruction)s.services.instruction(0x18b33bu); s.set(0,r(s.memory,(0xcu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b33fu); s.set(1,r(s.memory,(0x8cu+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b345u); w(s.memory,s.observe,0x18b345u,(0x38u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x18b349u); goto L18b367;
L18b34b: if(s.services.instruction)s.services.instruction(0x18b34bu); s.set(2,r(s.memory,(0xcu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b34fu); s.set(0,r(s.memory,(0x94u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b355u); w(s.memory,s.observe,0x18b355u,(0x38u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x18b359u); goto L18b367;
L18b35b: if(s.services.instruction)s.services.instruction(0x18b35bu); s.set(1,r(s.memory,(0xcu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b35fu); s.set(2,r(s.memory,(0x74u+s.gp[1]+s.gp[0]*8u),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b363u); w(s.memory,s.observe,0x18b363u,(0x38u+s.gp[4]),4,s.reg(2,4,0));
L18b367: if(s.services.instruction)s.services.instruction(0x18b367u); s.set(0,r(s.memory,(0x4u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b36au); s.set(1,r(s.memory,(0xcu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b36du); w(s.memory,s.observe,0x18b36du,(0x1cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x18b371u); w(s.memory,s.observe,0x18b371u,(0x24u+s.gp[4]),4,s.reg(1,4,0));
L18b375: if(s.services.instruction)s.services.instruction(0x18b375u); s.set(0,r(s.memory,(0x44u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b379u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b37bu); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18b37cu); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18b37du); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x18b37eu); if(s.flags.zf)goto L18b386;
if(s.services.instruction)s.services.instruction(0x18b380u); s.set(2,r(s.memory,(0xcu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b384u); w(s.memory,s.observe,0x18b384u,(0x0u+s.gp[0]),4,s.reg(2,4,0));
L18b386: if(s.services.instruction)s.services.instruction(0x18b386u); s.set(0,r(s.memory,(0x3cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b38au); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b38cu); if(s.flags.zf)goto L18b394;
if(s.services.instruction)s.services.instruction(0x18b38eu); s.set(1,r(s.memory,(0x10u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b392u); w(s.memory,s.observe,0x18b392u,(0x0u+s.gp[0]),4,s.reg(1,4,0));
L18b394: if(s.services.instruction)s.services.instruction(0x18b394u); s.set(0,r(s.memory,(0x40u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b398u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b39au); if(s.flags.zf)goto L18b3a2;
if(s.services.instruction)s.services.instruction(0x18b39cu); s.set(2,r(s.memory,(0x8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b3a0u); w(s.memory,s.observe,0x18b3a0u,(0x0u+s.gp[0]),4,s.reg(2,4,0));
L18b3a2: if(s.services.instruction)s.services.instruction(0x18b3a2u); s.set(0,r(s.memory,(0x44u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b3a6u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b3a8u); if(s.flags.zf)goto L18b3b0;
if(s.services.instruction)s.services.instruction(0x18b3aau); s.set(1,r(s.memory,(0x2cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b3aeu); w(s.memory,s.observe,0x18b3aeu,(0x0u+s.gp[0]),4,s.reg(1,4,0));
L18b3b0: if(s.services.instruction)s.services.instruction(0x18b3b0u); s.set(0,r(s.memory,(0x48u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b3b4u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b3b6u); if(s.flags.zf)goto L18b3be;
if(s.services.instruction)s.services.instruction(0x18b3b8u); s.set(2,r(s.memory,(0x14u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b3bcu); w(s.memory,s.observe,0x18b3bcu,(0x0u+s.gp[0]),4,s.reg(2,4,0));
L18b3be: if(s.services.instruction)s.services.instruction(0x18b3beu); s.set(0,r(s.memory,(0x4cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b3c2u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x18b3c4u); if(s.flags.zf)goto L18b3cc;
if(s.services.instruction)s.services.instruction(0x18b3c6u); s.set(1,r(s.memory,(0x18u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x18b3cau); w(s.memory,s.observe,0x18b3cau,(0x0u+s.gp[0]),4,s.reg(1,4,0));
L18b3cc: if(s.services.instruction)s.services.instruction(0x18b3ccu); s.set(4,s.flags.plus(s.reg(4,4,0),0x1cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x18b3cfu); s.pop();s.gp[4]+=48u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_190420(MathState &s) {
if(s.services.instruction)s.services.instruction(0x190420u); s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x190423u); s.flags.minus(s.reg(1,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x190426u); if(s.flags.zf)goto L190458;
if(s.services.instruction)s.services.instruction(0x190428u); s.set(2,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x19042eu); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x190434u); s.set(1,s.flags.shift(s.reg(1,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x190437u); s.set(1,r(s.memory,(0x8u+s.gp[1]+s.gp[2]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x19043bu); s.push(0x19043bu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x19043cu); s.set(6,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x19043eu); s.set(2,s.flags.logic((s.reg(2,4,0))^(s.reg(2,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x190440u); s.flags.logic((s.reg(6,4,0))&(s.reg(6,4,0)),4);
if(s.services.instruction)s.services.instruction(0x190442u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L190457;
if(s.services.instruction)s.services.instruction(0x190444u); s.set(1,r(s.memory,(0x4u+s.gp[1]),4),4,0);
L190447: if(s.services.instruction)s.services.instruction(0x190447u); s.flags.minus(r(s.memory,(0x0u+s.gp[1]),4),s.reg(7,4,0),4);
if(s.services.instruction)s.services.instruction(0x190449u); if(s.flags.zf)goto L190455;
if(s.services.instruction)s.services.instruction(0x19044bu); s.set(2,s.flags.increment(s.reg(2,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x19044cu); s.set(1,s.flags.plus(s.reg(1,4,0),0x38u,4),4,0);
if(s.services.instruction)s.services.instruction(0x19044fu); s.flags.minus(s.reg(2,4,0),s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x190451u); if(s.flags.sf!=s.flags.of)goto L190447;
if(s.services.instruction)s.services.instruction(0x190453u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x190454u); s.pop();s.gp[4]+=0u;return;
L190455: if(s.services.instruction)s.services.instruction(0x190455u); s.set(0,s.reg(2,4,0),4,0);
L190457: if(s.services.instruction)s.services.instruction(0x190457u); s.set(6,s.pop(),4,0);
L190458: if(s.services.instruction)s.services.instruction(0x190458u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_228220(MathState &s) {
if(s.services.instruction)s.services.instruction(0x228220u); s.set(0,r(s.memory,(0x4ecc1cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x228225u); s.set(1,r(s.memory,(0xc0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22822bu); s.flags.logic((s.reg(1,4,0))&(s.reg(1,4,0)),4);
if(s.services.instruction)s.services.instruction(0x22822du); if(s.flags.zf)goto L228237;
if(s.services.instruction)s.services.instruction(0x22822fu); s.set(0,r(s.memory,(0xc4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x228235u); goto L228239;
L228237: if(s.services.instruction)s.services.instruction(0x228237u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
L228239: if(s.services.instruction)s.services.instruction(0x228239u); s.set(0,r(s.memory,(0x4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x22823cu); s.set(1,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x228242u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x228247u); s.set(0,s.flags.shift(s.reg(0,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x22824au); s.set(2,r(s.memory,(0x8u+s.gp[0]+s.gp[1]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x22824eu); s.set(0,sign_extend(r(s.memory,(0x4u+s.gp[4]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x228253u); s.set(1,r(s.memory,(0x4u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x228256u); s.set(0,s.flags.product(s.reg(0,4,0),0x5cu),4,0);
if(s.services.instruction)s.services.instruction(0x228259u); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(1,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x22825bu); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_2e44d0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x2e44d0u); s.push(0x2e44d0u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x2e44d1u); s.set(5,s.reg(4,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x2e44d3u); s.set(4,s.flags.logic((s.reg(4,4,0))&(0xfffffff0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x2e44d6u); s.set(4,s.flags.minus(s.reg(4,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0x2e44d9u); s.set(0,r(s.memory,(0xcu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2e44dcu); s.set(2,r(s.memory,(0x8u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2e44dfu); s.push(0x2e44dfu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x2e44e0u); s.set(6,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x2e44e2u); s.push(0x2e44e2u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x2e44e3u); s.set(6,s.flags.minus(s.reg(6,4,0),s.reg(0,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x2e44e5u); s.set(7,0x4u,4,0);
if(s.services.instruction)s.services.instruction(0x2e44eau); s.set(3,(0x0u+s.gp[3]),4,0);
L2e44f0: if(s.services.instruction)s.services.instruction(0x2e44f0u); s.xmm[0]=v(s.memory,(0x0u+s.gp[0]));
if(s.services.instruction)s.services.instruction(0x2e44f3u); s.xmm[2]=v(s.memory,(0x20u+s.gp[2]));
if(s.services.instruction)s.services.instruction(0x2e44f7u); s.xmm[3]=v(s.memory,(0x10u+s.gp[2]));
if(s.services.instruction)s.services.instruction(0x2e44fbu); s.xmm[1]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x2e44feu); s.xmm[1]=_mm_shuffle_ps(s.xmm[1],s.xmm[0],170);
if(s.services.instruction)s.services.instruction(0x2e4502u); s.xmm[2]=_mm_mul_ps(s.xmm[2],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x2e4505u); s.xmm[1]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x2e4508u); s.xmm[1]=_mm_shuffle_ps(s.xmm[1],s.xmm[0],85);
if(s.services.instruction)s.services.instruction(0x2e450cu); s.xmm[3]=_mm_mul_ps(s.xmm[3],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x2e450fu); s.xmm[1]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x2e4512u); s.xmm[1]=_mm_shuffle_ps(s.xmm[1],s.xmm[0],0);
if(s.services.instruction)s.services.instruction(0x2e4516u); s.xmm[0]=v(s.memory,(0x0u+s.gp[2]));
if(s.services.instruction)s.services.instruction(0x2e4519u); s.xmm[0]=_mm_mul_ps(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x2e451cu); s.xmm[0]=_mm_add_ps(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x2e451fu); s.xmm[0]=_mm_add_ps(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x2e4522u); vw(s.memory,s.observe,0x2e4522u,(0x0u+s.gp[6]+s.gp[0]*1u),s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x2e4526u); s.set(0,s.flags.plus(s.reg(0,4,0),0x10u,4),4,0);
if(s.services.instruction)s.services.instruction(0x2e4529u); s.set(7,s.flags.increment(s.reg(7,4,0),4,true),4,0);
if(s.services.instruction)s.services.instruction(0x2e452au); if(!s.flags.zf)goto L2e44f0;
if(s.services.instruction)s.services.instruction(0x2e452cu); s.xmm[0]=v(s.memory,(0x30u+s.gp[2]));
if(s.services.instruction)s.services.instruction(0x2e4530u); s.xmm[1]=v(s.memory,(0x30u+s.gp[1]));
if(s.services.instruction)s.services.instruction(0x2e4534u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x2e4535u); s.xmm[1]=_mm_add_ps(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x2e4538u); vw(s.memory,s.observe,0x2e4538u,(0x30u+s.gp[1]),s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x2e453cu); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x2e453du); s.set(4,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x2e453fu); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x2e4540u); s.pop();s.gp[4]+=8u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}

void run_2e4650(MathState &s) {
if(s.services.instruction)s.services.instruction(0x2e4650u); s.push(0x2e4650u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x2e4651u); s.set(5,s.reg(4,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x2e4653u); s.set(4,s.flags.logic((s.reg(4,4,0))&(0xfffffff0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x2e4656u); s.set(4,s.flags.minus(s.reg(4,4,0),0x40u,4),4,0);
if(s.services.instruction)s.services.instruction(0x2e4659u); s.xmm[0]=v(s.memory,(0x0u+s.gp[1]));
if(s.services.instruction)s.services.instruction(0x2e465cu); s.set(0,r(s.memory,(0x8u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2e465fu); vw(s.memory,s.observe,0x2e465fu,(0x0u+s.gp[4]),s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x2e4663u); s.xmm[0]=v(s.memory,(0x10u+s.gp[1]));
if(s.services.instruction)s.services.instruction(0x2e4667u); vw(s.memory,s.observe,0x2e4667u,(0x10u+s.gp[4]),s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x2e466cu); s.xmm[0]=v(s.memory,(0x20u+s.gp[1]));
if(s.services.instruction)s.services.instruction(0x2e4670u); s.push(0x2e4670u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2e4671u); s.set(2,(0x4u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x2e4675u); vw(s.memory,s.observe,0x2e4675u,(0x24u+s.gp[4]),s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x2e467au); s.xmm[0]=v(s.memory,(0x30u+s.gp[1]));
if(s.services.instruction)s.services.instruction(0x2e467eu); s.push(0x2e467eu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x2e467fu); vw(s.memory,s.observe,0x2e467fu,(0x38u+s.gp[4]),s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x2e4684u); s.push(0x2e4684u,0x2e4689u);run_2e44d0(s);
if(s.services.instruction)s.services.instruction(0x2e4689u); s.set(4,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x2e468bu); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x2e468cu); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 contact state fell outside authenticated CFG");
}
} // namespace
void source_run_physics_vehicle_environment_state_runtime(M &m,std::uint32_t entry,
 SourcePhysicsContactRuntimeAbi &abi,FP &fp,const SourcePhysicsContactRuntimeServices &services,const O &observe) {
 auto bound=services;
 bound.required_owner=[&](M &memory,const SourcePhysicsContactRuntimeCall &call,
  SourcePhysicsContactRuntimeAbi &state,const O &observer) {
if(call.target==0xbcdf0u){source_run_physics_vehicle_environment_state_runtime(memory,0xbcdf0u,state,fp,services,observer);return;}
if(call.target==0xc6120u){source_run_physics_vehicle_environment_state_runtime(memory,0xc6120u,state,fp,services,observer);return;}
if(call.target==0x11e600u){source_run_physics_vehicle_environment_state_runtime(memory,0x11e600u,state,fp,services,observer);return;}
if(call.target==0x11e850u){source_run_physics_vehicle_environment_state_runtime(memory,0x11e850u,state,fp,services,observer);return;}
if(call.target==0x14cdb0u){source_run_physics_vehicle_environment_state_runtime(memory,0x14cdb0u,state,fp,services,observer);return;}
if(call.target==0x16e8a0u){source_run_physics_vehicle_environment_state_runtime(memory,0x16e8a0u,state,fp,services,observer);return;}
if(call.target==0x16ea60u){source_run_physics_vehicle_environment_state_runtime(memory,0x16ea60u,state,fp,services,observer);return;}
if(call.target==0x1755b0u){source_run_physics_vehicle_environment_state_runtime(memory,0x1755b0u,state,fp,services,observer);return;}
if(call.target==0x177020u){source_run_physics_vehicle_environment_state_runtime(memory,0x177020u,state,fp,services,observer);return;}
if(call.target==0x177fa0u){source_run_physics_vehicle_environment_state_runtime(memory,0x177fa0u,state,fp,services,observer);return;}
if(call.target==0x17a030u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a030u,state,fp,services,observer);return;}
if(call.target==0x17a230u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a230u,state,fp,services,observer);return;}
if(call.target==0x17a2f0u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a2f0u,state,fp,services,observer);return;}
if(call.target==0x17a380u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a380u,state,fp,services,observer);return;}
if(call.target==0x17a420u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a420u,state,fp,services,observer);return;}
if(call.target==0x17a4a0u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a4a0u,state,fp,services,observer);return;}
if(call.target==0x18add0u){source_run_physics_vehicle_environment_state_runtime(memory,0x18add0u,state,fp,services,observer);return;}
if(call.target==0x18ae80u){source_run_physics_vehicle_environment_state_runtime(memory,0x18ae80u,state,fp,services,observer);return;}
if(call.target==0x18af20u){source_run_physics_vehicle_environment_state_runtime(memory,0x18af20u,state,fp,services,observer);return;}
if(call.target==0x18b0c0u){source_run_physics_vehicle_environment_state_runtime(memory,0x18b0c0u,state,fp,services,observer);return;}
if(call.target==0x18b160u){source_run_physics_vehicle_environment_state_runtime(memory,0x18b160u,state,fp,services,observer);return;}
if(call.target==0x18b1d0u){source_run_physics_vehicle_environment_state_runtime(memory,0x18b1d0u,state,fp,services,observer);return;}
if(call.target==0x190420u){source_run_physics_vehicle_environment_state_runtime(memory,0x190420u,state,fp,services,observer);return;}
if(call.target==0x228220u){source_run_physics_vehicle_environment_state_runtime(memory,0x228220u,state,fp,services,observer);return;}
if(call.target==0x2e44d0u){source_run_physics_vehicle_environment_state_runtime(memory,0x2e44d0u,state,fp,services,observer);return;}
if(call.target==0x2e4650u){source_run_physics_vehicle_environment_state_runtime(memory,0x2e4650u,state,fp,services,observer);return;}
if(call.target==0x2b3e0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x2b3e0u,state,fp,bound,observer);return;}
if(call.target==0x30bd0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x30bd0u,state,fp,bound,observer);return;}
if(call.target==0xb17d0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xa8380u,state,fp,bound,observer);return;}
if(call.target==0xb9390u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xb8250u,state,fp,bound,observer);return;}
if(call.target==0xb9770u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xb8630u,state,fp,bound,observer);return;}
if(call.target==0xb9800u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xb86c0u,state,fp,bound,observer);return;}
if(call.target==0xbacd0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xb9b90u,state,fp,bound,observer);return;}
if(call.target==0xbad60u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xb9c20u,state,fp,bound,observer);return;}
if(call.target==0xbbbc0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xbaa80u,state,fp,bound,observer);return;}
if(call.target==0xbc190u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xbb050u,state,fp,bound,observer);return;}
if(call.target==0xbc200u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xbb0c0u,state,fp,bound,observer);return;}
if(call.target==0xbcfb0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xbbe70u,state,fp,bound,observer);return;}
if(call.target==0xbda50u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xbc910u,state,fp,bound,observer);return;}
if(call.target==0xbe7e0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xbd6a0u,state,fp,bound,observer);return;}
if(call.target==0xd3750u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xd26c0u,state,fp,bound,observer);return;}
if(call.target==0xde8c0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xde8c0u,state,fp,bound,observer);return;}
if(call.target==0xe8d50u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xe7e30u,state,fp,bound,observer);return;}
if(call.target==0xe8d80u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xe7e60u,state,fp,bound,observer);return;}
if(call.target==0xf11f0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf02d0u,state,fp,bound,observer);return;}
if(call.target==0xf15d0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf06b0u,state,fp,bound,observer);return;}
if(call.target==0xf1a50u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf0b30u,state,fp,bound,observer);return;}
if(call.target==0xf1dd0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf0eb0u,state,fp,bound,observer);return;}
if(call.target==0xf20c0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf11a0u,state,fp,bound,observer);return;}
if(call.target==0xf2190u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf1270u,state,fp,bound,observer);return;}
if(call.target==0xf2340u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf1420u,state,fp,bound,observer);return;}
if(call.target==0xf2c20u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf1d00u,state,fp,bound,observer);return;}
if(call.target==0xf3880u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf2960u,state,fp,bound,observer);return;}
if(call.target==0xf38e0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf29c0u,state,fp,bound,observer);return;}
if(call.target==0xf3a50u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf2b30u,state,fp,bound,observer);return;}
if(call.target==0xf3aa0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf2b80u,state,fp,bound,observer);return;}
if(call.target==0xf3b30u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf2c10u,state,fp,bound,observer);return;}
if(call.target==0xf3ca0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf2d80u,state,fp,bound,observer);return;}
if(call.target==0xf3eb0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf2f90u,state,fp,bound,observer);return;}
if(call.target==0xf41e0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf32c0u,state,fp,bound,observer);return;}
if(call.target==0xf4230u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf3310u,state,fp,bound,observer);return;}
if(call.target==0xf44e0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf35c0u,state,fp,bound,observer);return;}
if(call.target==0xf48c0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf39a0u,state,fp,bound,observer);return;}
if(call.target==0xf5570u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf4650u,state,fp,bound,observer);return;}
if(call.target==0xf67b0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf5890u,state,fp,bound,observer);return;}
if(call.target==0xf6850u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf5930u,state,fp,bound,observer);return;}
if(call.target==0xf6a00u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf5ae0u,state,fp,bound,observer);return;}
if(call.target==0xf6d60u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf5e40u,state,fp,bound,observer);return;}
if(call.target==0xf8150u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf7230u,state,fp,bound,observer);return;}
if(call.target==0xf82d0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf73b0u,state,fp,bound,observer);return;}
if(call.target==0xf8360u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf7440u,state,fp,bound,observer);return;}
if(call.target==0xf8440u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf7520u,state,fp,bound,observer);return;}
if(call.target==0xf8570u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf7650u,state,fp,bound,observer);return;}
if(call.target==0xf9130u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0xf8210u,state,fp,bound,observer);return;}
if(call.target==0x111500u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1105e0u,state,fp,bound,observer);return;}
if(call.target==0x116520u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x115600u,state,fp,bound,observer);return;}
if(call.target==0x120150u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x11eeb0u,state,fp,bound,observer);return;}
if(call.target==0x144090u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x142d00u,state,fp,bound,observer);return;}
if(call.target==0x144910u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x143580u,state,fp,bound,observer);return;}
if(call.target==0x144a60u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1436d0u,state,fp,bound,observer);return;}
if(call.target==0x144bf0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x143860u,state,fp,bound,observer);return;}
if(call.target==0x145560u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1441d0u,state,fp,bound,observer);return;}
if(call.target==0x145680u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1442f0u,state,fp,bound,observer);return;}
if(call.target==0x1459b0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x144620u,state,fp,bound,observer);return;}
if(call.target==0x16e6a0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x16e6a0u,state,fp,bound,observer);return;}
if(call.target==0x178e30u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x178e30u,state,fp,bound,observer);return;}
if(call.target==0x17a0f0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x17a0f0u,state,fp,bound,observer);return;}
if(call.target==0x17dec0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x17dec0u,state,fp,bound,observer);return;}
if(call.target==0x185070u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x183c50u,state,fp,bound,observer);return;}
if(call.target==0x18bbc0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x18bbc0u,state,fp,bound,observer);return;}
if(call.target==0x18c440u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x18c440u,state,fp,bound,observer);return;}
if(call.target==0x18cdd0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x18cdd0u,state,fp,bound,observer);return;}
if(call.target==0x18d4b0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x18d4b0u,state,fp,bound,observer);return;}
if(call.target==0x18d510u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x18d510u,state,fp,bound,observer);return;}
if(call.target==0x1903a0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1903a0u,state,fp,bound,observer);return;}
if(call.target==0x1cd6a0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1c8430u,state,fp,bound,observer);return;}
if(call.target==0x1d64c0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1d1c00u,state,fp,bound,observer);return;}
if(call.target==0x1d7d70u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1d34b0u,state,fp,bound,observer);return;}
if(call.target==0x1d7e40u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1d3580u,state,fp,bound,observer);return;}
if(call.target==0x1d8c80u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1d43c0u,state,fp,bound,observer);return;}
if(call.target==0x1d8d80u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1d44c0u,state,fp,bound,observer);return;}
if(call.target==0x1d8dd0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1d4510u,state,fp,bound,observer);return;}
if(call.target==0x1d8e60u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1d45a0u,state,fp,bound,observer);return;}
if(call.target==0x1d8b90u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1d8b90u,state,fp,bound,observer);return;}
if(call.target==0x1e11b0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1dc8f0u,state,fp,bound,observer);return;}
if(call.target==0x1e1200u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x1dc940u,state,fp,bound,observer);return;}
if(call.target==0x20d680u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x208d80u,state,fp,bound,observer);return;}
if(call.target==0x20d770u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x208e70u,state,fp,bound,observer);return;}
if(call.target==0x20d7d0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x208ed0u,state,fp,bound,observer);return;}
if(call.target==0x20d9a0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x2090a0u,state,fp,bound,observer);return;}
if(call.target==0x20dea0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x2095a0u,state,fp,bound,observer);return;}
if(call.target==0x20e350u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x209a50u,state,fp,bound,observer);return;}
if(call.target==0x20fcb0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x20b3b0u,state,fp,bound,observer);return;}
if(call.target==0x20fd70u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x20b470u,state,fp,bound,observer);return;}
if(call.target==0x22e1b0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22e1b0u,state,fp,bound,observer);return;}
if(call.target==0x22e210u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22e210u,state,fp,bound,observer);return;}
if(call.target==0x22e390u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22e390u,state,fp,bound,observer);return;}
if(call.target==0x22e4f0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22e4f0u,state,fp,bound,observer);return;}
if(call.target==0x22eec0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22eec0u,state,fp,bound,observer);return;}
if(call.target==0x22efd0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22efd0u,state,fp,bound,observer);return;}
if(call.target==0x22fe30u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22fe30u,state,fp,bound,observer);return;}
if(call.target==0x22fea0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x22fea0u,state,fp,bound,observer);return;}
if(call.target==0x230a60u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x230a60u,state,fp,bound,observer);return;}
if(call.target==0x230ae0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x230ae0u,state,fp,bound,observer);return;}
if(call.target==0x3150e0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x3115f0u,state,fp,bound,observer);return;}
if(call.target==0x3248b0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x31f9e0u,state,fp,bound,observer);return;}
if(call.target==0x324be0u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x321070u,state,fp,bound,observer);return;}
if(call.target==0x324c40u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x3210d0u,state,fp,bound,observer);return;}
if(call.target==0x32a660u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x326af0u,state,fp,bound,observer);return;}
if(call.target==0x32b26cu){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x3276fcu,state,fp,bound,observer);return;}
if(call.target==0x32b2feu){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x32778eu,state,fp,bound,observer);return;}
if(call.target==0x32b38cu){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x32781cu,state,fp,bound,observer);return;}
if(call.target==0x32b3c5u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x327855u,state,fp,bound,observer);return;}
if(call.target==0x32b3d6u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x327866u,state,fp,bound,observer);return;}
if(call.target==0x331ecbu){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x32e35bu,state,fp,bound,observer);return;}
if(call.target==0x331f68u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x32e3f8u,state,fp,bound,observer);return;}
if(call.target==0x331fa1u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x32e431u,state,fp,bound,observer);return;}
if(call.target==0x331fa3u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x32e433u,state,fp,bound,observer);return;}
if(call.target==0x331fcbu){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x32e45bu,state,fp,bound,observer);return;}
if(call.target==0x33200au){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x32e49au,state,fp,bound,observer);return;}
if(call.target==0x37cde8u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x379338u,state,fp,bound,observer);return;}
if(call.target==0x37cdeau){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x37933au,state,fp,bound,observer);return;}
if(call.target==0x37d46cu){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x3799bcu,state,fp,bound,observer);return;}
if(call.target==0x37d536u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x379a86u,state,fp,bound,observer);return;}
if(call.target==0x37dc0au){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x37a15au,state,fp,bound,observer);return;}
if(call.target==0x37dc14u){source_run_physics_vehicle_constraint_cleanup_runtime(memory,0x37a164u,state,fp,bound,observer);return;}

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
case 0xde8c0u:
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
case 0x16e6a0u:
case 0x178e30u:
case 0x17a0f0u:
case 0x17dec0u:
case 0x183c50u:
case 0x18bbc0u:
case 0x18c440u:
case 0x18cdd0u:
case 0x18d4b0u:
case 0x18d510u:
case 0x1903a0u:
case 0x1c8430u:
case 0x1d1c00u:
case 0x1d34b0u:
case 0x1d3580u:
case 0x1d43c0u:
case 0x1d44c0u:
case 0x1d4510u:
case 0x1d45a0u:
case 0x1d8b90u:
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
case 0x22e210u:
case 0x22e390u:
case 0x22e4f0u:
case 0x22eec0u:
case 0x22efd0u:
case 0x22fe30u:
case 0x22fea0u:
case 0x230a60u:
case 0x230ae0u:
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

  try {source_run_physics_vehicle_constraint_cleanup_runtime(m,entry,abi,fp,bound,observe);}
  catch(...) {if(abi.floating_point)fp=*abi.floating_point;throw;}
  if(abi.floating_point)fp=*abi.floating_point;return;
case 0xbcdf0u:break;
case 0xc6120u:break;
case 0x11e600u:break;
case 0x11e850u:break;
case 0x14cdb0u:break;
case 0x16e8a0u:break;
case 0x16ea60u:break;
case 0x1755b0u:break;
case 0x177020u:break;
case 0x177fa0u:break;
case 0x17a030u:break;
case 0x17a230u:break;
case 0x17a2f0u:break;
case 0x17a380u:break;
case 0x17a420u:break;
case 0x17a4a0u:break;
case 0x18add0u:break;
case 0x18ae80u:break;
case 0x18af20u:break;
case 0x18b0c0u:break;
case 0x18b160u:break;
case 0x18b1d0u:break;
case 0x190420u:break;
case 0x228220u:break;
case 0x2e44d0u:break;
case 0x2e4650u:break;

 default:throw std::runtime_error("Required TU1.5 vehicle entry unavailable");
 }
 FpScope scope(fp);MathState s{m,observe,bound,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
 try {switch(entry) {
case 0xbcdf0u:run_bcdf0(s);break;
case 0xc6120u:run_c6120(s);break;
case 0x11e600u:run_11e600(s);break;
case 0x11e850u:run_11e850(s);break;
case 0x14cdb0u:run_14cdb0(s);break;
case 0x16e8a0u:run_16e8a0(s);break;
case 0x16ea60u:run_16ea60(s);break;
case 0x1755b0u:run_1755b0(s);break;
case 0x177020u:run_177020(s);break;
case 0x177fa0u:run_177fa0(s);break;
case 0x17a030u:run_17a030(s);break;
case 0x17a230u:run_17a230(s);break;
case 0x17a2f0u:run_17a2f0(s);break;
case 0x17a380u:run_17a380(s);break;
case 0x17a420u:run_17a420(s);break;
case 0x17a4a0u:run_17a4a0(s);break;
case 0x18add0u:run_18add0(s);break;
case 0x18ae80u:run_18ae80(s);break;
case 0x18af20u:run_18af20(s);break;
case 0x18b0c0u:run_18b0c0(s);break;
case 0x18b160u:run_18b160(s);break;
case 0x18b1d0u:run_18b1d0(s);break;
case 0x190420u:run_190420(s);break;
case 0x228220u:run_228220(s);break;
case 0x2e44d0u:run_2e44d0(s);break;
case 0x2e4650u:run_2e4650(s);break;

 }}catch(...) {abi=s.save();fp={control_word(),_mm_getcsr()};throw;}
 abi=s.save();fp={control_word(),_mm_getcsr()};
}
} // namespace halo2
