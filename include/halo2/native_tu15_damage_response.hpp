#pragma once
#include <cstdint>
#include <stdexcept>
namespace halo2 {
// Authored response kinds at the actual DCB71/DCB75 TU1.5 boundary. Keep
// numeric names until the enclosing model-response fields have semantic owners.
// These are typed decisions, never Xbox branch addresses or CPU state.
enum class NativeTu15DamageResponseRoute {Type0,Type1,Type2,Stop,Type4};
constexpr NativeTu15DamageResponseRoute native_tu15_damage_response_route(std::uint16_t kind){
    switch(kind){
    case 0:return NativeTu15DamageResponseRoute::Type0;
    case 1:return NativeTu15DamageResponseRoute::Type1;
    case 2:return NativeTu15DamageResponseRoute::Type2;
    case 3:return NativeTu15DamageResponseRoute::Stop;
    case 4:return NativeTu15DamageResponseRoute::Type4;
    default:throw std::runtime_error("Native TU1.5 damage response kind is outside its authenticated authored domain");
    }
}
} // namespace halo2
