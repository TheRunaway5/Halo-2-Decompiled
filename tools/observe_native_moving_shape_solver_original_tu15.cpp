// Read-only original execution census. No result is returned to any native kernel.
int main(int argc,char**argv){J report{{"passed",false},{"profiles",J::array()},{"native_results_supplied",false}};try{
 check(argc==5,"Supply original ELF/typed PROCESS/metadata/report");std::ifstream input(argv[2]);const auto capture=J::parse(input);const auto&typed=capture.at("input");
 for(unsigned profile=0;profile<4;++profile){
  Child child;start_process(child,argv[1],typed,profile);auto initial=child.fp();if(profile){initial.cwd=(initial.cwd&~0xc00u)|(profile<<10u);initial.mxcsr=(initial.mxcsr&~0x6000u)|(profile<<13u);request(PTRACE_SETFPREGS,child.pid,nullptr,&initial);}
  child.step();J calls=J::array(),virtuals=J::array();std::map<std::uint32_t,std::string>executed;unsigned instructions{};bool solver_entered{};std::uint32_t result{};
  for(;;){const auto regs=child.regs();const auto pc=std::uint32_t(regs.rip);if(solver_entered&&pc==0x2f8cc2u){result=std::uint32_t(regs.rax);break;}check(++instructions<200000u,"Original simplex census exceeded finite instruction budget");if(pc==0x30e930u)solver_entered=true;
   const auto opcode=child.read(pc,15);if(solver_entered)executed.try_emplace(pc,process_hex(opcode));
   const auto byte=[&](unsigned n){return std::to_integer<unsigned>(opcode[n]);};const bool call=byte(0)==0xe8u||(byte(0)==0xffu&&((byte(1)>>3u)&7u)==2u);
   if(solver_entered&&(pc==0x30e96du||pc==0x30e985u||pc==0x30eeaeu||pc==0x30f312u||pc==0x30f48eu)){
    const auto shape=std::uint32_t(regs.rcx),sp=std::uint32_t(regs.rsp);const bool batch=pc==0x30e96du||pc==0x30e985u;const auto args=child.read(sp,batch?12u:8u);J values=J::array();for(unsigned n=0;n<args.size();n+=4){std::uint32_t value{};std::memcpy(&value,args.data()+n,4);values.push_back(value);}const auto table_bytes=child.read(shape,4);std::uint32_t table{};std::memcpy(&table,table_bytes.data(),4);const auto owner_bytes=child.read(table+(batch?0x34u:0x30u),4);std::uint32_t owner{};std::memcpy(&owner,owner_bytes.data(),4);
    virtuals.push_back(J{{"callsite",pc},{"semantic_method",batch?"batched_cached_vertices":"support_vertex"},{"shape",shape},{"owner",owner},{"arguments",values}});
   }
   try{child.step();}catch(...){report["failed_instruction"]=pc;report["failed_opcode15"]=process_hex(opcode);report["original_stop_signal"]=WIFSTOPPED(child.status)?WSTOPSIG(child.status):0;report["calls_before_failure"]=calls;report["virtual_shape_methods_before_failure"]=virtuals;J progress=J::array();for(const auto&[address,bytes]:executed)progress.push_back(J{{"instruction",address},{"original_bytes15",bytes}});report["executed_unique_solver_sites_before_failure"]=progress;throw;}if(solver_entered&&call)calls.push_back(J{{"callsite",pc},{"actual_target",std::uint32_t(child.regs().rip)}});
  }
  J sites=J::array();for(const auto&[pc,bytes]:executed)sites.push_back(J{{"instruction",pc},{"original_bytes15",bytes}});
  report["profiles"].push_back(J{{"passed",true},{"actual_input_fp",profile==0},{"declared_rounding_control",profile},{"executed_instructions_including_parent_prefix",instructions},{"executed_unique_solver_sites",sites},{"calls",calls},{"virtual_shape_methods",virtuals},{"original_solver_return",result},{"original_return_site",0x2f8cc2u},{"after_fp",fp_words(child.fp())}});
 }
 report["passed"]=true;
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';return report.at("passed").get<bool>()?0:1;
}
