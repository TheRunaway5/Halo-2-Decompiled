#include "halo2/native_constraint_schema_emitter_consumer_tu15.hpp"
#include <algorithm>
#include <map>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>
namespace halo2 {
namespace {
thread_local NativeConstraintSchemaEmitterConsumerScopeTu15*active{};
struct Registration {const NativeConstraintSchemaEmitterConsumerScopeTu15*scope;NativeConstraintSchemaEmitterActivityTu15*activity;};
std::mutex mutex;std::map<const SourcePhysicsContactMemory*,std::vector<Registration>>registered;
std::map<const SourcePhysicsContactMemory*,NativeConstraintSchemaEmitterActivityTu15>activity;
template<class Function>void record(const SourcePhysicsContactMemory&m,Function function){
    std::lock_guard lock(mutex);function(activity[&m]);
    if(const auto i=registered.find(&m);i!=registered.end()){
        std::vector<NativeConstraintSchemaEmitterActivityTu15*>seen;
        for(const auto&r:i->second)if(r.activity&&std::find(seen.begin(),seen.end(),r.activity)==seen.end()){seen.push_back(r.activity);function(*r.activity);}
    }
}
}
NativeConstraintSchemaEmitterConsumerScopeTu15::NativeConstraintSchemaEmitterConsumerScopeTu15(SourcePhysicsContactMemory&m,NativeConstraintSchemaEmitterConsumerPolicyTu15 policy)
 :memory_(&m),policy_(std::move(policy)),executor_(std::this_thread::get_id()),previous_(active){
    if(policy_.caller_stack_begin>=policy_.caller_stack_end||!policy_.current_actual_phase||!policy_.publish_phase)throw std::invalid_argument("Native constraint schema emitter scope requires actual caller provenance, phase publisher and retained stack bounds");
    {std::lock_guard lock(mutex);registered[&m].push_back({this,policy_.world_activity});}active=this;
}
NativeConstraintSchemaEmitterConsumerScopeTu15::~NativeConstraintSchemaEmitterConsumerScopeTu15(){
    if(executor_!=std::this_thread::get_id()||active!=this)std::terminate();
    {std::lock_guard lock(mutex);auto i=registered.find(memory_);if(i==registered.end())std::terminate();auto&r=i->second;r.erase(std::remove_if(r.begin(),r.end(),[&](const auto&item){return item.scope==this;}),r.end());if(r.empty())registered.erase(i);}active=previous_;
}
const NativeConstraintSchemaEmitterConsumerPolicyTu15*native_constraint_schema_emitter_policy_tu15(const SourcePhysicsContactMemory&m){for(auto*p=active;p;p=p->previous_)if(p->memory_==&m&&p->executor_==std::this_thread::get_id())return&p->policy_;return nullptr;}
bool native_constraint_schema_emitter_bank_scoped_tu15(const SourcePhysicsContactMemory&m){std::lock_guard lock(mutex);return registered.contains(&m);}
NativeConstraintSchemaEmitterActivityTu15 native_constraint_schema_emitter_activity_tu15(const SourcePhysicsContactMemory&m){std::lock_guard lock(mutex);const auto i=activity.find(&m);return i==activity.end()?NativeConstraintSchemaEmitterActivityTu15{}:i->second;}
void native_constraint_schema_emitter_activity_reset_tu15(const SourcePhysicsContactMemory&m){std::lock_guard lock(mutex);activity.erase(&m);}
void native_constraint_schema_emitter_call_tu15(const SourcePhysicsContactMemory&m,unsigned caller){record(m,[&](auto&a){++a.calls;++a.live_cursor_calls;++a.caller_kinds.at(caller);});}
void native_constraint_schema_emitter_completed_tu15(const SourcePhysicsContactMemory&m){record(m,[](auto&a){++a.completed_calls;});}
void native_constraint_schema_emitter_refused_tu15(const SourcePhysicsContactMemory&m,NativeConstraintSchemaEmitterRefusalTu15 why){record(m,[&](auto&a){++a.refusals;switch(why){case NativeConstraintSchemaEmitterRefusalTu15::Observation:++a.observation_refusals;break;case NativeConstraintSchemaEmitterRefusalTu15::Caller:++a.caller_refusals;break;case NativeConstraintSchemaEmitterRefusalTu15::Stack:++a.stack_refusals;break;case NativeConstraintSchemaEmitterRefusalTu15::Executor:++a.executor_refusals;break;case NativeConstraintSchemaEmitterRefusalTu15::Image:++a.image_refusals;break;}});}
} // namespace halo2
