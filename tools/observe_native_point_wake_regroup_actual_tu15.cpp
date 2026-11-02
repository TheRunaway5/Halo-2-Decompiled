// Genuine1470 source DATA; ordinary original formals are declared separately.
// No fabricated contacts, poses, flags, captured GP, or original outcomes.
int main(int argc,char**argv){J report{{"passed",false},{"supplied_original_outcomes",false},{"captured_original_GP",false},{"captured_actual_wake_call",false},{"source_physics_fields_overridden",false}};try{
 check(argc==4,"Supply actualELF,typedBefore,report");std::ifstream input(argv[2]);const auto capture=J::parse(input);constexpr std::uint32_t sp=0x7200f000u;
 const auto manager=capture.at("input").at("manager").get<std::uint32_t>(),point=capture.at("input").at("point").get<std::uint32_t>();J cases=J::array();
 for(unsigned mode=0;mode<4;++mode)for(unsigned seed:{0u,0xa5u,0x5au}){
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);const auto world=original.word(manager+8u),island=original.word(manager+0x24u);check(island==original.word(manager+0x24u),"Actual receiver changed");
  const auto receiver=original.word(manager+0x10u+0x14u);check(original.read(receiver+0x29u,1)[0]==std::byte{0},"Genuine point receiver must be inactive");
  auto before=original.fp();before.cwd=0x27fu|(mode<<10u);before.swd=0;before.mxcsr=0x1f80u|(mode<<13u);before.ftw=0;std::memset(before.st_space,0,sizeof(before.st_space));request(PTRACE_SETFPREGS,original.pid,nullptr,&before);request(PTRACE_SETFPREGS,native.pid,nullptr,&before);
  ActivationMemory memory(native,0x72000000u,0x72010000u);std::vector<std::byte>pod(0x4000u,std::byte(seed));native.write(0x72004000u,pod);NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};
  NativeTu15ContactPointBindingActivity removal;NativeTu15ObjectActivationBindingActivity activation;NativeTu15IslandRegroupBindingActivity regroup;unsigned unknown{};J phases=J::array();
  const auto call_original=[&](std::uint32_t entry,std::uint32_t ecx,std::span<const std::uint32_t>args){
    const auto put=[&](std::uint32_t address,std::uint32_t value){original.write(address,{reinterpret_cast<const std::byte*>(&value),4});};put(sp,0x1ccacau);for(unsigned n=0;n<args.size();++n)put(sp+4u+4u*n,args[n]);auto r=original.regs();r.rip=entry;r.rcx=ecx;r.rsp=sp;request(PTRACE_SETREGS,original.pid,nullptr,&r);original.go(0x1ccacau);check(std::uint32_t(original.regs().rsp)==sp+4u+args.size()*4u,"Original declared formal RET differs");
  };
  const auto compare=[&](const char*phase){std::uint64_t bytes{},different{};J first=J::array();for(const auto&bank:capture.at("regions")){const auto p=bank.at("address").get<std::uint32_t>(),n=bank.at("size").get<std::uint32_t>();const auto a=original.read(p,n),b=native.read(p,n);bytes+=n;for(unsigned k=0;k<n;++k)if(a[k]!=b[k]){++different;if(first.size()<8)first.push_back(p+k);}}
    const auto actual=original.fp();const bool equal=fp_words(actual)==J::array({fp.control_word,fp.status,fp.mxcsr})&&actual.ftw==0;phases.push_back(J{{"phase",phase},{"passed",different==0&&equal},{"all_actual_before_bytes",bytes},{"different_bytes",different},{"first_different_addresses",first},{"fp_equal",equal}});check(different==0&&equal,std::string("Source/native mismatch after ")+phase);
  };
  const std::array point_args{point};call_original(0x322f70u,manager,point_args);
  native_tu15_remove_bound_contact_point(memory,manager,point,{0x72004000u,0x72004020u,0x72004040u,0x72004020u},fp,0,{}, {},&removal);compare("actual_point_removal");
  const std::array wake_args{receiver};call_original(0x316d40u,world,wake_args);
  native_tu15_activate_island_with_residuals(memory,{world},{receiver},fp,0,
    {0x72007000u,0x72007004u,0x72007020u,0x72007100u,0x800u},
    [&](auto owner,std::span<const std::uint32_t>,std::optional<std::uint32_t>,unsigned,auto&)->std::uint32_t{++unknown;throw std::runtime_error("Native actual1470 wake requires owner="+std::to_string(owner));},{},&activation);compare("declared_standalone_wake_on_genuine_DATA");
  call_original(0x3187c0u,world,{});
  native_tu15_regroup_physics_islands_with_native_services(memory,{world},
    {{0x72005000u},{0x72005020u},{0x72005040u},{0x72005060u},{0x72005080u},{{0x720050a0u},0x720050c0u},{0x72005100u},0x72005120u,{0x72005160u}},fp,{},&regroup);compare("declared_standalone_regroup_after_native_wake");
  check(removal.completed==1&&activation.activation.island_calls==1&&regroup.completed==1&&regroup.regroup.partition.builds>0&&regroup.regroup.fully_connected>0&&!unknown,"Native genuine DATA composition was not positive");
  cases.push_back(J{{"passed",true},{"mode",mode},{"native_POD_seed",seed},{"phases",phases},{"point_removals",removal.completed},{"native_wakes",activation.activation.island_calls},{"native_agent_factories",activation.native_agent_factories},{"body_updates",activation.services.body_updates},{"regroup_calls",regroup.completed},{"graph_builds",regroup.regroup.partition.builds},{"fully_connected",regroup.regroup.fully_connected},{"new_islands",regroup.regroup.created_islands},{"remaining_owner_calls",unknown},{"source_partition_flag_injected",false}});
 }
 report.update(J{{"passed",true},{"cases",cases},{"scope","All captured actual1470 banks and FP after whole physical original point/wake/regroup versus same native production builders. Wake/regroup are declared standalone original formals on source-authentic computed DATA, not captured actual callers or live invocation. No contact rows/flags/poses manufactured."}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=4){std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;}
