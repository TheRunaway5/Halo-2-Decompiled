// Self-contained offline physical bridge for actual activation inputs.
// The semantic kernel never receives GP/SIMD postimages or recorded results.
#include "halo2/native_tu15_activation_agents.hpp"
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
long request(__ptrace_request operation,pid_t child,void*address=nullptr,void*value=nullptr){errno=0;const auto result=ptrace(operation,child,address,value);if(result==-1&&errno)throw std::runtime_error(strerror(errno));return result;}
struct Child {
 pid_t pid=-1;int status{};
 ~Child(){if(pid>0){kill(pid,SIGKILL);waitpid(pid,&status,0);}}
 void wait(){check(waitpid(pid,&status,0)==pid,"Physical activation wait failed");check(WIFSTOPPED(status),"Original activation left without boundary");}
 void debug(unsigned index,unsigned long value){request(PTRACE_POKEUSER,pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)+index*sizeof(unsigned long)),reinterpret_cast<void*>(value));}
 user_regs_struct regs(){user_regs_struct result{};request(PTRACE_GETREGS,pid,nullptr,&result);return result;}
 user_fpregs_struct fp(){user_fpregs_struct result{};request(PTRACE_GETFPREGS,pid,nullptr,&result);return result;}
 void go(std::uint32_t target){debug(0,target);debug(6,0);debug(7,1);request(PTRACE_CONT,pid);wait();const auto r=regs();check(WSTOPSIG(status)==SIGTRAP&&r.rip==target,"Physical activation service failed before target "+std::to_string(target)+" at "+std::to_string(r.rip)+" signal "+std::to_string(WSTOPSIG(status)));debug(7,0);}
 void step(){request(PTRACE_SINGLESTEP,pid);wait();check(WSTOPSIG(status)==SIGTRAP,"Physical original activation instruction failed");}
 void start(const char*executable,unsigned){pid=fork();check(pid>=0,"Cannot fork original activation process");if(!pid){const auto fd=open("/dev/null",O_WRONLY);dup2(fd,1);close(fd);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(executable,executable,nullptr);_exit(127);}wait();request(PTRACE_SETOPTIONS,pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,pid);wait();check(WSTOPSIG(status)==SIGTRAP,"Original activation BEFORE bridge failed");}
 std::vector<std::byte>read(std::uint32_t address,std::size_t size){std::vector<std::byte>bytes(size);iovec local{bytes.data(),size},remote{reinterpret_cast<void*>(std::uintptr_t(address)),size};check(process_vm_readv(pid,&local,1,&remote,1,0)==std::ptrdiff_t(size),"Cannot read original activation storage "+std::to_string(address));return bytes;}
 void write(std::uint32_t address,std::span<const std::byte>bytes){iovec local{const_cast<std::byte*>(bytes.data()),bytes.size()},remote{reinterpret_cast<void*>(std::uintptr_t(address)),bytes.size()};check(process_vm_writev(pid,&local,1,&remote,1,0)==std::ptrdiff_t(bytes.size()),"Cannot install original activation BEFORE input "+std::to_string(address));}
 std::uint32_t word(std::uint32_t address){const auto bytes=read(address,4);std::uint32_t result{};for(unsigned n=0;n<4;++n)result|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return result;}
};
J fp_words(const user_fpregs_struct&fp){return J::array({fp.cwd,fp.swd,fp.mxcsr});}
std::string activation_hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4u];s+=d[n&15u];}return s;}
std::uint32_t activation_register(const user_regs_struct&r,const std::string&name){
 if(name.empty())return 0;if(name=="eax")return r.rax;if(name=="ecx")return r.rcx;if(name=="edx")return r.rdx;if(name=="ebx")return r.rbx;if(name=="esp")return r.rsp;if(name=="ebp")return r.rbp;if(name=="esi")return r.rsi;if(name=="edi")return r.rdi;throw std::runtime_error("Unqualified activation address register");
}
struct ActivationMemory:SourcePhysicsContactMemory{
 Child&child;std::uint32_t private_first,private_end;mutable J ledger=J::array();
 ActivationMemory(Child&c,std::uint32_t first,std::uint32_t end):child(c),private_first(first),private_end(end){}
 bool retained(std::uint32_t p,unsigned n)const{return p<private_first||std::uint64_t(p)+n>private_end;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{const auto bytes=child.read(p,b.size());std::copy(bytes.begin(),bytes.end(),b.begin());if(retained(p,b.size()))ledger.push_back(J::array({"read",p,b.size(),activation_hex(b)}));}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{child.write(p,b);if(retained(p,b.size()))ledger.push_back(J::array({"write",p,b.size(),activation_hex(b)}));}
};
J activation_trace(Child&child,const J&metadata,const ActivationMemory&contract,J&calls,std::size_t&steps){
 J ledger=J::array();
 while(std::uint32_t(child.regs().rip)!=0x1ccacau){
  check(++steps<4096,"Activation parent exceeded bounded original instructions");const auto r=child.regs();const auto pc=std::uint32_t(r.rip);const auto key=std::to_string(pc);check(metadata.contains(key),"Activation parent left authenticated CFG at "+key);const auto&i=metadata.at(key);
  struct Write{std::size_t n;std::uint32_t p;unsigned size;};std::vector<Write>writes;
  for(const auto&m:i.at("memory")){const auto p=activation_register(r,m.at("base"))+activation_register(r,m.at("index"))*m.at("scale").get<unsigned>()+m.at("displacement").get<std::uint32_t>();const auto size=m.at("bytes").get<unsigned>();if(!contract.retained(p,size))continue;if(m.at("read").get<bool>())ledger.push_back(J::array({"read",p,size,activation_hex(child.read(p,size))}));if(m.at("write").get<bool>()){writes.push_back({ledger.size(),p,size});ledger.push_back(J::array({"write",p,size,""}));}}
  if(i.contains("service_arguments")){
   child.step();const auto entered=child.regs();const auto returned=pc+i.at("size").get<unsigned>();J arguments=J::array();for(unsigned n=0;n<i.at("service_arguments").get<unsigned>();++n)arguments.push_back(child.word(std::uint32_t(entered.rsp)+4u+n*4u));J event{{"entry",std::uint32_t(entered.rip)},{"this",std::uint32_t(entered.rcx)},{"arguments",arguments},{"original_return_boundary",returned},{"before_esp",std::uint32_t(entered.rsp)},{"before_fp",fp_words(child.fp())},{"ecx_consumed",i.at("ecx_consumed")},{"result_consumed",i.at("result_consumed")}};child.go(returned);const auto after=child.regs();event.update(J{{"returned_eax",std::uint32_t(after.rax)},{"after_esp",std::uint32_t(after.rsp)},{"after_fp",fp_words(child.fp())}});calls.push_back(event);
  }else child.step();
  for(const auto&w:writes)ledger[w.n][3]=activation_hex(child.read(w.p,w.size));
 }
 return ledger;
}
struct ActivationBridge{
 Child&child;std::uint32_t pair_base,body_base;user_fpregs_struct entry_fp;J calls=J::array();
 std::uint32_t call(std::uint32_t owner,std::uint32_t receiver,std::initializer_list<std::uint32_t>args,std::uint32_t returned,NativeTu15PhysicsFrameFp&fp,bool body=false){
  const auto base=body?body_base:pair_base;const std::uint32_t sp=base-4u*(args.size()+1u);std::vector<std::byte>bytes((args.size()+1u)*4u);unsigned n{};const auto append=[&](auto value){for(unsigned k=0;k<4;++k)bytes[n*4+k]=std::byte(value>>(k*8u));++n;};append(returned);for(auto value:args)append(value);child.write(sp,bytes);
  auto r=child.regs();r.rip=owner;r.rcx=receiver;r.rsp=sp;request(PTRACE_SETREGS,child.pid,nullptr,&r);auto f=child.fp();f.cwd=fp.control_word;f.swd=fp.status;f.mxcsr=fp.mxcsr;f.ftw=entry_fp.ftw;std::memcpy(f.st_space,entry_fp.st_space,sizeof(f.st_space));request(PTRACE_SETFPREGS,child.pid,nullptr,&f);const auto before=fp_words(f);child.go(returned);r=child.regs();f=child.fp();fp={f.cwd,f.swd,f.mxcsr};calls.push_back(J{{"entry",owner},{"this",receiver},{"arguments",args},{"original_return_boundary",returned},{"before_esp",sp},{"after_esp",std::uint32_t(r.rsp)},{"before_fp",before},{"after_fp",fp_words(f)},{"returned_eax",std::uint32_t(r.rax)}});return r.rax;
 }
};
}
int main(int argc,char**argv){J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false},{"native_parent_receives_guest_register_state",false},{"remaining_services_execute_original_cpu",true},{"production_native_only_closure",false}};try{
 check(argc==7,"Supply ELF, capture, island ordinal, declared variation, metadata and output");std::ifstream capture_input(argv[2]),metadata_input(argv[5]);const auto capture=J::parse(capture_input),metadata=J::parse(metadata_input);const auto ordinal=std::stoul(argv[3]),variation=std::stoul(argv[4]);Child original;original.start(argv[1],0);const auto entry=original.regs();auto initial_fp=original.fp();check(initial_fp.ftw==0,"Activation actual capture requires empty incoming x87");
 const auto world=original.word(0x52be48u),islands=original.word(world+8u);check(ordinal<original.word(world+12u),"Activation actual island ordinal unavailable");const auto island=original.word(islands+ordinal*4u),entries=original.word(island+0x68u),count=original.word(island+0x6cu);check(count>0&&count<=16,"Actual activation contact cohort unavailable");
 const auto put=[&](std::uint32_t p,std::uint32_t value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
 if(variation&1u)for(unsigned n=0;n<count;++n)put(entries+n*16u+8u,0);
 if(variation&2u)put(world+0xd0u,0);
 if(variation&8u){const auto bodies=original.word(island+0x3cu),body_count=original.word(island+0x40u);check(body_count<=16,"Unbounded declared body-policy cohort");const auto refs=original.word(0x48d3dcu);put(0x48d3dcu,refs+(body_count<<16u));for(unsigned n=0;n<body_count;++n)put(original.word(bodies+n*4u)+0x54u,0x48d3d8u);}
 if(variation&4u)put(island+0x40u,0);
 if(const auto control=variation>>4u){check(control<=16,"Unqualified declared FP mode");const auto mode=control-1u;initial_fp.cwd=0x27fu|((mode&3u)<<10u);initial_fp.swd=0x100u;initial_fp.mxcsr=0x1f80u|((mode&3u)<<13u)|std::array{0u,0x40u,0x8000u,0x8040u}[mode>>2u];request(PTRACE_SETFPREGS,original.pid,nullptr,&initial_fp);}
 const auto sp=std::uint32_t(entry.rsp)-0x1000u;put(sp,0x1ccacau);put(sp+4u,island);auto root_entry=entry;root_entry.rip=0x282870;root_entry.rsp=sp;request(PTRACE_SETREGS,original.pid,nullptr,&root_entry);
 struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;for(const auto&row:capture.at("regions")){const auto address=row.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,row.at("size")),{}});}const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>();check(sp>private_first+0x400u&&sp+8u<private_end,"Activation caller-owned private storage unavailable");
 ActivationMemory original_contract(original,private_first,private_end);J source_calls=J::array();std::size_t steps{};const auto source_ledger=activation_trace(original,metadata,original_contract,source_calls,steps);const auto expected_fp=original.fp();for(auto&bank:banks)bank.after=original.read(bank.address,bank.before.size());
 Child plain;plain.start(argv[1],0);for(const auto&bank:banks)plain.write(bank.address,bank.before);request(PTRACE_SETREGS,plain.pid,nullptr,&root_entry);auto before=initial_fp;request(PTRACE_SETFPREGS,plain.pid,nullptr,&before);plain.go(0x1ccaca);
 Child native;native.start(argv[1],0);for(const auto&bank:banks)native.write(bank.address,bank.before);request(PTRACE_SETREGS,native.pid,nullptr,&root_entry);request(PTRACE_SETFPREGS,native.pid,nullptr,&before);ActivationMemory memory(native,private_first,private_end);ActivationBridge bridge{native,sp-40u,sp-32u,initial_fp};NativeTu15ActivationAgentServices services;unsigned classifications{},motion_queries{};
 services.filter_pair=[&](auto receiver,auto owner,auto output,auto first,auto second,auto&fp){return bridge.call(owner.identity,receiver,{output,first.identity,second.identity},0x2828f1u,fp);};
 services.create_manager=[&](auto factory,auto owner,auto first,auto second,auto context,auto&fp){motion_queries=0;return NativeTu15ActivationContactManager{bridge.call(owner.identity,factory,{first.identity,second.identity,context.identity},0x282930u,fp)};};
 services.classify_shape=[&](auto shape,auto owner,auto&fp){return bridge.call(owner.identity,shape.identity,{},classifications++%2u?0x282958u:0x28294fu,fp);};
 services.create_agent=[&](auto owner,auto first,auto second,auto context,auto manager,auto&fp){return NativeTu15ContactAgent{bridge.call(owner.identity,context.identity,{first.identity,second.identity,context.identity,manager.identity},0x28297fu,fp)};};
 services.motion_kind=[&](auto motion,auto owner,auto&fp){const auto returned=motion_queries++?0x2829f4u:0x2829d8u;return bridge.call(owner.identity,motion,{},returned,fp);};
 services.test_discrete_pair=[&](auto dispatcher,auto owner,auto output,auto first,auto second,auto&fp){motion_queries=0;return bridge.call(owner.identity,dispatcher,{output,first.identity,second.identity},0x282a1au,fp);};
 services.release_manager=[&](auto manager,auto owner,auto&fp){motion_queries=0;bridge.call(owner.identity,manager.identity,{},0x282a51u,fp);};
 services.body_policy_kind=[&](auto policy,auto owner,auto&fp){return bridge.call(owner.identity,policy,{},0x282a8eu,fp,true);};
 services.mark_body=[&](auto body,auto flags,auto&fp){bridge.call(0x31b730u,body.identity,{flags},0x282aa0u,fp,true);};
 NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ActivationAgentStats stats;native_tu15_recreate_island_agents(memory,{island},{sp+4u,sp-21u},fp,services,{},&stats);
 //316200 is fully native; its real source call is included in the ordered
 // reads and is independently observed, but removed from residual-call ABI
 // comparison. No recorded return value is supplied to the native kernel.
 J residual_calls=J::array();for(const auto&call:source_calls)if(call.at("entry")!=0x316200u)residual_calls.push_back(call);J service_differences=J::array();if(residual_calls.size()!=bridge.calls.size())service_differences.push_back("Actual service count differs");for(std::size_t n=0;n<std::min(residual_calls.size(),bridge.calls.size());++n){const auto&a=residual_calls[n],&b=bridge.calls[n];for(const auto*key:{"entry","arguments","original_return_boundary","before_esp","after_esp","before_fp","after_fp"})if(a[key]!=b[key])service_differences.push_back(J{{"index",n},{"field",key},{"original",a[key]},{"native",b[key]}});if(a.at("ecx_consumed").get<bool>()&&a.at("this")!=b.at("this"))service_differences.push_back(J{{"index",n},{"field","this"},{"original",a.at("this")},{"native",b.at("this")}});if(a.at("result_consumed").get<bool>()&&a.at("returned_eax")!=b.at("returned_eax"))service_differences.push_back(J{{"index",n},{"field","returned_eax"},{"original",a.at("returned_eax")},{"native",b.at("returned_eax")}});}
 std::size_t different{},plain_differences{},compared{};J differences=J::array();for(const auto&bank:banks){const auto actual=native.read(bank.address,bank.after.size()),control=plain.read(bank.address,bank.after.size());for(std::size_t n=0;n<actual.size();++n){const auto p=bank.address+n;if(p>=private_first&&p<private_end)continue;++compared;if(bank.after[n]!=control[n])++plain_differences;if(bank.after[n]!=actual[n]){++different;if(differences.size()<16)differences.push_back(J{{"address",p},{"original",std::to_integer<unsigned>(bank.after[n])},{"native",std::to_integer<unsigned>(actual[n])}});}}}
 J operand_differences=J::array();for(std::size_t n=0;n<std::min(source_ledger.size(),memory.ledger.size());++n)if(source_ledger[n]!=memory.ledger[n]){operand_differences.push_back(J{{"index",n},{"original",source_ledger[n]},{"native",memory.ledger[n]}});if(operand_differences.size()==8)break;}
 const bool ordered=source_ledger==memory.ledger,fp_equal=fp_words(expected_fp)==J::array({fp.control_word,fp.status,fp.mxcsr}),plain_fp=fp_words(expected_fp)==fp_words(plain.fp());report.update(J{{"passed",different==0&&plain_differences==0&&plain_fp&&fp_equal&&ordered&&service_differences.empty()},{"world",world},{"island",island},{"island_ordinal",ordinal},{"declared_entry_variation",variation},{"persistent_compared_bytes",compared},{"different_bytes",different},{"traced_versus_plain_original_differences",plain_differences},{"original_parent_instructions",steps},{"original_operand_events",source_ledger.size()},{"native_operand_events",memory.ledger.size()},{"ordered_retained_operands_equal",ordered},{"first_operand_differences",operand_differences},{"first_differences",differences},{"service_context_differences",service_differences},{"fp_equal",fp_equal},{"plain_original_fp_equal",plain_fp},{"actual_original_service_calls",source_calls},{"actual_native_service_calls",bridge.calls},{"native_pairs",stats.pairs},{"native_existing_agents",stats.existing_agents},{"native_created_managers",stats.managers_created},{"native_created_agents",stats.factory_calls},{"native_marked_bodies",stats.bodies_marked}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"different_bytes",report.value("different_bytes",0)},{"ordered",report.value("ordered_retained_operands_equal",false)},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;}
