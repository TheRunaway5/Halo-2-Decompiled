// Shared by the ordinary memory owner and its ABI-compatible delivery freeze.
#include <mutex>
#include <shared_mutex>
#include <limits>
#include <cstdlib>
namespace halo2 {
namespace {
struct NativeWriteGuards {
    struct WriteEpoch {};
    std::shared_mutex mutex;
    std::map<const SourceRuntimeMemory *,std::map<const void *,SourceRuntimeWriteGuard>> banks;
    std::map<const SourceRuntimeMemory *,std::map<const void *,SourceRuntimeWriteGuard>> observers;
    std::map<const SourceRuntimeMemory *,std::map<const void *,SourceRuntimeWriteGuard>> topology;
    std::map<const SourceRuntimeMemory *,std::map<const void *,SourceRuntimeWriteGuard>> topology_observers;
    struct PolicyState {const SourceRuntimePolicyHold *held{};std::size_t changes{};};
    std::map<const SourceRuntimeMemory *,PolicyState> policies;
    // A retained identity avoids wraparound and address-reuse ambiguity in
    // per-thread dispatch caches. Only successful write-policy changes retire
    // it; publication admission and topology-only policies do not change it.
    std::shared_ptr<const WriteEpoch> write_epoch=std::make_shared<WriteEpoch>();
    const bool reference_dispatch=[] {const auto *p=std::getenv("HALO2_REFERENCE_WRITE_DISPATCH");return p && *p;}();
};
NativeWriteGuards &native_write_guards(){static NativeWriteGuards guards;return guards;}
void require_policy_change(const NativeWriteGuards &registry,const SourceRuntimeMemory &memory) {
    const auto policy=registry.policies.find(&memory);
    if(policy!=registry.policies.end() && policy->second.held)
        throw std::logic_error("Native publication cannot change source policies");
}
using PolicyMap=std::map<const SourceRuntimeMemory *,std::map<const void *,SourceRuntimeWriteGuard>>;
bool write_policy(PolicyMap NativeWriteGuards::*member) {
    return member==&NativeWriteGuards::banks || member==&NativeWriteGuards::observers;
}
void install_policy(PolicyMap NativeWriteGuards::*member,const SourceRuntimeMemory &memory,const void *owner,
    SourceRuntimeWriteGuard &argument,const char *missing,const char *duplicate) {
    SourceRuntimePolicyChange admission(memory);
    SourceRuntimeWriteGuard incoming;incoming.swap(argument);
    if(!owner || !incoming)throw std::invalid_argument(missing);
    const auto epoch=write_policy(member)?std::make_shared<NativeWriteGuards::WriteEpoch>():nullptr;
    auto &registry=native_write_guards();
    {
        std::unique_lock lock(registry.mutex);
        // try_emplace leaves incoming untouched on duplicate. A rejected or
        // unconsumed capture retires after unlocking, before admission ends.
        if(!(registry.*member)[&memory].try_emplace(owner,std::move(incoming)).second)
            throw std::logic_error(duplicate);
        if(epoch)registry.write_epoch=epoch;
    }
}
void remove_policy(PolicyMap NativeWriteGuards::*member,const SourceRuntimeMemory &memory,const void *owner) {
    SourceRuntimePolicyChange admission(memory);
    const auto epoch=write_policy(member)?std::make_shared<NativeWriteGuards::WriteEpoch>():nullptr;
    auto &registry=native_write_guards();
    std::map<const void *,SourceRuntimeWriteGuard>::node_type retired;
    {
        std::unique_lock lock(registry.mutex);
        auto &policies=registry.*member;
        const auto bank=policies.find(&memory);if(bank==policies.end())return;
        retired=bank->second.extract(owner);
        if(bank->second.empty())policies.erase(bank);
        if(epoch && retired)registry.write_epoch=epoch;
    }
    // Captures may perform legitimate retirement work. Do not call them under
    // the registry mutex, and do not let publication overtake that retirement.
}
}
SourceRuntimePolicyHold::SourceRuntimePolicyHold(const SourceRuntimeMemory &memory):memory_(&memory) {
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    auto &policy=registry.policies[&memory];
    if(policy.changes || policy.held)
        throw std::logic_error("Native publication requires quiescent source policies");
    policy.held=this;
}
SourceRuntimePolicyHold::SourceRuntimePolicyHold(SourceRuntimePolicyChange &change):memory_(change.memory_) {
    if(std::this_thread::get_id()!=change.executor_ || !change.active_)
        throw std::logic_error("Native publication requires its active creator policy admission");
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    const auto found=registry.policies.find(memory_);
    if(found==registry.policies.end() || found->second.changes!=1 || found->second.held)
        throw std::logic_error("Native publication requires its sole policy admission");
    found->second.changes=0;found->second.held=this;
    change.active_=false;
}
SourceRuntimePolicyHold::~SourceRuntimePolicyHold() {
    if(std::this_thread::get_id()!=executor_)std::terminate();
    if(!active_)return;
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    const auto found=registry.policies.find(memory_);
    if(found==registry.policies.end() || found->second.held!=this || found->second.changes)std::terminate();
    registry.policies.erase(found);
}
SourceRuntimePolicyChange::SourceRuntimePolicyChange(const SourceRuntimeMemory &memory):memory_(&memory) {
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    require_policy_change(registry,memory);
    auto &count=registry.policies[&memory].changes;
    if(count==std::numeric_limits<std::size_t>::max())
        throw std::overflow_error("Native source policy admission count exhausted");
    ++count;
}
SourceRuntimePolicyChange::SourceRuntimePolicyChange(SourceRuntimePolicyHold &hold):memory_(hold.memory_) {
    if(std::this_thread::get_id()!=hold.executor_ || !hold.active_)
        throw std::logic_error("Native publication requires its active creator policy hold");
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    const auto found=registry.policies.find(memory_);
    if(found==registry.policies.end() || found->second.held!=&hold || found->second.changes)
        throw std::logic_error("Native publication lost its exact retained policy hold");
    found->second.held=nullptr;found->second.changes=1;
    hold.active_=false;
}
SourceRuntimePolicyChange::~SourceRuntimePolicyChange() {
    if(std::this_thread::get_id()!=executor_)std::terminate();
    if(!active_)return;
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    const auto found=registry.policies.find(memory_);
    if(found==registry.policies.end() || !found->second.changes || found->second.held)std::terminate();
    if(!--found->second.changes)registry.policies.erase(found);
}
void source_runtime_register_write_guard(const SourceRuntimeMemory &memory,const void *owner,
    SourceRuntimeWriteGuard guard) {
    install_policy(&NativeWriteGuards::banks,memory,owner,guard,
        "Native write guard requires its actual resource owner","Native resource write guard already registered");
}
void source_runtime_unregister_write_guard(const SourceRuntimeMemory &memory,const void *owner) {
    remove_policy(&NativeWriteGuards::banks,memory,owner);
}
void source_runtime_validate_write(const SourceRuntimeMemory &memory,std::uint32_t address,std::size_t bytes) {
    source_runtime_validate_write(memory,address,bytes,true);
}
void source_runtime_register_write_observer(const SourceRuntimeMemory &memory,const void *owner,SourceRuntimeWriteGuard observer){
    install_policy(&NativeWriteGuards::observers,memory,owner,observer,
        "Native write observer requires its owner","Native write observer already registered");
}
void source_runtime_unregister_write_observer(const SourceRuntimeMemory &memory,const void *owner){
    remove_policy(&NativeWriteGuards::observers,memory,owner);
}
void source_runtime_validate_write(const SourceRuntimeMemory &memory,std::uint32_t address,std::size_t bytes,bool observe) {
    if(!bytes)return;
    auto &registry=native_write_guards();std::shared_lock lock(registry.mutex);
    struct Dispatch {
        const SourceRuntimeMemory *memory{};
        std::shared_ptr<const NativeWriteGuards::WriteEpoch> epoch;
        std::array<const SourceRuntimeWriteGuard *,16> guards{},observers{};
        std::size_t guard_count{},observer_count{};
        bool complete{},active{};
    };
    thread_local Dispatch cached;
    // The shared lock still spans every callback. Policy retirement cannot
    // overtake a borrowed function. Recursive writes use the original lookup
    // so another bank cannot replace the outer call's cached pointer arrays.
    if(!registry.reference_dispatch && !cached.active) {
        if(cached.memory!=&memory || cached.epoch!=registry.write_epoch) {
            cached.memory=&memory;cached.epoch=registry.write_epoch;
            cached.guard_count=cached.observer_count=0;cached.complete=true;
            const auto collect=[&](const PolicyMap &policies,auto &out,auto &count) {
                if(const auto bank=policies.find(&memory);bank!=policies.end()) {
                    if(bank->second.size()>out.size()){cached.complete=false;return;}
                    for(const auto &[owner,callback]:bank->second){(void)owner;out[count++]=&callback;}
                }
            };
            collect(registry.banks,cached.guards,cached.guard_count);
            collect(registry.observers,cached.observers,cached.observer_count);
        }
        if(cached.complete) {
            struct Active {bool &flag;explicit Active(bool &p):flag(p){flag=true;}~Active(){flag=false;}} active(cached.active);
            for(std::size_t i=0;i<cached.guard_count;++i)(*cached.guards[i])(address,bytes);
            if(observe)for(std::size_t i=0;i<cached.observer_count;++i)(*cached.observers[i])(address,bytes);
            return;
        }
    }
    if(const auto bank=registry.banks.find(&memory);bank!=registry.banks.end())
        for(const auto &[owner,guard]:bank->second){(void)owner;guard(address,bytes);}
    if(observe)if(const auto bank=registry.observers.find(&memory);bank!=registry.observers.end())
        for(const auto &[owner,observer]:bank->second){(void)owner;observer(address,bytes);}
}
void source_runtime_register_topology_guard(const SourceRuntimeMemory &memory,const void *owner,SourceRuntimeWriteGuard guard){
    install_policy(&NativeWriteGuards::topology,memory,owner,guard,
        "Native topology guard requires its owner","Native topology guard already registered");
}
void source_runtime_unregister_topology_guard(const SourceRuntimeMemory &memory,const void *owner){
    remove_policy(&NativeWriteGuards::topology,memory,owner);
}
void source_runtime_validate_topology(const SourceRuntimeMemory &memory,std::uint32_t address,std::size_t bytes){
    auto &registry=native_write_guards();std::shared_lock lock(registry.mutex);
    const auto bank=registry.topology.find(&memory);if(bank==registry.topology.end())return;
    for(const auto &[owner,guard]:bank->second){(void)owner;guard(address,bytes);}
}
void source_runtime_register_topology_observer(const SourceRuntimeMemory &memory,const void *owner,SourceRuntimeWriteGuard observer){
    install_policy(&NativeWriteGuards::topology_observers,memory,owner,observer,
        "Native topology observer requires its owner","Native topology observer already registered");
}
void source_runtime_unregister_topology_observer(const SourceRuntimeMemory &memory,const void *owner){
    remove_policy(&NativeWriteGuards::topology_observers,memory,owner);
}
void source_runtime_observe_topology(const SourceRuntimeMemory &memory,std::uint32_t address,std::size_t bytes) noexcept {
    auto &registry=native_write_guards();std::shared_lock lock(registry.mutex);
    const auto bank=registry.topology_observers.find(&memory);if(bank==registry.topology_observers.end())return;
    for(const auto &[owner,observer]:bank->second){(void)owner;observer(address,bytes);}
}
} // namespace halo2
