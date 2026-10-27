// Offline physical service bridge. The native producer runs in this x64
// process; each declared remaining service executes the actual original i386
// body in a stopped child. No code patch, expected result or callback replay.
#include "halo2/native_tu15_contact_producer.hpp"
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
namespace {
struct ProducerFailure{};
struct ProducerMemory:Memory{
 bool active{};mutable bool failed{},cleanup{};std::uint32_t reject_read{};mutable unsigned reads_seen{};unsigned read_occurrence{};std::uint32_t record_header{};mutable J events=J::array();explicit ProducerMemory(Child&c):Memory(c){}
 [[noreturn]]void fail()const{failed=true;cleanup=true;throw ProducerFailure{};}
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(active&&reject_read==p&&!failed&&reads_seen++==read_occurrence)fail();if(cleanup&&p==record_header+8u)events.push_back("record_base_read");Memory::read(p,b);}
};
}
int main(int argc,char**argv){J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false},{"source_exception_dispatch_emulated",false},{"source_fs_registration_recreated",false},{"remaining_services_execute_original_cpu",true},{"production_native_only_closure",false}};try{
 check(argc==5,"Supply original ELF, capture.json, producer unwind cases, report");std::ifstream mi(argv[2]),ci(argv[3]);const auto capture=J::parse(mi),cases=J::parse(ci);Child original,native;original.start(argv[1],0);native.start(argv[1],0);const auto entry=original.regs();const auto initial=original.fp();check(initial.ftw==0,"Producer cleanup proof requires its actual empty x87 context");const auto sp=std::uint32_t(entry.rsp),p=((sp-4u)&~15u)-0x330u;const std::array<std::uint32_t,4>args{original.word(sp+4),original.word(sp+8),original.word(sp+12),original.word(sp+16)};struct Bank{std::uint32_t address;std::vector<std::byte>before;};std::vector<Bank>banks;for(const auto&r:capture.at("regions")){const auto a=r.at("address").get<std::uint32_t>();banks.push_back({a,original.read(a,r.at("size"))});}const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>(),fs=capture.at("before").at("fs_base").get<std::uint32_t>();J results=J::array();
 for(const auto&c:cases.at("cases")){
  const auto name=c.at("name").get<std::string>(),injection=c.at("injection").get<std::string>();report["active_case"]=name;for(const auto&b:banks){original.write(b.address,b.before);native.write(b.address,b.before);}auto r=entry;request(PTRACE_SETREGS,original.pid,nullptr,&r);request(PTRACE_SETREGS,native.pid,nullptr,&r);auto f=initial;request(PTRACE_SETFPREGS,original.pid,nullptr,&f);request(PTRACE_SETFPREGS,native.pid,nullptr,&f);NativeTu15PhysicsFrameFp fp{initial.cwd,initial.swd,initial.mxcsr};const auto stop=c.at("source_stop_before").get<std::uint32_t>();original.go(stop);auto expected_fp=original.fp();NativeTu15PhysicsFrameFp reference_fp{expected_fp.cwd,expected_fp.swd,expected_fp.mxcsr};Bridge reference{original,p,initial,report};const auto state=c.at("source_state").get<int>();if(state>=0){auto action_regs=original.regs();action_regs.rbp=p+0x330u;request(PTRACE_SETREGS,original.pid,nullptr,&action_regs);reference.packet=p-0x500u;if(state==1)reference.call(0x37edeb,0,{},0x304ab7,reference_fp);reference.call(0x37ede0,0,{},0x304ab7,reference_fp);}
ProducerMemory memory(native);memory.record_header=p+0x110u;memory.active=true;Bridge bridge{native,p,initial,report};auto services=bridge.services();const auto maybe=[&](std::string_view tag){if(!memory.failed&&injection==tag)memory.fail();};
auto update=services.update_candidates;services.update_candidates=[&,update](auto agent,auto first,auto second,auto context,auto callback,auto&status){maybe("update_before");update(agent,first,second,context,callback,status);maybe("update_after");};
auto initialize=services.initialize_component_buffer;services.initialize_component_buffer=[&,initialize](auto storage,auto count,auto&status){maybe("initialize_before");initialize(storage,count,status);maybe("initialize_after");};
auto query=services.query_contact;services.query_contact=[&,query](auto child,auto owner,auto valid,auto first,auto second,auto context,auto packet,auto&status){maybe("query_before");query(child,owner,valid,first,second,context,packet,status);maybe("query_after");};
auto append=services.append_record;services.append_record=[&,append](auto array,auto&status){maybe("append_before");append(array,status);maybe("append_after");};
auto publish=services.publish_contact;services.publish_contact=[&,publish](auto first,auto second,auto context,auto record,auto value,auto data,auto count,auto&status){maybe("publish_before");publish(first,second,context,record,value,data,count,status);maybe("publish_after");};
auto distance=services.publish_distance;services.publish_distance=[&,distance](auto first,auto second,auto context,auto value,auto data,auto count,auto output,auto owner,auto&status){distance(first,second,context,value,data,count,output,owner,status);maybe("distance_after");};
auto destroy=services.destroy_component_buffer;services.destroy_component_buffer=[&,destroy](auto storage,auto&status){if(memory.cleanup)memory.events.push_back("component_destroy");maybe("destroy_before");destroy(storage,status);if(injection=="final_capacity_read"&&!memory.failed)memory.reject_read=p+0x118u;maybe("destroy_after");};
if(injection=="post_constructor_count_read"){memory.reject_read=std::uint32_t(entry.rcx)+0x10u;memory.read_occurrence=1;}
SourcePhysicsContactRecordObserver observer=[&](const auto&s){if(injection=="post_component_arm_store"&&s.instruction==0x30442au&&!memory.failed)memory.fail();};bool thrown=false;try{native_tu15_produce_contacts(memory,{std::uint32_t(entry.rcx)},{args[0]},{args[1]},{args[2]},{args[3]},{p},fp,0,services,observer);}catch(const ProducerFailure&){thrown=true;}check(thrown,name+" native producer did not propagate its declared exception");
std::size_t raw{},platform{},compared{};J differences=J::array();for(const auto&b:banks){const auto expected=original.read(b.address,b.before.size()),actual=native.read(b.address,b.before.size());for(std::size_t n=0;n<actual.size();++n){const auto address=b.address+n;if(address>=private_first&&address<private_end)continue;++compared;if(expected[n]==actual[n])continue;++raw;if(address>=fs&&address<fs+4u)++platform;if(differences.size()<32)differences.push_back(J{{"address",address},{"original",std::to_integer<unsigned>(expected[n])},{"native",std::to_integer<unsigned>(actual[n])}});}}
const auto desired=state==1?J::array({"component_destroy","record_base_read"}):state==0?J::array({"record_base_read"}):J::array();const bool order=memory.events==desired,header=original.read(p+0x110u,12)==native.read(p+0x110u,12),component=state<1||original.read(p+0x6cu,20)==native.read(p+0x6cu,20),fp_equal=J::array({reference_fp.control_word,reference_fp.status,reference_fp.mxcsr})==J::array({fp.control_word,fp.status,fp.mxcsr});const bool passed=raw==platform&&order&&header&&component&&fp_equal;results.push_back(J{{"name",name},{"native_cleanup_semantics_passed",passed},{"whole_bank_raw_passed",raw==0&&header&&component&&fp_equal},{"raw_different_bytes",raw},{"source_platform_registration_different_bytes",platform},{"physics_different_bytes",raw-platform},{"cleanup_events",memory.events},{"source_action_order",desired},{"ownership_headers_equal",header&&component},{"fp_equal",fp_equal},{"persistent_compared_bytes",compared},{"first_differences",differences},{"actual_original_cleanup_calls",reference.calls},{"actual_original_native_services",bridge.calls}});report["cases"]=results;
  }
  const bool passed=std::all_of(results.begin(),results.end(),[](const auto&r){return r.at("native_cleanup_semantics_passed").template get<bool>();});report.update(J{{"passed",passed},{"native_cleanup_semantics_only",true},{"physical_cases",results.size()},{"ownership_provenance",cases.at("ownership_provenance")},{"scope","Inline actual producer context: genuine original source prefixes and actual original state1/state0 funclets versus native C++ exception guards. Existing FS raw differences and dynamic opaque-record gates remain open."}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report.at("passed")},{"cases",report.value("physical_cases",0u)},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;}
