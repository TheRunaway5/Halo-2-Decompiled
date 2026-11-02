// Appended to the physical read/store bridge. All forests are explicitly
// declared source-domain inputs; none represents a captured live partition.
int main(int argc,char**argv){J report{{"passed",false},{"captured_live_partition_input",false},{"original_outcomes_supplied_to_native",false},{"native_original_CPU_children",false}};try{
 check(argc==5,"Supply originalELF,declaredcases,metadata,report");std::ifstream in(argv[2]),meta(argv[3]);const auto fixture=J::parse(in),metadata=J::parse(meta);
 constexpr std::uint32_t bank=0x1000000u,bytes=0x4000u,graph=bank,parents_header=bank+0x100u,edge_header=bank+0x200u,parents=bank+0x1000u,edges=bank+0x2000u,sp=0x6000f000u;
 J cases=J::array();std::uint64_t total_bytes{},total_events{},total_steps{};
 for(const auto&c:fixture.at("cases"))for(unsigned mode=0;mode<4;++mode)for(unsigned depth=0;depth<4;++depth){
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);std::vector<std::byte>before_bytes(bytes,std::byte{0xa5});
  const auto put_bank=[&](std::uint32_t p,std::uint32_t v){for(unsigned k=0;k<4;++k)before_bytes.at(p-bank+k)=std::byte(v>>(k*8u));};
  put_bank(graph,parents_header);put_bank(graph+4u,edge_header);put_bank(graph+8u,0x12345678u);
  put_bank(parents_header,parents);put_bank(parents_header+4u,c.at("parents").size());put_bank(parents_header+8u,0x80000000u|c.at("parents").size());
  put_bank(edge_header,edges);put_bank(edge_header+4u,c.at("edges").size());put_bank(edge_header+8u,0x80000000u|c.at("edges").size());
  for(unsigned n=0;n<c.at("parents").size();++n)put_bank(parents+n*4u,std::uint32_t(c.at("parents")[n].get<std::int32_t>()));
  for(unsigned n=0;n<c.at("edges").size();++n)put_bank(edges+n*4u,c.at("edges")[n]);
  original.write(bank,before_bytes);native.write(bank,before_bytes);
  auto before=original.fp();before.cwd=0x27fu|(mode<<10u);before.swd=0x100u;before.mxcsr=0x1f80u|(mode<<13u);before.ftw=0;std::memset(before.st_space,0,sizeof(before.st_space));
  const std::array<long double,3>incoming{.75L,-.5L,1.25L};if(depth){const auto top=(8u-depth)&7u;before.swd=std::uint16_t((before.swd&~0x3800u)|(top<<11u));for(unsigned n=0;n<depth;++n){std::memcpy(reinterpret_cast<std::byte*>(before.st_space)+n*16u,&incoming[n],10);before.ftw|=1u<<((top+n)&7u);}}
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);request(PTRACE_SETFPREGS,native.pid,nullptr,&before);
  const auto owner=c.at("owner").get<std::uint32_t>();std::vector<std::uint32_t>arguments{c.at("world")};
  auto entry=original.regs();entry.rip=owner;entry.rcx=graph;entry.rsp=sp;
  const auto put=[&](auto p,std::uint32_t v){original.write(p,{reinterpret_cast<const std::byte*>(&v),4});};put(sp,0x1ccacau);for(unsigned n=0;n<arguments.size();++n)put(sp+4u+n*4u,arguments[n]);request(PTRACE_SETREGS,original.pid,nullptr,&entry);
  ActivationMemory memory(native,0x60000000u,0x60010000u);J original_calls=J::array();std::size_t steps{};const auto ledger=activation_trace(original,metadata,memory,original_calls,steps);check(original_calls.empty(),"Pure graph helper entered a child CPU owner");
  std::uint32_t result{};
  check(owner==0x3212c0u,"Unqualified island constructor owner");result=native_tu15_construct_physics_island(memory,{graph},{c.at("world")}).identity;
  const auto source_after=original.read(bank,bytes),native_after=native.read(bank,bytes);const auto source_fp=original.fp();bool live=source_fp.ftw==before.ftw;
  for(unsigned n=0;n<depth;++n)live&=std::memcmp(reinterpret_cast<const std::byte*>(source_fp.st_space)+n*16u,reinterpret_cast<const std::byte*>(before.st_space)+n*16u,10)==0;
  const bool equal=source_after==native_after,ordered=ledger==memory.ledger,returned=(std::uint32_t(original.regs().rax)==result)&&std::uint32_t(original.regs().rsp)==sp+4u+arguments.size()*4u,fp=fp_words(source_fp)==fp_words(before);
  J differences=J::array();for(unsigned n=0;n<std::min(ledger.size(),memory.ledger.size())&&differences.size()<8;++n)if(ledger[n]!=memory.ledger[n])differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"name",c.at("name")},{"owner",owner},{"declared_mode",mode},{"declared_depth",depth},{"passed",equal&&ordered&&returned&&fp&&live},{"persistent_bytes_equal",equal},{"ordered_operands_equal",ordered},{"first_operand_differences",differences},{"return_DATA_RET_equal",returned},{"original_FP_unchanged",fp},{"original_live80_unchanged",live},{"source_operand_events",ledger.size()},{"source_instructions",steps},{"declared_world",c.at("world")}});
  total_bytes+=bytes;total_events+=ledger.size();total_steps+=steps;
 }
 bool passed=true;for(const auto&c:cases)passed&=c.at("passed").get<bool>();report.update(J{{"passed",passed},{"cases",cases},{"profiles",cases.size()},{"persistent_compared_bytes",total_bytes},{"strict_operand_events",total_events},{"original_instructions",total_steps},{"remaining",J::array({"Source-authenticated positive3187C0 partition graph input","37C2D0 graph build and37C460 group allocation/lifetime composition","Full parent and live partition ownership","Invalid/cyclic forests and mutable observer/fault prefixes"})}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;}
