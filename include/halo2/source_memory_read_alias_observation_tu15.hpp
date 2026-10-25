#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <stdexcept>

namespace halo2 {
class SourcePhysicsContactMemory;
struct SourceMemoryReadAliasTu15 {
    std::uint32_t entry,caller,address,canonical_value,read_value,tag,selector;
};
using SourceMemoryReadAliasObserverTu15=std::function<void(const SourceMemoryReadAliasTu15 &)>;
inline thread_local std::map<const SourcePhysicsContactMemory *,SourceMemoryReadAliasObserverTu15>
    source_memory_read_alias_observers_tu15;
// Read-only diagnostics of a typed input view; this supplies no call result.
class SourceMemoryReadAliasObserverBindingTu15 {
    const SourcePhysicsContactMemory *memory_;
public:
    SourceMemoryReadAliasObserverBindingTu15(const SourcePhysicsContactMemory &memory,
        SourceMemoryReadAliasObserverTu15 observer):memory_(&memory) {
        if(!source_memory_read_alias_observers_tu15.emplace(memory_,std::move(observer)).second)
            throw std::runtime_error("Source input read-alias diagnostics already bound");
    }
    ~SourceMemoryReadAliasObserverBindingTu15(){source_memory_read_alias_observers_tu15.erase(memory_);}
    SourceMemoryReadAliasObserverBindingTu15(const SourceMemoryReadAliasObserverBindingTu15 &)=delete;
    SourceMemoryReadAliasObserverBindingTu15 &operator=(const SourceMemoryReadAliasObserverBindingTu15 &)=delete;
};
inline void observe_source_memory_read_alias_tu15(const SourcePhysicsContactMemory &memory,
    const SourceMemoryReadAliasTu15 &input) {
    const auto observer=source_memory_read_alias_observers_tu15.find(&memory);
    if(observer!=source_memory_read_alias_observers_tu15.end()) {
        const auto callback=observer->second;callback(input);
    }
}
}
