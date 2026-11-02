// The physical entry is marshalled from actual descriptor/body-slot DATA.
// It is not a captured caller ABI, and no after bank supplies an input.
#include <algorithm>
int main(int argc,char**argv){J report{{"passed",false},{"captured_after_consumed",false},
 {"instruction_patches",J::array()},{"manufactured_constraints",false},
 {"source_caller_abi_captured",false},{"typed_inputs_projected_from_existing_descriptor",true}};try{
 check(argc==7,"Supply ELF,capture,island,FP mode,metadata,output");
 std::ifstream input(argv[2]),meta_input(argv[5]);const auto capture=J::parse(input),metadata=J::parse(meta_input);
 const auto selection=std::uint32_t(std::stoul(argv[3])),mode=std::uint32_t(std::stoul(argv[4]));
 check(mode<4,"Unqualified FP mode");std::vector<std::uint32_t>datums;
 {Child source;source.start(argv[1],0);const auto world=source.word(0x52be48u),ordinal=selection&255u,
  bank=source.word(world+(selection&256u?0x14u:8u)),count=source.word(world+(selection&256u?0x18u:12u));
  check(ordinal<count,"Actual island unavailable");const auto island=source.word(bank+ordinal*4u),
  bodies=source.word(island+0x3cu),body_count=source.word(island+0x40u);
  check(body_count>0&&body_count<=64,"Actual body cohort unavailable");
  for(unsigned n=0;n<body_count;++n){const auto body=source.word(bodies+n*4u),properties=source.word(body+0x30u),
   property_count=source.word(body+0x34u);for(unsigned k=0;k<property_count;++k){
    if(source.word(properties+k*8u)!=0x2001u)continue;const auto component_datum=source.word(properties+k*8u+4u);
    if(component_datum!=0xffffffffu){const auto components=source.word(source.word(0x52be5cu)+0x44u),
     component=components+(component_datum&0xffffu)*160u,list=source.word(component+0x20u);
     if(list!=0xffffffffu){const auto row=source.word(source.word(0x52d690u)+0x44u)+(list&0xffffu)*64u;
      const auto list_count=std::bit_cast<std::int16_t>(std::uint16_t(source.word(row+2u)));
      check(list_count>=0&&list_count<=14,"Unbounded actual instance list");
      for(unsigned j=0;j<unsigned(list_count);++j){const auto datum=source.word(row+4u+j*4u);
       if(std::find(datums.begin(),datums.end(),datum)==datums.end())datums.push_back(datum);}
     }
    }break;
   }
  }
 }
 check(!datums.empty(),"No genuine existing instance in selected cohort");
 const auto actual_sp=capture.at("before").at("gp").at(4).get<std::uint32_t>(),
  private_first=capture.contains("private_returned_stack")?capture.at("private_returned_stack").at("first").get<std::uint32_t>():((actual_sp-1u)&~0xffffu),
  private_end=actual_sp,sp=actual_sp-0x1000u;check(sp>=private_first+0x2000u,"Owned service storage unavailable");
 J cases=J::array();
 for(const auto datum:datums)for(unsigned selector_mode=0;selector_mode<3;++selector_mode)for(unsigned depth=0;depth<4;++depth){
   Child original,native;original.start(argv[1],0);native.start(argv[1],0);auto before=original.fp();
   check(before.ftw==0,"Actual input requires empty x87 before declared depth qualification");
   before.cwd=std::uint16_t((before.cwd&~0xc00u)|(mode<<10u));before.mxcsr=(before.mxcsr&~0x6000u)|(mode<<13u);
   const std::array<long double,3>incoming{.75L,-.5L,1.25L};
   if(depth){const auto top=(8u-depth)&7u;before.swd=std::uint16_t((before.swd&~0x3800u)|(top<<11u));before.ftw=0;
    std::memset(before.st_space,0,sizeof(before.st_space));for(unsigned n=0;n<depth;++n){std::memcpy(reinterpret_cast<std::byte*>(before.st_space)+n*16u,&incoming[n],10);before.ftw|=1u<<((top+n)&7u);}}
   request(PTRACE_SETFPREGS,original.pid,nullptr,&before);const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
   const auto instance=native.word(native.word(0x52d68cu)+0x44u)+(datum&0xffffu)*160u;
   const auto first_key=std::uint16_t(native.word(instance+0x2cu)),second_key=std::uint16_t(native.word(instance+0x2eu));
   const auto output=sp-0x600u;const std::array<std::uint32_t,6>outputs{output,output+4u,output+8u,output+12u,output+16u,output+20u};
   put(sp,std::uint32_t(0x1ccacau));put(sp+4u,instance+0x68u);put(sp+8u,std::uint32_t(first_key));put(sp+12u,std::uint32_t(second_key));put(sp+16u,selector_mode);
   for(unsigned n=0;n<6;++n)put(sp+20u+n*4u,outputs[n]);
   auto entry=original.regs();entry.rsp=sp;entry.rip=0x22eec0u;entry.rcx=instance;request(PTRACE_SETREGS,original.pid,nullptr,&entry);
   ActivationMemory memory(native,private_first,private_end);J calls=J::array();std::size_t steps{};
   const auto ledger=activation_trace(original,metadata,memory,calls,steps);
   NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ConstraintEffectSelectorStats stats;
   native_tu15_select_constraint_effects(memory,{instance,instance+0x68u,first_key,second_key,std::int32_t(selector_mode),outputs},
    {sp-0x700u,sp-0x800u,sp-0x900u},fp,{},&stats);
   const bool values=original.read(output,24)==native.read(output,24);
   std::size_t compared{},different{};
   for(const auto&bank:capture.at("regions")){const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();
    const auto a=original.read(address,size),b=native.read(address,size);for(unsigned n=0;n<size;++n){const auto p=address+n;
     if(p>=private_first&&p<private_end)continue;++compared;if(a[n]!=b[n])++different;}}
   const auto after=original.fp();bool live_values=after.ftw==before.ftw;for(unsigned n=0;n<depth;++n)
    live_values&=std::memcmp(reinterpret_cast<const std::byte*>(after.st_space)+n*16u,reinterpret_cast<const std::byte*>(before.st_space)+n*16u,10)==0;
   const bool ordered=ledger==memory.ledger,fp_equal=fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr}),ret_equal=std::uint32_t(original.regs().rsp)==sp+44u;
   J differences=J::array();for(std::size_t n=0;n<std::min(ledger.size(),memory.ledger.size());++n)
    if(ledger[n]!=memory.ledger[n]&&differences.size()<8)differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
   cases.push_back(J{{"passed",ordered&&fp_equal&&ret_equal&&different==0&&values&&live_values},{"datum",datum},
    {"declared_selector_mode",selector_mode},{"declared_x87_depth",depth},{"ordered_operands_equal",ordered},{"first_operand_differences",differences},
    {"fp_equal",fp_equal},{"source_FP",fp_words(after)},{"native_FP",J::array({fp.control_word,fp.status,fp.mxcsr})},
    {"source_return_equal",ret_equal},{"values_equal",values},{"persistent_compared_bytes",compared},{"different_bytes",different},
    {"original_preserves_live_extended_values",live_values},{"source_operand_events",ledger.size()},{"native_operand_events",memory.ledger.size()},
    {"calls",stats.calls},{"secondary_components",stats.secondary_components},{"event_queries",stats.event_queries},
    {"event_fallbacks",stats.event_fallbacks},{"material_queries",stats.material_queries},{"material_rows",stats.material_rows},
    {"material_candidates",stats.material_candidates},{"selected_rows",stats.selected_rows},{"proximity_queries",stats.proximity_queries},
    {"camera_tests",stats.camera_tests},{"distance_refusals",stats.distance_refusals},
    {"location_calls",stats.location.location_calls},{"location_keys",stats.location.keys_selected},{"outside_bsp",stats.location.outside_bsp},
    {"unknown_constant_targets",stats.unknown_constant_targets}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();
 report.update(J{{"passed",passed},{"selection",selection},{"declared_FP_mode",mode},{"cases",cases}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}
 std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
