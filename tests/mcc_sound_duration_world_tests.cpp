#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/mcc_sound_timing_reference.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <thread>

using namespace halo2;
namespace {
void require(bool yes,const char *message){if(!yes)throw FormatError(message);}
}
int main(int argc,char **argv)try {
    if(argc!=3 || std::string(argv[1]).empty() || !std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);require(cache.is_mcc(),"Native duration integration requires an owned MCC map");
    std::optional<MccSoundRuntimePlan> sound;
    const auto delivery=MccRuntimeCacheDelivery::plan(cache,0x80061000u,256u*1024u*1024u,
        [&](MccSoundRuntimePlan &&plan){require(!sound,"Duplicate sound ownership transfer");sound.emplace(std::move(plan));});
    require(sound && delivery.issues().empty(),"Incomplete sound metadata delivery");
    const auto timing=build_mcc_sound_timing_reference(cache,*sound,
        [&](auto address,auto bytes){return delivery.runtime_address(address,bytes);});
    require(!timing.bindings().empty(),"No authenticated original sound timing bindings");
    std::set<std::size_t> selected{0,timing.bindings().size()-1};
    for(std::size_t i=0;i<timing.bindings().size();++i) {
        const auto &binding=timing.bindings()[i];
        const auto definition=std::ranges::find(sound->sounds,binding.sound,&MccSoundRuntimeDefinition::sound);
        if(definition!=sound->sounds.end() && definition->codec.compression==5){selected.insert(i);break;}
    }
    PhysicsLiveWorldTu15 world(cache,argv[2]);auto &memory=world.memory();unsigned cases{};
    world.initialize_map();world.configure_session(source_game_variant_default(0),1,30,0x12345678u);
    const auto native_base=Bytes(memory.view(0x556d90u,4)).u32(0);
    bool active_topology_rejected{};
    for(const auto index:selected) {
        const auto &binding=timing.bindings()[index];
        const auto *tag=cache.find_tag(binding.sound,TagGroup::from_string("snd!"));
        require(tag,"Actual sound root is missing");
        const auto native=world.resident_cache_address(cache,tag->address,20);
        require(native.value>0xffffffffu,"Native sound backing unexpectedly fits a serialized32-bit address");
        const auto logical=memory.source_address(native,20);
        std::array<std::byte,20> before;memory.read(logical,before);
        const std::array arguments{binding.relative_pitch,binding.relative_permutation,0u};
        auto abi=world.prepare_source_call(arguments);abi.gp[0]=binding.sound.value;
        const auto initial_sp=abi.gp[4];std::vector<SourcePhysicsContactRecordStore> stores;
        world.run_actual(0x21f6b0u,abi,[&](const auto &store){
            if(store.instruction>=0x21f590u && store.instruction<=0x21f662u)stores.push_back(store);
            if(store.instruction==0x21f5a1u && !active_topology_rejected) {
                try{memory.release(native_base);}
                catch(const std::exception &e){active_topology_rejected=
                    std::string(e.what()).find("topology changed during execution")!=std::string::npos;}
                require(active_topology_rejected,"Active duration allowed its native bank to be released");
            }
        });
        std::array<std::byte,20> after;memory.read(logical,after);
        require(before==after,"Native duration changed actual MCC sound bytes");
        require(stores.size()==3 && stores[0].instruction==0x21f5a1u &&
            stores[1].instruction==0x21f5c0u && stores[2].instruction==0x21f5feu,
            "Native duration omitted original stack stores");
        require(abi.gp[4]==initial_sp+16u && abi.x87_depth==1 && std::isfinite(abi.x87[0]) && abi.x87[0]>0,
            "Genuine duration caller did not complete its original ABI");
        auto repeated=world.prepare_source_call(arguments);repeated.gp[0]=binding.sound.value;
        world.run_actual(0x21f6b0u,repeated);
        require(repeated.x87[0]==abi.x87[0],"Retained actual resource changed duration on repeat");++cases;
    }
    require(active_topology_rejected,"No active timing topology control executed");
    // A rejected commit must not retire the existing proof. A successful
    // identical-byte restore must issue a fresh epoch over the real bank.
    const auto saved=world.authority_checkpoint();
    {
        auto prepared=world.prepare_authority(saved);int deny_owner{};
        source_runtime_register_topology_guard(memory,&deny_owner,[](auto,auto){throw FormatError("Test denied backing swap");});
        const bool denied=world.commit_authority(std::move(prepared));
        source_runtime_unregister_topology_guard(memory,&deny_owner);
        require(!denied,"Denied authority swap committed");
    }
    const auto &last=timing.bindings()[*selected.rbegin()];
    const std::array last_arguments{last.relative_pitch,last.relative_permutation,0u};
    auto after_denial=world.prepare_source_call(last_arguments);after_denial.gp[0]=last.sound.value;
    world.run_actual(0x21f6b0u,after_denial);
    auto replacement=world.prepare_authority(saved);
    require(world.commit_authority(std::move(replacement)),"Genuine authority swap failed");
    auto after_restore=world.prepare_source_call(last_arguments);after_restore.gp[0]=last.sound.value;
    world.run_actual(0x21f6b0u,after_restore);
    require(after_restore.x87[0]==after_denial.x87[0],"Restored World did not rebind original timing");
    auto direct=world.prepare_source_call({});const auto before=direct;unsigned direct_stores{};bool rejected{};
    try{world.run_actual(0x21f590u,direct,[&](const auto&){++direct_stores;});}
    catch(const std::exception &e){rejected=std::string(e.what()).find("authenticated original caller")!=std::string::npos;}
    require(rejected && !direct_stores && direct.gp==before.gp && direct.x87_depth==before.x87_depth,
        "Foreign direct duration call executed or changed its ABI");
    bool foreign_rejected{};std::thread foreign([&]{
        try{world.run_actual(0x21f6b0u,direct);}
        catch(const std::exception&){foreign_rejected=true;}
    });foreign.join();require(foreign_rejected,"Foreign executor entered World duration");
    std::cout<<"Actual MCC World duration cases="<<cases<<" with original stores, native64 backing, caller/thread, active-topology and denied/committed restore controls PASS\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
