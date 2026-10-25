#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace halo2 {
class SourcePhysicsContactMemory;
struct SourceEffectCompiledSymbolBindingTu15;

// A World owns this lease for the lifetime of its actual memory bank. The
// resolver belongs to its immutable Cache. Three literals in complete17B1F0
// and two101BC0 seed-name stores use their authenticated namespace boundaries;
// authored marker IDs and tag bytes stay intact.
class SourceEffectCompiledSymbolsTu15 {
public:
    using Name = std::function<std::string(std::uint32_t)>;
    SourceEffectCompiledSymbolsTu15(const SourcePhysicsContactMemory &, bool mcc,
                                   const Name &);
    ~SourceEffectCompiledSymbolsTu15();
    SourceEffectCompiledSymbolsTu15(const SourceEffectCompiledSymbolsTu15 &) = delete;
    SourceEffectCompiledSymbolsTu15 &operator=(const SourceEffectCompiledSymbolsTu15 &) = delete;
private:
    std::shared_ptr<const SourceEffectCompiledSymbolBindingTu15> binding_;
};

// Call only at authenticated compiled-literal comparisons, never on an
// authored value. An unbound/original bank and every other literal retain
// their original value. This changes no serialized address or ABI width.
std::uint32_t source_effect_compiled_symbol_tu15(
    const SourcePhysicsContactMemory &, std::uint32_t original);
// Only101CAD/101CC5 publish the compiled forward/backward explosion seeds.
// Other instructions, canonical gravity/up names and unbound banks stay raw.
std::uint32_t source_projectile_explosion_location_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t instruction,std::uint32_t original);
// Five comparisons in complete100E70, including acceleration_range. Adapt
// only compiled literals; authored aliases, attachment inputs and curves stay
// in the Cache's namespace and the original callback computes every result.
std::uint32_t source_projectile_function_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t instruction,std::uint32_t original);
// Exact compiled comparisons in complete object/unit/biped/vehicle/weapon
// and device callbacks, including folded StringID subtraction distances.
// Authored input IDs and curves remain raw; the original callback computes
// every scalar and active flag. Unbound banks retain their original operands.
std::uint32_t source_object_function_compiled_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t instruction,std::uint32_t original);
}
