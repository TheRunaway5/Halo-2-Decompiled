// Shared by the ordinary memory owner and its ABI-compatible delivery freeze.
#include <mutex>
#include <shared_mutex>
namespace halo2 {
namespace {
struct NativeWriteGuards {
    std::shared_mutex mutex;
    std::map<const SourceRuntimeMemory *,std::map<const void *,SourceRuntimeWriteGuard>> banks;
    std::map<const SourceRuntimeMemory *,std::map<const void *,SourceRuntimeWriteGuard>> observers;
    std::map<const SourceRuntimeMemory *,std::map<const void *,SourceRuntimeWriteGuard>> topology;
    std::map<const SourceRuntimeMemory *,std::map<const void *,SourceRuntimeWriteGuard>> topology_observers;
};
NativeWriteGuards &native_write_guards(){static NativeWriteGuards guards;return guards;}
}
void source_runtime_register_write_guard(const SourceRuntimeMemory &memory,const void *owner,
    SourceRuntimeWriteGuard guard) {
    if(!owner || !guard)throw std::invalid_argument("Native write guard requires its actual resource owner");
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    if(!registry.banks[&memory].emplace(owner,std::move(guard)).second)
        throw std::logic_error("Native resource write guard already registered");
}
void source_runtime_unregister_write_guard(const SourceRuntimeMemory &memory,const void *owner) {
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    const auto bank=registry.banks.find(&memory);if(bank==registry.banks.end())return;
    bank->second.erase(owner);if(bank->second.empty())registry.banks.erase(bank);
}
void source_runtime_validate_write(const SourceRuntimeMemory &memory,std::uint32_t address,std::size_t bytes) {
    source_runtime_validate_write(memory,address,bytes,true);
}
void source_runtime_register_write_observer(const SourceRuntimeMemory &memory,const void *owner,SourceRuntimeWriteGuard observer){
    if(!owner||!observer)throw std::invalid_argument("Native write observer requires its owner");
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    if(!registry.observers[&memory].emplace(owner,std::move(observer)).second)throw std::logic_error("Native write observer already registered");
}
void source_runtime_unregister_write_observer(const SourceRuntimeMemory &memory,const void *owner){
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    const auto bank=registry.observers.find(&memory);if(bank==registry.observers.end())return;
    bank->second.erase(owner);if(bank->second.empty())registry.observers.erase(bank);
}
void source_runtime_validate_write(const SourceRuntimeMemory &memory,std::uint32_t address,std::size_t bytes,bool observe) {
    if(!bytes)return;
    auto &registry=native_write_guards();std::shared_lock lock(registry.mutex);
    if(const auto bank=registry.banks.find(&memory);bank!=registry.banks.end())
        for(const auto &[owner,guard]:bank->second){(void)owner;guard(address,bytes);}
    if(observe)if(const auto bank=registry.observers.find(&memory);bank!=registry.observers.end())
        for(const auto &[owner,observer]:bank->second){(void)owner;observer(address,bytes);}
}
void source_runtime_register_topology_guard(const SourceRuntimeMemory &memory,const void *owner,SourceRuntimeWriteGuard guard){
    if(!owner||!guard)throw std::invalid_argument("Native topology guard requires its owner");
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    if(!registry.topology[&memory].emplace(owner,std::move(guard)).second)throw std::logic_error("Native topology guard already registered");
}
void source_runtime_unregister_topology_guard(const SourceRuntimeMemory &memory,const void *owner){
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    const auto bank=registry.topology.find(&memory);if(bank==registry.topology.end())return;
    bank->second.erase(owner);if(bank->second.empty())registry.topology.erase(bank);
}
void source_runtime_validate_topology(const SourceRuntimeMemory &memory,std::uint32_t address,std::size_t bytes){
    auto &registry=native_write_guards();std::shared_lock lock(registry.mutex);
    const auto bank=registry.topology.find(&memory);if(bank==registry.topology.end())return;
    for(const auto &[owner,guard]:bank->second){(void)owner;guard(address,bytes);}
}
void source_runtime_register_topology_observer(const SourceRuntimeMemory &memory,const void *owner,SourceRuntimeWriteGuard observer){
    if(!owner||!observer)throw std::invalid_argument("Native topology observer requires its owner");
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    if(!registry.topology_observers[&memory].emplace(owner,std::move(observer)).second)throw std::logic_error("Native topology observer already registered");
}
void source_runtime_unregister_topology_observer(const SourceRuntimeMemory &memory,const void *owner){
    auto &registry=native_write_guards();std::unique_lock lock(registry.mutex);
    const auto bank=registry.topology_observers.find(&memory);if(bank==registry.topology_observers.end())return;
    bank->second.erase(owner);if(bank->second.empty())registry.topology_observers.erase(bank);
}
void source_runtime_observe_topology(const SourceRuntimeMemory &memory,std::uint32_t address,std::size_t bytes) noexcept {
    auto &registry=native_write_guards();std::shared_lock lock(registry.mutex);
    const auto bank=registry.topology_observers.find(&memory);if(bank==registry.topology_observers.end())return;
    for(const auto &[owner,observer]:bank->second){(void)owner;observer(address,bytes);}
}
} // namespace halo2
