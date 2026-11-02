// Actual current-cache reference DATA and a real handle's context. The caller
// pair is declared, not a capturedC6120 ABI; no tagged-value row is created.
int main(int argc,char**argv){J report{{"passed",false},{"captured_after_consumed",false},
 {"instruction_patches",J::array()},{"manufactured_input_rows",false},{"source_caller_abi_captured",false},
 {"declared_reference_context_pairing",true}};try{
 check(argc==7,"Supply ELF,capture,selection,FP mode,metadata,output");
 std::ifstream input(argv[2]),meta_input(argv[5]);const auto capture=J::parse(input),metadata=J::parse(meta_input);
 const auto mode=std::uint32_t(std::stoul(argv[4]));check(mode<4,"Unqualified FP mode");
 std::vector<std::uint32_t>references;std::uint32_t context{};
 {Child source;source.start(argv[1],0);const auto table=source.word(0x4f0414u),index=source.word(0x556d90u),count=source.word(index+24u);
  check(count>0&&count<=65536,"Actual current-cache extent unavailable");
  for(unsigned n=0;n<count;++n)if(source.word(table+n*16u)==0x7464746cu)references.push_back(source.word(table+n*16u+4u));
  check(!references.empty(),"No actual referenced-group tag available");
  const auto pool=source.word(0x4f720cu),rows=source.word(pool+0x44u);check(source.word(pool+0x38u)>0,"No actual context owner available");
  context=source.word(rows+0x48u);
 }
 const auto actual_sp=capture.at("before").at("gp").at(4).get<std::uint32_t>(),
  private_first=capture.contains("private_returned_stack")?capture.at("private_returned_stack").at("first").get<std::uint32_t>():((actual_sp-1u)&~0xffffu),
  private_end=actual_sp,sp=actual_sp-0x1000u;check(sp>=private_first+0x2000u,"Owned service storage unavailable");J cases=J::array();
 for(const auto actual_reference:references)for(unsigned variant=0;variant<4;++variant){
  const auto reference=variant&1u?0xffffffffu:actual_reference,owner=variant&2u?0xffffffffu:context;
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);auto before=original.fp();
  check(before.ftw==0,"Actual input requires empty x87");before.cwd=std::uint16_t((before.cwd&~0xc00u)|(mode<<10u));
  before.mxcsr=(before.mxcsr&~0x6000u)|(mode<<13u);before.xmm_space[4]=0;
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
  put(sp,std::uint32_t(0x1ccacau));put(sp+4u,reference);put(sp+8u,owner);auto entry=original.regs();entry.rsp=sp;entry.rip=0xc6120u;
  request(PTRACE_SETREGS,original.pid,nullptr,&entry);ActivationMemory memory(native,private_first,private_end);
  J calls=J::array();std::size_t steps{};const auto ledger=activation_trace(original,metadata,memory,calls,steps);
  NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ConstraintTagValueStats stats;
  native_tu15_publish_constraint_tag_value(memory,reference,owner,0u,fp,{},&stats);
  std::size_t compared{},different{};
  for(const auto&bank:capture.at("regions")){const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();
   const auto a=original.read(address,size),b=native.read(address,size);for(unsigned n=0;n<size;++n){const auto p=address+n;
    if(p>=private_first&&p<private_end)continue;++compared;if(a[n]!=b[n])++different;}}
  const bool ordered=ledger==memory.ledger,fp_equal=fp_words(original.fp())==J::array({fp.control_word,fp.status,fp.mxcsr}),
   ret_equal=std::uint32_t(original.regs().rsp)==sp+12u;
  J differences=J::array();for(std::size_t n=0;n<std::min(ledger.size(),memory.ledger.size());++n)
   if(ledger[n]!=memory.ledger[n]&&differences.size()<8)differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"passed",ordered&&fp_equal&&ret_equal&&different==0},{"actual_reference",actual_reference},
   {"reference",reference},{"context",owner},{"declared_none_variant",variant},{"ordered_operands_equal",ordered},
   {"first_operand_differences",differences},{"fp_equal",fp_equal},{"source_return_equal",ret_equal},
   {"persistent_compared_bytes",compared},{"different_bytes",different},{"source_instructions",steps},
   {"calls",stats.calls},{"disabled_skips",stats.disabled_skips},{"invalid_inputs",stats.invalid_inputs},
   {"iterator_queries",stats.iterator_queries},{"candidates",stats.candidates},{"publications",stats.publications}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();
 report.update(J{{"passed",passed},{"declared_FP_mode",mode},{"cases",cases}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}
 std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';
 return report.at("passed").get<bool>()?0:1;
}
