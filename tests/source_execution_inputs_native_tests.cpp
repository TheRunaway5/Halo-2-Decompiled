#include "source_match_rebind_fixture.hpp"
#include "halo2/source_execution_inputs.hpp"
#include <set>

namespace {
void same(Owner &host,Owner &replica){
    same_gameplay(host,replica);
    const auto a=host.authority->checkpoint(binding),b=replica.authority->checkpoint(binding);
    check(a.match.result_events.start==b.match.result_events.start&&a.match.result_events.last==b.match.result_events.last&&
        a.match.result_events.index==b.match.result_events.index&&a.match.result_events.events==b.match.result_events.events,
        "Replay sampled another PC's PCR/event wall clock");
    if(!(a.world.world.logical_milliseconds==b.world.world.logical_milliseconds&&
        a.world.world.logical_cycles==b.world.world.logical_cycles&&
        a.world.world.logical_utc_filetime==b.world.world.logical_utc_filetime&&
        a.match.logical_result_seconds==b.match.logical_result_seconds))
        throw FormatError("Replay/export anchor mismatch: host/replica ms="+std::to_string(a.world.world.logical_milliseconds)+"/"+std::to_string(b.world.world.logical_milliseconds)+
            " cycles="+std::to_string(a.world.world.logical_cycles)+"/"+std::to_string(b.world.world.logical_cycles)+
            " UTC="+std::to_string(a.world.world.logical_utc_filetime)+"/"+std::to_string(b.world.world.logical_utc_filetime)+
            " result="+std::to_string(a.match.logical_result_seconds)+"/"+std::to_string(b.match.logical_result_seconds));
    if(a.world.world.bank!=b.world.world.bank){
        for(const auto &r:a.world.world.bank.regions){const auto at=std::ranges::find(b.world.world.bank.regions,r.address,&SourceRuntimeMemory::CheckpointRegion::address);
            if(at==b.world.world.bank.regions.end())throw FormatError("Replay lost a complete native bank region");
            for(std::size_t i=0;i<r.bytes.size();++i)if(r.bytes[i]!=at->bytes[i])
                throw FormatError("Replay native bank differs at "+std::to_string(std::uint64_t(r.address)+i));}
        throw FormatError("Replay bank topology/ownership metadata differs");
    }
}
}
int main(int argc,char **argv)try {
    if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    using K=SourceExecutionInputKind;std::uint32_t host_wall=1,replica_wall=906;bool unavailable=false;
    const std::array<unsigned,4> locals{0,1,2,3};
    Owner host(argv[1],argv[2],locals,1000,{},[&]{return host_wall;}),replica(argv[1],argv[2],locals,900000,{},[&]{
        if(unavailable)throw FormatError("Actual result clock unavailable");return replica_wall;});
    replica.clock->filetime+=600000000;
    const auto cut=host.authority->checkpoint(binding);auto prepared=replica.authority->prepare(cut,binding);
    check(replica.authority->commit(std::move(prepared)),"Native execution-input import failed");
    same(host,replica);
    std::set<K> kinds;std::size_t readings=0;
    for(unsigned tick=0;tick<8;++tick){
        host_wall=1+tick;replica_wall=906+tick*17;replica.clock->time+=6000;replica.clock->counter.fetch_add(1000000);
        host.clock->filetime+=10000;replica.clock->filetime+=50000;
        std::array<Controls,16> controls{};for(unsigned id=0;id<8;++id)controls[id].yaw=host.match->unit(id).yaw;
        controls[0].fire=tick<4;controls[0].grenade=tick==5;
        const auto phase=host.authority->capture_execution_phase([&]{host.step(controls);host.world->advance_effect_frame(1.f/60.f);});
        for(const auto &value:phase.inputs)kinds.insert(value.kind);readings+=phase.inputs.size();
        replica.authority->replay_execution_phase(phase,[&]{replica.step(controls);replica.world->advance_effect_frame(1.f/60.f);});
        same(host,replica);
    }
    check(kinds.contains(K::ResultWallSeconds)&&kinds.contains(K::ReciprocalSquareRoot),"Native fixture did not consume actual result/CPU inputs");
    // After replay, ordinary providers advance in the producer's anchored
    // domains without any tape or source function return substitution.
    for(unsigned tick=0;tick<40;++tick){
        ++host_wall;++replica_wall;host.clock->filetime+=330000;replica.clock->filetime+=330000;
        host.step({});host.world->advance_effect_frame(1.f/60.f);
        replica.step({});replica.world->advance_effect_frame(1.f/60.f);same(host,replica);
    }
    const auto tick=host.match->simulation().ticks();
    rejects([&]{host.authority->capture_execution_phase([&]{host.step({});host.world->advance_effect_frame(1.f/60.f);},1);});
    check(host.match->simulation().ticks()==tick+1,"Transcript overflow interrupted the actual host");
    const auto before_bad=host.authority->checkpoint(binding);
    prepared=replica.authority->prepare(before_bad,binding);check(replica.authority->commit(std::move(prepared)),"Replay reset failed");
    auto phase=host.authority->capture_execution_phase([&]{host.step({});host.world->advance_effect_frame(1.f/60.f);});
    check(!phase.inputs.empty(),"Bad replay fixture has no source input");auto changed=phase;changed.inputs.front().operand^=1;
    // Use a structurally valid but wrong estimate operand when the first read
    // has no operand; the real call sequence must reject it before use.
    changed.inputs.front().kind=K::ReciprocalSquareRoot;changed.inputs.front().value=0;
    rejects([&]{replica.authority->replay_execution_phase(changed,[&]{replica.step({});});});
    rejects([&]{replica.authority->checkpoint(binding);});
    prepared=replica.authority->prepare(host.authority->checkpoint(binding),binding);
    check(replica.authority->commit(std::move(prepared)),"Bad replay could not repair through complete import");same(host,replica);
    const auto bank_before=replica.world->memory().checkpoint();
    const auto preflight_cut=host.authority->checkpoint(binding);
    // The adversarial fake cycle provider advances on each read. Observe both
    // once; a rejected commit must leave the replica's existing domain intact.
    (void)replica.authority->checkpoint(binding);
    {auto p=replica.authority->prepare(preflight_cut,binding);unavailable=true;
        check(!replica.authority->commit(std::move(p)),"Unavailable result clock published the source bank");unavailable=false;}
    check(replica.world->memory().checkpoint()==bank_before,"Failed result-clock preflight changed the live bank");same(host,replica);
    phase=host.authority->capture_execution_phase([&]{host.step({});host.world->advance_effect_frame(1.f/60.f);});
    changed=phase;changed.inputs.push_back({K::Milliseconds,0,0});++changed.calls.back().input_end;
    rejects([&]{replica.authority->replay_execution_phase(changed,[&]{replica.step({});replica.world->advance_effect_frame(1.f/60.f);});});
    rejects([&]{replica.authority->checkpoint(binding);});
    prepared=replica.authority->prepare(host.authority->checkpoint(binding),binding);
    check(replica.authority->commit(std::move(prepared)),"Unconsumed replay could not repair through complete import");same(host,replica);
    phase=host.authority->capture_execution_phase([&]{host.step({});host.world->advance_effect_frame(1.f/60.f);});
    unavailable=true;
    rejects([&]{replica.authority->replay_execution_phase(phase,[&]{replica.step({});replica.world->advance_effect_frame(1.f/60.f);});});
    unavailable=false;rejects([&]{replica.authority->checkpoint(binding);});
    prepared=replica.authority->prepare(host.authority->checkpoint(binding),binding);
    check(replica.authority->commit(std::move(prepared)),"Unavailable phase clock could not repair through complete import");same(host,replica);
    std::atomic<bool> denied=false;
    {std::jthread thread([&]{try{replica.authority->replay_execution_phase({},[]{});}catch(const FormatError &){denied=true;}});}
    check(denied,"Wrong executor replay was admitted");same(host,replica);
    phase=host.authority->capture_execution_phase([&]{
        rejects([&]{host.authority->checkpoint(binding);});
        rejects([&]{host.authority->prepare(cut,binding);});
    });
    check(phase.inputs.empty(),"Active phase reached an unrecorded checkpoint clock read");
    replica.authority->replay_execution_phase(phase,[]{});same(host,replica);
    SourceExecutionInputs bound;
    {SourceExecutionInputBinding binding(host.world->memory(),bound);
        rejects([&]{host.world->tu15_world().execution_clocks();});
        rejects([&]{host.world->tu15_world().anchor_execution_clocks({});});
        rejects([&]{host.world->tu15_world().authority_checkpoint();});}
    bound.finish();same(host,replica);
    const SourceExecutionClocks wrap{UINT32_MAX-15u,UINT64_MAX-500u,UINT64_MAX-10000u};
    for(auto *owner:{&host,&replica}){
        owner->world->tu15_world().anchor_execution_clocks(wrap);owner->clock->time+=32;
        owner->clock->counter.fetch_add(1500);owner->clock->filetime+=20000;
    }
    const auto ha=host.world->tu15_world().execution_clocks(),ra=replica.world->tu15_world().execution_clocks();
    check(ha==ra&&ha.milliseconds==16&&ha.cycles==1999&&ha.utc_filetime==9999,"Native clock anchoring lost unsigned wrap semantics");
    same(host,replica);
    std::cout<<"PASS actual TU1.5 eight-player replay with "<<readings<<" ordered inputs, all external clock anchors, forty untaped future ticks, complete native banks/PCR rows, overflow survival, commit preflight and mismatch/leftover qualification\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
