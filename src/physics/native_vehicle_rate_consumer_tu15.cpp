#include "halo2/native_vehicle_rate_consumer_tu15.hpp"
#include <map>
#include <mutex>
#include <stdexcept>
#include <utility>
namespace halo2 {
namespace {
thread_local NativeVehicleRateConsumerScopeTu15*active{};
std::mutex activity_mutex;std::map<const SourcePhysicsContactMemory*,NativeVehicleRateActivityTu15>activity;
void increment(NativeVehicleRateActivityTu15&a,bool approach,bool cursor){
 if(approach)++a.approach_calls;else++a.acceleration_calls;if(cursor)++a.live_cursor_calls;
}
}
NativeVehicleRateConsumerScopeTu15::NativeVehicleRateConsumerScopeTu15(SourcePhysicsContactMemory&m,NativeVehicleRateConsumerPolicyTu15 p)
 :memory_(&m),policy_(std::move(p)),executor_(std::this_thread::get_id()),previous_(active){
 if(policy_.caller_stack_begin>=policy_.caller_stack_end||!policy_.publish_phase)
  throw std::invalid_argument("Native vehicle rate scope requires retained caller stack bounds and an actual phase publisher");
 active=this;
}
NativeVehicleRateConsumerScopeTu15::~NativeVehicleRateConsumerScopeTu15(){
 if(executor_!=std::this_thread::get_id()||active!=this)std::terminate();active=previous_;
}
const NativeVehicleRateConsumerPolicyTu15*native_vehicle_rate_consumer_policy_tu15(const SourcePhysicsContactMemory&m){
 for(auto*p=active;p;p=p->previous_)if(p->memory_==&m&&p->executor_==std::this_thread::get_id())return&p->policy_;return nullptr;
}
NativeVehicleRateActivityTu15 native_vehicle_rate_activity_tu15(const SourcePhysicsContactMemory&m){
 std::lock_guard lock(activity_mutex);const auto p=activity.find(&m);return p==activity.end()?NativeVehicleRateActivityTu15{}:p->second;
}
void native_vehicle_rate_activity_reset_tu15(const SourcePhysicsContactMemory&m){std::lock_guard lock(activity_mutex);activity.erase(&m);}
void native_vehicle_rate_call_tu15(const SourcePhysicsContactMemory&m,bool approach,bool cursor){
 std::lock_guard lock(activity_mutex);increment(activity[&m],approach,cursor);
 if(const auto*p=native_vehicle_rate_consumer_policy_tu15(m);p&&p->world_activity)increment(*p->world_activity,approach,cursor);
}
void native_vehicle_rate_observation_refused_tu15(const SourcePhysicsContactMemory&m){
 std::lock_guard lock(activity_mutex);++activity[&m].observation_refusals;
 if(const auto*p=native_vehicle_rate_consumer_policy_tu15(m);p&&p->world_activity)++p->world_activity->observation_refusals;
}
} // namespace halo2
