#include "halo2/object_initialization.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::filesystem::path map_path(const std::string& name){
    auto path=std::filesystem::path("assets/disc/maps")/(name+".map");
    if(!std::filesystem::exists(path))path=std::filesystem::path("assets/map-pack/maps")/(name+".map");
    return path;
}
const ScenarioObject& placed(const LevelContent& content,int type,int ordinal){
    const auto found=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto& object){
        return object.type==type&&object.placement==ordinal;});
    if(found==content.objects.end())throw std::runtime_error("missing original construction fixture placement");
    return *found;
}
SourceGameRandomInitializationServices prefix_services(){
    // The original constructor fixture begins at211CC0, not all137CA0 callbacks.
    // Empty surrounding callback ranges are explicit fixture inputs here.
    return {[](auto&){return true;},[](auto&){return true;}};
}
void phase_fixtures(){
    std::ifstream input("tests/fixtures/retail_object_initial_creation.tsv");
    require(bool(input),"missing original scenario creation fixtures");
    unsigned maps=0;std::string name;std::size_t count;
    while(input>>name>>count){
        Cache cache(map_path(name));const auto content=cache.load_content(0,true);
        const auto actual=source_scenario_initial_creation_attempts(content.objects,false);
        require(actual.size()==count,"source initial creation attempt count differs");
        for(const auto& key:actual){int type,placement;input>>type>>placement;
            require(key==SourceScenarioCreationKey{type,placement},"source initial creation attempt order differs");}
        ++maps;
    }
    require(input.eof()&&maps==21,"invalid original scenario construction fixtures");
}
void cold_prefix_fixtures(){
    Cache cache(map_path("zanzibar"));const auto content=cache.load_content(0,true);
    std::ifstream input("tests/fixtures/retail_object_cold_constructor.tsv");
    require(bool(input),"missing original cold constructor fixtures");
    std::uint32_t launch,after_initializer,after_prefix;unsigned fixtures=0,objects=0;
    while(input>>launch>>after_initializer>>after_prefix){
        std::uint32_t primary=0;SourceObjectInitialization state(primary);
        state.initialize_game(launch,prefix_services());
        require(state.primary_known()&&primary==after_initializer,"original211CC0 primary state differs");
        for(unsigned i=0;i<14;++i){
            int type,ordinal,variant;std::uint32_t definition,before,after;unsigned count;
            input>>type>>ordinal>>definition>>variant>>before>>after>>count;
            const auto& object=placed(content,type,ordinal);
            require(object.definition==definition,"source constructor definition differs");
            const auto handle=0x12340000u+i*0x10001u;
            const auto& record=state.create(cache,object,handle,0,true);
            require(record.seed_before_known&&record.constructor_consumption_known&&state.primary_known(),
                    "verified cold constructor stream became unknown");
            require(record.seed_before==before&&record.seed_after==after&&primary==after,
                    "original cold constructor primary state differs");
            require(record.selection.variant_index==variant&&record.selection.permutations.size()==count,
                    "original cold constructor region count/variant differs");
            for(auto value:record.selection.permutations){unsigned expected;input>>expected;
                require(value==expected,"original cold constructor active permutation differs");}
            for(const auto& value:record.selection.states){int permutation;unsigned damage,properties;
                input>>permutation>>damage>>properties;
                require(value==SourceInitialModelRegionState{std::int8_t(permutation),std::uint8_t(damage),std::uint8_t(properties)},
                        "original cold constructor state bytes differ");}
            require(state.find(handle)==&record&&state.find(record.key)==&record,
                    "source full-handle construction ledger lookup differs");
            if(fixtures<2&&ordinal==19){
                const auto model=cache.load_model(object.definition,false,object.variant,record.selection);
                const auto ids=source_initial_model_sections(cache.load_initial_model_definition(object.definition),record.selection,5);
                std::set<unsigned> expected(ids.begin(),ids.end()),actual;
                for(const auto& origin:model.vertex_origins)actual.insert(origin.section);
                require(expected==actual,"seeded geometry differs from original active regions");
                require(std::none_of(model.mesh.material_warnings.begin(),model.mesh.material_warnings.end(),[](const auto& warning){
                    return warning.find("unresolved asset preview")!=std::string::npos;}),"resolved geometry retained preview warning");
            }
            ++objects;
        }
        require(primary==after_prefix,"complete original cold constructor prefix final RNG differs");
        ++fixtures;
    }
    require(input.eof()&&fixtures==128&&objects==1792,"invalid original cold constructor fixtures");
}
void lifecycle_boundaries(){
    Cache cache(map_path("zanzibar"));const auto content=cache.load_content(0,true);
    const auto& pillar=placed(content,6,19);
    std::uint32_t primary=0;SourceObjectInitialization state(primary);
    state.initialize_game(0,{});
    require(!state.primary_known(),"unsupplied game initializers must remain unresolved");
    const auto& unknown=state.create(cache,pillar,0x10000u,0,true);
    require(!unknown.seed_before_known&&!unknown.constructor_consumption_known&&!unknown.selection.resolved(),
            "unknown primary stream must preserve uncertain source model regions");
    bool rejected=false;try{cache.load_model(pillar.definition,false,pillar.variant,unknown.selection);}
    catch(const FormatError&){rejected=true;}
    require(rejected,"source geometry must reject unresolved active bytes");
    state.initialize_game(0,prefix_services());
    const auto& first=state.create(cache,pillar,0x10000u,7,true);
    require(first.created_tick==7,"constructor must retain caller's exact creation tick");
    const auto first_sequence=first.creation_sequence;
    state.destroy(first.handle);require(!state.find(0x10000u),"destroyed full handle must leave construction ledger");
    primary=0xdeadbeefu; // An external source consumer mutates the same block.
    const auto& second=state.create(cache,pillar,0x20000u,19,true);
    require(second.seed_before==0xdeadbeefu&&second.creation_sequence>first_sequence&&second.created_tick==19,
            "recreation must share external primary state and explicit lifetime identity");
    const auto& unresolved=state.create(cache,pillar,0x30000u,20,false);
    require(!unresolved.constructor_consumption_known&&!state.primary_known(),
            "registered constructor services must not reuse cold-phase proof");
    ScenarioObject a;a.type=6;a.placement=3;a.name=-1;a.bsp_policy=0;a.source=1;a.origin_bsp=1;
    ScenarioObject b=a;b.type=7;b.placement=8;b.damageable=true;
    require(source_scenario_creation_mode(a)==1&&!source_scenario_initial_creation_candidate(a),
            "source unnamed non-damageable BSP creation mode differs");
    require(source_scenario_initial_creation_candidate(b),"damageable MACH must be created in initial phase");
    std::vector<ScenarioObject> objects{b,a};
    require(source_scenario_bsp_creation_attempts(objects,1)==std::vector<SourceScenarioCreationKey>{{6,3}},
            "BSP creation phase must exclude already initial damageable classes");
    a.name=0;a.unique_id=42;a.source=0;objects={a};
    std::vector<SourceScenarioObjectIdentity> existing{{42,6,3,0,2}};
    require(source_scenario_initial_creation_attempts(objects,true,existing).size()==1,
            "source full identifier must retain origin BSP for source0");
    existing[0].origin_bsp=1;
    require(source_scenario_initial_creation_attempts(objects,true,existing).empty(),
            "source existing full identifier suppression differs");
}
}
int main(){try{
    phase_fixtures();cold_prefix_fixtures();lifecycle_boundaries();
    std::cout<<"21 original creation phases and1792 cold constructor stream fixtures passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
