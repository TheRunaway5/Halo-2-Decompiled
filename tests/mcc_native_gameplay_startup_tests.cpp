#include "halo2/multiplayer_world.hpp"
#include <iostream>

int main(int argc,char** argv)try {
    using namespace halo2;
    if(argc!=3 && argc!=5)throw FormatError(
        "usage: mcc_native_gameplay_startup_tests MCC.map PRIVATE_SAVE_DIRECTORY [--ticks COUNT]");
    unsigned ticks{};
    if(argc==5) {
        if(std::string_view(argv[3])!="--ticks")throw FormatError("Unknown gameplay startup test option");
        std::size_t consumed{};
        const auto count=std::stoul(argv[4],&consumed);
        if(consumed!=std::string_view(argv[4]).size() || !count || count>600)
            throw FormatError("Gameplay startup test requires 1..600 ticks");
        ticks=unsigned(count);
    }
    MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=options.map.parent_path();options.local_players=1;
    options.source_object_savegame_directory=argv[2];
    options.session_random_seed=0x12345678u;
    // Ordinary construction retains map scripts and normal automatic source
    // ownership. No executable, replacement content, or injected script graph.
    MultiplayerPreparedWorld world(options);
    if(!world.cache->is_mcc() || !world.source_objects || !world.source_objects->owns_physics() ||
        !options.gameplay_xbe.empty() || !world.matches(options))
        throw FormatError("MCC startup did not retain its normal native gameplay owner");
    if(world.source_objects->object_pool().value<=UINT32_MAX ||
        world.source_objects->player_pool().value<=UINT32_MAX)
        throw FormatError("Normal MCC startup narrowed native pool ownership");
    if(world.local_slots.size()!=1)
        throw FormatError("Normal MCC startup changed its accepted local roster");
    std::cout<<"PASS normal MCC native gameplay ownership, roster and host64 pools"<<std::endl;
    const auto state=world.match.state();
    const auto slot=world.local_slots.front();
    if(slot>=state.players.size() || !state.players[slot].connected || state.players[slot].dead ||
        state.players[slot].source_unit==null_datum ||
        world.source_objects->object_body(state.players[slot].source_unit).value<=UINT32_MAX)
        throw FormatError("Normal MCC player startup did not produce its live player");
    std::cout<<"PASS normal MCC player birth"<<std::endl;
    if(state.players[slot].weapon!=world.rules.weapon ||
        (world.rules.weapon!=null_datum &&
            (state.players[slot].source_weapon==null_datum ||
                world.source_objects->object_definition(state.players[slot].source_weapon)!=world.rules.weapon)))
        throw FormatError("Normal MCC birth did not retain its authored starting equipment");
    if(ticks) {
        const std::array<Controls,multiplayer_capacity> controls{};
        for(unsigned n=0;n<ticks;++n) {
            try {world.match.step(controls);}
            catch(const std::exception& error) {
                throw FormatError("Normal MCC match continuation after "+std::to_string(n)+
                    " completed ticks: "+error.what());
            }
        }
        const auto after=world.match.state();
        if(after.players[slot].source_unit!=state.players[slot].source_unit || after.players[slot].dead)
            throw FormatError("MCC startup lost its living player during neutral simulation");
        std::cout<<"PASS normal MCC match continuation: "<<ticks<<" ticks"<<std::endl;
    }
    {
        const auto saved=world.source_objects->authority_checkpoint();
        auto prepared=world.source_objects->prepare_authority(saved);
        if(!prepared.for_owner(*world.source_objects))
            throw FormatError("Actual MCC match fixups did not admit their genuine native checkpoint");
        // Match and Simulation borrow pool views. Leave them on the live bank;
        // their complete transfer protocol owns rebinding during a real commit.
    }
    std::cout<<"PASS genuine native checkpoint admission after normal MCC match startup"<<std::endl;
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
