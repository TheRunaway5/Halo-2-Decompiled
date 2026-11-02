// Offline proof only. The native parents execute in this process. The sole
// remaining30E930 service executes genuine original code in a stopped child
// with ordinary typed arguments and independent poison in nonformal registers.
// No expected return, native solver implementation, or live fallback is here.
// The validator prepends the existing Child/Memory physical service utilities.
struct Boundary{};
void start_at(Child&child,const char*exe,unsigned ordinal){
 child.pid=fork();check(child.pid>=0,"fork failed");
 if(!child.pid){const int fd=open("/dev/null",O_WRONLY);dup2(fd,1);close(fd);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(exe,exe,nullptr);_exit(127);}
 child.wait();request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);child.wait();check(WSTOPSIG(child.status)==SIGTRAP,"Original frame bridge failed");
 for(unsigned n=0;n<=ordinal;++n){child.go(0x2f1a50u);if(n<ordinal)child.go(child.word(std::uint32_t(child.regs().rsp)));}
}
J fp_words(const user_fpregs_struct&f){return J::array({f.cwd,f.swd,f.mxcsr});}
std::string hex_bytes(std::span<const std::byte>raw){const char*d="0123456789abcdef";std::string s;for(auto b:raw){auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
}
int main(int argc,char**argv){J report{{"passed",false},{"profiles",J::array()},{"supplied_original_outcomes",false},{"remaining_services_execute_original_cpu",true},{"production_native_only_closure",false}};
try{
 check(argc==4,"Supply originalELF/capture/report");std::ifstream in(argv[2]);const auto capture=J::parse(in);
 const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>();
 for(unsigned ordinal=0;ordinal<2;++ordinal){
  Child original;start_at(original,argv[1],ordinal);const auto entry=original.regs();const auto source_fp=original.fp();check(source_fp.ftw==0,"Actual query proof requires empty incoming x87");
  const auto sp=std::uint32_t(entry.rsp),return_pc=original.word(sp),agent=std::uint32_t(entry.rcx);
  const std::array<std::uint32_t,4>args{original.word(sp+4u),original.word(sp+8u),original.word(sp+12u),original.word(sp+16u)};
  const auto header_before=original.word(args[3]);
  struct Bank{std::uint32_t first;std::vector<std::byte>before,after;};std::vector<Bank>banks;
  for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
  J solver_inputs=J::array();
  for(unsigned n=0;n<8;++n){const auto pc=original.go_any({0x30e930u,return_pc});if(pc==return_pc)break;const auto r=original.regs();const auto callsp=std::uint32_t(r.rsp),first=original.word(callsp+4u),second=original.word(callsp+8u),relative=original.word(callsp+12u),cache=original.word(callsp+16u),plane=original.word(callsp+20u);
   solver_inputs.push_back(J{{"first",first},{"second",second},{"cache",cache},{"relative",hex_bytes(original.read(relative,64))},{"flag",std::to_integer<unsigned>(original.read(std::uint32_t(r.rcx)+12u,1)[0])},{"before_fp",fp_words(original.fp())}});
   (void)plane;original.go(original.word(callsp));
  }
  check(original.regs().rip==return_pc&&std::uint32_t(original.regs().rsp)==sp+20u,"Genuine original query RET16 did not close");
  const auto expected_fp=original.fp();const auto expected_header=original.word(args[3]),expected_count=original.word(expected_header+4u),expected_backing=original.word(expected_header);check(expected_count<128,"Unbounded actual output");
  const auto expected_header_bytes=original.read(expected_header,20),expected_contacts=original.read(expected_backing,expected_count*48u);for(auto&b:banks)b.after=original.read(b.first,b.before.size());
  Child native;start_at(native,argv[1],ordinal);check(std::uint32_t(native.regs().rcx)==agent,"Independent original entry identity differs");check(fp_words(native.fp())==fp_words(source_fp),"Independent source entry FP differs");
  // Both processes independently reached the real parent entry. Do not install
  // a GP file or overwrite memory with the other process's later state.
  for(const auto&b:banks)check(native.read(b.first,b.before.size())==b.before,"Independent genuine BEFORE bytes differ");
  Memory memory(native);const auto data=private_first+0x8000u,call_top=private_first+0x6000u;
  check(data+0x500u<private_end&&call_top+0x1000u<data,"Native DATA aliases its real service stack");
  for(auto value:args)check(value<data||value>=data+0x500u,"Actual input aliases native owned DATA");
  const auto old_header=native.word(args[3]);check(old_header==header_before,"Actual output holder differs");
  NativeTu15PhysicsFrameFp fp{source_fp.cwd,source_fp.swd,source_fp.mxcsr};J calls=J::array();std::size_t solver_ordinal{};
  NativeTu15ConditionalContactStats conditional_stats;NativeTu15WrappedContactProcessStats wrapped_stats;NativeTu15ConvexOverlapQueryStats convex_stats;NativeTu15ReversedQueryProcessStats reversed_stats;
  NativeTu15CompoundProcessServices reversed;
  NativeTu15ConditionalContactServices conditional;
  NativeTu15ConvexOverlapQueryServices convex;
  convex.geometry.solve_simplex=[&](auto simplex,auto first,auto second,auto relative,auto cache,auto plane,auto&state)->std::uint32_t{
   check(solver_ordinal<solver_inputs.size(),"Unexpected native solver call");const auto&expected=solver_inputs[solver_ordinal];
   check(first.identity==expected.at("first")&&second.identity==expected.at("second")&&cache==expected.at("cache")&&hex_bytes(native.read(relative,64))==expected.at("relative").get<std::string>()&&std::to_integer<unsigned>(native.read(simplex+12u,1)[0])==expected.at("flag"),"Actual solver initialized input DATA differs");
   check(J::array({state.control_word,state.status,state.mxcsr})==expected.at("before_fp"),"Actual solver incoming FP differs");
   const auto call_sp=call_top-24u;std::array<std::byte,24>stack{};const std::array<std::uint32_t,6>words{0x2f849fu,first.identity,second.identity,relative,cache,plane};
   for(unsigned k=0;k<words.size();++k)for(unsigned j=0;j<4;++j)stack[k*4+j]=std::byte(words[k]>>(j*8u));native.write(call_sp,stack);
   auto r=native.regs();const auto poison=0x13579bdfu+unsigned(ordinal)*0x11111111u;
   r.rax=poison;r.rdx=poison^0xffffffffu;r.rbx=poison+7u;r.rbp=poison+13u;r.rsi=poison+19u;r.rdi=poison+23u;r.rcx=simplex;r.rsp=call_sp;r.rip=0x30e930u;r.eflags=2;
   request(PTRACE_SETREGS,native.pid,nullptr,&r);auto f=native.fp();f.cwd=state.control_word;f.swd=state.status;f.mxcsr=state.mxcsr;f.ftw=0;std::memset(f.st_space,0,sizeof(f.st_space));
   for(unsigned k=0;k<sizeof(f.xmm_space)/sizeof(f.xmm_space[0]);++k)f.xmm_space[k]=poison+k;
   request(PTRACE_SETFPREGS,native.pid,nullptr,&f);native.go(0x2f849fu);r=native.regs();f=native.fp();check(std::uint32_t(r.rsp)==call_top,"Original solver RET20 differs");check(f.ftw==0,"Original solver changed live empty x87 domain");state={f.cwd,f.swd,f.mxcsr};
   calls.push_back(J{{"owner",0x30e930u},{"first",first.identity},{"second",second.identity},{"cache",cache},{"return_word",std::uint32_t(r.rax)},{"return_fp",fp_words(f)},{"independent_nonformal_GP_XMM_poison",poison}});++solver_ordinal;return std::uint32_t(r.rax);
  };
  convex.collect=[&](auto collector,auto owner,auto,auto,auto&){check(owner.identity==0x30ba80u,"Actual query has unqualified collector owner");native_tu15_mark_contact_query_hit(memory,collector);};
  conditional.query=[&](auto receiver,auto owner,auto first,auto second,auto configuration,auto collector,auto&state){
   if(owner.identity==0x2f6f40u){native_tu15_query_wrapped_contact_agent(memory,receiver,{first.identity},second,configuration,collector,{{data+0x100u},data+0x110u},state,conditional,{},&wrapped_stats);return;}
   if(owner.identity==0x2f8410u){native_tu15_query_convex_overlap(memory,receiver,{first.identity},second,configuration,collector,{data+0x200u},state,0,convex,{},&convex_stats);return;}
   throw std::runtime_error("Actual conditional query owner is unqualified");
  };
  conditional.processing.process_child=[&](const auto&call,auto&){check(call.owner.identity==0x8eb50u,"Actual cached processing owner is unqualified");native_tu15_ignore_contact_process();};
  reversed.process_child=[&](const auto&call,auto&state){check(call.owner.identity==0x2f0f10u,"Actual reversed child owner differs");native_tu15_process_conditional_contact_agent(memory,call.receiver,{call.first.identity},call.second,call.configuration,call.output,{{data+0x10u},{data+0x28u}},state,conditional,{},&conditional_stats);};
  native_tu15_process_reversed_query_agent(memory,{agent},{args[0]},{args[1]},{args[2]},{args[3]},fp,reversed,{},&reversed_stats);
  check(solver_ordinal==solver_inputs.size()&&solver_ordinal>0,"Actual solver call counts differ");
  const auto actual_header=native.word(args[3]),actual_count=native.word(actual_header+4u),actual_backing=native.word(actual_header);
  check(actual_header==expected_header&&native.read(actual_header,20)==expected_header_bytes&&actual_count==expected_count&&native.read(actual_backing,actual_count*48u)==expected_contacts,"Actual native returned output header/contacts differ");
  std::size_t different{},raw_different{},persistent_compared{},compared{};J differences=J::array(),private_differences=J::array();
  for(const auto&b:banks){const auto after=native.read(b.first,b.after.size());compared+=after.size();for(std::size_t n=0;n<after.size();++n){const auto address=b.first+n;const bool private_control=address>=private_first&&address<private_end;if(!private_control)++persistent_compared;if(after[n]==b.after[n])continue;++raw_different;if(private_control){if(private_differences.size()<24)private_differences.push_back(J{{"address",address},{"native",std::to_integer<unsigned>(after[n])},{"original",std::to_integer<unsigned>(b.after[n])}});continue;}++different;if(differences.size()<64)differences.push_back(J{{"address",address},{"native",std::to_integer<unsigned>(after[n])},{"original",std::to_integer<unsigned>(b.after[n])}});}}
  J row{{"ordinal",ordinal},{"agent",agent},{"arguments",args},{"all_captured_bytes_compared_raw",compared},{"raw_different_bytes",raw_different},{"raw_all_bytes_green",raw_different==0},{"persistent_bytes_compared",persistent_compared},{"persistent_different_bytes",different},{"first_persistent_differences",differences},{"first_private_differences",private_differences},{"existing_void_frame_private_range",capture.at("private_returned_stack")},{"original_fp",fp_words(expected_fp)},{"native_fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"solver_calls",calls},{"reversed_completed",reversed_stats.completed},{"conditional_completed",conditional_stats.completed},{"wrapped_child_returns",wrapped_stats.child_returns},{"convex_completed",convex_stats.completed},{"native_collections",convex_stats.collect_calls},{"output_count",actual_count},{"passed",different==0&&fp_words(expected_fp)==J::array({fp.control_word,fp.status,fp.mxcsr})}};
  report["profiles"].push_back(row);check(row.at("passed"),"Actual native reversed-query composition differs");
 }
 report["passed"]=true;
}catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';std::cout<<J{{"passed",report.at("passed")},{"profiles",report["profiles"].size()},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
