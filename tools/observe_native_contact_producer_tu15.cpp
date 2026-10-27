// Offline physical service bridge. The native producer runs in this x64
// process; each declared remaining service executes the actual original i386
// body in a stopped child. No code patch, expected result or callback replay.
#include "halo2/native_tu15_contact_producer.hpp"
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
struct Bridge{
 Child&child;std::uint32_t packet;user_fpregs_struct caller_fp;J&report;J calls=J::array();std::uint32_t prior_child_class{};bool first_class=true;
 std::uint32_t call(std::uint32_t entry,std::uint32_t ecx,std::initializer_list<std::uint32_t>arguments,std::uint32_t return_pc,NativeTu15PhysicsFrameFp&fp,unsigned pending=0){
  const auto sp=packet-4u*(arguments.size()+pending+1u);std::vector<std::byte>stack((arguments.size()+pending+1u)*4u);unsigned index{};const auto put=[&](std::uint32_t value){for(unsigned n=0;n<4;++n)stack[index*4+n]=std::byte(value>>(n*8u));++index;};put(return_pc);for(auto value:arguments)put(value);if(pending){check(pending==1,"Invalid physical pending argument count");put(prior_child_class);}child.write(sp,stack);
  auto r=child.regs();r.rcx=ecx;r.rsp=sp;r.rip=entry;request(PTRACE_SETREGS,child.pid,nullptr,&r);auto f=child.fp();f.cwd=fp.control_word;f.swd=fp.status;f.mxcsr=fp.mxcsr;f.ftw=caller_fp.ftw;std::memcpy(f.st_space,caller_fp.st_space,sizeof(f.st_space));request(PTRACE_SETFPREGS,child.pid,nullptr,&f);const auto before=fp_words(f);report["active_service"]=J{{"entry",entry},{"this",ecx},{"arguments",arguments},{"return_boundary",return_pc}};child.go(return_pc);r=child.regs();f=child.fp();fp={f.cwd,f.swd,f.mxcsr};calls.push_back(J{{"entry",entry},{"this",ecx},{"arguments",arguments},{"pending_arguments",pending},{"original_return_boundary",return_pc},{"returned_eax",std::uint32_t(r.rax)},{"after_esp",std::uint32_t(r.rsp)},{"before_fp",before},{"after_fp",fp_words(f)}});return std::uint32_t(r.rax);
 }
 NativeTu15ContactProducerServices services(){NativeTu15ContactProducerServices s;
  s.update_candidates=[&](auto agent,auto a,auto b,auto context,auto callback,auto&fp){call(0x2f36f0,agent.identity,{a.identity,b.identity,context.identity,callback},0x3043e0,fp);};
  s.initialize_component_buffer=[&](auto storage,auto count,auto&fp){call(0x303c70,storage,{count},0x304416,fp);};
  s.invalid_child=[&](auto&fp){return call(0x30bad0,0,{},0x30445a,fp);};
  s.resolve_component=[&](auto collection,auto owner,auto key,auto storage,auto&fp){return NativeTu15ContactComponent{call(owner.identity,collection.identity,{key,storage},0x30447d,fp)};};
  s.component_class=[&](auto component,auto owner,auto&fp){const bool first=first_class;first_class=!first_class;const auto result=call(owner.identity,component.identity,{},first?0x304495u:0x30449du,fp,first?0:1);if(first)prior_child_class=result;return result;};
  s.test_pair=[&](auto owner,auto output,auto context,auto a,auto b,auto&fp){return call(owner,context,{output,context,a,b},0x3044af,fp);};
  s.query_contact=[&](auto child,auto owner,auto valid,auto first,auto second,auto context,auto packet,auto&fp){call(owner.identity,child.identity,{valid,first.identity,second.identity,context.identity,packet},0x3044e0,fp);};
  s.append_record=[&](auto array,auto&fp){call(0x303ee0,array.identity,{},0x3044ec,fp);};
  s.process_rejected_pair=[&](auto child,auto owner,auto first,auto second,auto context,auto output,auto&fp){call(owner.identity,child.identity,{first.identity,second.identity,context.identity,output.identity},0x304653,fp);};
  s.publish_contact=[&](auto first,auto second,auto context,auto record,auto value,auto data,auto count,auto&fp){call(0x30cd00,0,{first.identity,second.identity,context.identity,record.identity,value,data,count},0x3049ca,fp);};
  s.publish_distance=[&](auto first,auto second,auto context,auto distance,auto data,auto count,auto output,auto value,auto&fp){call(0x30cf40,context.identity,{first.identity,second.identity,context.identity,distance,data,count,output.identity,value},0x304a11,fp);};
  s.destroy_component_buffer=[&](auto storage,auto&fp){call(0x303d00,storage,{},0x304a6f,fp);};
  s.release_records=[&](auto allocator,auto owner,auto data,auto size,auto tag,auto&fp){call(owner.identity,allocator,{data,size,tag},0x304aa3,fp);};
  // Refuse a live clock request until a genuine source clock owner is added.
  return s;
 }
};
}
int main(int argc,char**argv){J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false},{"remaining_services_execute_original_cpu",true},{"production_native_only_closure",false}};try{
 check(argc==5,"Supply original ELF, capture.json, ordinal, report");std::ifstream input(argv[2]);const auto capture=J::parse(input);
 if(std::string_view(argv[3])=="list"){
  Child child;child.start(argv[1],0);J entries=J::array();std::vector<std::size_t>active;bool done{};
  for(unsigned n=0;n<4096&&!done;++n){const auto r=child.regs();const auto pc=std::uint32_t(r.rip);
   if(pc==0x304350u){const auto sp=std::uint32_t(r.rsp),packet=((sp-4u)&~15u)-0x330u;const auto ordinal=entries.size();entries.push_back(J{{"ordinal",ordinal},{"producer",std::uint32_t(r.rcx)},{"entry_sp",sp},{"packet",packet},{"actual_caller",child.word(sp)},{"producer_component_count",child.word(std::uint32_t(r.rcx)+0x10u)}});active.push_back(ordinal);}
   else if(pc==0x304ab7u){check(!active.empty(),"Producer return without entry");auto&entry=entries[active.back()];active.pop_back();const auto packet=entry.at("packet").get<std::uint32_t>(),count=child.word(packet+0x114u),data=child.word(packet+0x110u);entry["record_count"]=count;entry["record_data"]=data;entry["record_capacity"]=child.word(packet+0x118u);entry["return_fp"]=fp_words(child.fp());J flags=J::array();check(count<=256,"Unbounded original producer record list");for(unsigned k=0;k<count;++k)flags.push_back(J::array({std::to_integer<unsigned>(child.read(data+k*128u+0x7cu,1)[0]),std::to_integer<unsigned>(child.read(data+k*128u+0x7du,1)[0])}));entry["record_flags"]=flags;}
   else if(pc==0x1ccacau){done=true;break;}else throw std::runtime_error("Unexpected original producer enumeration PC");
   child.step();child.debug(0,0x304350);child.debug(1,0x304ab7);child.debug(2,0x1ccaca);child.debug(6,0);child.debug(7,0x15);request(PTRACE_CONT,child.pid);child.wait();check(WSTOPSIG(child.status)==SIGTRAP,"Producer enumeration faulted");
  }
  check(done&&active.empty(),"Producer enumeration did not close");report.update(J{{"passed",true},{"original_enumeration_only",true},{"native_physics_validated",false},{"producer_entries",entries}});std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';std::cout<<"Enumerated "<<entries.size()<<" genuine original producer calls\n";return 0;
 }
 const auto ordinal=std::stoul(argv[3]);Child original;original.start(argv[1],ordinal);const auto entry=original.regs();const auto before_fp=original.fp();check(before_fp.ftw==0,"Physical producer proof currently requires empty incoming x87 stack");const auto sp=std::uint32_t(entry.rsp);const std::array<std::uint32_t,4>args{original.word(sp+4),original.word(sp+8),original.word(sp+12),original.word(sp+16)};const auto packet=((sp-4u)&~15u)-0x330u;
 struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}original.go(0x304ab7);original.step();const auto expected_fp=original.fp();for(auto&b:banks)b.after=original.read(b.address,b.before.size());
 Child native;native.start(argv[1],ordinal);for(const auto&b:banks)native.write(b.address,b.before);auto installed=entry;request(PTRACE_SETREGS,native.pid,nullptr,&installed);auto installed_fp=before_fp;request(PTRACE_SETFPREGS,native.pid,nullptr,&installed_fp);Memory memory(native);Bridge bridge{native,packet,before_fp,report};auto services=bridge.services();NativeTu15PhysicsFrameFp fp{before_fp.cwd,before_fp.swd,before_fp.mxcsr};std::vector<SourcePhysicsContactRecordStore>stores;native_tu15_produce_contacts(memory,{std::uint32_t(entry.rcx)},{args[0]},{args[1]},{args[2]},{args[3]},{packet},fp,0,services,[&](const auto&s){stores.push_back(s);});
 const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>();std::size_t different{},private_different{},ranges{},compared{},persistent_compared{};J diffs=J::array();for(const auto&b:banks){const auto actual=native.read(b.address,b.after.size());compared+=actual.size();for(std::size_t n=0;n<actual.size();++n){const auto address=b.address+n;const bool is_private=address>=private_first&&address<private_end;if(!is_private)++persistent_compared;if(actual[n]==b.after[n])continue;if(is_private){++private_different;continue;}++different;if(n==0||actual[n-1]==b.after[n-1])++ranges;if(diffs.size()<256)diffs.push_back(J{{"address",address},{"native",std::to_integer<unsigned>(actual[n])},{"original",std::to_integer<unsigned>(b.after[n])}});}}const auto original_data=original.word(packet+0x110u),native_data=native.word(packet+0x110u),original_count=original.word(packet+0x114u),native_count=native.word(packet+0x114u);check(original_count<=64&&native_count<=64,"Unbounded final contact packet count");const bool record_packets_equal=original_count==native_count&&original_data==native_data&&original.read(original_data,original_count*128u)==native.read(native_data,native_count*128u);
 const bool fp_equal=fp_words(expected_fp)==J::array({fp.control_word,fp.status,fp.mxcsr});report.update(J{{"passed",different==0&&fp_equal&&record_packets_equal},{"source_entry",0x304350},{"ordinal",ordinal},{"entry_gp",gp(entry)},{"arguments",args},{"packet_base",packet},{"before_fp",fp_words(before_fp)},{"after_original_fp",fp_words(expected_fp)},{"after_native_fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"fp_equal",fp_equal},{"compared_all_captured_bytes",compared},{"different_bytes",different},{"different_ranges",ranges},{"persistent_compared_bytes",persistent_compared},{"existing_void_frame_private_range",capture.at("private_returned_stack")},{"private_control_stack_different_bytes",private_different},{"all_contact_packet_bytes_equal",record_packets_equal},{"first_differences",diffs},{"native_reads",memory.reads},{"native_writes",memory.writes},{"native_observed_stores",stores.size()},{"actual_original_service_calls",bridge.calls},{"native_contact_count",native.word(packet+0x114u)},{"original_contact_count",original.word(packet+0x114u)}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=5){std::ofstream output(argv[4]);output<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report.at("passed")},{"different_bytes",report.value("different_bytes",0u)},{"fp_equal",report.value("fp_equal",false)},{"all_contact_packet_bytes_equal",report.value("all_contact_packet_bytes_equal",false)},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;}
