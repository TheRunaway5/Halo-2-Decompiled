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
 for(const auto datum:datums){Child original,native;original.start(argv[1],0);native.start(argv[1],0);
  const auto instance=native.word(native.word(0x52d68cu)+0x44u)+(datum&0xffffu)*160u;
  ActivationMemory selector(native,private_first,private_end);
  const auto retained_slots=native_tu15_constraint_body_slots(selector,instance,datum);
  auto slots=retained_slots;
  // Dormant real descriptors can have no current resolved slot. For this
  // component proof only, declare slot0 from the same genuine component's
  // real body bank as a typed call input. No descriptor/request row changes.
  const auto component_rows=native.word(native.word(0x52be5cu)+0x44u);
  const auto first_component=component_rows+(native.word(instance+0x24u)&0xffffu)*160u;
  const auto first_count=native.word(first_component+0x74u);
  check(first_count>0,"Descriptor has no genuine first-component body");
  if(slots.first>=first_count)slots.first=0;
  const auto second_component_datum=native.word(instance+0x28u);
  if(second_component_datum!=0xffffffffu){
   const auto second_component=component_rows+(second_component_datum&0xffffu)*160u;
   const auto second_count=native.word(second_component+0x74u);
   check(second_count>0,"Descriptor has no genuine second-component body");
   if(slots.second>=second_count)slots.second=0;
  }
  auto before=original.fp();check(before.ftw==0,"Actual input requires empty x87");
  if(mode){before.cwd=std::uint16_t((before.cwd&~0xc00u)|(mode<<10u));before.mxcsr=(before.mxcsr&~0x6000u)|(mode<<13u);}
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);
  const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
  put(sp,std::uint32_t(0x1ccacau));put(sp+4u,slots.first);put(sp+8u,slots.second);
  auto entry=original.regs();entry.rsp=sp;entry.rip=0x22eba0u;entry.rax=instance;
  request(PTRACE_SETREGS,original.pid,nullptr,&entry);ActivationMemory memory(native,private_first,private_end);
  J calls=J::array();std::size_t steps{};const auto ledger=activation_trace(original,metadata,memory,calls,steps);
  NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ConstraintLocalPointStats stats;
  native_tu15_update_constraint_local_points(memory,instance,slots,{sp-0x600u,sp-0x500u},fp,{},&stats);
  std::size_t compared{},different{};J byte_differences=J::array();
  for(const auto&bank:capture.at("regions")){const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();
   const auto a=original.read(address,size),b=native.read(address,size);for(unsigned n=0;n<size;++n){const auto p=address+n;
    if(p>=private_first&&p<private_end)continue;++compared;if(a[n]!=b[n]){++different;
     if(byte_differences.size()<8)byte_differences.push_back(J{{"address",p},{"source",std::to_integer<unsigned>(a[n])},{"native",std::to_integer<unsigned>(b[n])}});}}
  }
  const bool ordered=ledger==memory.ledger,fp_equal=fp_words(original.fp())==J::array({fp.control_word,fp.status,fp.mxcsr}),
   ret_equal=std::uint32_t(original.regs().rsp)==sp+12u;
  J differences=J::array();for(std::size_t n=0;n<std::min(ledger.size(),memory.ledger.size());++n)
   if(ledger[n]!=memory.ledger[n]&&differences.size()<8)differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"passed",ordered&&fp_equal&&ret_equal&&different==0},{"datum",datum},{"instance",instance},
   {"first_slot",slots.first},{"second_slot",slots.second},{"retained_first_slot",retained_slots.first},
   {"retained_second_slot",retained_slots.second},{"declared_body_slot_projection",slots.first!=retained_slots.first||slots.second!=retained_slots.second},
   {"ordered_operands_equal",ordered},
   {"first_operand_differences",differences},{"fp_equal",fp_equal},{"source_return_equal",ret_equal},
   {"persistent_compared_bytes",compared},{"different_bytes",different},{"first_byte_differences",byte_differences},
   {"source_operand_events",ledger.size()},{"native_operand_events",memory.ledger.size()},{"source_instructions",steps},
   {"calls",stats.calls},{"retained_node_positions",stats.retained_node_positions},{"first_points",stats.first_points},
   {"second_points",stats.second_points},{"inverse_frames",stats.inverse_frames},{"body_frame_calls",stats.frames.body_frame_calls}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();
 report.update(J{{"passed",passed},{"selection",selection},{"declared_FP_mode",mode},{"cases",cases},
  {"private_returned_storage",J::array({private_first,private_end})}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}
 std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';
 return report.at("passed").get<bool>()?0:1;
}
