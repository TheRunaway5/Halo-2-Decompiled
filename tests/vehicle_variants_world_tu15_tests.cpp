#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/object_initialization.hpp"
#include "halo2/source_model_regions.hpp"
#include "helpers/campaign_vehicle_observer_inputs_tu15.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
struct Report {
  std::filesystem::path path;
  Json data{{"passed",false},{"full_vehicle_parity",false},
      {"constructor_admission_or_results_supplied",false},
      {"isolated_constructor_request_is_declared_input",true},
      {"driving_weapons_render_audio_or_campaign_completion_certified",false}};
  ~Report(){if(!path.empty()){std::ofstream file(path);file<<data.dump(2)<<'\n';}}
};
}
int main(int argc,char **argv){Report report;try {
  check(argc==5,"Supply MCC_MAP MCC_MAPS CASE_JSON REPORT");report.path=argv[4];
  std::ifstream input(argv[3]);const auto request=Json::parse(input);
  const auto name=request.at("vehicle").get<std::string>();
  const bool named=request.at("selection")=="named";
  report.data["case"]=request;
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  options.local_players=1;options.source_object_world=true;options.mute=true;
  options.session_random_seed=0x23ce15u;
  Cache cache(options.map,options.shared_maps);
  check(cache.is_mcc(),"Vehicle variant world requires prepared MCC cache inputs");
  const auto found=std::ranges::find_if(cache.tags(),[&](const auto &tag){return tag.group=="vehi" && tag.name==name;});
  check(found!=cache.tags().end(),"Vehicle definition absent in selected cache context");
  const auto definition=found->datum;
  const auto model=cache.load_initial_model_definition(definition);
  std::uint32_t selected=0;
  if(named){
    const auto variant=request.at("variant").get<std::string>();
    const auto row=std::ranges::find_if(model.variants,[&](const auto &entry){return cache.string_id(entry.name)==variant;});
    check(row!=model.variants.end(),"Named vehicle model variant absent in selected cache");
    selected=row->name;
  }
  const auto expected=source_initial_model_variant(model,selected?selected:model.default_variant);
  const bool campaign=std::filesystem::path(argv[1]).filename().string().starts_with("0");
  auto content=cache.load_content(0,!campaign);
  if(campaign){
    options.campaign_difficulty=1;
    // A campaign observer uses actual authored placement/BSP and independent
    // external start inputs. No multiplayer admission or vehicle result is
    // substituted when that observer cannot be established.
    std::vector<ScenarioObject> candidates;
    for(const auto &p:content.objects)if(p.type==1 && p.placement>=0 &&
        p.origin_bsp>=0 && !p.model_attachment)candidates.push_back(p);
    std::stable_partition(candidates.begin(),candidates.end(),[&](const auto &p){return p.definition==definition;});
    check(!candidates.empty(),"Campaign variant context needs an authored vehicle observer placement");
    // Scripted aircraft may be authored outside walkable geometry. Select the
    // first surveyed clear authored observer; the isolated constructor input
    // does not require an observer beside that particular scripted vehicle.
    std::optional<std::array<Spawn,1>> starts;
    std::size_t loaded_bsp=0;
    report.data["observer_survey"]=Json::array();
    for(const auto &placement:candidates) {
      options.bsp=std::size_t(placement.origin_bsp);
      if(loaded_bsp!=options.bsp){content=cache.load_content(options.bsp,false);loaded_bsp=options.bsp;}
      Json survey{{"bsp",options.bsp},{"placement",placement.placement},{"definition",placement.definition}};
      std::array<test::CampaignObserverStartProof,1> proof;
      try {starts=test::campaign_vehicle_observer_starts<1>(cache,content,options.bsp,placement,&proof);}
      catch(const FormatError &e) {
        if(std::string_view(e.what())!="Authored vehicle has no clear nearby cached BSP observer start")throw;
        survey["clear_floor_found"]=false;report.data["observer_survey"].push_back(survey);continue;
      }
      survey["clear_floor_found"]=true;survey["triangle"]=proof[0].triangle;
      survey["observer_distance"]=proof[0].observer_distance;
      report.data["observer_survey"].push_back(survey);report.data["observer"]=survey;break;
    }
    check(starts.has_value(),"No authored campaign vehicle has a clear cached BSP observer start");
    options.native_spawn_inputs.assign(starts->begin(),starts->end());
    options.campaign_spawn_input=[starts=*starts](unsigned slot,bool initial)->std::optional<Spawn>{
      if(initial && slot<starts.size())return starts[slot];return {};};
  }
  MultiplayerPreparedWorld world(options,campaign?std::optional<ScriptProgram>{ScriptProgram{}}:std::nullopt);
  world.bind_source_local_views(world.local_slots);
  auto &source=*world.source_objects;auto &native=source.tu15_world();
  const auto slot=world.local_slots.at(0);
  const auto position=world.match.state().players[slot].position+Vec3{0,0,.5f};
  constexpr std::uint32_t packet=0x60010000u;
  auto initializer=native.prepare_source_call(std::array{definition,null_datum,0u});
  initializer.gp[6]=packet;native.run_actual(0xb9960u,initializer);
  const auto word=[&](std::uint32_t at,std::uint32_t value){
    const std::array bytes{std::byte(value),std::byte(value>>8),std::byte(value>>16),std::byte(value>>24)};
    native.write_source_call_input(at,bytes);
  };
  // D75A0 supplies the selected variant at request+C. B9960 owns every other
  // request default; B9B70 owns admission, model choice, children and physics.
  word(packet+0xc,selected);
  word(packet+0x1c,std::bit_cast<std::uint32_t>(position.x));
  word(packet+0x20,std::bit_cast<std::uint32_t>(position.y));
  word(packet+0x24,std::bit_cast<std::uint32_t>(position.z));
  auto birth=native.prepare_source_call(std::array{packet});native.run_actual(0xb9b70u,birth);
  const auto object=birth.gp[0];report.data["actual_constructor_result"]=object;
  report.data["expected_variant_index"]=expected;
  report.data["constructor_completed"]=true;
  if(object==null_datum){
    report.data["unresolved_original_admission_comparison"]=true;
    throw FormatError("Native constructor returned NONE; original admission comparison required");
  }
  check(source.object_definition(object)==definition,"Native constructor selected another vehicle definition");
  const auto regions=source.object_model_regions(object);
  report.data["actual_variant_index"]=regions.selection.variant_index;
  report.data["permutations"]=regions.selection.permutations;
  check(regions.selection.variant_index==expected,"Native constructor selected another model variant");
  if(model.render_model!=null_datum)report.data["world_nodes"]=source.world_nodes(object).size();
  report.data["children"]=Json::array();
  for(const auto &child:source.live_objects())if(child.parent==object)
    report.data["children"].push_back({{"object",child.object},{"definition",child.definition},{"type",child.type}});
  native.destroy_object(object);
  check(std::ranges::none_of(source.live_objects(),[&](const auto &entry){return entry.object==object || entry.parent==object;}),
      "Original complete deletion retained the vehicle or an attached child");
  report.data["ordinary_deletion_completed"]=true;report.data["passed"]=true;
  std::cout<<"PASS native vehicle variant birth, model selection and ordinary deletion: "<<name<<'\n';return 0;
}catch(const std::exception &e){report.data["error"]=e.what();std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
