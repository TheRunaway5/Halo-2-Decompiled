#pragma once
#include "halo2/source_model_resources.hpp"

namespace halo2 {
struct SourceModelNodeWorld {
    SourceModelResourceWorld memory;
    bool resource_cache_present{}; // Actual4E6494 null gate.
    // DeepBD610 animation/deferred-node production remains an explicit owner.
    // Its caller-owned bank may be mutated before the returned token is read.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> deferred_nodes;
};
// Whole1376C0 over current WORLD nodes and literal inverse binds. Output is
// three float4 rows per node; every remaining output byte survives.
std::int32_t source_model_skin_node_bank(std::uint32_t model,std::uint32_t nodes,
    std::uint32_t output,const SourceModelResourceWorld&);
// Whole137800, including native1376C0/compound blends, compact palette, ordered
// resident section maps and their literal fallback counts. OriginalBD610's
// no-local-node and positive deferred-counter returns are native; deeper
// animation updates and12DE70 publication remain explicit owners.
void source_model_geometry_node_bank(std::uint32_t owner,std::uint32_t model,
    std::uint32_t nodes,std::uint32_t selected,std::int32_t lod,
    std::uint32_t first_person,std::uint32_t output,const SourceModelNodeWorld&);
}
