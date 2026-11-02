// Full build/extract/destroy sequence on declared index graphs with genuine
// actual09 allocator BEFORE backing. Remaining typed allocator owners execute
// their own original algorithm independently, outside the native graph kernel.
int main(int argc,char**argv){J report{{"passed",false},{"declared_graph_topology",true},{"captured_live_partition_caller",false},{"remaining_allocator_children_execute_original_CPU",true},{"native_parent_fallback",false},{"supplied_original_outcomes",false}};try{
 check(argc==5,"Supply actualELF,capture,metadata,report");std::ifstream input(argv[2]),meta(argv[3]);const auto capture=J::parse(input),metadata=J::parse(meta);
 constexpr std::uint32_t graph=0x72001000u,parents=graph+0x100u,roots=graph+0x200u,edges=graph+0x300u,groups=graph+0x400u,sp=0x7200f000u;
 J cases=J::array();
 for(unsigned variation=0;variation<2;++variation)for(unsigned mode=0;mode<4;++mode)for(unsigned depth=0;depth<4;++depth){
  const std::vector<std::array<std::uint32_t,2>>pairs=variation?std::vector<std::array<std::uint32_t,2>>{{1,0},{2,1},{4,3},{2,0}}:std::vector<std::array<std::uint32_t,2>>{};const std::uint32_t vertices=variation?6u:1u;
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);auto before=original.fp();before.cwd=0x27fu|(mode<<10u);before.swd=0x100u;before.mxcsr=0x1f80u|(mode<<13u);before.ftw=0;std::memset(before.st_space,0,sizeof(before.st_space));const std::array<long double,3>incoming{.75L,-.5L,1.25L};
  if(depth){const auto top=(8u-depth)&7u;before.swd=std::uint16_t((before.swd&~0x3800u)|(top<<11u));for(unsigned n=0;n<depth;++n){std::memcpy(reinterpret_cast<std::byte*>(before.st_space)+n*16u,&incoming[n],10);before.ftw|=1u<<((top+n)&7u);}}
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);request(PTRACE_SETFPREGS,native.pid,nullptr,&before);
  const auto put=[](Child&child,std::uint32_t p,std::uint32_t v){child.write(p,{reinterpret_cast<const std::byte*>(&v),4});};
  for(auto*child:{&original,&native}){
    std::vector<std::byte>zero(0x5000u);child->write(graph,zero);put(*child,graph,parents);put(*child,graph+4u,roots);put(*child,graph+8u,0x12345678u);
    put(*child,parents,graph+0x1000u);put(*child,parents+8u,0x80000000u|(variation?0u:8u));put(*child,roots,graph+0x2000u);put(*child,roots+8u,0x80000000u|(variation?0u:8u));
    put(*child,edges,graph+0x3000u);put(*child,edges+4u,pairs.size());put(*child,edges+8u,0x80000008u);for(unsigned n=0;n<pairs.size();++n){put(*child,graph+0x3000u+n*8u,pairs[n][0]);put(*child,graph+0x3004u+n*8u,pairs[n][1]);}
    put(*child,groups,graph+0x4000u);put(*child,groups+8u,0x80000000u);
  }
  ActivationMemory memory(native,0x72008000u,0x72010000u);NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15IslandPartitionBuildStats activity;J native_calls=J::array();std::size_t steps{};
  const auto call=[&](std::uint32_t owner,std::uint32_t receiver,std::initializer_list<std::uint32_t>args,unsigned pop){
    constexpr std::uint32_t call_sp=0x7200c000u;put(native,call_sp,0x1ccacau);unsigned n{};for(auto value:args)put(native,call_sp+4u+4u*n++,value);
    auto r=native.regs();r.rip=owner;r.rcx=receiver;r.rsp=call_sp;request(PTRACE_SETREGS,native.pid,nullptr,&r);auto f=native.fp();f.cwd=fp.control_word;f.swd=fp.status;f.mxcsr=fp.mxcsr;f.ftw=before.ftw;std::memcpy(f.st_space,before.st_space,sizeof(f.st_space));request(PTRACE_SETFPREGS,native.pid,nullptr,&f);const auto start_fp=fp_words(f);native.go(0x1ccacau);r=native.regs();f=native.fp();check(std::uint32_t(r.rsp)==call_sp+4u+pop,"Partition service RET-pop differs");
    check(f.ftw==before.ftw,"Partition service changed live x87 depth");for(unsigned k=0;k<depth;++k)check(std::memcmp(reinterpret_cast<const std::byte*>(f.st_space)+k*16u,reinterpret_cast<const std::byte*>(before.st_space)+k*16u,10)==0,"Partition service changed live80 DATA");
    fp={f.cwd,f.swd,f.mxcsr};native_calls.push_back(J{{"entry",owner},{"this",receiver},{"arguments",args},{"before_fp",start_fp},{"after_fp",fp_words(f)},{"returned_eax",std::uint32_t(r.rax)}});return std::uint32_t(r.rax);
  };
  NativeTu15IslandPartitionBuildServices services;
  services.reserve_array=[&](auto header,auto capacity,auto stride,auto&){call(0x2e31e0u,0,{header.identity,capacity,stride},0u);};
  services.allocate_group=[&](auto allocator,auto owner,auto bytes,auto alignment,auto&){return call(owner.identity,allocator,{bytes,alignment},8u);};
  services.release_group=[&](auto allocator,auto owner,auto data,auto&){call(owner.identity,allocator,{data},4u);};
  services.release_array=[&](const auto&r,auto&){call(r.owner,r.receiver,{r.data,r.bytes,r.memory_class},12u);};
  J phase_reports=J::array();bool passed=true;std::uint64_t total_events{};
  for(auto owner:{0x37c2d0u,0x37c460u,0x37c3d0u}){
    memory.ledger=J::array();native_calls=J::array();std::vector<std::uint32_t>args=owner==0x37c2d0u?std::vector<std::uint32_t>{edges,vertices}:owner==0x37c460u?std::vector<std::uint32_t>{groups}:std::vector<std::uint32_t>{};
    put(original,sp,0x1ccacau);for(unsigned n=0;n<args.size();++n)put(original,sp+4u+4u*n,args[n]);auto r=original.regs();r.rip=owner;r.rcx=owner==0x37c3d0u?groups:graph;r.rsp=sp;request(PTRACE_SETREGS,original.pid,nullptr,&r);J source_calls=J::array();const auto ledger=activation_trace(original,metadata,memory,source_calls,steps);
    if(owner==0x37c2d0u)native_tu15_build_island_partition_graph(memory,{graph},{edges},vertices,fp,services,{},&activity);
    else if(owner==0x37c460u)native_tu15_extract_island_partition_groups(memory,{graph},{groups},fp,services,{},&activity);
    else native_tu15_destroy_island_partition_groups(memory,{groups},fp,services,{},&activity);
    J call_differences=J::array();if(source_calls.size()!=native_calls.size())call_differences.push_back("Source allocator service count differs");for(unsigned n=0;n<std::min(source_calls.size(),native_calls.size());++n){const auto&a=source_calls[n],&b=native_calls[n];for(const auto*key:{"entry","arguments","before_fp","after_fp"})if(a[key]!=b[key])call_differences.push_back(J{{"index",n},{"field",key},{"original",a[key]},{"native",b[key]}});if(a.at("entry")!=0x2e31e0u&&a.at("this")!=b.at("this"))call_differences.push_back("Allocator receiver differs");if(a.at("result_consumed").get<bool>()&&a.at("returned_eax")!=b.at("returned_eax"))call_differences.push_back("Raw allocator ordinary DATA return differs");}
    J differences=J::array();for(unsigned n=0;n<std::min(ledger.size(),memory.ledger.size())&&differences.size()<8;++n)if(ledger[n]!=memory.ledger[n])differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
    const bool ordered=ledger==memory.ledger,workspace=original.read(graph,0x5000u)==native.read(graph,0x5000u),fp_equal=fp_words(original.fp())==J::array({fp.control_word,fp.status,fp.mxcsr}),returned=std::uint32_t(original.regs().rsp)==sp+4u+args.size()*4u;passed&=ordered&&workspace&&fp_equal&&returned&&call_differences.empty();total_events+=ledger.size();
    phase_reports.push_back(J{{"owner",owner},{"ordered_operands_equal",ordered},{"first_operand_differences",differences},{"workspace_equal",workspace},{"fp_equal",fp_equal},{"RET_equal",returned},{"service_differences",call_differences},{"original_calls",source_calls},{"native_calls",native_calls},{"source_events",ledger.size()}});
  }
  std::uint64_t compared{},different{};for(const auto&bank:capture.at("regions")){const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();const auto a=original.read(address,size),b=native.read(address,size);compared+=size;for(unsigned n=0;n<size;++n)different+=a[n]!=b[n];}passed&=different==0&&activity.unbound_services==0;
  cases.push_back(J{{"passed",passed},{"declared_graph",variation},{"mode",mode},{"depth",depth},{"phases",phase_reports},{"persistent_compared_bytes",compared},{"different_bytes",different},{"source_events",total_events},{"source_steps",steps},{"native_builds",activity.builds},{"native_extractions",activity.extractions},{"native_destructions",activity.destructions},{"native_unions",activity.graph.unions},{"array_reserves",activity.array_reserves},{"group_allocations",activity.group_allocations},{"group_releases",activity.group_releases},{"array_releases",activity.array_releases}});
 }
 bool passed=true;for(const auto&c:cases)passed&=c.at("passed").get<bool>();report.update(J{{"passed",passed},{"cases",cases}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;}
