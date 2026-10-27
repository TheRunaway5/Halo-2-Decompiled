#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_decal_draw.hpp"
#include "helpers/source_decal_frame_checks.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;
namespace {void check(bool value,const char* message){if(!value)throw FormatError(message);}}
int main(int argc,char**argv)try {
 check(argc==3,"Supply prepared MCC Zanzibar MAP and bounded REPORT");if(!std::filesystem::is_regular_file(argv[1]))return 77;
#if defined(_WIN32)
 check(_putenv_s("H2_NET_PROFILE","original")==0,"Cannot declare original network budget");
#else
 check(setenv("H2_NET_PROFILE","original",1)==0,"Cannot declare original network budget");
#endif
 MultiplayerOptions options;options.map=argv[1];options.shared_maps=options.map.parent_path();options.starting_weapon=5;
 options.mute=true;options.session_random_seed=0x12345678u;
 MultiplayerPreparedWorld world(options);check(world.cache->is_mcc(),"Runtime admission requires prepared MCC data");
 check(world.source_objects && world.source_objects->owns_physics(),"Actual source authority absent");
 world.bind_source_local_views(world.local_slots);
 SourceDecalScene scene(*world.cache);check(scene.active(),"Actual source cache consumer unavailable");
 LanHost host(world.match,0,0x444543414c574952ull);LanClient peer(host.join_address(),0x444543414c574952ull,1);
 std::array<Controls,multiplayer_capacity> controls{};const std::array<Controls,1> neutral{};
 for(unsigned n=0;n<128 && !peer.ready();++n) {
  peer.send(neutral);host.receive(controls);host.broadcast();peer.receive();
  if(!peer.ready())std::this_thread::sleep_for(std::chrono::milliseconds(1));
 }
 check(peer.ready() && peer.error().empty(),"Actual UDP peer did not join source world");
 const auto began=std::chrono::steady_clock::now();std::size_t peak_records{},peak_pages{};
 for(unsigned n=0;n<60;++n) {
  peer.send(neutral);host.receive(controls);const auto before=world.match.simulation().ticks();world.match.step(controls);
  world.source_objects->advance_effect_frame(world.match.simulation().ticks()==before+1?tick_seconds:0.f);
  const auto expected=world.match.state();host.broadcast();peer.receive();
  check(expected.source_decals && peer.state().source_decals,"Actual source cache ownership absent from complete UDP state");
  check(test::same_source_decal_frame(expected.source_decals,peer.state().source_decals),"Actual UDP cache changed completed records, shared pages or head banks");
  peak_records=std::max(peak_records,expected.source_decals->records.size());std::size_t pages{};
  for(const auto& allocation:expected.source_decals->allocations)pages+=allocation.quads.size();peak_pages=std::max(peak_pages,pages);
  std::this_thread::sleep_until(began+std::chrono::nanoseconds(std::uint64_t(n+1)*1000000000u/30u));
 }
 check(peak_records>0 && peak_pages>0,"Actual source authored decal cache remained empty; test has no positive presentation data");
 SourceBspAdmission all;for(unsigned cluster=0;cluster<512;++cluster)all.clusters.push_back({cluster,0,0xffff,0});
 std::array<ShaderVector,192> constants{};
 const auto reference=scene.draw(world.source_objects->memory(),all,constants,{0,0,640,480},{0,16777215,16,1048576});
 const auto received=scene.draw(*peer.state().source_decals,all,constants,{0,0,640,480},{0,16777215,16,1048576});
 check(!reference.empty() && reference.size()==received.size(),"Actual received cache lost completed source draw plan");
 std::size_t quads{};
 for(unsigned n=0;n<reference.size();++n) {
  const auto& a=reference[n];const auto& b=received[n];
  check(a.datum==b.datum && a.primary==b.primary && a.cache_datum==b.cache_datum && a.layer==b.layer &&
        a.texture==b.texture && a.color==b.color && a.quads.size()==b.quads.size() &&
        a.state.pixel.words==b.state.pixel.words && a.state.source_blend==b.state.source_blend &&
        a.state.destination_blend==b.state.destination_blend,"Received source draw identity, texture, tint or layer differs");
  quads+=b.quads.size();
 }
 const auto stats=host.snapshot_diagnostics();
 nlohmann::json report={{"passed",true},{"map",std::filesystem::absolute(options.map).string()},
  {"peak_source_records",peak_records},{"peak_unique_source_pages",peak_pages},{"received_draws",received.size()},
  {"received_draw_quads",quads},{"completed_source_ticks",60},{"caller_cadence_hz",30},
  {"wire_bytes",stats.stream_wire_bytes},{"wire_messages",stats.stream_messages},{"wire_packets",stats.stream_packets},
  {"ending_channel_budget_bits_per_second",stats.stream_budget_bps},{"over_budget",stats.stream_over_budget},
  {"scope","Actual ordinary MCC authority and real authenticated UDP peer with original channel profile. Complete nonempty static authored cache readback matches every completed native tick and produces identical CPU draw identities/resources/tints/layers. Declared all-cluster/zero-camera control isolates consumer data; no client physics world, dynamic blood, ordinary PVS/GPU pixels, dynamic late-join history or Xbox network-protocol parity is claimed."}};
 std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
