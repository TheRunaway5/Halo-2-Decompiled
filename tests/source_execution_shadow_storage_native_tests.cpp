#include "source_match_rebind_fixture.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_codec.hpp"
#include "halo2/source_live_shadow_storage_tu15.hpp"

namespace {
void equal(Owner &a,Owner &b){check(encode_source_authority(a.authority->checkpoint(binding))==
    encode_source_authority(b.authority->checkpoint(binding)),"Shadow-storage replay changed the complete coupled world");}
void restore(Owner &a,Owner &b){
    auto &world=b.world->tu15_world();const auto revision=world.authority_backing_revision();
    const auto saved=a.authority->checkpoint(binding);
    {auto cancelled=b.authority->prepare(saved,binding);
        check(world.authority_backing_revision()==revision,"Unpublished preparation changed the backing revision");}
    check(world.authority_backing_revision()==revision,"Cancelled preparation changed the backing revision");
    auto prepared=b.authority->prepare(saved,binding);
    check(b.authority->commit(std::move(prepared)),"Shadow-storage replay repair failed");
    check(world.authority_backing_revision()==revision+1,"Successful native bank publication did not retire renderer bindings");
    equal(a,b);
}
}
int main(int argc,char **argv)try {
    if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    const std::array<unsigned,4> locals{0,1,2,3};
    Owner host(argv[1],argv[2],locals,1000),replica(argv[1],argv[2],locals,900000);
    // Freeze the explicitly injected profiling clock for the read-only
    // invariant. Normal fixture clocks advance on every sampling operation,
    // including a checkpoint; no archive fields are normalized for comparison.
    host.clock->cycle_stride=replica.clock->cycle_stride=0;restore(host,replica);
    SourceLiveShadowStorageTu15 retained;
    for(unsigned activation=0;activation<2;++activation){
        const auto revision=host.world->tu15_world().authority_backing_revision();
        const auto phase=host.authority->capture_execution_phase([&]{
            retained=source_live_shadow_storage_activate_tu15(host.world->tu15_world());
            source_live_shadow_storage_validate_tu15(host.world->tu15_world(),retained);
        });
        check(host.world->tu15_world().authority_backing_revision()==revision,
            "Native shadow activation changed its independent backing-publication identity");
        check(!phase.calls.empty(),"Actual shadow activation lost its source operations");
        const auto encoded=encode_source_execution_phase(phase);
        replica.authority->replay_execution_phase(decode_source_execution_phase(encoded));equal(host,replica);
        const auto rebound=source_live_shadow_storage_rebind_tu15(replica.world->tu15_world(),retained);
        source_live_shadow_storage_validate_tu15(replica.world->tu15_world(),rebound);
        check(rebound.bsp==retained.bsp&&rebound.hash==retained.hash&&
            rebound.hash_native!=retained.hash_native,"Shadow activation did not retain source identities with independent native backing");
        const auto before=encode_source_authority(host.authority->checkpoint(binding));
        const auto boundary=host.authority->execution_boundary();
        const auto query=host.authority->capture_execution_phase([&]{
            source_live_shadow_storage_validate_tu15(host.world->tu15_world(),retained);
        });
        check(query.calls.empty()&&query.inputs.empty(),"Read-only shadow admission emitted a mutating source operation");
        check(boundary==host.authority->execution_boundary(),"Permission-only admission advanced the replay boundary mutation stamps");
        check(before==encode_source_authority(host.authority->checkpoint(binding)),"Read-only shadow admission mutated its coupled world");
        replica.authority->replay_execution_phase(query,[&]{
            source_live_shadow_storage_validate_tu15(replica.world->tu15_world(),rebound);
        });equal(host,replica);
    }
    const auto phase=host.authority->capture_execution_phase([]{});
    rejects([&]{replica.authority->replay_execution_phase(phase,[&]{
        (void)replica.world->memory().mutable_view(retained.hash,4);
    });});rejects([&]{replica.authority->checkpoint(binding);});restore(host,replica);
    rejects([&]{host.authority->capture_execution_phase([&]{
        const std::array<std::byte,4> changed{std::byte{0x15}};host.world->memory().write(retained.hash,changed);
    });});restore(host,replica);
    const auto tick=host.authority->capture_execution_phase([&]{host.step({});});
    replica.authority->replay_execution_phase(tick);equal(host,replica);
    std::cout<<"PASS actual TU1.5 shadow activation/reset/insertion, validation-only admission, independent native rebind, exact coupled replay and guarded write rejection/repair\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
