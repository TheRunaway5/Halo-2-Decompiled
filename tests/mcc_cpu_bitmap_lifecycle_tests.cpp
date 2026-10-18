#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_execution_inputs.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <net/if.h>
#include <netpacket/packet.h>
#include <nlohmann/json.hpp>
#include <thread>
using namespace halo2;
namespace {
void need(bool v,const char*m){if(!v)throw FormatError(m);}
SourceMachineIdentity machine(){ifaddrs*list{};need(!getifaddrs(&list),"native identity inventory failed");SourceMachineIdentity value{};bool found{};for(auto*p=list;p;p=p->ifa_next)if(p->ifa_addr&&p->ifa_addr->sa_family==AF_PACKET&&(p->ifa_flags&IFF_UP)&&!(p->ifa_flags&IFF_LOOPBACK)){const auto*l=reinterpret_cast<const sockaddr_ll*>(p->ifa_addr);if(l->sll_halen==value.size()){std::copy_n(l->sll_addr,value.size(),value.begin());found=true;break;}}freeifaddrs(list);need(found,"actual native machine identity absent");return value;}
template<class F>void rejected(F&&f){bool no=false;try{f();}catch(const std::exception&){no=true;}need(no,"invalid World lifecycle operation admitted");}
struct Result {std::vector<std::byte> collector,nodes;};
Result model(PhysicsLiveWorldTu15& w,unsigned unit) {
    w.begin_model_frame();w.begin_model_view();
    const auto collector=w.build_world_model(unit,0,1,false,1.f,1.f);
    const auto nodes=w.rendered_world_nodes(unit);
    need(nodes&&nodes->size()==33&&collector.bytes.size()==484,"actual completed public model scope differs");
    const auto live=w.memory().checkpoint();
    const auto saved=w.authority_checkpoint();
    unsigned omitted{};
    for(const auto&r:live.regions)
        if(std::ranges::find(saved.bank.regions,r.address,&SourceRuntimeMemory::CheckpointRegion::address)==saved.bank.regions.end()) {
            need(r.owner=="private owned sparse CPU bitmap delivery rows"||
                r.owner=="private exact CPU bitmap metadata header"||
                r.owner=="actual host physical allocation pages","public model omitted unrelated live owner");
            ++omitted;
        }
    need(omitted>=6&&omitted%3==0,"completed public model did not publish actual authenticated CPU resources");
    const auto bytes=std::as_bytes(std::span(*nodes));
    return {{collector.bytes.begin(),collector.bytes.end()},{bytes.begin(),bytes.end()}};
}
bool equal(const Result&a,const Result&b){return a.collector==b.collector&&a.nodes==b.nodes;}
}
int main(int argc,char**argv)try {
    if(argc!=3||!std::filesystem::is_regular_file(argv[1]))return 77;
    const Cache cache(argv[1]);need(cache.is_mcc(),"actual MCC source required");std::filesystem::create_directories(std::filesystem::path(argv[2])/"warm");std::filesystem::create_directories(std::filesystem::path(argv[2])/"fresh");
    PhysicsLiveWorldTu15 world(cache,std::filesystem::path(argv[2])/"warm");world.initialize_map();world.configure_session(source_game_variant_default(0),1,30,0x12345678u);
    const auto b=std::ranges::find_if(cache.tags(),[](const auto&t){return t.group=="bipd"&&t.name=="objects\\characters\\masterchief\\masterchief_mp";});need(b!=cache.tags().end()&&!cache.spawns().empty(),"actual source biped/spawn absent");
    const auto unit=world.create_biped(b->datum,cache.spawns().front().position,{1,0,0});need(unit!=0xffffffffu,"actual biped construction failed");DatumPool pool(world.memory(),world.player_pool());const auto player=pool.allocate_at(0);need(player!=DatumPool::none,"actual player allocation failed");world.construct_player(player,0,source_default_player_configuration(),machine());world.bind_player(player,unit);
    const auto definition=cache.read_tag_bytes(b->address,b->size);const auto*hlmt=cache.find_tag({Bytes(definition).u32(0x38)},TagGroup{"hlmt"});need(hlmt,"full-salt model absent");const auto*mode=cache.find_tag({Bytes(cache.read_tag_bytes(hlmt->address,8)).u32(4)},TagGroup{"mode"});need(mode,"full-salt render model absent");world.preload_render_model(mode->datum);
    const auto cold=world.authority_checkpoint(); // Genuine prior state BEFORE native lighting/model caches exist.
    std::cerr<<"stage=complete actual public model\n";const auto baseline=model(world,unit);const auto live=world.memory().checkpoint();
    unsigned groups=0,negative=0,completed=1;const auto group=[&](const char*name,auto&&f){f();++groups;std::cerr<<"PASS "<<name<<'\n';};const auto no=[&](auto&&f){rejected(f);++negative;};
    PhysicsLiveWorldTu15::AuthorityCheckpoint saved;
    group("actual World filtered export is copy-only",[&]{int observer;unsigned writes{},topology{};source_runtime_register_write_observer(world.memory(),&observer,[&](auto,auto){++writes;});source_runtime_register_topology_observer(world.memory(),&observer,[&](auto,auto){++topology;});saved=world.authority_checkpoint();source_runtime_unregister_write_observer(world.memory(),&observer);source_runtime_unregister_topology_observer(world.memory(),&observer);need(saved.version==3&&live==world.memory().checkpoint()&&!writes&&!topology,"export mutated live World or schema");auto expected=live;unsigned omitted{};std::erase_if(expected.regions,[&](const auto&r){const auto keep=std::ranges::find(saved.bank.regions,r.address,&SourceRuntimeMemory::CheckpointRegion::address);if(keep!=saved.bank.regions.end())return false;need(r.owner=="private owned sparse CPU bitmap delivery rows"||r.owner=="private exact CPU bitmap metadata header"||r.owner=="actual host physical allocation pages","unrelated actual World owner omitted");++omitted;return true;});need(omitted>=6&&omitted%3==0&&expected==saved.bank,"filtered World bank changed surviving bytes/owners");});
    group("wrong-thread and render-borrow export rejected",[&]{std::atomic<unsigned> n{};std::thread t([&]{try{(void)world.authority_checkpoint();}catch(...){++n;}});t.join();need(n==1,"foreign executor export admitted");++negative;world.inspect_render_memory([&](const auto&,const auto&){no([&]{(void)world.authority_checkpoint();});no([&]{auto p=world.prepare_authority(saved);});});need(live==world.memory().checkpoint(),"rejected export changed live bank");});
    group("malformed native identity and foreign immutable owner rejected",[&]{auto bad=saved;bad.resources.resident[0]^=std::byte{1};no([&]{auto p=world.prepare_authority(bad);});bad=saved;bad.bank.regions.push_back({0x70000000u,SourceRuntimeMemory::Access::ReadOnly,"foreign same-name CPU header",0,SourceRuntimeSnapshotBytes(std::vector<std::byte>(20)),{}});std::ranges::sort(bad.bank.regions,{},&SourceRuntimeMemory::CheckpointRegion::address);no([&]{auto p=world.prepare_authority(bad);});need(live==world.memory().checkpoint(),"malformed preparation changed actual World");});
    group("cancelled actual preparation retains warm resources",[&]{{auto p=world.prepare_authority(saved);}need(live==world.memory().checkpoint(),"cancelled preparation changed live World");const auto again=model(world,unit);++completed;need(equal(again,baseline),"model changed after cancelled preparation");});
    auto current=world.authority_checkpoint();
    group("denied actual commit retains current complete World",[&]{auto&memory=world.memory();const auto before=memory.checkpoint();{auto p=world.prepare_authority(current);int guard;source_runtime_register_topology_guard(memory,&guard,[](auto at,auto n){if(!at&&!n)throw FormatError("declared denied World swap");});need(!world.commit_authority(std::move(p)),"denied actual swap committed");source_runtime_unregister_topology_guard(memory,&guard);}++negative;need(before==world.memory().checkpoint(),"denied commit changed live World");});
    group("same-World commit drops stale resources after swap",[&]{auto p=world.prepare_authority(cold);need(world.commit_authority(std::move(p)),"genuine same-World restore failed");need(world.memory().checkpoint()==cold.bank,"successful restore retained excluded CPU resource bytes");need(world.world().value>UINT32_MAX,"World native allocation was narrowed");const auto again=model(world,unit);++completed;need(equal(again,baseline),"same-World regenerated model differs");});
    group("fresh World accepts filtered authority and regenerates resources",[&]{PhysicsLiveWorldTu15 fresh(cache,std::filesystem::path(argv[2])/"fresh");fresh.initialize_map();fresh.configure_session(source_game_variant_default(0),1,30,0x12345678u);auto p=fresh.prepare_authority(cold);need(fresh.commit_authority(std::move(p)),"fresh-World restore failed");need(fresh.memory().checkpoint()==cold.bank,"fresh-World restore changed filtered bytes");const auto regenerated=model(fresh,unit);++completed;need(equal(regenerated,baseline),"fresh-World regenerated model differs");});
    group("completed genuine source-store fault recovers prior authority",[&]{
        auto&memory=world.memory();const auto recovery=cold;const auto before=memory.checkpoint();int guard;bool reached=false;
        source_runtime_register_write_guard(memory,&guard,[&](auto at,auto n){if(at==0x556554u&&n==4){reached=true;throw FormatError("declared later genuine CRT store denial");}});
        no([&]{world.call_actual(0x381010u);});source_runtime_unregister_write_guard(memory,&guard);
        // Authenticated381010 is a load, one store, and RET. Denial of its
        // only store genuinely faults the source continuation but does not
        // imply earlier changed memory; partial delivery writes are tested below.
        need(reached,"genuine CRT source-store fault was not reached");
        no([&]{(void)world.authority_checkpoint();});
        auto p=world.prepare_authority(recovery);need(world.commit_authority(std::move(p)),"completed source-fault recovery rejected");
        need(memory.checkpoint()==recovery.bank,"fault recovery changed admitted prior bytes");
        need(equal(model(world,unit),baseline),"source-fault recovery did not regenerate model");++completed;
    });
    group("idle nontracking input binding preserves existing restore semantics",[&]{
        auto&memory=world.memory();const auto recovery=cold;SourceExecutionInputs journal;
        {SourceExecutionInputBinding binding(memory,journal);auto p=world.prepare_authority(recovery);need(world.commit_authority(std::move(p)),"idle bound journal restore rejected");no([&]{(void)world.authority_checkpoint();});}
        (void)journal.finish();need(memory.checkpoint()==recovery.bank,"idle bound restore changed source bytes");
        need(equal(model(world,unit),baseline),"idle bound restore did not regenerate model");++completed;
    });
    group("actual poisoned texture refresh restores prior completed authority",[&]{
        auto&memory=world.memory();const auto recovery=cold;
        const auto completed_snapshot=world.authority_checkpoint();
        // Strict export has already authenticated and omitted these actual
        // derived allocations. Labels here select a fault-injection witness,
        // never a production admission or restore exclusion capability.
        std::vector<unsigned> stamps;
        const auto before=memory.checkpoint();
        for(const auto&region:before.regions)if(region.owner=="private owned sparse CPU bitmap delivery rows") {
            need(std::ranges::find(completed_snapshot.bank.regions,region.address,&SourceRuntimeMemory::CheckpointRegion::address)==completed_snapshot.bank.regions.end(),"strict export did not authenticate the injection region");
            for(std::size_t offset=0;offset+116<=region.bytes.size();offset+=116) {
                const auto raw=std::span<const std::byte>(region.bytes).subspan(offset,116);
                const auto format=Bytes(raw).u16(12);const auto bytes=Bytes(raw).u32(0x34);
                if((format!=11&&format!=16)||!bytes)continue;
                const auto header=Bytes(raw).u32(0x50),encoded=Bytes(raw).u32(0x54);
                need(memory.is_read_only(header,20)&&memory.view(encoded,bytes).size()==bytes,"actual injection resource lease is not present");
                stamps.push_back(region.address+unsigned(offset)+0x70);
            }
        }
        need(!stamps.empty(),"actual strict-exported CPU resource row absent");
        // A source-created second object has no cached lighting result and
        // consumes the same real map resources. Repeating the original warm
        // object can legitimately reuse original RGB without a texture read.
        const auto probe=world.create_biped(b->datum,cache.spawns().front().position,{1,0,0});need(probe!=0xffffffffu,"actual new source biped absent");
        int observer;unsigned reached{};
        source_runtime_register_write_observer(memory,&observer,[&](auto p,auto n){if(n==4&&std::ranges::find(stamps,p)!=stamps.end()){++reached;throw FormatError("declared actual resource stamp fault");}});
        bool faulted=false;try{(void)model(world,probe);}catch(const std::exception&){faulted=true;}source_runtime_unregister_write_observer(memory,&observer);need(faulted&&reached==1,"actual new-object resource refresh did not fault");++negative;
        no([&]{(void)world.authority_checkpoint();});
        {auto prepared=world.prepare_authority(recovery);need(world.commit_authority(std::move(prepared)),"poisoned texture owner blocked genuine prior recovery");}
        need(memory.checkpoint()==recovery.bank,"poisoned texture recovery changed prior bytes");need(equal(model(world,unit),baseline),"poisoned texture did not regenerate");++completed;
    });
    for(const bool deny_rollback:{false,true})group(deny_rollback?"actual failed-stage/rollback pending leases recover":"actual partial-stage receipts recover",[&]{
        auto&memory=world.memory();const auto recovery=cold;
        {auto prepared=world.prepare_authority(recovery);need(world.commit_authority(std::move(prepared)),"resource-cold authority staging failed");}
        int guard;unsigned row{};bool failed{};unsigned denied{};
        source_runtime_register_topology_guard(memory,&guard,[&](auto at,auto n){
            if(!failed&&at>=0x80000000u&&at<0x90000000u&&n==24)row=at;
            if(row&&at==row+0x34u&&n==4){failed=true;throw FormatError("declared actual second delivery commit failure");}
            if(deny_rollback&&failed&&at>=0x80000000u&&at<0x90000000u&&(n==20||n>=4096)) {++denied;throw FormatError("declared actual resource rollback denial");}
        });
        no([&]{(void)model(world,unit);});source_runtime_unregister_topology_guard(memory,&guard);
        need(failed&&row&&(!deny_rollback||denied>=2),"actual partial stage/rollback failure not reached");
        no([&]{(void)world.authority_checkpoint();});
        {auto prepared=world.prepare_authority(recovery);need(world.commit_authority(std::move(prepared)),"actual pending texture receipts blocked prior restore");}
        need(memory.checkpoint()==recovery.bank,"actual partial-stage recovery changed prior bytes");need(equal(model(world,unit),baseline),"partial-stage texture did not regenerate");++completed;
    });
    std::cout<<nlohmann::json{{"status","PASS private CURRENT World CPU bitmap authority lifecycle"},{"groups",groups},{"negative_controls",negative},{"completed_public_model_calls",completed},{"model_collector_bytes",baseline.collector.size()},{"model_nodes",33},{"authority_version",saved.version},{"scope","Actual MCC normal-Cache constructor/current World authority export/prepare/cancel/denied/same/fresh restore and real public model path; no GPU/framebuffer/all-map/campaign parity claim"}}.dump(2)<<'\n';
}catch(const std::exception&e){std::cerr<<"actual World lifecycle: "<<e.what()<<'\n';return 1;}
