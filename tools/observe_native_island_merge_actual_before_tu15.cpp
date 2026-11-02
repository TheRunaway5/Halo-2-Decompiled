// Declared formal3190D0 calls from genuine queued frame-level BEFORE DATA.
// The native parent owns its algorithm; remaining typed child owners execute
// independently on native-side banks. No original outcome crosses the runs.
int main(int argc,char**argv){J report{{"passed",false},{"captured_guest_ABI",false},
 {"captured_after_consumed",false},{"supplied_original_outcomes",false},{"manufactured_rows",false},
 {"native_parent_falls_back",false},{"remaining_typed_children_execute_original_CPU",true}};try{
 check(argc==5,"Supply ELF,capture,instructions,output");std::ifstream capture_in(argv[2]),meta_in(argv[3]);
 const auto capture=J::parse(capture_in),metadata=J::parse(meta_in);constexpr std::uint32_t first=0x72000000u,end=0x72010000u,sp=0x7200f000u;
 std::uint32_t world{},first_island{},second_island{};{Child bank;bank.start(argv[1],0);world=bank.word(0x52be48u);
  check(bank.word(world+0x24u)>0,"No genuine pending merge pair");const auto queue=bank.word(world+0x20u);
  first_island=bank.word(bank.word(queue)+0x44u);second_island=bank.word(bank.word(queue+4u)+0x44u);
  check(first_island!=second_island,"Actual queued pair no longer needs an island merge");}
 const auto&recorded=capture.at("floating_point");J cases=J::array();
 for(unsigned mode=0;mode<4;++mode)for(unsigned depth=0;depth<4;++depth)for(unsigned variation=0;variation<3;++variation){
  const auto a=variation==1?second_island:first_island,b=variation==0?second_island:first_island;
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);auto before=original.fp();
  before.cwd=std::uint16_t((recorded.at("x87_control_word").get<unsigned>()&~0xc00u)|(mode<<10u));before.swd=recorded.at("x87_status").get<unsigned>();
  before.mxcsr=(recorded.at("mxcsr").get<unsigned>()&~0x6000u)|(mode<<13u);before.ftw=0;std::memset(before.st_space,0,sizeof(before.st_space));
  const std::array<long double,3>incoming{.75L,-.5L,1.25L};if(depth){const auto top=(8u-depth)&7u;before.swd=std::uint16_t((before.swd&~0x3800u)|(top<<11u));
   for(unsigned n=0;n<depth;++n){std::memcpy(reinterpret_cast<std::byte*>(before.st_space)+n*16u,&incoming[n],10);before.ftw|=1u<<((top+n)&7u);}}
  request(PTRACE_SETFPREGS,original.pid,nullptr,&before);const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
  put(sp,std::uint32_t(0x1ccacau));put(sp+4u,a);put(sp+8u,b);auto entry=original.regs();entry.rip=0x3190d0u;entry.rsp=sp;entry.rcx=world;
  request(PTRACE_SETREGS,original.pid,nullptr,&entry);ActivationMemory memory(native,first,end);J source_calls=J::array();std::size_t steps{};
  const auto ledger=activation_trace(original,metadata,memory,source_calls,steps);NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};
  J native_calls=J::array();NativeTu15IslandMergeServices services;NativeTu15IslandMergeStats stats;
  const auto invoke=[&](std::uint32_t owner,std::uint32_t receiver,std::uint32_t source_header,
    std::initializer_list<std::uint32_t>arguments,unsigned pop,NativeTu15PhysicsFrameFp&context){
    constexpr std::uint32_t call_sp=0x7200c000u;std::vector<std::byte>bytes((arguments.size()+1u)*4u);unsigned n{};
    const auto append=[&](std::uint32_t v){for(unsigned k=0;k<4;++k)bytes[n*4u+k]=std::byte(v>>(k*8u));++n;};append(0x1ccacau);for(auto value:arguments)append(value);native.write(call_sp,bytes);
    auto r=native.regs();r.rip=owner;r.rsp=call_sp;r.rcx=receiver;r.rsi=source_header;request(PTRACE_SETREGS,native.pid,nullptr,&r);
    auto f=native.fp();f.cwd=context.control_word;f.swd=context.status;f.mxcsr=context.mxcsr;f.ftw=before.ftw;std::memcpy(f.st_space,before.st_space,sizeof(f.st_space));
    request(PTRACE_SETFPREGS,native.pid,nullptr,&f);const auto initial=fp_words(f);native.go(0x1ccacau);r=native.regs();f=native.fp();
    check(std::uint32_t(r.rsp)==call_sp+4u+pop,"Actual typed merge child RET-pop differs");
    check(f.ftw==before.ftw,"Actual typed merge child changed preserved x87 depth");for(unsigned k=0;k<depth;++k)
     check(std::memcmp(reinterpret_cast<const std::byte*>(f.st_space)+k*16u,reinterpret_cast<const std::byte*>(before.st_space)+k*16u,10)==0,"Actual typed merge child changed live extended value");
    context={f.cwd,f.swd,f.mxcsr};native_calls.push_back(J{{"entry",owner},{"arguments",arguments},{"this",receiver},{"formal_ESI",source_header},{"before_fp",initial},{"after_fp",fp_words(f)}});
  };
  services.activate_island=[&](auto w,auto island,auto&state){invoke(0x316d40u,w.identity,0,{island.identity},4u,state);};
  services.merge_contact_records=[&](auto x,auto y,auto&state){invoke(0x318de0u,0,x.identity,{y.identity},0u,state);};
  services.reserve_array=[&](auto header,auto capacity,auto stride,auto&state){invoke(0x2e31e0u,0,0,{header.identity,capacity,stride},0u,state);};
  services.destroy_island=[&](auto island,auto owner,auto disposing,auto&state){invoke(owner.identity,island.identity,0,{disposing},4u,state);};
  const auto survivor=native_tu15_merge_physics_islands(memory,{world},{a},{b},fp,services,{},&stats);
  J service_differences=J::array();if(source_calls.size()!=native_calls.size())service_differences.push_back("Actual typed child count differs");
  for(unsigned n=0;n<std::min(source_calls.size(),native_calls.size());++n){const auto&s=source_calls[n],&t=native_calls[n];
   for(const auto*key:{"entry","arguments","before_fp","after_fp"})if(s[key]!=t[key])service_differences.push_back(J{{"index",n},{"field",key},{"original",s[key]},{"native",t[key]}});
   const auto owner=t.at("entry").get<std::uint32_t>();const auto*receiver_key=owner==0x318de0u?"formal_ESI":"this";
   if(owner!=0x2e31e0u&&s[receiver_key]!=t[receiver_key])service_differences.push_back(J{{"index",n},{"field",receiver_key},{"original",s[receiver_key]},{"native",t[receiver_key]}});
  }
  std::size_t compared{},different{};J byte_differences=J::array();for(const auto&bank:capture.at("regions")){
   const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();const auto x=original.read(address,size),y=native.read(address,size);compared+=size;
   for(unsigned n=0;n<size;++n)if(x[n]!=y[n]){++different;if(byte_differences.size()<8)byte_differences.push_back(J{{"address",address+n},{"original",std::to_integer<unsigned>(x[n])},{"native",std::to_integer<unsigned>(y[n])}});}}
  const auto after=original.fp();bool live=after.ftw==before.ftw;for(unsigned n=0;n<depth;++n)
   live&=std::memcmp(reinterpret_cast<const std::byte*>(after.st_space)+n*16u,reinterpret_cast<const std::byte*>(before.st_space)+n*16u,10)==0;
  const bool ordered=ledger==memory.ledger,fp_equal=fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr}),returned=std::uint32_t(original.regs().rsp)==sp+12u&&std::uint32_t(original.regs().rax)==survivor.identity;
  J differences=J::array();for(unsigned n=0;n<std::min(ledger.size(),memory.ledger.size());++n)if(ledger[n]!=memory.ledger[n]&&differences.size()<8)
   differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"passed",ordered&&fp_equal&&returned&&different==0&&live&&service_differences.empty()},
   {"declared_rounding_mode",mode},{"declared_x87_depth",depth},{"pair_variation",variation},{"genuine_queued_pair_order",variation==0},
   {"first",a},{"second",b},{"surviving_island",survivor.identity},{"ordered_parent_operands_equal",ordered},{"first_operand_differences",differences},
   {"service_context_differences",service_differences},{"fp_equal",fp_equal},{"source_RET8_and_EAX_equal",returned},{"original_preserves_live_extended_values",live},
   {"persistent_compared_bytes",compared},{"different_bytes",different},{"first_byte_differences",byte_differences},
   {"source_operand_events",ledger.size()},{"native_operand_events",memory.ledger.size()},{"source_instructions",steps},
   {"calls",stats.calls},{"same_island_skips",stats.same_island_skips},{"island_activations",stats.island_activations},
   {"contact_merges",stats.contact_merges},{"array_reserves",stats.array_reserves},{"moved_bodies",stats.moved_bodies},
   {"moved_actions",stats.moved_actions},{"moved_constraints",stats.moved_constraints},{"moved_rows",stats.moved_rows},
   {"island_destructions",stats.island_destructions},{"unbound_services",stats.unbound_services}});
 }
 bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();report.update(J{{"passed",passed},{"cases",cases},
  {"declared_formal_ABI",J{{"owner",0x3190d0u},{"world_ECX",world},{"actual_queued_first",first_island},{"actual_queued_second",second_island},{"RET_bytes",8}}}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
