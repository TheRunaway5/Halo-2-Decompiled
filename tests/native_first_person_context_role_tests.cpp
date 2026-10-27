#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_first_person_context_tu15.hpp"
#include "halo2/source_first_person_pose_tu15.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_execution_inputs.hpp"
#include <algorithm>
#include <cmath>
#include <bit>
#include <cfenv>
#include <x86intrin.h>
#include <fstream>
#include <iostream>
#include <tuple>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void need(bool value,const char *message){if(!value)throw FormatError(message);}
std::uint32_t word(const SourcePhysicsContactMemory &m,std::uint32_t at) {
 std::array<std::byte,4>b;m.read(at,b);return Bytes(b).u32(0);
}
float compare(const std::vector<SourceFirstPersonModelTu15>&a,const std::vector<SourceFirstPersonModelTu15>&b) {
 need(a.size()==b.size(),"Local temporal installation changed native model count");float difference{};
 for(unsigned n=0;n<a.size();++n){need(a[n].render_model==b[n].render_model && a[n].owner==b[n].owner &&
   a[n].mode==b[n].mode && a[n].nodes.size()==b[n].nodes.size(),"Local temporal installation changed native model identity/order");
  for(unsigned i=0;i<a[n].nodes.size();++i){const auto &x=a[n].nodes[i],&y=b[n].nodes[i];
   difference=std::max(difference,length(x.position-y.position));difference=std::max(difference,std::abs(x.scale-y.scale));
   for(unsigned j=0;j<3;++j)difference=std::max(difference,length(x.basis[j]-y.basis[j]));
  }
 }
 return difference;
}
void control_word(SourceFirstPersonTemporalHandTu15 &hand,unsigned offset,std::uint32_t value) {
 for(unsigned n=0;n<4;++n)hand.control[offset+n]=std::byte(value>>(n*8));
}
std::size_t matching_map_tails(const SourceFirstPersonTemporalStateTu15 &a,
    const SourceFirstPersonTemporalStateTu15 &b) {
 std::size_t bytes{};
 for(unsigned hand=0;hand<2;++hand)for(const auto [count_at,map_at]:
     {std::pair{0xf4u,0xfcu},std::pair{0xf8u,0x1fcu}}) {
  const auto count=Bytes(a.hands[hand].control).u32(count_at);
  need(count<=64 && count==Bytes(b.hands[hand].control).u32(count_at),"Positive map-tail comparison lost its native count");
  const auto first=map_at+count*4u,last=map_at+64u*4u;
  need(std::equal(a.hands[hand].control.begin()+first,a.hands[hand].control.begin()+last,
      b.hands[hand].control.begin()+first),"Local temporal installation changed unused map-tail bytes");
  bytes+=last-first;
 }
 return bytes;
}
}
int main(int argc,char**argv)try {
 need(argc==4,"MAP SHARED REPORT");const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Run through run_test.py");
 nlohmann::json report{{"cases",nlohmann::json::array()},{"scope","Native role continuation and actual sampler MXCSR admission"}};
 std::ofstream(argv[3])<<report.dump(2)<<'\n';
 for(unsigned rep:{2u,3u})for(bool dual:{false,true}) {
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.local_players=1;options.mute=true;options.source_object_world=true;options.starting_weapon=5;
  options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};options.source_object_savegame_directory=std::filesystem::path(scratch)/(std::to_string(rep)+(dual?"-dual":"-single"));
  if(rep==3){auto profile=source_default_player_configuration();profile[0x44]=1;options.selected_profiles={profile};}
  MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);
  auto &native=world.source_objects->tu15_world();const auto &memory=native.memory();const auto slot=world.local_slots.at(0);
  const auto weapon=world.cache->slayer_starting_weapon(5);std::array<Controls,multiplayer_capacity> controls{};unsigned elapsed{};bool tested{};
  for(unsigned tick=0;tick<100 && !tested;++tick) {
   const auto before=world.match.state();const auto &previous=before.players[slot];
   if(dual && previous.source_unit!=null_datum && previous.secondary_weapon.datum==null_datum) {
    const auto pickup=std::find_if(before.world_weapons.begin(),before.world_weapons.end(),[&](const auto&w){return w.definition==weapon;});
    if(pickup!=before.world_weapons.end())native.set_object_position(previous.source_unit,pickup->position+Vec3{-.2f,0,0});
   }
   controls[slot]={};controls[slot].dual_wield=dual;controls[slot].dual_wield_milliseconds=dual?1000:0;
   if(elapsed>=8){controls[slot].yaw=.4f*std::sin(elapsed*.1f);controls[slot].pitch=.15f;controls[slot].forward=.2f;}
   world.match.step(controls);const auto state=world.match.state();const auto &player=state.players[slot];
   if(player.source_unit==null_datum || (dual && player.secondary_weapon.datum==null_datum) || ++elapsed<20)continue;
   const Vec3 eye{1,2,3},forward{1,0,0},up{0,0,1};const auto expected=native.prepare_first_person_models(0,eye,forward,up);
   need(expected.size()==(dual?3u:2u),"Actual local native authority has no completed foreground pose");
   need(player.first_person_pose.has_value(),"Role fixture lost its actual native composition packet");
   const auto sampling_inputs=player.first_person_pose->temporal;
   const auto &identity=sampling_inputs.identity;
   auto temporal=SourceFirstPersonContextTu15::capture_local(*world.cache,memory,0,identity,
       sampling_inputs.generation,sampling_inputs.accepted_action_sequence);
   need(temporal.source_tick==state.tick && sampling_inputs.source_tick+1u==state.tick,
       "Role fixture lost its actual N -> N+1 clock boundary");
   temporal.source_tick=sampling_inputs.source_tick;
   const auto caller=native.prepare_source_call();SourceRuntimeWriteSerial serial(memory);
   SourceFirstPersonContextTu15 adopted(*world.cache);adopted.adopt_local(memory,caller,0,identity,
       temporal.generation,temporal.accepted_action_sequence);
   need(serial.load()==0,"Pure native local continuation adoption wrote authority bytes");
   const auto saved=adopted.checkpoint();SourceFirstPersonContextTu15 restored(*world.cache);restored.prepare_restore(memory,caller,saved);
   need(restored.state().hands[0].control==temporal.hands[0].control && restored.state().hands[1].control==temporal.hands[1].control,
      "Adopted local continuation changed its source hand controls");
   nlohmann::json sampler_fp_cases=nlohmann::json::array();
   for(const auto incoming:{0x1f80u,0x1fa0u,0x1fa1u}) {
    auto retained_caller=caller;need(retained_caller.floating_point.has_value(),"Native sampler caller lacks FP controls");
    retained_caller.floating_point->mxcsr=incoming;
    std::uint64_t instructions{},composition_entries{},submission_entries{},estimates{},timestamps{};
    bool incoming_status_preserved=true;
    const auto status=[&] {incoming_status_preserved&=(_mm_getcsr()&(incoming&0x3fu))==(incoming&0x3fu);};
    SourcePhysicsWorldRuntimeServices cpu;
    // Observe actual executed native instruction boundaries. A no-estimate
    // authored clip still proves status propagation without inventing a CPU
    // request merely to satisfy a coverage counter.
    cpu.instruction=[&](auto pc) {++instructions;composition_entries+=pc==0x16a4ddu;
      submission_entries+=pc==0x168dedu;status();};
    cpu.reciprocal_sqrt=[&](auto bits) {++estimates;status();return std::bit_cast<std::uint32_t>(
      _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));};
    cpu.timestamp_counter=[&] {++timestamps;status();return __rdtsc();};
    std::vector<SourceFirstPersonModelTu15> sampled;
    const auto outer_mxcsr=_mm_getcsr();std::fenv_t outer_environment;
    need(std::fegetenv(&outer_environment)==0,"Native sampler test cannot retain its real outer FP environment");
    std::uint16_t outer_control{},outer_status{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnstcw %0":"=m"(outer_control));asm volatile("fnstsw %0":"=m"(outer_status));
#else
    throw FormatError("Native sampler FP regression requires actual x87 environment instructions");
#endif
    std::string sampling_error;
    try {
     restored.with_sampling_inputs(memory,retained_caller,sampling_inputs,[&](const auto &inputs) {
      sampled=source_sample_first_person_pose_tu15(*world.cache,inputs,retained_caller,0,eye,forward,up,cpu);
     },cpu);
    } catch(const std::exception &error) {sampling_error=error.what();}
    const auto returned_mxcsr=_mm_getcsr();std::uint16_t returned_control{},returned_status{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnstcw %0":"=m"(returned_control));asm volatile("fnstsw %0":"=m"(returned_status));
#endif
    // glibc fesetenv restores x87 exception flags/controls; its high SW
    // condition codes are not part of the caller's C floating environment.
    // The explicit original ABI status still belongs to the retained caller.
    const bool outer_preserved=returned_mxcsr==outer_mxcsr && returned_control==outer_control &&
        (returned_status&0x3fu)==(outer_status&0x3fu);
    const bool caller_preserved=retained_caller.floating_point->mxcsr==incoming &&
        retained_caller.x87_status==caller.x87_status;
    report["sampler_fp_boundary"]={{"representation",rep},{"dual",dual},{"incoming_mxcsr",incoming},
      {"returned_caller_mxcsr",retained_caller.floating_point->mxcsr},{"sampling_error",sampling_error},
      {"native_instructions",instructions},{"composition_entries",composition_entries},{"submission_entries",submission_entries},
      {"outer_mxcsr",outer_mxcsr},{"returned_mxcsr",returned_mxcsr},
      {"outer_x87_control",outer_control},{"returned_x87_control",returned_control},
      {"outer_x87_status",outer_status},{"returned_x87_status",returned_status},
      {"outer_preserved",outer_preserved},{"caller_preserved",caller_preserved}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';
    need(outer_preserved && caller_preserved,"Native sampler changed incoming caller or actual outer FP environment");
    need(!serial.load(),"Native sampler FP regression changed borrowed source bytes");
    const auto difference=sampling_error.empty()?compare(sampled,expected):0.f;
    sampler_fp_cases.push_back({{"incoming_mxcsr",incoming},{"error",sampling_error},{"native_instructions",instructions},
      {"composition_entries",composition_entries},{"submission_entries",submission_entries},{"native_estimates",estimates},
      {"native_timestamps",timestamps},{"incoming_status_preserved",incoming_status_preserved},
      {"caller_preserved",caller_preserved},{"outer_environment_preserved",outer_preserved},{"max_matrix_error",difference}});
    // Preserve the genuine failing case before returning RED, including the
    // exact original gate text and actual native instruction/CPU coverage.
    report["sampler_fp_cases"][std::to_string(rep)+(dual?"-dual":"-single")]=sampler_fp_cases;
    std::ofstream(argv[3])<<report.dump(2)<<'\n';
    if(!sampling_error.empty())throw FormatError(sampling_error);
    need(instructions && composition_entries==(dual?2u:1u) && submission_entries==1,
      "Native sampler FP regression missed actual composition/submission execution");
    need(incoming_status_preserved,"Native sampler discarded incoming MXCSR status bits inside a genuine native instruction");
    need(difference<=1e-6f,"Native sampler MXCSR status changed actual model matrices");
   }
   const std::array args{0u,rep};auto reset=native.prepare_source_call(args);reset.gp[0]=null_datum;native.run_actual(0x16b16au,reset);
   reset=native.prepare_source_call(args);reset.gp[0]=identity.unit;native.run_actual(0x16b16au,reset);
   const auto view=word(memory,0x4f6498u),rng=word(memory,0x4f3cd8u),clock=word(memory,0x51df54u);
   std::array<std::byte,8> rng_before;std::array<std::byte,12> clock_before;std::array<std::byte,24> resources_before;
   std::array<std::array<std::byte,6>,2> sounds_before;
   memory.read(rng,rng_before);memory.read(clock,clock_before);memory.read(0x4f0418u,resources_before);
   for(unsigned h=0;h<2;++h)memory.read(view+0xcu+h*0x1010u+0x1008u,sounds_before[h]);
   nlohmann::json map_rejections=nlohmann::json::array();
   struct MapAttack {std::string name;unsigned offset;std::uint32_t value;};
   for(unsigned hand=0;hand<2;++hand)if(identity.weapons[hand]!=null_datum) {
    const Bytes actual(temporal.hands[hand].control);std::vector<MapAttack> attacks;
    for(const auto [name,count_at,map_at]:
        {std::tuple{"weapon",0xf4u,0xfcu},std::tuple{"arms",0xf8u,0x1fcu}}) {
     const auto count=actual.u32(count_at);
     need(count>0 && count<=64,"Actual SMG role fixture has no live native mapping prefix");
     attacks.push_back({std::string(name)+"-first-live-low",map_at,actual.u32(map_at)^1u});
     attacks.push_back({std::string(name)+"-first-live-upper16",map_at,actual.u32(map_at)^0x10000u});
     if(count>1)attacks.push_back({std::string(name)+"-last-live",map_at+(count-1u)*4u,
         actual.u32(map_at+(count-1u)*4u)^1u});
     attacks.push_back({std::string(name)+"-widened-count",count_at,count+1u});
     attacks.push_back({std::string(name)+"-count65",count_at,65u});
    }
    attacks.push_back({"graph-full-datum",8u,actual.u32(8u)^0x10000u});
    attacks.push_back({"weapon-full-datum",4u,actual.u32(4u)^0x10000u});
    for(bool pre:{false,true})for(const auto &attack:attacks) {
     auto invalid_post=temporal,invalid_pre=sampling_inputs;
     control_word((pre?invalid_pre:invalid_post).hands[hand],attack.offset,attack.value);
     SourceRuntimeWriteSerial writes(memory);std::uint64_t topology{};
     source_runtime_register_topology_observer(memory,&topology,[&](auto,auto){++topology;});
     struct TopologyUnobserve {
      const SourceRuntimeMemory &memory;const void *owner;
      ~TopologyUnobserve(){source_runtime_unregister_topology_observer(memory,owner);}
     } unobserve{memory,&topology};
     SourceExecutionInputs transcript;transcript.track_calls();bool rejected{};
     std::string admission_error,transcript_error;
     {
      SourceExecutionInputBinding binding(memory,transcript);
      try {SourceFirstPersonContextTu15::install_local(*world.cache,native,0,invalid_post,invalid_pre);}
      catch(const FormatError &error){rejected=true;admission_error=error.what();}
     }
     std::size_t operands{},native_calls{};
     try {operands=transcript.finish().size();native_calls=transcript.calls().size();}
     catch(const std::exception &error){transcript_error=error.what();}
     map_rejections.push_back({{"hand",hand},{"state",pre?"PRE":"POST"},{"attack",attack.name},
         {"control_offset",attack.offset},{"rejected",rejected},{"error",admission_error},
         {"world_writes",writes.load()},{"world_topology",topology},{"native_calls",native_calls},
         {"external_operands",operands},{"transcript_error",transcript_error}});
     report["map_domain_rejections"][std::to_string(rep)+(dual?"-dual":"-single")]=map_rejections;
     std::ofstream(argv[3])<<report.dump(2)<<'\n';
     need(rejected && !admission_error.empty() && !writes.load() && !topology && !native_calls && !operands &&
         transcript_error.empty(),"Malformed live-map/scalar input was not rejected before World writes and native calls");
    }
   }
   SourceFirstPersonContextTu15::install_local(*world.cache,native,0,temporal,sampling_inputs);
   std::array<std::byte,8> rng_after;std::array<std::byte,12> clock_after;std::array<std::byte,24> resources_after;
   memory.read(rng,rng_after);memory.read(clock,clock_after);memory.read(0x4f0418u,resources_after);
   need(rng_before==rng_after && clock_before==clock_after && resources_before==resources_after,
      "Local temporal installation changed native RNG/clock/resource owner roots");
   for(unsigned h=0;h<2;++h){std::array<std::byte,6> sound;memory.read(view+0xcu+h*0x1010u+0x1008u,sound);
    need(sound==sounds_before[h],"Local temporal installation replaced native sound ownership");}
   auto continued=SourceFirstPersonContextTu15::capture_local(*world.cache,memory,0,identity,
       temporal.generation,temporal.accepted_action_sequence);
   need(continued.source_tick==state.tick,"Local temporal installation changed the actual source tick");
   continued.source_tick=temporal.source_tick;
   need(continued==temporal,"Local role installation replaced genuine completed clocks with display inputs");
   const auto original_tail_bytes=matching_map_tails(continued,temporal);
   const auto actual=native.prepare_first_person_models(0,eye,forward,up);const auto error=compare(actual,expected);
   need(error<=1e-6f,"Local temporal installation changed genuine native foreground matrices");
   // Explicit input-domain control: unused tails do not supply a pose. Their
   // declared bytes must survive installation while all live nodes remain the
   // actual native prefix. The separate unchanged registry fixture owns the
   // real SMG-to-smaller-model historical-tail reproduction.
   auto tail_post=temporal,tail_pre=sampling_inputs;unsigned changed_tails{};
   for(unsigned hand=0;hand<2;++hand)for(const auto [count_at,map_at]:
       {std::pair{0xf4u,0xfcu},std::pair{0xf8u,0x1fcu}}) {
    const auto count=Bytes(temporal.hands[hand].control).u32(count_at);
    need(count<=64,"Actual map-tail trial escaped native fixed-bank capacity");
    if(count<64) {
     const auto at=map_at+count*4u;
     control_word(tail_post.hands[hand],at,Bytes(tail_post.hands[hand].control).u32(at)^0x10000u);
     control_word(tail_pre.hands[hand],at,Bytes(tail_pre.hands[hand].control).u32(at)^0x10000u);
     ++changed_tails;
    }
   }
   need(changed_tails>0,"Actual role case has no unused map-tail input domain");
   SourceFirstPersonContextTu15::install_local(*world.cache,native,0,tail_post,tail_pre);
   auto tail_installed=SourceFirstPersonContextTu15::capture_local(*world.cache,memory,0,identity,
       temporal.generation,temporal.accepted_action_sequence);
   need(tail_installed.source_tick==state.tick,"Unused-tail input changed the native clock");
   tail_installed.source_tick=temporal.source_tick;
   need(tail_installed==tail_post,"Unused-tail input installation lost complete POST controls or saved locals");
   const auto declared_tail_bytes=matching_map_tails(tail_installed,tail_post);
   const auto tail_error=compare(native.prepare_first_person_models(0,eye,forward,up),expected);
   need(tail_error<=1e-6f,"Unused map-tail input changed actual native foreground matrices");
   SourcePhysicsWorldRuntimeServices tail_cpu;
   tail_cpu.reciprocal_sqrt=[](auto bits){return std::bit_cast<std::uint32_t>(
       _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));};
   tail_cpu.timestamp_counter=[](){return __rdtsc();};
   std::vector<SourceFirstPersonModelTu15> tail_sampled;
   {
    SourceRuntimeWriteSerial writes(memory);
    restored.with_sampling_inputs(memory,caller,tail_pre,[&](const auto &inputs) {
     tail_sampled=source_sample_first_person_pose_tu15(*world.cache,inputs,caller,0,eye,forward,up,tail_cpu);
    },tail_cpu);
    need(!writes.load(),"Unused-tail receiver input changed borrowed authority bytes");
   }
   const auto receiver_tail_error=compare(tail_sampled,expected);
   need(receiver_tail_error<=1e-6f,"Unused-tail receiver input changed actual native foreground matrices");
   SourceFirstPersonContextTu15::install_local(*world.cache,native,0,temporal,sampling_inputs);
   auto original_restored=SourceFirstPersonContextTu15::capture_local(*world.cache,memory,0,identity,
       temporal.generation,temporal.accepted_action_sequence);
   need(original_restored.source_tick==state.tick,"Original POST restore changed the source clock");
   original_restored.source_tick=temporal.source_tick;
   need(original_restored==temporal,"Declared unused-tail trial did not restore exact original POST bytes");
   memory.read(rng,rng_after);memory.read(clock,clock_after);memory.read(0x4f0418u,resources_after);
   need(rng_before==rng_after && clock_before==clock_after && resources_before==resources_after,
       "Unused-tail input trial changed native RNG/clock/resource owners");
   for(unsigned h=0;h<2;++h){std::array<std::byte,6> sound;memory.read(view+0xcu+h*0x1010u+0x1008u,sound);
    need(sound==sounds_before[h],"Unused-tail input trial changed native sound ownership");}
   report["cases"].push_back({{"representation",rep},{"dual",dual},{"models",actual.size()},{"max_matrix_error",error},{"adopt_source_writes",0},
      {"map_domain_attacks_rejected",map_rejections.size()},{"map_rejections_before_world_writes_or_native_calls",true},
      {"full_post_control_bytes_preserved",true},{"original_unused_map_tail_bytes_compared",original_tail_bytes},
      {"declared_unused_tail_dwords_changed",changed_tails},{"declared_unused_tail_bytes_compared",declared_tail_bytes},
      {"declared_unused_tail_post_bytes_preserved",true},{"declared_unused_tail_max_matrix_error",tail_error},
      {"declared_unused_tail_receiver_max_matrix_error",receiver_tail_error},{"original_post_restored_after_tail_trial",true},
      {"pre_composition_feedback_differs_from_continuation",sampling_inputs.view_feedback!=temporal.view_feedback},
      {"post_composition_clock_preserved",true},
      {"native_rng_preserved",true},{"native_clock_preserved",true},{"native_resource_roots_preserved",true},{"native_sound_owners_preserved",true}});
   tested=true;
  }
  need(tested,"Native local installation fixture did not complete");
 }
 std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
