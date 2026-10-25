// Capture the real scheduler input preceding Midship's missing BV-agent owner.
// The deliberate diagnostic stop supplies no owner return or admission result.
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include <cstdlib>
#include <iostream>
using namespace halo2;
namespace {struct CapturedInput {};}
int main(int argc,char **argv)try {
    if(argc!=4)throw FormatError("Supply MCC_MAP MCC_MAPS OWNED_OUTPUT");
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
    if(!scratch)throw FormatError("Vehicle entry capture requires tools/run_test.py");
    const auto output=std::filesystem::weakly_canonical(argv[3]);
    const auto owner=std::filesystem::weakly_canonical(scratch);
    bool owned=false;
    for(auto parent=owner;!parent.empty() && parent!=parent.parent_path();parent=parent.parent_path()) {
        const auto relative=output.lexically_relative(parent);
        if(parent.filename().string().starts_with("halo2-test-") &&
           !relative.empty() && relative!="." && *relative.begin()!="..")owned=true;
    }
    if(!owned)
        throw FormatError("Vehicle entry capture must stay inside its owned scratch");
    test::LivePhysicsEntryCaptureTu15 capture(output,true);
    std::optional<SourcePhysicsEntryObserverBinding> binding;
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
    options.local_players=1;options.source_object_world=true;options.mute=true;
    options.session_random_seed=0x23ce15u;
    options.source_world_observer=[&](const SourceRuntimeMemory &memory) {
        binding.emplace(memory,[&](bool entering,std::uint32_t entry,const auto &abi) {
            if(!entering || entry!=0x14fec0u)return;
            capture.observe(true,abi,memory,entry);
            throw CapturedInput{};
        });
    };
    try {MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);}
    catch(const CapturedInput &) {
        std::cout<<"Captured actual14FEC0 entry input; deliberate diagnostic stop, no gameplay acceptance\n";
        return std::filesystem::is_regular_file(output/"capture-entry.json")?0:1;
    }
    throw FormatError("Actual player scheduler entry was not observed");
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
