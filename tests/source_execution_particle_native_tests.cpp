#include "source_match_rebind_fixture.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_codec.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/bsp_atmosphere.hpp"
namespace {
struct ParticleOwner {
 std::shared_ptr<Clock> clock=std::make_shared<Clock>();MultiplayerPreparedWorld world;SourceMatchAuthority authority;SourceWorldParticleDrawsTu15 graphics;
 ParticleOwner(const MultiplayerOptions &options,std::uint32_t time):world(options,std::nullopt,clock),authority(world.match,*world.source_objects),graphics(*world.cache,world.content.objects){clock->time=time;world.bind_source_local_views(std::array{0u});}
};
void equal(ParticleOwner &a,ParticleOwner &b){check(encode_source_authority(a.authority.checkpoint(binding))==encode_source_authority(b.authority.checkpoint(binding)),"Native particle replay changed the complete coupled world");}
void restore(ParticleOwner &a,ParticleOwner &b){auto prepared=b.authority.prepare(a.authority.checkpoint(binding),binding);check(b.authority.commit(std::move(prepared)),"Native particle cut could not commit");equal(a,b);}
}
int main(int argc,char **argv)try {
 if(argc!=4||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
 MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];options.shared_maps=argv[3];options.local_players=1;options.starting_weapon=5;options.mute=true;
 ParticleOwner host(options,1000),replica(options,900000);check(host.graphics.active()&&replica.graphics.active(),"Actual Backwash swarm resources absent");restore(host,replica);
 const Vec3 eye{0,11.4f,-20.85f};const auto angle=source_camera_angles(1.2217304763960306f);SourceFrustumProjectionInput pi;pi.projection_angle=angle.projection_angle;pi.outer=pi.safe={0,0,480,640};pi.scenario_present=true;pi.scenario_type=host.world.cache->load_source_geometry_scenario_type();const auto projection=source_frustum_projection(pi);
 const auto camera=source_frustum_camera({eye,{1,0,0},{0,0,1},.015625f,1024.f,projection.clipping_rectangle()});SourcePortalMaskState mask;mask.initialize(host.world.cache->load_source_portal_mask_definition(0),{});
 const auto visibility=source_portal_visibility(host.world.cache->load_source_portal_bsp(0),std::span{&camera,1},host.world.cache->load_bsp_atmosphere(0).cluster_at(eye),mask.words);
 MuzzleEffectView view;view.position=eye;view.forward={1,0,0};view.up={0,0,1};view.width=640;view.height=480;view.orientation={1,0,0,0,1,0,0,0,1};
 unsigned peak{},last{},calls{};
 for(unsigned batch=0;batch<10;++batch){auto phase=host.authority.capture_execution_phase([&]{
  auto &source=host.world.source_objects->tu15_world();host.graphics.prepare_storage(source);
  for(unsigned tick=0;tick<24;++tick){host.graphics.publish_observer(source,0,eye,{1,0,0},{0,0,1},1.2217304763960306f);host.clock->time+=33;host.clock->filetime+=330000;host.world.match.step({});host.graphics.advance(source,1.f/30.f);source.begin_model_frame();
   const auto draws=host.graphics.draw(source,visibility,view,0,pi.projection_angle,angle.lighting_distance_scale);last=unsigned(draws.size());peak=std::max(peak,last);
  }
 });calls+=unsigned(phase.calls.size());replica.authority.replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);}
 check(peak&&last,"Actual source swarm did not emit visible particles");check(word(host.world.source_objects->memory(),0x4c6854u)==240,"Native source frame counter lost a phase");
 const auto expected_seconds=std::uint32_t(std::int64_t(host.clock->filetime/10'000'000ull)-11'644'473'600ll);
 check(host.authority.checkpoint(binding).match.logical_result_seconds==expected_seconds,"Prepared-world result time escaped its explicitly supplied UTC clock");
 std::cout<<"PASS actual Backwash particle journal:240 match/render frames, "<<calls<<" original calls, "<<peak<<" peak / "<<last<<" final particles, original observer/queue/RNG/time state and complete coupled archives\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
