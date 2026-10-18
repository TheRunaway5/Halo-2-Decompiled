#include "halo2/source_script_sound_admission_tu15.hpp"
#include "halo2/source_script_sound_clock_native64_tu15.hpp"
#include "halo2/kernels.hpp"
#include <atomic>
#include <bit>
#include <iostream>
#include <limits>

namespace {
using namespace halo2;
void need(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
template<class F>void rejects(F&& f,const char* why){bool no=false;try{f();}catch(const std::exception&){no=true;}need(no,why);}
LevelContent arena(){
  LevelContent c;
  c.collision={{{-100,-100,0},{100,-100,0},{100,100,0},0,0},
      {{-100,-100,0},{100,100,0},{-100,100,0},0,0}};
  c.player.datum=1;c.player.height=1;c.player.radius=.2f;c.player.crouch_height=.6f;
  c.player.camera_height=.8f;c.player.vitality={100,100,2,.1f,0};
  c.movement={2,2,2,10,1,1,1,5,2,1};return c;
}
struct Program {
  ScriptProgram p;
  std::uint32_t node(ScriptNode n){const auto i=unsigned(p.nodes.size());n.salt=std::uint16_t(0xe000u+i);p.nodes.push_back(n);return (unsigned(n.salt)<<16)|i;}
  std::uint32_t literal(ScriptValue v){return node({0,v.type,v.type,9,null_datum,0,v.bits});}
  std::uint32_t call(const std::string& name,unsigned opcode,unsigned type,std::vector<std::uint32_t> args){
    const auto at=unsigned(p.strings.size());p.strings+=name;p.strings+='\0';
    for(unsigned i=0;i<args.size();++i)p.nodes[args[i]&65535u].next=i+1<args.size()?args[i+1]:null_datum;
    const auto fn=node({0,2,2,9,args.empty()?null_datum:args.front(),at,0});
    return node({0,std::uint16_t(opcode),std::uint16_t(type),8,null_datum,0,fn});
  }
  Program(const std::string& name,unsigned opcode,std::vector<ScriptValue> args,unsigned result_type=8){
    p.globals.push_back({"result",std::uint16_t(result_type),literal({std::uint16_t(result_type),0,{}})});
    const auto target=node({0,std::uint16_t(result_type),std::uint16_t(result_type),13,null_datum,0,0});
    std::vector<std::uint32_t> children;for(const auto& a:args)children.push_back(literal(a));
    const auto query=call(name,opcode,result_type,std::move(children));
    p.scripts.push_back({"sample",0,4,call("set",4,result_type,{target,query})});
  }
};
SourceScriptSoundClockQueriesTu15 constant(std::int32_t value=7){return {
  [=](auto){return value;},[=](auto,auto){return value;},[=](auto){return value;}};}
void registry(){
  auto c=arena();Simulation sim(c,0,Spawn{});unsigned calls=0;
  need(!source_query_script_sound_stored_time_tu15(sim,0xffffffffu),"Unbound stored query did not decline");
  need(!source_query_script_sound_explicit_time_tu15(sim,0xffffffffu,-1),"Unbound explicit query did not decline");
  need(!source_query_script_sound_language_time_tu15(sim,0xffffffffu),"Unbound language query did not decline");
  for(unsigned missing=0;missing<3;++missing){auto q=constant();if(missing==0)q.stored_time={};else if(missing==1)q.explicit_time={};else q.language_time={};rejects([&]{SourceScriptSoundClockQueryBindingTu15 invalid(sim,std::move(q));},"Incomplete query bundle admitted");}
  {
    auto q=constant();q.stored_time=[&](auto sound){++calls;need(sound==0xea12089du,"Query narrowed salted sound");return INT32_MIN;};
    SourceScriptSoundClockQueryBindingTu15 bound(sim,std::move(q));
    SourceScriptSoundAdmissionBindingTu15 events(sim,[](const auto&){});
    rejects([&]{SourceScriptSoundClockQueryBindingTu15 duplicate(sim,constant());},"Duplicate query binding admitted");
    need(source_query_script_sound_stored_time_tu15(sim,0xea12089du)==INT32_MIN,"Stored query changed signed bits");
    std::atomic<unsigned> foreign=0;
    std::thread other([&]{auto no=[&](auto f){try{f();}catch(const std::runtime_error&){++foreign;}};
      no([&]{source_query_script_sound_stored_time_tu15(sim,0xea12089du);});
      no([&]{source_query_script_sound_explicit_time_tu15(sim,0xea12089du,30);});
      no([&]{source_query_script_sound_language_time_tu15(sim,0xea12089du);});});other.join();
    need(foreign==3&&calls==1,"Foreign thread declined/fell back or invoked bound callback");
  }
  std::unique_ptr<SourceScriptSoundClockQueryBindingTu15> removable;
  auto q=constant();q.stored_time=[&](auto sound){++calls;removable.reset();
    rejects([&]{source_query_script_sound_stored_time_tu15(sim,sound);},"Unbind evaded in-flight query guard");
    rejects([&]{SourceScriptSoundClockQueryBindingTu15 rebound(sim,constant());},"Rebind evaded in-flight query guard");return 19;};
  removable=std::make_unique<SourceScriptSoundClockQueryBindingTu15>(sim,std::move(q));
  need(source_query_script_sound_stored_time_tu15(sim,42)==19&&!removable,"Callback destroyed unretained service");
  need(!source_query_script_sound_stored_time_tu15(sim,42),"Removed query binding remained bound");
  {SourceScriptSoundClockQueryBindingTu15 replacement(sim,constant(23));need(source_query_script_sound_stored_time_tu15(sim,42)==23,"Unwound query prevented fresh binding");}
  bool fail=true;q=constant();q.stored_time=[&](auto){if(fail){fail=false;throw FormatError("Declared bound failure");}return 31;};
  {SourceScriptSoundClockQueryBindingTu15 bound(sim,std::move(q));rejects([&]{source_query_script_sound_stored_time_tu15(sim,42);},"Bound error was swallowed");need(source_query_script_sound_stored_time_tu15(sim,42)==31,"Exception retained query guard");}
}
struct CallableProbe {const Simulation* owner;bool armed=false,throw_copy=false;unsigned copies=0,destroyed=0;std::function<void()> on_copy;};
struct Copying {
  std::shared_ptr<CallableProbe> p;bool guarded_copy=false;
  explicit Copying(std::shared_ptr<CallableProbe> s):p(std::move(s)){}
  Copying(const Copying& other):p(other.p),guarded_copy(p->armed){if(guarded_copy){++p->copies;rejects([&]{source_query_script_sound_language_time_tu15(*p->owner,42);},"Callable copy bypassed reentry guard");if(p->on_copy)p->on_copy();if(p->throw_copy)throw FormatError("Declared callable copy failure");}}
  ~Copying(){if(guarded_copy){++p->destroyed;rejects([&]{source_query_script_sound_language_time_tu15(*p->owner,42);},"Callable destructor bypassed reentry guard");}}
  std::int32_t operator()(std::uint32_t)const{return 29;}
};
void callable_lifetime(){
  auto c=arena();Simulation sim(c,0,Spawn{});auto state=std::make_shared<CallableProbe>();state->owner=&sim;
  auto q=constant();q.stored_time=Copying(state);
  {SourceScriptSoundClockQueryBindingTu15 bound(sim,std::move(q));state->armed=true;
    need(source_query_script_sound_stored_time_tu15(sim,42)==29&&state->copies==1&&state->destroyed==1,"In-flight callable not copied/destroyed under guard");
    state->throw_copy=true;rejects([&]{source_query_script_sound_stored_time_tu15(sim,42);},"Callable copy error was swallowed");
    state->throw_copy=false;need(source_query_script_sound_stored_time_tu15(sim,42)==29,"Callable copy unwind retained query guard");state->armed=false;}
  std::unique_ptr<SourceScriptSoundClockQueryBindingTu15> removable;
  q=constant();q.stored_time=Copying(state);removable=std::make_unique<SourceScriptSoundClockQueryBindingTu15>(sim,std::move(q));
  state->armed=true;state->on_copy=[&]{removable.reset();};
  need(source_query_script_sound_stored_time_tu15(sim,42)==29&&!removable,"Callable copy destroyed an unretained service");
  need(!source_query_script_sound_stored_time_tu15(sim,42),"Copy-time removal retained a registry entry");state->armed=false;state->on_copy={};
}
void vm_dispatch(){
  // Primary459930/459944/459958: LONG(SOUND), LONG(SOUND,LONG), LONG(SOUND).
  // The corresponding MCC descriptors602/603/604 have the same tuples.
  const ScriptValue sound{32,0xea12089du,{}};
  struct Case {std::string name;unsigned opcode;std::vector<ScriptValue> args;unsigned context;std::int32_t returned;std::optional<std::int32_t> ticks;};
  const Case cases[]={
    {"sound_impulse_time",0x258,{sound},8,16777217,{}},
    {"sound_impulse_time",0x259,{sound,{8,0x80000000u,{}}},8,INT32_MIN,INT32_MIN},
    {"sound_impulse_time",0x259,{sound,{7,0x1234ffffu,{}}},8,-1,-1},
    {"sound_impulse_time",0x259,{sound,{6,std::bit_cast<std::uint32_t>(-3.75f),{}}},8,23,-3},
    {"sound_impulse_time",0x259,{sound,{6,0x7fc00000u,{}}},8,31,INT32_MIN},
    {"sound_impulse_language_time",0x25a,{sound},8,-2147483647,{}},
    {"sound_impulse_time",0x258,{sound},6,-17,{}},
    {"sound_impulse_language_time",0x25a,{sound},7,65535,{}},
    {"sound_impulse_time",0x259,{{32,0xffffffffu,{}},{8,30,{}}},8,0,30},
    {"sound_impulse_language_time",0x25a,{{32,0xffffffffu,{}}},8,0,{}}};
  for(const auto& test:cases){auto c=arena();c.scripts=Program(test.name,test.opcode,test.args,test.context).p;Simulation sim(c,0,Spawn{});unsigned callbacks=0,pcm=0;
    sim.cinematic_text_services([](auto){return true;},[&](auto){++pcm;return 4.f;});
    auto q=constant();q.stored_time=[&](auto s){need(!test.ticks&&test.name=="sound_impulse_time"&&s==test.args[0].bits,"Wrong stored signature dispatched");++callbacks;return test.returned;};
    q.explicit_time=[&](auto s,auto ticks){need(test.ticks&&ticks==*test.ticks&&s==test.args[0].bits,"Explicit scalar coercion/signature changed");++callbacks;return test.returned;};
    q.language_time=[&](auto s){need(!test.ticks&&test.name=="sound_impulse_language_time"&&s==test.args[0].bits,"Wrong language signature dispatched");++callbacks;return test.returned;};
    SourceScriptSoundClockQueryBindingTu15 bound(sim,std::move(q));sim.start_level();sim.step({});
    const auto value=sim.scripts().globals().at(0);need(sim.scripts().diagnostics().empty()&&callbacks==1&&pcm==0,"Bound VM query faulted, duplicated, or used PCM fallback");
    need(value.type==test.context&&value.bits==scalar_cast_bits(std::bit_cast<std::uint32_t>(test.returned),8,test.context),"VM finish lost declared raw signed LONG or context conversion");
  }
  const std::pair<std::string,std::vector<ScriptValue>> bad[]={
    {"sound_impulse_time",{}},{"sound_impulse_time",{sound,{8,1,{}},{8,2,{}}}},
    {"sound_impulse_language_time",{sound,{8,1,{}}}},{"sound_impulse_time",{{8,sound.bits,{}}}},
    {"sound_impulse_time",{sound,{32,30,{}}}}};
  for(const auto& [name,args]:bad){auto c=arena();c.scripts=Program(name,0x258,args).p;Simulation sim(c,0,Spawn{});unsigned calls=0;auto q=constant();q.stored_time=[&](auto){++calls;return 0;};q.explicit_time=[&](auto,auto){++calls;return 0;};q.language_time=[&](auto){++calls;return 0;};SourceScriptSoundClockQueryBindingTu15 bound(sim,std::move(q));sim.start_level();sim.step({});need(!sim.scripts().diagnostics().empty()&&calls==0,"Invalid HS shape invoked a bound query");}
  {auto c=arena();c.scripts=Program("sound_impulse_time",0x258,{sound}).p;Simulation sim(c,0,Spawn{});unsigned pcm=0;sim.cinematic_text_services([](auto){return true;},[&](auto){++pcm;return 1.25f;});sim.start_level();sim.step({});need(sim.scripts().diagnostics().empty()&&pcm==1&&sim.scripts().globals()[0].number()==38,"Unbound development duration path changed");}
  {auto c=arena();c.scripts=Program("sound_impulse_time",0x259,{sound,{8,30,{}}}).p;Simulation sim(c,0,Spawn{});sim.cinematic_text_services([](auto){return true;},[](auto){return 1.f;});sim.start_level();sim.step({});need(!sim.scripts().diagnostics().empty(),"Unbound development path invented explicit overload");}
  {auto c=arena();c.scripts=Program("sound_impulse_time",0x258,{sound}).p;Simulation sim(c,0,Spawn{});unsigned pcm=0;sim.cinematic_text_services([](auto){return true;},[&](auto){++pcm;return 1.f;});auto q=constant();q.stored_time=[](auto)->std::int32_t{throw FormatError("Declared bound query failure");};SourceScriptSoundClockQueryBindingTu15 bound(sim,std::move(q));sim.start_level();sim.step({});need(!sim.scripts().diagnostics().empty()&&pcm==0,"Bound error silently used legacy PCM path");}
}
void numeric_clock(){
  const SoundDriverFloatingPointContext fp{0x027f,0x1f80};SourceScriptSoundClockNative64Tu15 clock;
  SourceScriptSoundClockTimeTu15 now{100,60,0x3c888889u};unsigned reads=0;
  SourceScriptSoundClockPolicyTu15 policy;
  policy.time=[&]{++reads;return now;};policy.select=[](auto,auto& s){s={0,0};};
  policy.selected_seconds=[](auto,auto,const auto&){return 0x3f800000u;};
  const auto ticket=clock.begin(42,policy,{},fp);need(ticket&&clock.finish_failure(*ticket),"Metadata query fixture did not admit");
  auto c=arena();Simulation sim(c,0,Spawn{});
  SourceScriptSoundClockQueriesTu15 q;
  q.stored_time=[&](auto sound){const auto query=policy.time();const auto returned=policy.time();return clock.stored_remaining(sound,1000,query,returned,fp).value;};
  bool change_after_query=false;
  q.explicit_time=[&](auto sound,auto ticks){const auto game=clock.explicit_remaining(sound,ticks,policy.time(),fp);if(change_after_query)now.delta_time_bits=0x3d088889u;const auto fresh=policy.time();return source_script_sound_clock_hs_explicit_tail_tu15(game.value,fresh.delta_time_bits,fp);};
  q.language_time=[&](auto sound){return clock.language_remaining(sound,policy,fp).value;};
  SourceScriptSoundClockQueryBindingTu15 bound(sim,std::move(q));
  need(source_query_script_sound_stored_time_tu15(sim,42)==30&&source_query_script_sound_explicit_time_tu15(sim,42,30)==30&&source_query_script_sound_language_time_tu15(sim,42)==30,"Native64 query units collapsed");
  change_after_query=true;
  need(source_query_script_sound_explicit_time_tu15(sim,42,30)==60,"HS tail reused query unit without delta phase");
  change_after_query=false;
  const auto before=reads;need(source_query_script_sound_language_time_tu15(sim,43)==0&&reads==before,"No-match language read source duration/time");
  now.delta_time_bits=0x7f800000u;
  need(source_query_script_sound_explicit_time_tu15(sim,0xffffffffu,30)==INT32_MIN,"NONE bypassed genuine wrapper FP tail");
}
}
int main()try{registry();callable_lifetime();vm_dispatch();numeric_clock();std::cout<<"TU1.5 typed HS sound query registry, VM/coercion/return, lifetime/thread/reentry and native64 numeric phase controls PASS\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
