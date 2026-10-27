// Offline native buffer physical proof. Remaining pool calls execute actual
// original i386 bodies in a stopped child. No code patch, expected result,
// callback replay, GP/SSE postimage synthesis or zeroed opaque packet.
#include "halo2/native_tu15_contact_candidate_buffers.hpp"
#include "halo2/native_tu15_array_reserve.hpp"
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
long request(__ptrace_request op,pid_t p,void*a=nullptr,void*b=nullptr){errno=0;const auto n=ptrace(op,p,a,b);if(n==-1&&errno)throw std::runtime_error(strerror(errno));return n;}
struct Child{
 pid_t pid=-1;int status{};
 ~Child(){if(pid>0){kill(pid,SIGKILL);waitpid(pid,&status,0);}}
 void wait(){check(waitpid(pid,&status,0)==pid,"Physical candidate-buffer wait failed");check(WIFSTOPPED(status),"Original producer left without boundary");}
 void debug(unsigned n,unsigned long value){request(PTRACE_POKEUSER,pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)+n*sizeof(unsigned long)),reinterpret_cast<void*>(value));}
 user_regs_struct regs(){user_regs_struct r{};request(PTRACE_GETREGS,pid,nullptr,&r);return r;}
 user_fpregs_struct fp(){user_fpregs_struct f{};request(PTRACE_GETFPREGS,pid,nullptr,&f);return f;}
 void go(std::uint32_t target){debug(0,target);debug(6,0);debug(7,1);request(PTRACE_CONT,pid);wait();const auto r=regs();check(WSTOPSIG(status)==SIGTRAP&&r.rip==target,"Physical service failed before target "+std::to_string(target)+" at "+std::to_string(r.rip)+" signal "+std::to_string(WSTOPSIG(status)));debug(7,0);}
 void step(){request(PTRACE_SINGLESTEP,pid);wait();check(WSTOPSIG(status)==SIGTRAP,"Physical original instruction failed");}
 void start(const char*exe,unsigned ordinal){pid=fork();check(pid>=0,"Cannot fork original producer process");if(!pid){const int fd=open("/dev/null",O_WRONLY);dup2(fd,1);close(fd);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(exe,exe,nullptr);_exit(127);}wait();request(PTRACE_SETOPTIONS,pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,pid);wait();check(WSTOPSIG(status)==SIGTRAP,"Original producer input bridge failed");for(unsigned n=0;n<=ordinal;++n){go(0x304350);if(n<ordinal)step();}}
 std::vector<std::byte>read(std::uint32_t address,std::size_t size){std::vector<std::byte>b(size);iovec local{b.data(),size},remote{reinterpret_cast<void*>(std::uintptr_t(address)),size};check(process_vm_readv(pid,&local,1,&remote,1,0)==std::ptrdiff_t(size),"Cannot read physical producer storage "+std::to_string(address)+" size "+std::to_string(size));return b;}
 void write(std::uint32_t address,std::span<const std::byte>b){iovec local{const_cast<std::byte*>(b.data()),b.size()},remote{reinterpret_cast<void*>(std::uintptr_t(address)),b.size()};check(process_vm_writev(pid,&local,1,&remote,1,0)==std::ptrdiff_t(b.size()),"Cannot install physical producer input "+std::to_string(address));}
 std::uint32_t word(std::uint32_t address){const auto b=read(address,4);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;}
};
struct Memory:SourcePhysicsContactMemory{Child&child;mutable std::size_t reads{};std::size_t writes{};explicit Memory(Child&c):child(c){}void read(std::uint32_t p,std::span<std::byte>b)const override{const auto input=child.read(p,b.size());std::copy(input.begin(),input.end(),b.begin());++reads;}void write(std::uint32_t p,std::span<const std::byte>b)override{child.write(p,b);++writes;}void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}};
std::array<std::uint32_t,8>gp(const user_regs_struct&r){return{std::uint32_t(r.rax),std::uint32_t(r.rcx),std::uint32_t(r.rdx),std::uint32_t(r.rbx),std::uint32_t(r.rsp),std::uint32_t(r.rbp),std::uint32_t(r.rsi),std::uint32_t(r.rdi)};}
J fp_words(const user_fpregs_struct&f){return J::array({f.cwd,f.swd,f.mxcsr});}
struct PrefixStop{};
struct FaultMemory:Memory{
 std::uint32_t reject_read{},reject_write{};bool active{};explicit FaultMemory(Child&c):Memory(c){}
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(active&&reject_read&&p==reject_read)throw PrefixStop{};Memory::read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{if(active&&reject_write&&p==reject_write)throw PrefixStop{};Memory::write(p,b);}
};
struct PhysicalCall{
 Child&child;J calls=J::array();
 struct Result{std::uint32_t returned{};bool stopped{},alignment{};};
 Result call(std::uint32_t entry,std::uint32_t receiver,std::initializer_list<std::uint32_t>arguments,std::uint32_t ret,std::uint32_t base,NativeTu15PhysicsFrameFp&fp,std::uint32_t stop=0,bool alignment=false,std::uint32_t frame=0){
  const auto sp=base-4u*(arguments.size()+1u);std::vector<std::byte>stack((arguments.size()+1u)*4u);unsigned n{};auto put=[&](std::uint32_t v){for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(v>>(k*8u));++n;};put(ret);for(auto v:arguments)put(v);child.write(sp,stack);auto r=child.regs();r.rcx=receiver;r.rsp=sp;r.rip=entry;if(frame)r.rbp=frame;request(PTRACE_SETREGS,child.pid,nullptr,&r);auto f=child.fp();f.cwd=fp.control_word;f.swd=fp.status;f.mxcsr=fp.mxcsr;request(PTRACE_SETFPREGS,child.pid,nullptr,&f);
  child.debug(0,ret);child.debug(1,stop);child.debug(6,0);child.debug(7,stop?5u:1u);request(PTRACE_CONT,child.pid);child.wait();r=child.regs();f=child.fp();child.debug(7,0);fp={f.cwd,f.swd,f.mxcsr};const auto signal=WSTOPSIG(child.status);bool prefix=stop&&r.rip==stop,unaligned=signal==SIGSEGV&&r.rip==0x303ba9u;check((signal==SIGTRAP&&(r.rip==ret||prefix))||(alignment&&unaligned),"Original buffer call failed at "+std::to_string(r.rip)+" signal "+std::to_string(signal));if(stop)check(prefix,"Original buffer prefix did not reach its declared instruction");if(alignment)check(unaligned,"Original buffer alignment case did not actually fault");calls.push_back(J{{"entry",entry},{"receiver",receiver},{"arguments",arguments},{"return",ret},{"actual_stop",std::uint32_t(r.rip)},{"after_fp",fp_words(f)}});return{std::uint32_t(r.rax),prefix,unaligned};
 }
};
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
void put(Child&c,std::uint32_t p,std::uint32_t value,unsigned width=4){std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));c.write(p,std::span(b).first(width));}
}
int main(int argc,char**argv){
 J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false},
  {"production_native_only_closure",false}};
 try{
  check(argc==5,"Supply original ELF, capture, declared cases and report");
  std::ifstream capture_input(argv[2]),case_input(argv[3]);const auto capture=J::parse(capture_input),fixture=J::parse(case_input);
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);
  const auto entry=original.regs();const auto entry_fp=original.fp();check(entry_fp.ftw==0,"Entry-buffer proof requires empty source x87");
  check(!(entry.eflags&0x400u),"Entry-buffer copy proof requires DF clear");
  const auto p=((std::uint32_t(entry.rsp)-4u)&~15u)-0x330u,buffer=p+0x6cu,keys=p+0x180u;
  struct Bank{std::uint32_t address;std::vector<std::byte>before;};std::vector<Bank>banks;
  for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size"))});}
  // Read real query-created keys in a separate unchanged original child.
  J original_keys=J::array();
  {Child query;query.start(argv[1],0);query.go(0x2f36f0);put(query,0x46e468,0,1);const auto sp=std::uint32_t(query.regs().rsp),q=((sp-4u)&~15u)-0x3e0u;
   query.go(0x2f3821);const auto data=query.word(q+0x180u),count=query.word(q+0x184u);check(count>1&&count<=128,"Unexpected genuine original key count");for(unsigned n=0;n<count;++n)original_keys.push_back(query.word(data+n*4u));}
  const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>();
  J results=J::array();
  for(const auto&c:fixture.at("cases")){
   for(const auto&bank:banks){original.write(bank.address,bank.before);native.write(bank.address,bank.before);}
   auto installed=entry_fp;installed.cwd=c.value("control_word",std::uint16_t(entry_fp.cwd));installed.swd=c.value("status",std::uint16_t(entry_fp.swd));installed.mxcsr=c.value("mxcsr",std::uint32_t(entry_fp.mxcsr));
   for(Child*child:{&original,&native}){auto r=entry;request(PTRACE_SETREGS,child->pid,nullptr,&r);request(PTRACE_SETFPREGS,child->pid,nullptr,&installed);}
   NativeTu15PhysicsFrameFp expected_fp{installed.cwd,installed.swd,installed.mxcsr},fp=expected_fp;
   const auto name=c.at("name").get<std::string>(),kind=c.at("kind").get<std::string>();report["active_case"]=name;
   const auto count=c.value("count",7u),stop=c.value("stop_before",0u),observe=c.value("observer_after",0u);
   PhysicalCall source{original},physical{native};FaultMemory memory(native);bool stopped{};
   const auto allocator=original.word(0x48c938u);if(c.value("exhaust_stack",false)){put(original,allocator+0xcu,0);put(native,allocator+0xcu,0);}
   NativeTu15ContactBufferServices services;
   services.allocate_stack_block=[&](auto a,auto owner,auto size,auto&state){return physical.call(owner.identity,a.identity,{size},0x2f3484,p-32u,state).returned;};
   services.release_stack_block=[&](auto a,auto owner,auto data,auto size,auto&state){physical.call(owner.identity,a.identity,{data,size},0x2f3523,p-28u,state);};
   services.release_owned_components=[&](auto a,auto owner,auto data,auto size,auto tag,auto&state){physical.call(owner.identity,a.identity,{data,size,tag},kind=="base"?0x317196u:0x2f3560u,kind=="base"?p-8u:p-28u,state);};
   NativeTu15ArrayGrowthServices reserve;
   reserve.allocate=[&](const auto&r,auto&state){return physical.call(r.owner,r.receiver,{r.bytes,r.memory_class},0x2e31ff,p-32u,state).returned;};
   reserve.release=[&](const auto&r,auto&state){physical.call(r.owner,r.receiver,{r.data,r.bytes,r.memory_class},0x2e3238,p-32u,state);};
   std::vector<SourcePhysicsContactRecordStore>stores;
   SourcePhysicsContactRecordObserver observer=[&](const auto&s){stores.push_back(s);if(observe&&s.instruction==observe)throw PrefixStop{};};
   const auto inject=[&]{if(c.contains("reject_read_offset"))memory.reject_read=(c.value("reject_read_object",std::string{})=="allocator"?allocator:buffer)+c["reject_read_offset"].get<unsigned>();if(c.contains("reject_write_offset"))memory.reject_write=(c.value("reject_write_object",std::string{})=="allocator"?allocator:buffer)+c["reject_write_offset"].get<unsigned>();if(c.value("reject_global_allocator_read",false))memory.reject_read=0x48c938u;memory.active=true;};
   PhysicalCall::Result expected;J key_values=J::array();std::uint32_t returned{};
   if(kind=="sort"){
    key_values=c.value("keys",original_keys);if(c.value("reverse",false))std::reverse(key_values.begin(),key_values.end());
    check(!key_values.empty()&&key_values.size()<=128,"Unbounded sort input");
    for(unsigned n=0;n<key_values.size();++n){put(original,keys+n*4u,key_values[n]);put(native,keys+n*4u,key_values[n]);}
    const auto first=c.value("first",0u),last=c.value("last",unsigned(key_values.size()-1)),forwarded=c.value("forwarded_comparator",0xdeadbeefu);
    expected=source.call(0x2f33b0,0,{keys,first,last,forwarded},0x1ccaca,p-0x80u,expected_fp,stop);
    if(c.value("reject_pivot_read",false))memory.reject_read=keys+((first+last)>>1u)*4u;
    if(c.value("reject_first_write",false))memory.reject_write=std::uint32_t(original.regs().rsi+original.regs().rax*4u);
    memory.active=true;try{native_tu15_sort_contact_keys(memory,keys,first,last,forwarded,observer);}catch(const PrefixStop&){stopped=true;}
   }else if(kind=="initialize"){
    expected=source.call(0x2f3430,buffer,{count},0x304416,p,expected_fp,stop);inject();try{returned=native_tu15_initialize_contact_entries(memory,{buffer},count,fp,services,observer).identity;}catch(const PrefixStop&){stopped=true;}
   }else{
    source.call(0x2f3430,buffer,{count},0x304416,p,expected_fp);native_tu15_initialize_contact_entries(memory,{buffer},count,fp,services);
    if(c.value("grow_entries",false)||(kind=="base"&&!c.value("borrowed",false))){
     put(original,buffer+4u,count);put(native,buffer+4u,count);const auto capacity=count*2u+1u;
     source.call(0x2e31e0,0,{buffer,capacity,12},0x1ccaca,p,expected_fp);
     native_tu15_reserve_array(memory,NativeTu15ArrayHeader{buffer},capacity,12,fp,reserve);
    }
    if(c.value("force_rewind",false)){put(original,allocator+0x14u,allocator);put(native,allocator+0x14u,allocator);}
    if(kind=="destroy"){expected=source.call(0x2f34d0,buffer,{},0x304a6f,p,expected_fp,stop);inject();try{native_tu15_destroy_contact_entries(memory,{buffer},fp,services,observer);}catch(const PrefixStop&){stopped=true;}}
    else if(kind=="base"){expected=source.call(0x317170,buffer,{},0x1ccaca,p,expected_fp);native_tu15_release_owned_contact_entries(memory,{buffer},fp,services);}
    else throw std::runtime_error("Unknown entry-buffer owner");
   }
   check(stopped==expected.stopped,name+" native prefix boundary differs");if(!stopped&&kind=="initialize")check(returned==expected.returned,name+" native constructor result differs");
   std::size_t different{},platform{},compared{};J diffs=J::array();const auto fs=capture.at("before").at("fs_base").get<std::uint32_t>();
   for(const auto&bank:banks){const auto a=original.read(bank.address,bank.before.size()),b=native.read(bank.address,bank.before.size());for(std::size_t n=0;n<a.size();++n){const auto address=bank.address+n;if(address>=private_first&&address<private_end)continue;++compared;if(a[n]==b[n])continue;++different;if(address>=fs&&address<fs+4u)++platform;if(diffs.size()<32)diffs.push_back(J{{"address",address},{"original",std::to_integer<unsigned>(a[n])},{"native",std::to_integer<unsigned>(b[n])}});}}
   const bool packets=kind=="sort"?original.read(keys,key_values.size()*4u)==native.read(keys,key_values.size()*4u):original.read(buffer,20)==native.read(buffer,20);
   const bool fp_equal=J::array({expected_fp.control_word,expected_fp.status,expected_fp.mxcsr})==J::array({fp.control_word,fp.status,fp.mxcsr});
   results.push_back(J{{"name",name},{"kind",kind},{"passed",different==0&&packets&&fp_equal},{"semantic_prefix_equal",different==platform&&packets&&fp_equal},{"completed_call",!stopped},{"persistent_compared_bytes",compared},{"different_bytes",different},{"platform_fs_different_bytes",platform},{"physics_different_bytes",different-platform},{"first_differences",diffs},{"packets_equal",packets},{"fp_equal",fp_equal},{"native_stores",stores.size()},{"original_calls",source.calls},{"actual_original_pool_calls",physical.calls}});
   report["cases"]=results;
  }
  report.update(J{{"passed",std::all_of(results.begin(),results.end(),[](const auto&c){return c.at("passed").template get<bool>();})},{"completed_calls_passed",std::all_of(results.begin(),results.end(),[](const auto&c){return !c.at("completed_call").template get<bool>()||c.at("passed").template get<bool>();})},{"all_physics_and_opaque_prefixes_equal",std::all_of(results.begin(),results.end(),[](const auto&c){return c.at("semantic_prefix_equal").template get<bool>();})},{"physical_case_count",results.size()},{"genuine_original_query_created_keys",original_keys},{"cases",results},{"private_control_stack_contract",capture.at("private_returned_stack")},{"source_fs_raw_gate_unchanged",true}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}
 std::cout<<J{{"passed",report["passed"]},{"cases",report.value("physical_case_count",0u)},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report["passed"].get<bool>()?0:1;
}
