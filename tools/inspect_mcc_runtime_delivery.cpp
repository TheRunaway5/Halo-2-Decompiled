#include "halo2/mcc_runtime_delivery.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

int main(int argc,char**argv)try{
    if(argc!=2)throw std::runtime_error("usage: inspect_mcc_runtime_delivery MCC.map");
    halo2::Cache cache(argv[1]);
    // Explicit inspection budget only; this does not select the runtime's
    // physical arena, reserve pages, or claim startup/activation completion.
    const auto plan=halo2::MccRuntimeCacheDelivery::plan(cache,0x80061000u,256u*1024*1024);
    nlohmann::json issues=nlohmann::json::array();
    for(const auto& issue:plan.issues()){
        const auto* tag=cache.find_tag(issue.tag);
        issues.push_back({{"datum",issue.tag.value},{"group",issue.group.string()},{"path",tag?tag->name:""},
            {"field_offset",issue.field_offset?nlohmann::json(*issue.field_offset):nlohmann::json(nullptr)},
            {"field",issue.field},{"reason",issue.reason}});
    }
    bool rejected=false;try{(void)plan.tag_image();}catch(const halo2::FormatError&){rejected=true;}
    std::cout<<nlohmann::json({{"map",cache.name()},{"tag_slots",plan.index().count},
        {"canonical_index_bytes",plan.index().region.bytes.size()},{"canonical_table",plan.index().table},
        {"planned_reservation_bytes",plan.reservation_bytes()},{"declared_relocation_allocations",plan.relocations().size()},
        {"ordered_schema_issues",issues},{"runtime_publication_rejected",rejected},
        {"inspection_budget_only",true}}).dump(2)<<'\n';
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
