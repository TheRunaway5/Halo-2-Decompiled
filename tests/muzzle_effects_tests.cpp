#include "halo2/muzzle_effects.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/simulation.hpp"
#include "halo2/effect_sound.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include "halo2/fixed_effect_event.hpp"

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
}
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw FormatError("Supply original Lockout map");
    if(!std::filesystem::exists(argv[1]))return 77;
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    const auto cues=fixed_effect_sound_cues(cache,0xe70a0595u);
    check(cues.size()==1 && cues[0].sound==0xe70f059au && cues[0].gain==1 &&
        cues[0].elapsed==std::bit_cast<float>(0x3d088889u),"Actual fusion-coil sound event/timing changed");
    NativeEffectSounds sounds(cache);sounds.spawn(0xe70a0595u,0x10001,{1,2,3});unsigned played{};
    sounds.advance(0,[&](const EffectSoundCue &cue,std::uint32_t owner,Vec3 position){
      check(cue.sound==0xe70f059au && owner==0x10001 && position.x==1 && position.y==2 && position.z==3,
          "Actual destruction sound lost its retained event owner");++played;
    });
    check(played==0,"Fusion-coil flags1C incorrectly consumed a one-second initial interval");
    sounds.advance(tick_seconds,[&](const EffectSoundCue &cue,std::uint32_t owner,Vec3 position){
      check(cue.sound==0xe70f059au && owner==0x10001 && position.x==1 && position.y==2 && position.z==3,
          "Actual destruction sound lost its retained event owner");++played;
    });
    sounds.advance(2,[&](const auto&,auto,auto){++played;});
    check(played==1,"Actual destruction sound duplicated or never submitted");
    std::ifstream original(std::filesystem::path(__FILE__).parent_path()/"fixtures/sword_ready_sequence_original.tsv");
    check(bool(original),"Missing independent Sword ready fixture");
    SourceWeaponShaderState ready_shader{13,0,0};
    auto countdown=source_weapon_ready_begin(ready_shader,.7f,30);
    FixedEffectEventState fixed{0,.5f,false,false};unsigned starts{};
    for(std::string line;std::getline(original,line);) {
      if(line.empty() || line[0]=='#')continue;
      std::istringstream row(line);char kind;row>>kind;row>>std::hex;
      if(kind=='R') {
        std::uint32_t item,flags,power,ticks;row>>item>>flags>>power>>ticks;
        check(ready_shader.item_flags==item && ready_shader.weapon_flags==flags &&
            std::bit_cast<std::uint32_t>(ready_shader.power_fraction)==power &&
            std::uint16_t(countdown)==ticks,"Sword ready countdown differs from complete original1021D0");
        source_weapon_ready_tick(ready_shader,countdown);
      }else {
        std::uint32_t tick,elapsed,target,active,finished,begins;row>>tick>>elapsed>>target>>active>>finished>>begins;
        const auto before=starts;
        advance_fixed_effect_event(fixed,.5f,tick?tick_seconds:0,[&]{++starts;},[](float,float){});
        check(std::bit_cast<std::uint32_t>(fixed.elapsed)==elapsed &&
            std::bit_cast<std::uint32_t>(fixed.target)==target && fixed.active==bool(active) &&
            fixed.finished==bool(finished) && starts-before==begins,"Fixed activation event differs from original17AD20");
      }
    }
    const auto content=cache.load_content(0,true);
    const auto needler=std::find_if(content.weapons.begin(),content.weapons.end(),[](const auto &p){return p.second.name.ends_with("\\needler");});
    check(needler!=content.weapons.end(),"Actual Needler definition absent");
    const auto needler_firing=needler->second.firing_effects.at(0).effects[0];
    const auto needler_effect=cache.load_muzzle_effect(needler_firing);
    check(!needler_effect.systems.empty(),"Actual Needler firing effect lost its particle systems");
    // Compare complete deterministic event state and every emitted slice to
    //independent executions of the original17AD20/17AC00/179D10 bodies.
    std::ifstream sequence(std::filesystem::path(__FILE__).parent_path()/"fixtures/weapon_fixed_events_original.tsv");
    check(bool(sequence),"Missing independent weapon event fixture");
    FixedEffectSequenceState sequence_state;std::uint32_t previous_case=0xffffffffu;unsigned sequence_rows{},camera_rows{};
    std::vector<FixedEffectEventDefinition> sequence_events;
    for(std::string line;std::getline(sequence,line);) {
      if(line.empty() || line[0]=='#')continue;
      std::istringstream row(line);char kind;row>>kind>>std::hex;
      if(kind=='T') {
        std::uint32_t case_id,t,datum,dt,index,elapsed,target,active,finished,n;
        row>>case_id>>t>>datum>>dt>>index>>elapsed>>target>>active>>finished>>n;
        std::vector<unsigned> expected_begin(n);for(auto &v:expected_begin)row>>v;
        row>>n;std::vector<std::array<std::uint32_t,3>> expected_slices(n);
        for(auto &slice:expected_slices)row>>slice[0]>>slice[1]>>slice[2];
        if(previous_case!=case_id) {
          sequence_events=cache.load_muzzle_effect(datum).events;
          sequence_state={{0,sequence_events.front().delay,false,false},0};previous_case=case_id;
        }
        std::vector<unsigned> begins;std::vector<std::array<std::uint32_t,3>> slices;
        advance_fixed_effect_sequence(sequence_state,sequence_events,std::bit_cast<float>(dt),
            [&](unsigned event){begins.push_back(event);},[&](unsigned event,float before,float seconds){
              slices.push_back({event,std::bit_cast<std::uint32_t>(before),std::bit_cast<std::uint32_t>(seconds)});
            });
        check(sequence_state.event==index && sequence_state.clock.active==bool(active) &&
            sequence_state.clock.finished==bool(finished) && std::bit_cast<std::uint32_t>(sequence_state.clock.elapsed)==elapsed &&
            std::bit_cast<std::uint32_t>(sequence_state.clock.target)==target && begins==expected_begin && slices==expected_slices,
            "Native weapon event sequence differs from unchanged original instructions");++sequence_rows;
      }else if(kind=='C') {
        std::uint32_t event,users,mode,underwater,n;row>>event>>users>>mode>>underwater>>n;
        std::vector<unsigned> expected(n);for(auto &v:expected)row>>v;
        std::vector<unsigned> selected;
        // Literal Needler systems: four shared-camera muzzle systems in0;
        //paired first/third-person world-coordinate smoke systems in1.
        for(unsigned j=0;j<(event?2u:4u);++j)
          if(source_effect_system_admitted(0,event?j+1:3,underwater,users,mode==0))selected.push_back(j);
        check(selected==expected,"Needler camera admission differs from original17AA90");++camera_rows;
      }else throw FormatError("Invalid weapon event fixture row");
      check(bool(row),"Truncated independent weapon event fixture");
    }
    check(sequence_rows==372 && camera_rows==36,"Independent sequence/camera cases omitted");
    const auto &weapon=content.weapons.at(cache.slayer_starting_weapon(5));
    const auto effect=weapon.firing_effects.at(0).effects[0];
    NativeMuzzleEffects owner(cache,0x12345678);
    std::uint32_t primary=0x78a8,auxiliary=0x12345678;
    NativeMuzzleEffects shared_owner(cache,SourceEffectRandom{primary,auxiliary});
    const auto model=cache.load_model(weapon.first_person_model,false);
    const auto nodes=pose_model_nodes_world(model,Transform{},nullptr,0);
    const auto markers=pose_model_markers(source_model_markers(cache.load_source_tag_metadata(weapon.first_person_model),
        owner.marker_name(effect)),nodes);
    check(markers.size()==1,"Actual stock muzzle marker was not resolved");
    MuzzleEffectView view{};view.forward={1,0,0};view.up={0,0,1};view.width=640;view.height=480;
    view.orientation={1,0,0,0,1,0,0,0,1};
    std::uint32_t tick{},previous=0xffffffff;
    // More replaced weapon lifetimes than every original pool's capacity.
    // A flash that looks correct once must still release all its owners.
    for(unsigned shot=0;shot<600;++shot) {
      owner.advance(tick,tick_seconds);
      const auto handle=owner.request({effect,0x10000u+shot,previous,0x3f800000,0x3f800000},tick);
      check(handle!=0xffffffff,"Retired muzzle owners exhausted their original pools");
      const auto emitted_sounds=owner.take_sounds();
      check(emitted_sounds.size()==1 && emitted_sounds[0].owner==0x10000u+shot &&
          emitted_sounds[0].cue.sound==weapon.sound && emitted_sounds[0].cue.gain==1,
          "SMG normal effect lost its one authored sound part");
      owner.pose(0x10000u+shot,[&](auto){return markers[0].world;},view,tick,tick_seconds);
      const auto draws=owner.draw(0x10000u+shot,markers[0].world,view,tick,0,tick_seconds);
      check(!draws.empty() && owner.particle_count()>0,"Actual muzzle event failed to birth particles");
      if(shot==0) {
        shared_owner.advance(tick,tick_seconds);
        shared_owner.request({effect,0x10000u+shot,previous,0x3f800000,0x3f800000},tick);
        shared_owner.pose(0x10000u+shot,[&](auto){return markers[0].world;},view,tick,tick_seconds);
        const auto borrowed=shared_owner.draw(0x10000u+shot,markers[0].world,view,tick,0,tick_seconds);
        check(borrowed.size()==draws.size() && auxiliary!=0x12345678 && primary==0x78a8,
            "Muzzle birth did not retain its shared auxiliary stream");
        for(std::size_t i=0;i<draws.size();++i)
          check(borrowed[i].vertex.attributes==draws[i].vertex.attributes &&
              borrowed[i].vertex.constants==draws[i].vertex.constants,
              "Borrowed muzzle stream changed the authored particle birth");
      }
      // Bound only the regression's wait; each particle's authored lifetime,
      // rather than this bound, controls the production removal decision.
      unsigned elapsed{};
      do {owner.advance(++tick,tick_seconds);++elapsed;}while(owner.particle_count() && elapsed<300);
      check(!owner.particle_count(),"Actual muzzle particles did not expire");
      previous=handle; // Exercises validation of a now stale full handle.
    }
    // Drawing is optional: a switched weapon or occluded view must still
    // emit and release every native owner on the simulation clock.
    NativeMuzzleEffects unseen(cache,0x12345678);
    unsigned unseen_tick{};
    for(unsigned shot=0;shot<600;++shot) {
      const auto handle=unseen.request({effect,0x50000u+shot,0xffffffffu,0x3f800000,0x3f800000},unseen_tick);
      check(handle!=0xffffffffu,"Undrawn effects leaked their original effect pool");
      unseen.pose(0x50000u+shot,[&](auto){return markers[0].world;},view,unseen_tick,tick_seconds);
      for(unsigned n=0;n<90;++n)unseen.advance(++unseen_tick,tick_seconds);
      check(!unseen.particle_count(),"Undrawn effects retained expired particles");
    }
    check(needler_effect.events.size()==2 && needler_effect.systems.size()==5,
        "Needler omitted its original shared-camera muzzle or FP smoke system");
    NativeMuzzleEffects firing(cache,0x12345678);
    const auto needler_model=cache.load_model(needler->second.first_person_model,false);
    const auto needler_nodes=pose_model_nodes_world(needler_model,Transform{},nullptr,0);
    const auto needler_marker=[&](std::uint32_t name) {
      const auto selected=pose_model_markers(source_model_markers(cache.load_source_tag_metadata(needler->second.first_person_model),name),needler_nodes);
      check(selected.size()==1,"Needler firing marker has no actual singleton pose");return selected[0].world;
    };
    std::set<std::uint32_t> expected_bitmaps,observed_bitmaps;
    for(const auto &system:needler_effect.systems)for(const auto &bitmap:system.material->bitmaps)if(bitmap)expected_bitmaps.insert(bitmap->datum);
    unsigned firing_sounds{},firing_draws{};std::uint32_t firing_previous=0xffffffffu;
    //Repeated live refreshes exercise the real multi-event owner with old
    //particles retained; subsequent detached lifetimes exceed all pools.
    unsigned firing_tick{};
    for(unsigned shot=0;shot<600;++shot) {
      firing.advance(firing_tick,tick_seconds);
      const auto handle=firing.request({needler_firing,0x40000u+(shot<30?0:shot),shot<30?firing_previous:0xffffffffu,0,0},firing_tick);
      check(handle!=0xffffffffu,"Needler firing owners leaked their source pools");firing_previous=handle;
      const auto owner=0x40000u+(shot<30?0:shot);
      const unsigned wait=shot<29?4:80;
      for(unsigned n=0;n<wait;++n) {
        firing.pose(owner,needler_marker,view,firing_tick,tick_seconds);
        firing.advance(firing_tick,tick_seconds);
        for(const auto &sound:firing.take_sounds()) {
          check(sound.owner==owner && sound.cue.sound==needler->second.sound && sound.cue.gain==1,
              "Needler firing sound lost its authored identity, gain or owner");++firing_sounds;
        }
        const auto particles=firing.draw(owner,needler_marker,view,firing_tick,0,tick_seconds);
        firing_draws+=unsigned(particles.size());
        for(const auto &draw:particles)for(const auto &bitmap:draw.material->bitmaps)if(bitmap)observed_bitmaps.insert(bitmap->datum);
        ++firing_tick;
      }
    }
    check(firing_sounds==600 && firing_draws>600 && !firing.particle_count() && observed_bitmaps==expected_bitmaps,
        "Needler firing lost sound, authored textures, smoke, emission or retirement");
    std::cout<<"Needler firing: "<<firing_draws<<" particle draws, "<<firing_sounds
        <<" sounds, "<<observed_bitmaps.size()<<" exact authored bitmap identities; "
        <<sequence_rows<<" event states and "<<camera_rows<<" camera controls match original\n";
    const auto sword=std::find_if(content.weapons.begin(),content.weapons.end(),[](const auto &p){return p.second.name.ends_with("\\energy_blade");});
    check(sword!=content.weapons.end() && sword->second.ready_seconds==.7f && sword->second.ready_effect==0xe2fb0186u,
        "Sword ready authored fields changed");
    NativeMuzzleEffects activation(cache,0x12345678);
    const auto sword_model=cache.load_model(sword->second.first_person_model,false);
    const auto sword_nodes=pose_model_nodes_world(sword_model,Transform{},nullptr,0);
    const auto placement=[&](std::uint32_t name) {
      const auto selected=pose_model_markers(source_model_markers(cache.load_source_tag_metadata(sword->second.first_person_model),name),sword_nodes);
      check(selected.size()==1,"Sword activation marker does not resolve to one authored pose");
      return selected[0].world;
    };
    activation.advance(0,tick_seconds);activation.request({sword->second.ready_effect,0x30000,0xffffffffu,0,0},0);
    unsigned activation_sounds{},activation_draws{};
    for(unsigned t=0;t<120;++t) {
      activation.pose(0x30000,placement,view,t,tick_seconds);
      activation.advance(t,tick_seconds);
      for(const auto &sound:activation.take_sounds()) {
        check(t==15 && sound.owner==0x30000 && sound.cue.sound==0xe2fc0187u && sound.cue.gain==1,
            "Authored Sword ready sound duplicated or changed timing/owner");++activation_sounds;
      }
      const auto draws=activation.draw(0x30000,placement,view,t,0,tick_seconds);
      if(t<15)check(draws.empty(),"Sword activation emitted before its authored delay");
      if(!activation_draws && !draws.empty())std::cout<<"First standalone Sword particles: tick="<<t<<'\n';
      activation_draws+=unsigned(draws.size());
    }
    check(activation_sounds==1 && activation_draws>0 && activation.particle_count()==0,
        "Sword activation lost sound, particles or retirement");
    const auto *blade=cache.query_tag({.group=tag_groups::effect,
        .name="effects\\objects\\weapons\\melee\\energy_blade\\blade_on"});
    check(blade,"Original persistent blade attachment is absent");
    const NativeMuzzleEffects::Markers blade_placement=[&](std::uint32_t name) {
      const auto selected=pose_model_markers(source_model_markers(cache.load_source_tag_metadata(sword->second.first_person_model),name),sword_nodes);
      check(!selected.empty(),"Blade location has no actual authored marker bank");
      std::vector<Transform> result;for(const auto &marker:selected)result.push_back(marker.world);return result;
    };
    NativeMuzzleEffects attached(cache,0x12345678);std::uint32_t retained=null_datum;
    unsigned blade_draws{};std::size_t blade_locations{};
    for(unsigned t=0;t<600;++t) {
      const auto handle=attached.attachment(blade->datum,0x60000,t,{1,1});
      check(handle!=null_datum && (retained==null_datum || handle==retained),"Blade attachment lost its retained lifetime");
      retained=handle;attached.pose(0x60000,blade_placement,view,t,tick_seconds);
      attached.advance(t,tick_seconds);
      const auto draws=attached.draw(0x60000,placement,view,t,0,tick_seconds);
      blade_draws+=unsigned(draws.size());blade_locations=std::max(blade_locations,attached.location_count());
      const auto second=attached.draw(0x60000,placement,view,t,0,tick_seconds);
      check(second.size()==draws.size(),"Drawing another view changed the blade population");
    }
    check(blade_locations==17,"Blade attachment omitted authored glow/flare marker locations");
    check(blade_draws>600 && attached.particle_count()>0,"Blade attachment stopped at its first event");
    for(unsigned t=600;t<900;++t) {
      attached.attachment(blade->datum,0x60000,t,{0,1},false);
      attached.pose(0x60000,blade_placement,view,t,tick_seconds);attached.advance(t,tick_seconds);
    }
    check(!attached.particle_count(),"Disabled source blade function continued emission");
    check(attached.attachment(blade->datum,0x60000,900,{1,1},true)==retained,
        "Blade reactivation allocated a second attachment owner");
    for(unsigned t=900;t<930;++t) {
      attached.pose(0x60000,blade_placement,view,t,tick_seconds);attached.advance(t,tick_seconds);
    }
    check(attached.particle_count()>0,"Blade function did not restart the original event");
    attached.retire_attachments_except({});
    for(unsigned t=930;t<1230;++t)attached.advance(t,tick_seconds);
    check(!attached.particle_count(),"Retired blade attachment retained expired particles");
    std::cout<<"Persistent blade: "<<blade_draws<<" draws across 600 ticks, retained handle and complete retirement\n";
    std::cout<<"PASS 600 actual SMG births/expiry, original Sword countdown/event timeline, "
        <<activation_draws<<" Sword particle draws and one delayed ready sound\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
