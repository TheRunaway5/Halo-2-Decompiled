#include "source_match_rebind_fixture.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_codec.hpp"
#include "halo2/source_bsp_part_commands.hpp"
#include "halo2/source_live_model_draws_tu15.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_geometry_lod.hpp"
#include <cstring>
namespace {
void equal(Owner &a,Owner &b){check(encode_source_authority(a.authority->checkpoint(binding))==encode_source_authority(b.authority->checkpoint(binding)),"Render command replay changed the complete coupled world");}
void restore(Owner &a,Owner &b){auto p=b.authority->prepare(a.authority->checkpoint(binding),binding);check(b.authority->commit(std::move(p)),"Render command repair failed");equal(a,b);}
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
SourceBspPartCommandState commands(PhysicsLiveWorldTu15 &world){
 SourceBspPartCommandState state;const auto &memory=world.memory();
 for(unsigned n=0;n<3;++n){auto &bank=state.banks[n];memory.read(0x4ce268u+n*24u,std::as_writable_bytes(std::span(bank.header)));bank.storage.resize(bank.header[0]);memory.read(bank.header[5],std::as_writable_bytes(std::span(bank.storage)));
  if(n)bank.storage.back().words[5]=(bank.storage.back().words[5]&0xffffu)|0x55550000u;}
 state.calls.resize(129,0x41c70u);return state;
}
std::uint32_t geometry_header(PhysicsLiveWorldTu15 &world,const SourceObjectDrawCollector &collector){
 const auto datum=Bytes(collector.bytes).u32(0x5c);const auto &memory=world.memory();const auto table=word(memory,0x4f0414u),model=word(memory,table+(datum&65535u)*16u+8u);
 const auto count=word(memory,model+0x24u),sections=word(memory,model+0x28u);
 for(unsigned n=0;n<count;++n){const auto header=sections+n*0x5cu+0x38u;if(std::bit_cast<std::int32_t>(word(memory,header+8u))>0)return header;}
 throw FormatError("Actual produced render model has no resource header");
}
}
int main(int argc,char **argv)try {
 if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
 const std::array<unsigned,4> locals{0,1,2,3};Owner host(argv[1],argv[2],locals,1000),replica(argv[1],argv[2],locals,900000);restore(host,replica);
 unsigned attachments{},requests{};
 for(unsigned frame=0;frame<2;++frame){
  auto phase=host.authority->capture_execution_phase([&]{host.step({});host.world->begin_model_frame();auto &world=host.world->tu15_world();
   for(unsigned local=0;local<4;++local){const auto collector=world.build_world_model(host.match->unit(local).source_datum,local,4,false,10,1);const auto header=geometry_header(world,collector);
    check(world.request_render_geometry(2,header),"Actual resident geometry request failed");++requests;
    const auto v=view(host,local);for(unsigned mode:{1u,2u}){
     // The ordinary bank0 is already authoritative. Publish its retained
     // caller bytes into the real bank; secondary traversal must not replace
     // them with a typed callback copy. Banks1/2 remain secondary caller input.
     const auto capacity=word(world.memory(),0x4ce268u),data=word(world.memory(),0x4ce27cu);
     const std::array<std::byte,2> marker{std::byte{0x55},std::byte{0x55}};
     world.write_source_call_input(data+capacity*24u-2u,marker);
     auto state=commands(world);state.banks[0].storage.back().words[5]^=0x11110000u;
     world.append_render_attachments(state,v.admission,mode);++attachments;
     for(const auto &bank:state.banks)check((bank.storage.back().words[5]&0xffff0000u)==0x55550000u,"Original attachment projection lost retained command bytes");}
   }
  });replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
 }
 auto &world=host.world->tu15_world();auto v=view(host,0);auto state=commands(world);
 auto phase=host.authority->capture_execution_phase([&]{world.append_render_attachments(state,v.admission,1);});auto changed=phase;changed.calls.front().result[0]^=std::byte{1};
 rejects([&]{replica.authority->replay_execution_phase(changed);});rejects([&]{replica.authority->checkpoint(binding);});restore(host,replica);
 auto malformed=phase;malformed.calls.front().arguments.push_back(std::byte{});rejects([&]{replica.authority->replay_execution_phase(malformed);});equal(host,replica);
 const auto collector=world.build_world_model(host.match->unit(0).source_datum,0,4,false,10,1);restore(host,replica);const auto header=geometry_header(world,collector);
 phase=host.authority->capture_execution_phase([&]{check(world.request_render_geometry(2,header),"Original geometry result missing");});changed=phase;changed.calls.front().result=source_execution_result(0u);
 rejects([&]{replica.authority->replay_execution_phase(changed);});restore(host,replica);
 std::cout<<"PASS actual render commands: "<<attachments<<" native light/shadow attachment traversals, "<<requests<<" resident resource requests, retained bytes, complete coupled worlds, real outputs, rejection and repair\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
