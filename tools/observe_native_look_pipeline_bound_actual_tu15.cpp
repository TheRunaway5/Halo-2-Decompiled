// Offline proof suffix. Child/Memory utilities are prepended by the validator.
void start_look(Child&child,const char*exe){
 child.pid=fork();check(child.pid>=0,"Cannot fork actual look reference");
 if(!child.pid){const int fd=open("/dev/null",O_WRONLY);dup2(fd,1);close(fd);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(exe,exe,nullptr);_exit(127);}
 child.wait();request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);child.wait();check(WSTOPSIG(child.status)==SIGTRAP,"Original look caller failed");
}
J look_fp(const user_fpregs_struct&f){return J::array({f.cwd,f.swd,f.mxcsr});}
}
int main(int argc,char**argv){J report{{"passed",false},{"profiles",J::array()},{"remaining_services_execute_original_cpu",false},{"production_native_only_closure",false},{"supplied_original_outcomes",false}};
try{
 check(argc==4,"Supply originalELF/typedcapture/report");std::ifstream in(argv[2]);const auto capture=J::parse(in),typed=capture.at("input");const auto index=typed.at("control_index").get<std::uint32_t>();
 const std::array<std::array<std::uint32_t,2>,4>arguments{{{typed.at("yaw_delta_bits"),typed.at("pitch_delta_bits")},{0x3e800000u,0x3dcccccdu},{0xbf000000u,0xbdcccccdu},{0x40c90fdbu,0u}}};
 for(unsigned profile=0;profile<arguments.size();++profile){
  Child original;start_look(original,argv[1]);const auto before_fp=original.fp();check(before_fp.ftw==0,"Actual look requires its declared empty x87 domain");
  std::array<std::byte,8>args{};for(unsigned k=0;k<2;++k)for(unsigned j=0;j<4;++j)args[k*4+j]=std::byte(arguments[profile][k]>>(j*8u));original.write(0x0200f004u,args);
  struct Bank{std::uint32_t first;std::vector<std::byte>before,after;};std::vector<Bank>banks;
  for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
  struct Camera{std::uint32_t object,destination;J before_fp,after_fp;std::vector<std::byte>position;};std::vector<Camera>expected_cameras;
  for(unsigned call=0;call<8;++call){
   const auto pc=original.go_any({0xcd4a0u,0x05000000u});if(pc==0x05000000u)break;
   const auto r=original.regs();Camera camera{std::uint32_t(r.rcx),std::uint32_t(r.rax),look_fp(original.fp()),J{}, {}};
   original.go(original.word(std::uint32_t(r.rsp)));camera.after_fp=look_fp(original.fp());camera.position=original.read(camera.destination,12);expected_cameras.push_back(std::move(camera));
  }
  check(original.regs().rip==0x05000000u&&std::uint32_t(original.regs().rsp)==0x0200f00cu,"Original18A300 RET8 differs");const auto expected_fp=original.fp();for(auto&b:banks)b.after=original.read(b.first,b.before.size());
  constexpr auto source_context=0x0200f000u-0xb4u+0x2cu;
  const auto expected_context_object=original.word(source_context),expected_constraints=original.word(source_context+8u);
  const auto expected_seat=original.read(source_context+4u,2),expected_position=original.read(source_context+12u,12);
  Child native;start_look(native,argv[1]);for(const auto&b:banks)check(native.read(b.first,b.before.size())==b.before,"Independent actual BEFORE differs");Memory memory(native);
  NativeTu15PhysicsFrameFp fp{before_fp.cwd,before_fp.swd,before_fp.mxcsr};NativeTu15LookControllerStats stats;J camera_calls=J::array();
  NativeTu15LookPipelineActivity pipeline;
  native_tu15_update_bound_look_controller(memory,index,arguments[profile][0],arguments[profile][1],{{0x06000000u},{0x06000100u,{0x06000200u}}},{fp.control_word,fp.status,fp.mxcsr,0},true,{},&pipeline);
  stats=pipeline.controller;
  check(pipeline.attempts==1&&pipeline.completed==1&&pipeline.refusals==0&&pipeline.query_refusals==0&&pipeline.camera.completed==expected_cameras.size(),"Same production native look activity differs");
  check(expected_cameras.size()==1&&native.read(0x06000000u+0x2cu+12u,12)==expected_cameras[0].position,"Same production camera output differs");
  camera_calls.push_back(J{{"owner",0xcd4a0u},{"native_only",true},{"calls",pipeline.camera.calls},{"completed",pipeline.camera.completed},{"on_foot",pipeline.camera.on_foot},{"markers",pipeline.camera.markers},{"queries",pipeline.camera.segment_calls}});
  constexpr auto native_context=0x06000000u+0x2cu;
  check(native.word(native_context)==expected_context_object&&native.word(native_context+8u)==expected_constraints&&native.read(native_context+4u,2)==expected_seat&&native.read(native_context+12u,12)==expected_position,"Complete defined context DATA differs");
  std::size_t compared{},different{};J diffs=J::array();
  for(const auto&b:banks){const auto actual=native.read(b.first,b.after.size());compared+=actual.size();for(std::size_t k=0;k<actual.size();++k)if(actual[k]!=b.after[k]){++different;if(diffs.size()<64)diffs.push_back(J{{"address",b.first+k},{"native",std::to_integer<unsigned>(actual[k])},{"original",std::to_integer<unsigned>(b.after[k])}});}}
  const bool fp_equal=look_fp(expected_fp)==J::array({fp.control_word,fp.status,fp.mxcsr});const bool activity=stats.calls==1&&stats.completed==1&&stats.refusals==0&&stats.context.completed==1&&camera_calls.size()==expected_cameras.size();
  J row{{"profile",profile},{"captured_deltas",profile==0},{"arguments",arguments[profile]},{"all_captured_bytes_compared",compared},{"captured_bytes_excluded",0},{"different_bytes",different},{"first_differences",diffs},{"original_fp",look_fp(expected_fp)},{"native_fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"camera_calls",camera_calls},{"complete_defined_context_fields_equal",true},{"source_private_context_padding_claimed",false},{"native_parent_completed",stats.completed},{"marker_requests",stats.marker_requests},{"angle_calls",stats.angle_calls},{"passed",different==0&&fp_equal&&activity}};report["profiles"].push_back(row);check(row.at("passed"),"Actual native look return differs");
 }
 report["passed"]=true;
}catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';std::cout<<J{{"passed",report.at("passed")},{"profiles",report["profiles"].size()},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
