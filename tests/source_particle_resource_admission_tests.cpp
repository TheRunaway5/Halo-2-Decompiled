#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/particle_resource_error.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
bool empty(SourceWorldParticleDrawsTu15::ResourceState state) {
    return !state.materials && !state.world_systems && !state.first_person_systems;
}
void check_breakable_surface_census(const Cache& cache) {
    const auto surfaces=cache.select_tags({.group=TagGroup::from_string("bsdt"),.require_address=true});
    check(surfaces.begin()!=surfaces.end(),"Particle census fixture requires an actual authored breakable surface");
    const auto datum=(*surfaces.begin()).datum;
    SourceRuntimeMemory memory;
    const auto mount=[&](unsigned address,unsigned size) {
        memory.mount(address,std::vector<std::byte>(size),SourceRuntimeMemory::Access::ReadWrite,
            "Declared full-identity particle census fixture storage");
    };
    const auto put=[&](unsigned address,unsigned value){memory.write(address,std::as_bytes(std::span(&value,1)));};
    for(auto address:{0x4f720cu,0x51df74u,0x4f0414u})mount(address,4);
    mount(0x62000000u,0x4000u);mount(0x63000000u,((datum&65535u)+1u)*16u);
    put(0x4f720cu,0x62000000u);put(0x51df74u,0x62000100u);put(0x4f0414u,0x63000000u);
    for(unsigned pass=0;pass<2;++pass) {
        const auto pool=0x62000000u+pass*0x100u;
        put(pool+0x20u,1);put(pool+0x24u,pass?84:400);put(pool+0x38u,pass);
        put(pool+0x44u,0x62001000u+pass*0x1000u);put(pool+0x48u,0x62000300u+pass*4u);
    }
    const auto tag=0x63000000u+(datum&65535u)*16u;
    put(tag,0x62736474u);put(tag+4u,datum);put(0x62000304u,1);
    put(0x62002000u,0xabcdu);put(0x62002010u,datum);put(0x62002014u,0xffffffffu);
    SourceWorldParticleDrawsTu15 bank(cache,{});
    check(bank.preload_live_world(memory)==std::vector{datum} && bank.world_effect(datum) &&
        bank.require_live_world_resident(memory)==std::vector{datum},
        "Actual breakable-surface definition was not admitted from its full native system identity");
    put(tag+4u,datum^0x10000u);bool rejected{};
    try{(void)bank.preload_live_world(memory);}catch(const ParticleResourceError&){throw FormatError("Stale native tag salt was relabeled as visual content");}
    catch(const FormatError&){rejected=true;}
    check(rejected,"Breakable-surface census accepted a stale full tag identity");
    put(tag+4u,datum);put(0x6200200cu,0xffff0000u);rejected=false;
    try{(void)bank.require_live_world_resident(memory);}catch(const ParticleResourceError&){throw FormatError("Native system ordinal was relabeled as visual content");}
    catch(const FormatError&){rejected=true;}
    check(rejected,"Breakable-surface census accepted an unauthored system ordinal");
}
int catalog_all(const std::filesystem::path& source,const std::filesystem::path& output) {
    std::vector<std::filesystem::path> maps;
    if(std::filesystem::is_directory(source)) {
        for(const auto& entry:std::filesystem::directory_iterator(source))
            if(entry.is_regular_file() && entry.path().extension()==".map")maps.push_back(entry.path());
        std::sort(maps.begin(),maps.end());
    }else if(std::filesystem::is_regular_file(source))maps.push_back(source);
    check(!maps.empty(),"Whole particle-plan census requires prepared MCC map inputs");
    nlohmann::json report{{"passed",true},{"maps",nlohmann::json::array()},
        {"failures",nlohmann::json::array()},
        {"scope","All resident particle resources and both authored draw-pass plans; no native simulation, RNG, device or framebuffer claim"}};
    for(const auto& path:maps) {
        Cache cache(path,path.parent_path());check(cache.is_mcc(),"Whole particle-plan census requires MCC content");
        SourceWorldParticleDrawsTu15 bank(cache,{});const auto catalog=bank.preload_catalog();
        std::size_t requested{},admitted{},rejected{},no_systems{};
        nlohmann::json groups;
        for(const auto& tag:cache.tags()) {
            if(!tag.address || (tag.group!="effe" && tag.group!="prt3" && tag.group!="PRTM" && tag.group!="bsdt"))continue;
            auto& group=groups[tag.group];
            for(const bool first_person:{false,true}) {
                ++requested;const auto before=bank.resource_failure_count();
                const auto success=first_person?bank.try_preload_first_person(tag.datum):bank.try_preload_world(tag.datum);
                const auto status=success?"admitted":bank.resource_failure_count()>before?"rejected":"no_systems";
                auto& pass=group[first_person?"first_person":"world"];
                if(pass.is_null())pass=nlohmann::json::object();
                pass[status]=pass.value(status,0u)+1;
                if(success)++admitted;else if(bank.resource_failure_count()>before)++rejected;else ++no_systems;
            }
        }
        const auto state=bank.resource_state();
        report["maps"].push_back({{"map",cache.name()},{"file",path.filename().string()},
            {"catalog",{{"requested",catalog.requested},{"admitted",catalog.admitted},{"rejected",catalog.rejected}}},
            {"passes",{{"requested",requested},{"admitted",admitted},{"rejected",rejected},{"no_systems",no_systems}}},
            {"world_systems",state.world_systems},{"first_person_systems",state.first_person_systems},
            {"materials",state.materials},{"groups",groups}});
        if(catalog.rejected || rejected)report["passed"]=false;
        for(const auto& failure:bank.resource_failures()) {
            if(report["failures"].size()>=12)break;
            report["failures"].push_back({{"map",cache.name()},{"definition",failure.definition},
                {"phase",unsigned(failure.phase)},{"reason",failure.reason}});
        }
    }
    std::ofstream file(output);file<<report.dump(2)<<'\n';check(bool(file),"Could not retain particle-plan census report");
    const bool passed=report["passed"].get<bool>();
    std::cout<<(passed?"PASS ":"FAIL ")<<"complete particle draw-pass plans for "<<maps.size()<<" prepared MCC maps\n";
    return passed?0:1;
}
}
int main(int argc,char** argv)try {
    if(argc==4 && std::string_view(argv[1])=="--catalog-all")return catalog_all(argv[2],argv[3]);
    check(argc==3,"Supply actual prepared MCC map and owned JSON report path");
    if(!*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    check(cache.is_mcc(),"Particle admission regression requires actual MCC content");
    nlohmann::json report{{"passed",false},{"map",cache.name()}};
    const auto save=[&]{std::ofstream file(argv[2]);file<<report.dump(2)<<'\n';};
    for(const auto& tag:cache.select_tags({.group=TagGroup::from_string("effe"),.require_address=true})) {
        unsigned calls{};bool injected{},reject=true;
        SourceWorldParticleDrawsTu15 graphics(cache,{},[&](auto) {
            if(++calls==2 && reject) {
                injected=true;throw ParticleResourceError("Declared second-material admission failure");
            }
        });
        try {(void)graphics.preload_world(tag.datum);}
        catch(const FormatError&) {
            if(!injected)continue;
            const auto after=graphics.resource_state();
            report["effect"]=tag.name;report["effect_datum"]=tag.datum;
            report["validator_calls_before_failure"]=calls;
            report["materials_after_failure"]=after.materials;
            report["world_systems_after_failure"]=after.world_systems;
            report["active_after_failure"]=graphics.active();save();
            check(empty(after) && !graphics.world_effect(tag.datum) && !graphics.active(),
                "Failed multi-system particle preload published partial resources or effect state");
            reject=false;calls=0;
            check(graphics.preload_world(tag.datum) && graphics.world_effect(tag.datum),
                "Corrected material validator did not admit the previously rejected actual effect");
            const auto complete=graphics.resource_state();
            check(complete.materials>=2 && complete.world_systems>=2 && calls>=2,
                "Retry omitted an authored material or retained a rejected partial load");
            check(graphics.preload_world(tag.datum),"Repeated actual effect admission changed success");
            const auto repeat=graphics.resource_state();
            check(repeat.materials==complete.materials && repeat.world_systems==complete.world_systems &&
                repeat.first_person_systems==complete.first_person_systems,
                "Repeated actual particle preload duplicated admitted resources");
            unsigned safe_calls{};
            SourceWorldParticleDrawsTu15 quarantined(cache,{},[&](auto) {
                if(++safe_calls==2)throw ParticleResourceError("Declared recoverable content failure");
            });
            check(!quarantined.try_preload_world(tag.datum) && empty(quarantined.resource_state()) &&
                !quarantined.active() && quarantined.resource_failure_count()==1,
                "Recoverable particle failure escaped or partially admitted its rejected effect");
            check(!quarantined.try_preload_world(tag.datum) && safe_calls==2 &&
                quarantined.resource_failures().size()==1,
                "Quarantined particle content was retried or diagnosed again on the next frame");
            check(quarantined.resource_failures()[0].definition==tag.datum &&
                quarantined.resource_failures()[0].phase==SourceWorldParticleDrawsTu15::ResourcePhase::world,
                "Particle rejection lost its full authored effect identity or pass");
            bool unaffected{};
            for(const auto& other:cache.select_tags({.group=TagGroup::from_string("effe"),.require_address=true}))
                if(other.datum!=tag.datum && quarantined.try_preload_world(other.datum)) {
                    unaffected=quarantined.world_effect(other.datum);break;
                }
            check(unaffected,"One rejected particle effect disabled unrelated valid effects");
            SourceWorldParticleDrawsTu15 fatal(cache,{},[](auto){throw FormatError("Declared device/system failure");});
            bool strict_failure{};
            try {(void)fatal.try_preload_world(tag.datum);}
            catch(const ParticleResourceError&) {throw FormatError("Device/system failure was relabeled as content");}
            catch(const FormatError&) {strict_failure=true;}
            check(strict_failure && empty(fatal.resource_state()) && !fatal.resource_failure_count(),
                "Particle safety path swallowed or partially published a device/system failure");
            SourceRuntimeMemory missing_native_pools;strict_failure=false;
            try {(void)quarantined.preload_live_world(missing_native_pools);}
            catch(const ParticleResourceError&) {throw FormatError("Native pool invariant was relabeled as content");}
            catch(const std::exception&) {strict_failure=true;}
            check(strict_failure,"Particle safety path swallowed a missing native-pool invariant");
            SourceWorldParticleDrawsTu15 catalog(cache,{});
            const auto catalog_admission=catalog.preload_catalog();
            check(catalog_admission.requested && catalog_admission.admitted && !catalog.active() &&
                !catalog.resource_state().world_systems && !catalog.resource_state().first_person_systems &&
                catalog.resident_materials().size()==catalog_admission.admitted,
                "Particle catalog admission created source effects/systems or lost prepared materials");
            const auto catalog_repeat=catalog.preload_catalog();
            check(catalog_repeat.requested==catalog_admission.requested &&
                catalog_repeat.admitted==catalog_admission.admitted && catalog_repeat.rejected==catalog_admission.rejected &&
                catalog.resource_failures().size()<=64,
                "Repeated particle catalog preparation changed admission or exceeded diagnostic bounds");
            report["retry_materials"]=complete.materials;report["retry_world_systems"]=complete.world_systems;
            report["recoverable_failure_cached"]=true;report["unrelated_effect_admitted"]=unaffected;
            report["device_and_native_errors_still_strict"]=true;
            check_breakable_surface_census(cache);report["breakable_surface_live_census_full_identity"]=true;
            report["catalog_requested"]=catalog_admission.requested;
            report["catalog_admitted"]=catalog_admission.admitted;report["catalog_rejected"]=catalog_admission.rejected;
            report["passed"]=true;save();
            std::cout<<"PASS actual multi-system particle admission rollback/retry: "<<tag.name<<'\n';return 0;
        }
    }
    throw FormatError("Actual MCC map has no multi-system effect reaching the second material validator");
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
