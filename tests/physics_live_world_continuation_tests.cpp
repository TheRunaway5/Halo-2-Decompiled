#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/native_object_lifetimes.hpp"
#include "halo2/player_command.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ifaddrs.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <unistd.h>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F &&f){bool failed=false;try{f();}catch(const std::exception &){failed=true;}check(failed,"Invalid or active authority checkpoint accepted");}
struct Directory {
    std::filesystem::path path;
    Directory(){auto name=(std::filesystem::temp_directory_path()/"halo2-physics-continuation-XXXXXX").string();check(::mkdtemp(name.data()),"Cannot create native test directory");path=name;}
    ~Directory(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t time{};
    mutable std::atomic<std::uint64_t> counter{};
    std::uint32_t milliseconds()const noexcept override{return time;}
    std::uint64_t cycles()const noexcept override{return counter.fetch_add(1000);}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
SourceMachineIdentity machine() {
    ifaddrs *interfaces=nullptr;check(!::getifaddrs(&interfaces),"Native interface inventory failed");
    SourceMachineIdentity result{};bool found=false;
    for(auto *at=interfaces;at;at=at->ifa_next)if(at->ifa_addr && at->ifa_addr->sa_family==AF_PACKET &&
        (at->ifa_flags&IFF_UP) && !(at->ifa_flags&IFF_LOOPBACK)) {
        const auto link=reinterpret_cast<const sockaddr_ll*>(at->ifa_addr);
        if(link->sll_halen==result.size()){std::copy_n(link->sll_addr,result.size(),result.begin());found=true;break;}
    }
    ::freeifaddrs(interfaces);check(found,"No actual native machine identity");return result;
}
void command_axis(std::array<std::byte,0x5c> &command,float value) {
    const auto bits=std::bit_cast<std::uint32_t>(value);
    for(unsigned i=0;i<4;++i)command[0xcu+i]=std::byte(bits>>(8*i));
}
void shared_resources(const Cache &cache) {
    Directory directory;
    const auto path=directory.path/"shared.map";
    {std::ofstream file(path,std::ios::binary);for(unsigned i=0;i<1024;++i)file.put(char(i&255u));}
    Cache owned(cache.path(),directory.path);
    SourceRuntimeResources first(owned);const auto identity=first.identity();
    check(first.load(0x80000025u,2)==std::vector<std::byte>{std::byte{37},std::byte{38}},"Pinned shared resource read differs");
    {std::ofstream file(path,std::ios::binary|std::ios::trunc);file.put('x');}
    check(first.identity()==identity && first.load(0x80000025u,2)[0]==std::byte{37},"Source file mutation changed pinned binding");
    SourceRuntimeResources changed(owned);check(changed.identity()!=identity,"Different resource content retained the same identity");
    std::filesystem::remove(path);
    check(first.load(0x80000025u,2)[0]==std::byte{37},"Unlink changed an admitted resource owner");
    rejects([&]{first.load(0x40000000u,4);});rejects([&]{first.load(0x80000400u,1);});
}
}
int main(int argc,char **argv) try {
    std::cout<<std::unitbuf;
    check(argc==3,"Supply the owned TU1.5 executable and Xbox map");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2]);shared_resources(cache);
    Directory source_files,target_files;
    auto source_clock=std::make_shared<Clock>(),target_clock=std::make_shared<Clock>();
    source_clock->time=1000;source_clock->counter=100000;target_clock->time=900000;target_clock->counter=90000000;
    PhysicsLiveWorldTu15 source(cache,argv[1],source_files.path,0,source_clock);
    PhysicsLiveWorldTu15 target(cache,argv[1],target_files.path,0,target_clock);
    rejects([&]{source.authority_checkpoint();});
    source.initialize_map();auto variant=source_game_variant_default(0);source_game_variant_set(variant,0x59,5);
    source.configure_session(variant,1,30,0x12345678);
    target.initialize_map();target.configure_session(variant,1,30,0x12345678);
    NativeObjectLifetimes source_lifetimes(source.memory(),source.object_pool());
    NativeObjectLifetimes target_lifetimes(target.memory(),target.object_pool());
    DatumPool imported(target.memory(),target.player_pool());
    const auto biped=std::ranges::find_if(cache.tags(),[](const auto &tag){return tag.group=="bipd" && tag.name=="objects\\characters\\masterchief\\masterchief_mp";});
    check(biped!=cache.tags().end() && !cache.spawns().empty(),"Owned multiplayer definition/spawn unavailable");
    const auto unit=source.create_biped(biped->datum,cache.spawns().front().position,{1,0,0});
    check(unit!=0xffffffffu,"Actual source biped creation failed");
    source_lifetimes.observe_source_birth(unit);
    DatumPool players(source.memory(),source.player_pool());const auto player=players.allocate_at(0);
    check(player!=DatumPool::none,"Actual source player allocation failed");
    source.construct_player(player,0,source_default_player_configuration(),machine());source.bind_player(player,unit);
    source.initialize_player_equipment(player,unit);
    std::array<std::byte,0x5c> command;initialize_source_neutral_command(command);command_axis(command,1.f);
    for(unsigned tick=0;tick<10;++tick){source_clock->time+=33;source.deliver_command(player,unit,command);source.step_objects();}
    std::cout<<"Actual configured source world completed ten controller/physics ticks\n";
    bool inside_call=false;
    source.step_physics([&](const SourcePhysicsContactRecordStore &) {
        if(inside_call)return;rejects([&]{source.authority_checkpoint();});inside_call=true;
    });
    check(inside_call,"No actual source write exercised the in-call export guard");
    source.begin_model_frame();const auto model=source.build_world_model(unit,0,1,false,10.f,1.f);
    check(model.address && Bytes(model.bytes).u32(0)==unit,"Actual source model collector absent");
    rejects([&]{source.source_object_body(unit^0x10000u);});
    rejects([&]{source.preload_render_model(0xffffffffu);}); // Rejected preconditions did not mutate the world.
    const auto saved=source.authority_checkpoint();
    check(std::ranges::count(saved.exit_callbacks,0x383b30u)==1 &&
        Bytes(source.memory().view(0x48cdc4u,4)).u32(0)==0x41d638u,
        "Original static contact-agent initializer/destructor binding absent");
    const auto saved_lifetimes=source_lifetimes.source_authority_checkpoint();
    const auto saved_players=players.source_binding();
    check(saved.model_scratch_mounted && !saved.model_collectors.empty() && !saved.resource_reads.empty(),
        "Actual source model/resource sidecars were not included");
    auto &source_bank=source.memory();auto &target_bank=target.memory();
    const auto before=target_bank.checkpoint();const auto old_native=target_bank.native_address(0x10000,4);
    {
        auto staged=target.prepare_authority(saved);
        auto staged_lifetimes=target_lifetimes.prepare_source_authority(saved_lifetimes,staged.replacement_memory());
        auto staged_players=imported.prepare_source_view(saved_players,staged.replacement_memory());
        check(!target_lifetimes.commit_source_authority(std::move(staged_lifetimes)) &&
            !imported.commit_source_view(std::move(staged_players)),"Pool views published before the physics bank");
        check(staged.for_owner(target) && !source.commit_authority(std::move(staged)),"Foreign world accepted authority resources");
        rejects([&]{target.authority_checkpoint();});rejects([&]{target.call_actual(0x1490b0u);});
        rejects([&]{target.prepare_authority(saved);});
        check(target_bank.checkpoint()==before,"Preparation changed current world backing");
    }
    check(target_bank.checkpoint()==before,"Cancelled preparation changed current world");
    auto bad=saved;++bad.version;rejects([&]{target.prepare_authority(bad);});
    bad=saved;++bad.bsp;rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.resources.resident[0]^=std::byte{1};rejects([&]{target.prepare_authority(bad);});
    bad=saved;++bad.main_thread_token;rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.exit_callbacks.push_back(0x12345678);rejects([&]{target.prepare_authority(bad);});
    bad=saved;std::erase(bad.exit_callbacks,0x383b30u);rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.exit_callbacks.push_back(0x383b30u);rejects([&]{target.prepare_authority(bad);});
    bad=saved;
    const auto static_agent=std::ranges::find_if(bad.bank.regions,[](const auto &region){
        return region.address<=0x48cdc4u && std::uint64_t(region.address)+region.bytes.size()>=0x48cdc8u;});
    check(static_agent!=bad.bank.regions.end(),"Source static contact-agent region absent");
    static_agent->bytes[0x48cdc4u-static_agent->address]^=std::byte{1};
    rejects([&]{target.prepare_authority(bad);});
    bad=saved;++bad.next_resource_read;rejects([&]{target.prepare_authority(bad);});
    bad=saved;bad.model_scratch_mounted=!bad.model_scratch_mounted;rejects([&]{target.prepare_authority(bad);});
    bad=saved;const auto immutable=std::ranges::find(bad.bank.regions,SourceRuntimeMemory::Access::ReadOnly,&SourceRuntimeMemory::CheckpointRegion::access);
    check(immutable!=bad.bank.regions.end(),"Source image immutable region absent");immutable->bytes[0]^=std::byte{1};
    rejects([&]{target.prepare_authority(bad);});
    check(target_bank.checkpoint()==before,"Rejected world import changed existing authority");
    auto staged=target.prepare_authority(saved);
    auto staged_lifetimes=target_lifetimes.prepare_source_authority(saved_lifetimes,staged.replacement_memory());
    auto staged_players=imported.prepare_source_view(saved_players,staged.replacement_memory());
    check(target.commit_authority(std::move(staged)),"World authority publication failed");
    check(target_lifetimes.commit_source_authority(std::move(staged_lifetimes)) &&
        imported.commit_source_view(std::move(staged_players)),"Outer source pool/lifetime publication failed");
    check(target_bank.checkpoint()==source_bank.checkpoint(),"Published complete source bank differs");
    rejects([&]{target_bank.view(old_native,4);});
    check(target.source_object_body(unit)==source.source_object_body(unit),"Imported full salted object identity diverged");
    check(!imported.lookup(player).empty() && std::ranges::equal(imported.lookup(player),players.lookup(player)),
        "Imported full player salt/record disappeared");
    check(target_lifetimes.source_authority_checkpoint()==saved_lifetimes && target_lifetimes.contains(unit),
        "Actual source body lifetime sidecars diverged");
    // Native input bases differ. Monotonic rebasing must preserve later source
    // profiling/timer writes as well as gameplay bodies under equal inputs.
    for(unsigned tick=0;tick<60;++tick) {
        source_clock->time+=33;target_clock->time+=33;
        if(tick==30)command_axis(command,0.f);
        source.deliver_command(player,unit,command);source.step_objects();
        target.deliver_command(player,unit,command);target.step_objects();
        check(target_bank.checkpoint()==source_bank.checkpoint(),"Future complete source execution diverged after import");
    }
    std::cout<<"Imported world matched sixty later complete source ticks under controlled native clock inputs\n";
    source.begin_model_frame();target.begin_model_frame();
    const auto source_model=source.build_world_model(unit,1,2,false,0.f,1.f);
    const auto target_model=target.build_world_model(unit,1,2,false,0.f,1.f);
    check(source_model.address==target_model.address && source_model.bytes==target_model.bytes &&
        source_bank.checkpoint()==target_bank.checkpoint(),"Future source model allocation/build diverged");
    const auto born_source=source.create_biped(biped->datum,cache.spawns().back().position,{1,0,0});
    const auto born_target=target.create_biped(biped->datum,cache.spawns().back().position,{1,0,0});
    source_lifetimes.observe_source_birth(born_source);target_lifetimes.observe_source_birth(born_target);
    check(born_source==born_target && born_source!=0xffffffffu && source_bank.checkpoint()==target_bank.checkpoint(),
        "Future original body/physics allocation or RNG diverged");
    check(source_lifetimes.source_authority_checkpoint()==target_lifetimes.source_authority_checkpoint(),
        "Later actual source body lifetime order diverged");
    source.destroy_object(born_source);target.destroy_object(born_target);
    source_lifetimes.observe_source_retirement(born_source);target_lifetimes.observe_source_retirement(born_target);
    check(!source_lifetimes.contains(born_source) && !target_lifetimes.contains(born_target) &&
        source_lifetimes.source_authority_checkpoint()==target_lifetimes.source_authority_checkpoint() &&
        source_bank.checkpoint()==target_bank.checkpoint(),"Actual post-import body retirement diverged");
    // Repeat the handoff; no object/player reconstruction or token reset.
    auto again=source.authority_checkpoint();auto repeated=target.prepare_authority(again);
    auto repeated_lifetimes=target_lifetimes.prepare_source_authority(source_lifetimes.source_authority_checkpoint(),repeated.replacement_memory());
    auto repeated_players=imported.prepare_source_view(players.source_binding(),repeated.replacement_memory());
    check(target.commit_authority(std::move(repeated)) && target_lifetimes.commit_source_authority(std::move(repeated_lifetimes)) &&
        imported.commit_source_view(std::move(repeated_players)) && target_bank.checkpoint()==source_bank.checkpoint(),"Repeated authority handoff failed");
    rejects([&]{target.call_actual(0x12345678u);});
    rejects([&]{target.authority_checkpoint();}); // A failed source call cannot advertise completed authority.
    auto recovered=target.prepare_authority(again);
    auto recovered_lifetimes=target_lifetimes.prepare_source_authority(source_lifetimes.source_authority_checkpoint(),recovered.replacement_memory());
    auto recovered_players=imported.prepare_source_view(players.source_binding(),recovered.replacement_memory());
    check(target.commit_authority(std::move(recovered)) && target_lifetimes.commit_source_authority(std::move(recovered_lifetimes)) &&
        imported.commit_source_view(std::move(recovered_players)) && target_bank.checkpoint()==source_bank.checkpoint(),
        "Known-good authority could not replace a faulted source call");
    std::cout<<"PASS complete source physics-owner staging/publication and outer source player/object pool views/lifetime sidecars, immutable resource/image binding, rollback, salted identities, clock rebasing and future source continuation\n";
    return 0;
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
