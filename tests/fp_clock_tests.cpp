#include "halo2/fp_registration.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/first_person_ik.hpp"
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
using namespace halo2;
namespace {
std::uint32_t word(std::istringstream &row) {
  std::string value;if(!(row>>value))throw FormatError("Incomplete original FP clock fixture");
  return std::stoul(value,nullptr,16);
}
void check(bool value,const std::string &label) {if(!value)throw FormatError("Original FP clock differs: "+label);}
using State=std::array<std::uint32_t,11>;
State read(std::istringstream &row) {State result;for(auto &v:result)v=word(row);return result;}
State state(const FirstPersonHandBinding &h) {
  return {h.graph,h.animation_identifier,h.channel_delay,h.channel_flags,h.channel_options,h.events,h.future_events,
      std::bit_cast<std::uint32_t>(h.rate),std::bit_cast<std::uint32_t>(h.frame),h.activity,
      std::uint32_t(std::int32_t(h.fire_ticks))};
}
void compare(const FirstPersonHandBinding &h,const State &expected,const std::string &label) {
  const auto actual=state(h);
  for(unsigned i=0;i<actual.size();++i)
    if(actual[i]!=expected[i]) {
      std::ostringstream error;error<<label<<" field"<<i<<" expected "<<std::hex<<expected[i]<<" got "<<actual[i];
      check(false,error.str());
    }
}
FirstPersonAnimationEvent event(std::istringstream &row) {
  FirstPersonAnimationEvent e;e.type=std::uint16_t(word(row));e.frame=std::bit_cast<std::int16_t>(std::uint16_t(word(row)));
  e.tag=word(row);e.id=word(row);e.reference_flags=word(row);e.marker=word(row);return e;
}
void compare_event(const FirstPersonAnimationEvent &a,const FirstPersonAnimationEvent &b,const std::string &label) {
  check(a.type==b.type && a.frame==b.frame && a.tag==b.tag && a.id==b.id &&
      a.reference_flags==b.reference_flags && a.marker==b.marker,label+" original callback20 bytes");
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3 && argc!=4)throw FormatError("Supply original map and FP clock fixture [--presentation]");
    const bool presentation=argc==4 && std::string(argv[3])=="--presentation";
    if(argc==4 && !presentation)throw FormatError("Unknown FP clock test option");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    const Cache cache(argv[1]);auto content=cache.load_content(0,true);content.kill_triggers.clear();
    const auto definition=cache.load_first_person_binding(cache.slayer_starting_weapon(5),2);
    check(definition.continuous_fire_animation,"authored weapon flag17");
    const auto &smg_definition=*content.weapons.at(definition.weapon_definition).smg;
    check(bool(content.original_data),"original direction table");
    std::ifstream file(argv[2]);FirstPersonViewBinding view;SmgState smg;std::uint32_t seed{};
    std::unique_ptr<MultiplayerMatch> match;std::string name;unsigned ticks=0,local_ticks=0,events=0,clips=0;
    unsigned delivered_events=0;
    std::vector<FirstPersonAnimationEvent> delivered;
    const std::array<unsigned,5> refresh_rates{30,60,120,144,240};
    std::array<unsigned,5> render_frames{};unsigned interpolated=0,cuts=0,fractional=0;
    std::optional<RenderModel> render_weapon,render_arms;
    std::optional<FirstPersonIkDefinition> render_ik;
    std::map<std::uint32_t,AnimationClip> render_clips;
    std::set<std::uint32_t> posed_activities;
    if(presentation) {
      render_weapon=cache.load_model(definition.weapon_model,false);
      render_arms=cache.load_model(definition.arms_model,false);
      render_ik=cache.load_first_person_ik(definition.graph,definition.weapon_model,definition.arms_model);
      for(const auto &clip:definition.clock_clips)
        render_clips.emplace(clip.activity,cache.load_animation_identifier(definition.graph,first_person_clip_identifier(clip)));
    }
    for(std::string line;std::getline(file,line);) {
      std::istringstream row(line);char kind;row>>kind;
      if(kind=='D') {
        const auto activity=word(row),index=word(row),frames=word(row),loop=word(row),count=word(row);
        const auto found=std::find_if(definition.clock_clips.begin(),definition.clock_clips.end(),
            [&](const auto &c){return c.activity==activity;});
        check(found!=definition.clock_clips.end() && found->index==index && unsigned(found->frames)==frames &&
            unsigned(found->loop_frame)==loop && found->events.size()==count,"authored clip metadata");
        for(const auto &e:found->events)compare_event(e,event(row),"authored event");++clips;
      } else if(kind=='I') {
        row>>name;view={};FirstPersonBindingInput input;
        input.player=0x10000;input.unit=0x10001;input.representation=2;input.weapons[0]=0x10002;
        observe_first_person_binding(view,input,[&](auto,auto){return &definition;});
        compare(view.hands[0],read(row),name+" initial");
        smg={};smg.loaded=60;smg.reserve=120;smg_initial_equip(smg,smg_definition);seed=0x12345678;
        match.reset();
        if(name!="normal_wrapper_ready") {
          SlayerRules rules;rules.weapon=definition.weapon_definition;rules.score_limit=rules.time_ticks=0;
          rules.random_seed=seed;match=std::make_unique<MultiplayerMatch>(content,0,cache.spawns(),rules);
          const unsigned player=match->join("Original FP clock");
          match->configure_replicated_first_person(definition);
          match->configure_local_views(std::span(&player,1),definition);
          match->configure_first_person_event_owner(
              [&](unsigned local_view,unsigned local_hand,const FirstPersonViewBinding &owner,
                  const FirstPersonAnimationEvent &event) {
                check(local_view==0 && local_hand==0,"actual local event recipient");
                check(owner.unit==match->unit(0).source_datum &&
                      owner.hands[0].weapon==match->unit(0).weapon.source_datum &&
                      owner.hands[0].graph==definition.graph,"actual salted unit/weapon/graph event owner");
                delivered.push_back(event);
              });
        }
      } else if(kind=='T') {
        const auto tick=word(row);const bool held=word(row),reload=word(row),ready=word(row);
        std::array<State,5> expected;for(auto &phase:expected)phase=read(row);
        const auto count=word(row);std::vector<FirstPersonAnimationEvent> expected_events,actual_events;
        for(unsigned i=0;i<count;++i)expected_events.push_back(event(row));
        auto &hand=view.hands[0];const auto label=name+" tick"+std::to_string(tick);
        const SmgActivityCallback activity=[&](auto a){first_person_activity(hand,definition,a);};
        compare(hand,expected[0],label+" before");smg_input(smg,held);
        if(ready) { // Supplied current-index NONE: timed wrapper admission is outside this native producer.
          smg.blocking_ticks=smg_definition.ready_ticks;first_person_activity(hand,definition,0x05000024);
        }
        if(reload)smg_reload(smg,smg_definition,activity);
        compare(hand,expected[1],label+" unit");smg_update(smg,smg_definition,tick_seconds,activity);
        compare(hand,expected[2],label+" weapon");
        smg_dispatch(smg,smg_definition,{1,0,0},seed,content.original_data->weapon_directions);
        compare(hand,expected[3],label+" postpass");
        const auto clock_origin=view;
        first_person_animation_clock(hand,definition,tick_seconds,[&](const auto &e){actual_events.push_back(e);});
        compare(hand,expected[4],label+" FP");
        if(presentation) {
          const auto end=first_person_render_frame(clock_origin,view,definition,1,true);
          check(std::bit_cast<std::uint32_t>(end.frame)==std::bit_cast<std::uint32_t>(hand.frame),
              label+" presentation endpoint changed source frame");
          end.interpolated?++interpolated:++cuts;
          for(unsigned rate=0;rate<refresh_rates.size();++rate) {
            // Render instants use the exact rational refresh clock, including
            // the alternating four/five samples per30Hz interval at144Hz.
            const auto hz=refresh_rates[rate];
            const unsigned first=(ticks*hz+29)/30,last=((ticks+1)*hz+29)/30;
            for(unsigned frame=first;frame<last;++frame) {
              const float fraction=float(double(frame)*30.0/double(hz)-double(ticks));
              const auto rendered=first_person_render_frame(clock_origin,view,definition,fraction,true);
              check(rendered.frame>=0 && std::isfinite(rendered.frame),label+" finite fractional presentation");
              if(end.interpolated)check(rendered.interpolated,label+" continuity varied with refresh rate");
              else check(rendered.frame==hand.frame && !rendered.interpolated,label+" cut blended an action/reset");
              fractional+=rendered.interpolated && fraction>0 && rendered.frame!=hand.frame &&
                  rendered.frame!=clock_origin.hands[0].frame;
              ++render_frames[rate];
            }
          }
          // Rendering cannot write authoritative channels, event masks or
          // feedback timers. The original full post-clock state remains exact.
          compare(hand,expected[4],label+" after all render frequencies");
          const auto start=first_person_render_frame(clock_origin,view,definition,0,true);
          check(std::bit_cast<std::uint32_t>(start.frame)==std::bit_cast<std::uint32_t>(
              end.interpolated?clock_origin.hands[0].frame:hand.frame),label+" exact fraction-zero endpoint");
          const auto gap=first_person_render_frame(clock_origin,view,definition,.5f,false);
          check(!gap.interpolated && gap.frame==hand.frame,label+" snapshot-gap cut");
          auto replacement=clock_origin;replacement.unit^=0x10000;
          const auto respawn=first_person_render_frame(replacement,view,definition,.5f,true);
          check(!respawn.interpolated && respawn.frame==hand.frame,label+" salted respawn cut");
          if(end.interpolated && hand.frame>clock_origin.hands[0].frame &&
              clock_origin.hands[0].frame>2 && !posed_activities.contains(hand.activity)) {
            // Actual decoded arms/weapon geometry, under a declared fixed
            // preview camera/aim input: fractional samples must change both
            // meshes, rather than only changing a diagnostic clock value.
            const auto &clip=render_clips.at(hand.activity);
            const auto offset=content.weapons.at(definition.weapon_definition).first_person_offset;
            const auto pose_at=[&](float fraction) {
              const auto frame=first_person_render_frame(clock_origin,view,definition,fraction,true);
              return pose_first_person_models_world_frame(*render_weapon,*render_arms,clip,frame.frame,
                  offset,Transform{},*render_ik,0);
            };
            const auto a=pose_at(.25f),b=pose_at(.75f);
            const auto changed=[](const auto &a,const auto &b) {
              if(a.size()!=b.size() || a.empty())return false;
              for(unsigned n=0;n<a.size();++n)
                if(length(a[n].position-b[n].position)>0)return true;
              return false;
            };
            check(changed(a.weapon,b.weapon) && changed(a.arms,b.arms),label+" fractional skinning stayed stepped");
            posed_activities.insert(hand.activity);
          }
        }
        check(actual_events.size()==expected_events.size(),label+" callback count");
        for(unsigned i=0;i<actual_events.size();++i)compare_event(actual_events[i],expected_events[i],label);
        if(match) {
          check(match->simulation().ticks()==tick,label+" production tick");
          compare(match->first_person_view(0).hands[0],expected[0],label+" local before");
          std::array<Controls,multiplayer_capacity> controls{};
          controls[0].fire=held;controls[0].reload=reload;controls[0].yaw=match->unit(0).yaw;
          delivered.clear();
          match->step(controls);compare(match->first_person_view(0).hands[0],expected[4],label+" local FP");++local_ticks;
          check(delivered.size()==expected_events.size(),label+" delivered local callback count / no replica duplication");
          for(unsigned i=0;i<delivered.size();++i)compare_event(delivered[i],expected_events[i],label+" delivered source event");
          delivered_events+=unsigned(delivered.size());
          auto replicated=match->first_person_view(0);FirstPersonViewBinding replicated_origin;
          check(apply_replicated_first_person_clock(replicated_origin,replicated,
              match->state().players[0].first_person_clock,definition),label+" native replica clock presence");
          compare(replicated_origin.hands[0],expected[3],label+" replica source-clock origin");
          compare(replicated.hands[0],expected[4],label+" replica source-clock output");
          if(presentation)compare(match->first_person_clock_origin(0).hands[0],expected[3],label+" actual local presentation origin");
        }
        ++ticks;events+=count;
      } else throw FormatError("Unknown FP clock fixture command");
      std::string extra;check(!(row>>extra),"trailing fixture word");
    }
    check(clips==5 && ticks==541 && local_ticks==516 && events==4,"complete independent source fixture");
    check(delivered_events>0,"actual gameplay did not deliver authored events");
    std::cout<<"Actual local event delivery: "<<delivered_events
        <<" original callbacks with full unit/weapon/graph owners; no remote replica duplication\n";
    if(presentation) {
      check(interpolated>400 && cuts>0 && fractional>1000,"continuous/cut/high-refresh coverage");
      check(posed_activities.size()>=3,"actual decoded idle/fire/reload skin coverage");
      for(unsigned rate=0;rate<refresh_rates.size();++rate)
        check(render_frames[rate]==(ticks*refresh_rates[rate]+29)/30,"exact rational refresh cadence");
      std::cout<<"Presentation at30/60/120/144/240Hz: "<<interpolated<<" continuous source pairs, "
          <<cuts<<" source cuts and "<<fractional<<" intermediate frames; authoritative events unchanged\n";
    }
    std::cout<<ticks<<" original ticks/"<<ticks*5<<" primary-channel phase states,"<<events<<" original callbacks and "
        <<local_ticks<<" local match ticks matched\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
