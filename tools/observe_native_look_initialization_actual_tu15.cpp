// Offline constructor comparison. Child and exact-bank access are prepended.
void start_constructor(Child&child,const char*exe,std::uint32_t index,std::uint32_t unit,unsigned profile){
 child.pid=fork();check(child.pid>=0,"Cannot fork actual-bank constructor reference");
 if(!child.pid){const int fd=open("/dev/null",O_WRONLY);dup2(fd,1);close(fd);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(exe,exe,nullptr);_exit(127);}
 child.wait();request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);child.wait();check(WSTOPSIG(child.status)==SIGTRAP,"Constructor formal caller failed");
 auto r=child.regs();r.rax=index;r.rcx=unit;r.rdx=0x13579bdfu+profile;r.rbx=0x2468ace1u+profile;r.rbp=0x31415926u+profile;r.rsi=0x57721566u+profile;r.rdi=0x16180339u+profile;r.eflags=2;request(PTRACE_SETREGS,child.pid,nullptr,&r);
}
}
int main(int argc,char**argv){J report{{"passed",false},{"profiles",J::array()},{"captured_constructor_invocation",false},{"original_children_selected_by_native",false},{"supplied_original_outcomes",false}};
try{
 check(argc==4,"Supply originalELF/typedLookcapture/report");std::ifstream in(argv[2]);const auto capture=J::parse(in);const auto unit=capture.at("caller").at("unit").get<std::uint32_t>();
 for(unsigned profile=0;profile<8;++profile){
  const auto index=profile%4;Child original;start_constructor(original,argv[1],index,unit,profile);auto before_fp=original.fp();
  if(profile>=4){before_fp.cwd=(before_fp.cwd&~0xc00u)|((index&3u)<<10u);before_fp.mxcsr=(before_fp.mxcsr&~0x6000u)|((index&3u)<<13u);request(PTRACE_SETFPREGS,original.pid,nullptr,&before_fp);}
  check(before_fp.ftw==0,"Constructor proof requires empty incoming x87");
  struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;
  for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
  original.go(0x05000000u);check(std::uint32_t(original.regs().rsp)==0x0200f004u,"Original constructor RET0 differs");const auto after_fp=original.fp();for(auto&b:banks)b.after=original.read(b.address,b.before.size());
  Child native;start_constructor(native,argv[1],index,unit,profile^0x5555u);for(const auto&b:banks)check(native.read(b.address,b.before.size())==b.before,"Independent constructor BEFORE differs");Memory memory(native);
  NativeTu15PhysicsFrameFp fp{before_fp.cwd,before_fp.swd,before_fp.mxcsr};
#if defined(TEST_PRODUCTION_BINDING)
  NativeTu15LookInitializationBindingActivity activity;auto&stats=activity.initialization;
  native_tu15_initialize_bound_look_controller(memory,index,unit,{fp.control_word,fp.status,fp.mxcsr,0},true,{},&activity);
  check(activity.attempts==1&&activity.completed==1&&activity.refusals==0,"Production constructor activity differs");
#else
  NativeTu15LookInitializationStats stats;native_tu15_initialize_look_controller(memory,index,unit,fp,0,{},&stats);
#endif
  std::size_t compared{},different{};J first=J::array();for(const auto&b:banks){const auto actual=native.read(b.address,b.after.size());compared+=actual.size();for(std::size_t k=0;k<actual.size();++k)if(actual[k]!=b.after[k]){++different;if(first.size()<16)first.push_back(J{{"address",b.address+k},{"native",std::to_integer<unsigned>(actual[k])},{"original",std::to_integer<unsigned>(b.after[k])}});}}
  const J expected_fp=J::array({after_fp.cwd,after_fp.swd,after_fp.mxcsr}),actual_fp=J::array({fp.control_word,fp.status,fp.mxcsr});const bool passed=different==0&&actual_fp==expected_fp&&stats.calls==1&&stats.completed==1&&stats.refusals==0;
  report["profiles"].push_back(J{{"passed",passed},{"index",index},{"unit",unit},{"declared_formal_constructor_control",true},{"all_captured_bytes_compared",compared},{"captured_bytes_excluded",0},{"different_bytes",different},{"first_differences",first},{"before_fp",J::array({before_fp.cwd,before_fp.swd,before_fp.mxcsr})},{"original_fp",expected_fp},{"native_fp",actual_fp},{"cleared_records",stats.cleared_records},{"unit_directions",stats.unit_directions},{"unchanged",stats.unchanged}});check(passed,"Actual-bank native constructor differs");
 }
 report["passed"]=true;
}catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';std::cout<<J{{"passed",report.at("passed")},{"profiles",report["profiles"].size()},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
