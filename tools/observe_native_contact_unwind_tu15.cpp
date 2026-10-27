// Offline native buffer physical proof. Remaining pool calls execute actual
// original i386 bodies in a stopped child. No code patch, expected result,
// callback replay, GP/SSE postimage synthesis or zeroed opaque packet.
#include "halo2/native_tu15_contact_buffers.hpp"
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
 void wait(){check(waitpid(pid,&status,0)==pid,"Physical producer wait failed");check(WIFSTOPPED(status),"Original producer left without boundary");}
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
namespace {
struct NativeFailure{};
struct UnwindMemory:Memory{
 std::uint32_t reject_read{},reject_write{};unsigned read_occurrence{},write_occurrence{};mutable unsigned seen_read{};unsigned seen_write{};bool active{};mutable bool primary_fired{},cleanup_phase{};mutable std::vector<std::pair<std::uint32_t,std::size_t>>cleanup_reads;
 explicit UnwindMemory(Child&c):Memory(c){}
 [[noreturn]]void fail()const{primary_fired=true;cleanup_phase=true;throw NativeFailure{};}
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(active&&reject_read&&p==reject_read&&!primary_fired&&seen_read++==read_occurrence)fail();if(cleanup_phase)cleanup_reads.emplace_back(p,b.size());Memory::read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{if(active&&reject_write&&p==reject_write&&!primary_fired&&seen_write++==write_occurrence)fail();Memory::write(p,b);}
};
}
int main(int argc,char**argv){J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false},{"source_fs_registration_recreated",false},{"exception_dispatch_emulated",false},{"remaining_pool_calls_execute_original_cpu",true},{"production_native_only_closure",false}};try{
 check(argc==5,"Supply original ELF, capture.json, unwind cases, report");std::ifstream input(argv[2]),declared(argv[3]);const auto capture=J::parse(input),cases=J::parse(declared);Child original,native;original.start(argv[1],0);native.start(argv[1],0);const auto entry=original.regs();const auto initial=original.fp();check(initial.ftw==0,"Cleanup proof requires its original empty x87 entry");const auto p=((std::uint32_t(entry.rsp)-4u)&~15u)-0x330u,buffer=p+0x6cu,records=p+0x110u,probe=p-0x500u;struct Bank{std::uint32_t address;std::vector<std::byte>before;};std::vector<Bank>banks;for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size"))});}const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>(),fs=capture.at("before").at("fs_base").get<std::uint32_t>();J results=J::array();
 for(const auto&c:cases.at("cases")){
  const auto name=c.at("name").get<std::string>(),kind=c.at("kind").get<std::string>(),injection=c.value("injection",std::string{});report["active_case"]=name;for(const auto&b:banks){original.write(b.address,b.before);native.write(b.address,b.before);}auto regs=entry;request(PTRACE_SETREGS,original.pid,nullptr,&regs);request(PTRACE_SETREGS,native.pid,nullptr,&regs);auto original_initial=initial;request(PTRACE_SETFPREGS,original.pid,nullptr,&original_initial);request(PTRACE_SETFPREGS,native.pid,nullptr,&original_initial);NativeTu15PhysicsFrameFp original_fp{initial.cwd,initial.swd,initial.mxcsr},fp=original_fp;PhysicalCall source{original},physical{native};UnwindMemory memory(native);J native_events=J::array();const auto count=c.value("count",7u);const auto allocator=original.word(0x48c938u);const auto stop=c.value("source_stop_before",0u),action=c.value("source_action",0u);const auto observer_pc=c.value("observer_after",0u);
  if(c.value("exhaust_stack",false)){put(original,allocator+0xcu,0);put(native,allocator+0xcu,0);}
  SourcePhysicsContactRecordObserver observer=[&](const auto&s){native_events.push_back(J{{"kind","store"},{"pc",s.instruction},{"address",s.address},{"cleanup",memory.cleanup_phase}});if(observer_pc&&s.instruction==observer_pc&&!memory.primary_fired)memory.fail();};
  const auto maybe_fail=[&](std::string_view when){if(memory.active&&!memory.primary_fired&&injection==when)memory.fail();};
  NativeTu15ContactBufferServices services;
  services.allocate_stack_block=[&](auto a,auto owner,auto bytes,auto&context){native_events.push_back(J{{"kind","allocate_stack"},{"cleanup",memory.cleanup_phase},{"data_bytes",bytes}});maybe_fail("allocate_before");const auto data=physical.call(owner.identity,a.identity,{bytes},0x303cc2,p-0x20u,context).returned;maybe_fail("allocate_after");return data;};
  services.release_stack_block=[&](auto a,auto owner,auto data,auto bytes,auto&context){native_events.push_back(J{{"kind","release_stack"},{"cleanup",memory.cleanup_phase},{"data",data},{"bytes",bytes}});maybe_fail("release_stack_before");physical.call(owner.identity,a.identity,{data,bytes},0x303d4f,p-0x1cu,context);maybe_fail("release_stack_after");};
  services.release_owned_components=[&](auto a,auto owner,auto data,auto bytes,auto tag,auto&context){native_events.push_back(J{{"kind","release_owned"},{"cleanup",memory.cleanup_phase},{"data",data},{"bytes",bytes}});maybe_fail("owned_before");physical.call(owner.identity,a.identity,{data,bytes,tag},memory.cleanup_phase?0x303b93u:0x303d89u,memory.cleanup_phase?probe-8u:p-0x1cu,context);maybe_fail("owned_after");};
  NativeTu15ArrayGrowthServices growth;growth.allocate=[&](const auto&req,auto&context){return physical.call(req.owner,req.receiver,{req.bytes,req.memory_class},0x2e327e,p-0x1cu,context).returned;};growth.release=[&](const auto&req,auto&context){native_events.push_back(J{{"kind","release_records"},{"cleanup",memory.cleanup_phase},{"data",req.data},{"bytes",req.bytes}});physical.call(req.owner,req.receiver,{req.data,req.bytes,req.memory_class},memory.cleanup_phase?0x303f73u:0x2e32b3u,memory.cleanup_phase?probe-8u:p-0x1cu,context);};
  const auto activate=[&]{memory.active=true;const auto object=c.value("reject_object",std::string{});const auto base=object=="allocator"?allocator:object=="global"?0x48c938u:buffer;memory.reject_read=c.value("reject_read",false)?base+c.value("reject_offset",0u):0;memory.reject_write=c.value("reject_write",false)?base+c.value("reject_offset",0u):0;memory.read_occurrence=c.value("read_occurrence",0u);memory.write_occurrence=c.value("write_occurrence",0u);};
  bool thrown=false;PhysicalCall::Result point;
  if(kind=="constructor"){
   point=source.call(0x303c70,buffer,{count},0x304416,p,original_fp,stop);activate();try{native_tu15_initialize_contact_components(memory,{buffer},count,fp,services,observer);}catch(const NativeFailure&){thrown=true;}
  }else if(kind=="destructor"){
   source.call(0x303c70,buffer,{count},0x304416,p,original_fp);native_tu15_initialize_contact_components(memory,{buffer},count,fp,services);
   if(c.value("owned",false)){for(Child*child:{&original,&native})put(*child,buffer+4u,count);source.call(0x2e3250,0,{buffer,256},0x3043e0,p,original_fp);NativeTu15ArrayGrowthMemory array(memory,{buffer});native_tu15_grow_array(array,256,fp,growth);}
   if(c.value("force_rewind",false)){put(original,allocator+0x14u,allocator);put(native,allocator+0x14u,allocator);}if(c.value("force_release",false)){put(original,allocator+0x14u,0);put(native,allocator+0x14u,0);}
   point=source.call(0x303d00,buffer,{},0x304a6f,p,original_fp,stop);activate();try{native_tu15_destroy_contact_components(memory,{buffer},fp,services,observer);}catch(const NativeFailure&){thrown=true;}
  }else if(kind=="record_base"){
   for(Child*child:{&original,&native}){put(*child,records,p+0x120u);put(*child,records+4u,4);put(*child,records+8u,0x80000004u);}if(c.value("owned",false)){source.call(0x2e3250,0,{records,128},0x3043e0,p,original_fp);NativeTu15ArrayGrowthMemory array(memory,{records});native_tu15_grow_array(array,128,fp,growth);}memory.cleanup_phase=true;native_tu15_release_owned_contact_records(memory,{records},fp,growth);
  }else throw std::runtime_error("Unknown declared cleanup owner");
  const bool should_throw=c.value("exception",false);check(thrown==should_throw,name+" native exception propagation differs");if(should_throw)check(point.stopped,name+" original prefix did not stop before its declared exception phase");if(action){const auto frame=kind=="constructor"?p-8u:kind=="destructor"?p-4u:p+0x330u;source.call(action,0,{},0x304a6f,probe,original_fp,0,false,frame);}
  std::size_t raw_different{},platform_different{},compared{};J differences=J::array();for(const auto&bank:banks){const auto expected=original.read(bank.address,bank.before.size()),actual=native.read(bank.address,bank.before.size());for(std::size_t n=0;n<actual.size();++n){const auto address=bank.address+n;if(address>=private_first&&address<private_end)continue;++compared;if(expected[n]==actual[n])continue;++raw_different;if(address>=fs&&address<fs+4u)++platform_different;if(differences.size()<32)differences.push_back(J{{"address",address},{"original",std::to_integer<unsigned>(expected[n])},{"native",std::to_integer<unsigned>(actual[n])}});}}
  const bool packets=kind=="record_base"?original.read(records,12)==native.read(records,12):original.read(buffer,20)==native.read(buffer,20);const bool fp_equal=J::array({original_fp.control_word,original_fp.status,original_fp.mxcsr})==J::array({fp.control_word,fp.status,fp.mxcsr});const auto expected_reads=c.value("cleanup_reads",0u);const bool reads_equal=memory.cleanup_reads.size()==expected_reads;const bool qualified=raw_different==platform_different&&packets&&fp_equal&&reads_equal;results.push_back(J{{"name",name},{"native_cleanup_semantics_passed",qualified},{"whole_bank_raw_passed",raw_different==0&&packets&&fp_equal},{"raw_different_bytes",raw_different},{"source_platform_registration_different_bytes",platform_different},{"physics_different_bytes",raw_different-platform_different},{"opaque_packets_equal",packets},{"fp_equal",fp_equal},{"cleanup_read_count",memory.cleanup_reads.size()},{"source_action_expected_reads",expected_reads},{"cleanup_reads",memory.cleanup_reads},{"exception_propagated",thrown},{"source_prefix",stop},{"actual_original_action",action},{"persistent_compared_bytes",compared},{"first_differences",differences},{"native_events",native_events},{"original_calls",source.calls},{"actual_original_pool_calls",physical.calls}});report["cases"]=results;
 }
 const bool qualified=std::all_of(results.begin(),results.end(),[](const auto&r){return r.at("native_cleanup_semantics_passed").template get<bool>();});const bool raw=std::all_of(results.begin(),results.end(),[](const auto&r){return r.at("whole_bank_raw_passed").template get<bool>();});report.update(J{{"passed",qualified},{"native_cleanup_semantics_passed",qualified},{"whole_bank_raw_passed",raw},{"physical_cases",results.size()},{"source_ownership_provenance",cases.at("source_ownership_provenance")},{"scope","Actual original resource funclets compared to native C++ exception cleanup. Original live FS registration differences stay explicit raw RED; this is not exception-dispatch/platform equivalence or producer opaque-field closure."}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report.at("passed")},{"cases",report.value("physical_cases",0u)},{"whole_bank_raw_passed",report.value("whole_bank_raw_passed",false)},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;}
