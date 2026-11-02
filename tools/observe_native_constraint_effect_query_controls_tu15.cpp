// Direct positive authored queries. These ordinary parameters are projected
// from real material/event rows; no physics or metadata row is manufactured.
namespace halo2 {
std::uint32_t test_native_effect_definition_query(const SourcePhysicsContactMemory&,std::uint32_t,std::uint32_t,std::uint16_t,bool,NativeTu15ConstraintEffectSelectorStats*);
std::uint32_t test_native_effect_material_query(const SourcePhysicsContactMemory&,std::uint16_t,std::uint32_t,std::uint16_t,bool,NativeTu15ConstraintEffectSelectorStats*);
}
int main(int argc,char**argv){J report{{"passed",false},{"captured_after_consumed",false},{"manufactured_rows",false},
 {"supplied_original_outcomes",false},{"source_caller_abi_captured",false}};try{
 check(argc==7,"Supply ELF,capture,census,FP mode,metadata,output");
 std::ifstream in(argv[2]),queries_in(argv[3]),metadata_in(argv[5]);const auto capture=J::parse(in),census=J::parse(queries_in),metadata=J::parse(metadata_in);
 const auto mode=std::uint32_t(std::stoul(argv[4]));check(mode<4,"Unqualified FP mode");J queries;
 for(const auto&context:census.at("contexts"))if(context.at("capture")==argv[2])queries=context.at("typed_queries");
 check(queries.is_array()&&!queries.empty(),"No declared query controls for authenticated BEFORE context");
 const auto actual_sp=capture.at("before").at("gp").at(4).get<std::uint32_t>(),
  private_first=capture.contains("private_returned_stack")?capture.at("private_returned_stack").at("first").get<std::uint32_t>():((actual_sp-1u)&~0xffffu),
  private_end=actual_sp,sp=actual_sp-0x1000u;check(sp>=private_first+0x2000u,"Owned argument storage unavailable");J cases=J::array();
 for(const auto&query:queries)for(unsigned kind=0;kind<2;++kind){
  const auto definition=query.at("definition_datum").get<std::uint32_t>(),event=query.at("event_type").get<std::uint32_t>(),
   source_material=query.at("source_material").get<std::uint32_t>(),key=query.at("material_key").get<std::uint32_t>();
  const bool secondary=query.at("secondary");Child original,native;original.start(argv[1],0);native.start(argv[1],0);
  auto before=original.fp();check(before.ftw==0,"Actual input requires empty x87");before.cwd=std::uint16_t((before.cwd&~0xc00u)|(mode<<10u));before.mxcsr=(before.mxcsr&~0x6000u)|(mode<<13u);
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
  put(sp,std::uint32_t(0x1ccacau));auto entry=original.regs();entry.rsp=sp;entry.rax=unsigned(secondary);
  if(kind==0){entry.rip=0x18af20u;entry.rcx=definition;entry.rdx=event;put(sp+4u,key);}
  else{entry.rip=0x18b0c0u;put(sp+4u,source_material);put(sp+8u,event);put(sp+12u,key);}
  request(PTRACE_SETREGS,original.pid,nullptr,&entry);ActivationMemory memory(native,private_first,private_end);J calls=J::array();std::size_t steps{};
  const auto ledger=activation_trace(original,metadata,memory,calls,steps);NativeTu15ConstraintEffectSelectorStats stats;
  const auto selected=kind==0?test_native_effect_definition_query(memory,definition,event,std::uint16_t(key),secondary,&stats):
   test_native_effect_material_query(memory,std::uint16_t(source_material),event,std::uint16_t(key),secondary,&stats);
  std::size_t compared{},different{};for(const auto&bank:capture.at("regions")){
   const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();const auto a=original.read(address,size),b=native.read(address,size);
   for(unsigned n=0;n<size;++n){const auto p=address+n;if(p>=private_first&&p<private_end)continue;++compared;if(a[n]!=b[n])++different;}}
  const auto source=original.regs();const bool ordered=ledger==memory.ledger,fp_equal=fp_words(original.fp())==fp_words(before),
   returned=std::uint32_t(source.rsp)==sp+(kind?16u:8u)&&std::uint32_t(source.rax)==selected;
  J differences=J::array();for(std::size_t n=0;n<std::min(ledger.size(),memory.ledger.size());++n)if(ledger[n]!=memory.ledger[n]&&differences.size()<8)
   differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"passed",ordered&&fp_equal&&returned&&different==0},{"query_kind",kind},{"query",query},
   {"ordered_operands_equal",ordered},{"first_operand_differences",differences},{"fp_equal",fp_equal},{"source_return_equal",returned},
   {"persistent_compared_bytes",compared},{"different_bytes",different},{"source_selected_row",std::uint32_t(source.rax)},
   {"native_selected_row",selected},{"positive_source_return",std::uint32_t(source.rax)!=0u},
   {"event_queries",stats.event_queries},{"event_fallbacks",stats.event_fallbacks},{"material_queries",stats.material_queries},
   {"material_rows",stats.material_rows},{"material_candidates",stats.material_candidates},{"selected_rows",stats.selected_rows},
   {"unknown_constant_targets",stats.unknown_constant_targets}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();report.update(J{{"passed",passed},{"cases",cases},{"declared_FP_mode",mode}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
