#include "halo2/mcc_script_sound_world_access.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cfenv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace {
using namespace halo2;
void need(bool yes,const char* why){if(!yes)throw FormatError(why);}
template<class F>void rejects(F&& f,const char* why){bool bad=false;try{f();}catch(const std::exception&){bad=true;}need(bad,why);}
std::uint32_t advance(std::uint32_t s){return s*0x19660du+0x3c6ef35fu;}
std::array<std::byte,4> bytes(std::uint32_t n){std::array<std::byte,4> b;for(unsigned i=0;i<4;++i)b[i]=std::byte(n>>(8*i));return b;}
void patch(SourceRuntimeMemory::Checkpoint& checkpoint,std::uint32_t at,std::span<const std::byte> value){
  for(auto& region:checkpoint.regions)if(at>=region.address&&std::uint64_t(at)-region.address+value.size()<=region.bytes.size()){
    std::ranges::copy(value,region.bytes.begin()+(at-region.address));return;}
  throw FormatError("Expected patch escapes actual checkpoint owner");
}
struct Fixture {TagHandle sound;std::string name;unsigned pitch{};};
}
int main(int argc,char** argv)try{
  if(argc!=3||!std::filesystem::is_regular_file(argv[1]))return 77;
  std::filesystem::create_directories(argv[2]);const Cache cache(argv[1]);
  auto world=std::make_unique<PhysicsLiveWorldTu15>(cache,argv[2]);world->initialize_map();world->configure_session(source_game_variant_default(0),1,30,0x12345678u);
  auto access=world->script_sound_access(cache);auto provider=world->sound_variation_provider(cache);auto& memory=world->memory();
  const auto word=[&](unsigned p){return Bytes(memory.view(p,4)).u32(0);};const auto auxiliary=word(0x4f3cd8u)+4,gestalt=word(0x52d664u);
  const auto location=[&](TagHandle sound,unsigned pitch){const auto root=word(word(0x4f0414u)+unsigned(sound.index())*16+8);const auto first=std::bit_cast<std::int16_t>(Bytes(memory.view(root,20)).u16(8));return word(gestalt+36)+(unsigned(first)+pitch)*12;};
  std::map<std::string,Fixture> fixtures;std::optional<Fixture> ff;
  for(const auto& tag:cache.tags())if(tag.group=="snd!"&&tag.address){try{
    const auto d=provider.load(cache,tag.handle());const auto count=d.permutations.size();const bool persistent=!(Bytes(d.sound_class).u16(10)&0x20);
    if(persistent&&count==1&&!fixtures.contains("single"))fixtures.emplace("single",Fixture{tag.handle(),tag.name});
    if(persistent&&count>1&&!fixtures.contains("persistent"))fixtures.emplace("persistent",Fixture{tag.handle(),tag.name});
    if(!persistent&&count>1&&!fixtures.contains("nonpersistent"))fixtures.emplace("nonpersistent",Fixture{tag.handle(),tag.name});
    if(count==32&&!fixtures.contains("count32"))fixtures.emplace("count32",Fixture{tag.handle(),tag.name});
    if(d.pitch_ranges.size()>1&&!ff)ff=Fixture{tag.handle(),tag.name};
  }catch(const FormatError&) {}}
  need(fixtures.contains("single")&&fixtures.contains("persistent")&&fixtures.contains("nonpersistent")&&ff,
       "Actual native bank lacks required first-permutation witnesses");
  nlohmann::json out{{"map",cache.name()},{"cases",nlohmann::json::array()},{"groups",nlohmann::json::array()}};
  const auto pass=[&](const char* s){out["groups"].push_back(s);std::cerr<<"PASS "<<s<<'\n';};
  {SourceScriptSoundClockSelectionTu15 selection;access.select(cache,ff->sound,selection);need(selection==SourceScriptSoundClockSelectionTu15{},"Multi-pitch actual213410 did not leave FF selectors");out["genuine_213410_ff"]={{"sound",ff->sound.value},{"name",ff->name},{"pitch_ranges",provider.load(cache,ff->sound).pitch_ranges.size()},{"range_address",location(ff->sound,0)}};}
  const auto run=[&](const std::string& kind,const Fixture& f,bool force){
    const auto address=location(f.sound,f.pitch);if(force)world->write_source_call_input(address+5,std::array{std::byte{0}});
    const auto d=provider.load(cache,f.sound,f.pitch);const auto original=imported_sound_range_state(d);auto state=original;const auto old=word(auxiliary),post=advance(advance(old));SoundLocalRandom random(post);
    const auto expected=source_sound_permutation(d,state,random);auto checkpoint=memory.checkpoint();const auto used=Bytes(d.range).u16(6)>>3;const auto used_address=word(gestalt+60)+used;
    patch(checkpoint,auxiliary,bytes(random.seed));const bool persistent=!(Bytes(d.sound_class).u16(10)&0x20);
    if(persistent){if(original.forced!=-1)patch(checkpoint,address+5,std::array{std::byte(std::uint8_t(state.forced))});else if(!d.used.empty())patch(checkpoint,used_address,std::span<const std::byte>(bytes(state.used)).first(d.used.size()));patch(checkpoint,address+4,std::array{std::byte(std::uint8_t(state.last))});}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::fenv_t environment;const auto host_mxcsr=_mm_getcsr();const bool fp_control=kind=="single";
    if(fp_control){need(std::fegetenv(&environment)==0,"Caller FP environment unavailable");std::fesetround(FE_DOWNWARD);std::feraiseexcept(FE_INVALID|FE_INEXACT);}
    const auto before_exceptions=std::fetestexcept(FE_ALL_EXCEPT);const auto before_round=std::fegetround();const auto before_mxcsr=_mm_getcsr();std::uint16_t before_cw;asm volatile("fnstcw %0":"=m"(before_cw));
#endif
    const auto actual=access.first_permutation_after_pitch(cache,{f.sound,std::uint16_t(f.pitch),old,post});
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(fp_control){std::uint16_t after_cw;asm volatile("fnstcw %0":"=m"(after_cw));const bool equal=before_round==std::fegetround()&&before_mxcsr==_mm_getcsr()&&before_cw==after_cw&&before_exceptions==std::fetestexcept(FE_ALL_EXCEPT);std::fesetenv(&environment);_mm_setcsr(host_mxcsr);need(equal,"First permutation leaked source FP environment into caller");}
#endif
    need(actual.relative_permutation==expected&&actual.auxiliary_seed_after==random.seed&&word(auxiliary)==random.seed,"Typed first permutation differs source kernel result/RNG");
    need(memory.checkpoint()==checkpoint,"First permutation changed bytes outside exact native Aux/range/used effects");
    const auto after=provider.load(cache,f.sound,f.pitch);const auto after_state=imported_sound_range_state(after);
    need(after_state.last==state.last&&after_state.forced==state.forced&&after_state.used==state.used,"Typed first permutation lost native state publication");
    std::vector<unsigned> chances;for(const auto& p:d.permutations)chances.push_back(Bytes(p).u16(2));
    out["cases"].push_back({{"kind",kind},{"sound",f.sound.value},{"name",f.name},{"relative_pitch",f.pitch},{"range_address",address},{"used_address",used_address},{"used_bytes",d.used.size()},
      {"count",d.permutations.size()},{"class_flags10",Bytes(d.sound_class).u16(10)},{"last",int(original.last)},{"forced",int(original.forced)},{"used",original.used},{"seed",post},{"creation_before",old},{"chances",chances},
      {"expected",{{"selected",actual.relative_permutation},{"last",int(after_state.last)},{"forced",int(after_state.forced)},{"used",after_state.used},{"seed",actual.auxiliary_seed_after}}},{"whole_bank_exact",true}});
  };
  for(const auto& [kind,f]:fixtures){run(kind,f,false);run(kind+"-fresh-repeat",f,false);}
  run("persistent-forced",fixtures.at("persistent"),true);run("genuine-FF-multipitch",*ff,false);
  pass("actual single,persistent,nonpersistent,optionalcount32,repeatedfreshstate,forced and genuineFF witnesses; whole bank exact");
  {std::ofstream captured(std::filesystem::path(argv[2])/"first-permutation-cases.json");need(bool(captured),"Could not preserve actual source tuple witnesses");captured<<out.dump(2)<<'\n';}
  const auto f=fixtures.at("persistent");const auto address=location(f.sound,f.pitch);const auto stable=memory.checkpoint();unsigned stores=0;int observer{};
  source_runtime_register_write_observer(memory,&observer,[&](auto,auto){++stores;});
  const auto request=[&]{const auto seed=word(auxiliary);return MccScriptSoundFirstPermutationInput{f.sound,0,seed,advance(advance(seed))};};
  {auto q=request();q.post_gain_pitch_seed^=1;rejects([&]{access.first_permutation_after_pitch(cache,q);},"Invalid creation phase admitted");q=request();q.expected_auxiliary_seed^=1;q.post_gain_pitch_seed=advance(advance(q.expected_auxiliary_seed));rejects([&]{access.first_permutation_after_pitch(cache,q);},"Stale Aux preimage admitted");q=request();q.sound.value^=0x10000;rejects([&]{access.first_permutation_after_pitch(cache,q);},"Foreign full-salt sound admitted");q=request();q.relative_pitch=0xffff;rejects([&]{access.first_permutation_after_pitch(cache,q);},"Foreign relative pitch admitted");}
  need(stores==0&&memory.checkpoint()==stable,"Precondition rejection wrote native state");
  {int guard{};source_runtime_register_write_guard(memory,&guard,[&](auto at,auto size){if(at==address+4&&size==1)throw FormatError("Declared narrow range denial");});rejects([&]{access.first_permutation_after_pitch(cache,request());},"Narrow range deny admitted");source_runtime_unregister_write_guard(memory,&guard);need(stores==0&&memory.checkpoint()==stable,"Narrow denial followed an earlier facade write");}
  pass("wrong phase,seed,salt,pitch and narrow deny reject before any facade write");
  {int guard{};bool changed=false;source_runtime_register_write_guard(memory,&guard,[&](auto at,auto size){if(at==auxiliary&&size==4&&!changed){changed=true;world->write_source_call_input(auxiliary,bytes(word(auxiliary)^1));}});const auto old=word(auxiliary);rejects([&]{access.first_permutation_after_pitch(cache,{f.sound,0,old,advance(advance(old))});},"Mutable guard changed seed without rejection");source_runtime_unregister_write_guard(memory,&guard);need(changed&&stores==1&&word(auxiliary)==(old^1),"Seed recheck lost external change or emitted facade writes");world->write_source_call_input(auxiliary,bytes(old));}
  source_runtime_unregister_write_observer(memory,&observer);need(memory.checkpoint()==stable,"External guard control cleanup differs");pass("postguard complete beforeimage recheck retains external write and emits no facade write");
  {const auto d=provider.load(cache,f.sound);const auto root=word(word(0x4f0414u)+unsigned(f.sound.index())*16+8);
    const auto corrupt=[&](unsigned at,std::vector<std::byte> after){const auto before=std::vector<std::byte>(memory.view(at,after.size()).begin(),memory.view(at,after.size()).end());world->write_source_call_input(at,after);const auto changed=memory.checkpoint();unsigned writes=0;int owner{};source_runtime_register_write_observer(memory,&owner,[&](auto,auto){++writes;});rejects([&]{access.first_permutation_after_pitch(cache,request());},"Corrupted owner/metadata admitted");source_runtime_unregister_write_observer(memory,&owner);need(writes==0&&memory.checkpoint()==changed,"Corrupted owner rejection emitted facade writes");world->write_source_call_input(at,before);access.require_current(cache);};
    const auto alias=bytes(auxiliary);corrupt(gestalt+60,{alias.begin(),alias.end()});
    const auto wrong_aux=bytes(word(0x4f3cd8u)+4);corrupt(0x4f3cd8u,{wrong_aux.begin(),wrong_aux.end()});
    corrupt(root,{d.sound[0]^std::byte{1}});
    const auto first=Bytes(d.range).u16(8);const auto total=word(gestalt+40);need(unsigned(first)+d.permutations.size()+1<total,"Need in-bounds permutation remap fixture");corrupt(address+8,{std::byte((first+1)&255),std::byte((first+1)>>8)});
    const auto permutation=word(gestalt+44)+unsigned(first)*16;corrupt(permutation+2,{d.permutations[0][2]^std::byte{1}});
    need(memory.checkpoint()==stable,"Owner corruption cleanup changed complete bank");
  }
  {const auto d=provider.load(cache,f.sound);const auto used_address=word(gestalt+60)+(Bytes(d.range).u16(6)>>3);
    for(const auto at:std::array{address+4,used_address}){const auto before=memory.view(at,1)[0];bool changed=false;unsigned writes=0;int guard{},owner{};
      source_runtime_register_write_observer(memory,&owner,[&](auto,auto){++writes;});source_runtime_register_write_guard(memory,&guard,[&](auto p,auto size){if(p==auxiliary&&size==4&&!changed){changed=true;world->write_source_call_input(at,std::array{before^std::byte{1}});}});
      rejects([&]{access.first_permutation_after_pitch(cache,request());},"Mutable range/used changed without preimage rejection");source_runtime_unregister_write_guard(memory,&guard);source_runtime_unregister_write_observer(memory,&owner);need(changed&&writes==1,"Range/used recheck emitted facade writes or lost external store");world->write_source_call_input(at,std::array{before});}
    need(memory.checkpoint()==stable,"Mutable range/used diagnostic cleanup changed whole bank");
  }
  pass("typed Aux/gestalt/packed-used alias,root and in-bounds permutation mapping guards; postguard range/used beforeimages");
  {const Cache foreign(argv[1]);rejects([&]{access.first_permutation_after_pitch(foreign,request());},"Foreign Cache admitted");const auto q=request();unsigned denied=0;std::thread thread([&]{try{access.first_permutation_after_pitch(cache,q);}catch(const FormatError&){++denied;}});thread.join();need(denied==1,"Foreign thread touched World");world->inspect_render_memory([&](const auto&,const auto&){rejects([&]{access.first_permutation_after_pitch(cache,q);},"Render borrow admitted first permutation");});}
  {int owner{};unsigned reentry=0;source_runtime_register_write_observer(memory,&owner,[&](auto at,auto){if(at==auxiliary){rejects([&]{access.first_permutation_after_pitch(cache,request());},"Observer reentered first permutation");++reentry;}});access.first_permutation_after_pitch(cache,request());source_runtime_unregister_write_observer(memory,&owner);need(reentry==1,"Default World observer did not run");}
  pass("foreign Cache/thread,actual read borrow and observer reentry boundaries");
  {auto distinct=world->script_sound_access(cache);unsigned guarded=0;int guard{};
    source_runtime_register_write_guard(memory,&guard,[&](auto at,auto size){if(at==auxiliary&&size==4){rejects([&]{distinct.first_permutation_after_pitch(cache,request());},"Distinct capability bypassed actual-owner operation gate");rejects([&]{world->script_sound_access(cache);},"Fresh factory bypassed actual-owner operation gate");++guarded;}});
    access.first_permutation_after_pitch(cache,request());source_runtime_unregister_write_guard(memory,&guard);need(guarded==2,"Cross-capability guard did not cover mutable admission and publication");
  }
  {auto removable=std::make_unique<MccScriptSoundWorldAccess>(world->script_sound_access(cache));const auto* raw=removable.get();bool removed=false;int guard{};
    source_runtime_register_write_guard(memory,&guard,[&](auto at,auto size){if(at==auxiliary&&size==4&&!removed){removed=true;removable.reset();}});
    const auto result=raw->first_permutation_after_pitch(cache,request());source_runtime_unregister_write_guard(memory,&guard);need(removed&&!removable&&word(auxiliary)==result.auxiliary_seed_after,"Permission callback destroyed an unretained capability/operation gate");
  }
  pass("distinct/fresh capability owner-level reentry rejected and last public wrapper removal retains strong in-flight state");
  {world->write_source_call_input(address+5,std::array{std::byte{0}});const auto saved=world->authority_checkpoint();const auto old=word(auxiliary);int guard{};unsigned attempts=0;
    source_runtime_register_write_guard(memory,&guard,[&](auto at,auto size){if(at==address+4&&size==1&&++attempts==2)throw FormatError("Declared later range publication failure");});
    rejects([&]{access.first_permutation_after_pitch(cache,{f.sound,0,old,advance(advance(old))});},"Declared later publication failure missing");source_runtime_unregister_write_guard(memory,&guard);
    need(word(auxiliary)==advance(advance(old))&&memory.view(address+5,1)[0]==std::byte{0xff},"Later failure rolled back earlier Aux/forced stores");rejects([&]{access.require_current(cache);},"Partial publication did not retain World fault barrier");rejects([&]{world->script_sound_access(cache);},"Fresh access bypassed partial-publication World barrier");
    {auto prepared=world->prepare_authority(saved);need(world->commit_authority(std::move(prepared)),"Authority recovery failed");}rejects([&]{access.require_current(cache);},"Restore retained old access");access=world->script_sound_access(cache);provider=world->sound_variation_provider(cache);
  }
  pass("actual later store failure retains Aux/forced writes and World fault; authority restore retires access");
  {world->write_source_call_input(address+5,std::array{std::byte{0}});const auto saved=world->authority_checkpoint();const auto old=word(auxiliary);int owner{};
    source_runtime_register_write_observer(memory,&owner,[&](auto at,auto){if(at==address+5)throw FormatError("Declared hostile publication observer");});rejects([&]{access.first_permutation_after_pitch(cache,{f.sound,0,old,advance(advance(old))});},"Hostile publication observer did not reject");source_runtime_unregister_write_observer(memory,&owner);
    need(word(auxiliary)==advance(advance(old)),"Hostile observer rolled back completed Aux store");rejects([&]{world->script_sound_access(cache);},"Fresh access bypassed observer-fault World barrier");
    {auto prepared=world->prepare_authority(saved);need(world->commit_authority(std::move(prepared)),"Observer-fault authority recovery failed");}access=world->script_sound_access(cache);provider=world->sound_variation_provider(cache);
  }
  pass("declared hostile observer fault retains completed store and prevents fresh access until actual authority restore");
  {const auto saved=world->authority_checkpoint();const auto before=access.clock_rows(cache);auto after=before;after[0].sound=f.sound.value;after[0].start_tick_bits=access.time(cache).tick_bits;after[0].selection={0,0};after[0].predicate=0;
    const auto rows=word(0x4f9b58u)+0x128;unsigned attempts=0;int guard{};source_runtime_register_write_guard(memory,&guard,[&](auto at,auto size){if(at==rows&&size==4&&++attempts==2)throw FormatError("Declared later clock publication failure");});
    rejects([&]{access.publish_clock_rows(cache,before,after);},"Later clock metadata guard did not reject");source_runtime_unregister_write_guard(memory,&guard);
    need(word(rows)==before[0].sound&&word(rows+4)==after[0].start_tick_bits&&Bytes(memory.view(rows+12,2)).u16(0)==0,"Later clock failure rolled back partial selector/start stores");
    rejects([&]{world->script_sound_access(cache);},"Fresh access bypassed compound clock publication barrier");{auto prepared=world->prepare_authority(saved);need(world->commit_authority(std::move(prepared)),"Clock publication authority recovery failed");}access=world->script_sound_access(cache);provider=world->sound_variation_provider(cache);need(access.clock_rows(cache)==before,"Clock restore did not recover actual source metadata");
  }
  pass("existing clock metadata CAS shares compound write fault boundary; partial stores retained and actual restore required");
  {auto held=access;world.reset();rejects([&]{held.first_permutation_after_pitch(cache,{f.sound,0,0,advance(advance(0))});},"Teardown retained borrowed World access");}
  pass("teardown rejects retained access before borrowed World dereference");out["scope"]="Typed initial post-pitch FF final-state CAS only; no PCM, Audio/Main binding, literal event/FP register trace, source voice, cull/allocation-failure or complete gameplay parity claim. Forced state control is declared metadata fixture; each actual tuple exported for independent unchanged original CPU comparison.";
  std::cout<<out.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
