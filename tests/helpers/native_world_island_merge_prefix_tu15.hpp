#pragma once
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
// Frame-level typed09 BEFORE DATA. An ordinary owned adapter frame is declared;
// no original caller ABI, GP/SIMD or post-call results were captured/imported.
inline int native_world_island_merge_prefix(const std::filesystem::path&reference,
    const std::filesystem::path&report_path){
  using J=nlohmann::json;std::ifstream file(reference/"capture-native-before.json");const auto capture=J::parse(file);
  check(capture.at("reference")=="English TU1.5 only"&&!capture.at("guest_abi_fabricated").get<bool>()&&
    !capture.at("captured_after_consumed").get<bool>()&&capture.at("preserved_x87_depth")==0,"Genuine typed09 BEFORE required");
  Memory m;std::uint64_t bytes{};unsigned banks{};
  for(const auto&r:capture.at("regions")){
    const auto address=r.at("address").get<std::uint32_t>();std::ifstream in(reference/r.at("before_file").get<std::string>(),std::ios::binary);
    check(bool(in),"Missing authenticated09 BEFORE bank");std::vector<char>c((std::istreambuf_iterator<char>(in)),{});check(c.size()==r.at("size"),"Actual09 bank size differs");
    std::vector<std::byte>b(c.size());for(unsigned n=0;n<c.size();++n)b[n]=std::byte(std::uint8_t(c[n]));
    check(m.banks.emplace(address,std::move(b)).second,"Duplicate actual09 region");bytes+=c.size();++banks;
  }
  // Joining exactly adjacent spans only makes an ordinary byte-addressed
  // memory view; no hole, instruction, allocator result or state is supplied.
  for(auto i=m.banks.begin();i!=m.banks.end();){auto next=std::next(i);if(next!=m.banks.end()&&std::uint64_t(i->first)+i->second.size()==next->first){i->second.insert(i->second.end(),next->second.begin(),next->second.end());m.banks.erase(next);}else ++i;}
  constexpr std::uint32_t arena=0x72000000u;for(const auto&[p,b]:m.banks)check(std::uint64_t(p)+b.size()<=arena||p>=arena+0x12000u,"Owned World adapter arena aliases actual source input");
  m.banks[arena].resize(0x12000u);
  const auto actual_world=m.get(0x52be48u),queue=m.get(actual_world+0x20u);
  check(m.get(actual_world+0x24u)==1,"Actual09 must have exactly one genuine queued merge");
  const auto first_body=m.get(queue),second_body=m.get(queue+4u),first_island=m.get(first_body+0x44u),second_island=m.get(second_body+0x44u),old_active=m.get(actual_world+0xcu);
  check(first_island!=second_island&&m.get(first_island+0x29u,1)&&m.get(second_island+0x29u,1),"Actual09 merge identities/active admission changed");
  SourcePhysicsContactRuntimeAbi state{};const auto&recorded=capture.at("floating_point");
  NativeTu15PhysicsFrameFp fp{recorded.at("x87_control_word"),recorded.at("x87_status"),recorded.at("mxcsr")};
  state.gp[4]=arena+0x11000u;state.x87_depth=0;state.x87_status=fp.status;state.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};state.eflags=2;
  // Existing configuration values are declared previous-step input DATA;
  // the frame capture did not record formal319D50/319F90 arguments.
  const NativeTu15PhysicsStep step{m.get(actual_world+0x140u),m.get(actual_world+0x144u)};
  NativeTu15WorldIntegrationStats stats;std::uint64_t residuals{},ticks{},stores{};std::vector<std::uint32_t>calls;struct Complete{};bool reached=false;
  try{
    native_tu15_step_physics_world_with_residuals(m,{actual_world},step,fp,state,0x70001000u,{arena,0x10000u},
      [&](std::uint32_t target,SourcePhysicsContactRuntimeAbi&,const SourcePhysicsContactRecordObserver&){calls.push_back(target);throw std::runtime_error("World merge prefix reached an unconverted original owner");},
      [&](const SourcePhysicsContactRecordStore&e){++stores;if(e.instruction==0x31979cu&&e.address==actual_world+0x24u){reached=true;throw Complete{};}},
      &stats,&residuals,[&]{return ++ticks;});
  }catch(const Complete&){}
  check(reached,"World did not finish native deferred merge prefix");
  check(stats.deferred.merges==1&&stats.island_merge.calls==1&&stats.island_contact_merge.calls==1&&
    stats.island_merge.moved_bodies==1&&stats.island_lifetime.island_disposals==1,"World native merge/lifetime was not selected");
  check(stats.native_world_merge_refusals==0&&stats.native_world_activation_refusals==0&&stats.native_world_activation_calls==0&&
    stats.island_merge.unbound_services==0&&stats.island_contact_merge.unbound_services==0&&stats.island_lifetime.unbound_services==0&&
    calls.empty()&&residuals==0,"World native merge prefix used an old/unqualified child");
  check(m.get(second_body+0x44u)==first_island&&m.get(actual_world+0xcu)==old_active-1u&&m.get(actual_world+0x24u)==0,"World merge did not publish source-consistent membership");
  const J report{{"passed",true},{"scope","Genuine09 World binding PREFIX after31979C operation drain; no whole original World comparison or live invocation claim"},
    {"archive_sha256","0fd26e268228c50ea01c07a5fea7b68a1e06c515eac2ec90562f303c5873caac"},{"before_banks",banks},{"before_bytes",bytes},
    {"actual_world",actual_world},{"actual_queued_bodies",J::array({first_body,second_body})},{"actual_queued_islands",J::array({first_island,second_island})},
    {"native_merge_calls",stats.island_merge.calls},{"native_contact_merge_calls",stats.island_contact_merge.calls},{"native_body_moves",stats.island_merge.moved_bodies},
    {"native_constraint_moves",stats.island_merge.moved_constraints},{"native_disposals",stats.island_lifetime.island_disposals},{"native_activation_calls",stats.native_world_activation_calls},
    {"duplicate_records",stats.island_contact_merge.duplicates},{"remaining_original_services",calls},{"source_labeled_stores",stores},{"captured_GP_SIMD_or_after_supplied",false},
    {"declared_adapter_frame",true},{"declared_previous_step_DATA",true},{"full_original_World_compared",false},{"explicit_observation_stop",true}};
  std::ofstream out(report_path);check(bool(out),"Cannot retain owned prefix report");out<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}
