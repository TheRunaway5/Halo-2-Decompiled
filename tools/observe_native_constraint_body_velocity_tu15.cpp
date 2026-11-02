// Physical1D8C80 on actual body slots; the output is private DATA.
#include <algorithm>
int main(int argc,char**argv){J report{{"passed",false},{"captured_after_consumed",false},{"instruction_patches",J::array()},{"native_parent_receives_guest_register_state",false},{"manufactured_constraints",false},{"typed_body_inputs",true}};try{
 check(argc==7,"Supply ELF,capture,island,FP mode,metadata,output");std::ifstream input(argv[2]),meta_input(argv[5]);const auto capture=J::parse(input),metadata=J::parse(meta_input);const auto selection=std::uint32_t(std::stoul(argv[3])),mode=std::uint32_t(std::stoul(argv[4]));check(mode<4,"Unqualified FP mode");std::vector<std::uint32_t>components;
 {Child source;source.start(argv[1],0);const auto world=source.word(0x52be48u),ordinal=selection&255u,bank=source.word(world+(selection&256u?0x14u:8u)),count=source.word(world+(selection&256u?0x18u:12u));check(ordinal<count,"Actual island unavailable");const auto island=source.word(bank+ordinal*4u),bodies=source.word(island+0x3cu),body_count=source.word(island+0x40u);check(body_count>0&&body_count<=64,"Actual body cohort unavailable");for(unsigned n=0;n<body_count;++n){const auto body=source.word(bodies+n*4u),properties=source.word(body+0x30u),property_count=source.word(body+0x34u);for(unsigned k=0;k<property_count;++k)if(source.word(properties+k*8u)==0x2001u){const auto datum=source.word(properties+k*8u+4u);if(datum!=0xffffffffu){const auto manager=source.word(0x52be5cu),component=source.word(manager+0x44u)+(datum&0xffffu)*160u;if(std::find(components.begin(),components.end(),component)==components.end())components.push_back(component);}break;}}}
 check(!components.empty(),"Actual components unavailable");
 const auto actual_sp=capture.at("before").at("gp").at(4).get<std::uint32_t>(),
  private_first=capture.contains("private_returned_stack")?capture.at("private_returned_stack").at("first").get<std::uint32_t>():((actual_sp-1u)&~0xffffu),
  private_end=actual_sp,sp=actual_sp-0x1000u;check(sp>=private_first+0x2000u,"Owned service storage unavailable");J cases=J::array();
 for(const auto component:components){unsigned slots{};{Child source;source.start(argv[1],0);slots=source.word(component+0x74u);}
  check(slots>0&&slots<=64,"Unbounded actual body cohort");for(unsigned slot=0;slot<slots;++slot){
   Child original,native;original.start(argv[1],0);native.start(argv[1],0);auto before=original.fp();
   check(before.ftw==0,"Actual input requires empty x87");before.cwd=std::uint16_t((before.cwd&~0xc00u)|(mode<<10u));before.mxcsr=(before.mxcsr&~0x6000u)|(mode<<13u);
   request(PTRACE_SETFPREGS,original.pid,nullptr,&before);const auto put=[&](auto p,auto v){original.write(p,{reinterpret_cast<const std::byte*>(&v),4});};
   put(sp,std::uint32_t(0x1ccacau));auto entry=original.regs();entry.rsp=sp;entry.rip=0x1d8c80u;entry.rax=slot;entry.rdx=component;entry.rcx=sp-0x400u;
   request(PTRACE_SETREGS,original.pid,nullptr,&entry);ActivationMemory memory(native,private_first,private_end);
   J calls=J::array();std::size_t steps{};const auto ledger=activation_trace(original,metadata,memory,calls,steps);
   NativeTu15ConstraintAuxiliaryMathStats stats;const auto velocity=native_tu15_constraint_body_linear_velocity(memory,{component},slot,&stats);
   const auto expected=original.read(sp-0x400u,12);const bool values=std::memcmp(expected.data(),velocity.data(),12)==0;
   std::size_t compared{},different{};
   for(const auto&bank:capture.at("regions")){const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();
    const auto a=original.read(address,size),b=native.read(address,size);for(unsigned n=0;n<size;++n){const auto p=address+n;
     if(p>=private_first&&p<private_end)continue;++compared;if(a[n]!=b[n])++different;}}
   const bool ordered=ledger==memory.ledger,fp_equal=fp_words(original.fp())==fp_words(before),ret_equal=std::uint32_t(original.regs().rsp)==sp+4u;
   J differences=J::array();for(std::size_t n=0;n<std::min(ledger.size(),memory.ledger.size());++n)
    if(ledger[n]!=memory.ledger[n]&&differences.size()<8)differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
   cases.push_back(J{{"passed",ordered&&fp_equal&&ret_equal&&values&&different==0},{"component",component},{"slot",slot},
    {"ordered_operands_equal",ordered},{"first_operand_differences",differences},{"fp_equal",fp_equal},{"values_equal",values},
    {"source_return_equal",ret_equal},{"persistent_compared_bytes",compared},{"different_bytes",different},
    {"velocity_bits",velocity},{"body_velocity_calls",stats.body_velocity_calls},{"default_velocities",stats.default_velocities}});
  }
 }
 check(!cases.empty(),"No actual body velocity input");bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();
 report.update(J{{"passed",passed},{"selection",selection},{"declared_FP_mode",mode},{"cases",cases}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}
 std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
