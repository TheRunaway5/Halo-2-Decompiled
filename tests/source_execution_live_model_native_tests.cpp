#include "source_match_rebind_fixture.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/source_execution_model_inputs.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_codec.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_geometry_lod.hpp"
#include <cstring>

namespace {
void equal(Owner &a,Owner &b){check(encode_source_authority(a.authority->checkpoint(binding))==encode_source_authority(b.authority->checkpoint(binding)),
    "Live model replay changed the complete canonical coupled world");}
void restore(Owner &a,Owner &b){auto p=b.authority->prepare(a.authority->checkpoint(binding),binding);check(b.authority->commit(std::move(p)),"Live-model repair failed");equal(a,b);}
struct View {
    SourceLiveModelDrawContextTu15 context;SourceObjectAdmission admission;SourceDrawUnitState units;
};
View view(Owner &owner,unsigned local){
    View result;auto &c=result.context;c.view=local;c.local_players=4;
    const auto position=owner.match->unit(local).body.position;
    c.entry.eye=position+Vec3{2,-2,1.7f};c.forward=normalized(position-c.entry.eye);c.entry.distance_scale=1;
    c.entry.coverage_multiplier=source_geometry_lod_policy(4).coverage_multiplier;c.draw.eye=c.entry.eye;c.draw.viewport={0,0,360,640};
    SourceFrustumProjectionInput projection;projection.projection_angle=1.2217304706573486f;projection.outer=projection.safe=c.draw.viewport;
    const auto projected=source_frustum_projection(projection);
    c.draw.rendering=source_view_projection({c.entry.eye,c.forward,{0,0,1},.015625f,1024.f,{},projected.projection_program});
    c.entry.coverage=source_shader_coverage_camera(c.draw.rendering);c.draw.draw.mask_arena_address=0x71001000u;
    const auto camera=source_frustum_camera({c.entry.eye,c.forward,{0,0,1},.015625f,1024.f,projected.clipping_rectangle()});
    SourcePortalVisibilitySet visible;visible.groups.push_back(camera);visible.frustums.push_back(camera.full_frustum);
    const auto count=owner.cache.load_source_bsp_admission(0).clusters.size();std::vector<std::uint8_t> fog(count);
    // Declared all-cluster camera input; admission and collection still use
    // the actual original spatial/object/model/resource owners.
    for(unsigned n=0;n<count;++n){SourceClusterFrustumRanges r;r.source_cluster=std::int16_t(n);r.counts[0]=1;visible.clusters.push_back(r);}
    SourceObjectAdmissionInput input;input.containing_fog=fog;
    result.admission=owner.world->tu15_world().admit_render_objects(visible,input);
    check(!result.admission.objects.empty(),"Real source admission produced no live model callers");
    result.units.units.back().words[0]=0x15facade;result.units.arena.words.back()=0x15beef;
    return result;
}
}
int main(int argc,char **argv)try {
    if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    const std::array<unsigned,4> locals{0,1,2,3};Owner host(argv[1],argv[2],locals,1000),replica(argv[1],argv[2],locals,900000);restore(host,replica);
    unsigned observed{},drawn{},root_calls{};
    for(unsigned frame=0;frame<2;++frame){
        auto phase=host.authority->capture_execution_phase([&]{host.step({});host.world->begin_model_frame();
            for(unsigned local=0;local<4;++local){auto v=view(host,local);v.context.built=[&](const auto &collector){++observed;
                    check(Bytes(collector.bytes).u32(0)!=null_datum,"Original collector lost its owner");(void)host.world->tu15_world().rendered_world_nodes(Bytes(collector.bytes).u32(0));};
                source_live_model_draws_tu15(host.world->tu15_world(),v.admission,v.units,v.context);drawn+=v.units.draw_count;
                const auto prefix=v.units.main_prefix;v.context.entry.mode=v.context.draw.draw.mode=1;source_draw_units_begin(v.units,1);
                source_live_model_draws_tu15(host.world->tu15_world(),v.admission,v.units,v.context);
                check(v.units.main_prefix==prefix&&v.units.draw_count>=prefix,"Secondary model collection lost its actual main-view prefix");
                drawn+=v.units.draw_count-prefix;}
        });
        for(const auto &call:phase.calls)if(call.kind==SourceExecutionCallKind::LiveModelDraws){++root_calls;check(call.result.size()==32,"Live draw result was not sealed");}
        replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    }
    check(observed&&drawn&&root_calls==16,"Native four-view live collection omitted original main/secondary builders/draws");
    auto v=view(host,0);auto phase=host.authority->capture_execution_phase([&]{source_live_model_draws_tu15(host.world->tu15_world(),v.admission,v.units,v.context);});
    auto changed=phase;changed.calls.front().result[0]^=std::byte{1};
    rejects([&]{replica.authority->replay_execution_phase(changed);});rejects([&]{replica.authority->checkpoint(binding);});restore(host,replica);
    auto malformed=phase;malformed.calls.front().arguments.push_back(std::byte{});rejects([&]{replica.authority->replay_execution_phase(malformed);});equal(host,replica);
    v=view(host,0);const auto observer_input=v;
    phase=host.authority->capture_execution_phase([&]{source_live_model_draws_tu15(host.world->tu15_world(),v.admission,v.units,v.context);});
    auto bad=observer_input;bad.context.built=[&](const auto&){auto &world=replica.world->tu15_world();const auto bytes=world.memory().view(0x4c66f4u,4);world.write_source_call_input(0x4c66f4u,bytes);};
    rejects([&]{replica.authority->replay_execution_phase(phase,[&]{source_live_model_draws_tu15(replica.world->tu15_world(),bad.admission,bad.units,bad.context);});});
    rejects([&]{replica.authority->checkpoint(binding);});restore(host,replica);
    v=view(host,0);unsigned continued{};v.context.built=[&](const auto&){++continued;auto &world=host.world->tu15_world();const auto bytes=world.memory().view(0x4c66f4u,4);world.write_source_call_input(0x4c66f4u,bytes);};
    rejects([&]{host.authority->capture_execution_phase([&]{source_live_model_draws_tu15(host.world->tu15_world(),v.admission,v.units,v.context);});});
    check(continued>0,"Observer recording failure stopped actual host collection");restore(host,replica);
    phase=host.authority->capture_execution_phase([&]{host.step({});});replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    std::cout<<"PASS actual live model journal: sixteen real-player four-view main/secondary collections, "<<observed<<" collectors, "<<drawn
        <<" draws, complete coupled worlds, actual result digests, observer isolation, rejection and repair\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
