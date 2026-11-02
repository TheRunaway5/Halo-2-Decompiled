// Offline audit fragment. The validator combines this with the existing
// physical contact-advance observer's bank/child helpers. Every execution here
// is unchanged original code: declared incoming register values are varied to
// test whether the real next contact parent consumes them. None is installed
// into production or supplied to a native replacement as an expected result.
int main(int argc,char**argv){try{
 check(argc==3,"Supply genuine BEFORE input metadata and audit report");
 std::ifstream input(argv[1]);const auto meta=J::parse(input);
 SP=meta.at("entry_sp");PRIVATE_FIRST=meta.at("private_stack_first");
 Memory before;for(const auto&region:meta.at("regions")){
  std::vector<std::byte>data(region.at("bytes").get<std::size_t>());
  std::ifstream file(region.at("file").get<std::string>(),std::ios::binary);
  file.read(reinterpret_cast<char*>(data.data()),data.size());
  check(std::size_t(file.gcount())==data.size(),"Contact context BEFORE bank truncated");
  before.regions.emplace(region.at("address"),Region{region.at("address"),std::move(data)});
 }
 const auto elf=meta.at("elf").get<std::string>();J results=J::array();
 const auto actual_gp=[](const user_regs_struct&r){return J::array({std::uint32_t(r.rax),std::uint32_t(r.rcx),std::uint32_t(r.rdx),std::uint32_t(r.rbx),std::uint32_t(r.rsp),std::uint32_t(r.rbp),std::uint32_t(r.rsi),std::uint32_t(r.rdi)});};
 const auto debug=[](Child&child,unsigned slot,std::uint64_t value){errno=0;const auto result=ptrace(PTRACE_POKEUSER,child.pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)+slot*sizeof(unsigned long)),reinterpret_cast<void*>(value));check(result!=-1||errno==0,"Original context debug boundary setup failed");};
 for(const auto&test:meta.at("cases")){
  auto current=before;for(const auto&patch:test.value("patches",J::array())){const auto address=patch[0].get<std::uint32_t>(),value=patch[1].get<std::uint32_t>();current.write(address,{reinterpret_cast<const std::byte*>(&value),4});}
  const auto name=test.at("name").get<std::string>();
  const NativeTu15PhysicsFrameFp initial{meta.at("before_fp")[0],meta.at("before_fp")[1],meta.at("before_fp")[2]};
  Child entry(elf);entry.put_all(current);entry.initial_simd(meta.at("before_xmm"));
  entry.enter(0x3205b0,test.at("island"),{test.at("configuration"),test.at("agent_entry"),test.at("output"),test.at("time_bits")},SP,initial,J{{"gp",meta.at("before_gp")},{"eflags",meta.at("before_eflags")}});
  debug(entry,0,0x2f7a50);debug(entry,1,STOP);debug(entry,6,0);debug(entry,7,5);entry.request(PTRACE_CONT);entry.wait(SIGTRAP);debug(entry,7,0);
  const auto actual_entry=entry.regs();check(actual_entry.rip==0x2f7a50,name+" did not exercise actual next2F7A50 contact parent");
  const auto entry_fp=entry.fp();const auto entry_gp=actual_gp(actual_entry),entry_xmm=entry.simd();
  const auto stack=std::uint32_t(actual_entry.rsp);std::array<std::uint32_t,5>words{};entry.read(stack,{reinterpret_cast<std::byte*>(words.data()),sizeof(words)});
  const std::vector<std::uint32_t>arguments(words.begin()+1,words.end());
  for(auto&[address,region]:current.regions)entry.read(address,region.data);
  std::vector<std::pair<std::string,J>>poisons;
  poisons.emplace_back("actual_incoming",J{{"gp",entry_gp},{"xmm",entry_xmm}});
  for(const auto bits:{0u,0xffffffffu,0x3f800001u,0xbf000001u,0x7fc12345u,0x7f812345u,0x00000001u,0x80000001u}){
   auto xmm=entry_xmm;for(unsigned reg=0;reg<8;++reg)for(unsigned lane=0;lane<4;++lane)xmm[reg][lane]=bits;
   poisons.emplace_back("all_xmm_"+std::to_string(bits),J{{"gp",entry_gp},{"xmm",xmm}});
  }
  for(unsigned reg=0;reg<8;++reg){auto xmm=entry_xmm;for(unsigned lane=0;lane<4;++lane)xmm[reg][lane]=0x7f812345u^(reg<<5u)^lane;poisons.emplace_back("single_xmm_"+std::to_string(reg),J{{"gp",entry_gp},{"xmm",xmm}});}
  for(const auto bits:{0u,0xffffffffu,0xdeadbeefu}){auto gp=entry_gp;gp[0]=bits;gp[2]=bits;poisons.emplace_back("volatile_eax_edx_"+std::to_string(bits),J{{"gp",gp},{"xmm",entry_xmm}});}
  auto expected=current;NativeTu15PhysicsFrameFp expected_fp{};J variants=J::array();
  for(const auto&[label,poison]:poisons){
   auto actual=current;Child child(elf);child.put_all(current);child.initial_simd(poison.at("xmm"));child.enter(0x2f7a50,actual_entry.rcx,arguments,stack,entry_fp,J{{"gp",poison.at("gp")},{"eflags",std::uint32_t(actual_entry.eflags)}});child.run();const auto after_fp=child.fp();child.get(actual);
   if(label=="actual_incoming"){expected=actual;expected_fp=after_fp;}
   std::size_t compared{},different{};std::uint32_t first{};
   for(const auto&[address,region]:expected.regions){const auto&other=actual.regions.at(address).data;for(std::size_t n=0;n<region.data.size();++n){if(address+n>=PRIVATE_FIRST&&address+n<SP+20u)continue;++compared;if(region.data[n]!=other[n]){if(!different)first=address+n;++different;}}}
   const bool fp_equal=after_fp.control_word==expected_fp.control_word&&after_fp.status==expected_fp.status&&after_fp.mxcsr==expected_fp.mxcsr;
   variants.push_back(J{{"name",label},{"passed",different==0&&fp_equal},{"different_persistent_bytes",different},{"first_difference",first},{"compared_persistent_bytes",compared},{"fp_equal",fp_equal}});
  }
  const bool passed=std::all_of(variants.begin(),variants.end(),[](const auto&v){return v.at("passed").template get<bool>();});
  results.push_back(J{{"name",name},{"passed",passed},{"actual_parent",0x2f7a50},{"parent_receiver",std::uint32_t(actual_entry.rcx)},{"parent_arguments",arguments},{"actual_entry_gp",entry_gp},{"declared_input_variations",variants}});
  std::cout<<name<<" original contact parent "<<(passed?"ignores":"depends on")<<" declared incoming volatile inputs across "<<variants.size()<<" variants\n";
 }
 const bool passed=std::all_of(results.begin(),results.end(),[](const auto&v){return v.at("passed").template get<bool>();});
 std::ofstream output(argv[2]);output<<J{{"passed",passed},{"cases",results},{"required_backend","unchanged-original-contact-parent-entry-input-audit-v1"},{"all_execution_is_original",true},{"native_kernel_validated",false},{"production_postimages_supplied",false},{"capture_after_consumed",false},{"scope","Actual retained contact parent entry reached by unchanged original3205B0 and its real motion owners; vary incoming EAX/EDX and XMM values, compare complete persistent banks and FP"},{"excluded",J::array({"All unexercised branches and different contact-parent owners","Callee-saved incoming GP changes","Returned private control stack and caller-volatile GP/SIMD postimages"})}}.dump(2)<<'\n';check(bool(output),"Contact input-context audit report failed");return passed?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
