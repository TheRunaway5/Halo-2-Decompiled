namespace {
struct FunctionErrorEnvironment{
 alignas(16)std::array<std::byte,108>saved{};NativeTu15PhysicsFrameFp&fp;unsigned mxcsr;
 FunctionErrorEnvironment(NativeTu15PhysicsFrameFp&f):fp(f),mxcsr(_mm_getcsr()){
  asm volatile("fnsave %0":"=m"(saved)::"memory");std::array<std::byte,28>env{};std::memcpy(env.data(),saved.data(),28);
  const std::uint16_t empty=0xffffu;std::memcpy(env.data(),&f.control_word,2);std::memcpy(env.data()+4,&f.status,2);std::memcpy(env.data()+8,&empty,2);
  asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(f.mxcsr);
 }
 ~FunctionErrorEnvironment(){asm volatile("fnstcw %0":"=m"(fp.control_word));asm volatile("fnstsw %0":"=a"(fp.status));fp.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(saved):"memory");_mm_setcsr(mxcsr);}
};
}
int main(int argc,char**argv){J report{{"passed",false},{"source_outcomes_supplied",false},{"captured_runtime_inputs",false}};try{
 check(argc==4,"Supply real originalELF,fixture,report");std::ifstream f(argv[2]);const auto fixture=J::parse(f);
 constexpr std::uint32_t sp=0x7200f000u,data=0x1000000u,record=data+0x100u,output=data+0x200u,caller_word=data+0x300u,errno_cell=data+0x808u;
 const auto put=[](Child&c,std::uint32_t p,std::uint64_t value,unsigned n=4){c.write(p,{reinterpret_cast<const std::byte*>(&value),n});};
 const auto compare=[&](Child&a,Child&b){std::uint64_t bytes{},different{};for(const auto&bank:fixture.at("regions")){auto p=bank.at("address").get<std::uint32_t>(),n=bank.at("size").get<std::uint32_t>();auto x=a.read(p,n),y=b.read(p,n);bytes+=n;for(unsigned i=0;i<n;++i)different+=x[i]!=y[i];}return std::pair{bytes,different};};
 J special=J::array(),errors=J::array();std::uint64_t total_bytes{};
 const std::array<std::uint64_t,20>patterns{0,0x8000000000000000ull,0x3fe0000000000000ull,0xbfe0000000000000ull,0x3ff0000000000000ull,0xbff0000000000000ull,0x4000000000000000ull,0xc000000000000000ull,0x4008000000000000ull,0xc008000000000000ull,0x7ff0000000000000ull,0xfff0000000000000ull,1,0x8000000000000001ull,0x3fefffffffffffffull,0x3ff0000000000001ull,0x4330000000000001ull,0xc330000000000001ull,0x4340000000000000ull,0xc340000000000000ull};
 for(auto base:patterns)for(auto exponent:patterns){
  if((base&0x7fffffffffffffffull)!=0x7ff0000000000000ull&&(exponent&0x7fffffffffffffffull)!=0x7ff0000000000000ull)continue;
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);
  for(auto*c:{&original,&native})put(*c,output,0xa5a5a5a55a5a5a5aull,8);
  put(original,sp,0x1ccacau);put(original,sp+4,base,8);put(original,sp+12,exponent,8);put(original,sp+20,output);
  auto r=original.regs();r.rip=0x37d948u;r.rsp=sp;request(PTRACE_SETREGS,original.pid,nullptr,&r);original.go(0x1ccacau);
  const auto result=native_tu15_authored_function_infinite_power(base,exponent);put(native,output,result.result_bits,8);
  const auto[bytes,different]=compare(original,native);total_bytes+=bytes;
  special.push_back(J{{"passed",different==0&&std::uint32_t(original.regs().rax)==unsigned(result.report_domain_error)&&std::uint32_t(original.regs().rsp)==sp+4},
   {"base",base},{"exponent",exponent},{"result",result.result_bits},{"domain_event",result.report_domain_error},{"different_bytes",different}});
 }
 for(unsigned kind:{2u,8u})for(unsigned mode=0;mode<4;++mode)for(unsigned depth:{0u,5u})
  for(auto value:{0ull,0x8000000000000000ull,0x7ff0000000000000ull,0xfff0000000000000ull,0x7ff8123456789abcull,0x7ff0123456789abcull}){
   Child original,native;original.start(argv[1],0);native.start(argv[1],0);const auto caller=std::uint16_t(0x27fu|(mode<<10u));
   for(auto*c:{&original,&native}){
    put(*c,record,kind);put(*c,record+4,0x473368u);put(*c,record+8,0x3ff0000000000000ull,8);put(*c,record+16,0x4000000000000000ull,8);put(*c,record+24,value,8);
    put(*c,caller_word,caller,2);put(*c,0x71000028u,data+0x700u);put(*c,data+0x728u,0);put(*c,0x573840u,data+0x800u);put(*c,errno_cell,0xa5a55a5au);
   }
   auto before=original.fp();before.cwd=0x27fu;before.swd=0x100u;before.mxcsr=0x1f80u|(mode<<13u);before.ftw=0;std::memset(before.st_space,0,sizeof(before.st_space));
   const std::array<long double,5>live_values{.75L,-.5L,1.25L,-2.5L,3.125L};
   if(depth){auto top=(8u-depth)&7u;before.swd|=top<<11u;for(unsigned i=0;i<depth;++i){std::memcpy(reinterpret_cast<std::byte*>(before.st_space)+i*16u,&live_values[i],10);before.ftw|=1u<<((top+i)&7u);}}
   request(PTRACE_SETFPREGS,original.pid,nullptr,&before);put(original,sp,0x1ccacau);put(original,sp+4,0x1du);put(original,sp+8,record);put(original,sp+12,caller_word);
   auto r=original.regs();r.rip=0x3334f0u;r.rsp=sp;request(PTRACE_SETREGS,original.pid,nullptr,&r);original.go(0x1ccacau);
   NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15FunctionPowerErrorStats stats;SourceMathErrorServicesTu15 services;
   services.store_error_number=[&](std::int32_t v){put(native,errno_cell,std::bit_cast<std::uint32_t>(v));};std::uint64_t returned;
   {FunctionErrorEnvironment environment(fp);returned=std::bit_cast<std::uint64_t>(native_tu15_authored_function_power_error_current(kind,std::bit_cast<double>(value),caller,services,&stats));}
   const auto after=original.fp();bool live=after.ftw==before.ftw;for(unsigned i=0;i<depth;++i)live&=std::memcmp(reinterpret_cast<const std::byte*>(after.st_space)+i*16u,reinterpret_cast<const std::byte*>(before.st_space)+i*16u,10)==0;
   const auto[bytes,different]=compare(original,native);total_bytes+=bytes;const bool fp_equal=fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr});
   errors.push_back(J{{"passed",different==0&&fp_equal&&returned==value&&live&&std::uint32_t(original.regs().rsp)==sp+4&&stats.refused_profiles==0},
    {"kind",kind},{"mode",mode},{"depth",depth},{"value",value},{"different_bytes",different},{"fp_equal",fp_equal},{"live80_equal",live},
    {"native_fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"original_fp",fp_words(after)}});
  }
 bool passed=true;for(const auto&j:{special,errors})for(const auto&c:j)passed&=c.at("passed").get<bool>();
 report.update(J{{"passed",passed},{"infinite_cases",special},{"error_cases",errors},{"total_declared_compared_bytes",total_bytes},
  {"infinite_FP_scope","Source37CF1C FNSAVE /37CF28 FRSTOR discards classifier arithmetic FP. Typed result/event/fullDATA only; direct37D948 rawFP is not claimed."},
  {"error_FP_scope","Whole3334F0 nativekind2/8 with genuine32E7B5/32ED0D/32ECE9/32E9A2 and realTLS child; typed native errno store. FullFP/live80/banks/returnstack compared. No GP result supplied."}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=4){std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
