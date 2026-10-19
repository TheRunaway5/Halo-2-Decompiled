#define SDL_MAIN_HANDLED
#include "halo2/math.hpp"
#include "halo2/texture.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/source_portal_visibility.hpp"
#include <SDL.h>
#include <iostream>
#include <set>
#include <utility>
#include <chrono>
#if defined(__GLIBC__)
#include <malloc.h>
#endif
using namespace halo2;
namespace {
void check(bool b, const char *m) {
  if (!b)
    throw FormatError(m);
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 4)
      throw FormatError("Supply MAP SHARED_MAPS UPDATE_XBE");
    auto cache = std::make_shared<Cache>(argv[1], argv[2],
                                         OriginalData::load_xbe(argv[3]));
    const auto content = cache->load_content(0, true);
    UiVariant variant{"Slayer", 0, {}};
    variant.values.set(0x59, 5);
    auto rules = slayer_rules(variant);
    rules.weapon = cache->slayer_starting_weapon(rules.starting_weapon);
    MultiplayerMatch match(content, 0, cache->spawns(), rules);
    const auto player = match.join("Effects");
    match.configure_grenades(cache, *cache->original_data()->grenade_constants);
    SDL_SetMainReady();
    SdlSoundPlatformConfiguration platform{
        SdlSoundPlatformConfiguration::Delivery::Manual, 256, {}};
    unsigned audio_packets{}, explosion_packets{};
    MultiplayerAudio audio(*cache, 0x10293847, platform,
                           [&](const auto &packet) {
                             ++audio_packets;
                             if (packet.sound == 0xe67304feu)
                               ++explosion_packets;
                           });
    std::vector<WorldEffectSound> pending_sounds;
    Mesh mesh;
    std::uint32_t shared_visual=0x12345678;
    NativeWorldEffects effects(*cache, mesh, match.simulation().collision(),
                               SourceEffectRandom{shared_visual,shared_visual});
    Mesh batched_mesh;
    NativeWorldEffects batched(*cache, batched_mesh,
                               match.simulation().collision(), 0x12345678);
    std::vector<BulletImpact> pending_impacts;
    std::vector<NativeGrenadeDetonation> pending_detonations;
    MuzzleEffectView view{};
    view.forward = {1, 0, 0};
    view.up = {0, 0, 1};
    view.width = 640;
    view.height = 480;
    view.orientation = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    // Declared presentation boundary: an actual authored firing graph with
    // completed source locations must not require a native player/model owner.
    // The ordinary model-marker path supplies an independent equivalent bank.
    {
      const auto needler=std::find_if(content.weapons.begin(),content.weapons.end(),
          [](const auto &w){return w.second.name.ends_with("\\needler");});
      check(needler!=content.weapons.end(),"Missing original Needler firing definition");
      const auto firing=needler->second.firing_effects.at(0).effects[0];
      const auto definition=cache->load_muzzle_effect(firing);
      NativeWorldEffects::ObjectMarkers locations;
      const WorldEffectFrame frame{{1,2,3},{0,0,1},{1,0,0},{1,0,0},{0,0,1},false};
      locations[cache->string_id(definition.marker)]={frame};
      for(const auto &system:definition.systems)locations[cache->string_id(system.marker)]={frame};
      Mesh source_mesh,model_mesh;
      NativeWorldEffects source(*cache,source_mesh,match.simulation().collision(),0x10203040);
      NativeWorldEffects model(*cache,model_mesh,match.simulation().collision(),0x10203040);
      constexpr std::uint32_t weapon=0x12340001,instance=0x45670002;
      model.object_markers(weapon,locations);
      source.request_weapon(firing,weapon,1,{}, {},1,1,instance,false,&locations);
      model.request_weapon(firing,weapon,1,{}, {},1,1,instance,false);
      std::size_t peak{};
      for(unsigned tick=1;tick<20;++tick) {
        if(tick==4) {
          for(auto &[name,frames]:locations)for(auto &f:frames)f.position.x+=2;
          source.update_weapon_locations(weapon,instance,locations);
          model.object_markers(weapon,locations);
        }
        source.advance(tick,tick_seconds);model.advance(tick,tick_seconds);
        const auto a=source.draw(view,0,tick_seconds),b=model.draw(view,0,tick_seconds);
        peak=std::max(peak,a.size());
        check(a.size()==b.size(),"Source firing locations lost or duplicated authored systems");
        for(unsigned i=0;i<a.size();++i)check(a[i].vertex.attributes==b[i].vertex.attributes,
            "Moving source firing locations differ from equivalent authored model markers");
      }
      check(peak>0,"Unmapped source firing locations silently discarded every particle");
      source.advance(6000,tick_seconds);
      check(source.diagnostics().particles==0,"Unmapped source firing particles did not retire");
    }
    std::array<Controls, 16> controls{};
    controls[player].yaw = match.unit(player).yaw;
    controls[player].pitch = -1.2f;
    std::uint64_t serial{}, detonation{};
    std::size_t peak_particles{}, peak_decals{}, peak_systems{},peak_lights{};
    std::size_t death_sounds{};
    bool directional_sparks{},handoff_checked{};unsigned light_handoffs{};
    std::set<std::uint32_t> impact_spark_bitmaps;
    LanHost host(match, 0, 0x454646454354ull);
    LanClient peer("127.0.0.1:" + std::to_string(host.port()),
                   0x454646454354ull, 1);
    std::array<Controls, 16> received{};
    std::array<Controls, 1> outgoing{};
    for (unsigned n = 0; n < 30 && !peer.ready(); ++n) {
      peer.send(outgoing);
      host.receive(received);
      host.broadcast();
      peer.receive();
    }
    check(peer.ready(), "Effects LAN handshake failed");
    for (unsigned t = 0; t < 250; ++t) {
      controls[player].fire = t < 150;
      controls[player].grenade = t == 170;
      match.step(controls);
      const auto state = match.state();
      for (const auto &e : state.bullet_impacts)
        if (e.serial > serial) {
          serial = e.serial;
          effects.impact(e);
          pending_impacts.push_back(e);
        }
      for (const auto &e : state.grenade_detonations)
        if (e.serial > detonation) {
          detonation = e.serial;
          const WorldEffectFrame frame{e.position, e.up, e.forward,
                                       e.forward,  e.up, false};
          pending_detonations.push_back(e);
          Mesh isolated_mesh;
          std::array<std::uint32_t,2> isolated_restore_random{};
          NativeWorldEffects isolated(*cache, isolated_mesh,
                                      match.simulation().collision(),
                                      0x12345678);
          isolated.spawn(e.effect, e.projectile, e.tick, frame);
          isolated.advance(e.tick, tick_seconds);
          const auto lights=isolated.lights();
          check(lights.size()==1 && lights[0].definition==0xe63a04c5 && lights[0].owner==null_datum && lights[0].effect_owner==e.projectile && lights[0].tick==e.tick && lights[0].state.brightness==4,
                "Actual frag lost its authored light/owner/birth brightness");
          check(lights[0].registration.identity==0xe96c0000 && (lights[0].registration.flags&0x1b)==0x1b && lights[0].registration.spatial_radius==2,
                "Actual frag did not allocate/register its original standalone light");
          check(lights[0].registration.location.cluster<0 || lights[0].registration.reference_head!=null_datum,
                "Actual frag location lost its spatial membership links");
          if(lights[0].registration.location.cluster>=0) {
            const auto eye=lights[0].registration.location_center;
            const auto camera=source_frustum_camera({eye,{1,0,0},{0,0,1},.015625f,1024.f,{-1,1,-1,1}});
            std::array<std::uint32_t,16> open;open.fill(0xffffffffu);
            const auto visibility=source_portal_visibility(cache->load_source_portal_bsp(0),
                std::span{&camera,1},lights[0].registration.location.cluster,open);
            const auto frustums=visibility.sphere_bank();
            SourceEffectLightCollectionContext collection{};collection.visibility.frustums=&frustums;
            collection.containing_cluster=lights[0].registration.location.cluster;
            SourceEffectLightCandidateContext candidates{};candidates.admission={1,0,0,2,{}};
            candidates.depth_plane={1,0,0,-eye.x};candidates.pixel_scale=240;
            const auto geometry=cache->load_source_bsp_admission(0);
            const auto fog=source_bsp_fog_admission(cache->load_bsp_atmosphere(0),{true,true,null_datum,0});
            SourceBspAdmissionInput main_input;main_input.containing_fog=fog;
            const auto main=source_bsp_admission(geometry,visibility,main_input);
            for(unsigned local=0;local<4;++local) {
              collection.local_player=local;
              const auto selected=isolated.light_view(eye,collection,candidates,1);
              check(selected.epoch==local*2+1 && selected.visible.size()==1 && selected.selected.size()==1 &&
                    selected.selected[0].identity==lights[0].registration.identity && selected.records[0].fades.overall==1,
                    "Actual registered frag was lost or duplicated by a local view's source visibility/selection");
              const auto receivers=isolated.light_receivers(selected,geometry,fog,&main);
              check(receivers.size()==1 && receivers[0].identity==lights[0].registration.identity &&
                    receivers[0].epoch==local*2+2 && receivers[0].view.pass_mode==1 && receivers[0].view.query_mode==1 &&
                    receivers[0].view.visibility.groups.size()==6 && !receivers[0].view.visibility.clusters.empty(),
                    "Actual frag lost its nested original receiver query or local-view chronology");
            }
            const auto query_cut=isolated.checkpoint();isolated_restore_random=query_cut.random();
            (void)isolated.light_view(eye,collection,candidates,2);
            {
              check(isolated_restore_random[0]==isolated_restore_random[1],"Owned visual RNG lost its shared word");
              auto prepared=isolated.prepare_restore(query_cut,{isolated_restore_random[0],isolated_restore_random[0]},{});
              check(isolated.commit_restore(std::move(prepared)),"Light query state was not restored");
            }
            const auto resumed=isolated.light_view(eye,collection,candidates,1);
            check(resumed.epoch==9 && resumed.visible.size()==1 && resumed.selected.size()==1 &&
                resumed.selected[0].identity==lights[0].registration.identity && resumed.records[0].fades.overall==1,
                "Restored light query lost its spatial pool, four local caches or epoch");++light_handoffs;
            check(isolated.lights().size()==1 && isolated.diagnostics().particles==24,
                "Per-view light queries changed tick-produced grenade effects");
          }
          check(isolated.diagnostics().systems == 6,
                "Actual frag detonation lost its six authored systems");
          const auto frag_draws = isolated.draw(view, 0, tick_seconds);
          const auto sparks = std::find_if(
              cache->tags().begin(), cache->tags().end(), [](const auto &t) {
                return t.group == "prt3" &&
                       t.name == "effects\\generic\\sparks\\sparks_cluster";
              });
          check(sparks != cache->tags().end(),
                "Frag authored sparks definition missing");
          const auto spark_material =
              cache->load_particle_material(sparks->datum, 15, 0, 0);
          check(std::any_of(frag_draws.begin(), frag_draws.end(),
                            [&](const auto &d) {
                              return d.material->bitmaps[0] &&
                                     spark_material.bitmaps[0] &&
                                     d.material->bitmaps[0]->datum ==
                                         spark_material.bitmaps[0]->datum;
                            }),
                "Actual frag detonation lost authored sparks");
          effects.spawn(e.effect, e.projectile, e.tick, frame);
        }
      effects.advance(state.tick, tick_seconds);
      const auto sounds = effects.take_sounds();
      pending_sounds.insert(pending_sounds.end(), sounds.begin(), sounds.end());
      check(effects.take_sounds().empty(),
            "World sounds replayed for another view");
      for (const auto &sound : sounds) {
        death_sounds+=sound.kind==WorldEffectSoundKind::Death;
        check(sound.tick <= state.tick && sound.owner != null_datum &&
                  std::isfinite(sound.position.x) && sound.gain >= 0 &&
                  sound.gain <= 1,
              "World audio lost its actual owning contact/frame/gain");
        audio.impulse(sound.sound, sound.gain, MultiplayerAudioKind::Impulse,
                      sound.owner);
      }
      audio.render(1600);
      audio.update();
      const auto d = effects.diagnostics();
      peak_particles = std::max(peak_particles, d.particles);
      peak_decals = std::max(peak_decals, d.decals);
      peak_systems = std::max(peak_systems, d.systems);
      peak_lights=std::max(peak_lights,d.lights);
      auto draws = effects.draw(view, 0, tick_seconds);
      check(draws.size() <= d.particles,
            "World renderer duplicated retained particles");
      if (t % 5 == 4) {
        for (const auto &e : pending_impacts)
          batched.impact(e);
        for (const auto &e : pending_detonations)
          batched.spawn(e.effect, e.projectile, e.tick,
                        {e.position, e.up, e.forward, e.forward, e.up, false});
        pending_impacts.clear();
        pending_detonations.clear();
        batched.advance(state.tick, tick_seconds);
        const auto a_lights=effects.lights(),b_lights=batched.lights();
        check(a_lights.size()==b_lights.size(),"Render cadence changed effect-light lifetime");
        for(unsigned n=0;n<a_lights.size();++n)
          check(a_lights[n].definition==b_lights[n].definition && a_lights[n].tick==b_lights[n].tick && a_lights[n].state.control==b_lights[n].state.control && a_lights[n].state.brightness==b_lights[n].state.brightness &&
                a_lights[n].registration.identity==b_lights[n].registration.identity && a_lights[n].registration.flags==b_lights[n].registration.flags &&
                a_lights[n].registration.reference_head==b_lights[n].registration.reference_head &&
                a_lights[n].registration.location.leaf==b_lights[n].registration.location.leaf && a_lights[n].registration.location.cluster==b_lights[n].registration.location.cluster,
                "Render cadence changed effect-light state");
        const auto slow_sounds = batched.take_sounds();
        check(slow_sounds.size() == pending_sounds.size(),
              "Render cadence changed actual world sound requests");
        for (unsigned n = 0; n < slow_sounds.size(); ++n) {
          const auto &a = slow_sounds[n], &b = pending_sounds[n];
          check(a.kind == b.kind && a.sound == b.sound && a.owner == b.owner && a.tick == b.tick &&
                    a.gain == b.gain && a.position.x == b.position.x &&
                    a.position.y == b.position.y &&
                    a.position.z == b.position.z,
                "Render cadence changed world audio selection/frame/gain");
        }
        pending_sounds.clear();
        const auto slow_draws = batched.draw(view, 0, tick_seconds);
        check(slow_draws.size() == draws.size(),
              "Render cadence changed actual impact particle count");
        for (unsigned n = 0; n < draws.size(); ++n)
          check(slow_draws[n].vertex.attributes == draws[n].vertex.attributes,
                "Render cadence changed actual particle birth/position/color");
        check(batched.decals(state.tick, 0, tick_seconds).size() ==
                  effects.decals(state.tick, 0, tick_seconds).size(),
              "Render cadence changed actual decals");
        if(!handoff_checked && d.lights && d.decals && d.particles) {
          const auto material_count=mesh.materials.size();std::vector<const std::byte*> resident_images;
          for(const auto &material:std::as_const(mesh).materials)resident_images.push_back(material.diffuse.pixels.data());
          auto obsolete=std::make_shared<int>(0);std::weak_ptr<int> retired=obsolete;
          effects.collision_query([obsolete](auto,auto,auto)->std::optional<ParticleCollisionHit>{return {};});
          const auto cut=effects.checkpoint();const auto random=cut.random();
#if defined(__GLIBC__)
          // Explicit normal-build allocator probe. Keep a full history of
          // actual active-light cuts alive at once; no source state or
          // implementation-specific resource identity is supplied by the test.
          if(std::getenv("HALO2_EFFECT_STORAGE_PROBE")) {
            std::vector<NativeWorldEffects::Checkpoint> retained;retained.reserve(128);
            const auto heap=[] {const auto m=mallinfo2();return std::uint64_t(m.uordblks)+m.hblkhd;};
            const auto before=heap();const auto started=std::chrono::steady_clock::now();
            for(unsigned n=0;n<128;++n)retained.push_back(effects.checkpoint());
            const auto after=heap();
            check(after>before,"Active-light storage probe requires a normal glibc allocator build");
            for(const auto &saved:retained)check(saved.tick()==state.tick&&saved.random()==random,
                "Retaining active light history changed its cut or RNG");
            const auto micros=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-started).count();
            std::cout<<"OBSERVED active_light_checkpoints=128 retained_heap_bytes="<<after-before
                <<" capture_microseconds="<<micros<<" particles="<<d.particles<<" decals="<<d.decals<<" lights="<<d.lights<<'\n';
          }
#endif
          effects.collision_query({});obsolete.reset();
          check(retired.expired(),"World checkpoint retained a predecessor collision closure");
          check(cut.tick()==state.tick && random[0]==shared_visual && random[1]==shared_visual,
              "World checkpoint lost its accepted tick or aliased RNG values");
          const auto &grenade=state.grenade_detonations.back();
          // Declared discarded-future presentation request, not a native birth.
          effects.spawn(grenade.effect,0x98760001,state.tick+1,
              {grenade.position,grenade.up,grenade.forward,grenade.forward,grenade.up,false});
          effects.advance(state.tick+3,tick_seconds);
          bool reversed=false;try{effects.advance(state.tick,tick_seconds);}catch(const FormatError&){reversed=true;}
          check(reversed,"Discarded-future control did not reach a later visual clock");
          const auto future_random=shared_visual;
          bool refused=false;try{(void)batched.prepare_restore(cut,{shared_visual,shared_visual},{});}catch(const FormatError&){refused=true;}
          check(refused,"Foreign world checkpoint was admitted");
          std::uint32_t separate=shared_visual;
          refused=false;try{(void)effects.prepare_restore(cut,{shared_visual,separate},{});}catch(const FormatError&){refused=true;}
          check(refused && shared_visual==future_random,"Changed world RNG aliasing was admitted or wrote live words");
          {
            auto prepared=effects.prepare_restore(cut,{shared_visual,shared_visual},{});
            check(shared_visual==future_random,"World preparation wrote shared RNG");
            check(!batched.commit_restore(std::move(prepared)),"Foreign world restore token was published");
            check(effects.commit_restore(std::move(prepared)) && !effects.commit_restore(std::move(prepared)),
                "World restoration lost or reused its publication token");
            check(shared_visual==future_random,"World commit wrote shared RNG");
          } // Drop the discarded definition graph before observing the clone.
          check(mesh.materials.size()==material_count,"World restoration changed resident material slots");
          for(unsigned n=0;n<material_count;++n)check(std::as_const(mesh).materials[n].diffuse.pixels.data()==resident_images[n],
              "World restoration replaced resident image backing");
          check(cut.random()==random,"World checkpoint borrowed live RNG");shared_visual=random[0];
          const auto restored=effects.draw(view,0,tick_seconds);const auto restored_counts=effects.diagnostics();
          check(restored.size()==slow_draws.size() && restored_counts.particles==d.particles &&
              restored_counts.decals==d.decals && restored_counts.lights==d.lights && restored_counts.systems==d.systems,
              "World restoration cleared active particles, decals, lights or systems");
          for(unsigned n=0;n<restored.size();++n)check(restored[n].vertex.attributes==slow_draws[n].vertex.attributes &&
              restored[n].vertex.constants==slow_draws[n].vertex.constants && restored[n].effect_owner==slow_draws[n].effect_owner &&
              restored[n].effect_definition==slow_draws[n].effect_definition,"Restored world draw differs from untouched control");
          const auto restored_lights=effects.lights();
          check(restored_lights.size()==b_lights.size() && effects.take_sounds().empty(),"World restore lost lights or replayed submitted sounds");
          for(unsigned n=0;n<restored_lights.size();++n)check(restored_lights[n].registration.identity==b_lights[n].registration.identity &&
              restored_lights[n].registration.reference_head==b_lights[n].registration.reference_head &&
              restored_lights[n].registration.flags==b_lights[n].registration.flags &&
              restored_lights[n].state.brightness==b_lights[n].state.brightness,"World restore lost light allocation/spatial identity");
          handoff_checked=true;
          std::cout<<"PASS staged active-world restore at tick "<<state.tick<<" particles="<<d.particles<<" decals="<<d.decals
              <<" lights="<<d.lights<<"; independent batched continuation follows\n";
        }
      }
      // Before the grenade, classification8 comes from actual SMG contacts.
      // Verify the exact authored bitmap and complete decoded texel bank that
      // the GPU upload receives, rather than merely accepting any spark draw.
      if(t<170)for(const auto &draw:draws)if(draw.material->vertex.classification==8) {
        const auto &bitmap=draw.material->bitmaps[0];const auto &texture=draw.material->textures[0];
        check(bitmap && texture,"Actual ricochet spark has no authored texture");
        impact_spark_bitmaps.insert(bitmap->datum);
        const auto raw_texture=cache->load_bitmap(bitmap->datum,bitmap->index);
        const auto authored=raw_texture.format==11?raw_texture:prepare_xemu_bc_texture(raw_texture);
        check(texture->width==authored.width && texture->height==authored.height && texture->pixels==authored.pixels,
            "Ricochet spark submission substituted or changed the authored texture");
      }
      directional_sparks |=
          std::any_of(draws.begin(), draws.end(), [](const auto &v) {
            return v.material->vertex.classification == 8;
          });
      auto vertices = effects.decals(state.tick, 0, tick_seconds);
      check(vertices.size() % 3 == 0,
            "Decal clipping produced incomplete triangles");
      for (const auto &v : vertices)
        check(v.uv.x >= -.0001f && v.uv.x <= 1.0001f && v.uv.y >= -.0001f &&
                  v.uv.y <= 1.0001f,
              "Decal UV escaped clipped receiver");
      const auto before = effects.diagnostics();
      effects.advance(state.tick, tick_seconds);
      check(effects.diagnostics().particles == before.particles,
            "Repeated frame aged particles twice");
      view.width = 1920;
      view.height = 1080;
      check(effects.draw(view, 0, tick_seconds).size() == draws.size(),
            "Resolution changed effect simulation");
      view.width = 640;
      view.height = 480;
      host.broadcast();
      peer.receive();
      const auto updates = peer.received_states();
      if (!updates.empty()) {
        const auto &remote = updates.back();
        check(remote.bullet_impacts.size() == state.bullet_impacts.size(),
              "LAN lost actual impacts");
        if (!state.bullet_impacts.empty()) {
          const auto &a = remote.bullet_impacts.back();
          const auto &b = state.bullet_impacts.back();
          check(a.serial == b.serial && a.material == b.material &&
                    a.position.x == b.position.x && a.normal.z == b.normal.z,
                "LAN changed impact material/plane/identity");
          check(a.presentation && b.presentation &&
              a.presentation->response==b.presentation->response &&
              a.presentation->admitted==b.presentation->admitted &&
              a.presentation->scale_a==b.presentation->scale_a &&
              a.presentation->scale_b==b.presentation->scale_b,
              "LAN lost the actual selected contact response/scales");
        }
      }
    }
    for (const auto &s : effects.diagnostics().unavailable)
      std::cerr << "BOUNDARY " << s << '\n';
    check(audio_packets > 0 && explosion_packets == 1,
          "Real contacts/frag did not submit exactly one authored explosion "
          "PCM packet");
    check(serial > 0, "Actual SMG never delivered a world contact");
    check(peak_particles > 0,
          "Actual impact never produced authored particles");
    check(death_sounds>0,"Actual rock particles never dispatched their authored death sounds");
    check(peak_decals > 0, "Actual contact never produced a surface decal");
    check(handoff_checked && light_handoffs>0,"Active world/light handoff coverage did not run");
    check(detonation > 0, "Actual Q never produced a grenade explosion");
    check(peak_systems >= 6, "Frag explosion lost authored systems");
    check(peak_lights>0,"Actual frag created no source light state");
    std::cout<<"Actual SMG impact spark bitmaps:";for(auto datum:impact_spark_bitmaps)std::cout<<' '<<std::hex<<datum;std::cout<<std::dec<<'\n';
    check(impact_spark_bitmaps==std::set<std::uint32_t>{0xe1f3007eu},"Actual Lockout SMG ricochet lost its animated spark atlas");
    check(directional_sparks,
          "Actual SMG impact lost its directional spark shader");
    check(shared_visual!=0x12345678,"World particle events copied their borrowed random owner");
    effects.advance(6000, tick_seconds);
    check(effects.diagnostics().particles == 0,
          "Retired effect particles leaked");
    check(effects.diagnostics().decals == 0,
          "Authored decal lifetime did not expire");
    check(effects.lights().empty(),"Authored light lifetime did not expire");
    std::cout << "PASS actual contacts=" << serial
              << " peak particles=" << peak_particles
              << " decals=" << peak_decals << " systems=" << peak_systems
              << " grenade=" << detonation << "; SDL packets=" << audio_packets
              << "; LAN/resolution/retirement verified\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
