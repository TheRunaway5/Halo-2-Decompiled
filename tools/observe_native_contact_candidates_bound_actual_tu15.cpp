// Offline physical oracle and native-only candidate binding. This driver owns
// ptrace; production helpers receive only semantic memory, formals and FP.
#include "halo2/native_tu15_contact_candidates_binding.hpp"
#include "halo2/native_tu15_convex_agent_binding.hpp"
#include "halo2/native_tu15_stack_arena.hpp"
#include "halo2/native_tu15_compound_shape_bounds.hpp"
#ifdef HALO2_CONTACT_CANDIDATE_PRIVATE_DATA
#include "halo2/native_tu15_contact_candidate_private_data.hpp"
#endif
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <vector>
#include <nlohmann/json.hpp>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
long request(__ptrace_request op,pid_t pid,void*a=nullptr,void*b=nullptr){errno=0;auto n=ptrace(op,pid,a,b);if(n==-1&&errno)throw std::runtime_error(strerror(errno));return n;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string out;for(auto x:b){auto n=std::to_integer<unsigned>(x);out+=d[n>>4];out+=d[n&15];}return out;}
struct Child {
 pid_t pid=-1;int status{};
 ~Child(){if(pid>0){kill(pid,SIGKILL);waitpid(pid,&status,0);}}
 void wait(){check(waitpid(pid,&status,0)==pid&&WIFSTOPPED(status),"Original exited before boundary");}
 user_regs_struct regs(){user_regs_struct r{};request(PTRACE_GETREGS,pid,nullptr,&r);return r;}
 user_fpregs_struct fp(){user_fpregs_struct f{};request(PTRACE_GETFPREGS,pid,nullptr,&f);return f;}
 void start(const char*exe){pid=fork();check(pid>=0,"Cannot create independent physical process");if(!pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(exe,exe,nullptr);_exit(127);}wait();request(PTRACE_SETOPTIONS,pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,pid);wait();check(WSTOPSIG(status)==SIGTRAP,"Original input scaffold failed");}
 void step(){request(PTRACE_SINGLESTEP,pid);wait();check(WSTOPSIG(status)==SIGTRAP,"Original instruction fault pc="+std::to_string(regs().rip)+" signal="+std::to_string(WSTOPSIG(status)));}
 std::vector<std::byte>read(std::uint32_t p,std::size_t n)const{std::vector<std::byte>b(n);iovec l{b.data(),n},r{reinterpret_cast<void*>(std::uintptr_t(p)),n};check(process_vm_readv(pid,&l,1,&r,1,0)==std::ptrdiff_t(n),"Unmapped physical DATA read="+std::to_string(p)+" bytes="+std::to_string(n));return b;}
 void write(std::uint32_t p,std::span<const std::byte>b){iovec l{const_cast<std::byte*>(b.data()),b.size()},r{reinterpret_cast<void*>(std::uintptr_t(p)),b.size()};check(process_vm_writev(pid,&l,1,&r,1,0)==std::ptrdiff_t(b.size()),"Unmapped physical DATA write");}
 std::uint32_t word(std::uint32_t p,unsigned n=4)const{auto b=read(p,n);std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);return v;}
 void put(std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));write(p,b);}
};
struct Regions {
 std::map<std::uint32_t,std::uint32_t> rows;
 explicit Regions(const J&regions){for(const auto&r:regions)rows.emplace(r.at("address"),r.at("size"));}
 bool contains(std::uint32_t p,std::size_t n)const{while(n){auto i=rows.upper_bound(p);if(i==rows.begin())return false;--i;if(std::uint64_t(p-i->first)>=i->second)return false;const auto take=std::min(n,std::size_t(i->second-(p-i->first)));p+=take;n-=take;}return true;}
};
constexpr std::uint32_t sp=0x7200f000,stop=0x70000080,payload=0x7200fc00;
constexpr std::uint32_t packet=((sp-4u)&~15u)-0x3e0u;
void setup(Child&c,const J&input,unsigned mode){
 // Independent declared ordinary DATA packet. No private Source/control
 // postimage or expected boundary value is read to initialize this packet.
 c.write(0x72000000,std::vector<std::byte>(0x10000,std::byte{0xa5}));
 const auto outer=input.at("second").get<std::uint32_t>();
 c.put(payload,c.word(c.word(outer)+12u));c.put(payload+4u,c.word(outer+4u));c.put(payload+8u,c.word(outer+8u));
 c.put(payload+12u,outer); // actual3043B5, source-written parent DATA
 c.put(sp,stop);const std::array<std::uint32_t,4>a{input.at("first"),payload,input.at("configuration"),0x2f3140u};for(unsigned i=0;i<a.size();++i)c.put(sp+4u+4u*i,a[i]);
 auto r=c.regs();r.rax=r.rbx=r.rdx=r.rsi=r.rdi=r.rbp=0;r.rcx=input.at("agent");r.rsp=sp;r.rip=0x2f36f0u;r.eflags&=~0x400u;request(PTRACE_SETREGS,c.pid,nullptr,&r);
 auto f=c.fp();const auto&scalar=input.at("floating_point");f.cwd=scalar.at("x87_control_word");f.swd=scalar.at("x87_status");f.mxcsr=scalar.at("mxcsr");if(mode){f.cwd=(f.cwd&~0xc00u)|(mode<<10);f.mxcsr=(f.mxcsr&~0x6000u)|(mode<<13);}f.ftw=0;std::memset(f.st_space,0,sizeof(f.st_space));request(PTRACE_SETFPREGS,c.pid,nullptr,&f);
}
J fp_words(const user_fpregs_struct&f){return J::array({f.cwd,f.swd,f.mxcsr});}
struct Observation {J reads=J::array(),stores=J::array(),dispatch_code_reads=J::array();std::set<std::uint32_t>executed;std::size_t steps{};};
Observation original(Child&c,const J&meta,const Regions&regions){
 std::map<std::uint32_t,J>sites;for(const auto&s:meta.at("sites"))sites.emplace(s.at("instruction"),s);
 Observation out;
 while(std::uint32_t(c.regs().rip)!=stop){
  check(++out.steps<1000000,"Original candidate exceeded finite instruction bound");auto r=c.regs();const auto pc=std::uint32_t(r.rip);auto found=sites.find(pc);check(found!=sites.end(),"Original escaped authenticated closure pc="+std::to_string(pc));out.executed.insert(pc);
  const std::array<std::uint32_t,8>g{std::uint32_t(r.rax),std::uint32_t(r.rcx),std::uint32_t(r.rdx),std::uint32_t(r.rbx),std::uint32_t(r.rsp),std::uint32_t(r.rbp),std::uint32_t(r.rsi),std::uint32_t(r.rdi)};J pending=J::array();
  if(!(found->second.at("repeat").get<bool>()&&!g[1]))for(const auto&a:found->second.at("operands")){
   auto p=a.at("displacement").get<std::uint32_t>();const auto b=a.at("base").get<int>(),i=a.at("index").get<int>();if(b>=0)p+=g[b];if(i>=0)p+=g[i]*a.at("scale").get<std::uint32_t>();const auto n=a.at("bytes").get<unsigned>();if(!regions.contains(p,n))continue;
   bool dispatch=false;
   if(a.at("read").get<bool>()&&meta.contains("immutable_dispatch_CODE"))for(const auto&fetch:meta.at("immutable_dispatch_CODE").at("dispatch_fetches"))if(pc==fetch.at("instruction")){
    const auto first=fetch.at("table").get<std::uint32_t>(),width=fetch.at("bytes").get<unsigned>(),entries=fetch.at("entries").get<unsigned>();check(n==width&&p>=first&&p+n<=first+entries*width&&(p-first)%width==0,"Dispatch CODE operand escaped exact authenticated fetch");
    for(const auto&table:meta.at("immutable_dispatch_CODE").at("immutable_dispatch_CODE_tables"))if(table.at("address")==first){const auto expected=table.at("hex").get<std::string>().substr((p-first)*2,n*2);check(hex(c.read(p,n))==expected,"Original CODE dispatch bytes changed");dispatch=true;}
    check(dispatch,"Missing authenticated CODE dispatch table");out.dispatch_code_reads.push_back(J{{"instruction",pc},{"address",p},{"bytes",n}});
   }
   if(a.at("read").get<bool>()&&!dispatch)out.reads.push_back(J::array({p,n}));
   if(a.at("write").get<bool>())pending.push_back(J{{"instruction",pc},{"address",p},{"before",hex(c.read(p,n))},{"bytes",n}});
  }
  c.step();for(auto&a:pending){a["written"]=hex(c.read(a.at("address"),a.at("bytes")));a.erase("bytes");out.stores.push_back(a);}
 }
 check(c.regs().rsp==sp+20u,"Original candidate RET16 differs");check(c.fp().ftw==0,"Original left nonempty x87 values");return out;
}
struct Memory final:SourcePhysicsContactMemory {
 Child&child;const Regions&regions;bool log_reads;mutable J reads=J::array();
 Memory(Child&c,const Regions&r,bool log):child(c),regions(r),log_reads(log){}
 void read(std::uint32_t p,std::span<std::byte>b)const override{auto value=child.read(p,b.size());std::copy(value.begin(),value.end(),b.begin());if(log_reads&&regions.contains(p,b.size()))reads.push_back(J::array({p,b.size()}));}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{child.write(p,b);}
};
J activity(const NativeTu15ContactCandidateBindingActivity&a){return J{{"attempts",a.attempts},{"completed",a.completed},{"refusals",a.refusals},{"bounds",a.bounds_calls},{"queries",a.key_queries},{"resolutions",a.resolutions},{"filters",a.filters},{"dispatches",a.dispatches},{"created",a.candidate.created},{"released",a.candidate.released},{"unknown",a.unqualified_owners}};}
void native(SourcePhysicsContactMemory&m,const J&input,NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&o,NativeTu15ContactCandidateBindingActivity&stats){
 NativeTu15PairDeletionNativeScope scope;NativeTu15PairDeletionNativeScopeActivity scoped;
 const auto unbound=[&](std::uint32_t owner){scope.reject_unconverted(owner);throw std::runtime_error("Native-only provider requires owner="+std::to_string(owner));};
 NativeTu15ContactCandidateBindingProviders providers;providers.owner_memory=&m;
 NativeTu15ArrayAllocatorMemory router(m);NativeTu15ArrayAllocatorServices heap;
 heap.allocate_owner=[&](const auto&r,auto&){if(r.owner!=0x233250u)unbound(r.owner);NativeTu15PoolMemory pool(m,r.receiver,o);return native_tu15_pool_allocate(pool,r.bytes);};
 heap.release_owner=[&](const auto&r,auto&){if(r.owner!=0x233420u)unbound(r.owner);NativeTu15PoolMemory pool(m,r.receiver,o);if(!native_tu15_pool_release(pool,r.data,r.bytes))unbound(r.owner);};
 heap.default_heap=[&](auto&)->std::uint32_t{unbound(0x149930u);return 0;};
 providers.allocation.allocate=[&](const auto&r,auto&f){if(r.owner!=0x149da0u)unbound(r.owner);return native_tu15_route_array_allocation(router,r.bytes,r.memory_class,f,heap);};
 providers.allocation.release=[&](const auto&r,auto&f){if(r.owner!=0x149e00u)unbound(r.owner);native_tu15_route_array_release(router,r.data,r.bytes,r.memory_class,f,heap);};
 providers.buffers.allocate_stack_block=[&](auto arena,auto owner,auto bytes,auto&f){if(owner.identity!=0x2e10a0u)unbound(owner.identity);return native_tu15_allocate_stack_arena(m,arena.identity,bytes,f,providers.allocation,o);};
 providers.buffers.release_stack_block=[&](auto arena,auto owner,auto block,auto bytes,auto&f){if(owner.identity!=0x2e1110u)unbound(owner.identity);native_tu15_release_stack_arena(m,arena.identity,block,bytes,f,providers.allocation,o);};
 providers.buffers.release_owned_components=[&](auto arena,auto owner,auto block,auto bytes,auto tag,auto&f){providers.allocation.release({arena.identity,owner.identity,block,bytes,tag},f);};
 providers.bounds.child_bounds=[&](auto shape,auto owner,auto transform,auto margin,auto output,auto&f){
  if(owner==0x2e5e80u)native_tu15_motion_shape_bounds(m,shape,transform,margin,output,f,providers.bounds,o);
  else if(owner==0x2e9600u)native_tu15_forward_shape_bounds(m,shape,transform,margin,output,f,providers.bounds);
  else if(owner==0x2e8870u)native_tu15_transformed_shape_bounds(m,shape,transform,[&]{return margin;},[&]{return output;},{0x72004300u},f,providers.bounds,o);
  else if(owner==0x306760u)native_tu15_convex_box_bounds(m,shape,transform,margin,output,f,0,o);
  else if(owner==0x2e8120u)native_tu15_endpoint_shape_bounds(m,shape,transform,margin,output,f,0,o);
  else unbound(owner);
 };
 const auto kind=[&](auto,auto owner,auto&)->std::uint32_t{if(owner.identity==0xa9520u)return native_tu15_contact_child_class_23();if(owner.identity==0xab4b0u)return native_tu15_contact_child_class_24();if(auto type=native_tu15_activation_type(owner.identity))return *type;unbound(owner.identity);return 0;};
 providers.agents.classify_shape=kind;
 providers.agents.create_agent=[&](auto owner,auto first,auto second,auto,auto user,auto&f){if(owner.identity!=0x2e4eb0u)unbound(owner.identity);return native_tu15_create_bound_convex_contact_agent(m,{[&]{return first.identity;},[&]{return second.identity;},[&]{return user;}},{0x72005000u,0x72005100u,0x72005200u,0x72006000u,0x72007000u,0x72008000u},f,0,o);};
 providers.filter.classify_shape=kind;providers.filter.motion_type=[&](auto,auto owner,auto&f){return kind(0,owner,f);};
 const NativeTu15CollisionResolveScratch resolve{{0x72004400u},{0x72004420u},{0x72004440u},{0x72004450u},0x72004460u};
 providers.filter.resolve_child=[&](auto collection,auto owner,auto key,auto output,auto&f){return native_tu15_resolve_bound_contact_candidate_child(m,collection,owner,key,output,{0x72004500u},resolve,f,o,&stats);};
 providers.release_agent=[&](auto,auto owner,auto&){unbound(owner.identity);};
 native_tu15_update_bound_contact_candidates(m,{input.at("agent")},{input.at("first")},{payload},{input.at("configuration")},{0x2f3140u},{{packet},{{0x72004000u},0x72004100u},{0x72004500u},resolve,{0x72004600u}},fp,0,providers,scope,{o},&stats,&scoped);
 check(!scope.active()&&scoped.refused==0,"Native scope leaked or used unknown child");
}
J first_difference(const J&a,const J&b){std::size_t n{};while(n<a.size()&&n<b.size()&&a[n]==b[n])++n;return J{{"index",n},{"original",n<a.size()?a[n]:J{}},{"native",n<b.size()?b[n]:J{}},{"original_count",a.size()},{"native_count",b.size()}};}
}
#ifdef HALO2_CONTACT_CANDIDATE_PRIVATE_DATA
#include "native_contact_candidate_private_data_checks_tu15.hpp"
#endif
int main(int argc,char**argv){J report{{"passed",false},{"profiles",J::array()},{"captured_bank_exclusions",J::array()},{"native_original_executor_calls",0},{"declared_public_packet_poison",165},{"production_private_data_admission_qualified",false}};try{
#ifdef HALO2_CONTACT_CANDIDATE_PRIVATE_DATA
 report["private_data_controls"]=check_private_models();
#endif
 check(argc==4,"Supply original ELF, authenticated metadata and report");std::ifstream stream(argv[2]);const auto meta=J::parse(stream);Regions regions(meta.at("regions"));const auto&input=meta.at("input");
 for(unsigned mode=0;mode<4;++mode){
  Child source;source.start(argv[1]);setup(source,input,mode);const auto before=source.fp();const auto expected=original(source,meta,regions);const auto after=source.fp();J row{{"mode",mode},{"before_fp",fp_words(before)},{"after_fp",fp_words(after)},{"source_instructions",expected.steps},{"source_reads",expected.reads.size()},{"source_stores",expected.stores.size()},{"passed",false}};
  row["immutable_dispatch_CODE_reads"]=expected.dispatch_code_reads;report["profiles"].push_back(row);auto&current=report["profiles"].back();
  for(bool read_pass:{false,true}){
   Child target;target.start(argv[1]);setup(target,input,mode);Memory memory(target,regions,read_pass);NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};J stores=J::array();SourcePhysicsContactRecordObserver observe;
   if(!read_pass)observe=[&](const auto&e){if(regions.contains(e.address,e.written.size()))stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
   NativeTu15ContactCandidateBindingActivity stats;
#ifdef HALO2_CONTACT_CANDIDATE_PRIVATE_DATA
   const std::array<NativeTu15ContactPrivateRange,15>owned{{{packet,0x3d0u},{0x72004000u,0x100u},{0x72004100u,64u},{0x72004300u,64u},{0x72004400u,20u},{0x72004420u,20u},{0x72004440u,16u},{0x72004450u,16u},{0x72004460u,4u},{0x72004500u,8u},{0x72004600u,256u},{0x72005000u,64u},{0x72005100u,128u},{0x72005200u,16u},{0x72006000u,0x3000u}}};
   std::vector<NativeTu15ContactPrivateRange>retained;for(const auto&r:meta.at("regions"))retained.push_back({r.at("address"),r.at("size")});retained.push_back({payload,16u});
   NativeTu15ContactCandidatePrivateData guarded(memory,owned,retained);native(guarded,input,fp,observe,stats);current["ordinary_unprovided_reads_allowed"]=false;
#else
   native(memory,input,fp,observe,stats);
#endif
   current["activity"]=activity(stats);
   check(fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr}),"Complete native candidate FP differs original="+fp_words(after).dump()+" native="+J::array({fp.control_word,fp.status,fp.mxcsr}).dump());
   if(read_pass){current["read_order_equal"]=memory.reads==expected.reads;if(memory.reads!=expected.reads){current["read_difference"]=first_difference(expected.reads,memory.reads);throw std::runtime_error("Native persistent read order differs");}}
   else{current["store_order_equal"]=stores==expected.stores;if(stores!=expected.stores){current["store_difference"]=first_difference(expected.stores,stores);throw std::runtime_error("Native persistent store order/preimages differ");}}
   std::size_t compared{},different{};J differences=J::array();for(const auto&r:meta.at("regions")){const auto p=r.at("address").get<std::uint32_t>(),n=r.at("size").get<unsigned>();const auto a=source.read(p,n),b=target.read(p,n);compared+=n;for(unsigned i=0;i<n;++i)if(a[i]!=b[i]){++different;if(differences.size()<32)differences.push_back(J{{"address",p+i},{"original",std::to_integer<unsigned>(a[i])},{"native",std::to_integer<unsigned>(b[i])}});}}
   current["compared_bytes"]=compared;current["different_bytes"]=different;current["differences"]=differences;check(different==0,"Captured candidate bank bytes differ");check(stats.completed==1&&stats.refusals==0&&stats.candidate.created>0,"Native positive candidate activity missing");
  }
  current["passed"]=true;
 }
 report["passed"]=true;
}catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc==4){std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;}
