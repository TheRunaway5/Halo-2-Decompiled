#include "halo2/script_clock_builtin.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/simulation.hpp"
#include <iostream>

namespace {
using namespace halo2;
void require(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F &&f) {
  bool rejected=false;try{f();}catch(const FormatError&){rejected=true;}
  require(rejected,"Unsafe clock initializer admitted");
}
struct Program {
  ScriptProgram data;
  std::uint32_t add(ScriptNode node) {
    const auto index=unsigned(data.nodes.size());node.salt=std::uint16_t(0xe000u+index);
    data.nodes.push_back(node);return (unsigned(node.salt)<<16u)|index;
  }
  std::uint32_t literal(int value,unsigned type=8) {
    return add({0,std::uint16_t(type),std::uint16_t(type),9,null_datum,0,std::uint32_t(value)});
  }
  std::uint32_t call(const std::string &name,unsigned opcode,unsigned type,
      std::vector<std::uint32_t> children={}) {
    const auto offset=unsigned(data.strings.size());data.strings+=name;data.strings.push_back('\0');
    for(unsigned n=0;n<children.size();++n)
      data.nodes[children[n]&0xffffu].next=n+1u<children.size()?children[n+1u]:null_datum;
    const auto fn=add({0,2,2,9,children.empty()?null_datum:children.front(),offset,0});
    return add({0,std::uint16_t(opcode),std::uint16_t(type),8,null_datum,0,fn});
  }
};
void put(SourceRuntimeMemory &memory,unsigned address,unsigned value,unsigned count=4) {
  std::array<std::byte,4> bytes{};for(unsigned n=0;n<count;++n)bytes[n]=std::byte(value>>(n*8u));
  memory.write(address,std::span(bytes).first(count));
}
void mount_clock(SourceRuntimeMemory &memory,unsigned address,int rate) {
  memory.mount(0x51df54u,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,"clock pointer fixture");
  memory.mount(address,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,"clock fixture");
  put(memory,0x51df54u,address);put(memory,address+2u,std::uint16_t(rate),2);
}
LevelContent arena() {
  LevelContent c;
  c.collision={{{-100,-100,0},{100,-100,0},{100,100,0},0,0},
      {{-100,-100,0},{100,100,0},{-100,100,0},0,0}};
  c.player.datum=1;c.player.height=1;c.player.radius=.2f;c.player.crouch_height=.6f;
  c.player.camera_height=.8f;c.player.vitality={100,100,2,.1f,0};
  c.movement={2,2,2,10,1,1,1,5,2,1};
  return c;
}
}
int main()try {
  SourceRuntimeMemory memory;mount_clock(memory,0x90006000u,30);
  const auto builtin=mcc_script_clock_builtins([&]{return source_script_tick_rate(memory);});
  unsigned host_calls=0;
  ScriptVm::Host general_host=[&](const auto&,const auto&)->ScriptValue{++host_calls;throw FormatError("General host invoked");};
  const auto wrapped_host=[&](const auto &name,const auto &args) {
    if(auto value=builtin(name,args))return *value;
    return general_host(name,args);
  };
  Program program;
  program.data.globals.push_back({"tick_seconds",7,program.call("game_tick_rate",0x38f,7)});
  const auto scalar=program.call("game_tick_rate_scalar",0x390,6);
  const auto target=program.add({0,6,6,13,null_datum,0,1});
  program.data.globals.push_back({"observed_scalar",6,program.literal(0,6)});
  program.data.scripts.push_back({"sample",0,6,program.call("set",4,6,{target,scalar})});
  ScriptVm vm(program.data,wrapped_host,{},{},{},builtin);
  require(vm.globals()[0].type==7 && vm.globals()[0].number()==30 && host_calls==0,
      "MCC initializer did not use its readonly live clock owner");
  vm.start("sample");const auto saved=vm.snapshot();const auto memory_saved=memory.checkpoint();
  for(const int rate:{-32768,-61,-31,-30,-29,0,29,30,31,59,60,61,32767}) {
    put(memory,0x90006002u,std::uint16_t(rate),2);
    vm.restore(saved);vm.tick(0);
    require(vm.globals()[1].type==6 && vm.globals()[1].number()==rate/30 && vm.diagnostics().empty(),
        "Runtime MCC scalar getter differs from signed integral division");
    const auto raw=builtin("game_tick_rate",{});
    require(raw && raw->type==8 && raw->number()==rate,"Getter lost declared LONG or signed rate");
  }
  memory.restore(memory_saved);vm.restore(saved);vm.tick(0);
  require(vm.globals()[1].number()==1,"Clock getter retained a pre-restore value or backing view");
  SourceRuntimeMemory replacement;mount_clock(replacement,0x90007000u,60);memory.swap_backing(replacement);
  vm.restore(saved);vm.tick(0);
  require(vm.globals()[1].number()==2,"Clock getter retained its previous game-time address");
  require(!builtin("game_tick_rate",{ScriptValue::numeric(60,8)}) && !builtin("unrelated",{}),
      "MCC getter captured Xbox setter or unrelated builtin");
  require(!mcc_script_clock_builtins({}),"Absent clock owner acquired a default rate");
  rejects([&]{ScriptVm missing(program.data,general_host);});
  rejects([&]{ScriptVm wrong_type(program.data,general_host,{},{},{},
      [](const auto&,const auto&)->std::optional<ScriptValue>{return ScriptValue::numeric(60,7);});});
  Program wrong_arity;
  wrong_arity.data.globals.push_back({"bad",8,wrong_arity.call("game_tick_rate",0x38f,8,{wrong_arity.literal(60)})});
  rejects([&]{ScriptVm invalid(wrong_arity.data,general_host,{},{},{},builtin);});
  wrong_arity.data.nodes.back().opcode=0x210;
  rejects([&]{ScriptVm setter(wrong_arity.data,general_host,{},{},{},builtin);});
  Program old;
  old.data.globals.push_back({"arithmetic",8,old.call("+",7,8,{old.literal(30),old.literal(12)})});
  ScriptVm legacy(old.data,general_host);
  require(legacy.globals()[0].number()==42 && host_calls==0,"Existing arithmetic constructor changed");
  auto content=arena();content.scripts=program.data;
  Simulation simulation(content,0,Spawn{}, {},{},{},{},{},{},{},
      [&]{return source_script_tick_rate(memory);});
  require(simulation.scripts().globals()[0].number()==60,
      "Simulation did not initialize scripts through the prepared clock provider");
  simulation.start_level();simulation.step({});
  require(simulation.scripts().globals()[1].number()==2 && simulation.scripts().diagnostics().empty(),
      "Simulation runtime host did not retain the prepared clock provider");
  Simulation transferred(content,BspContent{content.collision,content.navigation},0,Spawn{},
      {},{},{},{},{},{},{},[&]{return source_script_tick_rate(memory);});
  require(transferred.scripts().globals()[0].number()==60,"Transferred-BSP clock provider not wired");
  auto legacy_content=arena();legacy_content.scripts=old.data;
  Simulation legacy_simulation(legacy_content,0,Spawn{});
  require(legacy_simulation.scripts().globals()[0].number()==42,"Existing Simulation constructor changed");
  std::cout<<"MCC C++ script clock initializer/runtime/checkpoint and Simulation constructor tests passed\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
