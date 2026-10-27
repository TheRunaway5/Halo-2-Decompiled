// Offline driver fragment, combined with the pinned producer physical bridge
// and authenticated instruction metadata by the owned validator. Decoder/GP
// inspection belongs only to this test; the native kernel has typed inputs.
namespace {
using CandidateBranch=NativeTu15ContactCandidateBranch;
using CandidateRelease=NativeTu15ContactCandidateRelease;
std::string byte_hex(std::span<const std::byte>bytes){
 constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(bytes.size()*2);
 for(const auto b:bytes){const auto n=std::to_integer<unsigned>(b);result+=digits[n>>4u];result+=digits[n&15u];}return result;
}
std::uint32_t register_value(const user_regs_struct&r,const std::string&name){
 if(name.empty())return 0;
 if(name=="eax")return r.rax;if(name=="ebx")return r.rbx;
 if(name=="ecx")return r.rcx;if(name=="edx")return r.rdx;
 if(name=="esi")return r.rsi;if(name=="edi")return r.rdi;
 if(name=="ebp")return r.rbp;if(name=="esp")return r.rsp;
 throw std::runtime_error("Unqualified original address register "+name);
}
bool retained_operand(std::uint32_t address,std::size_t bytes,std::uint32_t packet,
 std::uint32_t entry_sp){
 if(address<packet-256u||address>=entry_sp+24u)return true;
 // Explicit retained data packets. Source control locals, stack arguments,
 // register saves, return addresses and the platform FS chain are excluded.
 const std::array<std::pair<unsigned,unsigned>,5>ranges{{{0x2c,0x40},{0x40,0x74},
  {0x80,0x180},{0x180,0x38c},{0x390,0x3d0}}};
 for(const auto&[first,end]:ranges)if(address>=packet+first&&address+bytes<=packet+end)return true;
 return false;
}
struct CandidateMemory:SourcePhysicsContactMemory{
 Child&child;std::uint32_t packet,entry_sp;mutable J ledger=J::array();
 std::size_t reads{},writes{};
 CandidateMemory(Child&c,std::uint32_t p,std::uint32_t sp):child(c),packet(p),entry_sp(sp){}
 void read(std::uint32_t p,std::span<std::byte>b)const override{
  const auto input=child.read(p,b.size());std::copy(input.begin(),input.end(),b.begin());
  if(retained_operand(p,b.size(),packet,entry_sp))ledger.push_back(J::array({"read",p,b.size(),byte_hex(b)}));
  ++const_cast<CandidateMemory*>(this)->reads;
 }
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{
  child.write(p,b);if(retained_operand(p,b.size(),packet,entry_sp))ledger.push_back(J::array({"write",p,b.size(),byte_hex(b)}));++writes;
 }
};
J trace_parent(Child&child,const J&instructions,std::uint32_t packet,std::uint32_t sp,
 J&services,std::size_t&steps){
 J ledger=J::array();
 while(std::uint32_t(child.regs().rip)!=0x3043e0u){
  check(++steps<32768,"Original candidate left bounded authenticated owners");
  const auto r=child.regs();const auto pc=std::uint32_t(r.rip);
  const auto key=std::to_string(pc);check(instructions.contains(key),"Original candidate left authenticated CFG at "+key);
  const auto&instruction=instructions.at(key);
  struct PendingWrite{std::size_t index;std::uint32_t address;unsigned bytes;};
  std::vector<PendingWrite>pending_writes;
  for(const auto&operand:instruction.at("memory")){
   if(instruction.value("repeat_copy",false)&&!std::uint32_t(r.rcx))continue;
   const auto address=register_value(r,operand.at("base"))+register_value(r,operand.at("index"))*operand.at("scale").get<unsigned>()+operand.at("displacement").get<std::uint32_t>();
   const auto bytes=operand.at("bytes").get<unsigned>();
   if(!retained_operand(address,bytes,packet,sp))continue;
   if(operand.at("read").get<bool>())ledger.push_back(J::array({"read",address,bytes,byte_hex(child.read(address,bytes))}));
   if(operand.at("write").get<bool>()){pending_writes.push_back({ledger.size(),address,bytes});ledger.push_back(J::array({"write",address,bytes,""}));}
  }
  if(instruction.contains("service_arguments")){
   const auto return_pc=pc+instruction.at("size").get<unsigned>();
   const auto argument_count=instruction.at("service_arguments").get<unsigned>();
   child.step();const auto entered=child.regs();J arguments=J::array();
   for(unsigned n=0;n<argument_count;++n)arguments.push_back(child.word(std::uint32_t(entered.rsp)+4u+n*4u));
   const auto f=child.fp();J event{{"entry",std::uint32_t(entered.rip)},
    {"this",std::uint32_t(entered.rcx)},{"arguments",arguments},{"original_return_boundary",return_pc},
    {"before_esp",std::uint32_t(entered.rsp)},{"before_fp",fp_words(f)},
    {"ecx_consumed",instruction.at("ecx_consumed")},
    {"result_consumed",instruction.at("result_consumed")}};
   child.go(return_pc);const auto after=child.regs();event["returned_eax"]=std::uint32_t(after.rax);
   event["after_esp"]=std::uint32_t(after.rsp);event["after_fp"]=fp_words(child.fp());services.push_back(event);
  }else child.step();
  for(const auto&write:pending_writes)ledger[write.index][3]=byte_hex(child.read(write.address,write.bytes));
 }
 return ledger;
}
std::uint32_t select_branch(CandidateBranch branch,std::uint32_t a,std::uint32_t b,std::uint32_t c){
 return branch==CandidateBranch::unsorted?a:branch==CandidateBranch::sorted_merge?b:c;
}
}
int main(int argc,char**argv){
 J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false}};
 try{
  check(argc==7,"Supply ELF, capture, ordinal, variation, metadata and output");
  std::ifstream in(argv[2]),metadata_in(argv[5]);const auto capture=J::parse(in),instructions=J::parse(metadata_in);
  const unsigned ordinal=std::stoul(argv[3]),variation=std::stoul(argv[4]),ordering=variation&1u;
  Child original;original.start(argv[1],ordinal);original.go(0x2f36f0);const auto entry=original.regs();
  const auto before=original.fp();check(before.ftw==0,"Candidates require declared empty incoming x87");check(!(entry.eflags&0x400u),"Native forward-copy contract requires source DF clear");
  const auto sp=std::uint32_t(entry.rsp),packet=((sp-4u)&~15u)-0x3e0u;
  const std::array<std::uint32_t,4>args{original.word(sp+4),original.word(sp+8),original.word(sp+12),original.word(sp+16)};
  original.write(0x46e468,std::array{std::byte{0}});original.write(0x48cbb8,std::array{std::byte(ordering)});
  const auto owner=std::uint32_t(entry.rcx);const auto write_word=[&](std::uint32_t address,std::uint32_t value){original.write(address,{reinterpret_cast<const std::byte*>(&value),4});};
  if(variation>=2){check(original.word(owner+0x10u)>0,"Creation variation requires actual old entries");write_word(owner+0x10u,variation>=4?1u:0u);write_word(owner+0x14u,0x80000000u|(variation>=4?1u:0u));if(variation>=4)write_word(original.word(owner+0xcu),0xfffffffeu);}
  struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;
  for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
  J actual_services=J::array();std::size_t steps{};const auto actual_ledger=trace_parent(original,instructions,packet,sp,actual_services,steps);
  const auto expected_fp=original.fp();for(auto&bank:banks)bank.after=original.read(bank.address,bank.before.size());
  // A separate uninterrupted original parent proves that stepping/recording
  // did not change its persistent state. This result is evidence only.
  std::size_t plain_differences{};
  {Child plain;plain.start(argv[1],ordinal);plain.go(0x2f36f0);
   for(const auto&bank:banks)plain.write(bank.address,bank.before);
   auto plain_entry=entry;request(PTRACE_SETREGS,plain.pid,nullptr,&plain_entry);
   auto plain_fp=before;request(PTRACE_SETFPREGS,plain.pid,nullptr,&plain_fp);
   plain.go(0x3043e0);
   const auto first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),end=capture.at("private_returned_stack").at("end").get<std::uint32_t>();
   for(const auto&bank:banks){const auto actual=plain.read(bank.address,bank.after.size());for(std::size_t n=0;n<actual.size();++n)if((bank.address+n<first||bank.address+n>=end)&&actual[n]!=bank.after[n])++plain_differences;}
   check(fp_words(plain.fp())==fp_words(expected_fp),"Tracing changed original FP state");}
  Child native;native.start(argv[1],ordinal);native.go(0x2f36f0);for(const auto&bank:banks)native.write(bank.address,bank.before);
  auto installed=entry;request(PTRACE_SETREGS,native.pid,nullptr,&installed);auto installed_fp=before;request(PTRACE_SETFPREGS,native.pid,nullptr,&installed_fp);
  CandidateMemory memory(native,packet,sp);Bridge dependencies{native,packet,before,report};NativeTu15ContactCandidateServices services;
  services.bounds=[&](auto shape,auto method,auto transform,auto margin,auto output,auto&fp){dependencies.call(method.identity,shape.identity,{transform,margin,output},0x2f3769,fp);};
  services.query_keys=[&](auto shape,auto method,auto bounds,auto header,auto&fp){dependencies.call(method.identity,shape.identity,{bounds,header},0x2f3821,fp);};
  services.resolve_child=[&](auto shape,auto method,auto key,auto backing,CandidateBranch branch,auto&fp){return NativeTu15ContactChild{dependencies.call(method.identity,shape.identity,{key,backing},select_branch(branch,0x2f3a47u,0x2f3bd5u,0x2f3c9bu),fp)};};
  services.filter=[&](auto receiver,auto method,auto result,auto context,auto first,auto second,auto parent,auto key,CandidateBranch branch,auto&fp){return dependencies.call(method.identity,receiver,{result,context.identity,first.identity,second.identity,parent,key},select_branch(branch,0x2f3a74u,0x2f3c05u,0x2f3cc4u),fp);};
  services.create_agent=[&](auto method,auto first,auto second,auto context,auto data,CandidateBranch,auto&fp){
   check(method.identity==0x2f3140u,"Candidate factory requires its qualified native dispatch owner");
   // The native parent calls the native dispatcher. Its remaining class and
   // selected factory services run real unchanged bodies at their real source
   // nesting depth; no original dispatcher outcome is supplied.
   Bridge dispatch{native,packet-36u,before,report};unsigned classifications{};
   NativeTu15ContactAgentDispatchServices owners;
   owners.classify_shape=[&](auto shape,auto owner,auto&state){return dispatch.call(owner.identity,shape.identity,{},classifications++?0x2f3163u:0x2f3156u,state);};
   owners.create_agent=[&](auto owner,auto a,auto b,auto settings,auto user,auto&state){return NativeTu15ContactAgent{dispatch.call(owner.identity,b.identity,{a.identity,b.identity,settings.identity,user},0x2f317du,state)};};
   const auto result=native_tu15_dispatch_contact_agent(memory,first,second,context,data,fp,owners);
   for(const auto&call:dispatch.calls)dependencies.calls.push_back(call);return result;
  };
  services.release_agent=[&](auto agent,auto method,CandidateBranch branch,auto&fp){dependencies.call(method.identity,agent.identity,{},select_branch(branch,0x2f38bbu,0x2f3c53u,0x2f3c76u),fp);};
  NativeTu15ContactBufferServices entry_owners;
  entry_owners.allocate_stack_block=[&](auto receiver,auto method,auto bytes,auto&fp){Bridge pool{native,packet-32u,before,report};const auto result=pool.call(method.identity,receiver.identity,{bytes},0x2f3484u,fp);for(const auto&call:pool.calls)dependencies.calls.push_back(call);return result;};
  entry_owners.release_stack_block=[&](auto receiver,auto method,auto data,auto bytes,auto&fp){Bridge pool{native,packet-28u,before,report};pool.call(method.identity,receiver.identity,{data,bytes},0x2f3523u,fp);for(const auto&call:pool.calls)dependencies.calls.push_back(call);};
  entry_owners.release_owned_components=[&](auto receiver,auto method,auto data,auto bytes,auto tag,auto&fp){Bridge pool{native,packet-28u,before,report};pool.call(method.identity,receiver.identity,{data,bytes,tag},0x2f3560u,fp);for(const auto&call:pool.calls)dependencies.calls.push_back(call);};
  services.sort_keys=[&](auto keys,auto first,auto last,auto comparator,auto&){native_tu15_sort_contact_keys(memory,keys,first,last,comparator);};
  services.initialize_entries=[&](auto header,auto count,auto&fp){native_tu15_initialize_contact_entries(memory,{header},count,fp,entry_owners);};
  services.destroy_entries=[&](auto header,auto&fp){native_tu15_destroy_contact_entries(memory,{header},fp,entry_owners);};
  services.reserve_entries=[&](auto header,auto capacity,auto stride,auto&fp){
   Bridge reserve{native,packet-32u,before,report};NativeTu15ArrayGrowthServices owners;
   owners.allocate=[&](const auto&request,auto&state){return reserve.call(request.owner,request.receiver,{request.bytes,request.memory_class},0x2e31ffu,state);};
   owners.release=[&](const auto&request,auto&state){reserve.call(request.owner,request.receiver,{request.data,request.bytes,request.memory_class},0x2e3238u,state);};
   native_tu15_reserve_array(memory,NativeTu15ArrayHeader{header},capacity,stride,fp,owners);
   for(const auto&call:reserve.calls)dependencies.calls.push_back(call);
  };
  services.release=[&](auto receiver,auto method,auto data,auto bytes,auto tag,CandidateRelease phase,auto&fp){check(phase!=CandidateRelease::unwind_keys,"Unwind service context requires independent cleanup qualification");dependencies.call(method.identity,receiver,{data,bytes,tag},phase==CandidateRelease::replacement?0x2f3d2eu:0x2f3dc2u,fp);};
  services.allocate=[&](auto receiver,auto method,auto bytes,auto tag,auto&fp){return dependencies.call(method.identity,receiver,{bytes,tag},0x2f3d46,fp);};
  NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ContactCandidateStats stats;
  native_tu15_update_contact_candidates(memory,{owner},{args[0]},{args[1]},{args[2]},{args[3]},{packet},fp,0,services,{},&stats);
  const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>();
  std::size_t compared{},different{},private_different{};J diffs=J::array();
  for(const auto&bank:banks){const auto actual=native.read(bank.address,bank.after.size());for(std::size_t n=0;n<actual.size();++n){const auto address=bank.address+n;const bool is_private=address>=private_first&&address<private_end;if(!is_private)++compared;if(actual[n]==bank.after[n])continue;if(is_private){++private_different;continue;}++different;if(diffs.size()<32)diffs.push_back(J{{"address",address},{"native",std::to_integer<unsigned>(actual[n])},{"original",std::to_integer<unsigned>(bank.after[n])}});}}
  J service_errors=J::array();if(actual_services.size()!=dependencies.calls.size())service_errors.push_back("service count differs");
  for(std::size_t n=0;n<std::min(actual_services.size(),dependencies.calls.size());++n){const auto&a=actual_services[n];const auto&b=dependencies.calls[n];for(const auto*key:{"entry","arguments","original_return_boundary","before_esp","after_esp","before_fp","after_fp"})if(a[key]!=b[key])service_errors.push_back(J{{"index",n},{"field",key},{"original",a[key]},{"native",b[key]}});if(a.at("result_consumed").get<bool>()&&a["returned_eax"]!=b["returned_eax"])service_errors.push_back(J{{"index",n},{"field","returned_eax"},{"original",a["returned_eax"]},{"native",b["returned_eax"]}});if(a.at("ecx_consumed").get<bool>()&&a["this"]!=b["this"])service_errors.push_back(J{{"index",n},{"field","this"},{"original",a["this"]},{"native",b["this"]}});}
  J ledger_errors=J::array();for(std::size_t n=0;n<std::min(actual_ledger.size(),memory.ledger.size());++n)if(actual_ledger[n]!=memory.ledger[n]){ledger_errors.push_back(J{{"index",n},{"original",actual_ledger[n]},{"native",memory.ledger[n]}});if(ledger_errors.size()==8)break;}
  const bool ordered=actual_ledger==memory.ledger,fp_equal=fp_words(expected_fp)==J::array({fp.control_word,fp.status,fp.mxcsr});
  report.update(J{{"passed",different==0&&fp_equal&&ordered&&service_errors.empty()&&plain_differences==0},{"source_entry",0x2f36f0},{"producer_ordinal",ordinal},{"declared_ordering_flag",ordering},{"declared_entry_variation",variation},{"declared_cache_flag",0},{"arguments",args},{"fp_equal",fp_equal},{"persistent_compared_bytes",compared},{"different_bytes",different},{"private_control_stack_different_bytes",private_different},{"first_differences",diffs},{"native_reads",memory.reads},{"native_writes",memory.writes},{"original_parent_instructions",steps},{"ordered_retained_operands_equal",ordered},{"original_operand_events",actual_ledger.size()},{"native_operand_events",memory.ledger.size()},{"first_operand_differences",ledger_errors},{"service_context_differences",service_errors},{"traced_versus_plain_original_persistent_differences",plain_differences},{"independently_recorded_original_service_calls",actual_services},{"actual_native_service_calls",dependencies.calls},{"native_created",stats.created},{"native_released",stats.released},{"native_reserves",stats.reserve_calls},{"remaining_services_execute_original_cpu",true},{"production_native_only_closure",false}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}
 std::cout<<J{{"passed",report["passed"]},{"different_bytes",report.value("different_bytes",0)},{"ordered",report.value("ordered_retained_operands_equal",false)},{"service_errors",report.value("service_context_differences",J::array()).size()},{"error",report.value("error",std::string{})}}.dump()<<'\n';
 return report.at("passed").get<bool>()?0:1;
}
