#pragma once
#include "halo2/source_model_admission.hpp"
#include "halo2/source_model_node_bank.hpp"

namespace halo2 {
struct SourceModelBuildWorld {
    SourceModelAdmissionWorld admission;
    SourceModelNodeWorld nodes;
    std::uint32_t cached_models{},view_owner=0xffffffffu,view_players{};
    std::uint32_t arena_cursor{},arena{},scratch{};
    // Original3D480 sky rebase,23690 PRT and3DB00 lighting-query owners.
    // Rebase writes caller-owned scratch; lighting supplies its84-byte bank.
    // These services may mutate the same current banks, including lighting
    // validity/cadence fields. The model builder does not manufacture them.
    std::function<void(std::uint32_t,std::int32_t,std::uint32_t,std::uint32_t)> rebase;
    std::function<bool(const std::array<std::uint32_t,7>&)> prt;
    std::function<std::array<std::uint32_t,21>(std::uint32_t,float)> lighting;
};
// Whole4B5D0 joined to native admission, cache, input/resource/node production
// and shared geometry allocation. The output starts at collector+4; this
// routine preserves its caller's collector owner. Native scratch addresses
// belong to the current caller and must remain writable through all services.
void source_model_build(std::uint32_t owner,float metric,float lighting_metric,
    std::uint32_t output,std::uint32_t rebase,std::uint32_t force_world,
    std::uint32_t caller_mode,const SourceModelBuildWorld&);
}
