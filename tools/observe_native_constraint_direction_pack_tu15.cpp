// Native arithmetic versus actual122B60, with ordinary declared vector DATA.
int main(int argc,char**argv){J report{{"passed",false},{"captured_after_consumed",false},
 {"instruction_patches",J::array()},{"manufactured_constraints",false},
 {"source_caller_abi_captured",false},{"declared_vector_variants",true}};try{
 check(argc==7,"Supply ELF,capture,island,FP mode,metadata,output");
 std::ifstream input(argv[2]),meta_input(argv[5]);const auto capture=J::parse(input),metadata=J::parse(meta_input);
 const auto selection=std::uint32_t(std::stoul(argv[3])),mode=std::uint32_t(std::stoul(argv[4]));
 check(mode<4,"Unqualified FP mode");std::uint32_t datum=0xffffffffu,fallback{};std::array<std::uint32_t,3>actual{};
 {Child source;source.start(argv[1],0);const auto world=source.word(0x52be48u),ordinal=selection&255u,
  bank=source.word(world+(selection&256u?0x14u:8u)),count=source.word(world+(selection&256u?0x18u:12u));
  check(ordinal<count,"Actual island unavailable");const auto island=source.word(bank+ordinal*4u),
  bodies=source.word(island+0x3cu),body_count=source.word(island+0x40u);
  check(body_count>0&&body_count<=64,"Actual body cohort unavailable");
  for(unsigned n=0;n<body_count&&datum==0xffffffffu;++n){const auto body=source.word(bodies+n*4u),properties=source.word(body+0x30u),
   property_count=source.word(body+0x34u);for(unsigned k=0;k<property_count;++k){
    if(source.word(properties+k*8u)!=0x2001u)continue;const auto component_datum=source.word(properties+k*8u+4u);
    if(component_datum!=0xffffffffu){const auto components=source.word(source.word(0x52be5cu)+0x44u),
     component=components+(component_datum&0xffffu)*160u,list=source.word(component+0x20u);
     if(list!=0xffffffffu){const auto row=source.word(source.word(0x52d690u)+0x44u)+(list&0xffffu)*64u;
      if(std::bit_cast<std::int16_t>(std::uint16_t(source.word(row+2u)))>0)datum=source.word(row+4u);}
    }break;
   }
  }
  check(datum!=0xffffffffu,"No genuine descriptor vector available");
  const auto instance=source.word(source.word(0x52d68cu)+0x44u)+(datum&0xffffu)*160u;
  for(unsigned n=0;n<3;++n)actual[n]=source.word(instance+0x5cu+n*4u);
  fallback=source.word(0x474d10u);
 }
 const auto actual_sp=capture.at("before").at("gp").at(4).get<std::uint32_t>(),
  private_first=capture.contains("private_returned_stack")?capture.at("private_returned_stack").at("first").get<std::uint32_t>():((actual_sp-1u)&~0xffffu),
  private_end=actual_sp,sp=actual_sp-0x1000u,vector=sp-0x600u;
 check(vector>=private_first+0x1000u,"Owned ordinary vector DATA unavailable");
 J cases=J::array();const std::array<long double,7>incoming{.75L,-.5L,1.25L,2.5L,-4.75L,8.L,-1.375L};
 for(unsigned variant=0;variant<11;++variant)for(unsigned depth=0;depth<8;++depth){
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);
  std::array<std::uint32_t,3>value=actual;
  switch(variant){case 1:value={0,0,0};break;case 2:value={1,0,0};break;
   case 3:value={0x40400000u,0x40800000u,0};break;
   case 4:value={0x7fc12345u,0x3f800000u,0xbf800000u};break;
   case 5:value={0x7f812345u,0x3f800000u,0xbf800000u};break;
   case 6:value={0x7f800000u,0x3f800000u,0xbf800000u};break;
   case 7:value={original.word(0x46a0ecu),0,0};break;
   case 8:value={0x80000000u,0,0x80000000u};break;
   case 9:value={0x7f7fffffu,0x7f7fffffu,0x7f7fffffu};break;
   case 10:value={0x00800000u,0x007fffffu,0x80800000u};break;default:break;}
  const auto put=[&](auto p,auto v){original.write(p,{reinterpret_cast<const std::byte*>(&v),4});};
  for(unsigned n=0;n<3;++n){put(vector+n*4u,value[n]);native.write(vector+n*4u,{reinterpret_cast<const std::byte*>(&value[n]),4});}
  auto before=original.fp();check(before.ftw==0,"Actual input requires empty x87 before declared depth qualification");
  before.cwd=std::uint16_t((before.cwd&~0xc00u)|(mode<<10u));before.mxcsr=(before.mxcsr&~0x6000u)|(mode<<13u);
  if(depth){const auto top=(8u-depth)&7u;before.swd=std::uint16_t((before.swd&~0x3800u)|(top<<11u));before.ftw=0;
   std::memset(before.st_space,0,sizeof(before.st_space));for(unsigned n=0;n<depth;++n){
    std::memcpy(reinterpret_cast<std::byte*>(before.st_space)+n*16u,&incoming[n],10);before.ftw|=1u<<((top+n)&7u);}}
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);put(sp,std::uint32_t(0x1ccacau));
  auto entry=original.regs();entry.rsp=sp;entry.rip=0x18bbc0u;entry.rax=vector;entry.rcx=fallback;
  request(PTRACE_SETREGS,original.pid,nullptr,&entry);
  // Ordinary vector DATA below the service frame participates in the ordered
  // ledger; only the real temporary call/length stack bytes are excluded.
  ActivationMemory memory(native,sp-0x100u,private_end);J calls=J::array();std::size_t steps{};
  const auto ledger=activation_trace(original,metadata,memory,calls,steps);
  NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ConstraintAuxiliaryMathStats stats;
  const auto length=native_tu15_pack_constraint_direction(memory,vector,fp,depth,&stats);
  const auto after=original.fp();const bool values=original.read(vector,12)==native.read(vector,12)&&length==std::uint32_t(original.regs().rax);
  bool live_values=after.ftw==before.ftw;for(unsigned n=0;n<depth;++n)
   live_values&=std::memcmp(reinterpret_cast<const std::byte*>(after.st_space)+n*16u,reinterpret_cast<const std::byte*>(before.st_space)+n*16u,10)==0;
  const bool ordered=ledger==memory.ledger,fp_equal=fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr}),
   ret_equal=std::uint32_t(original.regs().rsp)==sp+4u;
  J differences=J::array();for(std::size_t n=0;n<std::min(ledger.size(),memory.ledger.size());++n)
   if(ledger[n]!=memory.ledger[n]&&differences.size()<8)differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"passed",ordered&&fp_equal&&ret_equal&&values&&live_values},{"datum",datum},
   {"declared_vector_variant",variant},{"declared_x87_depth",depth},{"vector_bits",value},
   {"ordered_operands_equal",ordered},{"first_operand_differences",differences},{"fp_equal",fp_equal},
   {"source_FP",fp_words(after)},{"native_FP",J::array({fp.control_word,fp.status,fp.mxcsr})},
   {"source_return_equal",ret_equal},{"values_equal",values},{"source_packed",std::uint32_t(original.regs().rax)},
   {"native_packed",length},{"source_vector",activation_hex(original.read(vector,12))},
   {"native_vector",activation_hex(native.read(vector,12))},{"original_preserves_live_extended_values",live_values},
   {"source_instructions",steps},{"pack_calls",stats.pack_calls},{"unsupported_profiles",stats.unsupported_profiles}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();
 report.update(J{{"passed",passed},{"selection",selection},{"declared_FP_mode",mode},{"cases",cases},
  {"private_returned_storage",J::array({private_first,private_end})}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}
 std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';
 return report.at("passed").get<bool>()?0:1;
}
