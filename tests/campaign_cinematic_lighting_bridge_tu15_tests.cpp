#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/simulation.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include <cstring>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    auto content=cache.load_content(0,false);
    auto source=std::make_shared<CampaignSourceRuntime>(cache,argv[1],0,1,0x12345678u);
    Simulation simulation(content,BspContent{std::move(content.collision),content.navigation},0,cache.spawns().at(0),
        [&](auto graph,auto name){return cache.load_named_animation(graph,name);},{},{},{},{},{},source);
    simulation.run_script("x01_02_setup");simulation.step({});
    const auto dervish=simulation.scene().resolve(0);
    if(dervish==null_datum)throw FormatError("Authored scene setup did not create Dervish");
    auto &world=source->objects().tu15_world();
    const auto bank=Bytes(world.memory().view(0x4eabc4u,4)).u32(0);
    if(!bank)throw FormatError("Original lighting bank absent");
    unsigned count{};
    for(const auto &script:content.scripts.scripts)if(script.name.starts_with("cinematic_lighting_")) {
        simulation.run_script(script.name);simulation.step({});
        if(!simulation.scripts().diagnostics().empty())throw FormatError(simulation.scripts().diagnostics().back());
        const auto *cinematic=simulation.cinematic_state();
        if(!cinematic)throw FormatError("Authored lighting script did not publish cinematic state");
        const auto expected=cinematic->lighting.source_values();
        if(std::memcmp(expected.data(),world.memory().view(bank+0x1cu,80).data(),80))
            throw FormatError("Authored cinematic lighting differs between script renderer and original source bank: "+script.name);
        ++count;
    }
    if(count!=13)throw FormatError("Incomplete opening lighting script coverage");
    const auto body=world.source_object_body(dervish);
    if(!(Bytes(world.memory().view(body+4u,4)).u32(0)&0x01000000u))
        throw FormatError("Dervish cinematic-lighting command did not reach original object flag");
    std::cout<<"PASS all13 actual opening lighting scripts share exact original80-byte bank and Dervish bit24\n";
    std::cout<<"LIMIT CPU/script handoff only; not original framebuffer or visible shadow acceptance\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
