#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/mcc_script_layout.hpp"
#include "halo2/native_mcc_script_builtin.hpp"
#include "halo2/mcc_sound_geometry.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
int main(int argc,char **argv) {
    if(argc!=6 && !(argc==7 && std::string_view(argv[6])=="--resources")){
        std::cerr<<"Supply MAP SHARED_MAPS PREPARED_ROOT campaign|multiplayer REPORT [--resources]\n";return 2;}
    Json report{{"schema","H2_WORKSHOP_NATIVE_INSPECTION_V1"},{"inspection_completed",false},
        {"gameplay_qualified",false},{"cache_abi",{{"bytes",sizeof(Cache)},{"alignment",alignof(Cache)}}},
        {"scope","All authored shader metadata and all BSP content readers; optional bitmap frames, first audio permutations and facial resources; no full mission or GPU acceptance"}};
    try {
        const bool multiplayer=std::string_view(argv[4])=="multiplayer";
        if(!multiplayer && std::string_view(argv[4])!="campaign")throw FormatError("Invalid scenario mode");
        const Cache cache(argv[1],argv[2],{},argv[3]);
        const auto *scenario=cache.find_tag(cache.scenario_tag());
        if(!scenario)throw FormatError("Missing scenario");
        const Bytes root(cache.read_tag_bytes(scenario->address,scenario->size));
        const auto block=[&](unsigned field,unsigned stride) {
            const auto count=root.u32(field);
            if(count>(stride==1?16u*1024u*1024u:65536u))throw FormatError("Scenario script bank exceeds bound");
            return count?cache.read_tag_bytes(root.u32(field+4),std::size_t(count)*stride):std::span<const std::byte>{};
        };
        const auto strings=block(0x1b0,1);
        const auto layout=project_mcc_script_layout(cache,block(0x238,20),strings,block(0x1b8,0x28),{true});
        const NativeMccScriptBuiltinProfile profile(layout,strings,true);
        std::set<std::string> requirements;
        for(const auto &need:layout.requirements)if(!profile.owns(need))requirements.insert(need.name);
        report["script_requirements"]=requirements;
        report["script_calls"]=layout.builtin_calls;
        report["bsp_count"]=cache.bsps().size();
        report["content"]=Json::array();
        for(unsigned bsp=0;bsp<cache.bsps().size();++bsp) {
            Json row{{"bsp",bsp},{"loaded",false}};
            try {
                const auto content=cache.load_content(bsp,multiplayer);
                row.update({{"loaded",true},{"description",content.describe()},{"limitations",content.limitations}});
            }catch(const std::exception &e){row["error"]=e.what();}
            report["content"].push_back(std::move(row));
        }
        Json templates=Json::object();unsigned material_count{},without_passes{},errors{};
        report["material_errors"]=Json::array();
        for(const auto &tag:cache.tags())if(tag.group=="shad" && tag.address) {
            ++material_count;
            try {
                const auto material=cache.load_material(tag.datum,false,false);
                const auto key=material.template_name.empty()?"<unknown>":material.template_name;
                if(!templates.contains(key))templates[key]={{"instances",0},{"without_native_passes",Json::array()},
                    {"gap_messages",Json::object()}};
                auto &row=templates[key];row["instances"]=row["instances"].get<unsigned>()+1;
                if(material.passes.empty()) {++without_passes;row["without_native_passes"].push_back(tag.name);}
                for(const auto &gap:material.rendering_gaps) {
                    auto &counts=row["gap_messages"];counts[gap]=counts.value(gap,0u)+1;
                }
            }catch(const std::exception &e){++errors;report["material_errors"].push_back({{"shader",tag.name},{"error",e.what()}});}
        }
        report["materials"]={{"instances",material_count},{"without_native_passes",without_passes},
            {"read_errors",errors},{"templates",std::move(templates)}};
        if(argc==7) {
            Json resources{{"bitmap_frames",0},{"sound_first_permutations",0},{"empty_sound_ranges",0},
                {"errors",Json::array()}};
            for(const auto& tag:cache.tags())if(tag.address && (tag.group=="bitm" || tag.group=="snd!")) {
                try {
                    if(tag.group=="bitm") {
                        for(unsigned n=0;n<cache.bitmap_count(tag.datum);++n) {
                            (void)cache.load_bitmap_resource(tag.datum,n);
                            resources["bitmap_frames"]=resources["bitmap_frames"].get<unsigned>()+1;
                        }
                    }else {
                        const auto pitches=cache.sound_pitch_range_count(tag.datum);
                        if(!pitches)resources["empty_sound_ranges"]=resources["empty_sound_ranges"].get<unsigned>()+1;
                        for(unsigned pitch=0;pitch<pitches;++pitch) {
                            if(!cache.sound_permutation_count(tag.datum,pitch)) {
                                resources["empty_sound_ranges"]=resources["empty_sound_ranges"].get<unsigned>()+1;continue;
                            }
                            (void)cache.load_sound(tag.datum,pitch,0);
                            resources["sound_first_permutations"]=resources["sound_first_permutations"].get<unsigned>()+1;
                        }
                    }
                }catch(const std::exception& e){resources["errors"].push_back({{"tag",tag.name},{"group",tag.group},{"error",e.what()}});}
            }
            try {resources["facial_resources"]=plan_mcc_sound_geometry_resources(cache).size();}
            catch(const std::exception& e){resources["errors"].push_back({{"group","facial"},{"error",e.what()}});}
            report["resource_checks_passed"]=resources["errors"].empty();
            report["resources"]=std::move(resources);
        }
        report["inspection_completed"]=true;
    }catch(const std::exception &e){report["error"]=e.what();}
    std::ofstream file(argv[5]);file<<report.dump(2)<<'\n';
    if(!file){std::cerr<<"Could not write bounded inspection report\n";return 2;}
    return report["inspection_completed"].get<bool>() && report.value("resource_checks_passed",true)?0:1;
}
