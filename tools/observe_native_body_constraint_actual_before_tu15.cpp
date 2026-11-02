// Original22D460 is invoked through a declared ordinary formal ABI on genuine
// frame-level BEFORE banks. No original22D460 caller ABI was captured.
int main(int argc,char**argv){J report{{"passed",false},{"captured_guest_ABI",false},
 {"captured_after_consumed",false},{"supplied_original_outcomes",false},
 {"manufactured_rows",false},{"native_parent_falls_back",false}};try{
 check(argc==5,"Supply ELF,capture,instructions,output");std::ifstream capture_in(argv[2]),meta_in(argv[3]);
 const auto capture=J::parse(capture_in),metadata=J::parse(meta_in);const auto datum=capture.at("component").at("datum").get<std::uint32_t>();
 constexpr std::uint32_t first=0x72000000u,end=0x72010000u,sp=0x7200f000u,scratch=0x72001000u;
 const auto&recorded=capture.at("floating_point");J cases=J::array();
 for(unsigned mode=0;mode<4;++mode)for(unsigned depth=0;depth<4;++depth){
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);auto before=original.fp();
  before.cwd=std::uint16_t((recorded.at("x87_control_word").get<unsigned>()&~0xc00u)|(mode<<10u));
  before.swd=recorded.at("x87_status").get<unsigned>();before.mxcsr=(recorded.at("mxcsr").get<unsigned>()&~0x6000u)|(mode<<13u);
  before.ftw=0;std::memset(before.st_space,0,sizeof(before.st_space));const std::array<long double,3>incoming{.75L,-.5L,1.25L};
  if(depth){const auto top=(8u-depth)&7u;before.swd=std::uint16_t((before.swd&~0x3800u)|(top<<11u));
   for(unsigned n=0;n<depth;++n){std::memcpy(reinterpret_cast<std::byte*>(before.st_space)+n*16u,&incoming[n],10);before.ftw|=1u<<((top+n)&7u);}}
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);
  const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
  put(sp,std::uint32_t(0x1ccacau));put(sp+4u,datum);auto entry=original.regs();entry.rip=0x22d460u;entry.rsp=sp;
  request(PTRACE_SETREGS,original.pid,nullptr,&entry);ActivationMemory memory(native,first,end);J calls=J::array();std::size_t steps{};
  const auto ledger=activation_trace(original,metadata,memory,calls,steps);
  NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ModelConstraintUpdateStats parent;
  NativeTu15ModelConstraintLookupStats lookup;NativeTu15ModelConstraintCapacityStats capacity;NativeTu15ModelConstraintCreationStats creation;
  NativeTu15ModelConstraintServices services;unsigned unbound_removals{},unbound_runtime{};
  services.find_existing=[&](auto component,auto request,auto flags,auto&state){return native_tu15_find_model_constraint(memory,component,request,flags,state,&lookup);};
  services.allow_creation=[&](auto component,auto slot,auto extent,auto&state){NativeTu15ModelConstraintCapacityServices required;
   required.remove_constraint=[&](auto,auto&){++unbound_removals;throw std::runtime_error("Actual positive constraint requires saturated removal qualification");};
   return native_tu15_admit_model_constraint(memory,component,slot,extent,state,required,{},&capacity);};
  services.create=[&](auto request,auto flags,auto&state){NativeTu15ModelConstraintCreationServices required;
   required.reference.remove_runtime_constraint=[&](auto,auto,auto,auto&){++unbound_runtime;throw std::runtime_error("Actual positive creation requires runtime lifetime qualification");};
   return native_tu15_create_model_constraint(memory,request,flags,{scratch+64u,scratch+128u},state,depth,required,{},&creation);};
  native_tu15_update_body_constraints(memory,datum,{{scratch}},fp,services,{},&parent);
  std::size_t compared{},different{};J byte_differences=J::array();for(const auto&bank:capture.at("regions")){
   const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();
   const auto a=original.read(address,size),b=native.read(address,size);compared+=size;for(unsigned n=0;n<size;++n)if(a[n]!=b[n]){++different;
    if(byte_differences.size()<8)byte_differences.push_back(J{{"address",address+n},{"original",std::to_integer<unsigned>(a[n])},{"native",std::to_integer<unsigned>(b[n])}});}}
  const auto after=original.fp();bool live=after.ftw==before.ftw;for(unsigned n=0;n<depth;++n)
   live&=std::memcmp(reinterpret_cast<const std::byte*>(after.st_space)+n*16u,reinterpret_cast<const std::byte*>(before.st_space)+n*16u,10)==0;
  const bool ordered=ledger==memory.ledger,fp_equal=fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr}),returned=std::uint32_t(original.regs().rsp)==sp+8u;
  J differences=J::array();for(std::size_t n=0;n<std::min(ledger.size(),memory.ledger.size());++n)if(ledger[n]!=memory.ledger[n]&&differences.size()<8)
   differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"passed",ordered&&fp_equal&&returned&&different==0&&live&&!unbound_removals&&!unbound_runtime},
   {"component_datum",datum},{"declared_rounding_mode",mode},{"declared_x87_depth",depth},{"actual_captured_x87_depth",0},
   {"ordered_operands_equal",ordered},{"first_operand_differences",differences},{"fp_equal",fp_equal},{"source_FP",fp_words(after)},
   {"native_FP",J::array({fp.control_word,fp.status,fp.mxcsr})},{"source_RET4_equal",returned},{"original_preserves_live_extended_values",live},
   {"persistent_compared_bytes",compared},{"different_bytes",different},{"first_byte_differences",byte_differences},
   {"source_operand_events",ledger.size()},{"native_operand_events",memory.ledger.size()},{"source_instructions",steps},
   {"parent_calls",parent.calls},{"authored_rows",parent.rows},{"requests",parent.requests},{"existing",parent.existing},
   {"parent_created",parent.created},{"appends",parent.appends},{"list_allocations",parent.list_allocations},{"published",parent.published},
   {"frame_compositions",parent.frame_compositions},{"reference_distance_points",parent.reference_distance_points},
   {"lookup_calls",lookup.calls},{"lookup_candidates",lookup.candidates},{"lookup_matches",lookup.found},
   {"capacity_calls",capacity.calls},{"capacity_refusals",capacity.refusals},{"creation_calls",creation.calls},
   {"allocations",creation.allocations},{"created",creation.created},{"second_definitions",creation.second_definitions},
   {"local_point_updates",creation.local_point_updates},{"reference_releases",creation.reference_releases},
   {"reference_decrements",creation.reference.decrements},{"inverse_frames",creation.points.inverse_frames},
   {"first_points",creation.points.first_points},{"second_points",creation.points.second_points},
   {"normalizations",creation.normalize.normalized},{"unbound_removals",unbound_removals},{"unbound_runtime",unbound_runtime}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();report.update(J{{"passed",passed},{"cases",cases},
  {"genuine_frame_level_component",true},{"declared_original_formal_ABI",J{{"entry",0x22d460u},{"arguments",J::array({datum})},{"ESP",sp},{"RET_bytes",4}}},
  {"original22D460_caller_captured",false},{"native_services_execute_original_CPU",false}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
