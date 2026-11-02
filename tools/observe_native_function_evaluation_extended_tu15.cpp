// Declared authored DATA; real whole public13DE70 and genuine child routines.
int main(int argc,char**argv){
 J report{{"passed",false},{"source_outcomes_supplied",false},{"declared_authored_function_inputs",true},{"all_eleven_kinds_qualified",false}};
 try{
  check(argc==5,"Supply originalELF,fixture,metadata,report");std::ifstream in(argv[2]),meta(argv[3]);
  const auto fixture=J::parse(in),metadata=J::parse(meta);constexpr std::uint32_t reference=0x1000100u,blob=0x1000200u,sp=0x7200f000u;
  J cases=J::array();std::uint64_t total_bytes{},total_steps{};unsigned diagnostics{};
  for(unsigned kind=0;kind<=10;++kind)for(unsigned flags=0;flags<2;++flags)
   for(unsigned authored=0;authored<(kind==9?9u:kind==10?7u:kind==5?2u:(kind==2||kind==3)?2u:1u);++authored)
   for(auto parameter:{0u,0x3e800000u,0x3f800000u,0xbf000000u})for(unsigned mode=0;mode<4;++mode)for(unsigned depth:{0u,4u,5u}){
    if(kind>=9u&&(mode!=0u||(parameter==0xbf000000u&&(kind==10u?authored!=6u:authored==0u))))continue;
    if(kind>=9u&&parameter==0xbf000000u&&depth>4u)continue;
    Child original,native;original.start(argv[1],0);native.start(argv[1],0);
    const auto put=[](Child&child,std::uint32_t p,std::uint32_t value,unsigned size=4){child.write(p,{reinterpret_cast<const std::byte*>(&value),size});};
    for(auto*child:{&original,&native}){
      put(*child,reference,512);put(*child,reference+4,blob);put(*child,blob,kind,1);put(*child,blob+1,flags,1);
      put(*child,blob+2,authored?(kind==2?3u:6u):0u,1);put(*child,blob+3,authored?(kind==2?5u:7u):0u,1);
      const std::array<std::uint32_t,8>coefficients{0x3e000000u,0x3f600000u,0x3f000000u,0x3e800000u,0x3f800000u,0xbe800000u,0x40000000u,0x3e800000u};
      for(unsigned n=0;n<64;++n)put(*child,blob+0x14u+n*4u,coefficients[n%coefficients.size()]);
      if(kind==9u){const std::array<std::uint32_t,9>exponents{0x3f000000u,0x40000000u,0u,0xbf800000u,0x40400000u,0xc0000000u,0x35800000u,0x40000000u,0x40000000u};put(*child,blob+0x1cu,exponents[authored]);put(*child,blob+0x28u,authored==7u?0xbf800000u:authored==8u?0u:0x40000000u);}
      if(kind==5u&&authored)for(unsigned layer=0;layer<2;++layer)for(unsigned n=0;n<3;++n){put(*child,blob+0x14u+layer*0x50u+0x24u+n*4u,0xbf800000u);put(*child,blob+0x14u+layer*0x50u+0x34u+n*4u,0x40000000u);}
      if(kind==10u)for(unsigned layer=0;layer<2;++layer){
        const auto at=blob+0x14u+layer*48u;
        for(unsigned n=0;n<8;++n)put(*child,at+n*4u,coefficients[n]);
        const bool zero_b=authored==1u||(authored==4u&&layer==1u)||(authored==5u&&layer==0u);
        if(zero_b){put(*child,at+16u,0);put(*child,at+20u,0);put(*child,at+24u,0);put(*child,at+28u,0);}
        if(authored==2u){put(*child,at,0);put(*child,at+4u,0);put(*child,at+8u,0);put(*child,at+12u,0);}
        if(authored==3u)for(unsigned n=0;n<4;++n)put(*child,at+16u+n*4u,coefficients[n]);
        // Declared source-format points produce the native source exponent3;
        // this exercises both negative-power lower clamps without a CRT error.
        if(authored==6u){put(*child,at,0x40350d6fu);put(*child,at+4u,0);put(*child,at+8u,0);put(*child,at+12u,0);put(*child,at+16u,0x3f800000u);put(*child,at+20u,0);put(*child,at+24u,0);put(*child,at+28u,0);}
      }
    }
    auto before=original.fp();before.cwd=0x27fu|(mode<<10u);before.swd=0x100u;
    before.mxcsr=0x1f80u|(mode<<13u)|((mode&1u)?0x40u:0u)|((mode&2u)?0x8000u:0u);
    before.ftw=0;std::memset(before.st_space,0,sizeof(before.st_space));
    const std::array<long double,5>values{.75L,-.5L,1.25L,-2.5L,3.125L};
    if(depth){const auto top=(8u-depth)&7u;before.swd=(before.swd&~0x3800u)|(top<<11u);
      for(unsigned n=0;n<depth;++n){std::memcpy(reinterpret_cast<std::byte*>(before.st_space)+n*16u,&values[n],10);before.ftw|=1u<<((top+n)&7u);}}
    request(PTRACE_SETFPREGS,original.pid,nullptr,&before);request(PTRACE_SETFPREGS,native.pid,nullptr,&before);
    put(original,sp,0x1ccacau);put(original,sp+4u,parameter);put(original,sp+8u,0x3e800000u);
    auto r=original.regs();r.rip=0x13de70u;r.rdi=reference;r.rsp=sp;request(PTRACE_SETREGS,original.pid,nullptr,&r);
    ActivationMemory memory(native,0x72000000u,0x72010000u);J calls=J::array();std::size_t steps{};
    const auto ledger=activation_trace(original,metadata,memory,calls,steps);NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};
    NativeTu15FunctionEvaluationExtendedStats stats;const auto result=native_tu15_evaluate_authored_function_extended(memory,reference,parameter,0x3e800000u,fp,depth,&stats);
    const auto after=original.fp();bool live=after.ftw==before.ftw;
    for(unsigned n=0;n<depth;++n)live&=std::memcmp(reinterpret_cast<const std::byte*>(after.st_space)+n*16u,reinterpret_cast<const std::byte*>(before.st_space)+n*16u,10)==0;
    std::uint64_t bytes{},different{};for(const auto&bank:fixture.at("regions")){
      const auto p=bank.at("address").get<std::uint32_t>(),n=bank.at("size").get<std::uint32_t>();const auto a=original.read(p,n),b=native.read(p,n);
      bytes+=n;for(unsigned k=0;k<n;++k)different+=a[k]!=b[k];}
    const bool ordered=ledger==memory.ledger,equal=result==after.xmm_space[0],fp_equal=fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr}),ret=std::uint32_t(original.regs().rsp)==sp+12u;
    J diff=J::array();if(!ordered){unsigned n{};while(n<ledger.size()&&n<memory.ledger.size()&&ledger[n]==memory.ledger[n])++n;diff=J{{"first",n},{"source",ledger},{"native",memory.ledger}};}
    J row{{"passed",different==0&&ordered&&equal&&fp_equal&&ret&&live&&calls.size()==stats.curves.native_remainders},
      {"kind",kind},{"flags",flags},{"authored",authored},{"parameter_bits",parameter},{"mode",mode},{"depth",depth},
      {"ordered_reads_equal",ordered},{"result_equal",equal},{"fp_equal",fp_equal},{"RET8_equal",ret},{"live80_equal",live},
      {"different_bytes",different},{"native_remainders",stats.curves.native_remainders},{"native_powers",stats.power_calls},{"native_control_points",stats.control_point_queries}};
    if(!row.at("passed").get<bool>()&&diagnostics++<8u)row.update(J{{"native_result",result},{"original_result",after.xmm_space[0]},
      {"native_fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"original_fp",fp_words(after)},{"read_differences",diff}});
    cases.push_back(row);total_bytes+=bytes;total_steps+=steps;
   }
  bool passed=true;for(const auto&c:cases)passed&=c.at("passed").get<bool>();
  J hit=J::array(),missing=J::array();for(const auto&pc:fixture.at("parent_sites")){const auto address=pc.get<std::uint32_t>();(function_sites.contains(address)?hit:missing).push_back(address);}
  report.update(J{{"passed",passed},{"cases",cases},{"profiles",cases.size()},{"total_declared_compared_bytes",total_bytes},
    {"total_original_instructions",total_steps},{"parent_executed_sites",hit},{"parent_unexercised_sites",missing},
    {"scope","Whole13DE70 all11kinds in admitted finite native curve/power domain. Declared authoredDATA; no runtimecall/outcome claim. ExceptionalCRT/error/overflow/unmasked/source-stackfault profiles remain explicit unqualified."}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}
 std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
