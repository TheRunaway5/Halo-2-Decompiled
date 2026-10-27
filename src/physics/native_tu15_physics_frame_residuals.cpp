#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <array>
#include <limits>
#include <stdexcept>
namespace halo2 {
namespace {
void check(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
void put(SourcePhysicsContactMemory&m,std::uint32_t address,std::uint32_t value){
  std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(i*8u));m.write(address,b);
}
std::uint32_t word(SourcePhysicsContactMemory&m,std::uint32_t address){
  std::array<std::byte,4>b{};m.read(address,b);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(b[i])<<(i*8u);return value;
}
void valid_live_values(const SourcePhysicsContactRuntimeAbi&a){
  check(a.x87_depth<=8,"Native physics frame residual has invalid live x87 depth");
  check(!a.active_mmx||!a.x87_depth,"Native physics frame residual mixes MMX and logical x87 ownership");
  if(a.x87_depth)check(sizeof(long double)>=10,"Native physics frame residual requires original extended precision live values");
}
}
void native_tu15_step_physics_frame_with_residuals(SourcePhysicsContactMemory&m,
  NativeTu15PhysicsFrameFp&fp,SourcePhysicsContactRuntimeAbi&state,
  NativeTu15PhysicsFrameScratch scratch,const NativeTu15PhysicsFrameResidualCall&invoke,
  const SourcePhysicsContactRecordObserver&observe,NativeTu15PhysicsFrameExecutionStats*stats,
  const NativeTu15PhysicsFrameWorldStep&native_world_step){
  if(stats)++stats->frame_calls;
  NativeTu15PhysicsFrameMemory storage(m,observe);
  // Preserve the original null-world prefix: it needs no stack/backend owner.
  if(!storage.world().identity)return;
  valid_live_values(state);
  const auto end=std::uint64_t(scratch.first)+scratch.bytes;
  check(scratch.bytes>=0x800u&&end<=std::numeric_limits<std::uint32_t>::max()&&end<=state.gp[4],
    "Native physics frame requires an owned retained stack interval below the active caller");
  const auto call_stack=std::uint32_t(end)-0x40u,step_packet=std::uint32_t(end)-0x10u;
  // Reuse the actual caller-owned continuation token as the bridge return
  // protocol. It is not an invented original root result or guest execution.
  const auto continuation=word(m,state.gp[4]);
  auto call=[&](std::uint32_t entry,std::span<const std::uint32_t>args,
      std::optional<std::uint32_t>eax,std::optional<std::uint32_t>ecx,
      unsigned callee_pop,NativeTu15PhysicsFrameFp&context){
    check(bool(invoke),"Native physics frame residual owner unavailable");valid_live_values(state);
    state.gp[4]=call_stack;if(eax)state.gp[0]=*eax;if(ecx)state.gp[1]=*ecx;
    state.x87_status=context.status;state.floating_point=SourcePhysicsFactoryFloatingPoint{context.control_word,context.mxcsr};
    put(m,call_stack,continuation);for(unsigned n=0;n<args.size();++n)put(m,call_stack+4u+n*4u,args[n]);
    if(stats)++stats->residual_calls;
    try{invoke(entry,state,observe);}catch(...){
      // A real missing-owner/fault prefix keeps all actual callee FP effects.
      if(state.floating_point){context.control_word=state.floating_point->x87_control_word;context.mxcsr=state.floating_point->mxcsr;context.status=state.x87_status;}
      throw;
    }
    check(bool(state.floating_point),"Native physics frame residual lost actual FP policy");valid_live_values(state);
    context.control_word=state.floating_point->x87_control_word;context.mxcsr=state.floating_point->mxcsr;context.status=state.x87_status;
    check(state.gp[4]==call_stack+4u+callee_pop,"Native physics frame residual return stack differs from its original ABI");
  };
  NativeTu15PhysicsFrameServices s;
  s.resize_debug_arena=[&](auto bytes,auto&f){const std::array args{bytes};call(0x2e2990u,args,{},{},0,f);};
  s.reset_debug_arena=[&](auto&f){call(0x2e2910u,{},{},{},0,f);};
  s.publish_debug_arena=[&](auto arena,auto&f){call(0x2e2b30u,{},{},arena,0,f);};
  s.drain_commands=[&](auto&f){call(0x1496f0u,{},{},{},0,f);};
  s.step_world=[&](auto world,const auto&step,auto&f){
    if(native_world_step){native_world_step(world,step,f);return;}
    put(m,step_packet,step.seconds_bits);put(m,step_packet+4u,step.inverse_seconds_bits);
    const std::array args{step_packet};call(0x31b1a0u,args,{},world.identity,4,f);
  };
  s.post_step=[&](auto&f){call(0x282c10u,{},{},{},0,f);};
  s.update_contacts=[&](auto&f){call(0x1cce00u,{},{},{},0,f);};
  s.publish_object=[&](auto object,auto&f){call(0xbda50u,{},object.datum,{},0,f);};
  s.destroy_body=[&](auto body,bool deleting,auto&f){
    const auto vtable=word(m,body.identity),target=word(m,vtable);
    const std::array args{std::uint32_t(deleting)};call(target,args,{},body.identity,4,f);
  };
  native_tu15_step_physics_frame(storage,fp,s);
  // Only real FP state is published to the residual context. Its other fields
  // remain the actual last callee output; callers must not use them as if the
  // native root had reproduced the original dead register file.
  state.x87_status=fp.status;state.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
}
} // namespace halo2
