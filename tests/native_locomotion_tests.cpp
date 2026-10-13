#define SDL_MAIN_HANDLED
#include "halo2/native_locomotion.hpp"
#include "halo2/native_player_animation.hpp"
#include "halo2/frontend_model_locals.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"
#include <fstream>
#include <iostream>
#include <set>
#include <cmath>
using namespace halo2;
namespace{void check(bool b,const char*m){if(!b)throw FormatError(m);}}
// Compare real cache codec8 tracks with the statically recovered original
// component decoders, including their literal16-byte quaternion frame step.
unsigned verify_float_tracks(const Cache &cache,const AnimationClip &clip) {
 const auto tag=cache.load_source_tag_metadata(clip.source_graph);
 const Bytes root(tag.bytes_at(tag.source_address(),0xb4));
 const Bytes definition(tag.bytes_at(root.u32(0x30)+clip.source_index*0x6c,0x4c));
 const auto resource=cache.load_source_runtime_animation_image(root.u32(0xb0)+definition.u32(0x24)*20);
 const unsigned offset=definition.u32(0x28),ss=definition.u16(0x42),as=definition.u32(0x48);
 if(!as || std::to_integer<unsigned>(resource.bytes.at(offset+ss))!=8)return 0;
 const Bytes codec(std::span(resource.bytes).subspan(offset+ss,as));
 SourceRuntimeMemory memory;
 memory.mount(0x50bc00,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"test original component globals");
 memory.mount(0x70000000,std::vector<std::byte>(65536),SourceRuntimeMemory::Access::ReadWrite,"test original component stack");
 memory.mount(0x60000000,std::span(resource.bytes).subspan(offset+ss,as),SourceRuntimeMemory::Access::ReadOnly,"actual cache codec8 bytes");
 auto put=[&](std::uint32_t p,std::uint32_t v){auto b=memory.mutable_view(p,4);for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));};
 put(0x50bc70,0x60000000);put(0x50bcb0,0x50bd00);
 const unsigned mask_size=((clip.nodes.size()+31)/32)*4;
 const Bytes raw(resource.bytes);unsigned comparisons=0;
 for(unsigned c=0;c<3;++c) {
  unsigned ordinal=0;
  for(unsigned n=0;n<clip.nodes.size();++n) {
   if(!(raw.u32(offset+ss+as+std::to_integer<unsigned>(definition.slice(0x3c,1)[0])+c*mask_size+(n/32)*4)&(1u<<(n%32))))continue;
   put(0x50bca4+c*4,ordinal++);
   for(unsigned frame=0;frame<clip.frame_count;++frame) {
    put(0x50bc54,frame);SourceUnitAnimationRuntimeAbi abi{};abi.gp[4]=0x70008000;abi.eflags=2;
    source_run_unit_animation_selection_1_1(memory,c==0?0x292970:c==1?0x2929b0:0x2cb1e0,abi,{0x27f,0x1f80},{});
    const auto &key=clip.channels[n][c].keys.at(frame);
    const Bytes result(memory.view(0x50bd00+(c==0?0:c==1?16:28),c==0?16:c==1?12:4));
    for(unsigned k=0;k<(c==0?4u:c==1?3u:1u);++k)
     check(std::bit_cast<std::uint32_t>(key[k])==result.u32(k*4),"Cache codec8 differs from original component decoder");
    ++comparisons;
   }
  }
  check(ordinal==std::to_integer<unsigned>(codec.slice(c+1,1)[0]),"Codec8 original component count mismatch");
 }
 return comparisons;
}
int main(int argc,char**argv){try{
 check(argc==5,"Supply MAP SHARED GAMEPLAY_XBE FOOTSTEP_FIXTURE");
 if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[3]))return 77;
 SDL_SetMainReady();auto data=OriginalData::load_xbe(argv[3]);Cache cache(argv[1],argv[2],data);
 check(data->unit_locomotion_deadzone_bits==0x3c23d70bu,"Original source deadzone operand changed");
 const auto content=cache.load_content(0,true);const auto weapon=cache.slayer_starting_weapon(5);
 const auto bipd=cache.load_source_tag_metadata(content.player.datum);
 const Bytes b(bipd.bytes_at(bipd.source_address(),0x304));const auto foot=b.u32(0x58);
 const auto model=cache.load_source_tag_metadata(b.u32(0x38));const Bytes m(model.bytes_at(model.source_address(),0x94));
 const auto graph=m.u32(0x14);
 std::ifstream fixtures(argv[4]);check(bool(fixtures),"Original footstep fixture missing");
 int material,variant;unsigned kind;std::string a,c,d;unsigned comparisons=0;
 while(fixtures>>material>>kind>>variant>>a>>c>>d){const auto result=authored_footstep_sounds(cache,foot,std::int16_t(material),kind,variant);
  check(result==std::array<std::uint32_t,3>{std::uint32_t(std::stoul(a,nullptr,16)),std::uint32_t(std::stoul(c,nullptr,16)),std::uint32_t(std::stoul(d,nullptr,16))},"Whole original footstep sound selection mismatch");++comparisons;}
 check(comparisons==154,"Footstep original fixture coverage missing");
 const auto fp_graph=content.weapons.at(weapon).first_person_models.front().animation;
 const auto fp_tag=cache.load_source_tag_metadata(fp_graph);
 const Bytes fp_root(fp_tag.bytes_at(fp_tag.source_address(),0x34));
 unsigned fp_float_channels=0;
 for(unsigned i=0;i<fp_root.u32(0x2c);++i)
  fp_float_channels+=verify_float_tracks(cache,cache.load_animation(fp_graph,i));
 check(fp_float_channels>0,"Real codec8 component differential was not exercised");
 std::cout<<"original codec8 first-person component comparisons="<<fp_float_channels<<'\n';
 for(unsigned representation:{2u,3u}){
 const auto bipd=cache.load_source_tag_metadata(content.player_representations[representation].biped.datum);
 const Bytes b(bipd.bytes_at(bipd.source_address(),0x304));const auto foot=b.u32(0x58);
 const auto model=cache.load_source_tag_metadata(b.u32(0x38));const Bytes m(model.bytes_at(model.source_address(),0x94));
 const auto graph=m.u32(0x14);const auto ids=cache.load_weapon_animation_identifiers(weapon);
 NativeLocomotion gait(cache,graph,weapon,{0x12345678,0x23456789});
 gait.bind_facing(content.player_representations[representation].biped.datum,0);
 unsigned action_index=0;
 constexpr std::array<std::uint32_t,5> actions{0x0400000c,0x0a000017,0x09000016,0x0a000014,0x09000015};
 for(const auto &v:std::array<std::array<float,3>,5>{{{0,0,185},{1,0,189},{-1,0,188},{0,-1,190},{0,1,191}}}){
  const auto authored=cache.load_unit_weapon_action(graph,0x06000086,ids[0],ids[1],actions[action_index++]);
  gait.tick(v[0],v[1],true,false,{2,0,0},{});
  check(gait.frame().clip->source_graph==authored.source_graph,"Gait lost authored inherited graph");
  if(representation==2)check(gait.frame().clip->source_index==unsigned(v[2]),"Authored original gait selection mismatch");
  if(authored.source_graph!=graph)check(Bytes(gait.manager()).u16(4)!=0xffff,"Inherited gait lost original packed binding");
  if(v[0]||v[1])check(gait.frame().frame==0,"New authored gait must begin at source frame zero");}
 // E5700 requests the authored airborne activity, including while moving.
 gait.tick(1,0,false,false,{0,0,3},{});
 check(gait.frame().activity==0x0800001e,"Jumping player incorrectly plays idle instead of airborne");
 const auto airborne=cache.load_unit_weapon_action(graph,0x06000086,ids[0],ids[1],0x0800001e);
 check(gait.frame().clip->source_graph==airborne.source_graph &&
       gait.frame().clip->source_index==airborne.source_index,"Jump lost authored airborne binding");
 gait.tick(1,0,true,true,{2,0,0},{});
 const auto crouched=cache.load_unit_weapon_action(graph,0x06000087,ids[0],ids[1],0x0a000014);
 check(gait.frame().clip->source_graph==crouched.source_graph &&
       gait.frame().clip->source_index==crouched.source_index,"Crouch walk incorrectly plays standing run");
 const auto render_model=cache.load_model(content.player_representations[representation].biped.datum,false);
 NativePlayerAnimation smg_pose(cache,graph,weapon);
 const auto *smg_idle=smg_pose.action(0x0400000cu,false,false);
 const auto *smg_grip=smg_pose.action(0x0400060bu,false,false);
 const auto expected_idle=cache.load_unit_weapon_action(graph,0x06000086u,ids[0],ids[1],0x0400000cu);
 check(smg_idle && smg_idle->source_graph==expected_idle.source_graph && smg_idle->source_index==expected_idle.source_index,
     "Pose action lookup lost the authored primary bank fallback");
 check(smg_idle->source_index==(representation==2?185u:156u),"SMG selected the wrong species-specific base stance");
 check(smg_grip && smg_grip->source_index==(representation==2?213u:201u),
     "SMG body selected the wrong species-specific weapon grip");
 for(float angle:{-1.5707963268f,0.f,1.5707963268f}) {
  auto expected=frontend_model_locals(render_model,*smg_idle,0,{},nullptr,true);
  // The actual standing SMG screens have one +/-90-degree column and row.
  // At grid endpoints no host interpolation can conceal a missing channel.
  const float column=angle<0?0.f:angle>0?2.f:1.f;
  for(auto activity:{0x0c000026u,0x0e000028u}) {
   const auto *screen=smg_pose.action(activity,false,false);check(screen,"Standing SMG aim screen missing");
   apply_native_player_overlay(render_model,expected,*screen,column+3);
  }
  apply_native_player_overlay(render_model,expected,*smg_grip,0);
  const auto actual=smg_pose.pose(render_model,*smg_idle,0,0,false,false,false,nullptr,0,nullptr,0,angle);
  for(unsigned n=0;n<expected.size();++n)
   check(actual[n].rotation==expected[n].rotation && actual[n].scale==expected[n].scale &&
       actual[n].translation.x==expected[n].translation.x && actual[n].translation.y==expected[n].translation.y &&
       actual[n].translation.z==expected[n].translation.z,"SMG stance omitted or reordered its authored aim/grip channels");
 }
 std::vector<unsigned> legs;
 for(unsigned n=0;n<render_model.bones.size();++n) {
  const auto name=cache.string_id(render_model.bones[n].name);
  if(name.ends_with("thigh") || name.ends_with("calf") || name.ends_with("foot"))legs.push_back(n);
 }
 check(legs.size()>=6,"Actual body model lost its authored leg nodes");
 for(bool crouch:{false,true})for(float target:{1.8f,-1.8f}) {
  NativeLocomotion turning(cache,graph,weapon,{0x12345678,0x23456789});
  turning.bind_facing(content.player_representations[representation].biped.datum,0);
  std::set<unsigned> turn_frames;bool changed_heading=false,returned_idle=false;
  for(unsigned tick=0;tick<90;++tick) {
   turning.tick(0,0,true,crouch,{}, {},nullptr,&target);
   const auto frame=turning.frame();
   if(frame.activity==(target>0?0x0900000eu:0x0a00000fu))turn_frames.insert(unsigned(frame.frame));
   changed_heading|=std::abs(turning.facing())>.2f;
   if(changed_heading && frame.activity==0x0400000cu)returned_idle=true;
  }
  if(!(turn_frames.size()>1 && changed_heading && returned_idle))
   std::cerr<<"Turn representation="<<representation<<" crouch="<<crouch<<" target="<<target
       <<" frames="<<turn_frames.size()<<" facing="<<turning.facing()<<" returned_idle="<<returned_idle
       <<" activity="<<std::hex<<turning.frame().activity<<std::dec<<'\n';
  check(turn_frames.size()>1 && changed_heading && returned_idle,
      "Stationary left/right turn failed to animate, rotate the body and recover to idle");
 }
 // A descending capsule can gain support through the solver's short floor
 // probe without penetrating a triangle. That contact must still deliver its
 // incoming speed to E5960 and select the authored landing channel.
 const auto &biped_definition=content.player_representations[representation].biped;
 CollisionWorld support_world({{{-10,-10,0},{10,-10,0},{10,10,0},0,0},
                               {{-10,-10,0},{10,10,0},{-10,10,0},0,0}});
 for(const float impact:{(biped_definition.landing.soft_speed+biped_definition.landing.hard_speed)*.5f,
                         (biped_definition.landing.hard_speed+biped_definition.landing.maximum_speed)*.5f}) {
  Body contact{{0,0,impact/30.f+.003f},{0,0,-impact},biped_definition.height,biped_definition.radius,false};
  support_world.move(contact,1.f/30.f);
  check(contact.grounded,"Landing fixture did not reach the native floor support probe");
  check(std::abs(contact.landing_speed-impact)<.0001f,
      "Floor support contact lost the incoming speed and skipped its landing animation");
  SourceLandingState expected,actual;bool forced=false;
  begin_source_landing(expected,biped_definition.landing,impact,30,forced);
  // Multiplayer bipeds intentionally author zero hard-landing recovery time.
  // Preserve that omission rather than inventing a duration for the clip.
  begin_source_landing(actual,biped_definition.landing,contact.landing_speed,30,forced);
  check(actual.kind==expected.kind && actual.duration==expected.duration,
      "Floor support contact lost the incoming speed and skipped its landing animation");
  NativeLocomotion contact_gait(cache,graph,weapon,{0x12345678,0x23456789});
  contact_gait.tick(0,0,false,false,{0,0,-impact},{});
  contact_gait.tick(0,0,true,false,contact.velocity,{},&actual);
  check(contact_gait.frame().activity==source_landing_activity(expected,0x0400000c),
      "Actual support contact did not reach its authored body landing channel");
 }
 unsigned body_cases=0,float_tracks=0;std::set<std::pair<std::uint32_t,unsigned>> verified;
 const auto differs=[](const auto &a,const auto &b){
   for(std::size_t i=0;i<a.size();++i)if(a[i].rotation!=b[i].rotation ||
       a[i].translation.x!=b[i].translation.x || a[i].translation.y!=b[i].translation.y ||
       a[i].translation.z!=b[i].translation.z || a[i].scale!=b[i].scale)return true;
   return false;
 };
 for(const auto &[datum,definition]:content.weapons)for(bool dual:{false,true}) {
  if(definition.first_person_models.empty() || definition.first_person_models.front().model==null_datum ||
      definition.first_person_models.front().animation==null_datum)continue;
  if(dual&&!definition.dual_wieldable)continue;
  NativeLocomotion cycle(cache,graph,datum,{0x12345678,0x23456789},dual);
  NativePlayerAnimation body(cache,graph,datum);
  for(bool crouch:{false,true}) {
   for(const auto axes:std::array<std::array<float,2>,4>{{{1,0},{-1,0},{0,-1},{0,1}}}) {
    cycle.tick(axes[0],axes[1],true,crouch,{2,0,0},{});
    const auto start=cycle.frame();
    const auto first=body.pose(render_model,*start.clip,start.frame,0,crouch,dual,true);
    std::set<unsigned> selected_frames;bool animated_legs=false,looped=false;float previous_frame=start.frame;
    for(unsigned tick=0;tick<90;++tick){
     cycle.tick(axes[0],axes[1],true,crouch,{2,0,0},{});
     const auto frame=cycle.frame();selected_frames.insert(unsigned(frame.frame));
     looped|=frame.frame<previous_frame;previous_frame=frame.frame;
     const auto posed=body.pose(render_model,*frame.clip,frame.frame,0,crouch,dual,true);
     for(auto n:legs)animated_legs|=posed[n].rotation!=first[n].rotation ||
         posed[n].translation.x!=first[n].translation.x || posed[n].translation.y!=first[n].translation.y ||
         posed[n].translation.z!=first[n].translation.z;
    }
    check(selected_frames.size()>5 && looped,"Directional walk/sneak cycle did not advance and loop");
    check(animated_legs,"Directional walk/sneak clock advanced without animating the body legs");
   }
   cycle.tick(1,0,true,crouch,{2,0,0},{});
   const auto start=cycle.frame();
   cycle.tick(1,0,false,crouch,{0,0,3},{});
   check(cycle.frame().activity==0x0800001e,"Weapon/crouch/dual jump fell back to idle");
   const auto takeoff=cycle.frame();
   const auto takeoff_locals=body.pose(render_model,*takeoff.clip,takeoff.frame,0,crouch,dual,false);
   std::set<unsigned> air_frames;bool airborne_pose_changed=false;
   for(unsigned tick=0;tick<16;++tick) {
    cycle.tick(1,0,false,crouch,{0,0,tick<8?2.f:-2.f},[&](const auto&){throw FormatError("Airborne player emitted a footstep");});
    const auto frame=cycle.frame();air_frames.insert(unsigned(frame.frame));
    airborne_pose_changed|=differs(takeoff_locals,body.pose(render_model,*frame.clip,frame.frame,0,crouch,dual,false));
   }
   check(air_frames.size()>8 && airborne_pose_changed,"Jump clock advanced without animating its authored body pose");
   for(std::int16_t kind:{std::int16_t(0),std::int16_t(1)}) {
    const SourceLandingState landing{kind,0,8};
    cycle.tick(0,0,true,crouch,{}, {},&landing);
    check(cycle.frame().activity==(kind==0?0x0900001fu:0x09000020u),"Authored weapon/stance landing channel fell back to idle");
    const auto landed=cycle.frame();
    const auto locals=body.pose(render_model,*landed.clip,0,0,crouch,dual,false);
    for(const auto &bone:locals)check(std::isfinite(bone.rotation[0]),"Authored landing resource produced a nonfinite pose");
    if(landed.clip->source_index==139 && representation==2)
      check(landed.clip->frame_count==29 && landed.clip->loop_frame==30,"Loader rewrote the authored hard landing loop marker");
    bool landing_pose_changed=false;auto recovery=landing;
    for(unsigned tick=0;tick<8;++tick) {
     advance_source_landing(recovery,false);
     cycle.tick(0,0,true,crouch,{}, {},&recovery);
     const auto frame=cycle.frame();
     if(recovery.kind!=-1)landing_pose_changed|=differs(locals,
         body.pose(render_model,*frame.clip,frame.frame,0,crouch,dual,false));
    }
    check(landing_pose_changed,"Landing clock advanced without animating its authored body pose");
    check(cycle.frame().activity==0x0400000c,"Landing recovery did not return the body to idle");
   }
   const auto flat=body.pose(render_model,*start.clip,0,0,crouch,dual,true);
   const auto up=body.pose(render_model,*start.clip,0,1,crouch,dual,true);
   const auto down=body.pose(render_model,*start.clip,0,-1,crouch,dual,true);
   check(differs(flat,up)&&differs(flat,down)&&differs(up,down),"Player aim pitch never reaches body bones");
   const auto left=body.pose(render_model,*start.clip,0,0,crouch,dual,true,nullptr,0,nullptr,0,-.5f);
   const auto right=body.pose(render_model,*start.clip,0,0,crouch,dual,true,nullptr,0,nullptr,0,.5f);
   if(datum==weapon && !(differs(flat,left)&&differs(flat,right)&&differs(left,right)))
    std::cerr<<"Aim yaw representation="<<representation<<" weapon="<<std::hex<<datum<<std::dec<<" crouch="<<crouch<<" dual="<<dual<<'\n';
   if(datum==weapon)check(differs(flat,left)&&differs(flat,right)&&differs(left,right),"Player aim yaw never reaches body bones");
   for(auto activity:{0x0400060bu,0x06000006u,0x08000002u,0x05000024u,0x08000025u,0x0e000607u,0x0d000021u,0x1000006cu,0x0b00053eu,0x0d00053au})if(const auto *action=body.action(activity,crouch,dual)) {
    if(verified.emplace(action->source_graph,action->source_index).second)float_tracks+=verify_float_tracks(cache,*action);
    bool changed=false;
    for(unsigned frame=0;frame<action->frame_count;++frame) {
     const auto posed=body.pose(render_model,*start.clip,0,0,crouch,dual,true,action,float(frame));
     changed|=differs(flat,posed);
     for(const auto &bone:posed)check(std::isfinite(bone.scale)&&std::isfinite(bone.rotation[0]),"Action produced nonfinite body pose");
     for(std::size_t n=0;n<render_model.bones.size();++n) {
      const auto found=std::find(action->nodes.begin(),action->nodes.end(),render_model.bones[n].name);
      const auto mask=found==action->nodes.end()?0:action->components[std::size_t(found-action->nodes.begin())];
      if(!(mask&1))check(posed[n].rotation==flat[n].rotation,"Action replaced an unauthored bone rotation");
     }
    }
    if(activity!=0x0400060bu)check(changed,"Authored fire/reload action never reaches body pose");
   }
   ++body_cases;
  }
 }
 std::cout<<"authored body animation combinations: representation="<<representation<<" cases="<<body_cases<<" original_codec8_channels="<<float_tracks<<'\n';
 SdlSoundPlatformConfiguration platform{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
 unsigned footsteps=0;MultiplayerAudio audio(cache,0x10293847,platform,[&](const auto&s){if(s.kind==MultiplayerAudioKind::Footstep)++footsteps;});
 for(unsigned views: {1u,4u}){
  SlayerRules rules;rules.weapon=weapon;rules.time_ticks=0;rules.score_limit=0;
  MultiplayerMatch match(content,0,cache.spawns(),rules);std::array<Controls,16> controls{};
  for(unsigned i=0;i<views;++i){std::array<std::uint8_t,9> appearance{10,0,0,10,std::uint8_t(representation-2),0,0,0,0};
   const auto name="Walking"+std::to_string(i);match.join(name,-1,native_selected_player_configuration(name,appearance,-1));}
  NativeLocomotion live(cache,graph,weapon,{0x12345678,0x23456789});
  std::set<unsigned> frames;unsigned contacts=0,events=0;
  for(unsigned tick=0;tick<120;++tick){controls[0].forward=tick>=5&&tick<95?1.f:0.f;match.step(controls);const auto &u=match.unit(0);
   live.tick(controls[0].forward,0,u.body.grounded,false,u.body.velocity,[&](const auto&e){++events;
    if(!u.body.support_triangle)return;const auto tri=*u.body.support_triangle;
    if(tri>=content.collision.size())return;const auto &t=match.simulation().collision().triangles()[tri];
    check(match.state().players[0].native_support_material==t.material,
        "Snapshot changed the actual global support material");
    if(t.material==0xffff)return;++contacts;
    for(auto sound:authored_footstep_sounds(cache,foot,std::bit_cast<std::int16_t>(t.material),e.kind,int(std::to_integer<unsigned>(b.slice(0x1a,1)[0]))))
      if(sound!=null_datum)audio.impulse(sound,1,MultiplayerAudioKind::Footstep,u.source_datum,0);
   });
   frames.insert(unsigned(live.frame().frame));const auto p=live.sample(.5f);check(p.clip&&std::isfinite(p.frame),"Gait interpolation lost decoded clip");
   audio.render(1600);audio.update();}
  check(events>2&&contacts>2&&frames.size()>8&&footsteps>2,"Native grounded walking failed authored timing/material playback");
  const auto before=footsteps;for(unsigned tick=0;tick<45;++tick){live.tick(0,0,true,false,{},[&](const auto&){throw FormatError("Idle invented footstep cadence");});audio.render(1600);audio.update();}
  check(before==footsteps,"Stopped walking replayed footstep sounds");
  std::cout<<"actual native walking: representation="<<representation<<" views="<<views<<" source_timing_events="<<events<<" ground_contacts="<<contacts<<" SDL_footstep_packets="<<footsteps<<'\n';
 }
 }
 std::cout<<"PASS 154 original selections, five authored gaits, actual grounded walking and real SDL footstep submissions\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
