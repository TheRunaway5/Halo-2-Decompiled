#include "halo2/source_vehicle_compiled_symbols_tu15.hpp"
#include "gameplay/vehicles/native_vehicle_variant_symbols_tu15.hpp"
#include <map>
#include <mutex>
namespace halo2 {
struct SourceVehicleCompiledSymbolBindingTu15 {const SourcePhysicsContactMemory *memory;};
namespace {
std::mutex registry_mutex;
std::map<const SourcePhysicsContactMemory *,std::weak_ptr<const SourceVehicleCompiledSymbolBindingTu15>> registry;
}
SourceVehicleCompiledSymbolsTu15::SourceVehicleCompiledSymbolsTu15(
    const SourcePhysicsContactMemory &memory,bool mcc,const Name &name) {
    if(!mcc)return;
    if(!name)throw std::invalid_argument("MCC vehicle symbols require their Cache name resolver");
    for(const auto &symbol:detail::vehicle_variant_symbols_tu15)
        (void)detail::compiled_vehicle_variant_tu15(true,name,symbol.original);
    auto binding=std::make_shared<const SourceVehicleCompiledSymbolBindingTu15>(SourceVehicleCompiledSymbolBindingTu15{&memory});
    std::lock_guard lock(registry_mutex);const auto previous=registry.find(&memory);
    if(previous!=registry.end()&&!previous->second.expired())
        throw std::invalid_argument("Source bank already owns its vehicle symbol namespace");
    registry[&memory]=binding;binding_=std::move(binding);
}
SourceVehicleCompiledSymbolsTu15::~SourceVehicleCompiledSymbolsTu15() {
    if(!binding_)return;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(binding_->memory);
    if(at!=registry.end()&&at->second.lock()==binding_)registry.erase(at);
}
std::uint32_t source_vehicle_compiled_symbol_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t instruction,std::uint32_t original) {
    std::lock_guard lock(registry_mutex);const auto at=registry.find(&memory);
    if(at==registry.end()||at->second.expired())return original;
    const auto expected=instruction==0x15cbc2u?0x05000694u:
        (instruction==0x15ccb9u||instruction==0x15ccceu)?0x0b00078bu:0xffffffffu;
    if(expected==0xffffffffu||original!=expected)
        throw std::invalid_argument("MCC vehicle symbol lacks its exact compiled-literal site");
    for(const auto &symbol:detail::vehicle_variant_symbols_tu15)
        if(symbol.original==original)return symbol.mcc;
    throw std::invalid_argument("MCC vehicle compiled variant has no authenticated name");
}
}
