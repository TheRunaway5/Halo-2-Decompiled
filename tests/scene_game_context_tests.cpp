#include "halo2/object_initialization.hpp"
#include "halo2/multiplayer.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
struct Gate {
    int type{},game_type{},engine{};
    std::uint32_t flags{};
    int mask{};
    bool allowed{};
};
struct Placement {
    ScenarioObject object;
    bool initial_attempt{},empty_bsp_attempt{},slayer_allowed{},campaign_allowed{};
};
struct Fixture {
    std::vector<Gate> gates;
    std::map<std::string,std::vector<Placement>> maps;
};
Fixture load(const char* path) {
    std::ifstream input(path);require(bool(input),"missing untouched scene game-context fixture");
    Fixture out;std::string line;
    while(std::getline(input,line)) {
        if(line.empty() || line.front()=='#')continue;
        std::istringstream row(line);char kind;row>>kind;
        if(kind=='G') {
            Gate g;row>>g.type>>g.game_type>>g.engine>>g.flags>>g.mask>>g.allowed;
            out.gates.push_back(g);
        } else if(kind=='M') {
            std::string name;Placement p;int mask;unsigned manual;
            p.object.type=6;
            row>>name>>p.object.placement>>p.object.definition>>p.object.flags>>mask
               >>p.object.name>>p.object.source>>p.object.origin_bsp>>p.object.bsp_policy
               >>manual>>p.initial_attempt>>p.empty_bsp_attempt
               >>p.slayer_allowed>>p.campaign_allowed;
            p.object.valid_multiplayer_games=std::uint16_t(mask);
            p.object.manual_bsps=std::uint16_t(manual);
            out.maps[name].push_back(p);
        } else throw std::runtime_error("unknown scene game-context fixture row");
        require(bool(row),"short untouched scene game-context fixture row");
        std::string extra;require(!(row>>extra),"extra scene game-context fixture field");
    }
    require(input.eof() && out.gates.size()==2160 && out.maps.size()==21,
            "incomplete original game-context branch/map bank");
    return out;
}
std::set<int> active_scenery(const LevelContent& content,const SceneObjects& scene) {
    std::set<int> out;
    for(const auto& live:scene.objects())if(live.handle!=null_datum) {
        const auto& object=content.objects.at(live.placement);
        if(object.type==6) {
            require(out.insert(object.placement).second,"duplicate native scenery placement");
            require(scene.get(live.handle).placement==live.placement,
                    "native full-generation scenery identity failed");
        }
    }
    return out;
}
void gates_and_manual_creation(const Fixture& fixture) {
    unsigned overrides=0,manual_flag0=0;
    for(const auto& g:fixture.gates) {
        ScenarioObject object;object.type=g.type;object.definition=1;object.name=0;
        object.flags=g.flags;object.valid_multiplayer_games=std::uint16_t(g.mask);
        require(source_scenario_factory_game_allowed(object,g.game_type,g.engine)==g.allowed,
                "generic source factory gate differs from untouched D5060");
        LevelContent content;content.objects={object};content.object_names={{"literal gate",g.type,0}};
        SceneObjects scene(content,0,SourceSceneGameContext{g.game_type,g.engine});
        // Placement flag0 belongs to D59F0/D5640. It never becomes a second
        // factory gate when a script explicitly calls object_create.
        require((scene.resolve(0)!=null_datum)==(g.allowed && !(g.flags&1u)),
                "native automatic construction did not separate placement/factory gates");
        scene.destroy_all();
        const auto handle=scene.create(0,false,17);
        require((handle!=null_datum)==g.allowed,"native explicit factory game context was lost by script create");
        if(handle!=null_datum) {
            require(scene.get(handle).created_tick==17,"manual admitted generation lost its creation tick");
            if(g.flags&1u)++manual_flag0;
            if(g.flags&64u)++overrides;
        }
        // Campaign/default construction keeps its pre-existing flag64 rule.
        // An explicit non-multiplayer game type has exactly the captured gate.
        if(g.game_type==1) {
            SceneObjects compatibility(content,0);
            require((compatibility.resolve(0)!=null_datum)==(g.allowed && !(g.flags&1u)),
                    "no-context campaign automatic constructor changed");
            compatibility.destroy_all();
            require(compatibility.create(0)!=null_datum,
                    "no-context campaign explicit creation compatibility changed");
        }
    }
    require(overrides>0 && manual_flag0>0,"source replacement/explicit-create controls did not exercise admission");
}
void retained_context_restore() {
    // Literal original G inputs: type6, mask1, flags0, game2/engine2 rejects;
    // game1/engine2 admits. Restoration must restore the scene's context too.
    LevelContent content;ScenarioObject object;object.type=6;object.name=0;
    object.definition=1;object.valid_multiplayer_games=1;content.objects={object};
    content.object_names={{"retained context",6,0}};
    SceneObjects campaign(content,0,SourceSceneGameContext{1,2});
    SceneObjects multiplayer(content,0,SourceSceneGameContext{2,2});
    require(campaign.resolve(0)!=null_datum && multiplayer.resolve(0)==null_datum,
            "restore fixture contexts did not differ");
    campaign.restore(multiplayer);require(campaign.create(0)==null_datum,
            "scene restore retained the destination's old campaign game gate");
    SceneObjects no_context(content,0);
    multiplayer.restore(no_context);multiplayer.destroy_all();
    require(multiplayer.create(0)!=null_datum,"scene restore did not restore absent campaign context");
}
std::filesystem::path map_path(const std::filesystem::path& disc,
    const std::filesystem::path& pack,const std::string& name) {
    auto path=disc/(name+".map");return std::filesystem::exists(path)?path:pack/(name+".map");
}
void authored_maps(const Fixture& fixture,const char* disc,const char* pack) {
    unsigned total=0,generic_count=0,slayer_count=0,extra=0,different_maps=0;
    for(const auto& [name,placements]:fixture.maps) {
        Cache cache(map_path(disc,pack,name));const auto content=cache.load_content(0,true);
        std::set<int> generic_expected,slayer_expected;
        unsigned native_rows=0;
        for(const auto& object:content.objects)if(object.type==6)++native_rows;
        require(native_rows==placements.size(),"native scenery inventory differs from literal source placement bank");
        for(const auto& p:placements) {
            ++total;const auto& raw=p.object;
            const auto native=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto& value) {
                return value.type==6 && value.placement==raw.placement;
            });
            require(native!=content.objects.end() && native->definition==raw.definition && native->flags==raw.flags &&
                native->valid_multiplayer_games==raw.valid_multiplayer_games && native->name==raw.name &&
                native->source==raw.source && native->origin_bsp==raw.origin_bsp &&
                native->bsp_policy==raw.bsp_policy && native->manual_bsps==raw.manual_bsps,
                "native scenery factory fields differ from independent raw-cache fixture");
            require(source_scenario_factory_game_allowed(*native,2,2)==p.slayer_allowed &&
                source_scenario_factory_game_allowed(*native,1,2)==p.campaign_allowed,
                "actual map game gate differs from untouched source execution");
            // Both literals came from original attempted-key lists. Factory
            // successes were deliberately NONE, so these expected native sets
            // describe native final admission, not captured Xbox allocations.
            const bool attempted=p.initial_attempt || p.empty_bsp_attempt;
            if(attempted && raw.definition!=null_datum && !(raw.flags&1u)) {
                if(p.campaign_allowed)generic_expected.insert(raw.placement);
                if(p.slayer_allowed)slayer_expected.insert(raw.placement);
            }
        }
        SceneObjects generic(content,0);
        SceneObjects explicit_campaign(content,0,SourceSceneGameContext{1,2});
        SceneObjects explicit_slayer(content,0,SourceSceneGameContext{2,2});
        require(active_scenery(content,generic)==generic_expected &&
                active_scenery(content,explicit_campaign)==generic_expected,
                "campaign/default scene membership changed on actual authored placement inputs");
        require(active_scenery(content,explicit_slayer)==slayer_expected,
                "Slayer native scene membership differs from literal original attempts/game gates");
        explicit_slayer.destroy_all();explicit_slayer.switch_bsp(0);
        std::set<int> bsp_expected;
        for(const auto& p:placements)if(p.empty_bsp_attempt && p.object.definition!=null_datum &&
                !(p.object.flags&1u) && p.slayer_allowed)bsp_expected.insert(p.object.placement);
        require(active_scenery(content,explicit_slayer)==bsp_expected,
                "stored multiplayer game context was lost during automatic BSP construction");
        auto rules=slayer_rules({"SMG Slayer",0,{{0x59,5}}});
        rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
        MultiplayerMatch match(content,0,cache.spawns(),rules);
        require(active_scenery(content,match.simulation().scene())==slayer_expected,
                "production MultiplayerMatch did not supply source2/2 before scene construction");
        generic_count+=generic_expected.size();slayer_count+=slayer_expected.size();
        const auto difference=generic_expected.size()-slayer_expected.size();extra+=difference;
        if(difference)++different_maps;
        std::cout<<name<<": "<<generic_expected.size()<<" compatibility / "<<slayer_expected.size()
                 <<" source Slayer scenery candidates admitted by native scene\n";
    }
    require(total==410 && generic_count==392 && slayer_count==278 && extra==114 && different_maps==20,
            "original 21-map native/scenery game-context negative control changed");
}
}
int main(int argc,char** argv) {try {
    require(argc==4,"scene game-context test requires original fixture/disc maps/map-pack maps");
    const auto fixture=load(argv[1]);gates_and_manual_creation(fixture);
    retained_context_restore();authored_maps(fixture,argv[2],argv[3]);
    std::cout<<"2160 untouched D5060 fixtures and410 actual placements passed; 21 source Slayer scenes278 vs392 compatibility candidates; 114 excess rejected\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
