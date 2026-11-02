// Ordinary typed arguments on a genuine original TU1.5 view read closure.
// It is not a current MCC input or a captured caller ABI.
int main(int argc,char**argv){J report{{"passed",false},{"supplied_original_outcomes",false},{"manufactured_view_registration",false},
 {"current_MCC_live_input",false},{"captured_caller_abi",false}};try{
 check(argc==5,"Supply ELF,reference metadata,instructions,output");std::ifstream ref_in(argv[2]),meta_in(argv[3]);const auto reference=J::parse(ref_in),metadata=J::parse(meta_in);
 const auto view=reference.at("active_slots").at(0).get<std::uint32_t>(),camera=0x4f655cu+view*0x358u;
 J cases=J::array();
 for(unsigned mode=0;mode<4;++mode)for(unsigned type=0;type<21;++type)for(unsigned scaled=0;scaled<2;++scaled)for(unsigned projected=0;projected<3;++projected){
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);auto before=original.fp();before.cwd=std::uint16_t(0x27fu|(mode<<10u));before.swd=0;before.ftw=0;before.mxcsr=0x1f80u|(mode<<13u);
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
  constexpr std::uint32_t sp=0x6000f000u,point=0x60001000u;
  auto point_bytes=original.read(camera,12);
  if(projected){std::array<float,3>v{};if(projected==2){std::memcpy(v.data(),point_bytes.data(),12);v[0]+=10000.f;v[1]-=10000.f;v[2]+=10000.f;}
   std::memcpy(point_bytes.data(),v.data(),12);}
  original.write(point,point_bytes);native.write(point,point_bytes);
  put(sp,std::uint32_t(0x1ccacau));auto entry=original.regs();entry.rsp=sp;entry.rip=0x18b160u;entry.rax=type;request(PTRACE_SETREGS,original.pid,nullptr,&entry);
  ActivationMemory memory(native,0x60000000u,0x60010000u);J calls=J::array();std::size_t steps{};auto ledger=activation_trace(original,metadata,memory,calls,steps);
  NativeTu15ConstraintEffectSelectorStats stats;const auto radius=native_tu15_constraint_effect_radius(memory,type,&stats);
  const auto source_radius=original.fp().xmm_space[0];const bool radius_equal=radius==source_radius;
  put(sp,std::uint32_t(0x1ccacau));entry=original.regs();entry.rsp=sp;entry.rip=0x1755b0u;entry.rax=scaled;entry.rdi=point;request(PTRACE_SETREGS,original.pid,nullptr,&entry);
  const auto tail=activation_trace(original,metadata,memory,calls,steps);for(const auto&event:tail)ledger.push_back(event);
  NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};const auto near=native_tu15_constraint_effect_near_camera(memory,point,radius,bool(scaled),fp,&stats);
  const auto source=original.regs();const bool returned=(source.rax&255u)==unsigned(near)&&std::uint32_t(source.rsp)==sp+4u;
  const bool fp_equal=fp_words(original.fp())==J::array({fp.control_word,fp.status,fp.mxcsr}),ordered=ledger==memory.ledger;
  std::size_t compared{},different{};for(const auto&region:reference.at("regions")){
   const auto address=region.at("address").get<std::uint32_t>(),size=region.at("size").get<std::uint32_t>();const auto a=original.read(address,size),b=native.read(address,size);
   compared+=size;for(unsigned n=0;n<size;++n)if(a[n]!=b[n])++different;}
  J differences=J::array();for(std::size_t n=0;n<std::min(ledger.size(),memory.ledger.size());++n)if(ledger[n]!=memory.ledger[n]&&differences.size()<8)
   differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"passed",radius_equal&&returned&&fp_equal&&ordered&&different==0},{"declared_rounding_mode",mode},{"declared_event_type",type},
   {"declared_scaled_distance",scaled},{"declared_point_projection",projected},{"source_radius",source_radius},{"native_radius",radius},
   {"source_near",(source.rax&255u)!=0u},{"native_near",near},{"ordered_operands_equal",ordered},{"first_operand_differences",differences},
   {"fp_equal",fp_equal},{"source_FP",fp_words(original.fp())},{"native_FP",J::array({fp.control_word,fp.status,fp.mxcsr})},
   {"source_return_equal",returned},{"persistent_compared_bytes",compared},{"different_bytes",different},{"camera_tests",stats.camera_tests},
   {"unknown_constant_targets",stats.unknown_constant_targets}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();report.update(J{{"passed",passed},{"cases",cases},{"reference",reference}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
