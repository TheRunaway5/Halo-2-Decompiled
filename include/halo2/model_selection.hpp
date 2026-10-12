#pragma once
#include "halo2/cache.hpp"
#include <optional>

namespace halo2 {
struct SourceInitialModelState {
    std::int8_t permutation=-1;
    std::uint8_t properties{};
    std::uint16_t damage{};
    float probability{};
};
struct SourceInitialVariantPermutation {
    std::int8_t permutation=-1;
    float weight{};
    std::vector<SourceInitialModelState> states;
};
struct SourceInitialVariantRegion {
    std::vector<SourceInitialVariantPermutation> permutations;
};
struct SourceInitialModelVariant {
    std::uint32_t name{};
    std::array<std::int8_t,16> regions{};
    std::vector<SourceInitialVariantRegion> definitions;
};
struct SourceInitialModelRegion {
    std::uint32_t name{};
    std::vector<std::uint8_t> permutation_flags;
};
struct SourceInitialModelDefinition {
    std::uint32_t model=0xffffffffu,render_model=0xffffffffu,default_variant{};
    std::vector<SourceInitialModelVariant> variants;
    std::vector<SourceInitialModelRegion> regions;
    // Original4C2B0 indexes this by the same region ordinal as16D280.
    std::vector<std::vector<std::array<std::int16_t,6>>> render_sections;
};
struct SourceInitialModelRegionState {
    std::int8_t variant_permutation=-1;
    std::uint8_t damage{},properties{};
    bool operator==(const SourceInitialModelRegionState&)const=default;
};
struct SourceInitialModelSelection {
    std::int32_t variant_index=-1;
    std::vector<std::uint8_t> permutations;
    std::vector<SourceInitialModelRegionState> states;
    // Unseeded selection leaves uncertain regions at NONE. These are not
    // authored absent regions and must not be used as active collision bytes.
    std::vector<unsigned> unresolved_rng_regions;
    bool resolved()const{return unresolved_rng_regions.empty();}
};
// Original16D180: StringID NONE selects variant0 when the block is nonempty.
std::int32_t source_initial_model_variant(const SourceInitialModelDefinition&,
                                        std::uint32_t name);
// Original16D280/16DAD0/16DCE0. The caller supplies the actual RNG stream;
// singleton weighted choices do not consume it, initial state choices do.
SourceInitialModelSelection source_initial_model_selection(
    const SourceInitialModelDefinition&,std::uint32_t variant,std::uint32_t &rng,
    std::uint32_t skipped_regions=0);
// Proves fixed output across every possible16-bit random sample independently.
// Correlated random choices may be conservatively unresolved. No RNG seed is
// invented, and initial effect/marker choices discarded by16D280 are omitted.
SourceInitialModelSelection source_deterministic_initial_model_selection(
    const SourceInitialModelDefinition&,std::uint32_t variant);
// Source4C2B0/137800's resident section array in model-region order. Duplicate
// sections remain duplicated. This does not simulate cache residency fallback.
std::vector<unsigned> source_initial_model_sections(
    const SourceInitialModelDefinition&,const SourceInitialModelSelection&,unsigned lod);
}
