#include "source_match_rebind_fixture.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_codec.hpp"
#include "halo2/source_live_model_draws_tu15.hpp"
#include "halo2/source_live_shadow_selection_tu15.hpp"
#include "halo2/source_live_shadow_main_inputs_tu15.hpp"
#include "halo2/source_camera.hpp"
#include <cstring>
#include <cmath>
#include <set>

namespace {
void equal(Owner &a,Owner &b){check(encode_source_authority(a.authority->checkpoint(binding))==
    encode_source_authority(b.authority->checkpoint(binding)),"Caller ABI replay changed the complete coupled world");}
void restore(Owner &a,Owner &b){auto p=b.authority->prepare(a.authority->checkpoint(binding),binding);
    check(b.authority->commit(std::move(p)),"Caller ABI replay repair failed");equal(a,b);}
void live_fp(SourcePhysicsWorldRuntimeAbi &abi){
    // Declared caller operands, including precision unavailable in binary64.
    abi.gp[3]=0x15abcdef;abi.eflags=0x246;
    for(unsigned n=0;n<8;++n)abi.xmm[n]={0x7fc00001u+n,0x80000000u,1u+n,0x3f800000u};
    abi.x87_depth=3;abi.x87_status=0x1000;abi.x87[0]=1.0000000000000000001L;abi.x87[1]=-0.L;abi.x87[2]=0x1p-16382L;
}
}
int main(int argc,char **argv)try {
    if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    const std::array<unsigned,4> locals{0,1,2,3};
    Owner host(argv[1],argv[2],locals,1000),replica(argv[1],argv[2],locals,900000);restore(host,replica);
    using K=SourceExecutionCallKind;std::set<K> kinds;
    for(unsigned frame=0;frame<2;++frame){
        auto phase=host.authority->capture_execution_phase([&]{
            host.step({});auto &world=host.world->tu15_world();
            std::vector<std::uint32_t> args(160,0x15face00u+frame);auto abi=world.prepare_source_call(args);live_fp(abi);
            world.run(0x9d8e0u,abi);check(abi.gp[0]==8,"Original TU1.5 constant getter did not execute");
            check(abi.x87_depth==3&&abi.x87[0]==1.0000000000000000001L&&std::signbit(abi.x87[1]),"Original getter lost live x87 inputs");
            check(world.call(0x9d900u)==4,"Original canonical call did not execute");
            abi=world.prepare_source_call();live_fp(abi);world.run_actual(0x335b0u,abi);
            (void)world.call_actual(0x226b0u);
            abi=world.prepare_source_call();abi.active_mmx.emplace();abi.active_mmx->known_mask=0x81;
            for(unsigned n:{0u,7u})for(unsigned b=0;b<10;++b)abi.active_mmx->physical[n][b]=std::byte(n*10+b);
            world.run(0x9d8e0u,abi);check(abi.active_mmx&&abi.active_mmx->known_mask==0x81,"Original getter lost active MMX aliases");
            const std::array<std::uint32_t,3> scratch{0x15f00d00u+frame,0x80000000u,1};
            world.write_source_call_input(abi.gp[4]-0x280u,std::as_bytes(std::span(scratch)));
            // Completed ordinary main views with declared empty admission
            // queues: exercise the real camera/queue caller and333F0 gates.
            world.begin_model_frame();
            for(unsigned view=0;view<4;++view){
                const auto eye=host.match->unit(view).body.position+Vec3{0,0,1.7f};
                SourceLiveModelDrawContextTu15 context;context.view=view;context.local_players=4;
                context.entry.eye=eye;context.entry.distance_scale=1;context.forward={1,0,0};
                context.draw.eye=eye;context.draw.viewport={0,0,360,640};
                SourceFrustumProjectionInput projection;projection.projection_angle=1.2217304706573486f;
                projection.outer=projection.safe=context.draw.viewport;
                context.draw.rendering=source_view_projection({eye,context.forward,{0,0,1},.015625f,1024.f,{},source_frustum_projection(projection).projection_program});
                source_live_shadow_main_user_tu15(world,std::int32_t(view));
                source_live_shadow_main_inputs_tu15(world,context);
                source_live_shadow_main_queues_tu15(world,SourceBspAdmission{},SourceObjectAdmission{});
                const auto selection=source_live_shadow_selection_tu15(world,SourceBspAdmission{},SourceObjectAdmission{},context,std::int32_t(view));
                check(selection.selected.empty(),"Empty admitted source queues selected synthetic shadows");
                SourceShadowCandidate candidate;candidate.owner=std::int32_t(host.match->unit(view).source_datum);candidate.requested_fade=1;
                const auto roots=source_live_shadow_root_frames_tu15(world,std::span(&candidate,1));
                check(roots.size()==1&&roots.front().owner==host.match->unit(view).source_datum&&roots.front().route==SourceLiveShadowRootRouteTu15::disabled,
                    "Actual four-player shadow-root eligibility lost its original disabled-shadow route");
            }
        });
        for(const auto &call:phase.calls)kinds.insert(call.kind);
        replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    }
    for(auto kind:{K::WriteSourceCallInput,K::PrepareSourceCall,K::RunSourceCall,K::RunActualSourceCall,K::CallSource,K::CallActualSource})
        check(kinds.contains(kind),"An actual ABI dispatch path was omitted");
    auto phase=host.authority->capture_execution_phase([&]{auto &world=host.world->tu15_world();auto abi=world.prepare_source_call();live_fp(abi);world.run(0x9d8e0u,abi);});
    auto changed=phase;changed.calls.back().result[0]^=std::byte{1};
    SourcePhysicsWorldRuntimeAbi actual{};
    rejects([&]{replica.authority->replay_execution_phase(changed,[&]{auto &world=replica.world->tu15_world();actual=world.prepare_source_call();live_fp(actual);world.run(0x9d8e0u,actual);});});
    check(actual.gp[0]==8,"Result verification supplied a stored return instead of executing the original entry");
    rejects([&]{replica.authority->checkpoint(binding);});restore(host,replica);
    phase=host.authority->capture_execution_phase([&]{auto &world=host.world->tu15_world();auto abi=world.prepare_source_call();live_fp(abi);world.run_actual(0x335b0u,abi);});
    rejects([&]{replica.authority->replay_execution_phase(phase,[&]{auto &world=replica.world->tu15_world();auto abi=world.prepare_source_call();live_fp(abi);abi.xmm[3][0]^=1;world.run_actual(0x335b0u,abi);});});restore(host,replica);
    phase=host.authority->capture_execution_phase([&]{host.world->tu15_world().write_source_call_input(0x4c66f4u,std::as_bytes(std::span(locals).first(1)));});
    const auto original_view=word(replica.world->tu15_world().memory(),0x4c66f4u);
    rejects([&]{replica.authority->replay_execution_phase(phase,[&]{const std::uint32_t wrong=2;replica.world->tu15_world().write_source_call_input(0x4c66f4u,std::as_bytes(std::span(&wrong,1)));});});
    check(word(replica.world->tu15_world().memory(),0x4c66f4u)==original_view,"Changed caller input mutated the native bank before rejection");restore(host,replica);
    auto malformed=phase;malformed.calls.front().arguments.push_back(std::byte{});
    rejects([&]{replica.authority->replay_execution_phase(malformed);});equal(host,replica);
    rejects([&]{host.authority->capture_execution_phase([&]{const std::uint32_t value=3;host.world->memory().write(0x4c66f4u,std::as_bytes(std::span(&value,1)));});});
    check(word(host.world->tu15_world().memory(),0x4c66f4u)==3,"Recording failure stopped the actual host input write");
    // The real host keeps running after a recording failure. Repair the
    // declared input through a complete cut before the next recorded tick.
    restore(host,replica);phase=host.authority->capture_execution_phase([&]{host.step({});});replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    std::cout<<"PASS actual native caller scratch/full ABI/results: original canonical and TU1.5 entries, live x87/XMM/MMX, four-view empty-queue selection and real-unit disabled shadow roots, exact complete worlds, rejection and repair\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
