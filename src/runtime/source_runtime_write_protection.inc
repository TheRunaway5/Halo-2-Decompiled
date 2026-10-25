// Shared by the ordinary memory owner and its ABI-compatible delivery freeze.
#include <mutex>
#include <shared_mutex>
#include <limits>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <atomic>
#if defined(__linux__)
#include <linux/membarrier.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace halo2 {
struct SourceRuntimeWriteSerial::Counters {
    // Dispatch owner stores alone (single writer); other threads use RMW.
    std::atomic<std::uint64_t> owner{0},shared{0};
};
namespace {
struct NativeWriteGuards {
    struct WriteEpoch {};
    std::shared_mutex mutex;
    std::map<const SourceRuntimeMemory *,std::map<const void *,SourceRuntimeWriteGuard>> banks;
    // Writes ending at or below a guard's bound cannot reach its protection.
    std::map<const SourceRuntimeMemory *,std::map<const void *,std::uint64_t>> guard_bounds;
    // One counter per bank replaces observers whose only effect is a serial.
    struct WriteSerial {std::shared_ptr<SourceRuntimeWriteSerial::Counters> count;std::size_t subscribers{};};
    std::map<const SourceRuntimeMemory *,WriteSerial> serials;
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
    // Write dispatch borrows policy callbacks through a per-thread slot rather
    // than the shared mutex. Write-policy changes quiesce every active slot
    // while holding the unique mutex, as draining shared readers did before.
    struct alignas(64) DispatchSlot {std::atomic<bool> active{false};unsigned depth{};bool owner{};};
    std::mutex slot_mutex;
    std::vector<const DispatchSlot *> slots;
    // One dispatching thread may publish its slot with only a compiler fence;
    // a policy writer on any other thread issues the process-wide barrier.
    const bool asymmetric_barrier=[] {
#if defined(__linux__) && defined(__NR_membarrier)
        return syscall(__NR_membarrier,MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED,0,0)==0;
#else
        return false;
#endif
    }();
    const DispatchSlot *owner{}; // Guarded by slot_mutex.
    std::atomic<bool> owner_vacant{asymmetric_barrier};
    std::atomic<bool> writer{false};
};
NativeWriteGuards &native_write_guards(){static NativeWriteGuards guards;return guards;}
struct DispatchSlotOwner {
    NativeWriteGuards &registry;
    NativeWriteGuards::DispatchSlot slot;
    explicit DispatchSlotOwner(NativeWriteGuards &owner):registry(owner) {
        std::lock_guard lock(registry.slot_mutex);registry.slots.push_back(&slot);
    }
    ~DispatchSlotOwner() {
        std::lock_guard lock(registry.slot_mutex);std::erase(registry.slots,&slot);
        if(registry.owner==&slot){registry.owner=nullptr;registry.owner_vacant.store(true,std::memory_order_relaxed);}
    }
};
NativeWriteGuards::DispatchSlot &dispatch_slot(NativeWriteGuards &registry) {
    thread_local DispatchSlotOwner owner(registry);return owner.slot;
}
void claim_dispatch_owner(NativeWriteGuards &registry,NativeWriteGuards::DispatchSlot &slot) {
    std::lock_guard lock(registry.slot_mutex);
    if(registry.owner || !registry.asymmetric_barrier)return;
    registry.owner=&slot;slot.owner=true;registry.owner_vacant.store(false,std::memory_order_relaxed);
}
// Held with the unique registry mutex for every dispatch-visible change.
// New dispatchers see the writer flag and wait on the shared mutex instead.
struct WriteQuiescence {
    NativeWriteGuards &registry;
    explicit WriteQuiescence(NativeWriteGuards &owner):registry(owner) {
        // A shared-locked callback could never acquire the unique mutex; keep
        // that exclusion explicit instead of waiting on this thread's slot.
        const auto &mine=dispatch_slot(registry);
        if(mine.depth)
            throw std::logic_error("Native write policy cannot change during its own write dispatch");
        std::lock_guard lock(registry.slot_mutex);
        registry.writer.store(true,std::memory_order_seq_cst);
#if defined(__linux__) && defined(__NR_membarrier)
        // The owner's slot store and writer load are separated only by a
        // compiler fence; this barrier orders them against the flag above.
        if(registry.owner && registry.owner!=&mine &&
            syscall(__NR_membarrier,MEMBARRIER_CMD_PRIVATE_EXPEDITED,0,0)!=0)
            std::terminate();
#endif
        for(const auto *slot:registry.slots)
            if(slot!=&mine)while(slot->active.load(std::memory_order_seq_cst))std::this_thread::yield();
    }
    ~WriteQuiescence(){registry.writer.store(false,std::memory_order_seq_cst);}
};
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
    SourceRuntimeWriteGuard &argument,const char *missing,const char *duplicate,std::uint64_t bound=0) {
    SourceRuntimePolicyChange admission(memory);
    SourceRuntimeWriteGuard incoming;incoming.swap(argument);
    if(!owner || !incoming)throw std::invalid_argument(missing);
    const auto epoch=write_policy(member)?std::make_shared<NativeWriteGuards::WriteEpoch>():nullptr;
    auto &registry=native_write_guards();
    {
        std::unique_lock lock(registry.mutex);
        WriteQuiescence quiescent(registry);
        // try_emplace leaves incoming untouched on duplicate. A rejected or
        // unconsumed capture retires after unlocking, before admission ends.
        if(!(registry.*member)[&memory].try_emplace(owner,std::move(incoming)).second)
            throw std::logic_error(duplicate);
        if(member==&NativeWriteGuards::banks && bound)
            try{registry.guard_bounds[&memory][owner]=bound;}
            catch(...){auto &bank=registry.banks[&memory];bank.erase(owner);if(bank.empty())registry.banks.erase(&memory);throw;}
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
        WriteQuiescence quiescent(registry);
        auto &policies=registry.*member;
        const auto bank=policies.find(&memory);if(bank==policies.end())return;
        retired=bank->second.extract(owner);
        if(bank->second.empty())policies.erase(bank);
        if(member==&NativeWriteGuards::banks)
            if(const auto bounds=registry.guard_bounds.find(&memory);bounds!=registry.guard_bounds.end()) {
                bounds->second.erase(owner);if(bounds->second.empty())registry.guard_bounds.erase(bounds);
            }
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
void source_runtime_register_write_guard(const SourceRuntimeMemory &memory,const void *owner,
    SourceRuntimeWriteGuard guard,std::uint64_t inert_through) {
    install_policy(&NativeWriteGuards::banks,memory,owner,guard,
        "Native write guard requires its actual resource owner","Native resource write guard already registered",
        inert_through);
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
namespace {
void count_write(SourceRuntimeWriteSerial::Counters &counters,bool owner) noexcept {
    if(owner)counters.owner.store(counters.owner.load(std::memory_order_relaxed)+1,std::memory_order_relaxed);
    else counters.shared.fetch_add(1,std::memory_order_relaxed);
}
void dispatch_write(NativeWriteGuards &registry,const SourceRuntimeMemory &memory,std::uint32_t address,std::size_t bytes,bool observe,bool owner) {
    struct Dispatch {
        const SourceRuntimeMemory *memory{};
        std::shared_ptr<const NativeWriteGuards::WriteEpoch> epoch;
        std::array<const SourceRuntimeWriteGuard *,16> guards{},observers{};
        std::array<std::uint64_t,16> bounds{};
        SourceRuntimeWriteSerial::Counters *serial{};
        std::size_t guard_count{},observer_count{};
        bool complete{},active{};
    };
    thread_local Dispatch cached;
    const auto limit=std::uint64_t(address)+bytes;
    // The borrowed callbacks stay alive until this dispatch completes. Recursive
    // writes use the original lookup so another bank cannot replace the outer
    // call's cached pointer arrays.
    if(!registry.reference_dispatch && !cached.active) {
        if(cached.memory!=&memory || cached.epoch!=registry.write_epoch) {
            cached.memory=&memory;cached.epoch=registry.write_epoch;
            cached.guard_count=cached.observer_count=0;cached.complete=true;cached.serial=nullptr;
            const auto bounds=registry.guard_bounds.find(&memory);
            if(const auto bank=registry.banks.find(&memory);bank!=registry.banks.end()) {
                if(bank->second.size()>cached.guards.size())cached.complete=false;
                else for(const auto &[owner,callback]:bank->second) {
                    std::uint64_t bound=0;
                    if(bounds!=registry.guard_bounds.end())
                        if(const auto found=bounds->second.find(owner);found!=bounds->second.end())bound=found->second;
                    cached.bounds[cached.guard_count]=bound;cached.guards[cached.guard_count++]=&callback;
                }
            }
            if(const auto bank=registry.observers.find(&memory);bank!=registry.observers.end()) {
                if(bank->second.size()>cached.observers.size())cached.complete=false;
                else for(const auto &[owner,callback]:bank->second){(void)owner;cached.observers[cached.observer_count++]=&callback;}
            }
            if(const auto serial=registry.serials.find(&memory);serial!=registry.serials.end())
                cached.serial=serial->second.count.get();
        }
        if(cached.complete) {
            struct Active {bool &flag;explicit Active(bool &p):flag(p){flag=true;}~Active(){flag=false;}} active(cached.active);
            for(std::size_t i=0;i<cached.guard_count;++i)if(limit>cached.bounds[i])(*cached.guards[i])(address,bytes);
            if(observe) {
                if(cached.serial)count_write(*cached.serial,owner);
                for(std::size_t i=0;i<cached.observer_count;++i)(*cached.observers[i])(address,bytes);
            }
            return;
        }
    }
    if(const auto bank=registry.banks.find(&memory);bank!=registry.banks.end()) {
        const auto bounds=registry.guard_bounds.find(&memory);
        for(const auto &[owner,guard]:bank->second) {
            if(bounds!=registry.guard_bounds.end())
                if(const auto found=bounds->second.find(owner);found!=bounds->second.end() && limit<=found->second)continue;
            guard(address,bytes);
        }
    }
    if(!observe)return;
    if(const auto serial=registry.serials.find(&memory);serial!=registry.serials.end())
        count_write(*serial->second.count,owner);
    if(const auto bank=registry.observers.find(&memory);bank!=registry.observers.end())
        for(const auto &[owner,observer]:bank->second){(void)owner;observer(address,bytes);}
}
}
void source_runtime_validate_write(const SourceRuntimeMemory &memory,std::uint32_t address,std::size_t bytes,bool observe) {
    if(!bytes)return;
    auto &registry=native_write_guards();
    auto &slot=dispatch_slot(registry);
    if(!registry.reference_dispatch) {
        if(!slot.depth && !slot.owner && registry.owner_vacant.load(std::memory_order_relaxed))
            claim_dispatch_owner(registry,slot);
        // Dekker handshake with WriteQuiescence: publish this slot, then check
        // for a writer; a writer publishes its flag, then drains every slot.
        bool borrowed=slot.depth!=0;
        if(!borrowed) {
            if(slot.owner) {
                slot.active.store(true,std::memory_order_relaxed);
                std::atomic_signal_fence(std::memory_order_seq_cst);
                borrowed=!registry.writer.load(std::memory_order_acquire);
            } else {
                slot.active.exchange(true,std::memory_order_seq_cst);
                borrowed=!registry.writer.load(std::memory_order_seq_cst);
            }
            if(!borrowed)slot.active.store(false,std::memory_order_release);
        }
        if(borrowed) {
            struct Borrow {
                NativeWriteGuards::DispatchSlot &slot;
                explicit Borrow(NativeWriteGuards::DispatchSlot &p):slot(p){++slot.depth;}
                ~Borrow(){if(!--slot.depth)slot.active.store(false,std::memory_order_release);}
            } borrow(slot);
            dispatch_write(registry,memory,address,bytes,observe,slot.owner);
            return;
        }
    }
    std::shared_lock lock(registry.mutex);
    dispatch_write(registry,memory,address,bytes,observe,slot.owner);
}
SourceRuntimeWriteSerial::SourceRuntimeWriteSerial(const SourceRuntimeMemory &memory):memory_(&memory) {
    SourceRuntimePolicyChange admission(memory);
    const auto epoch=std::make_shared<NativeWriteGuards::WriteEpoch>();
    auto created=std::make_shared<Counters>();
    auto &registry=native_write_guards();
    std::unique_lock lock(registry.mutex);
    WriteQuiescence quiescent(registry);
    auto &serial=registry.serials[&memory];
    if(!serial.count)serial.count=std::move(created);
    ++serial.subscribers;
    counters_=serial.count;
    base_=serial.count->owner.load(std::memory_order_relaxed)+serial.count->shared.load(std::memory_order_relaxed);
    registry.write_epoch=epoch;
}
SourceRuntimeWriteSerial::~SourceRuntimeWriteSerial() {
    SourceRuntimePolicyChange admission(*memory_);
    const auto epoch=std::make_shared<NativeWriteGuards::WriteEpoch>();
    auto &registry=native_write_guards();
    std::unique_lock lock(registry.mutex);
    WriteQuiescence quiescent(registry);
    const auto serial=registry.serials.find(memory_);
    if(serial!=registry.serials.end() && !--serial->second.subscribers)registry.serials.erase(serial);
    registry.write_epoch=epoch;
}
std::uint64_t SourceRuntimeWriteSerial::load() const noexcept {
    return counters_->owner.load(std::memory_order_relaxed)+counters_->shared.load(std::memory_order_relaxed)-base_;
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
