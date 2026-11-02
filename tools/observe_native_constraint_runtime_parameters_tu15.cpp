// Physical17DEC0 inputs are actual salt-bearing pool records or declared
// invalid datum arguments. No runtime/constraint input row is altered.
int main(int argc,char**argv){J report{{"passed",false},{"captured_after_consumed",false},
 {"instruction_patches",J::array()},{"manufactured_input_rows",false},{"source_caller_abi_captured",false}};try{
 check(argc==7,"Supply ELF,capture,selection,FP mode,metadata,output");
 std::ifstream input(argv[2]),meta_input(argv[5]);const auto capture=J::parse(input),metadata=J::parse(meta_input);
 const auto mode=std::uint32_t(std::stoul(argv[4]));check(mode<4,"Unqualified FP mode");
 std::vector<std::uint32_t>handles;std::uint32_t extent{};
 {Child source;source.start(argv[1],0);const auto pool=source.word(0x4f720cu),bank=source.word(pool+0x44u),stride=source.word(pool+0x24u);
  extent=source.word(pool+0x38u);check(extent>0&&extent<=4096,"Actual handle pool unavailable");
  for(unsigned index=0;index<extent;++index){const auto salt=source.word(bank+index*stride)&0xffffu;
   if(salt)handles.push_back((salt<<16u)|index);}
 }
 check(!handles.empty(),"No genuine runtime handles available");const auto genuine_count=handles.size();
 handles.push_back(0xffffffffu);handles.push_back((handles.front()&0xffff0000u)|extent);handles.push_back(handles.front()^0x10000u);
 const auto actual_sp=capture.at("before").at("gp").at(4).get<std::uint32_t>(),
  private_first=capture.contains("private_returned_stack")?capture.at("private_returned_stack").at("first").get<std::uint32_t>():((actual_sp-1u)&~0xffffu),
  private_end=actual_sp,sp=actual_sp-0x1000u;check(sp>=private_first+0x2000u,"Owned service storage unavailable");
 J cases=J::array();
 for(unsigned index=0;index<handles.size();++index)for(unsigned options=0;options<8;++options){
  const auto handle=handles[index];Child original,native;original.start(argv[1],0);native.start(argv[1],0);
  auto before=original.fp();check(before.ftw==0,"Actual input requires empty x87");
  before.cwd=std::uint16_t((before.cwd&~0xc00u)|(mode<<10u));before.mxcsr=(before.mxcsr&~0x6000u)|(mode<<13u);
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);
  const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
  const auto pool=native.word(0x4f720cu),bank=native.word(pool+0x44u),stride=native.word(pool+0x24u);
  const auto record=bank+(handles.front()&0xffffu)*stride,frame_pool=native.word(0x4f7208u),frame_bank=native.word(frame_pool+0x44u);
  std::uint32_t frame_node=0xffffffffu;
  for(unsigned k=0;k<0x20u;++k){const auto node=native.word(record+0x7cu+k*4u);if(node!=0xffffffffu){frame_node=node;break;}}
  check(frame_node!=0xffffffffu,"Genuine frame input unavailable");
  const auto frame=sp-0x600u;const auto frame_bytes=native.read(frame_bank+(frame_node&0xffffu)*0x3cu+8u,52);
  original.write(frame,frame_bytes);native.write(frame,frame_bytes);
  const NativeTu15ConstraintRuntimeParameterUpdate values{options&1u?record+0x30u:0u,options&2u?record+0x74u:0u,
   options&4u?frame:0u,native.word(record+0x6cu),native.word(record+0x70u)};
  put(sp,std::uint32_t(0x1ccacau));put(sp+4u,values.secondary_scalar_bits);put(sp+8u,values.frame);put(sp+12u,values.bounds);
  auto entry=original.regs();entry.rsp=sp;entry.rip=0x17dec0u;entry.rcx=handle;entry.rax=values.position;
  // The scalar is a declared ordinary argument, not a retained CPU outcome.
  auto source_fp=original.fp();std::memcpy(source_fp.xmm_space,&values.primary_scalar_bits,4);request(PTRACE_SETFPREGS,original.pid,nullptr,&source_fp);
  request(PTRACE_SETREGS,original.pid,nullptr,&entry);ActivationMemory memory(native,private_first,private_end);
  J calls=J::array();std::size_t steps{};const auto ledger=activation_trace(original,metadata,memory,calls,steps);
  NativeTu15ConstraintRuntimeParameterStats stats;const auto result=native_tu15_update_constraint_runtime_parameters(memory,handle,values,{},&stats);
  std::size_t compared{},different{};J byte_differences=J::array();
  for(const auto&bank:capture.at("regions")){const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();
   const auto a=original.read(address,size),b=native.read(address,size);for(unsigned n=0;n<size;++n){const auto p=address+n;
    if(p>=private_first&&p<private_end)continue;++compared;if(a[n]!=b[n]){++different;
     if(byte_differences.size()<8)byte_differences.push_back(J{{"address",p},{"source",std::to_integer<unsigned>(a[n])},{"native",std::to_integer<unsigned>(b[n])}});}}
  }
  const bool ordered=ledger==memory.ledger,fp_equal=fp_words(original.fp())==fp_words(before),
   ret_equal=std::uint32_t(original.regs().rsp)==sp+16u&&(std::uint32_t(original.regs().rax)&255u)==unsigned(result);
  J differences=J::array();for(std::size_t n=0;n<std::min(ledger.size(),memory.ledger.size());++n)
   if(ledger[n]!=memory.ledger[n]&&differences.size()<8)differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"passed",ordered&&fp_equal&&ret_equal&&different==0},
   {"handle",handle},{"genuine_salt_bearing_pool_handle",index<genuine_count},{"declared_optional_argument_mask",options},
   {"ordered_operands_equal",ordered},{"first_operand_differences",differences},{"fp_equal",fp_equal},
   {"source_return_equal",ret_equal},{"persistent_compared_bytes",compared},{"different_bytes",different},
   {"first_byte_differences",byte_differences},{"source_operand_events",ledger.size()},
   {"native_operand_events",memory.ledger.size()},{"source_instructions",steps},{"calls",stats.calls},{"none_skips",stats.none_skips},{"index_skips",stats.index_skips},
   {"salt_skips",stats.salt_skips},{"valid_handles",stats.valid_handles},{"positions",stats.positions},
   {"bounds",stats.bounds},{"frame_lists",stats.frame_lists},{"frame_nodes",stats.frame_nodes}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();
 report.update(J{{"passed",passed},{"declared_FP_mode",mode},{"cases",cases},
  {"private_returned_storage",J::array({private_first,private_end})}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}
 std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';
 return report.at("passed").get<bool>()?0:1;
}
