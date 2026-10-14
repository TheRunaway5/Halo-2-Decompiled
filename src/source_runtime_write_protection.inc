// Shared by the ordinary memory owner and its ABI-compatible delivery freeze.
#include <mutex>
#include <shared_mutex>
namespace halo2 {
namespace {
struct NativeWriteGuards {
    std::shared_mutex mutex;
    std::map<const SourceRuntimeMemory *,std::map<const void *,SourceRuntimeWriteGuard>> banks;
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
    if(!bytes)return;
    auto &registry=native_write_guards();std::shared_lock lock(registry.mutex);
    const auto bank=registry.banks.find(&memory);if(bank==registry.banks.end())return;
    for(const auto &[owner,guard]:bank->second){(void)owner;guard(address,bytes);}
}
} // namespace halo2
