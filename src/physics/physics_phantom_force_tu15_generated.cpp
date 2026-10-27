// Generated from complete owned TU1.5 instructions. No runtime decoder.
#include "halo2/physics_phantom_force_tu15.hpp"
#include "halo2/physics_velocity_observation.hpp"
#include "halo2/native_lift_scalar_consumer_tu15.hpp"
#include "physics/vehicles/physics_vehicle_motion_math_internal.hpp"
namespace halo2 {
namespace {
using namespace physics_vehicle_motion_math_detail;
template<class State>
void native_lift_scalar_call(State&s,std::uint32_t site,std::uint32_t continuation){
  const auto stack=s.gp[4];bool ordinary{},readonly_observer{},traced{};
  std::function<void(std::uint32_t)> phase;
  if(const auto*owner=native_lift_scalar_consumer_policy_tu15(s.memory)){
    traced=(owner->first_trace&&*owner->first_trace)||(owner->second_trace&&*owner->second_trace);
    readonly_observer=owner->observation_only_stores;phase=owner->publish_phase;
    ordinary=stack>=8u&&stack-8u>=owner->caller_stack_begin&&std::uint64_t(stack)+40u<=owner->caller_stack_end;
  }else{
    if constexpr(requires{s.instruction_cursor;s.literal_service_sites;}){
      if(const auto*cursor=s.instruction_cursor;s.literal_service_sites&&cursor){
        traced=(cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace);
        readonly_observer=cursor->observation_only_stores;phase=[cursor](std::uint32_t pc){*cursor->position=pc;};
      }
    }
    if constexpr(requires{s.memory.try_read_view(stack-8u,48u);})
      ordinary=stack>=8u&&stack<=0xffffffffu-40u&&bool(s.memory.try_read_view(stack-8u,48u));
  }
  if(traced||(s.services.instruction&&!phase)||(s.observe&&!readonly_observer)){
    native_lift_scalar_observation_refused_tu15(s.memory);
    throw std::runtime_error("Native lift scalar consumer does not admit full instruction/private-frame observation");
  }
  if(s.fp.count||s.active_mmx)throw std::runtime_error("Native lift scalar consumer requires its proved empty incoming x87 stack");
  if(!ordinary)
    throw std::runtime_error("Native lift scalar consumer requires ordinary retained caller argument storage");
  // Only the compatibility CALL/RET protocol remains here. Arithmetic and
  // branching live exclusively in the typed scalar/power/CRT modules.
  s.push(site,continuation);const auto incoming=s.fp.word();
  NativeLiftScalarConsumerTu15 context;
  context.call_site=site;context.entry_stack=context.stack_pointer=s.gp[4];context.scratch_word=s.gp[1];
  context.fs_base=s.fs_base;context.continuation=continuation;context.allow_reverse=(s.gp[3]&255u)!=0;
  context.executor=std::this_thread::get_id();context.floating_point={control_word(),_mm_getcsr(),std::uint8_t(incoming&0x3fu),incoming};context.numeric_status=incoming;
  const auto publish=[&]{
    s.gp[4]=context.stack_pointer;if(context.scratch_popped)s.gp[1]=context.scratch_word;
    s.fp.status=context.numeric_status;s.fp.top=(context.numeric_status>>11u)&7u;
    load_control_word(context.floating_point.x87_control_word);_mm_setcsr(context.floating_point.mxcsr);
    std::array<std::uint32_t,7> environment;
    asm volatile("fnstenv %0":"=m"(environment)::"memory");
    environment[1]=(environment[1]&~0xc7ffu)|(context.numeric_status&0xc7ffu);
    asm volatile("fldenv %0"::"m"(environment):"memory");
  };
  try{
    const auto result=native_lift_scalar_consumer_tu15(s.memory,context,phase,s.observe);
    publish();s.xmm[0]=_mm_set_ss(result.result.velocity_delta);
  }catch(...){publish();throw;}
  // Volatile EAX/EDX/other XMM/flags are intentionally not invented. The four
  // authenticated1DA750 continuations reload them before meaningful use.
}
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
void run_24570(MathState &);
void run_e1480(MathState &);
void run_11b930(MathState &);
void run_1d9080(MathState &);
void run_1d9100(MathState &);
void run_1d92c0(MathState &);
void run_1da530(MathState &);
void run_1da750(MathState &);
void run_315c90(MathState &);
void run_315cb0(MathState &);
void run_3239f0(MathState &);
void run_323a00(MathState &);
void run_24570(MathState &s) {
if(s.services.instruction)s.services.instruction(0x24570u); s.fp.push(ext(f(r(s.memory,(0x8u+s.gp[0]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x24573u); s.fp.push(ext(f(r(s.memory,(0x4u+s.gp[0]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x24576u); s.fp.push(ext(f(r(s.memory,(0x0u+s.gp[0]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x24578u); s.fp.push(s.fp.st(0));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x2457au); s.fp.st(0)=mul(s.fp.st(0),s.fp.st(1));
if(s.services.instruction)s.services.instruction(0x2457cu); s.fp.push(s.fp.st(2));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x2457eu); s.fp.st(0)=mul(s.fp.st(0),s.fp.st(3));
if(s.services.instruction)s.services.instruction(0x24580u); s.fp.st(1)=add(s.fp.st(1),s.fp.st(0));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x24582u); s.fp.push(s.fp.st(3));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x24584u); s.fp.st(0)=mul(s.fp.st(0),s.fp.st(4));
if(s.services.instruction)s.services.instruction(0x24586u); s.fp.st(1)=add(s.fp.st(1),s.fp.st(0));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x24588u); s.fp.st(0)=root(s.fp.st(0));
if(s.services.instruction)s.services.instruction(0x2458au); s.fp.st(3)=s.fp.st(0);s.fp.pop();
if(s.services.instruction)s.services.instruction(0x2458cu); s.fp.st(0)=s.fp.st(0);s.fp.pop();
if(s.services.instruction)s.services.instruction(0x2458eu); s.fp.st(0)=s.fp.st(0);s.fp.pop();
if(s.services.instruction)s.services.instruction(0x24590u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}

void run_e1480(MathState &s) {
if(s.services.instruction)s.services.instruction(0xe1480u); s.set(1,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0xe1486u); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0xe1489u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0xe148eu); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0xe1491u); s.set(0,r(s.memory,(0x8u+s.gp[2]+s.gp[0]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0xe1495u); w(s.memory,s.observe,0xe1495u,(0x348u+s.gp[0]),1,s.flags.logic((r(s.memory,(0x348u+s.gp[0]),1))|(0x80u),1));
if(s.services.instruction)s.services.instruction(0xe149cu); s.set(0,s.flags.plus(s.reg(0,4,0),0x348u,4),4,0);
if(s.services.instruction)s.services.instruction(0xe14a1u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}

void run_11b930(MathState &s) {
if(s.services.instruction)s.services.instruction(0x11b930u); s.set(1,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x11b936u); s.set(2,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x11b939u); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x11b93eu); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x11b941u); s.set(0,r(s.memory,(0x8u+s.gp[2]+s.gp[0]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x11b945u); w(s.memory,s.observe,0x11b945u,(0x12cu+s.gp[0]),1,s.flags.logic((r(s.memory,(0x12cu+s.gp[0]),1))|(0x40u),1));
if(s.services.instruction)s.services.instruction(0x11b94cu); s.set(0,s.flags.plus(s.reg(0,4,0),0x12cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x11b951u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}

void run_1d9080(MathState &s) {
if(s.services.instruction)s.services.instruction(0x1d9080u); s.push(0x1d9080u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x1d9081u); s.set(5,s.reg(4,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1d9083u); s.set(4,s.flags.logic((s.reg(4,4,0))&(0xfffffff0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d9086u); s.set(4,s.flags.minus(s.reg(4,4,0),0x1cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d9089u); s.set(2,r(s.memory,(0x8u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d908cu); s.set(2,r(s.memory,(0x70u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d908fu); s.set(1,(0x0u+s.gp[1]+s.gp[1]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x1d9092u); s.set(1,s.flags.shift(s.reg(1,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d9095u); s.push(0x1d9095u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x1d9096u); s.set(6,r(s.memory,(0x40u+s.gp[1]+s.gp[2]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d909au); s.set(1,r(s.memory,(0x40u+s.gp[6]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1d909du); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1d909fu); if(!s.flags.zf)goto L1d90f6;
if(s.services.instruction)s.services.instruction(0x1d90a1u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x1d90a5u); w(s.memory,s.observe,0x1d90a5u,(0x10u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d90abu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x1d90b0u); w(s.memory,s.observe,0x1d90b0u,(0x14u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d90b6u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x1d90bbu); s.set(0,(0xfu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1d90bfu); w(s.memory,s.observe,0x1d90bfu,(0x18u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d90c5u); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1d90c8u); s.push(0x1d90c8u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1d90c9u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1d90cbu); w(s.memory,s.observe,0x1d90cbu,(0x20u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d90d1u); s.push(0x1d90d1u,0x1d90d6u);run_315c90(s);
if(s.services.instruction)s.services.instruction(0x1d90d6u); s.flags.minus(r(s.memory,(0x0u+s.gp[0]),1),0x0u,1);
if(s.services.instruction)s.services.instruction(0x1d90d9u); if(!s.flags.zf)goto L1d90e9;
if(s.services.instruction)s.services.instruction(0x1d90dbu); s.set(0,r(s.memory,(0x44u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d90deu); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1d90e0u); if(s.flags.zf)goto L1d90e9;
if(s.services.instruction)s.services.instruction(0x1d90e2u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1d90e4u); s.push(0x1d90e4u,0x1d90e9u);run_315cb0(s);
L1d90e9: if(s.services.instruction)s.services.instruction(0x1d90e9u); s.set(1,r(s.memory,(0x3cu+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d90ecu); s.set(2,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d90eeu); s.set(0,(0x10u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1d90f2u); s.push(0x1d90f2u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1d90f3u); {const auto target=r(s.memory,(0x50u+s.gp[2]),4);s.push(0x1d90f3u,0x1d90f6u);s.required(0x1d90f3u,target,false);}
L1d90f6: if(s.services.instruction)s.services.instruction(0x1d90f6u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1d90f7u); s.set(4,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1d90f9u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1d90fau); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}

void run_1d9100(MathState &s) {
if(s.services.instruction)s.services.instruction(0x1d9100u); s.push(0x1d9100u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x1d9101u); s.set(5,s.reg(4,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1d9103u); s.set(4,s.flags.logic((s.reg(4,4,0))&(0xfffffff0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d9106u); s.set(4,s.flags.minus(s.reg(4,4,0),0x1cu,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d9109u); s.push(0x1d9109u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x1d910au); s.set(6,r(s.memory,(0x8u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d910du); s.set(2,r(s.memory,(0x4u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d9110u); s.set(2,s.flags.right(s.reg(2,4,0),0x1u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x1d9112u); s.flags.logic((s.reg(2,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x1d9115u); if(!s.flags.zf)goto L1d9184;
if(s.services.instruction)s.services.instruction(0x1d9117u); s.set(2,r(s.memory,(0x70u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d911au); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x1d911du); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d9120u); s.set(0,(0x40u+s.gp[0]+s.gp[2]*1u),4,0);
if(s.services.instruction)s.services.instruction(0x1d9124u); s.set(2,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d9126u); s.set(2,r(s.memory,(0x40u+s.gp[2]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1d9129u); s.flags.logic((s.reg(2,1,0))&(s.reg(2,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1d912bu); if(!s.flags.zf)goto L1d9184;
if(s.services.instruction)s.services.instruction(0x1d912du); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[1]),4)));
if(s.services.instruction)s.services.instruction(0x1d9131u); w(s.memory,s.observe,0x1d9131u,(0x10u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d9137u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[1]),4)));
if(s.services.instruction)s.services.instruction(0x1d913cu); w(s.memory,s.observe,0x1d913cu,(0x14u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d9142u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[1]),4)));
if(s.services.instruction)s.services.instruction(0x1d9147u); w(s.memory,s.observe,0x1d9147u,(0x18u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d914du); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1d9150u); w(s.memory,s.observe,0x1d9150u,(0x1cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1d9156u); s.set(6,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d9158u); s.set(0,(0xfu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1d915cu); s.push(0x1d915cu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1d915du); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1d915fu); s.push(0x1d915fu,0x1d9164u);run_315c90(s);
if(s.services.instruction)s.services.instruction(0x1d9164u); s.flags.minus(r(s.memory,(0x0u+s.gp[0]),1),0x0u,1);
if(s.services.instruction)s.services.instruction(0x1d9167u); if(!s.flags.zf)goto L1d9177;
if(s.services.instruction)s.services.instruction(0x1d9169u); s.set(0,r(s.memory,(0x44u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d916cu); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1d916eu); if(s.flags.zf)goto L1d9177;
if(s.services.instruction)s.services.instruction(0x1d9170u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1d9172u); s.push(0x1d9172u,0x1d9177u);run_315cb0(s);
L1d9177: if(s.services.instruction)s.services.instruction(0x1d9177u); s.set(1,r(s.memory,(0x3cu+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d917au); s.set(2,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d917cu); s.set(0,(0x10u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1d9180u); s.push(0x1d9180u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1d9181u); {const auto target=r(s.memory,(0x54u+s.gp[2]),4);s.push(0x1d9181u,0x1d9184u);s.required(0x1d9181u,target,false);}
L1d9184: if(s.services.instruction)s.services.instruction(0x1d9184u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1d9185u); s.set(4,s.reg(5,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1d9187u); s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1d9188u); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}

void run_1d92c0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x1d92c0u); s.set(4,s.flags.minus(s.reg(4,4,0),0xcu,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d92c3u); s.push(0x1d92c3u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x1d92c4u); s.set(0,(0x0u+s.gp[1]+s.gp[1]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x1d92c7u); s.push(0x1d92c7u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x1d92c8u); s.set(7,r(s.memory,(0x70u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d92cbu); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d92ceu); s.set(0,r(s.memory,(0x40u+s.gp[0]+s.gp[7]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d92d2u); s.set(3,r(s.memory,(0x40u+s.gp[0]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1d92d5u); s.flags.logic((s.reg(3,1,0))&(s.reg(3,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1d92d7u); if(!s.flags.zf)goto L1d92f0;
if(s.services.instruction)s.services.instruction(0x1d92d9u); s.set(0,r(s.memory,(0x3cu+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d92dcu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x40u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x1d92e1u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x44u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x1d92e6u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x48u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x1d92ebu); s.set(0,s.flags.plus(s.reg(0,4,0),0x40u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d92eeu); goto L1d931b;
L1d92f0: if(s.services.instruction)s.services.instruction(0x1d92f0u); s.set(0,r(s.memory,(0x474d04u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d92f5u); s.set(7,r(s.memory,(0x0u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d92f7u); w(s.memory,s.observe,0x1d92f7u,(0x8u+s.gp[4]),4,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x1d92fbu); s.set(7,r(s.memory,(0x4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d92feu); s.set(0,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1d9301u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d9307u); w(s.memory,s.observe,0x1d9307u,(0xcu+s.gp[4]),4,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x1d930bu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0xcu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1d9311u); w(s.memory,s.observe,0x1d9311u,(0x10u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1d9315u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x10u+s.gp[4]),4)));
L1d931b: if(s.services.instruction)s.services.instruction(0x1d931bu); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[2]),4)));
if(s.services.instruction)s.services.instruction(0x1d931fu); s.xmm[3]=_mm_add_ss(s.xmm[3],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x1d9323u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[2]),4)));
if(s.services.instruction)s.services.instruction(0x1d9328u); s.xmm[2]=_mm_add_ss(s.xmm[2],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1d932cu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[2]),4)));
if(s.services.instruction)s.services.instruction(0x1d9331u); s.xmm[1]=_mm_add_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1d9335u); s.push(0x1d9335u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x1d9336u); s.set(0,(0xcu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1d933au); w(s.memory,s.observe,0x1d933au,(0xcu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x1d9340u); w(s.memory,s.observe,0x1d9340u,(0x10u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x1d9346u); w(s.memory,s.observe,0x1d9346u,(0x14u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1d934cu); s.push(0x1d934cu,0x1d9351u);run_1d9080(s);
if(s.services.instruction)s.services.instruction(0x1d9351u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1d9352u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1d9353u); s.set(4,s.flags.plus(s.reg(4,4,0),0xcu,4),4,0);
if(s.services.instruction)s.services.instruction(0x1d9356u); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}

void run_1da530(MathState &s) {
if(s.services.instruction)s.services.instruction(0x1da530u); s.push(0x1da530u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1da531u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x24u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1da537u); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x46a180u),4))),false);
if(s.services.instruction)s.services.instruction(0x1da53eu); if(s.flags.cf||s.flags.zf)goto L1da57a;
if(s.services.instruction)s.services.instruction(0x1da540u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x10u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1da546u); s.xmm[1]=_mm_xor_ps(s.xmm[1],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1da549u); s.flags.scalar_compare(s.xmm[0],s.xmm[1],false);
if(s.services.instruction)s.services.instruction(0x1da54cu); if(s.flags.cf)goto L1da555;
if(s.services.instruction)s.services.instruction(0x1da54eu); w(s.memory,s.observe,0x1da54eu,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1da553u); goto L1da55e;
L1da555: if(s.services.instruction)s.services.instruction(0x1da555u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1da559u); w(s.memory,s.observe,0x1da559u,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
L1da55e: if(s.services.instruction)s.services.instruction(0x1da55eu); s.fp.push(ext(f(r(s.memory,(0x0u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1da561u); s.fp.push(ext(f(r(s.memory,(0x24u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1da565u); {const auto target=0x37cde8u;s.push(0x1da565u,0x1da56au);s.required(0x1da565u,target,false);}
if(s.services.instruction)s.services.instruction(0x1da56au); s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x14u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1da56eu); w(s.memory,s.observe,0x1da56eu,(0x24u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1da572u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x24u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1da578u); goto L1da580;
L1da57a: if(s.services.instruction)s.services.instruction(0x1da57au); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4)));
L1da580: if(s.services.instruction)s.services.instruction(0x1da580u); s.set(0,r(s.memory,(0x8u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da584u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1da586u); if(s.flags.zf)goto L1da59b;
if(s.services.instruction)s.services.instruction(0x1da588u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0xcu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1da58eu); s.flags.scalar_compare(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x46a180u),4))),false);
if(s.services.instruction)s.services.instruction(0x1da595u); if(s.flags.cf||s.flags.zf)goto L1da59b;
if(s.services.instruction)s.services.instruction(0x1da597u); s.xmm[0]=_mm_div_ss(s.xmm[0],s.xmm[1]);
L1da59b: if(s.services.instruction)s.services.instruction(0x1da59bu); s.set(0,r(s.memory,(0x51df54u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da5a0u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x1da5a5u); s.xmm[3]=_mm_xor_ps(s.xmm[3],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1da5a8u); s.set(0,s.flags.plus(s.reg(0,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1da5abu); s.flags.scalar_compare(s.xmm[0],s.xmm[3],false);
if(s.services.instruction)s.services.instruction(0x1da5aeu); w(s.memory,s.observe,0x1da5aeu,(0x8u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1da5b4u); if(s.flags.cf)goto L1da5bb;
if(s.services.instruction)s.services.instruction(0x1da5b6u); s.xmm[6]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1da5b9u); goto L1da5c2;
L1da5bb: if(s.services.instruction)s.services.instruction(0x1da5bbu); s.xmm[6]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x1da5beu); s.xmm[6]=_mm_sub_ss(s.xmm[6],s.xmm[0]);
L1da5c2: if(s.services.instruction)s.services.instruction(0x1da5c2u); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x46a180u),4))),false);
if(s.services.instruction)s.services.instruction(0x1da5c9u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x10u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1da5cfu); s.xmm[1]=_mm_sub_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x20u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1da5d5u); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x1cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1da5dbu); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x18u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1da5e1u); w(s.memory,s.observe,0x1da5e1u,(0x10u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1da5e7u); if(s.flags.cf||s.flags.zf)goto L1da66e;
if(s.services.instruction)s.services.instruction(0x1da5edu); s.set(1,r(s.memory,(0x28u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da5f1u); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1da5f3u); if(s.flags.zf)goto L1da5fb;
if(s.services.instruction)s.services.instruction(0x1da5f5u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1da5f9u); goto L1da64d;
L1da5fb: if(s.services.instruction)s.services.instruction(0x1da5fbu); s.xmm[2]=s.xmm[4];
if(s.services.instruction)s.services.instruction(0x1da5feu); s.xmm[2]=_mm_div_ss(s.xmm[2],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1da602u); s.xmm[2]=_mm_add_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x46a0d0u),4))));
if(s.services.instruction)s.services.instruction(0x1da60au); s.xmm[7]=s.xmm[4];
if(s.services.instruction)s.services.instruction(0x1da60du); s.xmm[7]=_mm_mul_ss(s.xmm[7],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x1da611u); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[7]);
if(s.services.instruction)s.services.instruction(0x1da615u); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x46a0ccu),4))));
if(s.services.instruction)s.services.instruction(0x1da61du); s.flags.scalar_compare(s.xmm[1],s.xmm[2],false);
if(s.services.instruction)s.services.instruction(0x1da620u); if(s.flags.cf||s.flags.zf)goto L1da62d;
if(s.services.instruction)s.services.instruction(0x1da622u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x1da625u); if(s.flags.cf||s.flags.zf)goto L1da62d;
if(s.services.instruction)s.services.instruction(0x1da627u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1da62bu); goto L1da64d;
L1da62d: if(s.services.instruction)s.services.instruction(0x1da62du); s.flags.scalar_compare(s.xmm[3],s.xmm[1],false);
if(s.services.instruction)s.services.instruction(0x1da630u); if(s.flags.cf||s.flags.zf)goto L1da637;
if(s.services.instruction)s.services.instruction(0x1da632u); s.xmm[0]=s.xmm[4];
if(s.services.instruction)s.services.instruction(0x1da635u); goto L1da64d;
L1da637: if(s.services.instruction)s.services.instruction(0x1da637u); s.fp.push(ext(f(r(s.memory,(0x8u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1da63bu); s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x10u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1da63fu); s.fp.st(0)=add(s.fp.st(0),s.fp.st(0));
if(s.services.instruction)s.services.instruction(0x1da641u); s.fp.st(0)=root(s.fp.st(0));
if(s.services.instruction)s.services.instruction(0x1da643u); w(s.memory,s.observe,0x1da643u,(0x8u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1da647u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x8u+s.gp[4]),4)));
L1da64d: if(s.services.instruction)s.services.instruction(0x1da64du); s.flags.logic((s.reg(3,1,0))&(s.reg(3,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1da64fu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x44aac8u),4)));
if(s.services.instruction)s.services.instruction(0x1da657u); s.xmm[1]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x1da65au); if(!s.flags.zf)goto L1da65f;
if(s.services.instruction)s.services.instruction(0x1da65cu); s.xmm[1]=s.xmm[4];
L1da65f: if(s.services.instruction)s.services.instruction(0x1da65fu); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x1da662u); if(!s.flags.cf&&!s.flags.zf)goto L1da69a;
if(s.services.instruction)s.services.instruction(0x1da664u); s.flags.scalar_compare(s.xmm[0],s.xmm[5],false);
if(s.services.instruction)s.services.instruction(0x1da667u); if(s.flags.cf||s.flags.zf)goto L1da6a4;
if(s.services.instruction)s.services.instruction(0x1da669u); s.xmm[0]=s.xmm[5];
if(s.services.instruction)s.services.instruction(0x1da66cu); goto L1da6a4;
L1da66e: if(s.services.instruction)s.services.instruction(0x1da66eu); s.xmm[1]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x1da671u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1da675u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[5]);
if(s.services.instruction)s.services.instruction(0x1da679u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x1da67cu); if(s.flags.cf||s.flags.zf)goto L1da683;
if(s.services.instruction)s.services.instruction(0x1da67eu); s.xmm[0]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x1da681u); goto L1da6a4;
L1da683: if(s.services.instruction)s.services.instruction(0x1da683u); s.flags.logic((s.reg(3,1,0))&(s.reg(3,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1da685u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x45695cu),4)));
if(s.services.instruction)s.services.instruction(0x1da68du); s.xmm[1]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x1da690u); if(!s.flags.zf)goto L1da695;
if(s.services.instruction)s.services.instruction(0x1da692u); s.xmm[1]=s.xmm[4];
L1da695: if(s.services.instruction)s.services.instruction(0x1da695u); s.flags.scalar_compare(s.xmm[0],s.xmm[1],false);
if(s.services.instruction)s.services.instruction(0x1da698u); if(s.flags.cf||s.flags.zf)goto L1da6a4;
L1da69a: if(s.services.instruction)s.services.instruction(0x1da69au); s.flags.logic((s.reg(3,1,0))&(s.reg(3,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1da69cu); s.xmm[0]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x1da69fu); if(!s.flags.zf)goto L1da6a4;
if(s.services.instruction)s.services.instruction(0x1da6a1u); s.xmm[0]=s.xmm[4];
L1da6a4: if(s.services.instruction)s.services.instruction(0x1da6a4u); s.flags.scalar_compare(s.xmm[5],s.xmm[3],false);
if(s.services.instruction)s.services.instruction(0x1da6a7u); if(s.flags.cf)goto L1da6ae;
if(s.services.instruction)s.services.instruction(0x1da6a9u); s.xmm[1]=s.xmm[5];
if(s.services.instruction)s.services.instruction(0x1da6acu); goto L1da6b5;
L1da6ae: if(s.services.instruction)s.services.instruction(0x1da6aeu); s.xmm[1]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x1da6b1u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[5]);
L1da6b5: if(s.services.instruction)s.services.instruction(0x1da6b5u); s.flags.scalar_compare(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x46a180u),4))),false);
if(s.services.instruction)s.services.instruction(0x1da6bcu); if(s.flags.cf||s.flags.zf)goto L1da6d8;
if(s.services.instruction)s.services.instruction(0x1da6beu); s.xmm[1]=s.xmm[4];
if(s.services.instruction)s.services.instruction(0x1da6c1u); s.xmm[1]=_mm_div_ss(s.xmm[1],s.xmm[5]);
if(s.services.instruction)s.services.instruction(0x1da6c5u); s.flags.scalar_compare(s.xmm[1],s.xmm[3],false);
if(s.services.instruction)s.services.instruction(0x1da6c8u); if(s.flags.cf)goto L1da6cf;
if(s.services.instruction)s.services.instruction(0x1da6cau); s.xmm[2]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x1da6cdu); goto L1da6db;
L1da6cf: if(s.services.instruction)s.services.instruction(0x1da6cfu); s.xmm[2]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x1da6d2u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1da6d6u); goto L1da6db;
L1da6d8: if(s.services.instruction)s.services.instruction(0x1da6d8u); s.xmm[2]=s.xmm[3];
L1da6db: if(s.services.instruction)s.services.instruction(0x1da6dbu); s.set(0,r(s.memory,(0x2cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da6dfu); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1da6e3u); s.xmm[3]=_mm_sub_ss(s.xmm[3],s.xmm[6]);
if(s.services.instruction)s.services.instruction(0x1da6e7u); s.flags.scalar_compare(s.xmm[3],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x1da6eau); w(s.memory,s.observe,0x1da6eau,(0x0u+s.gp[0]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x1da6eeu); if(s.flags.cf||s.flags.zf)goto L1da6f7;
if(s.services.instruction)s.services.instruction(0x1da6f0u); s.xmm[0]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x1da6f3u); s.set(1,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1da6f4u); s.pop();s.gp[4]+=40u;return;
L1da6f7: if(s.services.instruction)s.services.instruction(0x1da6f7u); s.flags.scalar_compare(s.xmm[0],s.xmm[6],false);
if(s.services.instruction)s.services.instruction(0x1da6fau); if(s.flags.cf||s.flags.zf)goto L1da6ff;
if(s.services.instruction)s.services.instruction(0x1da6fcu); s.xmm[0]=s.xmm[6];
L1da6ff: if(s.services.instruction)s.services.instruction(0x1da6ffu); s.set(1,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1da700u); s.pop();s.gp[4]+=40u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}

void run_1da750(MathState &s) {
if(s.services.instruction)s.services.instruction(0x1da750u); s.set(1,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da754u); s.set(0,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da757u); s.set(4,s.flags.minus(s.reg(4,4,0),0x1d4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1da75du); s.flags.logic((s.reg(0,4,0))&(0x104000u),4);
if(s.services.instruction)s.services.instruction(0x1da762u); if(s.flags.zf)goto L1db7ea;
if(s.services.instruction)s.services.instruction(0x1da768u); s.fp.push(ext(f(r(s.memory,(0x1dcu+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1da76fu); s.fp.st(0)=std::fabs(s.fp.st(0));
if(s.services.instruction)s.services.instruction(0x1da771u); s.fp.push(ext(f(r(s.memory,(0x46a0ecu),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1da777u); s.compare_and_pop(s.fp.st(1));
if(s.services.instruction)s.services.instruction(0x1da779u); s.fp.st(0)=s.fp.st(0);s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1da77bu); if(!s.flags.cf&&!s.flags.zf)goto L1da87d;
if(s.services.instruction)s.services.instruction(0x1da781u); s.set(0,s.flags.right(s.reg(0,4,0),0xdu,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x1da784u); s.flags.logic((s.reg(0,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x1da786u); w(s.memory,s.observe,0x1da786u,(0x8u+s.gp[4]),1,0x1u);
if(s.services.instruction)s.services.instruction(0x1da78bu); w(s.memory,s.observe,0x1da78bu,(0xau+s.gp[4]),1,0x1u);
if(s.services.instruction)s.services.instruction(0x1da790u); if(s.flags.zf)goto L1da797;
L1da792: if(s.services.instruction)s.services.instruction(0x1da792u); w(s.memory,s.observe,0x1da792u,(0xau+s.gp[4]),1,0x0u);
L1da797: if(s.services.instruction)s.services.instruction(0x1da797u); s.set(1,r(s.memory,(0x94u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da79du); s.flags.logic((s.reg(1,4,0))&(s.reg(1,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1da79fu); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1da7a2u); s.push(0x1da7a2u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x1da7a3u); s.push(0x1da7a3u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x1da7a4u); s.push(0x1da7a4u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x1da7a5u); w(s.memory,s.observe,0x1da7a5u,(0xfu+s.gp[4]),1,0x0u);
if(s.services.instruction)s.services.instruction(0x1da7aau); w(s.memory,s.observe,0x1da7aau,(0x18u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1da7b0u); if(s.flags.zf)goto L1db698;
if(s.services.instruction)s.services.instruction(0x1da7b6u); s.set(0,r(s.memory,(0x14u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da7bau); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1da7bcu); if(s.flags.zf)goto L1db698;
if(s.services.instruction)s.services.instruction(0x1da7c2u); s.set(0,r(s.memory,(0x1e4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7c9u); s.set(0,r(s.memory,(0x8u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7ccu); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7d1u); s.set(2,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x1da7d4u); s.set(0,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7d9u); s.set(0,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7dcu); s.set(2,r(s.memory,(0x8u+s.gp[0]+s.gp[2]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7e0u); s.set(2,r(s.memory,(0x0u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7e2u); s.set(0,r(s.memory,(0x4f0414u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7e7u); s.set(2,s.flags.logic((s.reg(2,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7edu); s.set(2,s.flags.shift(s.reg(2,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7f0u); s.set(2,r(s.memory,(0x8u+s.gp[2]+s.gp[0]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7f4u); s.set(2,r(s.memory,(0x38u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7f7u); s.set(2,s.flags.logic((s.reg(2,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da7fdu); s.set(2,s.flags.shift(s.reg(2,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1da800u); s.set(2,r(s.memory,(0x8u+s.gp[2]+s.gp[0]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da804u); s.set(2,r(s.memory,(0x24u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da807u); s.set(2,s.flags.logic((s.reg(2,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da80du); s.set(2,s.flags.shift(s.reg(2,4,0),0x4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1da810u); s.set(0,r(s.memory,(0x8u+s.gp[2]+s.gp[0]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da814u); w(s.memory,s.observe,0x1da814u,(0xfcu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1da81bu); s.set(0,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da81eu); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1da820u); w(s.memory,s.observe,0x1da820u,(0x58u+s.gp[4]),4,0x0u);
if(s.services.instruction)s.services.instruction(0x1da828u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L1db698;
if(s.services.instruction)s.services.instruction(0x1da82eu); s.push(0x1da82eu,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x1da82fu); ;
L1da830: if(s.services.instruction)s.services.instruction(0x1da830u); s.set(1,r(s.memory,(0x1e8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da837u); s.set(3,r(s.memory,(0x94u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da83du); s.set(0,r(s.memory,(0x5cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da841u); s.set(6,r(s.memory,(0x0u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da843u); s.set(0,s.flags.shift(s.reg(0,4,0),0x3u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1da846u); s.set(7,r(s.memory,(0x4u+s.gp[6]+s.gp[0]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da84au); s.set(6,s.flags.plus(s.reg(6,4,0),s.reg(0,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da84cu); s.set(0,sign_extend(r(s.memory,(0x0u+s.gp[6]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x1da84fu); s.set(2,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x1da852u); s.set(0,r(s.memory,(0x100u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da859u); s.set(1,r(s.memory,(0x44u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da85cu); s.set(5,sign_extend(r(s.memory,(0x8u+s.gp[1]+s.gp[2]*4u),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x1da861u); s.set(1,r(s.memory,(0x2cu+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da864u); s.set(5,s.flags.product(s.reg(5,4,0),0x68u),4,0);
if(s.services.instruction)s.services.instruction(0x1da867u); s.set(5,s.flags.plus(s.reg(5,4,0),s.reg(1,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da869u); s.flags.minus(s.reg(7,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x1da86cu); w(s.memory,s.observe,0x1da86cu,(0x64u+s.gp[4]),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x1da870u); if(s.flags.zf)goto L1da887;
if(s.services.instruction)s.services.instruction(0x1da872u); s.push(0x1da872u,0xffffffffu);
if(s.services.instruction)s.services.instruction(0x1da874u); s.set(0,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1da876u); {const auto target=0xbcdf0u;s.push(0x1da876u,0x1da87bu);s.required(0x1da876u,target,false);}
if(s.services.instruction)s.services.instruction(0x1da87bu); goto L1da889;
L1da87d: if(s.services.instruction)s.services.instruction(0x1da87du); w(s.memory,s.observe,0x1da87du,(0x8u+s.gp[4]),1,0x0u);
if(s.services.instruction)s.services.instruction(0x1da882u); goto L1da792;
L1da887: if(s.services.instruction)s.services.instruction(0x1da887u); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
L1da889: if(s.services.instruction)s.services.instruction(0x1da889u); s.flags.minus(s.reg(7,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x1da88cu); if(s.flags.zf)goto L1db656;
if(s.services.instruction)s.services.instruction(0x1da892u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1da894u); if(s.flags.zf)goto L1db656;
if(s.services.instruction)s.services.instruction(0x1da89au); s.set(0,r(s.memory,(0xb4u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da8a0u); s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x1da8a3u); if(s.flags.zf)goto L1db656;
if(s.services.instruction)s.services.instruction(0x1da8a9u); s.set(2,r(s.memory,(0x52be5cu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da8afu); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da8b4u); s.set(7,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1da8b6u); s.set(0,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da8b9u); s.set(7,(0x0u+s.gp[7]+s.gp[7]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x1da8bcu); s.set(7,s.flags.shift(s.reg(7,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1da8bfu); s.set(7,s.flags.plus(s.reg(7,4,0),s.reg(0,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da8c1u); s.set(0,r(s.memory,(0x1u+s.gp[6]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da8c4u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1da8c6u); w(s.memory,s.observe,0x1da8c6u,(0x20u+s.gp[4]),4,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x1da8cau); if(s.flags.sf==s.flags.of)goto L1da8d0;
if(s.services.instruction)s.services.instruction(0x1da8ccu); s.set(1,s.flags.logic((s.reg(1,4,0))^(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da8ceu); goto L1da8dc;
L1da8d0: if(s.services.instruction)s.services.instruction(0x1da8d0u); s.set(2,r(s.memory,(0x74u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da8d3u); s.set(1,sign_extend(s.reg(0,1,0),1),4,0);
if(s.services.instruction)s.services.instruction(0x1da8d6u); s.flags.minus(s.reg(1,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x1da8d8u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L1da8dc;
if(s.services.instruction)s.services.instruction(0x1da8dau); s.set(1,s.reg(2,4,0),4,0);
L1da8dc: if(s.services.instruction)s.services.instruction(0x1da8dcu); s.set(0,sign_extend(s.reg(0,1,0),1),4,0);
if(s.services.instruction)s.services.instruction(0x1da8dfu); s.flags.minus(s.reg(1,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x1da8e1u); if(!s.flags.zf)goto L1db656;
if(s.services.instruction)s.services.instruction(0x1da8e7u); s.set(0,r(s.memory,(0x0u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da8eau); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1da8ecu); s.set(0,s.flags.right(s.reg(0,4,0),0x2u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x1da8efu); s.set(0,s.flags.logic((s.reg(0,1,0))&(0x1u),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da8f1u); w(s.memory,s.observe,0x1da8f1u,(0x19u+s.gp[4]),1,s.reg(0,1,0));
if(s.services.instruction)s.services.instruction(0x1da8f5u); s.set(0,sign_extend(r(s.memory,(0x3u+s.gp[6]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x1da8f9u); s.set(1,s.flags.right(s.reg(1,4,0),0x1u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x1da8fbu); s.set(1,s.flags.logic((s.reg(1,1,0))&(0x1u),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da8feu); s.set(2,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x1da901u); s.set(0,r(s.memory,(0x1e8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da908u); w(s.memory,s.observe,0x1da908u,(0x60u+s.gp[4]),1,s.reg(1,1,0));
if(s.services.instruction)s.services.instruction(0x1da90cu); s.set(1,r(s.memory,(0x70u+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da90fu); s.set(2,s.flags.shift(s.reg(2,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1da912u); s.set(1,r(s.memory,(0x40u+s.gp[2]+s.gp[1]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da916u); s.set(0,r(s.memory,(0x40u+s.gp[1]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da919u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1da91bu); if(!s.flags.zf)goto L1da92f;
if(s.services.instruction)s.services.instruction(0x1da91du); s.set(2,(0x97u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1da924u); s.push(0x1da924u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1da925u); s.push(0x1da925u,0x1da92au);run_315c90(s);
if(s.services.instruction)s.services.instruction(0x1da92au); s.flags.minus(r(s.memory,(0x0u+s.gp[0]),1),0x0u,1);
if(s.services.instruction)s.services.instruction(0x1da92du); if(!s.flags.zf)goto L1da93b;
L1da92f: if(s.services.instruction)s.services.instruction(0x1da92fu); s.set(0,r(s.memory,(0x1au+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da933u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1da935u); if(!s.flags.zf)goto L1da93b;
if(s.services.instruction)s.services.instruction(0x1da937u); s.set(3,s.flags.logic((s.reg(3,1,0))^(s.reg(3,1,0)),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da939u); goto L1da93d;
L1da93b: if(s.services.instruction)s.services.instruction(0x1da93bu); s.set(3,0x1u,1,0);
L1da93d: if(s.services.instruction)s.services.instruction(0x1da93du); s.set(0,sign_extend(r(s.memory,(0x1u+s.gp[6]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x1da941u); s.set(1,r(s.memory,(0x70u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da944u); s.set(0,(0x0u+s.gp[0]+s.gp[0]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x1da947u); s.set(0,s.flags.shift(s.reg(0,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1da94au); s.set(1,r(s.memory,(0x40u+s.gp[0]+s.gp[1]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da94eu); s.set(2,(0x87u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1da955u); s.push(0x1da955u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1da956u); s.push(0x1da956u,0x1da95bu);run_315c90(s);
if(s.services.instruction)s.services.instruction(0x1da95bu); s.flags.minus(r(s.memory,(0x0u+s.gp[0]),1),0x0u,1);
if(s.services.instruction)s.services.instruction(0x1da95eu); if(!s.flags.zf)goto L1da969;
if(s.services.instruction)s.services.instruction(0x1da960u); s.flags.logic((s.reg(3,1,0))&(s.reg(3,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1da962u); w(s.memory,s.observe,0x1da962u,(0x1bu+s.gp[4]),1,0x0u);
if(s.services.instruction)s.services.instruction(0x1da967u); if(s.flags.zf)goto L1da96e;
L1da969: if(s.services.instruction)s.services.instruction(0x1da969u); w(s.memory,s.observe,0x1da969u,(0x1bu+s.gp[4]),1,0x1u);
L1da96e: if(s.services.instruction)s.services.instruction(0x1da96eu); s.set(0,r(s.memory,(0x0u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da971u); s.set(2,r(s.memory,(0x1e8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da978u); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1da97au); s.set(1,s.flags.right(s.reg(1,4,0),0x19u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x1da97du); s.set(1,s.flags.logic((s.reg(1,1,0))&(0x1u),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da980u); s.set(0,s.flags.right(s.reg(0,4,0),0x1au,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x1da983u); w(s.memory,s.observe,0x1da983u,(0x83u+s.gp[4]),1,s.reg(1,1,0));
if(s.services.instruction)s.services.instruction(0x1da98au); s.set(1,r(s.memory,(0x8u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da98du); s.set(0,s.flags.logic((s.reg(0,1,0))&(0x1u),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da98fu); s.push(0x1da98fu,0x1u);
if(s.services.instruction)s.services.instruction(0x1da991u); w(s.memory,s.observe,0x1da991u,(0x100u+s.gp[4]),1,s.reg(0,1,0));
if(s.services.instruction)s.services.instruction(0x1da998u); s.set(0,r(s.memory,(0x8u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da99bu); s.push(0x1da99bu,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1da99cu); s.push(0x1da99cu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1da99du); s.set(1,s.flags.logic((s.reg(1,1,0))^(s.reg(1,1,0)),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da99fu); s.set(3,(0x118u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1da9a6u); {const auto target=0xbad60u;s.push(0x1da9a6u,0x1da9abu);s.required(0x1da9a6u,target,false);}
if(s.services.instruction)s.services.instruction(0x1da9abu); s.flags.logic((s.reg(0,2,0))&(s.reg(0,2,0)),2);
if(s.services.instruction)s.services.instruction(0x1da9aeu); if(!s.flags.zf)goto L1da9c7;
if(s.services.instruction)s.services.instruction(0x1da9b0u); s.set(1,r(s.memory,(0x1e8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da9b7u); s.set(0,(0x144u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1da9beu); s.push(0x1da9beu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1da9bfu); s.set(0,r(s.memory,(0x8u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da9c2u); {const auto target=0xbc190u;s.push(0x1da9c2u,0x1da9c7u);s.required(0x1da9c2u,target,false);}
L1da9c7: if(s.services.instruction)s.services.instruction(0x1da9c7u); s.set(0,r(s.memory,(0x1bu+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1da9cbu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1da9cdu); if(s.flags.zf)goto L1db676;
if(s.services.instruction)s.services.instruction(0x1da9d3u); s.set(2,r(s.memory,(0x474d04u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da9d9u); s.set(0,r(s.memory,(0x0u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da9dbu); s.set(1,r(s.memory,(0x4u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da9deu); s.set(2,r(s.memory,(0x8u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da9e1u); w(s.memory,s.observe,0x1da9e1u,(0x28u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1da9e5u); w(s.memory,s.observe,0x1da9e5u,(0x24u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1da9e9u); s.set(0,r(s.memory,(0x8u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da9ecu); w(s.memory,s.observe,0x1da9ecu,(0x2cu+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1da9f0u); s.set(2,r(s.memory,(0x4ecbd0u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da9f6u); s.set(2,r(s.memory,(0x44u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1da9f9u); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1da9fbu); s.set(1,s.flags.logic((s.reg(1,4,0))&(0xffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daa01u); s.set(1,(0x0u+s.gp[1]+s.gp[1]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x1daa04u); s.set(1,r(s.memory,(0x3u+s.gp[2]+s.gp[1]*4u),1),4,0);
if(s.services.instruction)s.services.instruction(0x1daa09u); s.set(2,0x1u,4,0);
if(s.services.instruction)s.services.instruction(0x1daa0eu); s.set(2,s.flags.shift(s.reg(2,4,0),s.reg(1,1,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daa10u); s.flags.logic((s.reg(2,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x1daa13u); if(s.flags.zf)goto L1daa1c;
if(s.services.instruction)s.services.instruction(0x1daa15u); s.push(0x1daa15u,0x1daa1au);run_e1480(s);
if(s.services.instruction)s.services.instruction(0x1daa1au); goto L1daa26;
L1daa1c: if(s.services.instruction)s.services.instruction(0x1daa1cu); s.flags.logic((s.reg(2,1,8))&(0x10u),1);
if(s.services.instruction)s.services.instruction(0x1daa1fu); if(s.flags.zf)goto L1daa26;
if(s.services.instruction)s.services.instruction(0x1daa21u); s.push(0x1daa21u,0x1daa26u);run_11b930(s);
L1daa26: if(s.services.instruction)s.services.instruction(0x1daa26u); s.set(0,r(s.memory,(0x64u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daa2au); s.set(6,sign_extend(r(s.memory,(0x1u+s.gp[0]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x1daa2eu); s.set(2,r(s.memory,(0x70u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daa31u); s.set(1,(0x0u+s.gp[6]+s.gp[6]*2u),4,0);
if(s.services.instruction)s.services.instruction(0x1daa34u); s.set(1,s.flags.shift(s.reg(1,4,0),0x5u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1daa37u); s.set(0,r(s.memory,(0x40u+s.gp[1]+s.gp[2]*1u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daa3bu); s.set(0,r(s.memory,(0x3cu+s.gp[0]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daa3eu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x70u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x1daa43u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x74u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x1daa48u); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x78u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x1daa4du); s.set(0,s.flags.plus(s.reg(0,4,0),0x70u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1daa50u); s.set(1,(0x68u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1daa54u); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1daa56u); s.set(2,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1daa58u); w(s.memory,s.observe,0x1daa58u,(0xf0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x1daa61u); w(s.memory,s.observe,0x1daa61u,(0xf4u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x1daa6au); w(s.memory,s.observe,0x1daa6au,(0xf8u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[4])));
if(s.services.instruction)s.services.instruction(0x1daa73u); {const auto target=0x1d8d80u;s.push(0x1daa73u,0x1daa78u);s.required(0x1daa73u,target,false);}
if(s.services.instruction)s.services.instruction(0x1daa78u); s.set(1,(0x44u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1daa7cu); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1daa7eu); s.set(2,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1daa80u); {const auto target=0x1d8c80u;s.push(0x1daa80u,0x1daa85u);s.required(0x1daa80u,target,false);}
if(s.services.instruction)s.services.instruction(0x1daa85u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x20u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1daa8au); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1daa8du); s.flags.scalar_compare(s.xmm[1],s.xmm[0],true);
if(s.services.instruction)s.services.instruction(0x1daa90u); s.set(0,s.flags.bits()|2u,1,8);
if(s.services.instruction)s.services.instruction(0x1daa91u); s.flags.logic((s.reg(0,1,8))&(0x44u),1);
if(s.services.instruction)s.services.instruction(0x1daa94u); if(!s.flags.pf)goto L1dae3e;
if(s.services.instruction)s.services.instruction(0x1daa9au); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x24u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1daa9fu); s.flags.scalar_compare(s.xmm[1],s.xmm[0],true);
if(s.services.instruction)s.services.instruction(0x1daaa2u); s.set(0,s.flags.bits()|2u,1,8);
if(s.services.instruction)s.services.instruction(0x1daaa3u); s.flags.logic((s.reg(0,1,8))&(0x44u),1);
if(s.services.instruction)s.services.instruction(0x1daaa6u); if(!s.flags.pf)goto L1dae3e;
if(s.services.instruction)s.services.instruction(0x1daaacu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x16cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1daab5u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x1daab9u); w(s.memory,s.observe,0x1daab9u,(0xe0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1daac2u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x170u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1daacbu); s.set(1,r(s.memory,(0xe0u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daad2u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1daad6u); w(s.memory,s.observe,0x1daad6u,(0xe4u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1daadfu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x174u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1daae8u); s.set(2,r(s.memory,(0xe4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daaefu); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1daaf3u); w(s.memory,s.observe,0x1daaf3u,(0xe8u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1daafcu); s.set(0,r(s.memory,(0xe8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dab03u); w(s.memory,s.observe,0x1dab03u,(0x7cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1dab07u); s.set(0,(0x74u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1dab0bu); w(s.memory,s.observe,0x1dab0bu,(0x74u+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1dab0fu); w(s.memory,s.observe,0x1dab0fu,(0x78u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1dab13u); {const auto target=0x30bd0u;s.push(0x1dab13u,0x1dab18u);s.required(0x1dab13u,target,false);}
if(s.services.instruction)s.services.instruction(0x1dab18u); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x46a0e8u),4))),true);
if(s.services.instruction)s.services.instruction(0x1dab1fu); s.set(0,s.flags.bits()|2u,1,8);
if(s.services.instruction)s.services.instruction(0x1dab20u); s.flags.logic((s.reg(0,1,8))&(0x44u),1);
if(s.services.instruction)s.services.instruction(0x1dab23u); w(s.memory,s.observe,0x1dab23u,(0xbcu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1dab2cu); if(s.flags.pf)goto L1dab48;
if(s.services.instruction)s.services.instruction(0x1dab2eu); s.set(1,r(s.memory,(0x474d10u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dab34u); s.set(2,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dab36u); s.set(0,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dab39u); s.set(1,r(s.memory,(0x8u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dab3cu); w(s.memory,s.observe,0x1dab3cu,(0x74u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1dab40u); w(s.memory,s.observe,0x1dab40u,(0x78u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1dab44u); w(s.memory,s.observe,0x1dab44u,(0x7cu+s.gp[4]),4,s.reg(1,4,0));
L1dab48: if(s.services.instruction)s.services.instruction(0x1dab48u); s.fp.push(ext(f(r(s.memory,(0x4cu+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1dab4cu); s.set(0,r(s.memory,(0x83u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dab53u); s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x7cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1dab57u); s.set(1,r(s.memory,(0x18u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dab5au); s.fp.push(ext(f(r(s.memory,(0x48u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1dab5eu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x1ecu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dab67u); s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x78u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1dab6bu); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x20u+s.gp[5]),4))));
if(s.services.instruction)s.services.instruction(0x1dab70u); s.set(2,(0x108u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1dab77u); s.push(0x1dab77u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1dab78u); s.set(2,r(s.memory,(0x1cu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dab7bu); s.fp.st(1)=add(s.fp.st(1),s.fp.st(0));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1dab7du); s.fp.push(ext(f(r(s.memory,(0x48u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1dab81u); s.push(0x1dab81u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1dab82u); s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x7cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1dab86u); s.set(0,r(s.memory,(0x24u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dab89u); s.push(0x1dab89u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1dab8au); s.push(0x1dab8au,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1dab8bu); s.set(2,r(s.memory,(0xccu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dab92u); s.fp.st(1)=add(s.fp.st(1),s.fp.st(0));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1dab94u); s.push(0x1dab94u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1dab95u); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1dab97u); w(s.memory,s.observe,0x1dab97u,(0x28u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1dab9bu); s.set(1,r(s.memory,(0x28u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dab9fu); s.push(0x1dab9fu,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1daba0u); s.push(0x1daba0u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1daba1u); w(s.memory,s.observe,0x1daba1u,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1daba6u); s.push(0x1daba6u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1daba7u); s.set(1,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1daba9u); {const auto target=0x1d8e60u;s.push(0x1daba9u,0x1dabaeu);s.required(0x1daba9u,target,false);}
if(s.services.instruction)s.services.instruction(0x1dabaeu); s.set(0,r(s.memory,(0x80u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dabb5u); s.push(0x1dabb5u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1dabb6u); w(s.memory,s.observe,0x1dabb6u,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1dabbbu); s.push(0x1dabbbu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1dabbcu); s.set(3,0x1u,1,0);
if(s.services.instruction)s.services.instruction(0x1dabbeu); native_lift_scalar_call(s,0x1dabbeu,0x1dabc3u);
if(s.services.instruction)s.services.instruction(0x1dabc3u); s.set(1,r(s.memory,(0x51df54u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dabc9u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x74u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dabcfu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x78u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dabd5u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x7cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dabdbu); s.set(0,r(s.memory,(0x19u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1dabdfu); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1dabe1u); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[1]),4)));
if(s.services.instruction)s.services.instruction(0x1dabe6u); s.xmm[5]=_mm_mul_ss(s.xmm[5],_mm_set_ss(f(r(s.memory,(0x20u+s.gp[5]),4))));
if(s.services.instruction)s.services.instruction(0x1dabebu); s.xmm[6]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x1dabeeu); s.xmm[7]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x1dabf1u); s.xmm[4]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x1dabf4u); s.xmm[6]=_mm_mul_ss(s.xmm[6],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dabf8u); s.xmm[7]=_mm_mul_ss(s.xmm[7],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dabfcu); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dac00u); w(s.memory,s.observe,0x1dac00u,(0xdcu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x1dac09u); w(s.memory,s.observe,0x1dac09u,(0x30u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[6])));
if(s.services.instruction)s.services.instruction(0x1dac0fu); w(s.memory,s.observe,0x1dac0fu,(0x34u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[7])));
if(s.services.instruction)s.services.instruction(0x1dac15u); w(s.memory,s.observe,0x1dac15u,(0x38u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[4])));
if(s.services.instruction)s.services.instruction(0x1dac1bu); if(!s.flags.zf)goto L1dac52;
if(s.services.instruction)s.services.instruction(0x1dac1du); s.flags.scalar_compare(s.xmm[5],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x1dac20u); if(s.flags.cf||s.flags.zf)goto L1dac52;
if(s.services.instruction)s.services.instruction(0x1dac22u); s.set(2,r(s.memory,(0x52be68u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1dac28u); s.xmm[7]=_mm_set_ss(f(r(s.memory,(0x0u+s.gp[2]),4)));
if(s.services.instruction)s.services.instruction(0x1dac2cu); s.set(0,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1dac2eu); s.xmm[7]=_mm_mul_ss(s.xmm[7],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x1dac33u); s.xmm[5]=_mm_sub_ss(s.xmm[5],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dac37u); s.flags.scalar_compare(s.xmm[7],s.xmm[5],false);
if(s.services.instruction)s.services.instruction(0x1dac3au); if(!s.flags.cf&&!s.flags.zf)goto L1dac3f;
if(s.services.instruction)s.services.instruction(0x1dac3cu); s.xmm[5]=s.xmm[7];
L1dac3f: if(s.services.instruction)s.services.instruction(0x1dac3fu); s.xmm[7]=_mm_set_ss(f(r(s.memory,(0x34u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dac45u); s.xmm[5]=_mm_add_ss(s.xmm[5],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1dac49u); w(s.memory,s.observe,0x1dac49u,(0x38u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[5])));
if(s.services.instruction)s.services.instruction(0x1dac4fu); s.xmm[4]=s.xmm[5];
L1dac52: if(s.services.instruction)s.services.instruction(0x1dac52u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0xbcu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dac5bu); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x1cu+s.gp[5]),4))));
if(s.services.instruction)s.services.instruction(0x1dac60u); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x46a0e8u),4))),false);
if(s.services.instruction)s.services.instruction(0x1dac67u); if(s.flags.cf)goto L1dadf0;
if(s.services.instruction)s.services.instruction(0x1dac6du); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dac73u); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dac77u); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dac7bu); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dac7fu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x44u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dac85u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1dac89u); w(s.memory,s.observe,0x1dac89u,(0xb0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1dac92u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x48u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dac98u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x1dac9cu); w(s.memory,s.observe,0x1dac9cu,(0xb4u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1daca5u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dacabu); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1dacafu); s.set(0,(0xb0u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1dacb6u); w(s.memory,s.observe,0x1dacb6u,(0xb8u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1dacbfu); s.push(0x1dacbfu,0x1dacc4u);run_24570(s);
if(s.services.instruction)s.services.instruction(0x1dacc4u); w(s.memory,s.observe,0x1dacc4u,(0x14u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1dacc8u); s.fp.push(ext(f(r(s.memory,(0x46a0e8u),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1dacceu); s.xmm[1]=_mm_xor_ps(s.xmm[1],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1dacd1u); s.fp.push(ext(f(r(s.memory,(0x14u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1dacd5u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dacdbu); s.compare_and_pop(s.fp.st(1));
if(s.services.instruction)s.services.instruction(0x1dacddu); s.fp.st(0)=s.fp.st(0);s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1dacdfu); if(s.flags.cf)goto L1dace6;
if(s.services.instruction)s.services.instruction(0x1dace1u); s.xmm[2]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1dace4u); goto L1daced;
L1dace6: if(s.services.instruction)s.services.instruction(0x1dace6u); s.xmm[2]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x1dace9u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[0]);
L1daced: if(s.services.instruction)s.services.instruction(0x1dacedu); s.flags.scalar_compare(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x46a180u),4))),false);
if(s.services.instruction)s.services.instruction(0x1dacf4u); if(s.flags.cf||s.flags.zf)goto L1dadbf;
if(s.services.instruction)s.services.instruction(0x1dacfau); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0xdcu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dad03u); s.flags.scalar_compare(s.xmm[0],s.xmm[2],false);
if(s.services.instruction)s.services.instruction(0x1dad06u); if(s.flags.cf||s.flags.zf)goto L1dad80;
if(s.services.instruction)s.services.instruction(0x1dad08u); s.flags.scalar_compare(s.xmm[2],s.xmm[1],false);
if(s.services.instruction)s.services.instruction(0x1dad0bu); if(s.flags.cf)goto L1dad12;
if(s.services.instruction)s.services.instruction(0x1dad0du); s.xmm[1]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x1dad10u); goto L1dad16;
L1dad12: if(s.services.instruction)s.services.instruction(0x1dad12u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[2]);
L1dad16: if(s.services.instruction)s.services.instruction(0x1dad16u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0xb8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dad1fu); s.xmm[2]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1dad22u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1dad26u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0xb0u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dad2fu); s.xmm[2]=_mm_div_ss(s.xmm[2],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dad33u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x1dad3bu); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x1dad3fu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0xb4u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dad48u); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dad4cu); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dad50u); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dad54u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x30u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dad5au); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1dad5eu); w(s.memory,s.observe,0x1dad5eu,(0x30u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1dad64u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x34u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dad6au); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x1dad6eu); w(s.memory,s.observe,0x1dad6eu,(0x34u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1dad74u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x38u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dad7au); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1dad7eu); goto L1dadb9;
L1dad80: if(s.services.instruction)s.services.instruction(0x1dad80u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x30u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dad86u); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0xb0u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1dad8fu); w(s.memory,s.observe,0x1dad8fu,(0x30u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1dad95u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x34u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dad9bu); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0xb4u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1dada4u); w(s.memory,s.observe,0x1dada4u,(0x34u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1dadaau); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x38u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dadb0u); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0xb8u+s.gp[4]),4))));
L1dadb9: if(s.services.instruction)s.services.instruction(0x1dadb9u); w(s.memory,s.observe,0x1dadb9u,(0x38u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
L1dadbf: if(s.services.instruction)s.services.instruction(0x1dadbfu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x108u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dadc8u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x1cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dadceu); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x1dadd1u); if(!s.flags.cf&&!s.flags.zf)goto L1dadd9;
if(s.services.instruction)s.services.instruction(0x1dadd3u); w(s.memory,s.observe,0x1dadd3u,(0x1cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
L1dadd9: if(s.services.instruction)s.services.instruction(0x1dadd9u); s.xmm[6]=_mm_set_ss(f(r(s.memory,(0x30u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1daddfu); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x38u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dade5u); s.xmm[7]=_mm_set_ss(f(r(s.memory,(0x34u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dadebu); w(s.memory,s.observe,0x1dadebu,(0x13u+s.gp[4]),1,0x1u);
L1dadf0: if(s.services.instruction)s.services.instruction(0x1dadf0u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x24u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dadf6u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0xf0u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dadffu); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0xf4u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dae08u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[6]);
if(s.services.instruction)s.services.instruction(0x1dae0cu); w(s.memory,s.observe,0x1dae0cu,(0x24u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1dae12u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x28u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dae18u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[7]);
if(s.services.instruction)s.services.instruction(0x1dae1cu); w(s.memory,s.observe,0x1dae1cu,(0x28u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1dae22u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x2cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dae28u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1dae2cu); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0xf8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dae35u); w(s.memory,s.observe,0x1dae35u,(0x2cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1dae3bu); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
L1dae3e: if(s.services.instruction)s.services.instruction(0x1dae3eu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x28u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1dae43u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],true);
if(s.services.instruction)s.services.instruction(0x1dae46u); s.set(0,s.flags.bits()|2u,1,8);
if(s.services.instruction)s.services.instruction(0x1dae47u); s.flags.logic((s.reg(0,1,8))&(0x44u),1);
if(s.services.instruction)s.services.instruction(0x1dae4au); if(!s.flags.pf)goto L1db221;
if(s.services.instruction)s.services.instruction(0x1dae50u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x2cu+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1dae55u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],true);
if(s.services.instruction)s.services.instruction(0x1dae58u); s.set(0,s.flags.bits()|2u,1,8);
if(s.services.instruction)s.services.instruction(0x1dae59u); s.flags.logic((s.reg(0,1,8))&(0x44u),1);
if(s.services.instruction)s.services.instruction(0x1dae5cu); if(!s.flags.pf)goto L1db221;
if(s.services.instruction)s.services.instruction(0x1dae62u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x16cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dae6bu); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x148u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dae74u); s.xmm[7]=_mm_set_ss(f(r(s.memory,(0x14cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dae7du); s.xmm[6]=_mm_set_ss(f(r(s.memory,(0x150u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dae86u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x1dae8au); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x170u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dae93u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1dae97u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x174u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1daea0u); s.xmm[3]=_mm_sub_ss(s.xmm[3],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1daea4u); s.xmm[0]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x1daea7u); s.xmm[4]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x1daeaau); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[5]);
if(s.services.instruction)s.services.instruction(0x1daeaeu); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[7]);
if(s.services.instruction)s.services.instruction(0x1daeb2u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1daeb6u); s.xmm[4]=s.xmm[6];
if(s.services.instruction)s.services.instruction(0x1daeb9u); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1daebdu); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1daec1u); s.xmm[4]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1daec4u); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[5]);
if(s.services.instruction)s.services.instruction(0x1daec8u); s.xmm[5]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1daecbu); s.xmm[5]=_mm_mul_ss(s.xmm[5],s.xmm[7]);
if(s.services.instruction)s.services.instruction(0x1daecfu); s.xmm[6]=_mm_mul_ss(s.xmm[6],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1daed3u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1daed7u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[5]);
if(s.services.instruction)s.services.instruction(0x1daedbu); s.xmm[3]=_mm_sub_ss(s.xmm[3],s.xmm[6]);
if(s.services.instruction)s.services.instruction(0x1daedfu); s.set(0,(0x98u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1daee6u); w(s.memory,s.observe,0x1daee6u,(0x98u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1daeefu); w(s.memory,s.observe,0x1daeefu,(0x9cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x1daef8u); w(s.memory,s.observe,0x1daef8u,(0xa0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x1daf01u); {const auto target=0x30bd0u;s.push(0x1daf01u,0x1daf06u);s.required(0x1daf01u,target,false);}
if(s.services.instruction)s.services.instruction(0x1daf06u); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x46a0e8u),4))),true);
if(s.services.instruction)s.services.instruction(0x1daf0du); s.set(0,s.flags.bits()|2u,1,8);
if(s.services.instruction)s.services.instruction(0x1daf0eu); s.flags.logic((s.reg(0,1,8))&(0x44u),1);
if(s.services.instruction)s.services.instruction(0x1daf11u); w(s.memory,s.observe,0x1daf11u,(0x40u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1daf17u); if(!s.flags.pf)goto L1db21e;
if(s.services.instruction)s.services.instruction(0x1daf1du); s.fp.push(ext(f(r(s.memory,(0xa0u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1daf24u); s.set(2,r(s.memory,(0x83u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daf2bu); s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x4cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1daf2fu); s.set(0,r(s.memory,(0x18u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daf32u); s.fp.push(ext(f(r(s.memory,(0x9cu+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1daf39u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x28u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1daf3eu); s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x48u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1daf42u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x1ecu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1daf4bu); s.set(1,(0xecu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1daf52u); s.push(0x1daf52u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1daf53u); s.set(1,r(s.memory,(0x1cu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daf56u); s.fp.st(1)=add(s.fp.st(1),s.fp.st(0));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1daf58u); s.fp.push(ext(f(r(s.memory,(0x9cu+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1daf5fu); s.push(0x1daf5fu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1daf60u); s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x4cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1daf64u); s.set(2,r(s.memory,(0x2cu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daf67u); s.push(0x1daf67u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1daf68u); s.push(0x1daf68u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1daf69u); s.fp.st(1)=add(s.fp.st(1),s.fp.st(0));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1daf6bu); s.push(0x1daf6bu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1daf6cu); w(s.memory,s.observe,0x1daf6cu,(0x28u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1daf70u); s.set(0,r(s.memory,(0x28u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daf74u); s.push(0x1daf74u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1daf75u); s.push(0x1daf75u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1daf76u); s.set(1,r(s.memory,(0x5cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daf7au); w(s.memory,s.observe,0x1daf7au,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1daf7fu); s.push(0x1daf7fu,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1daf80u); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1daf82u); s.set(1,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1daf84u); {const auto target=0x1d8e60u;s.push(0x1daf84u,0x1daf89u);s.required(0x1daf84u,target,false);}
if(s.services.instruction)s.services.instruction(0x1daf89u); s.set(2,r(s.memory,(0x80u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1daf90u); s.push(0x1daf90u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1daf91u); w(s.memory,s.observe,0x1daf91u,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1daf96u); s.push(0x1daf96u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1daf97u); s.set(3,0x1u,1,0);
if(s.services.instruction)s.services.instruction(0x1daf99u); native_lift_scalar_call(s,0x1daf99u,0x1daf9eu);
if(s.services.instruction)s.services.instruction(0x1daf9eu); s.xmm[6]=_mm_set_ss(f(r(s.memory,(0x98u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dafa7u); s.xmm[7]=_mm_set_ss(f(r(s.memory,(0x9cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dafb0u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0xa0u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dafb9u); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dafbdu); s.xmm[1]=s.xmm[6];
if(s.services.instruction)s.services.instruction(0x1dafc0u); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dafc4u); s.xmm[2]=s.xmm[7];
if(s.services.instruction)s.services.instruction(0x1dafc7u); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1dafcbu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x40u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1dafd1u); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x1cu+s.gp[5]),4))));
if(s.services.instruction)s.services.instruction(0x1dafd6u); s.flags.scalar_compare(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x46a0e8u),4))),false);
if(s.services.instruction)s.services.instruction(0x1dafddu); w(s.memory,s.observe,0x1dafddu,(0x50u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1dafe3u); w(s.memory,s.observe,0x1dafe3u,(0x54u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x1dafe9u); w(s.memory,s.observe,0x1dafe9u,(0x58u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x1dafefu); if(s.flags.cf)goto L1db1fa;
if(s.services.instruction)s.services.instruction(0x1daff5u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x14cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1daffeu); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x148u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db007u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x48u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db00du); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x44u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db013u); s.xmm[5]=_mm_set_ss(f(r(s.memory,(0x150u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db01cu); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1db020u); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1db024u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x1db028u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x4cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db02eu); s.xmm[3]=s.xmm[5];
if(s.services.instruction)s.services.instruction(0x1db031u); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x1db035u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1db039u); s.xmm[3]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1db03cu); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1db040u); s.xmm[4]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1db043u); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1db047u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x48u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db04du); s.xmm[5]=_mm_mul_ss(s.xmm[5],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db051u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x44u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db057u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1db05bu); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0xa0u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db064u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1db068u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db06eu); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[5]);
if(s.services.instruction)s.services.instruction(0x1db072u); s.xmm[6]=_mm_mul_ss(s.xmm[6],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1db076u); s.xmm[7]=_mm_mul_ss(s.xmm[7],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1db07au); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1db07eu); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[6]);
if(s.services.instruction)s.services.instruction(0x1db082u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[7]);
if(s.services.instruction)s.services.instruction(0x1db086u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[4]);
if(s.services.instruction)s.services.instruction(0x1db08au); s.set(0,(0xa4u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db091u); w(s.memory,s.observe,0x1db091u,(0xa4u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db09au); w(s.memory,s.observe,0x1db09au,(0xa8u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1db0a3u); w(s.memory,s.observe,0x1db0a3u,(0xacu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x1db0acu); s.push(0x1db0acu,0x1db0b1u);run_24570(s);
if(s.services.instruction)s.services.instruction(0x1db0b1u); w(s.memory,s.observe,0x1db0b1u,(0x14u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1db0b5u); s.fp.push(ext(f(r(s.memory,(0x46a0e8u),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1db0bbu); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db0beu); s.fp.push(ext(f(r(s.memory,(0x14u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1db0c2u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db0c8u); s.compare_and_pop(s.fp.st(1));
if(s.services.instruction)s.services.instruction(0x1db0cau); s.fp.st(0)=s.fp.st(0);s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1db0ccu); if(s.flags.cf)goto L1db0d3;
if(s.services.instruction)s.services.instruction(0x1db0ceu); s.xmm[2]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x1db0d1u); goto L1db0da;
L1db0d3: if(s.services.instruction)s.services.instruction(0x1db0d3u); s.xmm[2]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1db0d6u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[1]);
L1db0da: if(s.services.instruction)s.services.instruction(0x1db0dau); s.flags.scalar_compare(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x46a180u),4))),false);
if(s.services.instruction)s.services.instruction(0x1db0e1u); if(s.flags.cf||s.flags.zf)goto L1db1c9;
if(s.services.instruction)s.services.instruction(0x1db0e7u); s.set(0,r(s.memory,(0x51df54u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db0ecu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x28u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1db0f1u); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x1db0f6u); s.flags.scalar_compare(s.xmm[1],s.xmm[2],false);
if(s.services.instruction)s.services.instruction(0x1db0f9u); if(s.flags.cf||s.flags.zf)goto L1db18a;
if(s.services.instruction)s.services.instruction(0x1db0ffu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x28u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1db104u); s.flags.scalar_compare(s.xmm[2],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x1db107u); if(s.flags.cf)goto L1db10e;
if(s.services.instruction)s.services.instruction(0x1db109u); s.xmm[0]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x1db10cu); goto L1db113;
L1db10e: if(s.services.instruction)s.services.instruction(0x1db10eu); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x28u+s.gp[5]),4))));
L1db113: if(s.services.instruction)s.services.instruction(0x1db113u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4))));
if(s.services.instruction)s.services.instruction(0x1db118u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0xacu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db121u); s.xmm[2]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x1db124u); s.xmm[2]=_mm_sub_ss(s.xmm[2],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db128u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x1db130u); s.xmm[2]=_mm_div_ss(s.xmm[2],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1db134u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x1db138u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0xa8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db141u); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db145u); s.xmm[1]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1db148u); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0xa4u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db151u); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db155u); s.xmm[0]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x1db158u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x50u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db15eu); s.xmm[3]=_mm_sub_ss(s.xmm[3],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1db162u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x54u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db168u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x1db16cu); w(s.memory,s.observe,0x1db16cu,(0x54u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1db172u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x58u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db178u); s.xmm[1]=_mm_sub_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db17cu); w(s.memory,s.observe,0x1db17cu,(0x50u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x1db182u); w(s.memory,s.observe,0x1db182u,(0x58u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1db188u); goto L1db1c9;
L1db18a: if(s.services.instruction)s.services.instruction(0x1db18au); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x50u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db190u); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0xa4u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db199u); w(s.memory,s.observe,0x1db199u,(0x50u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db19fu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x54u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db1a5u); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0xa8u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db1aeu); w(s.memory,s.observe,0x1db1aeu,(0x54u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db1b4u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x58u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db1bau); s.xmm[0]=_mm_sub_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0xacu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db1c3u); w(s.memory,s.observe,0x1db1c3u,(0x58u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
L1db1c9: if(s.services.instruction)s.services.instruction(0x1db1c9u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0xecu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db1d2u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x1cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db1d8u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x1db1dbu); if(!s.flags.cf&&!s.flags.zf)goto L1db1e3;
if(s.services.instruction)s.services.instruction(0x1db1ddu); w(s.memory,s.observe,0x1db1ddu,(0x1cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
L1db1e3: if(s.services.instruction)s.services.instruction(0x1db1e3u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x58u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db1e9u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x54u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db1efu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x50u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db1f5u); w(s.memory,s.observe,0x1db1f5u,(0x13u+s.gp[4]),1,0x1u);
L1db1fa: if(s.services.instruction)s.services.instruction(0x1db1fau); s.xmm[1]=_mm_add_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x24u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db200u); s.xmm[2]=_mm_add_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x28u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db206u); s.xmm[3]=_mm_add_ss(s.xmm[3],_mm_set_ss(f(r(s.memory,(0x2cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db20cu); w(s.memory,s.observe,0x1db20cu,(0x24u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1db212u); w(s.memory,s.observe,0x1db212u,(0x28u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x1db218u); w(s.memory,s.observe,0x1db218u,(0x2cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[3])));
L1db21e: if(s.services.instruction)s.services.instruction(0x1db21eu); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
L1db221: if(s.services.instruction)s.services.instruction(0x1db221u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x30u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1db226u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],true);
if(s.services.instruction)s.services.instruction(0x1db229u); s.set(0,s.flags.bits()|2u,1,8);
if(s.services.instruction)s.services.instruction(0x1db22au); s.flags.logic((s.reg(0,1,8))&(0x44u),1);
if(s.services.instruction)s.services.instruction(0x1db22du); if(!s.flags.pf)goto L1db340;
if(s.services.instruction)s.services.instruction(0x1db233u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x34u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1db238u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],true);
if(s.services.instruction)s.services.instruction(0x1db23bu); s.set(0,s.flags.bits()|2u,1,8);
if(s.services.instruction)s.services.instruction(0x1db23cu); s.flags.logic((s.reg(0,1,8))&(0x44u),1);
if(s.services.instruction)s.services.instruction(0x1db23fu); if(!s.flags.pf)goto L1db340;
if(s.services.instruction)s.services.instruction(0x1db245u); s.set(1,r(s.memory,(0x83u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db24cu); s.set(2,r(s.memory,(0x18u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db24fu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x44u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db255u); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x148u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db25eu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x48u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db264u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x14cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db26du); s.set(0,(0xd8u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db274u); s.push(0x1db274u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1db275u); s.set(0,r(s.memory,(0x1cu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db278u); s.push(0x1db278u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db279u); s.set(1,r(s.memory,(0x34u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db27cu); s.push(0x1db27cu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1db27du); s.push(0x1db27du,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1db27eu); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1db282u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x160u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db28bu); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x5cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db291u); s.push(0x1db291u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db292u); s.set(4,s.flags.minus(s.reg(4,4,0),0x8u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1db295u); s.xmm[0]=_mm_add_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1db299u); w(s.memory,s.observe,0x1db299u,(0x4u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db29fu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x30u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1db2a4u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x208u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db2adu); w(s.memory,s.observe,0x1db2adu,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db2b2u); s.push(0x1db2b2u,0x461c4000u);
if(s.services.instruction)s.services.instruction(0x1db2b7u); s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1db2b9u); s.set(1,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1db2bbu); {const auto target=0x1d8e60u;s.push(0x1db2bbu,0x1db2c0u);s.required(0x1db2bbu,target,false);}
if(s.services.instruction)s.services.instruction(0x1db2c0u); s.set(2,r(s.memory,(0x80u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db2c7u); s.push(0x1db2c7u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db2c8u); w(s.memory,s.observe,0x1db2c8u,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db2cdu); s.push(0x1db2cdu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1db2ceu); s.set(3,0x1u,1,0);
if(s.services.instruction)s.services.instruction(0x1db2d0u); native_lift_scalar_call(s,0x1db2d0u,0x1db2d5u);
if(s.services.instruction)s.services.instruction(0x1db2d5u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x150u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db2deu); s.xmm[1]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1db2e1u); s.xmm[1]=_mm_mul_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x148u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db2eau); s.xmm[1]=_mm_add_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x24u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db2f0u); s.xmm[2]=s.xmm[0];
if(s.services.instruction)s.services.instruction(0x1db2f3u); s.xmm[2]=_mm_mul_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x14cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db2fcu); s.xmm[2]=_mm_add_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x28u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db302u); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db306u); s.xmm[0]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x1db309u); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x2cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db30fu); w(s.memory,s.observe,0x1db30fu,(0x24u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1db315u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x1cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db31bu); w(s.memory,s.observe,0x1db31bu,(0x2cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db321u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0xd8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db32au); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x1db32du); w(s.memory,s.observe,0x1db32du,(0x28u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x1db333u); if(!s.flags.cf&&!s.flags.zf)goto L1db33b;
if(s.services.instruction)s.services.instruction(0x1db335u); w(s.memory,s.observe,0x1db335u,(0x1cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
L1db33b: if(s.services.instruction)s.services.instruction(0x1db33bu); w(s.memory,s.observe,0x1db33bu,(0x13u+s.gp[4]),1,0x1u);
L1db340: if(s.services.instruction)s.services.instruction(0x1db340u); s.set(0,r(s.memory,(0x19u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1db344u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1db346u); if(s.flags.zf)goto L1db368;
if(s.services.instruction)s.services.instruction(0x1db348u); s.set(0,r(s.memory,(0x51df54u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db34du); s.set(1,r(s.memory,(0x52be68u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db353u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x1db358u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x0u+s.gp[1]),4))));
if(s.services.instruction)s.services.instruction(0x1db35cu); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x2cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db362u); w(s.memory,s.observe,0x1db362u,(0x2cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
L1db368: if(s.services.instruction)s.services.instruction(0x1db368u); s.set(1,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1db36au); s.set(2,(0x24u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db36eu); s.set(6,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1db370u); s.push(0x1db370u,0x1db375u);run_1d92c0(s);
if(s.services.instruction)s.services.instruction(0x1db375u); s.set(2,r(s.memory,(0x4u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db378u); s.set(2,s.flags.right(s.reg(2,4,0),0x1u,4,false),4,0);
if(s.services.instruction)s.services.instruction(0x1db37au); s.flags.logic((s.reg(2,1,0))&(0x1u),1);
if(s.services.instruction)s.services.instruction(0x1db37du); if(!s.flags.zf)goto L1db676;
if(s.services.instruction)s.services.instruction(0x1db383u); s.set(0,r(s.memory,(0xcu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db386u); s.flags.minus(s.reg(0,4,0),0x7000001u,4);
if(s.services.instruction)s.services.instruction(0x1db38bu); if(s.flags.zf)goto L1db3a9;
if(s.services.instruction)s.services.instruction(0x1db38du); s.set(1,r(s.memory,(0x1e8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db394u); s.set(1,r(s.memory,(0x8u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db397u); s.push(0x1db397u,0x1u);
if(s.services.instruction)s.services.instruction(0x1db399u); s.push(0x1db399u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1db39au); s.push(0x1db39au,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db39bu); s.set(1,s.flags.logic((s.reg(1,1,0))^(s.reg(1,1,0)),1),1,0);
if(s.services.instruction)s.services.instruction(0x1db39du); s.set(3,(0x118u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db3a4u); {const auto target=0xbad60u;s.push(0x1db3a4u,0x1db3a9u);s.required(0x1db3a4u,target,false);}
L1db3a9: if(s.services.instruction)s.services.instruction(0x1db3a9u); s.set(2,r(s.memory,(0x474d04u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db3afu); s.set(0,r(s.memory,(0x0u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db3b1u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x58u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1db3b6u); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db3b9u); s.set(1,r(s.memory,(0x4u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db3bcu); s.set(2,r(s.memory,(0x8u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db3bfu); s.flags.scalar_compare(s.xmm[1],s.xmm[0],true);
if(s.services.instruction)s.services.instruction(0x1db3c2u); w(s.memory,s.observe,0x1db3c2u,(0x88u+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1db3c9u); s.set(0,s.flags.bits()|2u,1,8);
if(s.services.instruction)s.services.instruction(0x1db3cau); s.flags.logic((s.reg(0,1,8))&(0x44u),1);
if(s.services.instruction)s.services.instruction(0x1db3cdu); w(s.memory,s.observe,0x1db3cdu,(0x8cu+s.gp[4]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db3d4u); w(s.memory,s.observe,0x1db3d4u,(0x90u+s.gp[4]),4,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1db3dbu); if(!s.flags.pf)goto L1db603;
if(s.services.instruction)s.services.instruction(0x1db3e1u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x5cu+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1db3e6u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],true);
if(s.services.instruction)s.services.instruction(0x1db3e9u); s.set(0,s.flags.bits()|2u,1,8);
if(s.services.instruction)s.services.instruction(0x1db3eau); s.flags.logic((s.reg(0,1,8))&(0x44u),1);
if(s.services.instruction)s.services.instruction(0x1db3edu); if(!s.flags.pf)goto L1db603;
if(s.services.instruction)s.services.instruction(0x1db3f3u); s.set(0,r(s.memory,(0x64u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db3f7u); s.set(0,sign_extend(r(s.memory,(0x1u+s.gp[0]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x1db3fbu); s.set(1,r(s.memory,(0x20u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db3ffu); s.set(6,(0x18cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db406u); {const auto target=0x1d8b90u;s.push(0x1db406u,0x1db40bu);s.required(0x1db406u,target,false);}
if(s.services.instruction)s.services.instruction(0x1db40bu); s.set(0,(0x190u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db412u); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1db414u); {const auto target=0x145680u;s.push(0x1db414u,0x1db419u);s.required(0x1db414u,target,false);}
if(s.services.instruction)s.services.instruction(0x1db419u); s.set(0,(0x1c0u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db420u); s.set(1,(0x190u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db427u); s.set(2,(0x148u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db42eu); {const auto target=0x1459b0u;s.push(0x1db42eu,0x1db433u);s.required(0x1db42eu,target,false);}
if(s.services.instruction)s.services.instruction(0x1db433u); s.set(3,(0x17cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db43au); s.set(1,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1db43cu); {const auto target=0x144a60u;s.push(0x1db43cu,0x1db441u);s.required(0x1db43cu,target,false);}
if(s.services.instruction)s.services.instruction(0x1db441u); s.set(6,(0xccu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db448u); s.set(7,(0x104u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db44fu); s.set(0,s.reg(3,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x1db451u); {const auto target=0x120150u;s.push(0x1db451u,0x1db456u);s.required(0x1db451u,target,false);}
if(s.services.instruction)s.services.instruction(0x1db456u); s.fp.push(ext(f(r(s.memory,(0xccu+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1db45du); s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x68u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db461u); s.set(2,r(s.memory,(0xfcu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db468u); s.fp.push(ext(f(r(s.memory,(0xd4u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1db46fu); s.set(0,r(s.memory,(0x54u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db472u); s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x70u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db476u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x58u+s.gp[5]),4)));
if(s.services.instruction)s.services.instruction(0x1db47bu); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x1ecu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db484u); s.set(1,(0x3cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db488u); s.push(0x1db488u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db489u); s.fp.st(1)=add(s.fp.st(1),s.fp.st(0));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1db48bu); s.fp.push(ext(f(r(s.memory,(0xd4u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1db492u); s.set(1,r(s.memory,(0x5cu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db495u); s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,(0x70u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db499u); s.push(0x1db499u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1db49au); s.push(0x1db49au,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1db49bu); s.set(0,r(s.memory,(0x110u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db4a2u); s.fp.st(1)=add(s.fp.st(1),s.fp.st(0));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1db4a4u); s.push(0x1db4a4u,0x0u);
if(s.services.instruction)s.services.instruction(0x1db4a6u); s.push(0x1db4a6u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db4a7u); w(s.memory,s.observe,0x1db4a7u,(0x28u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1db4abu); s.set(2,r(s.memory,(0x28u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db4afu); s.push(0x1db4afu,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1db4b0u); s.push(0x1db4b0u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db4b1u); s.set(1,r(s.memory,(0x80u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db4b8u); w(s.memory,s.observe,0x1db4b8u,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db4bdu); s.push(0x1db4bdu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1db4beu); s.set(0,sign_extend(r(s.memory,(0x1u+s.gp[1]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x1db4c2u); s.set(1,r(s.memory,(0x40u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db4c6u); {const auto target=0x1d8e60u;s.push(0x1db4c6u,0x1db4cbu);s.required(0x1db4c6u,target,false);}
if(s.services.instruction)s.services.instruction(0x1db4cbu); s.set(2,r(s.memory,(0x80u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db4d2u); s.push(0x1db4d2u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db4d3u); w(s.memory,s.observe,0x1db4d3u,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db4d8u); s.push(0x1db4d8u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x1db4d9u); s.set(3,0x1u,1,0);
if(s.services.instruction)s.services.instruction(0x1db4dbu); native_lift_scalar_call(s,0x1db4dbu,0x1db4e0u);
if(s.services.instruction)s.services.instruction(0x1db4e0u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0xccu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db4e9u); s.set(0,r(s.memory,(0x51df54u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db4eeu); s.xmm[2]=s.xmm[1];
if(s.services.instruction)s.services.instruction(0x1db4f1u); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db4f5u); w(s.memory,s.observe,0x1db4f5u,(0x88u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x1db4feu); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0xd0u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db507u); s.xmm[3]=s.xmm[2];
if(s.services.instruction)s.services.instruction(0x1db50au); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db50eu); w(s.memory,s.observe,0x1db50eu,(0x8cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[3])));
if(s.services.instruction)s.services.instruction(0x1db517u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0xd4u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db520u); s.xmm[4]=s.xmm[3];
if(s.services.instruction)s.services.instruction(0x1db523u); s.xmm[4]=_mm_mul_ss(s.xmm[4],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db527u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db52du); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db531u); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db535u); s.xmm[3]=_mm_mul_ss(s.xmm[3],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db539u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x68u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db53fu); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[1]);
if(s.services.instruction)s.services.instruction(0x1db543u); w(s.memory,s.observe,0x1db543u,(0xc0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db54cu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x6cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db552u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[2]);
if(s.services.instruction)s.services.instruction(0x1db556u); w(s.memory,s.observe,0x1db556u,(0xc4u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db55fu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x70u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db565u); s.xmm[0]=_mm_sub_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1db569u); w(s.memory,s.observe,0x1db569u,(0x90u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[4])));
if(s.services.instruction)s.services.instruction(0x1db572u); w(s.memory,s.observe,0x1db572u,(0xc8u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db57bu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x4u+s.gp[0]),4)));
if(s.services.instruction)s.services.instruction(0x1db580u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x58u+s.gp[5]),4))));
if(s.services.instruction)s.services.instruction(0x1db585u); s.push(0x1db585u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db586u); s.set(0,(0xc4u+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db58du); w(s.memory,s.observe,0x1db58du,(0x0u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db592u); {const auto target=0xb17d0u;s.push(0x1db592u,0x1db597u);s.required(0x1db592u,target,false);}
if(s.services.instruction)s.services.instruction(0x1db597u); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x46a0dcu),4)));
if(s.services.instruction)s.services.instruction(0x1db59fu); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0xc0u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db5a8u); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0xc4u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db5b1u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0xc8u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db5bau); s.xmm[4]=_mm_set_ss(f(r(s.memory,(0x1cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db5c0u); s.xmm[0]=_mm_mul_ss(s.xmm[0],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1db5c4u); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x88u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db5cdu); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1db5d1u); s.xmm[1]=_mm_add_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x8cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db5dau); s.xmm[2]=_mm_mul_ss(s.xmm[2],s.xmm[3]);
if(s.services.instruction)s.services.instruction(0x1db5deu); s.xmm[3]=_mm_set_ss(f(r(s.memory,(0x3cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db5e4u); s.flags.scalar_compare(s.xmm[4],s.xmm[3],false);
if(s.services.instruction)s.services.instruction(0x1db5e7u); s.xmm[2]=_mm_add_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x90u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db5f0u); if(!s.flags.cf&&!s.flags.zf)goto L1db5f8;
if(s.services.instruction)s.services.instruction(0x1db5f2u); w(s.memory,s.observe,0x1db5f2u,(0x1cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[3])));
L1db5f8: if(s.services.instruction)s.services.instruction(0x1db5f8u); s.set(7,r(s.memory,(0x20u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db5fcu); w(s.memory,s.observe,0x1db5fcu,(0x13u+s.gp[4]),1,0x1u);
if(s.services.instruction)s.services.instruction(0x1db601u); goto L1db61e;
L1db603: if(s.services.instruction)s.services.instruction(0x1db603u); s.xmm[2]=_mm_set_ss(f(r(s.memory,(0x90u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db60cu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x8cu+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db615u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x88u+s.gp[4]),4)));
L1db61e: if(s.services.instruction)s.services.instruction(0x1db61eu); s.set(1,r(s.memory,(0x64u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db622u); s.set(0,sign_extend(r(s.memory,(0x1u+s.gp[1]),1),1),4,0);
if(s.services.instruction)s.services.instruction(0x1db626u); s.xmm[0]=_mm_add_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x68u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db62cu); s.xmm[1]=_mm_add_ss(s.xmm[1],_mm_set_ss(f(r(s.memory,(0x6cu+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db632u); s.xmm[2]=_mm_add_ss(s.xmm[2],_mm_set_ss(f(r(s.memory,(0x70u+s.gp[4]),4))));
if(s.services.instruction)s.services.instruction(0x1db638u); s.push(0x1db638u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x1db639u); s.set(1,(0x6cu+s.gp[4]),4,0);
if(s.services.instruction)s.services.instruction(0x1db63du); w(s.memory,s.observe,0x1db63du,(0x6cu+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction)s.services.instruction(0x1db643u); w(s.memory,s.observe,0x1db643u,(0x70u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1db649u); w(s.memory,s.observe,0x1db649u,(0x74u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[2])));
if(s.services.instruction)s.services.instruction(0x1db64fu); s.push(0x1db64fu,0x1db654u);run_1d9100(s);
if(s.services.instruction)s.services.instruction(0x1db654u); goto L1db676;
L1db656: if(s.services.instruction)s.services.instruction(0x1db656u); s.set(1,r(s.memory,(0x4u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db659u); s.set(0,r(s.memory,(0x0u+s.gp[3]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db65bu); s.set(2,r(s.memory,(0x5cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db65fu); s.set(1,s.flags.increment(s.reg(1,4,0),4,true),4,0);
if(s.services.instruction)s.services.instruction(0x1db660u); w(s.memory,s.observe,0x1db660u,(0x4u+s.gp[3]),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db663u); s.set(6,r(s.memory,(0x0u+s.gp[0]+s.gp[1]*8u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db666u); w(s.memory,s.observe,0x1db666u,(0x0u+s.gp[0]+s.gp[2]*8u),4,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x1db669u); s.set(1,r(s.memory,(0x4u+s.gp[0]+s.gp[1]*8u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db66du); w(s.memory,s.observe,0x1db66du,(0x4u+s.gp[0]+s.gp[2]*8u),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x1db671u); s.set(2,s.flags.increment(s.reg(2,4,0),4,true),4,0);
if(s.services.instruction)s.services.instruction(0x1db672u); w(s.memory,s.observe,0x1db672u,(0x5cu+s.gp[4]),4,s.reg(2,4,0));
L1db676: if(s.services.instruction)s.services.instruction(0x1db676u); s.set(2,r(s.memory,(0x1e8u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db67du); s.set(0,r(s.memory,(0x5cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db681u); s.set(1,r(s.memory,(0x94u+s.gp[2]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db687u); s.set(2,r(s.memory,(0x4u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db68au); s.set(0,s.flags.increment(s.reg(0,4,0),4,false),4,0);
if(s.services.instruction)s.services.instruction(0x1db68bu); s.flags.minus(s.reg(0,4,0),s.reg(2,4,0),4);
if(s.services.instruction)s.services.instruction(0x1db68du); w(s.memory,s.observe,0x1db68du,(0x5cu+s.gp[4]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1db691u); if(s.flags.sf!=s.flags.of)goto L1da830;
if(s.services.instruction)s.services.instruction(0x1db697u); s.set(5,s.pop(),4,0);
L1db698: if(s.services.instruction)s.services.instruction(0x1db698u); s.set(7,r(s.memory,(0x1e4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db69fu); s.set(6,r(s.memory,(0x94u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db6a5u); s.flags.logic((s.reg(6,4,0))&(s.reg(6,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1db6a7u); if(s.flags.zf)goto L1db6ea;
if(s.services.instruction)s.services.instruction(0x1db6a9u); s.set(0,r(s.memory,(0x4u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db6acu); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1db6aeu); if(!s.flags.zf)goto L1db6ea;
if(s.services.instruction)s.services.instruction(0x1db6b0u); s.set(0,r(s.memory,(0x8u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db6b3u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1db6b5u); if(s.flags.sf)goto L1db6d0;
if(s.services.instruction)s.services.instruction(0x1db6b7u); s.set(1,r(s.memory,(0x48c938u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db6bdu); s.set(2,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db6bfu); s.set(0,s.flags.logic((s.reg(0,4,0))&(0x7fffffffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db6c4u); s.push(0x1db6c4u,0x12u);
if(s.services.instruction)s.services.instruction(0x1db6c6u); s.set(0,s.flags.shift(s.reg(0,4,0),0x3u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1db6c9u); s.push(0x1db6c9u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1db6cau); s.set(0,r(s.memory,(0x0u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db6ccu); s.push(0x1db6ccu,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1db6cdu); {const auto target=r(s.memory,(0x14u+s.gp[2]),4);s.push(0x1db6cdu,0x1db6d0u);s.required(0x1db6cdu,target,false);}
L1db6d0: if(s.services.instruction)s.services.instruction(0x1db6d0u); s.set(1,r(s.memory,(0x48c938u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db6d6u); s.set(2,r(s.memory,(0x0u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db6d8u); s.push(0x1db6d8u,0x12u);
if(s.services.instruction)s.services.instruction(0x1db6dau); s.push(0x1db6dau,0xcu);
if(s.services.instruction)s.services.instruction(0x1db6dcu); s.push(0x1db6dcu,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x1db6ddu); {const auto target=r(s.memory,(0x14u+s.gp[2]),4);s.push(0x1db6ddu,0x1db6e0u);s.required(0x1db6ddu,target,false);}
if(s.services.instruction)s.services.instruction(0x1db6e0u); w(s.memory,s.observe,0x1db6e0u,(0x94u+s.gp[7]),4,0x0u);
L1db6ea: if(s.services.instruction)s.services.instruction(0x1db6eau); s.set(0,r(s.memory,(0x51df54u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db6efu); s.set(1,sign_extend(r(s.memory,(0x2u+s.gp[0]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x1db6f3u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0ccu),4)));
if(s.services.instruction)s.services.instruction(0x1db6fbu); s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],si(s.reg(1,4,0)));
if(s.services.instruction)s.services.instruction(0x1db6ffu); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db703u); w(s.memory,s.observe,0x1db703u,(0x38u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1db709u); s.fp.push(ext(f(r(s.memory,(0x38u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1db70du); w(s.memory,s.observe,0x1db70du,(0x3cu+s.gp[4]),4,integer_spill_observed(s));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1db711u); s.set(2,sign_extend(r(s.memory,(0x2u+s.gp[0]),2),2),4,0);
if(s.services.instruction)s.services.instruction(0x1db715u); s.xmm[1]=_mm_cvtsi32_ss(s.xmm[1],si(s.reg(2,4,0)));
if(s.services.instruction)s.services.instruction(0x1db719u); s.xmm[1]=_mm_mul_ss(s.xmm[1],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db71du); w(s.memory,s.observe,0x1db71du,(0x38u+s.gp[4]),4,u(_mm_cvtss_f32(s.xmm[1])));
if(s.services.instruction)s.services.instruction(0x1db723u); s.fp.push(ext(f(r(s.memory,(0x38u+s.gp[4]),4))));s.fp.status&=~0x200u;
if(s.services.instruction)s.services.instruction(0x1db727u); w(s.memory,s.observe,0x1db727u,(0x1cu+s.gp[4]),4,integer_spill_observed(s));s.fp.pop();
if(s.services.instruction)s.services.instruction(0x1db72bu); s.set(1,r(s.memory,(0xfu+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1db72fu); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1db731u); s.set(2,r(s.memory,(0x1au+s.gp[7]),1),4,0);
if(s.services.instruction)s.services.instruction(0x1db735u); s.set(0,r(s.memory,(0x1bu+s.gp[7]),1),4,0);
if(s.services.instruction)s.services.instruction(0x1db739u); if(s.flags.zf)goto L1db783;
if(s.services.instruction)s.services.instruction(0x1db73bu); s.xmm[0]=_mm_xor_ps(s.xmm[0],s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x1db73eu); s.xmm[1]=_mm_set_ss(f(r(s.memory,(0x18u+s.gp[4]),4)));
if(s.services.instruction)s.services.instruction(0x1db744u); s.flags.scalar_compare(s.xmm[0],s.xmm[1],false);
if(s.services.instruction)s.services.instruction(0x1db747u); if(!s.flags.cf&&!s.flags.zf)goto L1db759;
if(s.services.instruction)s.services.instruction(0x1db749u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction)s.services.instruction(0x1db751u); s.flags.scalar_compare(s.xmm[1],s.xmm[0],false);
if(s.services.instruction)s.services.instruction(0x1db754u); if(!s.flags.cf&&!s.flags.zf)goto L1db759;
if(s.services.instruction)s.services.instruction(0x1db756u); s.xmm[0]=s.xmm[1];
L1db759: if(s.services.instruction)s.services.instruction(0x1db759u); s.xmm[0]=_mm_mul_ss(s.xmm[0],_mm_set_ss(f(r(s.memory,(0x46a148u),4))));
if(s.services.instruction)s.services.instruction(0x1db761u); s.set(1,std::uint32_t(_mm_cvttss_si32(s.xmm[0])),4,0);
if(s.services.instruction)s.services.instruction(0x1db765u); s.flags.minus(s.reg(1,4,0),s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x1db767u); if(s.flags.zf)goto L1db77d;
if(s.services.instruction)s.services.instruction(0x1db769u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L1db773;
if(s.services.instruction)s.services.instruction(0x1db76bu); s.set(6,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db76fu); s.set(0,s.flags.plus(s.reg(0,4,0),s.reg(6,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db771u); goto L1db777;
L1db773: if(s.services.instruction)s.services.instruction(0x1db773u); s.set(0,s.flags.minus(s.reg(0,4,0),r(s.memory,(0x1cu+s.gp[4]),4),4),4,0);
L1db777: if(s.services.instruction)s.services.instruction(0x1db777u); s.flags.minus(s.reg(0,4,0),s.reg(1,4,0),4);
if(s.services.instruction)s.services.instruction(0x1db779u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L1db77d;
if(s.services.instruction)s.services.instruction(0x1db77bu); s.set(0,s.reg(1,4,0),4,0);
L1db77d: if(s.services.instruction)s.services.instruction(0x1db77du); s.set(2,s.flags.plus(s.reg(2,4,0),r(s.memory,(0x3cu+s.gp[4]),4),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db781u); goto L1db78f;
L1db783: if(s.services.instruction)s.services.instruction(0x1db783u); s.set(6,r(s.memory,(0x3cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db787u); s.set(1,r(s.memory,(0x1cu+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db78bu); s.set(2,s.flags.minus(s.reg(2,4,0),s.reg(6,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db78du); s.set(0,s.flags.minus(s.reg(0,4,0),s.reg(1,4,0),4),4,0);
L1db78f: if(s.services.instruction)s.services.instruction(0x1db78fu); s.flags.logic((s.reg(2,4,0))&(s.reg(2,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1db791u); if(s.flags.sf==s.flags.of)goto L1db797;
if(s.services.instruction)s.services.instruction(0x1db793u); s.set(1,s.flags.logic((s.reg(1,4,0))^(s.reg(1,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db795u); goto L1db7a6;
L1db797: if(s.services.instruction)s.services.instruction(0x1db797u); s.flags.minus(s.reg(2,4,0),0xffu,4);
if(s.services.instruction)s.services.instruction(0x1db79du); s.set(1,0xffu,4,0);
if(s.services.instruction)s.services.instruction(0x1db7a2u); if(!s.flags.zf&&(s.flags.sf==s.flags.of))goto L1db7a6;
if(s.services.instruction)s.services.instruction(0x1db7a4u); s.set(1,s.reg(2,4,0),4,0);
L1db7a6: if(s.services.instruction)s.services.instruction(0x1db7a6u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x1db7a8u); w(s.memory,s.observe,0x1db7a8u,(0x1au+s.gp[7]),1,s.reg(1,1,0));
if(s.services.instruction)s.services.instruction(0x1db7abu); if(s.flags.sf==s.flags.of)goto L1db7b1;
if(s.services.instruction)s.services.instruction(0x1db7adu); s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db7afu); goto L1db7bd;
L1db7b1: if(s.services.instruction)s.services.instruction(0x1db7b1u); s.flags.minus(s.reg(0,4,0),0xffu,4);
if(s.services.instruction)s.services.instruction(0x1db7b6u); if(s.flags.zf||(s.flags.sf!=s.flags.of))goto L1db7bd;
if(s.services.instruction)s.services.instruction(0x1db7b8u); s.set(0,0xffu,4,0);
L1db7bd: if(s.services.instruction)s.services.instruction(0x1db7bdu); w(s.memory,s.observe,0x1db7bdu,(0x1bu+s.gp[7]),1,s.reg(0,1,0));
if(s.services.instruction)s.services.instruction(0x1db7c0u); s.set(0,r(s.memory,(0x14u+s.gp[4]),1),1,0);
if(s.services.instruction)s.services.instruction(0x1db7c4u); s.flags.logic((s.reg(0,1,0))&(s.reg(0,1,0)),1);
if(s.services.instruction)s.services.instruction(0x1db7c6u); s.set(0,r(s.memory,(0x4u+s.gp[7]),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db7c9u); if(s.flags.zf)goto L1db7df;
if(s.services.instruction)s.services.instruction(0x1db7cbu); s.set(0,s.flags.logic((s.reg(0,4,0))|(0x2000u),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db7d0u); w(s.memory,s.observe,0x1db7d0u,(0x4u+s.gp[7]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1db7d3u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1db7d4u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1db7d5u); s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1db7d6u); s.set(4,s.flags.plus(s.reg(4,4,0),0x1d4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1db7dcu); s.pop();s.gp[4]+=8u;return;
L1db7df: if(s.services.instruction)s.services.instruction(0x1db7dfu); s.set(0,s.flags.logic((s.reg(0,4,0))&(0xffffdfffu),4),4,0);
if(s.services.instruction)s.services.instruction(0x1db7e4u); w(s.memory,s.observe,0x1db7e4u,(0x4u+s.gp[7]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x1db7e7u); s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1db7e8u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x1db7e9u); s.set(3,s.pop(),4,0);
L1db7ea: if(s.services.instruction)s.services.instruction(0x1db7eau); s.set(4,s.flags.plus(s.reg(4,4,0),0x1d4u,4),4,0);
if(s.services.instruction)s.services.instruction(0x1db7f0u); s.pop();s.gp[4]+=8u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}

void run_315c90(MathState &s) {
if(s.services.instruction)s.services.instruction(0x315c90u); s.set(0,r(s.memory,(0x44u+s.gp[1]),4),4,0);
if(s.services.instruction)s.services.instruction(0x315c93u); s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);
if(s.services.instruction)s.services.instruction(0x315c95u); if(!s.flags.zf)goto L315ca1;
if(s.services.instruction)s.services.instruction(0x315c97u); s.set(0,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x315c9bu); w(s.memory,s.observe,0x315c9bu,(0x0u+s.gp[0]),1,0x0u);
if(s.services.instruction)s.services.instruction(0x315c9eu); s.pop();s.gp[4]+=4u;return;
L315ca1: if(s.services.instruction)s.services.instruction(0x315ca1u); s.set(1,r(s.memory,(0x29u+s.gp[0]),1),1,0);
if(s.services.instruction)s.services.instruction(0x315ca4u); s.set(0,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x315ca8u); w(s.memory,s.observe,0x315ca8u,(0x0u+s.gp[0]),1,s.reg(1,1,0));
if(s.services.instruction)s.services.instruction(0x315caau); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}

void run_315cb0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x315cb0u); s.push(0x315cb0u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x315cb1u); s.set(6,s.reg(1,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x315cb3u); s.set(1,r(s.memory,(0x8u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x315cb6u); s.flags.logic((s.reg(1,4,0))&(s.reg(1,4,0)),4);
if(s.services.instruction)s.services.instruction(0x315cb8u); if(s.flags.zf)goto L315cd9;
if(s.services.instruction)s.services.instruction(0x315cbau); s.push(0x315cbau,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x315cbbu); {const auto target=0x3165d0u;s.push(0x315cbbu,0x315cc0u);s.required(0x315cbbu,target,false);}
if(s.services.instruction)s.services.instruction(0x315cc0u); s.set(0,r(s.memory,(0x44u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x315cc3u); s.set(1,r(s.memory,(0x29u+s.gp[0]),1),1,0);
if(s.services.instruction)s.services.instruction(0x315cc6u); s.flags.logic((s.reg(1,1,0))&(s.reg(1,1,0)),1);
if(s.services.instruction)s.services.instruction(0x315cc8u); if(!s.flags.zf)goto L315cd9;
if(s.services.instruction)s.services.instruction(0x315ccau); s.flags.minus(r(s.memory,(0x24u+s.gp[0]),4),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x315cceu); if(s.flags.zf)goto L315cd9;
if(s.services.instruction)s.services.instruction(0x315cd0u); s.set(1,r(s.memory,(0x8u+s.gp[6]),4),4,0);
if(s.services.instruction)s.services.instruction(0x315cd3u); s.push(0x315cd3u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x315cd4u); {const auto target=0x316d40u;s.push(0x315cd4u,0x315cd9u);s.required(0x315cd4u,target,false);}
L315cd9: if(s.services.instruction)s.services.instruction(0x315cd9u); s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x315cdau); s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}

void run_3239f0(MathState &s) {
if(s.services.instruction)s.services.instruction(0x3239f0u); s.set(0,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x3239f4u); s.xmm[0]=v(s.memory,(0x0u+s.gp[0]));
if(s.services.instruction)s.services.instruction(0x3239f7u); if(s.observe){SourceVelocityObservationScope observed_frame(s.gp);vw(s.memory,s.observe,0x3239f7u,(0x40u+s.gp[1]),s.xmm[0]);}else{vw(s.memory,s.observe,0x3239f7u,(0x40u+s.gp[1]),s.xmm[0]);}
if(s.services.instruction)s.services.instruction(0x3239fbu); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}

void run_323a00(MathState &s) {
if(s.services.instruction)s.services.instruction(0x323a00u); s.set(0,r(s.memory,(0x4u+s.gp[4]),4),4,0);
if(s.services.instruction)s.services.instruction(0x323a04u); s.xmm[0]=v(s.memory,(0x0u+s.gp[0]));
if(s.services.instruction)s.services.instruction(0x323a07u); vw(s.memory,s.observe,0x323a07u,(0x50u+s.gp[1]),s.xmm[0]);
if(s.services.instruction)s.services.instruction(0x323a0bu); s.pop();s.gp[4]+=4u;return;
throw std::runtime_error("TU1.5 phantom force fell outside authenticated CFG");
}
} // namespace
void source_run_physics_phantom_force_tu15_runtime(M &m,std::uint32_t entry,
 SourcePhysicsContactRuntimeAbi &abi,FP &fp,const SourcePhysicsContactRuntimeServices &services,const O &observe) {
 auto bound=services;
 bound.required_owner=[&](M &memory,const SourcePhysicsContactRuntimeCall &call,
  SourcePhysicsContactRuntimeAbi &state,const O &observer) {
if(call.target==0x24570u){source_run_physics_phantom_force_tu15_runtime(memory,0x24570u,state,fp,services,observer);return;}
if(call.target==0xe1480u){source_run_physics_phantom_force_tu15_runtime(memory,0xe1480u,state,fp,services,observer);return;}
if(call.target==0x11b930u){source_run_physics_phantom_force_tu15_runtime(memory,0x11b930u,state,fp,services,observer);return;}
if(call.target==0x1d9080u){source_run_physics_phantom_force_tu15_runtime(memory,0x1d9080u,state,fp,services,observer);return;}
if(call.target==0x1d9100u){source_run_physics_phantom_force_tu15_runtime(memory,0x1d9100u,state,fp,services,observer);return;}
if(call.target==0x1d92c0u){source_run_physics_phantom_force_tu15_runtime(memory,0x1d92c0u,state,fp,services,observer);return;}
if(call.target==0x1da530u){source_run_physics_phantom_force_tu15_runtime(memory,0x1da530u,state,fp,services,observer);return;}
if(call.target==0x1da750u){source_run_physics_phantom_force_tu15_runtime(memory,0x1da750u,state,fp,services,observer);return;}
if(call.target==0x315c90u){source_run_physics_phantom_force_tu15_runtime(memory,0x315c90u,state,fp,services,observer);return;}
if(call.target==0x315cb0u){source_run_physics_phantom_force_tu15_runtime(memory,0x315cb0u,state,fp,services,observer);return;}
if(call.target==0x3239f0u){source_run_physics_phantom_force_tu15_runtime(memory,0x3239f0u,state,fp,services,observer);return;}
if(call.target==0x323a00u){source_run_physics_phantom_force_tu15_runtime(memory,0x323a00u,state,fp,services,observer);return;}
if(call.target==0x2b3e0u){source_run_physics_vehicle_environment_state_runtime(memory,0x2b3e0u,state,fp,bound,observer);return;}
if(call.target==0x30bd0u){source_run_physics_vehicle_environment_state_runtime(memory,0x30bd0u,state,fp,bound,observer);return;}
if(call.target==0xb17d0u){source_run_physics_vehicle_environment_state_runtime(memory,0xa8380u,state,fp,bound,observer);return;}
if(call.target==0xb9390u){source_run_physics_vehicle_environment_state_runtime(memory,0xb8250u,state,fp,bound,observer);return;}
if(call.target==0xb9770u){source_run_physics_vehicle_environment_state_runtime(memory,0xb8630u,state,fp,bound,observer);return;}
if(call.target==0xb9800u){source_run_physics_vehicle_environment_state_runtime(memory,0xb86c0u,state,fp,bound,observer);return;}
if(call.target==0xbacd0u){source_run_physics_vehicle_environment_state_runtime(memory,0xb9b90u,state,fp,bound,observer);return;}
if(call.target==0xbad60u){source_run_physics_vehicle_environment_state_runtime(memory,0xb9c20u,state,fp,bound,observer);return;}
if(call.target==0xbbbc0u){source_run_physics_vehicle_environment_state_runtime(memory,0xbaa80u,state,fp,bound,observer);return;}
if(call.target==0xbc190u){source_run_physics_vehicle_environment_state_runtime(memory,0xbb050u,state,fp,bound,observer);return;}
if(call.target==0xbc200u){source_run_physics_vehicle_environment_state_runtime(memory,0xbb0c0u,state,fp,bound,observer);return;}
if(call.target==0xbcfb0u){source_run_physics_vehicle_environment_state_runtime(memory,0xbbe70u,state,fp,bound,observer);return;}
if(call.target==0xbda50u){source_run_physics_vehicle_environment_state_runtime(memory,0xbc910u,state,fp,bound,observer);return;}
if(call.target==0xbcdf0u){source_run_physics_vehicle_environment_state_runtime(memory,0xbcdf0u,state,fp,bound,observer);return;}
if(call.target==0xbe7e0u){source_run_physics_vehicle_environment_state_runtime(memory,0xbd6a0u,state,fp,bound,observer);return;}
if(call.target==0xc6120u){source_run_physics_vehicle_environment_state_runtime(memory,0xc6120u,state,fp,bound,observer);return;}
if(call.target==0xd3750u){source_run_physics_vehicle_environment_state_runtime(memory,0xd26c0u,state,fp,bound,observer);return;}
if(call.target==0xde8c0u){source_run_physics_vehicle_environment_state_runtime(memory,0xde8c0u,state,fp,bound,observer);return;}
if(call.target==0xe8d50u){source_run_physics_vehicle_environment_state_runtime(memory,0xe7e30u,state,fp,bound,observer);return;}
if(call.target==0xe8d80u){source_run_physics_vehicle_environment_state_runtime(memory,0xe7e60u,state,fp,bound,observer);return;}
if(call.target==0xf11f0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf02d0u,state,fp,bound,observer);return;}
if(call.target==0xf15d0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf06b0u,state,fp,bound,observer);return;}
if(call.target==0xf1a50u){source_run_physics_vehicle_environment_state_runtime(memory,0xf0b30u,state,fp,bound,observer);return;}
if(call.target==0xf1dd0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf0eb0u,state,fp,bound,observer);return;}
if(call.target==0xf20c0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf11a0u,state,fp,bound,observer);return;}
if(call.target==0xf2190u){source_run_physics_vehicle_environment_state_runtime(memory,0xf1270u,state,fp,bound,observer);return;}
if(call.target==0xf2340u){source_run_physics_vehicle_environment_state_runtime(memory,0xf1420u,state,fp,bound,observer);return;}
if(call.target==0xf2c20u){source_run_physics_vehicle_environment_state_runtime(memory,0xf1d00u,state,fp,bound,observer);return;}
if(call.target==0xf3880u){source_run_physics_vehicle_environment_state_runtime(memory,0xf2960u,state,fp,bound,observer);return;}
if(call.target==0xf38e0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf29c0u,state,fp,bound,observer);return;}
if(call.target==0xf3a50u){source_run_physics_vehicle_environment_state_runtime(memory,0xf2b30u,state,fp,bound,observer);return;}
if(call.target==0xf3aa0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf2b80u,state,fp,bound,observer);return;}
if(call.target==0xf3b30u){source_run_physics_vehicle_environment_state_runtime(memory,0xf2c10u,state,fp,bound,observer);return;}
if(call.target==0xf3ca0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf2d80u,state,fp,bound,observer);return;}
if(call.target==0xf3eb0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf2f90u,state,fp,bound,observer);return;}
if(call.target==0xf41e0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf32c0u,state,fp,bound,observer);return;}
if(call.target==0xf4230u){source_run_physics_vehicle_environment_state_runtime(memory,0xf3310u,state,fp,bound,observer);return;}
if(call.target==0xf44e0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf35c0u,state,fp,bound,observer);return;}
if(call.target==0xf48c0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf39a0u,state,fp,bound,observer);return;}
if(call.target==0xf5570u){source_run_physics_vehicle_environment_state_runtime(memory,0xf4650u,state,fp,bound,observer);return;}
if(call.target==0xf67b0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf5890u,state,fp,bound,observer);return;}
if(call.target==0xf6850u){source_run_physics_vehicle_environment_state_runtime(memory,0xf5930u,state,fp,bound,observer);return;}
if(call.target==0xf6a00u){source_run_physics_vehicle_environment_state_runtime(memory,0xf5ae0u,state,fp,bound,observer);return;}
if(call.target==0xf6d60u){source_run_physics_vehicle_environment_state_runtime(memory,0xf5e40u,state,fp,bound,observer);return;}
if(call.target==0xf8150u){source_run_physics_vehicle_environment_state_runtime(memory,0xf7230u,state,fp,bound,observer);return;}
if(call.target==0xf82d0u){source_run_physics_vehicle_environment_state_runtime(memory,0xf73b0u,state,fp,bound,observer);return;}
if(call.target==0xf8360u){source_run_physics_vehicle_environment_state_runtime(memory,0xf7440u,state,fp,bound,observer);return;}
if(call.target==0xf8440u){source_run_physics_vehicle_environment_state_runtime(memory,0xf7520u,state,fp,bound,observer);return;}
if(call.target==0xf8570u){source_run_physics_vehicle_environment_state_runtime(memory,0xf7650u,state,fp,bound,observer);return;}
if(call.target==0xf9130u){source_run_physics_vehicle_environment_state_runtime(memory,0xf8210u,state,fp,bound,observer);return;}
if(call.target==0x111500u){source_run_physics_vehicle_environment_state_runtime(memory,0x1105e0u,state,fp,bound,observer);return;}
if(call.target==0x116520u){source_run_physics_vehicle_environment_state_runtime(memory,0x115600u,state,fp,bound,observer);return;}
if(call.target==0x11e600u){source_run_physics_vehicle_environment_state_runtime(memory,0x11e600u,state,fp,bound,observer);return;}
if(call.target==0x11e850u){source_run_physics_vehicle_environment_state_runtime(memory,0x11e850u,state,fp,bound,observer);return;}
if(call.target==0x120150u){source_run_physics_vehicle_environment_state_runtime(memory,0x11eeb0u,state,fp,bound,observer);return;}
if(call.target==0x144090u){source_run_physics_vehicle_environment_state_runtime(memory,0x142d00u,state,fp,bound,observer);return;}
if(call.target==0x144910u){source_run_physics_vehicle_environment_state_runtime(memory,0x143580u,state,fp,bound,observer);return;}
if(call.target==0x144a60u){source_run_physics_vehicle_environment_state_runtime(memory,0x1436d0u,state,fp,bound,observer);return;}
if(call.target==0x144bf0u){source_run_physics_vehicle_environment_state_runtime(memory,0x143860u,state,fp,bound,observer);return;}
if(call.target==0x145560u){source_run_physics_vehicle_environment_state_runtime(memory,0x1441d0u,state,fp,bound,observer);return;}
if(call.target==0x145680u){source_run_physics_vehicle_environment_state_runtime(memory,0x1442f0u,state,fp,bound,observer);return;}
if(call.target==0x1459b0u){source_run_physics_vehicle_environment_state_runtime(memory,0x144620u,state,fp,bound,observer);return;}
if(call.target==0x14cdb0u){source_run_physics_vehicle_environment_state_runtime(memory,0x14cdb0u,state,fp,bound,observer);return;}
if(call.target==0x16e6a0u){source_run_physics_vehicle_environment_state_runtime(memory,0x16e6a0u,state,fp,bound,observer);return;}
if(call.target==0x16e8a0u){source_run_physics_vehicle_environment_state_runtime(memory,0x16e8a0u,state,fp,bound,observer);return;}
if(call.target==0x16ea60u){source_run_physics_vehicle_environment_state_runtime(memory,0x16ea60u,state,fp,bound,observer);return;}
if(call.target==0x1755b0u){source_run_physics_vehicle_environment_state_runtime(memory,0x1755b0u,state,fp,bound,observer);return;}
if(call.target==0x177020u){source_run_physics_vehicle_environment_state_runtime(memory,0x177020u,state,fp,bound,observer);return;}
if(call.target==0x177fa0u){source_run_physics_vehicle_environment_state_runtime(memory,0x177fa0u,state,fp,bound,observer);return;}
if(call.target==0x178e30u){source_run_physics_vehicle_environment_state_runtime(memory,0x178e30u,state,fp,bound,observer);return;}
if(call.target==0x17a030u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a030u,state,fp,bound,observer);return;}
if(call.target==0x17a0f0u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a0f0u,state,fp,bound,observer);return;}
if(call.target==0x17a230u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a230u,state,fp,bound,observer);return;}
if(call.target==0x17a2f0u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a2f0u,state,fp,bound,observer);return;}
if(call.target==0x17a380u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a380u,state,fp,bound,observer);return;}
if(call.target==0x17a420u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a420u,state,fp,bound,observer);return;}
if(call.target==0x17a4a0u){source_run_physics_vehicle_environment_state_runtime(memory,0x17a4a0u,state,fp,bound,observer);return;}
if(call.target==0x17dec0u){source_run_physics_vehicle_environment_state_runtime(memory,0x17dec0u,state,fp,bound,observer);return;}
if(call.target==0x185070u){source_run_physics_vehicle_environment_state_runtime(memory,0x183c50u,state,fp,bound,observer);return;}
if(call.target==0x18add0u){source_run_physics_vehicle_environment_state_runtime(memory,0x18add0u,state,fp,bound,observer);return;}
if(call.target==0x18ae80u){source_run_physics_vehicle_environment_state_runtime(memory,0x18ae80u,state,fp,bound,observer);return;}
if(call.target==0x18af20u){source_run_physics_vehicle_environment_state_runtime(memory,0x18af20u,state,fp,bound,observer);return;}
if(call.target==0x18b0c0u){source_run_physics_vehicle_environment_state_runtime(memory,0x18b0c0u,state,fp,bound,observer);return;}
if(call.target==0x18b160u){source_run_physics_vehicle_environment_state_runtime(memory,0x18b160u,state,fp,bound,observer);return;}
if(call.target==0x18b1d0u){source_run_physics_vehicle_environment_state_runtime(memory,0x18b1d0u,state,fp,bound,observer);return;}
if(call.target==0x18bbc0u){source_run_physics_vehicle_environment_state_runtime(memory,0x18bbc0u,state,fp,bound,observer);return;}
if(call.target==0x18c440u){source_run_physics_vehicle_environment_state_runtime(memory,0x18c440u,state,fp,bound,observer);return;}
if(call.target==0x18cdd0u){source_run_physics_vehicle_environment_state_runtime(memory,0x18cdd0u,state,fp,bound,observer);return;}
if(call.target==0x18d4b0u){source_run_physics_vehicle_environment_state_runtime(memory,0x18d4b0u,state,fp,bound,observer);return;}
if(call.target==0x18d510u){source_run_physics_vehicle_environment_state_runtime(memory,0x18d510u,state,fp,bound,observer);return;}
if(call.target==0x1903a0u){source_run_physics_vehicle_environment_state_runtime(memory,0x1903a0u,state,fp,bound,observer);return;}
if(call.target==0x190420u){source_run_physics_vehicle_environment_state_runtime(memory,0x190420u,state,fp,bound,observer);return;}
if(call.target==0x1cd6a0u){source_run_physics_vehicle_environment_state_runtime(memory,0x1c8430u,state,fp,bound,observer);return;}
if(call.target==0x1d64c0u){source_run_physics_vehicle_environment_state_runtime(memory,0x1d1c00u,state,fp,bound,observer);return;}
if(call.target==0x1d7d70u){source_run_physics_vehicle_environment_state_runtime(memory,0x1d34b0u,state,fp,bound,observer);return;}
if(call.target==0x1d7e40u){source_run_physics_vehicle_environment_state_runtime(memory,0x1d3580u,state,fp,bound,observer);return;}
if(call.target==0x1d8c80u){source_run_physics_vehicle_environment_state_runtime(memory,0x1d43c0u,state,fp,bound,observer);return;}
if(call.target==0x1d8d80u){source_run_physics_vehicle_environment_state_runtime(memory,0x1d44c0u,state,fp,bound,observer);return;}
if(call.target==0x1d8dd0u){source_run_physics_vehicle_environment_state_runtime(memory,0x1d4510u,state,fp,bound,observer);return;}
if(call.target==0x1d8e60u){source_run_physics_vehicle_environment_state_runtime(memory,0x1d45a0u,state,fp,bound,observer);return;}
if(call.target==0x1d8b90u){source_run_physics_vehicle_environment_state_runtime(memory,0x1d8b90u,state,fp,bound,observer);return;}
if(call.target==0x1e11b0u){source_run_physics_vehicle_environment_state_runtime(memory,0x1dc8f0u,state,fp,bound,observer);return;}
if(call.target==0x1e1200u){source_run_physics_vehicle_environment_state_runtime(memory,0x1dc940u,state,fp,bound,observer);return;}
if(call.target==0x20d680u){source_run_physics_vehicle_environment_state_runtime(memory,0x208d80u,state,fp,bound,observer);return;}
if(call.target==0x20d770u){source_run_physics_vehicle_environment_state_runtime(memory,0x208e70u,state,fp,bound,observer);return;}
if(call.target==0x20d7d0u){source_run_physics_vehicle_environment_state_runtime(memory,0x208ed0u,state,fp,bound,observer);return;}
if(call.target==0x20d9a0u){source_run_physics_vehicle_environment_state_runtime(memory,0x2090a0u,state,fp,bound,observer);return;}
if(call.target==0x20dea0u){source_run_physics_vehicle_environment_state_runtime(memory,0x2095a0u,state,fp,bound,observer);return;}
if(call.target==0x20e350u){source_run_physics_vehicle_environment_state_runtime(memory,0x209a50u,state,fp,bound,observer);return;}
if(call.target==0x20fcb0u){source_run_physics_vehicle_environment_state_runtime(memory,0x20b3b0u,state,fp,bound,observer);return;}
if(call.target==0x20fd70u){source_run_physics_vehicle_environment_state_runtime(memory,0x20b470u,state,fp,bound,observer);return;}
if(call.target==0x228220u){source_run_physics_vehicle_environment_state_runtime(memory,0x228220u,state,fp,bound,observer);return;}
if(call.target==0x22e1b0u){source_run_physics_vehicle_environment_state_runtime(memory,0x22e1b0u,state,fp,bound,observer);return;}
if(call.target==0x22e210u){source_run_physics_vehicle_environment_state_runtime(memory,0x22e210u,state,fp,bound,observer);return;}
if(call.target==0x22e390u){source_run_physics_vehicle_environment_state_runtime(memory,0x22e390u,state,fp,bound,observer);return;}
if(call.target==0x22e4f0u){source_run_physics_vehicle_environment_state_runtime(memory,0x22e4f0u,state,fp,bound,observer);return;}
if(call.target==0x22eec0u){source_run_physics_vehicle_environment_state_runtime(memory,0x22eec0u,state,fp,bound,observer);return;}
if(call.target==0x22efd0u){source_run_physics_vehicle_environment_state_runtime(memory,0x22efd0u,state,fp,bound,observer);return;}
if(call.target==0x22fe30u){source_run_physics_vehicle_environment_state_runtime(memory,0x22fe30u,state,fp,bound,observer);return;}
if(call.target==0x22fea0u){source_run_physics_vehicle_environment_state_runtime(memory,0x22fea0u,state,fp,bound,observer);return;}
if(call.target==0x230a60u){source_run_physics_vehicle_environment_state_runtime(memory,0x230a60u,state,fp,bound,observer);return;}
if(call.target==0x230ae0u){source_run_physics_vehicle_environment_state_runtime(memory,0x230ae0u,state,fp,bound,observer);return;}
if(call.target==0x2e44d0u){source_run_physics_vehicle_environment_state_runtime(memory,0x2e44d0u,state,fp,bound,observer);return;}
if(call.target==0x2e4650u){source_run_physics_vehicle_environment_state_runtime(memory,0x2e4650u,state,fp,bound,observer);return;}
if(call.target==0x3150e0u){source_run_physics_vehicle_environment_state_runtime(memory,0x3115f0u,state,fp,bound,observer);return;}
if(call.target==0x3248b0u){source_run_physics_vehicle_environment_state_runtime(memory,0x31f9e0u,state,fp,bound,observer);return;}
if(call.target==0x324be0u){source_run_physics_vehicle_environment_state_runtime(memory,0x321070u,state,fp,bound,observer);return;}
if(call.target==0x324c40u){source_run_physics_vehicle_environment_state_runtime(memory,0x3210d0u,state,fp,bound,observer);return;}
if(call.target==0x32a660u){source_run_physics_vehicle_environment_state_runtime(memory,0x326af0u,state,fp,bound,observer);return;}
if(call.target==0x32b26cu){source_run_physics_vehicle_environment_state_runtime(memory,0x3276fcu,state,fp,bound,observer);return;}
if(call.target==0x32b2feu){source_run_physics_vehicle_environment_state_runtime(memory,0x32778eu,state,fp,bound,observer);return;}
if(call.target==0x32b38cu){source_run_physics_vehicle_environment_state_runtime(memory,0x32781cu,state,fp,bound,observer);return;}
if(call.target==0x32b3c5u){source_run_physics_vehicle_environment_state_runtime(memory,0x327855u,state,fp,bound,observer);return;}
if(call.target==0x32b3d6u){source_run_physics_vehicle_environment_state_runtime(memory,0x327866u,state,fp,bound,observer);return;}
if(call.target==0x331ecbu){source_run_physics_vehicle_environment_state_runtime(memory,0x32e35bu,state,fp,bound,observer);return;}
if(call.target==0x331f68u){source_run_physics_vehicle_environment_state_runtime(memory,0x32e3f8u,state,fp,bound,observer);return;}
if(call.target==0x331fa1u){source_run_physics_vehicle_environment_state_runtime(memory,0x32e431u,state,fp,bound,observer);return;}
if(call.target==0x331fa3u){source_run_physics_vehicle_environment_state_runtime(memory,0x32e433u,state,fp,bound,observer);return;}
if(call.target==0x331fcbu){source_run_physics_vehicle_environment_state_runtime(memory,0x32e45bu,state,fp,bound,observer);return;}
if(call.target==0x33200au){source_run_physics_vehicle_environment_state_runtime(memory,0x32e49au,state,fp,bound,observer);return;}
if(call.target==0x37cde8u){source_run_physics_vehicle_environment_state_runtime(memory,0x379338u,state,fp,bound,observer);return;}
if(call.target==0x37cdeau){source_run_physics_vehicle_environment_state_runtime(memory,0x37933au,state,fp,bound,observer);return;}
if(call.target==0x37d46cu){source_run_physics_vehicle_environment_state_runtime(memory,0x3799bcu,state,fp,bound,observer);return;}
if(call.target==0x37d536u){source_run_physics_vehicle_environment_state_runtime(memory,0x379a86u,state,fp,bound,observer);return;}
if(call.target==0x37dc0au){source_run_physics_vehicle_environment_state_runtime(memory,0x37a15au,state,fp,bound,observer);return;}
if(call.target==0x37dc14u){source_run_physics_vehicle_environment_state_runtime(memory,0x37a164u,state,fp,bound,observer);return;}

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
case 0xbcdf0u:
case 0xbd6a0u:
case 0xc6120u:
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
case 0x11e600u:
case 0x11e850u:
case 0x11eeb0u:
case 0x142d00u:
case 0x143580u:
case 0x1436d0u:
case 0x143860u:
case 0x1441d0u:
case 0x1442f0u:
case 0x144620u:
case 0x14cdb0u:
case 0x16e6a0u:
case 0x16e8a0u:
case 0x16ea60u:
case 0x1755b0u:
case 0x177020u:
case 0x177fa0u:
case 0x178e30u:
case 0x17a030u:
case 0x17a0f0u:
case 0x17a230u:
case 0x17a2f0u:
case 0x17a380u:
case 0x17a420u:
case 0x17a4a0u:
case 0x17dec0u:
case 0x183c50u:
case 0x18add0u:
case 0x18ae80u:
case 0x18af20u:
case 0x18b0c0u:
case 0x18b160u:
case 0x18b1d0u:
case 0x18bbc0u:
case 0x18c440u:
case 0x18cdd0u:
case 0x18d4b0u:
case 0x18d510u:
case 0x1903a0u:
case 0x190420u:
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
case 0x228220u:
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
case 0x2e44d0u:
case 0x2e4650u:
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

  try {source_run_physics_vehicle_environment_state_runtime(m,entry,abi,fp,bound,observe);}
  catch(...) {if(abi.floating_point)fp=*abi.floating_point;throw;}
  if(abi.floating_point)fp=*abi.floating_point;return;
case 0x24570u:break;
case 0xe1480u:break;
case 0x11b930u:break;
case 0x1d9080u:break;
case 0x1d9100u:break;
case 0x1d92c0u:break;
case 0x1da530u:break;
case 0x1da750u:break;
case 0x315c90u:break;
case 0x315cb0u:break;
case 0x3239f0u:break;
case 0x323a00u:break;

 default:throw std::runtime_error("Required TU1.5 vehicle entry unavailable");
 }
 FpScope scope(fp);MathState s{m,observe,bound,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
 try {switch(entry) {
case 0x24570u:run_24570(s);break;
case 0xe1480u:run_e1480(s);break;
case 0x11b930u:run_11b930(s);break;
case 0x1d9080u:run_1d9080(s);break;
case 0x1d9100u:run_1d9100(s);break;
case 0x1d92c0u:run_1d92c0(s);break;
case 0x1da530u:run_1da530(s);break;
case 0x1da750u:run_1da750(s);break;
case 0x315c90u:run_315c90(s);break;
case 0x315cb0u:run_315cb0(s);break;
case 0x3239f0u:run_3239f0(s);break;
case 0x323a00u:run_323a00(s);break;

 }}catch(...) {abi=s.save();fp={control_word(),_mm_getcsr()};throw;}
 abi=s.save();fp={control_word(),_mm_getcsr()};
}
} // namespace halo2
