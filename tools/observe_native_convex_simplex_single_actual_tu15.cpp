// The original moving-shape prefix independently derives ordinary30DE DATA.
// This does not capture GP state or claim the30DE calling ABI was live-captured.
#include <set>
int main(int argc,char**argv){J report{{"passed",false},{"profiles",J::array()},{"supplied_original_outcomes",false}};try{
 check(argc==5,"Supply original ELF/typed PROCESS/metadata/report");std::ifstream in(argv[2]),mf(argv[3]);const auto capture=J::parse(in),meta=J::parse(mf);const auto&typed=capture.at("input");std::map<std::uint32_t,J>sites;for(const auto&s:meta.at("sites"))sites.emplace(s.at("instruction"),s);
 const auto prefix=[&](Child&c,unsigned profile){start_process(c,argv[1],typed,profile);if(profile){auto f=c.fp();f.cwd=(f.cwd&~0xc00u)|(profile<<10u);f.mxcsr=(f.mxcsr&~0x6000u)|(profile<<13u);request(PTRACE_SETFPREGS,c.pid,nullptr,&f);}c.step();for(unsigned n=0;n<200000;++n){if(c.regs().rip==0x30de60u)return;c.step();}throw std::runtime_error("Original prefix did not reach30DE");};
 for(unsigned profile=0;profile<4;++profile){
  Child original;prefix(original,profile);const auto entry=original.regs();const auto work=std::uint32_t(entry.rcx),sp=std::uint32_t(entry.rsp),ret=original.word(sp);std::array<std::uint32_t,4>args;for(unsigned k=0;k<4;++k)args[k]=original.word(sp+4u+k*4u);const auto before_fp=original.fp();check(before_fp.ftw==0,"Source simplex requires empty x87");
  struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks,inputs,outputs;
  for(const auto&r:capture.at("regions")){const auto p=r.at("address").get<std::uint32_t>();banks.push_back({p,original.read(p,r.at("size")),{}});}
  for(auto [p,n]:{std::pair{work,8u},std::pair{work+0x20u,16u},std::pair{work+0xa0u,16u},std::pair{work+0xe0u,16u},std::pair{args[2],64u}})inputs.push_back({p,original.read(p,n),{}});
  const auto contains=[](const auto&all,std::uint32_t p,std::size_t n){return std::any_of(all.begin(),all.end(),[&](const auto&b){return p>=b.address&&std::uint64_t(p-b.address)+n<=b.before.size();});};
  const auto captured=[&](std::uint32_t p,std::size_t n){return contains(banks,p,n);};
  const auto defined_read=[&](std::uint32_t p,std::size_t n){return captured(p,n)||contains(inputs,p,n);};
  const auto defined_output=[&](std::uint32_t p,std::size_t n){for(auto [a,z]:{std::pair{work,8u},std::pair{work+0x20u,16u},std::pair{work+0xe0u,16u},std::pair{work+0x120u,16u}})if(p>=a&&std::uint64_t(p-a)+n<=z)return true;return false;};
  J reads=J::array(),stores=J::array(),defined_stores=J::array(),dispatch=J::array();unsigned instructions{};std::set<std::uint32_t>executed;
  const auto&table=meta.at("instruction_dispatch_code_table");check(process_hex(original.read(table.at("address"),20))==table.at("bytes").get<std::string>(),"CPU dispatch table authentication failed");
  while(original.regs().rip!=ret){const auto regs=original.regs();const auto pc=std::uint32_t(regs.rip);check(++instructions<20000,"Source simplex exceeded bounded path");executed.insert(pc);const auto site=sites.find(pc);check(site!=sites.end(),"Source simplex escaped closure at"+std::to_string(pc));const auto values=gp(regs);J pending=J::array();
   for(const auto&op:site->second.at("operands")){auto p=op.at("displacement").get<std::uint32_t>();const auto base=op.at("base").get<int>(),index=op.at("index").get<int>();if(base>=0)p+=values.at(base);if(index>=0)p+=values.at(index)*op.at("scale").get<std::uint32_t>();const auto n=op.at("bytes").get<unsigned>();
    if(site->second.value("instruction_dispatch_code_fetch",false)){check(pc==0x313bf5u&&p==0x3146fcu&&n==4&&values[0]==1u,"CPU dispatch domain differs");dispatch.push_back(J{{"instruction",pc},{"address",p},{"bytes",n}});continue;}
    if(op.at("read").get<bool>()&&defined_read(p,n))reads.push_back(J::array({p,n}));
    if(op.at("write").get<bool>()&&(captured(p,n)||defined_output(p,n)))pending.push_back(J{{"address",p},{"bytes",n},{"captured",captured(p,n)},{"before",captured(p,n)?process_hex(original.read(p,n)):std::string{}}});
   }
   original.step();for(auto e:pending){e["written"]=process_hex(original.read(e.at("address"),e.at("bytes")));e.erase("bytes");const bool c=e.at("captured");e.erase("captured");if(c)stores.push_back(e);else{e.erase("before");defined_stores.push_back(e);}}
  }
  const auto source_result=std::uint32_t(original.regs().rax);check(original.regs().rsp==sp+20u,"Source30DE RET16 differs");const auto expected_fp=original.fp();for(auto&b:banks)b.after=original.read(b.address,b.before.size());
  for(auto [p,n]:{std::pair{work,8u},std::pair{work+0x20u,16u},std::pair{work+0xa0u,16u},std::pair{work+0xe0u,16u},std::pair{work+0x120u,16u}})outputs.push_back({p,{},original.read(p,n)});
  J passes=J::array();
  for(bool read_pass:{false,true}){
   Child native;prefix(native,profile);check(std::uint32_t(native.regs().rcx)==work,"Independent Source work identity differs");check(fp_words(native.fp())==fp_words(before_fp),"Independent prefix FP differs");for(const auto&b:banks)check(native.read(b.address,b.before.size())==b.before,"Independent captured BEFORE differs");for(const auto&b:inputs)check(native.read(b.address,b.before.size())==b.before,"Independent typed DATA differs");
   ProcessMemory memory(native);memory.data.fill(std::byte(profile*17u+0xa5u));
   struct Tracked:SourcePhysicsContactMemory{ProcessMemory&m;const std::function<bool(std::uint32_t,std::size_t)>&captured,&reading,&output;bool enabled{};mutable J reads=J::array();J stores=J::array(),defined_stores=J::array();Tracked(ProcessMemory&v,const std::function<bool(std::uint32_t,std::size_t)>&c,const std::function<bool(std::uint32_t,std::size_t)>&r,const std::function<bool(std::uint32_t,std::size_t)>&o):m(v),captured(c),reading(r),output(o){}void read(std::uint32_t p,std::span<std::byte>b)const override{m.read(p,b);if(enabled&&reading(p,b.size()))reads.push_back(J::array({p,b.size()}));}void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}void write(std::uint32_t p,std::span<const std::byte>b)override{J e;if(enabled&&captured(p,b.size())){std::vector<std::byte>before(b.size());m.read(p,before);e=J{{"address",p},{"before",process_hex(before)},{"written",process_hex(b)}};}m.write(p,b);if(enabled){if(captured(p,b.size()))stores.push_back(e);else if(output(p,b.size()))defined_stores.push_back(J{{"address",p},{"written",process_hex(b)}});}}};
   const std::function<bool(std::uint32_t,std::size_t)>c=captured,r=defined_read,o=defined_output;Tracked tracked(memory,c,r,o);NativeTu15ConvexSimplexMemory view(tracked);view.withhold(work,0x140u);
   // Only proven initialized caller DATA is provided. Other work bytes stay
   // unavailable even though the independent original prefix owns backing.
   for(const auto&b:inputs)if(b.address>=work&&b.address-work<0x140u)view.write(b.address,b.before);
   NativeTu15PhysicsFrameFp fp{before_fp.cwd,before_fp.swd,before_fp.mxcsr};constexpr auto p=ProcessMemory::private_data;
   const NativeTu15ContactShapeScratch child_shape{p+0x900u,p+0x910u,nullptr},shape_scratch{p+0x800u,p+0x810u,&child_shape};
   NativeTu15ConvexSimplexSingleServices services;services.support=[&](auto&guarded,auto shape,auto owner,auto direction,auto output,auto&current){check(&guarded==&view,"Support bypassed shared DATA guard");native_tu15_contact_shape_support(guarded,shape,owner,direction,output,shape_scratch,current,0);};
   NativeTu15ConvexSimplexSingleStats activity;tracked.enabled=true;
   const auto result=native_tu15_solve_single_vertex_convex_simplex(view,{work,{args[0]},{args[1]},args[2]},
       {p+0xa00u,p+0xb00u,p+0xb40u,{p+0x400u,{p+0x200u,p+0x204u,{p,p+0x10u,p+0x30u,p+0x60u,p+0x20u}}}},fp,0,services,{},&activity);
   tracked.enabled=false;check(result==source_result,"Native30DE result differs");check(activity.attempts==1&&activity.completed==1&&!activity.refusals&&!view.refused_reads(),"Native30DE activity/guard differs");
   check((read_pass?tracked.reads:tracked.stores)==(read_pass?reads:stores),std::string("Native30DE ordered retained ")+(read_pass?"reads":"writes")+" differ "+(read_pass?tracked.reads:tracked.stores).dump()+" / "+(read_pass?reads:stores).dump());check(tracked.defined_stores==defined_stores,"Native initialized work publications differ "+tracked.defined_stores.dump()+" / "+defined_stores.dump());
   std::size_t compared{};for(const auto&b:banks){check(native.read(b.address,b.after.size())==b.after,"Native30DE retained bank differs at"+std::to_string(b.address));compared+=b.after.size();}for(const auto&b:outputs){std::vector<std::byte>actual(b.after.size());view.read(b.address,actual);check(actual==b.after,"Native defined work output differs");}check(J::array({fp.control_word,fp.status,fp.mxcsr})==fp_words(expected_fp),"Native30DE FP differs");
   unsigned guard_controls{};for(auto offset:{0x30u,0x40u,0xf0u,0x100u}){std::array<std::byte,16>b{};bool refused{};try{view.read_vector(work+offset,b);}catch(const std::runtime_error&e){refused=std::string(e.what())=="Native simplex requires its unprovided DATA writer";}check(refused,"Dead work row became readable");++guard_controls;}
   std::array<std::byte,4>word{};view.write(work+0x30u,word);view.read(work+0x30u,word);bool remaining_refused{};try{std::array<std::byte,16>b;view.read_vector(work+0x30u,b);}catch(const std::runtime_error&){remaining_refused=true;}check(remaining_refused,"Partial write defined an entire unavailable row");++guard_controls;
   passes.push_back(J{{"passed",true},{"read_pass",read_pass},{"captured_bytes_compared",compared},{"captured_bytes_excluded",0},{"native_result",result},{"native_support_calls",activity.hull.hull.support.shape_support_calls},{"native_unprovided_reads",0},{"guard_controls",guard_controls}});
  }
  report["profiles"].push_back(J{{"passed",true},{"actual_input_fp",profile==0},{"declared_rounding_control",profile},{"work",work},{"source_formals",args},{"source_instructions",instructions},{"source_executed_sites",executed},{"source_result",source_result},{"ordered_reads",reads.size()},{"retained_stores",stores.size()},{"initialized_work_stores",defined_stores.size()},{"dispatch_fetches",dispatch},{"after_fp",fp_words(expected_fp)},{"passes",passes}});
 }
 report["passed"]=true;
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';return report.at("passed").get<bool>()?0:1;
}
