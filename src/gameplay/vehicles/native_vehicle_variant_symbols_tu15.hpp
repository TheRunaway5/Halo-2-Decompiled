#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string_view>
namespace halo2::detail {
struct VehicleVariantSymbolTu15 {std::uint32_t original,mcc;std::string_view name;};
// Exact TU1.5 instruction and original/MCC name join: v1034. These are
// compiled selector operands, never an authored field to rename globally.
inline constexpr std::array vehicle_variant_symbols_tu15{
    VehicleVariantSymbolTu15{0x05000694u,0x050006a0u,"gauss"},
    VehicleVariantSymbolTu15{0x0b00078bu,0x0b0007a0u,"multiplayer"}
};
template<class Name>std::uint32_t compiled_vehicle_variant_tu15(bool mcc,const Name &name,std::uint32_t original) {
    if(!mcc)return original;
    for(const auto &symbol:vehicle_variant_symbols_tu15)if(symbol.original==original) {
        if(name(symbol.mcc)!=symbol.name)
            throw std::invalid_argument("MCC vehicle variant namespace differs from its authenticated name join");
        return symbol.mcc;
    }
    return original;
}
}
