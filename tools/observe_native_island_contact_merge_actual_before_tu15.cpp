// Full native318DE0 and native allocator/temporary lifetime on genuine records.
int main(int argc,char**argv){J report{{"passed",false},{"captured_guest_ABI",false},{"captured_after_consumed",false},
 {"supplied_original_outcomes",false},{"manufactured_rows",false},{"native_services_execute_original_CPU",false}};try{
 check(argc==5,"Supply ELF,capture,instructions,output");std::ifstream capture_in(argv[2]),meta_in(argv[3]);const auto capture=J::parse(capture_in),metadata=J::parse(meta_in);
 constexpr std::uint32_t first=0x72000000u,end=0x72010000u,sp=0x7200f000u,header=0x72001000u;const auto&recorded=capture.at("floating_point");J cases=J::array();
 for(unsigned mode=0;mode<4;++mode)for(unsigned depth=0;depth<4;++depth)for(unsigned variation=0;variation<2;++variation){
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);const auto world=original.word(0x52be48u),queue=original.word(world+0x20u),
   original_first=original.word(original.word(queue)+0x44u)+0x68u,original_second=original.word(original.word(queue+4u)+0x44u)+0x68u,
   a=variation?original_second:original_first,b=variation?original_first:original_second;
  auto before=original.fp();before.cwd=std::uint16_t((recorded.at("x87_control_word").get<unsigned>()&~0xc00u)|(mode<<10u));before.swd=recorded.at("x87_status").get<unsigned>();
  before.mxcsr=(recorded.at("mxcsr").get<unsigned>()&~0x6000u)|(mode<<13u);before.ftw=0;std::memset(before.st_space,0,sizeof(before.st_space));
  const std::array<long double,3>incoming{.75L,-.5L,1.25L};if(depth){const auto top=(8u-depth)&7u;before.swd=std::uint16_t((before.swd&~0x3800u)|(top<<11u));
   for(unsigned n=0;n<depth;++n){std::memcpy(reinterpret_cast<std::byte*>(before.st_space)+n*16u,&incoming[n],10);before.ftw|=1u<<((top+n)&7u);}}
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
  put(sp,std::uint32_t(0x1ccacau));put(sp+4u,b);auto entry=original.regs();entry.rip=0x318de0u;entry.rsp=sp;entry.rsi=a;request(PTRACE_SETREGS,original.pid,nullptr,&entry);
  ActivationMemory memory(native,first,end),heap_memory(native,0u,0xffffffffu);J calls=J::array();std::size_t steps{};const auto ledger=activation_trace(original,metadata,memory,calls,steps);
  NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15IslandContactMergeServices services;NativeTu15IslandContactMergeStats stats;
  NativeTu15ArrayAllocatorMemory allocator(heap_memory);NativeTu15ArrayAllocatorServices heap;unsigned unbound_heap{},unbound_duplicates{};J native_calls=J::array();
  heap.allocate_owner=[&](const auto&r,auto&)->std::uint32_t{check(r.owner==0x233250u,"Unknown contact-merge pool allocation");NativeTu15PoolMemory pool(heap_memory,r.receiver);return native_tu15_pool_allocate(pool,r.bytes);};
  heap.release_owner=[&](const auto&r,auto&){check(r.owner==0x233420u,"Unknown contact-merge pool release");NativeTu15PoolMemory pool(heap_memory,r.receiver);check(native_tu15_pool_release(pool,r.data,r.bytes),"Contact merge requires large pool release");};
  heap.default_heap=[&](auto&)->std::uint32_t{++unbound_heap;throw std::runtime_error("Contact merge requires default heap");};
  NativeTu15ArrayGrowthServices backing;
  backing.allocate=[&](const auto&r,auto&state){check(r.owner==0x149da0u,"Unknown contact-merge allocation router");return native_tu15_route_array_allocation(allocator,r.bytes,r.memory_class,state,heap);};
  backing.release=[&](const auto&r,auto&state){check(r.owner==0x149e00u,"Unknown contact-merge release router");native_tu15_route_array_release(allocator,r.data,r.bytes,r.memory_class,state,heap);};
  const auto record_call=[&](std::uint32_t owner,std::uint32_t receiver,J args){native_calls.push_back(J{{"entry",owner},{"this",receiver},{"arguments",args},{"before_fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"after_fp",J::array({fp.control_word,fp.status,fp.mxcsr})}});};
  services.reserve_array=[&](auto array,auto capacity,auto stride,auto&state){record_call(0x2e31e0u,0,J::array({array.identity,capacity,stride}));native_tu15_reserve_array(heap_memory,array,capacity,stride,state,backing);};
  services.lifetime.allocation.release=[&](const auto&r,auto&state){record_call(r.owner,r.receiver,J::array({r.data,r.bytes,r.memory_class}));backing.release(r,state);};
  services.lifetime.allocate_arena=[&](auto arena,auto owner,auto bytes,auto&state){check(owner.identity==0x2e10a0u,"Unknown contact-merge arena allocation");record_call(owner.identity,arena,J::array({bytes}));return native_tu15_allocate_stack_arena(heap_memory,arena,bytes,state,backing);};
  services.lifetime.release_arena=[&](auto arena,auto owner,auto block,auto bytes,auto&state){check(owner.identity==0x2e1110u,"Unknown contact-merge arena release");record_call(owner.identity,arena,J::array({block,bytes}));native_tu15_release_stack_arena(heap_memory,arena,block,bytes,state,backing);};
  services.cleanup_duplicate_agent=[&](auto,auto,auto&){++unbound_duplicates;throw std::runtime_error("Actual contact duplicate requires its native agent owner");};
  services.release_duplicate_manager=[&](auto,auto,auto&){++unbound_duplicates;throw std::runtime_error("Actual contact duplicate requires its native manager owner");};
  native_tu15_merge_island_contact_records(memory,{a},{b},{{header}},fp,services,{},&stats);
  J service_differences=J::array();if(calls.size()!=native_calls.size())service_differences.push_back("Actual contact-merge child count differs");
  for(unsigned n=0;n<std::min(calls.size(),native_calls.size());++n){for(const auto*key:{"entry","arguments","before_fp","after_fp"})
   if(calls[n][key]!=native_calls[n][key])service_differences.push_back(J{{"index",n},{"field",key},{"original",calls[n][key]},{"native",native_calls[n][key]}});
   if(native_calls[n]["entry"]!=0x2e31e0u&&calls[n]["this"]!=native_calls[n]["this"])service_differences.push_back("Actual contact-merge child receiver differs");}
  std::size_t compared{},different{};J byte_differences=J::array();for(const auto&bank:capture.at("regions")){
   const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();const auto x=original.read(address,size),y=native.read(address,size);compared+=size;
   for(unsigned n=0;n<size;++n)if(x[n]!=y[n]){++different;if(byte_differences.size()<8)byte_differences.push_back(J{{"address",address+n},{"original",std::to_integer<unsigned>(x[n])},{"native",std::to_integer<unsigned>(y[n])}});}}
  const auto after=original.fp();bool live=after.ftw==before.ftw;for(unsigned n=0;n<depth;++n)live&=std::memcmp(reinterpret_cast<const std::byte*>(after.st_space)+n*16u,reinterpret_cast<const std::byte*>(before.st_space)+n*16u,10)==0;
  const bool ordered=ledger==memory.ledger,fp_equal=fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr}),returned=std::uint32_t(original.regs().rsp)==sp+4u;
  J differences=J::array();for(unsigned n=0;n<std::min(ledger.size(),memory.ledger.size());++n)if(ledger[n]!=memory.ledger[n]&&differences.size()<8)differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"passed",ordered&&fp_equal&&returned&&different==0&&live&&service_differences.empty()&&!unbound_heap&&!unbound_duplicates},
   {"declared_rounding_mode",mode},{"declared_x87_depth",depth},{"declared_swapped_headers",variation!=0},{"genuine_queued_pair_order",variation==0},
   {"ordered_operands_equal",ordered},{"first_operand_differences",differences},{"fp_equal",fp_equal},{"source_return_equal",returned},
   {"original_preserves_live_extended_values",live},{"persistent_compared_bytes",compared},{"different_bytes",different},{"first_byte_differences",byte_differences},
   {"service_context_differences",service_differences},{"source_operand_events",ledger.size()},{"native_operand_events",memory.ledger.size()},
   {"calls",stats.calls},{"temporary_copies",stats.temporary_copies},{"comparisons",stats.comparisons},{"first_moves",stats.first_moves},{"second_moves",stats.second_moves},
   {"duplicates",stats.duplicates},{"array_reserves",stats.array_reserves},{"temporary_constructors",stats.lifetime.temporary_constructors},
   {"temporary_destructors",stats.lifetime.temporary_destructors},{"fast_allocations",stats.lifetime.fast_allocations},{"fast_releases",stats.lifetime.fast_releases},
   {"slow_allocations",stats.lifetime.slow_allocations},{"slow_releases",stats.lifetime.slow_releases},{"unbound_heap",unbound_heap},{"unbound_duplicates",unbound_duplicates}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();report.update(J{{"passed",passed},{"cases",cases},{"remaining_duplicate_children_refuse",true}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
