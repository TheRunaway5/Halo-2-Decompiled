#include "halo2/prop_sandbox.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/model_selection.hpp"
#include "helpers/multiplayer_snapshot_checks.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
using namespace halo2;
namespace {void require(bool value,const char *message){if(!value)throw FormatError(message);}}
int main(int argc,char **argv)try{
    if(argc!=4)return 77;
    Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));auto content=cache.load_content(0,true);
    const auto weapon=cache.slayer_starting_weapon(5);SceneObjects scene(content,0,SourceSceneGameContext{2,2});
    PropSandbox props(content);props.synchronize(scene);const auto rows=props.states();
    const auto at=std::find_if(rows.begin(),rows.end(),[](const auto &row){return row.placement==39;});
    require(at!=rows.end() && at->definition==3919972401u,"actual Turf panel truck missing");
    const auto handle=at->handle;const auto &definition=*content.prop_sandbox.at(39);
    require(at->regions.selection.permutations==std::vector<std::uint8_t>{0},"actual Turf truck did not begin intact");
    unsigned hits=0;std::uint32_t random=123;
    while(!props.state(handle).vitality.dead && hits<100){++hits;props.damage(handle,content.weapons.at(weapon),
        std::bit_cast<float>(1083441152u),{},false,{.random=&random});}
    const auto &state=props.state(handle);const auto &bank=state.regions;
    require(hits==87 && state.vitality.dead,"actual Turf truck damage threshold changed");
    require(bank.selection.permutations==std::vector<std::uint8_t>{1} && bank.selection.states.at(0).damage==4 &&
        bank.selection.states[0].variant_permutation==0 && bank.selection.states[0].properties==0 &&
        bank.looping_effects==std::vector<std::uint32_t>{null_datum} && random==123,
        "actual Turf singleton damaged transition changed its region/effect/RNG state");
    require(props.synchronize(scene) && !props.synchronize(scene),"model transition did not invalidate collision exactly once");
    const auto events=props.tick();require(events.size()==3 && std::all_of(events.begin(),events.end(),[&](const auto &event){return event.handle==handle && !event.remove;}),"Turf truck did not retain its three authored non-removal responses");
    require(scene.get(handle).handle==handle,"damaged truck lost its actual generation");
    for(unsigned lod=0;lod<6;++lod){
        const auto sections=source_initial_model_sections(definition.region_definition,bank.selection,lod);
        const auto expected=cache.load_model(state.definition,false,content.objects[39].variant,bank.selection,lod);
        const auto actual=cache.load_model_sections(state.definition,false,content.objects[39].variant,lod,sections);
        require(expected.vertex_origins.size()==actual.vertex_origins.size() && std::equal(expected.vertex_origins.begin(),expected.vertex_origins.end(),actual.vertex_origins.begin(),[](const auto &a,const auto &b){return a.section==b.section && a.vertex==b.vertex && a.part==b.part && a.vertex_class==b.vertex_class;}) && expected.mesh.vertices.size()==actual.mesh.vertices.size() &&
            (actual.mesh.vertices.empty() || !std::memcmp(expected.mesh.vertices.data(),actual.mesh.vertices.data(),actual.mesh.vertices.size()*sizeof(Vertex))),
            "resident damaged geometry differs from the active original section collector");
        require(expected.compound_nodes.size()==actual.compound_nodes.size() && std::equal(expected.compound_nodes.begin(),expected.compound_nodes.end(),actual.compound_nodes.begin(),[](const auto &a,const auto &b){return a.nodes==b.nodes && !std::memcmp(a.weights.data(),b.weights.data(),sizeof(a.weights));}),"resident model section API changed the LOD palette");
    }
    auto rules=slayer_rules({"Slayer",0,{}});rules.time_ticks=0;rules.weapon=weapon;
    MultiplayerMatch authority(content,0,cache.spawns(),rules);
    authority.replicate_prop_regions(props.region_snapshot());
    const auto before=authority.state();LanHost host(authority,0,0x52454749);
    LanClient client(host.join_address(),0x52454749,1);
    std::array<Controls,multiplayer_capacity> controls{};std::array<Controls,1> remote{};
    for(unsigned i=0;i<20 && !client.ready();++i){client.send(remote);host.receive(controls);host.broadcast();client.receive();}
    require(client.ready(),"prop region late join did not receive UDP state");
    test::verify_prop_region_snapshot(authority.state(),client.state());
    MultiplayerMatch replica(content,0,cache.spawns(),rules);replica.replicate_prop_regions(client.state().prop_regions);
    const auto &replicated=replica.simulation().props().state(handle).regions;
    require(replicated.selection.permutations==bank.selection.permutations && replicated.selection.states==bank.selection.states,
        "client did not apply actual completed damaged model choices");
    require(replica.simulation().props().triangles().size()==props.triangles().size(),"client damaged prop retained old collision surfaces");
    replica.replicate_prop_regions(client.state().prop_regions);
    auto invalid=client.state().prop_regions;invalid.front().definition^=1;bool rejected=false;
    try{replica.replicate_prop_regions(invalid);}catch(const FormatError&){rejected=true;}
    require(rejected,"forged prop region definition mutated client state");
    invalid=client.state().prop_regions;invalid.front().regions.selection.permutations[0]=127;rejected=false;
    try{replica.replicate_prop_regions(invalid);}catch(const FormatError&){rejected=true;}
    require(rejected,"invalid prop render permutation mutated client state");
    test::verify_prop_region_snapshot(before,replica.state());
    std::cout<<"Turf truck: 87 impacts; state4/permutation1; all six LODs; actual UDP late join; collision and rejection checks passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
