#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_ai_actor_tu15.hpp"
#include "halo2/source_final_startup_storage_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using namespace halo2;
constexpr unsigned destination=0x56ae84;
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> unhex(const std::string &text) {
  std::vector<std::byte> bytes;check(text.size()%2==0,"Odd working-mask fixture hex");
  for(unsigned i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return bytes;
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string text;
  for(auto byte:bytes){const auto value=std::to_integer<unsigned>(byte);text+=digits[value>>4];text+=digits[value&15];}
  return text;
}
void run_case(const nlohmann::json &fixture,const nlohmann::json &input) {
  SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
  check(!memory.is_read_only(destination,20)&&
      std::ranges::all_of(memory.view(destination,20),[](auto b){return b==std::byte{};}),
      "AI working mask lacks its original loader-zeroed writable destination");
  const auto initial=memory.checkpoint();
  const auto owner=std::ranges::find(initial.regions,destination,&SourceRuntimeMemory::CheckpointRegion::address);
  check(owner!=initial.regions.end()&&owner->bytes.size()==20,"Working-mask owner extends past its complete record");
  for(const unsigned address:{destination-1,destination+20}) {
    bool exposed=false;try{(void)memory.view(address,1);exposed=true;}catch(const std::out_of_range&){}
    check(!exposed,"Working-mask owner supplies adjacent unowned state");
  }
  for(const auto &bank:fixture.at("native_component_banks"))memory.mount(bank.at(0),
      std::vector<std::byte>(bank.at(1).get<unsigned>()),SourceRuntimeMemory::Access::ReadWrite,
      "Declared original working-mask component input");
  for(const auto &patch:input.at("patches"))memory.write(patch.at(0),unhex(patch.at(1)));
  memory.write(destination,std::vector<std::byte>(20,std::byte(input.at("poison").get<unsigned>())));
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp=input.at("gp").get<decltype(abi.gp)>();
  abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();abi.eflags=0x202;
  abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
  std::vector<unsigned> trace;nlohmann::json stores=nlohmann::json::array();
  SourcePhysicsWorldRuntimeServices services{};
  services.instruction=[&](unsigned pc){trace.push_back(pc);};
  services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Working-mask fixture delegated a dependency");};
  source_run_ai_frame_tu15(memory,0x1b0720,abi,*abi.floating_point,services,[&](const auto &store){
    stores.push_back(nlohmann::json::array({store.instruction,store.address,hex(store.before),hex(store.written)}));});
  check(abi.gp==input.at("final_gp").get<decltype(abi.gp)>(),"Working-mask complete GPR state differs");
  check(abi.xmm==input.at("final_xmm").get<decltype(abi.xmm)>(),"Working-mask SIMD preservation differs");
  check((abi.eflags&~0x10u)==(input.at("final_flags").get<unsigned>()&~0x10u),"Working-mask defined flags differ");
  check(trace==input.at("trace").get<std::vector<unsigned>>(),"Working-mask original instruction path differs");
  check(stores==input.at("stores"),"Working-mask ordered writes/caller stack differ");
  check(hex(memory.view(destination,20))==input.at("output").get<std::string>(),"Working-mask complete output differs");
  const auto saved=memory.checkpoint();
  const auto retained=std::ranges::find(saved.regions,destination,&SourceRuntimeMemory::CheckpointRegion::address);
  check(retained!=saved.regions.end()&&hex(retained->bytes)==input.at("output").get<std::string>(),
      "Whole-bank checkpoint lost the actual working mask");
  memory.write(destination,std::vector<std::byte>(20,std::byte{0x42}));
  memory.restore(saved);
  check(hex(memory.view(destination,20))==input.at("output").get<std::string>()&&
      !memory.is_read_only(destination,20),"Whole-bank restore lost working-mask bytes or writable ownership");
  bool rejected=false;
  try{mount_native_tu15_final_startup_storage_data(memory);}catch(const std::exception&){rejected=true;}
  check(rejected&&hex(memory.view(destination,20))==input.at("output").get<std::string>(),
      "Repeated cold construction replaced live AI state");
}
void run_prefix_case(const nlohmann::json &fixture,const nlohmann::json &input) {
  SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
  const auto cold=memory.checkpoint();
  for(const auto &region:fixture.at("regions")) {
    const unsigned address=region.at(0),bytes=region.at(1);
    const auto owner=std::ranges::find(cold.regions,address,&SourceRuntimeMemory::CheckpointRegion::address);
    check(owner!=cold.regions.end()&&owner->bytes.size()==bytes&&!memory.is_read_only(address,bytes)&&
        std::ranges::all_of(memory.view(address,bytes),[](auto b){return b==std::byte{};}),
        "Decision workspace changed its exact cold owner or extent");
    memory.write(address,std::vector<std::byte>(bytes,std::byte(input.at("poison").get<unsigned>())));
  }
  memory.write(destination,std::vector<std::byte>(20,std::byte(input.at("poison").get<unsigned>())));
  for(const auto &bank:fixture.at("native_component_banks"))memory.mount(bank.at(0),
      std::vector<std::byte>(bank.at(1).get<unsigned>()),SourceRuntimeMemory::Access::ReadWrite,
      "Declared original decision-gathering component input");
  for(const auto &patch:input.at("patches"))memory.write(patch.at(0),unhex(patch.at(1)));
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp=input.at("gp").get<decltype(abi.gp)>();
  abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();abi.eflags=0x202;
  abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
  std::vector<unsigned> trace;nlohmann::json stores=nlohmann::json::array();
  struct PrefixEnd{};bool completed=false;
  SourcePhysicsWorldRuntimeServices services{};
  services.instruction=[&](unsigned pc){if(pc==fixture.at("prefix_end").get<unsigned>())throw PrefixEnd{};trace.push_back(pc);};
  services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Gathering prefix delegated a dependency");};
  try{source_run_ai_frame_tu15(memory,fixture.at("entry"),abi,*abi.floating_point,services,[&](const auto &store){
    stores.push_back(nlohmann::json::array({store.instruction,store.address,hex(store.before),hex(store.written)}));});}
  catch(const PrefixEnd&){completed=true;}
  check(completed,"Decision gathering did not reach its actual original boundary");
  check(abi.gp==input.at("final_gp").get<decltype(abi.gp)>(),"Decision prefix complete GPR state differs");
  check(abi.xmm==input.at("final_xmm").get<decltype(abi.xmm)>(),"Decision prefix SIMD state differs");
  check((abi.eflags&~0x10u)==(input.at("final_flags").get<unsigned>()&~0x10u),"Decision prefix defined flags differ");
  check(trace==input.at("trace").get<std::vector<unsigned>>(),"Decision prefix original instruction path differs");
  check(stores==input.at("stores"),"Decision prefix ordered writes or preserved packed padding differ");
  const auto saved=memory.checkpoint();
  for(const auto &output:input.at("outputs")) {
    const auto expected=unhex(output.at(1));
    check(hex(memory.view(output.at(0),expected.size()))==output.at(1).get<std::string>(),
        "Decision workspace output or untouched prior padding differs");
    memory.write(output.at(0),std::vector<std::byte>(expected.size(),std::byte{0x42}));
  }
  memory.restore(saved);
  for(const auto &output:input.at("outputs")) {
    const auto expected=unhex(output.at(1));
    check(hex(memory.view(output.at(0),expected.size()))==output.at(1).get<std::string>(),
        "Whole-bank restore lost decision workspace state or prior padding");
  }
}
void run_children_case(const nlohmann::json &fixture,const nlohmann::json &input) {
  SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
  for(const auto &range:fixture.at("selection_ranges")) {
    const unsigned address=range.at(0);const auto expected=unhex(range.at(1));
    std::vector<std::byte> actual(expected.size());memory.read(address,actual);
    check(actual==expected,"Authored child selection bytes differ from unchanged original");
    for(unsigned i=0;i<expected.size();++i)check(memory.is_read_only(address+i,1),
        "Child selection input became writable");
  }
  for(const auto &parent:fixture.at("parents"))for(const auto &child:parent.at("children")) {
    const unsigned row=child.at("address");
    for(const unsigned offset:{6u,7u,12u,13u,14u,15u,16u,17u,18u,19u}) {
      bool hole=false;try{(void)memory.view(row+offset,1);}catch(const std::out_of_range&){hole=true;}
      check(hole,"Child selection input borrowed unread row padding or tail");
    }
  }
  for(const auto &bank:fixture.at("native_component_banks"))memory.mount(bank.at(0),
      std::vector<std::byte>(bank.at(1).get<unsigned>()),SourceRuntimeMemory::Access::ReadWrite,
      "Declared original child constructor component input");
  for(const auto &patch:input.at("patches"))memory.write(patch.at(0),unhex(patch.at(1)));
  const unsigned output=fixture.at("output");memory.write(output,std::vector<std::byte>(200,std::byte{0xa5}));
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp=input.at("gp").get<decltype(abi.gp)>();
  abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();abi.eflags=0x202;
  abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
  std::vector<unsigned> trace;nlohmann::json stores=nlohmann::json::array();
  SourcePhysicsWorldRuntimeServices services{};
  services.instruction=[&](unsigned pc){trace.push_back(pc);};
  services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Child constructor delegated a dependency");};
  bool completed=true;
  try{source_run_ai_actor_tu15(memory,fixture.at("entry"),abi,*abi.floating_point,services,[&](const auto &store){
    stores.push_back(nlohmann::json::array({store.instruction,store.address,hex(store.before),hex(store.written)}));});}
  catch(const std::out_of_range& error){
    completed=false;
    if(input.at("completed").get<bool>())throw std::runtime_error("Child constructor missing input: parent="+
        std::to_string(input.at("parent").get<unsigned>())+" operation="+input.at("operation").dump()+
        " last_pc="+std::to_string(trace.empty()?0:trace.back())+" "+error.what());
  }
  check(completed==input.at("completed").get<bool>(),"Child constructor completion/input-failure behavior differs");
  if(!completed)check(!trace.empty()&&trace.back()==input.at("fault_pc"),"Child constructor failed at a different original read");
  check(abi.gp==input.at("final_gp").get<decltype(abi.gp)>(),"Child constructor complete GPR state differs");
  check(abi.xmm==input.at("final_xmm").get<decltype(abi.xmm)>(),"Child constructor SIMD state differs");
  check((abi.eflags&~0x10u)==(input.at("final_flags").get<unsigned>()&~0x10u),"Child constructor defined flags differ");
  check(trace==input.at("trace").get<std::vector<unsigned>>(),"Child constructor original instruction path differs");
  check(stores==input.at("stores"),"Child constructor ordered writes/caller stack differ");
  check(hex(memory.view(output,200))==input.at("output").get<std::string>(),"Child constructor output ordering or untouched tail differs");
  const auto saved=memory.checkpoint();memory.write(output,std::vector<std::byte>(200,std::byte{0x42}));memory.restore(saved);
  check(hex(memory.view(output,200))==input.at("output").get<std::string>(),"Whole-bank restore lost original child ordering");
}
}
int main(int argc,char **argv)try {
  check(argc==4,"Expected independent original mask, gathering and child constructor fixtures");
  {
    SourceRuntimeMemory memory;mount_native_tu15_final_startup_storage_data(memory);
    std::array<std::byte,10> sentinels;memory.read(0x47c1a4u,sentinels);
    check(memory.is_read_only(0x47c1a4u,4)&&memory.is_read_only(0x47c1a8u,6)&&
        hex(sentinels)=="ffff0000feff0000fdff",
        "Original behavior sentinels differ or became writable");
    bool trailing_hole=false;
    try{(void)memory.view(0x47c1aeu,1);}catch(const std::out_of_range&){trailing_hole=true;}
    check(trailing_hole,"Behavior sentinel declaration borrowed unread trailing file bytes");
    for(bool writable:{false,true}) {
      SourceRuntimeMemory conflict;
      const auto bytes=unhex(writable?"feff0000":"ffff0000");
      conflict.mount(0x47c1a8u,bytes,writable?SourceRuntimeMemory::Access::ReadWrite:
          SourceRuntimeMemory::Access::ReadOnly,"Retained incompatible behavior sentinel owner");
      bool rejected=false;
      try{mount_native_tu15_final_startup_storage_data(conflict);}catch(const std::invalid_argument&){rejected=true;}
      check(rejected&&hex(conflict.view(0x47c1a8u,4))==hex(bytes),
          "Behavior sentinel mounter replaced an incompatible existing owner");
    }
  }
  nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
  check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
      fixture.at("loader_zero_tail")==true&&fixture.at("bytes")==20,"Working-mask loader authority differs");
  check(fixture.at("instruction_patches").empty()&&fixture.at("supplied_dependency_returns").empty(),
      "Original working-mask fixture supplies an outcome");
  for(const auto &input:fixture.at("cases"))run_case(fixture,input);
  nlohmann::json gathering;std::ifstream(argv[2])>>gathering;
  check(gathering.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
      gathering.at("instruction_patches").empty()&&gathering.at("supplied_dependency_returns").empty(),
      "Original decision-gathering authority differs");
  for(const auto &input:gathering.at("cases"))run_prefix_case(gathering,input);
  nlohmann::json children;std::ifstream(argv[3])>>children;
  check(children.at("original_xbe_sha256")==fixture.at("original_xbe_sha256")&&
      children.at("instruction_patches").empty()&&children.at("supplied_dependency_returns").empty(),
      "Original child constructor authority differs");
  for(const auto &input:children.at("cases"))run_children_case(children,input);
  std::cout<<"PASS "<<fixture.at("cases").size()<<" original working-mask cases, "<<gathering.at("cases").size()
      <<" gathering prefixes, "<<children.at("cases").size()<<" child constructors/negative controls, "
      <<"exact owners, GPR/SIMD/flags/stores and checkpoint restore\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
