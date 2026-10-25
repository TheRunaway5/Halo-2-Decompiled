#include "halo2/multiplayer_world.hpp"
#include "halo2/native_unit_interaction.hpp"
#include <atomic>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

namespace halo2::physics_world_runtime_detail {
extern std::atomic<std::uint64_t> vehicle_choice_native_calls;
extern std::atomic<std::uint64_t> vehicle_choice_native_random_draws;
extern std::atomic<std::uint64_t> vehicle_choice_native_flying_calls;
}
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace {
void check(bool value,const char* text){if(!value)throw FormatError(text);}
}

int main(int argc,char** argv) {
    nlohmann::json report{{"passed",false},{"full_vehicle_parity",false},
        {"source_results_or_RNG_or_admission_supplied",false},{"runtime_XBE_argument",false}};
    std::filesystem::path output;
    try {
        check(argc==5,"Supply MCC_MAP MCC_MAPS FAMILY REPORT");output=argv[4];
        const std::string family=argv[3];
        const bool random=family=="light" || family=="heavy" || family=="turret";
        const bool flying_random=family=="flying-random";
        const bool flying=family=="flying-off" || family=="flying-on" || flying_random;
        const bool absent=family=="flying-off";
        check(random || flying || family=="secondary-heavy","Unknown authored vehicle setting");
        const int parameter=family=="light"?76:family=="heavy"?78:family=="turret"?81:flying?79:80;
        const int option=family=="light"?6:family=="heavy"?3:family=="turret"?5:absent?4:flying_random?3:flying?1:2;
        const int classification=family=="light"?1:family=="heavy"?3:family=="turret"?6:flying?4:5;
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
        options.local_players=1;options.source_object_world=true;options.mute=true;
        options.variant.values.set(parameter,option);options.session_random_seed=0x15cc10u;
        const auto calls=vehicle_choice_native_calls.load(),draws=vehicle_choice_native_random_draws.load();
        const auto flying_calls=vehicle_choice_native_flying_calls.load();
        MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);
        check(world.cache->is_mcc(),"Random vehicle acceptance requires actual MCC metadata");
        // Check the actual retained source-world table independently of the
        // cold-template catalogue. Agreement between two MCC consumers alone
        // would miss MCC's cut Mongoose in TU1.5's literal NONE slot6.
        const auto& bank=world.source_objects->memory();
        const auto word=[&](std::uint32_t at){return Bytes(bank.view(at,4)).u32(0);};
        const auto globals=word(0x4ecc1cu),tag_table=word(0x4f0414u);
        const auto mulg=word(tag_table+(word(globals+0x16cu)&0xffffu)*16u+8u);
        const auto runtime=word(mulg+12u),references=word(runtime+0x74u);
        check(word(runtime+0x70u)==11,"Native vehicle table retained a non-TU1.5 count");
        constexpr std::array<std::string_view,11> expected{
            "objects\\vehicles\\warthog\\warthog","objects\\vehicles\\ghost\\ghost",
            "objects\\vehicles\\scorpion\\scorpion","objects\\vehicles\\wraith\\wraith",
            "objects\\vehicles\\banshee\\banshee","objects\\vehicles\\spectre\\spectre","",
            "objects\\vehicles\\h_turret_ap\\h_turret_ap","","","objects\\vehicles\\c_turret_ap\\c_turret_ap"};
        for(unsigned index=0;index<expected.size();++index) {
            const auto group=word(references+index*8u),datum=word(references+index*8u+4u);
            if(expected[index].empty())check(group==null_datum && datum==null_datum,"Native TU1.5 NONE vehicle reference changed");
            else {
                const auto* tag=world.cache->query_tag({.handle=TagHandle{datum}});
                check(group==tag_groups::vehicle.value && tag && tag->type()==tag_groups::vehicle &&
                    tag->name==expected[index],"Native vehicle reference differs from the authenticated TU1.5 identity tuple");
            }
        }
        report["native_vehicle_reference_table_matches_TU15"]=true;
        const auto& content=world.content;
        std::set<unsigned> required;
        for(unsigned row=0;row<content.netgame_equipment.size();++row)
            if(content.netgame_equipment[row].classification==classification)required.insert(row);
        check(!required.empty(),"This map has no actual affected Random vehicle rows");
        for(const auto row:required)check(content.unit_interactions->netgame_placements.contains(row)!=absent,
            "Cold template presence disagrees with the authored vehicle setting");
        std::array<Controls,multiplayer_capacity> controls{};
        const auto initial=world.match.state();
        for(const auto slot:world.local_slots){controls[slot].yaw=initial.players[slot].yaw;controls[slot].pitch=initial.players[slot].pitch;}
        std::set<unsigned> observed;std::set<std::uint32_t> generations;
        auto births=nlohmann::json::array();
        for(unsigned tick=0;tick<240;++tick) {
            world.match.step(controls);const auto state=world.match.state();
            if(absent)for(const auto row:required)
                check(world.source_objects->netgame_object(row)==null_datum,"Flying Off retained an actual source vehicle generation");
            for(const auto& vehicle:state.world_vehicles) {
                const auto& placement=content.objects.at(vehicle.placement);
                if(placement.netgame_row<0 || !required.contains(unsigned(placement.netgame_row)))continue;
                const auto definition=world.source_objects->object_definition(vehicle.handle);
                const auto regions=world.source_objects->object_model_regions(vehicle.handle);
                check(placement.definition==definition,"Random vehicle scene retained another definition");
                check(vehicle.regions && vehicle.regions->selection.variant_index==regions.selection.variant_index,
                    "Random vehicle scene retained another model variant");
                const auto selected=content.unit_interactions->netgame_templates.at(
                    {unsigned(placement.netgame_row),definition,regions.selection.variant_index});
                check(selected==vehicle.placement,"Actual Random birth did not bind its authored cold template");
                check(vehicle.lighting_definition==definition,"Random chassis lighting retained another definition");
                observed.insert(unsigned(placement.netgame_row));
                if(generations.insert(vehicle.handle).second)births.push_back({{"row",placement.netgame_row},
                    {"object",vehicle.handle},{"definition",definition},{"variant",regions.selection.variant_index},
                    {"placement",vehicle.placement},{"tick",state.tick}});
            }
        }
        const auto native_calls=vehicle_choice_native_calls.load()-calls,native_draws=vehicle_choice_native_random_draws.load()-draws;
        if(random || flying_random)check(native_calls>0 && native_draws>0,"Actual world did not execute native Random selector kernels");
        const auto native_flying=vehicle_choice_native_flying_calls.load()-flying_calls;
        if(flying)check(native_flying>0,"Actual flying setting did not execute its native selector");
        check(absent?observed.empty():observed==required,"Affected authored rows do not match their actual vehicle setting");
        report.update({{"passed",true},{"family",family},{"parameter",parameter},{"value",option},
            {"ticks",240},{"required_rows",required},{"observed_rows",observed},{"births",births},
            {"native_selector_calls",native_calls},{"native_random_draws",native_draws},
            {"native_flying_selector_calls",native_flying},
            {"rendered_frames_verified",false},{"weapons_audio_destruction_verified",false}});
    }catch(const std::exception& error){report["error"]=error.what();}
    if(!output.empty()){std::ofstream stream(output);stream<<report.dump(2)<<'\n';}
    std::cout<<report.dump()<<'\n';return report["passed"].get<bool>()?0:1;
}
