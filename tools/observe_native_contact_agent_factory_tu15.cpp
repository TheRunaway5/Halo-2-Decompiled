// Offline physical service bridge. The native producer runs in this x64
// process; each declared remaining service executes the actual original i386
// body in a stopped child. No code patch, expected result or callback replay.
#include "halo2/native_tu15_contact_agent_factory.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <array>
#include <cstdlib>
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
J fp_words(const user_fpregs_struct&f){return J::array({f.cwd,f.swd,f.mxcsr});}
void put(Child&c,std::uint32_t p,std::uint32_t value,unsigned width=4){std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));c.write(p,std::span(b).first(width));}
std::string hex(std::span<const std::byte>b){constexpr char digits[]="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s+=digits[n>>4u];s+=digits[n&15u];}return s;}
std::uint32_t reg(const user_regs_struct&r,const std::string&name){if(name.empty())return 0;if(name=="eax")return r.rax;if(name=="ebx")return r.rbx;if(name=="ecx")return r.rcx;if(name=="edx")return r.rdx;if(name=="ebp")return r.rbp;if(name=="esp")return r.rsp;if(name=="esi")return r.rsi;if(name=="edi")return r.rdi;throw std::runtime_error("Unknown original address register "+name);}
struct NativeFailure{};
struct Memory:SourcePhysicsContactMemory{
 Child&child;std::uint32_t private_first,private_end,watched{},reject_read{},reject_write{};
 unsigned read_occurrence{},write_occurrence{},seen_read{},seen_write{};bool active{},primary_fired{},cleanup{};J ledger=J::array();
 Memory(Child&c,std::uint32_t first,std::uint32_t end):child(c),private_first(first),private_end(end){}
 bool retained(std::uint32_t p,unsigned bytes)const{return p<private_first||p>=private_end||(watched&&p>=watched&&p+bytes<=watched+64u);}
 [[noreturn]]void fail(){primary_fired=true;cleanup=true;throw NativeFailure{};}
 void read(std::uint32_t p,std::span<std::byte>b)const override{auto&self=*const_cast<Memory*>(this);if(active&&!primary_fired&&reject_read&&p==reject_read&&self.seen_read++==read_occurrence)self.fail();const auto value=child.read(p,b.size());std::copy(value.begin(),value.end(),b.begin());if(retained(p,b.size()))self.ledger.push_back(J::array({"read",p,b.size(),hex(b)}));}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{if(active&&!primary_fired&&reject_write&&p==reject_write&&seen_write++==write_occurrence)fail();child.write(p,b);if(retained(p,b.size()))ledger.push_back(J::array({"write",p,b.size(),hex(b)}));}
};
struct Original{
 Child&child;const J&metadata;Memory&contract;J ledger=J::array();unsigned instructions{};std::uint32_t last_agent{};bool alignment_fault{},plain{};
 std::uint32_t call(std::uint32_t entry,std::uint32_t receiver,const std::vector<std::uint32_t>&args,std::uint32_t base,std::uint32_t stop=0,std::uint32_t frame=0){
  constexpr std::uint32_t returned=0x1ccaca;const auto sp=base-4u*(args.size()+1u);put(child,sp,returned);for(unsigned n=0;n<args.size();++n)put(child,sp+4u+n*4u,args[n]);auto r=child.regs();r.rip=entry;r.rsp=sp;r.rcx=receiver;if(frame)r.rbp=frame;request(PTRACE_SETREGS,child.pid,nullptr,&r);
  if(plain){const auto target=stop?stop:returned;child.debug(0,target);child.debug(6,0);child.debug(7,1);request(PTRACE_CONT,child.pid);child.wait();child.debug(7,0);r=child.regs();alignment_fault=WSTOPSIG(child.status)==SIGSEGV&&r.rip==0x2f3658u;check(alignment_fault||(WSTOPSIG(child.status)==SIGTRAP&&r.rip==target),"Plain original factory left its actual boundary");if(!stop)last_agent=r.rax;return r.rip;}
  for(;;){r=child.regs();const auto pc=std::uint32_t(r.rip);if(pc==0x303c36u||pc==0x3040f6u)last_agent=r.rax;if(pc==returned||pc==stop)return pc;check(++instructions<100000,"Original factory owner exceeded bounded instructions");const auto key=std::to_string(pc);check(metadata.contains(key),"Original factory left authenticated native owners at "+key);const auto&i=metadata.at(key);check(child.read(pc,i.at("size"))==decode(i.at("bytes")),"Original instruction bytes changed");
   struct Write{std::size_t index;std::uint32_t p;unsigned bytes;};std::vector<Write>writes;
   for(const auto&m:i.at("memory")){const auto address=reg(r,m.at("base"))+reg(r,m.at("index"))*m.at("scale").get<unsigned>()+m.at("displacement").get<std::uint32_t>();const auto bytes=m.at("bytes").get<unsigned>();if(!contract.retained(address,bytes))continue;if(m.at("read").get<bool>())ledger.push_back(J::array({"read",address,bytes,hex(child.read(address,bytes))}));if(m.at("write").get<bool>()){writes.push_back({ledger.size(),address,bytes});ledger.push_back(J::array({"write",address,bytes,""}));}}
   request(PTRACE_SINGLESTEP,child.pid);child.wait();if(WSTOPSIG(child.status)!=SIGTRAP){check(WSTOPSIG(child.status)==SIGSEGV&&pc==0x2f3658u,"Original factory unexpected physical fault");for(const auto&w:writes)ledger.erase(ledger.begin()+w.index);alignment_fault=true;return pc;}
   for(const auto&w:writes)ledger[w.index][3]=hex(child.read(w.p,w.bytes));
  }
 }
 static std::vector<std::byte>decode(const std::string&s){std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
};
}
int main(int argc,char**argv){J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false},{"source_fs_registration_recreated",false},{"production_native_only_closure",false}};try{
 check(argc==6,"Supply original ELF, capture, cases, instruction metadata and output");std::ifstream capture_input(argv[2]),fixture_input(argv[3]),metadata_input(argv[4]);const auto capture=J::parse(capture_input),fixture=J::parse(fixture_input),metadata=J::parse(metadata_input);
 Child original,native,plain;original.start(argv[1],0);native.start(argv[1],0);plain.start(argv[1],0);const auto entry=original.regs();const auto before_fp=original.fp();check(before_fp.ftw==0,"Factory reference requires declared empty x87 entry");
 const auto p=((std::uint32_t(entry.rsp)-4u)&~15u)-0x330u,base=p-0x100u,probe=p-0x500u;
 const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>(),fs=capture.at("before").at("fs_base").get<std::uint32_t>();
 const auto original_user_data=original.word(std::uint32_t(entry.rcx)+8u);
 struct Bank{std::uint32_t address;std::vector<std::byte>before;};std::vector<Bank>banks;for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size"))});}
 J results=J::array();
 for(const auto&c:fixture.at("cases")){
  const auto name=c.at("name").get<std::string>(),kind=c.at("kind").get<std::string>();report["active_case"]=name;
  for(const auto&bank:banks){original.write(bank.address,bank.before);native.write(bank.address,bank.before);plain.write(bank.address,bank.before);}auto f=before_fp;f.cwd=c.value("control_word",std::uint16_t(f.cwd));f.swd=c.value("status",std::uint16_t(f.swd));f.mxcsr=c.value("mxcsr",std::uint32_t(f.mxcsr));for(Child*child:{&original,&native,&plain}){auto r=entry;request(PTRACE_SETREGS,child->pid,nullptr,&r);request(PTRACE_SETFPREGS,child->pid,nullptr,&f);}
  NativeTu15PhysicsFrameFp fp{f.cwd,f.swd,f.mxcsr};Memory memory(native,private_first,private_end),original_contract(original,private_first,private_end),plain_contract(plain,private_first,private_end);Original source{original,metadata,original_contract},ordinary{plain,metadata,plain_contract};ordinary.plain=true;
  const auto object=p+0x180u+c.value("object_offset",0u),user_data=c.value("user_data",original_user_data);std::uint32_t expected_agent{},actual_agent{};bool failed{},unaligned{};J native_events=J::array();
  const auto injection=c.value("injection",std::string{});const auto maybe_fail=[&](std::string_view phase){if(memory.active&&!memory.primary_fired&&injection==phase)memory.fail();};
  NativeTu15ArrayAllocatorMemory routing(memory);NativeTu15ArrayAllocatorServices routed;
  routed.allocate_owner=[&](const auto&r,auto&){check(r.owner==0x233250u,"Factory requires an unqualified pool allocation owner");NativeTu15PoolMemory pool(memory,r.receiver);return native_tu15_pool_allocate(pool,r.bytes);};
  routed.release_owner=[&](const auto&r,auto&){check(r.owner==0x233420u,"Factory requires an unqualified pool release owner");NativeTu15PoolMemory pool(memory,r.receiver);check(native_tu15_pool_release(pool,r.data,r.bytes),"Factory requires large release ownership");};
  NativeTu15ArrayGrowthServices allocation;
  allocation.allocate=[&](const auto&r,auto&state){native_events.push_back(J{{"kind","allocate"},{"request",J::array({r.receiver,r.owner,r.bytes,r.memory_class})}});maybe_fail("allocate_before");check(r.owner==0x149da0u,"Factory requires actual native allocation route149DA0");const auto data=native_tu15_route_array_allocation(routing,r.bytes,r.memory_class,state,routed);actual_agent=data;if(c.contains("reject_write_offset"))memory.reject_write=data+c["reject_write_offset"].get<unsigned>();maybe_fail("allocate_after");return data;};
  allocation.release=[&](const auto&r,auto&state){native_events.push_back(J{{"kind","release"},{"cleanup",memory.cleanup},{"request",J::array({r.receiver,r.owner,r.data,r.bytes,r.memory_class})}});check(r.owner==0x149e00u,"Factory requires actual native release route149E00");native_tu15_route_array_release(routing,r.data,r.bytes,r.memory_class,state,routed);};
  const auto observer_pc=c.value("observer_after",0u);SourcePhysicsContactRecordObserver observer;if(observer_pc)observer=[&](const auto&s){native_events.push_back(J{{"kind","store"},{"pc",s.instruction},{"address",s.address}});if(observer_pc==s.instruction&&!memory.primary_fired)memory.fail();};
  const auto activate=[&]{memory.active=true;memory.reject_read=c.value("reject_read",0u);memory.reject_write=c.contains("reject_write_offset")?actual_agent+c.at("reject_write_offset").get<unsigned>():0;memory.read_occurrence=c.value("read_occurrence",0u);memory.write_occurrence=c.value("write_occurrence",0u);};
  if(kind=="getter"){
   const auto owner=c.at("owner").get<std::uint32_t>();source.call(owner,0xdeadbeefu,{},base);ordinary.call(owner,0xdeadbeefu,{},base);expected_agent=original.regs().rax;actual_agent=owner==0xa9520u?native_tu15_contact_child_class_23():native_tu15_contact_child_class_24();
  }else if(kind=="constructor"){
   memory.watched=object;original_contract.watched=object;const auto stop=source.call(0x2f3620,object,{user_data},base,c.value("stop_before",0u));ordinary.call(0x2f3620,object,{user_data},base,c.value("stop_before",0u));expected_agent=original.regs().rax;actual_agent=object;activate();try{native_tu15_initialize_contact_agent(memory,{object},user_data,observer);}catch(const NativeFailure&){failed=true;}catch(const std::runtime_error&e){unaligned=std::string_view(e.what())=="Native TU1.5 contact agent requires aligned vector storage";if(!unaligned)throw;}
   check(source.alignment_fault==unaligned,"Constructor actual alignment fault differs");check(failed==c.value("exception",false),"Constructor native prefix failure differs");
  }else{
   const auto owner=c.value("owner",0x303c10u);const auto stop=source.call(owner,0,{0,0,0,user_data},base,c.value("stop_before",0u));ordinary.call(owner,0,{0,0,0,user_data},base,c.value("stop_before",0u));(void)stop;expected_agent=source.last_agent;
   NativeTu15ContactAgentFactoryArguments arguments{[&]{maybe_fail("user_data");return user_data;}};activate();
   try{actual_agent=native_tu15_create_contact_agent(memory,owner==0x303c10u?NativeTu15ContactAgentFactory::owner_303c10:NativeTu15ContactAgentFactory::owner_3040d0,arguments,fp,allocation,observer).identity;}catch(const NativeFailure&){failed=true;}
   check(failed==c.value("exception",false),"Factory native exception boundary differs");
   if(c.value("source_action",false)){source.call(0x37ed40u,0,{},probe,0,base-20u);ordinary.call(0x37ed40u,0,{},probe,0,base-20u);}
   if(kind=="release"){const auto plain_agent=std::uint32_t(plain.regs().rax);source.call(0x2f0380u,0,{expected_agent},base);ordinary.call(0x2f0380u,0,{plain_agent},base);native_tu15_release_contact_agent(memory,{actual_agent},fp,allocation);}
  }
  const bool semantic_result=failed||unaligned||kind=="release"||expected_agent==actual_agent;
  std::size_t different{},platform{},compared{},plain_differences{};J diffs=J::array();for(const auto&bank:banks){const auto a=original.read(bank.address,bank.before.size()),b=native.read(bank.address,bank.before.size()),control=plain.read(bank.address,bank.before.size());for(std::size_t n=0;n<a.size();++n){const auto address=bank.address+n;if(address>=private_first&&address<private_end)continue;++compared;if(a[n]!=control[n])++plain_differences;if(a[n]==b[n])continue;++different;if(address>=fs&&address<fs+4u)++platform;if(diffs.size()<16)diffs.push_back(J{{"address",address},{"original",std::to_integer<unsigned>(a[n])},{"native",std::to_integer<unsigned>(b[n])}});}}
  const bool packets=kind=="getter"||failed||unaligned?kind!="constructor"||original.read(object,64)==native.read(object,64):original.read(expected_agent,64)==native.read(actual_agent,64);
  const bool fp_equal=fp_words(original.fp())==J::array({fp.control_word,fp.status,fp.mxcsr});const bool ordered_compared=observer_pc==0;const bool ordered=!ordered_compared||source.ledger==memory.ledger;
  J first_events=J::array();for(std::size_t n=0;n<std::min(source.ledger.size(),memory.ledger.size());++n)if(source.ledger[n]!=memory.ledger[n]){first_events.push_back(J{{"index",n},{"original",source.ledger[n]},{"native",memory.ledger[n]}});if(first_events.size()==8)break;}
  const bool plain_equal=plain_differences==0&&fp_words(original.fp())==fp_words(plain.fp())&&(kind!="constructor"||original.read(object,64)==plain.read(object,64));
  const bool physics=different==platform&&packets&&fp_equal&&semantic_result&&ordered&&plain_equal;
  results.push_back(J{{"name",name},{"kind",kind},{"strict_raw_passed",different==0&&physics},{"native_semantics_passed",physics},{"different_bytes",different},{"platform_FS_bytes",platform},{"physics_different_bytes",different-platform},{"ordered_bytes_compared",ordered_compared},{"ordered_bytes_equal",ordered_compared?J(ordered):J(nullptr)},{"original_data_events",source.ledger.size()},{"native_data_events",memory.ledger.size()},{"first_event_differences",first_events},{"first_differences",diffs},{"semantic_result_equal",semantic_result},{"packets_equal",packets},{"fp_equal",fp_equal},{"native_exception",failed},{"actual_alignment_fault",unaligned},{"original_instructions",source.instructions},{"persistent_compared_bytes",compared},{"plain_original_equal",plain_equal},{"plain_original_persistent_differences",plain_differences},{"native_events",native_events}});report["cases"]=results;if(std::getenv("HALO2_CONTACT_AGENT_FACTORY_NEGATIVE_CONTROL")&&!physics)break;
 }
 report.update(J{{"passed",std::all_of(results.begin(),results.end(),[](const auto&c){return c.at("native_semantics_passed").template get<bool>();})},{"strict_raw_passed",std::all_of(results.begin(),results.end(),[](const auto&c){return c.at("strict_raw_passed").template get<bool>();})},{"physical_cases",results.size()},{"cases",results},{"native_allocator_composed",true},{"original_user_data_input",original_user_data},{"source_FS_raw_gate_unchanged",true}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=6){std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"cases",report.value("physical_cases",0u)},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report["passed"].get<bool>()?0:1;
}
