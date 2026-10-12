#pragma once
#include "halo2/source_model_resources.hpp"

namespace halo2 {
struct SourceModelAdmissionWorld {
    SourceModelResourceWorld memory;
    std::uint32_t first_person_owner=0xffffffffu; // Actual4C1BD0.
    std::int16_t first_person_models{}; // Actual4C1BD4.
    bool view_modifier{}; // Actual4BA019.
    std::int32_t ordinary_maximum{}; // Actual4BA01C.
    std::uint32_t rasterizer{}; // Actual485A80, not the matg root.
    // ActualBAB40 over current object function state. This remains explicit;
    // the pair is its AL result and output float, including false outputs.
    std::function<std::pair<bool,float>(std::uint32_t,std::uint32_t)> scalar;
};
// WholeB9CE0 parent visibility and4C0D0 parallel region sort-order bank.
bool source_model_owner_hidden(std::uint32_t,const SourceModelResourceWorld&);
void source_model_sort_orders(std::uint32_t owner,std::uint32_t output,
    std::int32_t models,const SourceModelResourceWorld&);
// Whole4BCC0 joined to native visibility/model count/LOD choice/cache/sort
// owners. Current caller mode, FP byte and LOD byte remain original fields;
// every other unwritten output byte survives.12DE70/BAB40 are explicit services.
std::int16_t source_model_render_admission(std::uint32_t owner,float metric,
    std::uint32_t caller_mode,std::uint32_t first_person,std::uint32_t force_world,
    std::uint32_t lod,std::uint32_t output,const SourceModelAdmissionWorld&);
}
