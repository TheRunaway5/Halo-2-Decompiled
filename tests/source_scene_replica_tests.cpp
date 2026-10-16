#include "halo2/simulation.hpp"
#include "halo2/render_interpolation.hpp"
#include <cstring>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw FormatError(message);}
template<class F> void rejects(F action,const char *message) {
    bool rejected=false;try {action();}catch(const FormatError &) {rejected=true;}
    check(rejected,message);
}
LevelContent content() {
    LevelContent c;c.player.datum=1;c.player.radius=.2f;c.player.height=1;
    for(unsigned n=0;n<2;++n) {
        ScenarioObject p;p.definition=0x12340000u+n;p.type=6;p.name=int(n);
        p.placement=int(n);p.position={float(n),0,0};c.objects.push_back(p);
        c.object_names.push_back({"replicated placement",6,int(n)});
    }
    return c;
}
void run() {
    const auto c=content();Simulation client(c,0,{});
    const auto local=client.scene().resolve(0);
    SourceSceneRoot root{0xe410002au,c.objects[0].definition,0,Transform{},true,true};
    root.created_tick=37;
    SourceInitialModelSelection region;region.variant_index=1;
    region.permutations={1};region.states={{0,2,3}};
    root.model_regions=source_model_region_bank(region);
    root.transform.position={10,20,30};
    SourceWorldPose pose{root.object,{root.transform},{{10,20,30},1}};
    // A late joiner does not allocate the authority's generations locally.
    client.replicate_source_scene(std::span(&root,1),std::span(&pose,1));
    check(client.scene().resolve(0)==root.object,"Late join retained its local scene generation");
    rejects([&]{client.scene().get(local);},"Late join retained the superseded local object");
    check(!client.object_lifetimes().contains(root.object),"Replica supplied a fabricated source body lifetime");
    check(client.scene().resolve(1)==null_datum,"Complete snapshot omission retained a local placement");
    const auto &adopted=client.scene().get(root.object);
    check(adopted.created_tick==37,"Late join replaced the authority's creation tick");
    check(adopted.source_model_regions && adopted.source_model_regions->selection.permutations==region.permutations,
        "Late join lost actual source region choices");
    check(adopted.solved_transform && adopted.solved_nodes &&
        !std::memcmp(&*adopted.solved_transform,&root.transform,sizeof(Transform)) &&
        !std::memcmp(adopted.solved_nodes->data(),pose.nodes.data(),sizeof(Transform)),
        "Late join changed the completed source root or WORLD bank");
    SceneInterpolation history;history.reset(100,client.scene().objects());
    const auto old=root.object;root.object+=0x10000u;pose.object=root.object;
    root.created_tick=101;
    root.model_regions->selection.permutations[0]=0;
    root.transform.position.x=100;pose.nodes[0]=root.transform;
    client.replicate_source_scene(std::span(&root,1),std::span(&pose,1));
    history.capture(101,client.scene().objects());
    rejects([&]{client.scene().get(old);},"Recreation retained its old full generation");
    check(history.sample(client.scene().get(root.object),0).position.x==100,
        "Replica interpolated a recreated object from its retired generation");
    check(client.scene().get(root.object).source_model_regions->selection.permutations[0]==0,
        "Recreated source owner inherited retired region choices");
    auto malformed=root;malformed.model_regions->selection.states.clear();
    rejects([&]{client.replicate_source_scene(std::span(&malformed,1),std::span(&pose,1));},
        "Replica accepted incomplete source regions");
    check(client.scene().get(root.object).source_model_regions->selection.permutations[0]==0,
        "Rejected regions partially replaced a live owner");
    auto absent=root;absent.model_regions.reset();
    client.replicate_source_scene(std::span(&absent,1),std::span(&pose,1));
    check(!client.scene().get(root.object).source_model_regions,"Absent regions retained stale choices");
    client.replicate_source_scene(std::span(&root,1),std::span(&pose,1));
    auto invalid=root;invalid.definition^=1;
    rejects([&]{client.replicate_source_scene(std::span(&invalid,1),std::span(&pose,1));},
        "Replica accepted the wrong authored definition");
    check(client.scene().resolve(0)==root.object && client.scene().get(root.object).position.x==100,
        "Rejected replica partially replaced the current scene");
    invalid=root;invalid.created_tick++;
    rejects([&]{client.replicate_source_scene(std::span(&invalid,1),std::span(&pose,1));},
        "Replica changed the creation tick of an existing full generation");
    std::array<SourceSceneRoot,2> duplicate{root,root};duplicate[1].placement=1;
    duplicate[1].definition=c.objects[1].definition;duplicate[1].object+=0x10000u;
    rejects([&]{client.replicate_source_scene(duplicate,std::span(&pose,1));},
        "Replica accepted two generations occupying one authority pool slot");
    client.replicate_source_scene({},{});
    rejects([&]{client.scene().get(root.object);},"Replica omission retained a retired object");
    client.replicate_source_scene(std::span(&root,1),std::span(&pose,1));
    check(client.scene().resolve(0)==root.object,"Replica could not adopt an object after an empty snapshot");
    // Equal bit patterns in the unrelated initial allocator cannot bind an
    // authority object to whichever local placement happened to get them.
    Simulation unrelated(c,0,{});root.object=unrelated.scene().resolve(1);pose.object=root.object;
    unrelated.replicate_source_scene(std::span(&root,1),std::span(&pose,1));
    check(unrelated.scene().get(root.object).placement==0 && unrelated.scene().resolve(1)==null_datum,
        "Late join confused an authority handle with its unrelated local placement");
    invalid=root;invalid.has_world_nodes=false;
    rejects([&]{unrelated.replicate_source_scene(std::span(&invalid,1),std::span(&pose,1));},
        "Replica accepted WORLD nodes for an explicitly empty hierarchy");
}
}
int main()try {run();std::cout<<"PASS source scene late join and generation recreation\n";return 0;}
catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
