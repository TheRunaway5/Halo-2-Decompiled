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
 for(const auto datum:datums)for(unsigned flag=0;flag<2;++flag)for(unsigned depth=0;depth<4;++depth){
   Child original,native;original.start(argv[1],0);native.start(argv[1],0);auto before=original.fp();
   check(before.ftw==0,"Actual input requires empty x87 before declared depth qualification");
   before.cwd=std::uint16_t((before.cwd&~0xc00u)|(mode<<10u));before.mxcsr=(before.mxcsr&~0x6000u)|(mode<<13u);
   const std::array<long double,3>incoming{.75L,-.5L,1.25L};
   if(depth){const auto top=(8u-depth)&7u;before.swd=std::uint16_t((before.swd&~0x3800u)|(top<<11u));before.ftw=0;
    std::memset(before.st_space,0,sizeof(before.st_space));for(unsigned n=0;n<depth;++n){std::memcpy(reinterpret_cast<std::byte*>(before.st_space)+n*16u,&incoming[n],10);before.ftw|=1u<<((top+n)&7u);}}
   request(PTRACE_SETFPREGS,original.pid,nullptr,&before);const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
   const auto instance=native.word(native.word(0x52d68cu)+0x44u)+(datum&0xffffu)*160u;
   ActivationMemory selector(native,private_first,private_end);const auto slots=native_tu15_constraint_body_slots(selector,instance,datum);
   const auto output=sp-0x600u;const NativeTu15ConstraintGeometryOutputs outputs{output,output+12u,output+24u,output+36u,output+48u,output+52u};
   put(sp,std::uint32_t(0x1ccacau));put(sp+4u,instance);put(sp+8u,datum);put(sp+12u,slots.first);
   put(sp+16u,outputs.first_axis);put(sp+20u,outputs.second_axis);put(sp+24u,outputs.first_extent);put(sp+28u,outputs.second_extent);put(sp+32u,flag);
   auto entry=original.regs();entry.rsp=sp;entry.rip=0x22e4f0u;entry.rax=outputs.normal;entry.rdi=outputs.position;request(PTRACE_SETREGS,original.pid,nullptr,&entry);
   // Include the real caller-owned output in the ordered ledger. Only the
   // callee's private call/temporary frames are excluded.
   ActivationMemory memory(native,sp-0x200u,private_end);J calls=J::array();std::size_t steps{};
   const auto ledger=activation_trace(original,metadata,memory,calls,steps);
   NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ConstraintGeometryStats stats;
   native_tu15_constraint_geometry_frame(memory,instance,datum,slots.first,outputs,flag,{sp-52u,sp-88u},fp,depth,{},&stats);
   const bool values=original.read(output,56)==native.read(output,56);
   std::size_t compared{},different{};
   for(const auto&bank:capture.at("regions")){const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();
    const auto a=original.read(address,size),b=native.read(address,size);for(unsigned n=0;n<size;++n){const auto p=address+n;
     if(p>=private_first&&p<private_end)continue;++compared;if(a[n]!=b[n])++different;}}
   const auto after=original.fp();bool live_values=after.ftw==before.ftw;for(unsigned n=0;n<depth;++n)
    live_values&=std::memcmp(reinterpret_cast<const std::byte*>(after.st_space)+n*16u,reinterpret_cast<const std::byte*>(before.st_space)+n*16u,10)==0;
   const bool ordered=ledger==memory.ledger,fp_equal=fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr}),ret_equal=std::uint32_t(original.regs().rsp)==sp+36u;
   J differences=J::array();for(std::size_t n=0;n<std::min(ledger.size(),memory.ledger.size());++n)
    if(ledger[n]!=memory.ledger[n]&&differences.size()<8)differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
   cases.push_back(J{{"passed",ordered&&fp_equal&&ret_equal&&different==0&&values&&live_values},{"datum",datum},
    {"first_body_slot",slots.first},{"declared_include_pending_points",flag},{"declared_x87_depth",depth},{"ordered_operands_equal",ordered},{"first_operand_differences",differences},
    {"fp_equal",fp_equal},{"source_FP",fp_words(after)},{"native_FP",J::array({fp.control_word,fp.status,fp.mxcsr})},
    {"source_return_equal",ret_equal},{"values_equal",values},{"persistent_compared_bytes",compared},{"different_bytes",different},
    {"original_preserves_live_extended_values",live_values},{"source_operand_events",ledger.size()},{"native_operand_events",memory.ledger.size()},
    {"calls",stats.calls},{"simple_axes",stats.simple_axes},{"alternate_axes",stats.alternate_axes},{"retained_node_frames",stats.retained_node_frames},{"measured_bounds",stats.measured_bounds},{"local_rows",stats.local_rows},{"matching_rows",stats.matching_rows},{"second_basis_selected",stats.second_basis_selected},{"basis_calls",stats.bases.calls},{"normalizations",stats.normalize.normalized+stats.bases.normalize.normalized},
    {"small_vectors",stats.normalize.small_vectors+stats.bases.normalize.small_vectors},{"unsupported_profiles",stats.unsupported_profiles+stats.normalize.unsupported_profiles+stats.bases.unsupported_profiles+stats.bases.normalize.unsupported_profiles}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();
 report.update(J{{"passed",passed},{"selection",selection},{"declared_FP_mode",mode},{"cases",cases}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}
 std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
