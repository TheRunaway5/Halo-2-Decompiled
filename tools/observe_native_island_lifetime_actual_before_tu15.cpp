// Native temporary/island lifetime on original frame-level owned DATA.
int main(int argc,char**argv){J report{{"passed",false},{"captured_guest_ABI",false},{"captured_after_consumed",false},
 {"supplied_original_outcomes",false},{"manufactured_rows",false},{"native_services_execute_original_CPU",false}};try{
 check(argc==5,"Supply ELF,capture,instructions,output");std::ifstream capture_in(argv[2]),meta_in(argv[3]);const auto capture=J::parse(capture_in),metadata=J::parse(meta_in);
 constexpr std::uint32_t first=0x72000000u,end=0x72010000u,sp=0x7200f000u,header=0x72001000u;
 const auto&recorded=capture.at("floating_point");J cases=J::array();
 for(unsigned mode=0;mode<4;++mode)for(unsigned depth=0;depth<4;++depth)for(unsigned variation=0;variation<5;++variation){
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);const auto world=original.word(0x52be48u),queue=original.word(world+0x20u),island=original.word(original.word(queue+4u)+0x44u);
  const auto actual_count=original.word(island+0x6cu),count=variation==2?0u:variation==3?actual_count:3u;auto before=original.fp();
  before.cwd=std::uint16_t((recorded.at("x87_control_word").get<unsigned>()&~0xc00u)|(mode<<10u));before.swd=recorded.at("x87_status").get<unsigned>();
  before.mxcsr=(recorded.at("mxcsr").get<unsigned>()&~0x6000u)|(mode<<13u);before.ftw=0;std::memset(before.st_space,0,sizeof(before.st_space));
  const std::array<long double,3>incoming{.75L,-.5L,1.25L};if(depth){const auto top=(8u-depth)&7u;before.swd=std::uint16_t((before.swd&~0x3800u)|(top<<11u));
   for(unsigned n=0;n<depth;++n){std::memcpy(reinterpret_cast<std::byte*>(before.st_space)+n*16u,&incoming[n],10);before.ftw|=1u<<((top+n)&7u);}}
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);ActivationMemory memory(native,first,end),heap_memory(native,0u,0xffffffffu);
  J calls=J::array();std::size_t steps{};const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
  const auto original_call=[&](std::uint32_t owner,std::uint32_t receiver,std::initializer_list<std::uint32_t>args){
   put(sp,std::uint32_t(0x1ccacau));unsigned n{};for(auto value:args)put(sp+4u+4u*n++,value);auto entry=original.regs();entry.rsp=sp;entry.rip=owner;entry.rcx=receiver;
   request(PTRACE_SETREGS,original.pid,nullptr,&entry);return activation_trace(original,metadata,memory,calls,steps);};
  auto ledger=variation<2?original_call(0x322100u,island,{variation}):original_call(0x317a40u,header,{count});
  if(variation>=2){const auto part=original_call(0x317ad0u,header,{});for(const auto&e:part)ledger.push_back(e);}
  NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15IslandLifetimeServices services;NativeTu15IslandLifetimeStats stats;
  NativeTu15ArrayAllocatorMemory allocator(heap_memory);NativeTu15ArrayAllocatorServices heap;unsigned unbound_heap{};J native_calls=J::array();
  heap.allocate_owner=[&](const auto&r,auto&)->std::uint32_t{if(r.owner!=0x233250u){++unbound_heap;throw std::runtime_error("Unknown actual island pool allocation owner");}
   NativeTu15PoolMemory pool(heap_memory,r.receiver);return native_tu15_pool_allocate(pool,r.bytes);};
  heap.release_owner=[&](const auto&r,auto&){if(r.owner!=0x233420u){++unbound_heap;throw std::runtime_error("Unknown actual island pool release owner");}
   NativeTu15PoolMemory pool(heap_memory,r.receiver);check(native_tu15_pool_release(pool,r.data,r.bytes),"Actual island pool requires a large release owner");};
  heap.default_heap=[&](auto&)->std::uint32_t{++unbound_heap;throw std::runtime_error("Actual island lifetime requires default heap");};
  NativeTu15ArrayGrowthServices backing;
  backing.allocate=[&](const auto&r,auto&state){check(r.owner==0x149da0u,"Unknown island backing allocator");return native_tu15_route_array_allocation(allocator,r.bytes,r.memory_class,state,heap);};
  backing.release=[&](const auto&r,auto&state){check(r.owner==0x149e00u,"Unknown island backing releaser");native_tu15_route_array_release(allocator,r.data,r.bytes,r.memory_class,state,heap);};
  const auto record_call=[&](std::uint32_t owner,std::uint32_t receiver,J args){native_calls.push_back(J{{"entry",owner},{"this",receiver},{"arguments",args},{"before_fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"after_fp",J::array({fp.control_word,fp.status,fp.mxcsr})}});};
  services.allocation.release=[&](const auto&r,auto&state){record_call(r.owner,r.receiver,J::array({r.data,r.bytes,r.memory_class}));backing.release(r,state);};
  services.allocate_arena=[&](auto arena,auto owner,auto bytes,auto&state){check(owner.identity==0x2e10a0u,"Unknown island temporary arena allocation");record_call(owner.identity,arena,J::array({bytes}));return native_tu15_allocate_stack_arena(heap_memory,arena,bytes,state,backing);};
  services.release_arena=[&](auto arena,auto owner,auto block,auto bytes,auto&state){check(owner.identity==0x2e1110u,"Unknown island temporary arena release");record_call(owner.identity,arena,J::array({block,bytes}));native_tu15_release_stack_arena(heap_memory,arena,block,bytes,state,backing);};
  std::uint32_t result{};if(variation<2)result=native_tu15_destroy_physics_island(memory,{island},variation,fp,services,{},&stats);
  else{native_tu15_construct_island_temporary_records(memory,{header},count,fp,services,{},&stats);native_tu15_destroy_island_temporary_records(memory,{header},fp,services,{},&stats);}
  J service_differences=J::array();if(calls.size()!=native_calls.size())service_differences.push_back("Actual typed lifetime child count differs");
  for(unsigned n=0;n<std::min(calls.size(),native_calls.size());++n)for(const auto*key:{"entry","this","arguments","before_fp","after_fp"})
   if(calls[n][key]!=native_calls[n][key])service_differences.push_back(J{{"index",n},{"field",key},{"original",calls[n][key]},{"native",native_calls[n][key]}});
  std::size_t compared{},different{};J byte_differences=J::array();for(const auto&bank:capture.at("regions")){
   const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();const auto a=original.read(address,size),b=native.read(address,size);compared+=size;
   for(unsigned n=0;n<size;++n)if(a[n]!=b[n]){++different;if(byte_differences.size()<8)byte_differences.push_back(J{{"address",address+n},{"original",std::to_integer<unsigned>(a[n])},{"native",std::to_integer<unsigned>(b[n])}});}}
  const auto after=original.fp();bool live=after.ftw==before.ftw;for(unsigned n=0;n<depth;++n)live&=std::memcmp(reinterpret_cast<const std::byte*>(after.st_space)+n*16u,reinterpret_cast<const std::byte*>(before.st_space)+n*16u,10)==0;
  const bool header_equal=variation<2||original.read(header,20)==native.read(header,20),ordered=ledger==memory.ledger,fp_equal=fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr}),
   returned=std::uint32_t(original.regs().rsp)==sp+(variation<2?8u:4u)&&(variation>=2||result==std::uint32_t(original.regs().rax));
  J differences=J::array();for(unsigned n=0;n<std::min(ledger.size(),memory.ledger.size());++n)if(ledger[n]!=memory.ledger[n]&&differences.size()<8)differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"passed",ordered&&fp_equal&&returned&&header_equal&&different==0&&live&&service_differences.empty()&&!unbound_heap},
   {"variation",variation},{"declared_rounding_mode",mode},{"declared_x87_depth",depth},{"island",island},{"declared_temporary_count",count},
   {"ordered_operands_equal",ordered},{"first_operand_differences",differences},{"header_DATA_equal",header_equal},{"fp_equal",fp_equal},
   {"source_return_equal",returned},{"original_preserves_live_extended_values",live},{"persistent_compared_bytes",compared},{"different_bytes",different},
   {"first_byte_differences",byte_differences},{"service_context_differences",service_differences},{"source_operand_events",ledger.size()},{"native_operand_events",memory.ledger.size()},
   {"temporary_constructors",stats.temporary_constructors},{"temporary_destructors",stats.temporary_destructors},{"fast_allocations",stats.fast_allocations},
   {"slow_allocations",stats.slow_allocations},{"fast_releases",stats.fast_releases},{"slow_releases",stats.slow_releases},{"owned_array_releases",stats.owned_array_releases},
   {"island_destructors",stats.island_destructors},{"island_disposals",stats.island_disposals},{"unbound_services",stats.unbound_services},{"unbound_heap",unbound_heap}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();report.update(J{{"passed",passed},{"cases",cases},{"declared_formal_lifetime_inputs",true},{"source_side_effects_used_as_native_inputs",false}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
