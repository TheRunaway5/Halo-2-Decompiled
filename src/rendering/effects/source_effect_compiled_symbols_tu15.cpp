#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string_view>

namespace halo2 {
struct SourceEffectCompiledSymbolBindingTu15 {
    const SourcePhysicsContactMemory *memory;
};
namespace {
struct Symbol {std::uint32_t original, mcc;std::string_view name;};
// Exact unique-name join of the authenticated Xbox and MCC namespaces, also
// retained by mcc_frontend_string_ids.inc. Gameplay source17B1F0 compares these
// literals at17B232,17B2EC,17B35D.101BC0 also stores compiled forward/backward
// names at101CAD/101CC5; its gravity/up names retain their original IDs.
constexpr std::array symbols{
    Symbol{0x030000d9u,0x030000d9u,"any"},
    Symbol{0x0700054eu,0x0700055au,"replace"},
    Symbol{0x0400054fu,0x0400055bu,"root"},
    Symbol{0x0700054cu,0x07000558u,"forward"},
    Symbol{0x08000550u,0x0800055cu,"backward"},
    Symbol{0x0f000563u,0x0f00056fu,"range_remaining"},
    Symbol{0x06000564u,0x06000570u,"tracer"},
    Symbol{0x0e000562u,0x0e00056eu,"time_remaining"},
    Symbol{0x11000566u,0x11000572u,"projectile_attach"},
    Symbol{0x12000565u,0x12000571u,"acceleration_range"}
};
struct ObjectFunctionSymbol {std::uint32_t instruction,original,mcc,mcc_name;std::string_view name;};
constexpr std::array object_function_symbols{
#include "source_object_function_symbols_tu15.inc"
};
std::mutex registry_mutex;
std::map<const SourcePhysicsContactMemory *,
         std::weak_ptr<const SourceEffectCompiledSymbolBindingTu15>> registry;
}
SourceEffectCompiledSymbolsTu15::SourceEffectCompiledSymbolsTu15(
    const SourcePhysicsContactMemory &memory, bool mcc, const Name &name) {
    if(!mcc)return;
    if(!name)throw std::invalid_argument("MCC effect symbols require their Cache name resolver");
    for(const auto &symbol:symbols)
        if(name(symbol.mcc)!=symbol.name)
            throw std::invalid_argument("MCC effect symbol namespace differs from its authenticated name join");
    for(const auto &symbol:object_function_symbols)
        if(name(symbol.mcc_name)!=symbol.name)
            throw std::invalid_argument("MCC object-function symbols differ from their exact named join");
    auto binding=std::make_shared<const SourceEffectCompiledSymbolBindingTu15>(
        SourceEffectCompiledSymbolBindingTu15{&memory});
    std::lock_guard lock(registry_mutex);
    const auto previous=registry.find(&memory);
    if(previous!=registry.end() && !previous->second.expired())
        throw std::invalid_argument("Source bank already owns its effect symbol namespace");
    registry[&memory]=binding;binding_=std::move(binding);
}
SourceEffectCompiledSymbolsTu15::~SourceEffectCompiledSymbolsTu15() {
    if(!binding_)return;
    std::lock_guard lock(registry_mutex);
    const auto at=registry.find(binding_->memory);
    if(at!=registry.end() && at->second.lock()==binding_)registry.erase(at);
}
std::uint32_t source_effect_compiled_symbol_tu15(
    const SourcePhysicsContactMemory &memory, std::uint32_t original) {
    const auto end=symbols.begin()+3;
    const auto symbol=std::find_if(symbols.begin(),end,
        [&](const auto &candidate){return candidate.original==original;});
    if(symbol==end)return original;
    std::lock_guard lock(registry_mutex);
    const auto at=registry.find(&memory);
    if(at==registry.end() || at->second.expired())return original;
    return symbol->mcc;
}
std::uint32_t source_projectile_explosion_location_symbol_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t instruction,std::uint32_t original) {
    const auto index=instruction==0x101cadu?3u:instruction==0x101cc5u?4u:0u;
    if(index<3 || symbols[index].original!=original)return original;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(&memory);
    if(at==registry.end() || at->second.expired())return original;
    return symbols[index].mcc;
}
std::uint32_t source_projectile_function_symbol_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t instruction,std::uint32_t original) {
    constexpr std::array sites{0x100ea4u,0x100eb0u,0x100eb7u,0x100f04u,0x100f0bu};
    const auto at=std::find(sites.begin(),sites.end(),instruction);
    if(at==sites.end())return original;
    const auto &symbol=symbols[5u+unsigned(at-sites.begin())];
    if(symbol.original!=original)return original;
    std::lock_guard lock(registry_mutex);const auto owner=registry.find(&memory);
    return owner==registry.end() || owner->second.expired()?original:symbol.mcc;
}
std::uint32_t source_object_function_compiled_symbol_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t instruction,std::uint32_t original) {
    const auto symbol=std::lower_bound(object_function_symbols.begin(),object_function_symbols.end(),instruction,
        [](const auto &candidate,auto pc){return candidate.instruction<pc;});
    if(symbol==object_function_symbols.end() || symbol->instruction!=instruction || symbol->original!=original)return original;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(&memory);
    return at==registry.end()||at->second.expired()?original:symbol->mcc;
}
}
