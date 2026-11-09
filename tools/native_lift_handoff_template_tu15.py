"""Native TU1.5 force handoff text and authenticated source-site substitutions."""
HANDOFFS = r'''bool native_lift_marker_call(MathState &state) {
  const auto *policy = native_lift_scalar_consumer_policy_tu15(state.memory);
  if (!policy) return false;
  if (!policy->request_marker || !policy->publish_phase ||
      (policy->first_trace && *policy->first_trace) ||
      (policy->second_trace && *policy->second_trace) ||
      (state.observe && !policy->observation_only_stores) || state.active_mmx)
    throw std::runtime_error("Native lift marker requires its typed DATA and observation owner");
  const auto stack = state.gp[4];
  const auto destination = state.gp[3];
  if (stack < policy->caller_stack_begin || std::uint64_t(stack) + 0x188u > policy->caller_stack_end ||
      destination != stack + 0x118u)
    throw std::runtime_error("Native lift marker needs its caller-owned record");
  const auto object = r(state.memory, stack, 4);
  const auto marker = r(state.memory, stack + 4u, 4);
  const auto capacity = r(state.memory, stack + 8u, 2);
  if (capacity != 1u || state.reg(1, 1, 0))
    throw std::runtime_error("Native lift marker caller arguments differ");
  NativeTu15PhysicsFrameFp fp{control_word(), state.fp.word(), _mm_getcsr()};
  policy->publish_phase(0xbad60u);
  const auto publish = [&] {
    state.fp.status = fp.status;
    state.fp.top = (fp.status >> 11u) & 7u;
    load_control_word(fp.control_word);
    _mm_setcsr(fp.mxcsr);
  };
  std::uint16_t count{};
  try {
    count = policy->request_marker(state.memory, object, false, marker, 1, destination, fp);
  } catch (...) {
    publish();
    throw;
  }
  publish();
  // The first caller tests AX. The second reloads EAX at its continuation.
  state.set(0, count, 2, 0);
  state.gp[4] += 12u;
  return true;
}
bool native_lift_body_velocity_call(MathState &s, std::uint32_t site) {
  const auto *policy=native_lift_scalar_consumer_policy_tu15(s.memory);
  if(!policy)return false; // Standalone original-ABI proof, outside live policy.
  const bool angular=site==0x1db64fu;
  if(!angular&&site!=0x1db370u)throw std::runtime_error("Native lift velocity callsite differs");
  const auto index=angular?1u:0u;
  auto *activity=policy->world_activity;
  if(activity)++activity->velocity_attempts[index];
  try {
  if(!policy->deliver_velocity || !policy->publish_phase ||
     (policy->first_trace&&*policy->first_trace) || (policy->second_trace&&*policy->second_trace) ||
     (s.observe&&!policy->observation_only_stores) || s.active_mmx)
    throw std::runtime_error("Native lift velocity requires its typed DATA and observation owner");
  const auto sp=s.gp[4],vector=s.gp[angular?1u:2u];
  if(sp<policy->caller_stack_begin || std::uint64_t(sp)+(angular?4u:0u)>policy->caller_stack_end ||
     vector<policy->caller_stack_begin || std::uint64_t(vector)+12u>policy->caller_stack_end)
    throw std::runtime_error("Native lift velocity needs retained caller DATA");
  const NativeTu15LiftVelocityDelivery input{
      angular?NativeTu15LiftVelocityPhase::set_angular:NativeTu15LiftVelocityPhase::add_linear,
      angular?r(s.memory,sp,4):s.gp[6],s.gp[angular?0u:1u],vector,s.fp.count};
  NativeTu15PhysicsFrameFp fp{control_word(),s.fp.word(),_mm_getcsr()};
  policy->publish_phase(angular?0x1d9100u:0x1d92c0u);
  NativeTu15RetainedStoreObserver observation(s.observe?SourcePhysicsContactRecordObserver([&](const auto&e){
      policy->publish_phase(e.instruction);s.observe(e);
      if(activity&&e.instruction==(angular?0x323a07u:0x3239f7u))++activity->velocity_observed_stores[index];
  }):SourcePhysicsContactRecordObserver{});
  const auto publish=[&]{
    s.fp.status=fp.status;s.fp.top=(fp.status>>11u)&7u;
    load_control_word(fp.control_word);_mm_setcsr(fp.mxcsr);
  };
  try{policy->deliver_velocity(s.memory,input,fp,observation);}
  catch(...){publish();throw;}
  publish();
  if(angular)s.gp[4]+=4u; // Actual RET4 pops the caller's ordinary component argument.
  if(activity)++activity->velocity_completed[index];
  return true;
  } catch(...) {
    if(activity)++activity->velocity_refusals;
    throw;
  }
}
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
'''
def once(text,old,new):
    assert text.count(old)==1,old
    return text.replace(old,new)
def apply_native_lift_handoffs(text):
    text=once(text,'#include "halo2/physics_velocity_observation.hpp"',
        '#include "halo2/physics_velocity_observation.hpp"\n#include "halo2/native_lift_scalar_consumer_tu15.hpp"\n#include "halo2/native_tu15_physics_frame.hpp"\n#include "halo2/native_tu15_retained_store_observer.hpp"')
    text=once(text,'using namespace physics_vehicle_motion_math_detail;\n','using namespace physics_vehicle_motion_math_detail;\n'+HANDOFFS)
    for site in (0x1dabbe,0x1daf99,0x1db2d0,0x1db4db):
        old=f's.push(0x{site:x}u,0x{site+5:x}u);run_1da530(s);'
        text=once(text,old,f'native_lift_scalar_call(s,0x{site:x}u,0x{site+5:x}u);')
    for site in (0x1da9a6,0x1db3a4):
        old=f'{{const auto target=0xbad60u;s.push(0x{site:x}u,0x{site+5:x}u);s.required(0x{site:x}u,target,false);}}'
        text=once(text,old,'if(!native_lift_marker_call(s))'+old)
    for site,owner in ((0x1db370,0x1d92c0),(0x1db64f,0x1d9100)):
        old=f's.push(0x{site:x}u,0x{site+5:x}u);run_{owner:x}(s);'
        text=once(text,old,f'if(!native_lift_body_velocity_call(s,0x{site:x}u)){{'+old+'}')
    return text
