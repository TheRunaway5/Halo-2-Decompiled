#include "halo2/native_lift_scalar_consumer_tu15.hpp"
#include "halo2/source_lift_scalar_storage_tu15.hpp"
#include "halo2/source_lift_power_tu15.hpp"
#include <atomic>
#include <bit>
#include <stdexcept>
#include <map>
#include <mutex>
#include <xmmintrin.h>
namespace halo2 {
namespace {
std::array<std::atomic<std::uint64_t>,4> calls{};
std::atomic<std::uint64_t> powers{},errnos{},refusals{};
std::mutex activity_mutex;
std::map<const SourcePhysicsContactMemory*,NativeLiftScalarActivityTu15> memory_activity;
thread_local NativeLiftScalarConsumerScopeTu15* active_scope{};
void note_power(const SourcePhysicsContactMemory&m){powers.fetch_add(1,std::memory_order_relaxed);std::lock_guard lock(activity_mutex);++memory_activity[&m].power_calls;if(const auto*p=native_lift_scalar_consumer_policy_tu15(m);p&&p->world_activity)++p->world_activity->power_calls;}
void note_errno(const SourcePhysicsContactMemory&m){errnos.fetch_add(1,std::memory_order_relaxed);std::lock_guard lock(activity_mutex);++memory_activity[&m].errno_writes;if(const auto*p=native_lift_scalar_consumer_policy_tu15(m);p&&p->world_activity)++p->world_activity->errno_writes;}
class Storage final:public SourceLiftScalarStorageTu15 {
  SourcePhysicsContactMemory&m_;NativeLiftScalarConsumerTu15&c_;
  const std::function<void(std::uint32_t)>&phase_;const SourcePhysicsContactRecordObserver&observe_;
  std::uint32_t clock_cell_{},ratio_cell_{};
  bool power_branch_{};
  void capture()const {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t control,status;asm volatile("fnstcw %0; fnstsw %1":"=m"(control),"=am"(status));
    c_.floating_point={control,_mm_getcsr(),std::uint8_t(status&0x3fu),status};c_.numeric_status=status;
#endif
  }
  void phase(std::uint32_t site)const {capture();if(phase_)phase_(site);}
  std::uint32_t word(std::uint32_t site,std::uint32_t address,unsigned count=4)const {
    phase(site);std::array<std::byte,4> raw{};
    try{m_.read(address,std::span(raw).first(count));}catch(...){capture();throw;}
    std::uint32_t value{};for(unsigned n=0;n<count;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8*n);return value;
  }
  float scalar(std::uint32_t site,std::uint32_t address)const{return std::bit_cast<float>(word(site,address));}
  void put(std::uint32_t site,std::uint32_t address,std::uint32_t value){
    phase(site);std::array<std::byte,4> before{},raw{};
    try{
      if(observe_)m_.read(address,before);
      for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(8*n));
      m_.write(address,raw);
      if(observe_)observe_({site,address,{before.begin(),before.end()},{raw.begin(),raw.end()}});
    }catch(...){capture();throw;}
  }
  void put_float(std::uint32_t site,std::uint32_t address,float value){put(site,address,std::bit_cast<std::uint32_t>(value));}
  std::uint32_t argument(unsigned offset)const{return c_.entry_stack+offset;}
public:
  Storage(SourcePhysicsContactMemory&m,NativeLiftScalarConsumerTu15&c,const std::function<void(std::uint32_t)>&p,const SourcePhysicsContactRecordObserver&o)
    :m_(m),c_(c),phase_(p),observe_(o){}
  void begin_local_scratch()override{c_.stack_pointer=c_.entry_stack-4;put(0x1da530u,c_.stack_pointer,c_.scratch_word);}
  float tested_exponent()const override{return scalar(0x1da531u,argument(32));}
  float epsilon(SourceLiftScalarEpsilonUse use)const override{
    constexpr std::array sites{0x1da537u,0x1da58eu,0x1da5c2u,0x1da6b5u};return scalar(sites[unsigned(use)],0x46a180u);
  }
  float target_for_power()const override{return scalar(0x1da540u,argument(12));}
  void power_base(float value,bool negated)override{power_branch_=true;put_float(negated?0x1da559u:0x1da54eu,c_.entry_stack-4,value);}
  float power_base()const override{return scalar(0x1da55eu,c_.entry_stack-4);}
  float exponent()const override{return scalar(0x1da561u,argument(32));}
  float acceleration()const override{return scalar(power_branch_?0x1da56au:0x1da57au,argument(16));}
  void power_acceleration(float value)override{put_float(0x1da56eu,argument(32),value);}
  float power_acceleration()const override{return scalar(0x1da572u,argument(32));}
  bool scale_by_mass()const override{return word(0x1da580u,argument(4),1)!=0;}
  float mass()const override{return scalar(0x1da588u,argument(8));}
  float seconds()override{clock_cell_=word(0x1da59bu,0x51df54u)+4;return scalar(0x1da5a0u,clock_cell_);}
  float braking_seconds()const override{return scalar(0x1da60du,clock_cell_);}
  void step(float value)override{put_float(0x1da5aeu,argument(4),value);}
  float target_for_distance()const override{return scalar(0x1da5c9u,argument(12));}
  float position()const override{return scalar(0x1da5cfu,argument(28));}
  float velocity_limit()const override{return scalar(0x1da5d5u,argument(24));}
  float velocity()const override{return scalar(0x1da5dbu,argument(20));}
  void distance(float value)override{put_float(0x1da5e1u,argument(12),value);}
  bool force_acceleration()const override{return word(0x1da5edu,argument(36),1)!=0;}
  float one()const override{return scalar(0x1da602u,0x46a0d0u);}
  float half()const override{return scalar(0x1da615u,0x46a0ccu);}
  float braking_step()const override{return scalar(0x1da637u,argument(4));}
  float braking_distance()const override{return scalar(0x1da63bu,argument(12));}
  void braking_result(float value)override{put_float(0x1da643u,argument(4),value);}
  float braking_result()const override{return scalar(0x1da647u,argument(4));}
  bool allow_reverse()const override{return c_.allow_reverse;}
  float reverse_minimum()const override{return scalar(0x1da64fu,0x44aac8u);}
  float reverse_maximum()const override{return scalar(0x1da685u,0x45695cu);}
  void prepare_ratio_destination()override{ratio_cell_=word(0x1da6dbu,argument(40));}
  void ratio(float value)override{put_float(0x1da6eau,ratio_cell_,value);}
  void finish_local_scratch(bool minimum)override{
    c_.scratch_word=word(minimum?0x1da6f3u:0x1da6ffu,c_.entry_stack-4);c_.scratch_popped=true;c_.stack_pointer=c_.entry_stack;
    const auto continuation=word(minimum?0x1da6f4u:0x1da700u,c_.entry_stack);
    if(continuation!=c_.continuation)throw std::runtime_error("Native lift scalar retained continuation changed");
    c_.stack_pointer=c_.entry_stack+44;
  }
  void errno_value(std::int32_t value){
    if(c_.executor!=std::this_thread::get_id())throw std::runtime_error("Native lift scalar errno belongs to another executor");
    if(word(0x330179u,c_.fs_base+0x24u,1)>=2)throw std::runtime_error("Native lift scalar requires original elevated-IRQL error owner");
    const auto thread=word(0x33018fu,c_.fs_base+0x28u);std::uint32_t holder=0x573840u;
    if(word(0x330195u,thread+0x28u)){
      const auto index=word(0x33019bu,0x48c920u),table=word(0x3301a0u,c_.fs_base+4u);
      holder=word(0x3301a7u,table+index*4u)+0xcu;
    }
    const auto data=word(0x3301b7u,holder);
    if(!data)throw std::runtime_error("Native lift scalar requires native CRT thread-data allocation owner00332383");
    put(value==33?0x32e9c3u:0x32e9b7u,data+8u,std::bit_cast<std::uint32_t>(value));note_errno(m_);
  }
};
}
NativeLiftScalarConsumerScopeTu15::NativeLiftScalarConsumerScopeTu15(SourcePhysicsContactMemory&m,NativeLiftScalarConsumerPolicyTu15 policy)
  :memory_(&m),policy_(std::move(policy)),executor_(std::this_thread::get_id()),previous_(active_scope){
  if(policy_.caller_stack_begin>=policy_.caller_stack_end)throw std::invalid_argument("Native lift scalar scope requires its actual caller stack interval");
  active_scope=this;
}
NativeLiftScalarConsumerScopeTu15::~NativeLiftScalarConsumerScopeTu15(){if(executor_!=std::this_thread::get_id()||active_scope!=this)std::terminate();active_scope=previous_;}
const NativeLiftScalarConsumerPolicyTu15* native_lift_scalar_consumer_policy_tu15(const SourcePhysicsContactMemory&m){
  for(auto*p=active_scope;p;p=p->previous_)if(p->memory_==&m&&p->executor_==std::this_thread::get_id())return &p->policy_;return nullptr;
}
NativeLiftScalarActivityTu15 native_lift_scalar_activity_tu15(const SourcePhysicsContactMemory&m){std::lock_guard lock(activity_mutex);const auto at=memory_activity.find(&m);return at==memory_activity.end()?NativeLiftScalarActivityTu15{}:at->second;}
void native_lift_scalar_activity_reset_tu15(const SourcePhysicsContactMemory&m){std::lock_guard lock(activity_mutex);memory_activity.erase(&m);}
NativeLiftScalarActivityTu15 native_lift_scalar_activity_tu15(){
  NativeLiftScalarActivityTu15 result;for(unsigned n=0;n<4;++n)result.caller_calls[n]=calls[n].load(std::memory_order_relaxed);
  result.power_calls=powers.load(std::memory_order_relaxed);result.errno_writes=errnos.load(std::memory_order_relaxed);result.observation_refusals=refusals.load(std::memory_order_relaxed);return result;
}
void native_lift_scalar_observation_refused_tu15(){refusals.fetch_add(1,std::memory_order_relaxed);}
void native_lift_scalar_observation_refused_tu15(const SourcePhysicsContactMemory&m){
  native_lift_scalar_observation_refused_tu15();std::lock_guard lock(activity_mutex);++memory_activity[&m].observation_refusals;
  if(const auto*p=native_lift_scalar_consumer_policy_tu15(m);p&&p->world_activity)++p->world_activity->observation_refusals;
}
SourceLiftScalarOutcome native_lift_scalar_consumer_tu15(SourcePhysicsContactMemory&m,NativeLiftScalarConsumerTu15&c,
    const std::function<void(std::uint32_t)>&phase,const SourcePhysicsContactRecordObserver&observe){
  constexpr std::array callers{0x1dabbeu,0x1daf99u,0x1db2d0u,0x1db4dbu};
  unsigned index{};while(index<callers.size()&&callers[index]!=c.call_site)++index;
  if(index==callers.size())throw std::runtime_error("Unproved native lift scalar consumer");
  Storage storage(m,c,phase,observe);
  SourceMathErrorServicesTu15 services;services.store_error_number=[&](std::int32_t value){storage.errno_value(value);};
  const SourceLiftPowerLazyTu15 power=[&](float base,float exponent,const SourceLiftAccelerationLoad&acceleration){
    note_power(m);return source_lift_power_tu15_current_lazy(base,exponent,acceleration,services);
  };
  const auto result=source_lift_scalar_storage_tu15(storage,c.floating_point,power);
  calls[index].fetch_add(1,std::memory_order_relaxed);{std::lock_guard lock(activity_mutex);++memory_activity[&m].caller_calls[index];if(const auto*p=native_lift_scalar_consumer_policy_tu15(m);p&&p->world_activity)++p->world_activity->caller_calls[index];}
  c.floating_point={result.control_word,result.mxcsr,std::uint8_t(result.x87_status&0x3fu),result.x87_status};c.numeric_status=result.x87_status;return result;
}
}
