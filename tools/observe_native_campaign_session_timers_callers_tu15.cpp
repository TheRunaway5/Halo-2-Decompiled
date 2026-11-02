// Offline physical i386 session timer callers observer. All changes precede the original
// entry and contain declared inputs only; no instruction or outcome is supplied.
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <time.h>
#include <array>
#include <bit>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <span>
#include <stdexcept>
#include <vector>
namespace {
using J=nlohmann::json;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
long request(__ptrace_request op,pid_t p,void*a=nullptr,void*b=nullptr){errno=0;const auto n=ptrace(op,p,a,b);if(n==-1&&errno)throw std::runtime_error(strerror(errno));return n;}
struct Child{pid_t pid=-1;~Child(){if(pid>0){kill(pid,SIGKILL);int status{};waitpid(pid,&status,0);}}};
std::vector<std::byte> bytes(const std::string&s){check(s.size()%2==0,"Invalid declared input bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
std::vector<std::byte> memory(pid_t p,std::uint32_t address,std::size_t size){std::vector<std::byte>b(size);iovec local{b.data(),size},remote{reinterpret_cast<void*>(std::uintptr_t(address)),size};check(process_vm_readv(p,&local,1,&remote,1,0)==std::ptrdiff_t(size),"Cannot read original input storage");return b;}
void memory(pid_t p,std::uint32_t address,std::span<const std::byte>b){iovec local{const_cast<std::byte*>(b.data()),b.size()},remote{reinterpret_cast<void*>(std::uintptr_t(address)),b.size()};check(process_vm_writev(p,&local,1,&remote,1,0)==std::ptrdiff_t(b.size()),"Cannot install declared original input storage");}
std::array<std::uint32_t,8> gp(const user_regs_struct&r){return{std::uint32_t(r.rax),std::uint32_t(r.rcx),std::uint32_t(r.rdx),std::uint32_t(r.rbx),std::uint32_t(r.rsp),std::uint32_t(r.rbp),std::uint32_t(r.rsi),std::uint32_t(r.rdi)};}
std::uint32_t address(const J&o,const std::array<std::uint32_t,8>&r){std::uint32_t p=o.at("displacement");const int base=o.at("base"),index=o.at("index");if(base>=0)p+=r.at(base);if(index>=0)p+=(r.at(index)&o.value("index_mask",0xffffffffu))*o.at("scale").get<std::uint32_t>();return p;}
}
int main(int argc,char**argv){try{
 check(argc==5,"Supply original ELF, operand metadata, declared cases, output");std::ifstream mi(argv[2]),ci(argv[3]);const auto metadata=J::parse(mi),cases=J::parse(ci);std::map<std::uint32_t,J>sites;for(const auto&s:metadata.at("sites"))check(sites.emplace(s.at("instruction"),s).second,"Duplicate original instruction");const auto stop=metadata.at("stop").get<std::uint32_t>();J result=J::array();std::size_t total{};
 for(const auto&c:cases){
  Child child;child.pid=fork();check(child.pid>=0,"Cannot fork original session timer callers process");if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
  int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Original session timer callers wait failed");};wait();check(WIFSTOPPED(status),"Missing original exec stop");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original declared-input bridge failed");
  for(const auto&b:c.at("banks"))memory(child.pid,b.at("address"),bytes(b.at("before")));
  const auto initial=c.at("gp").get<std::array<std::uint32_t,8>>();user_regs_struct regs{};request(PTRACE_GETREGS,child.pid,nullptr,&regs);regs.rax=initial[0];regs.rcx=initial[1];regs.rdx=initial[2];regs.rbx=initial[3];regs.rsp=initial[4];regs.rbp=initial[5];regs.rsi=initial[6];regs.rdi=initial[7];regs.rip=c.at("entry").get<std::uint32_t>();regs.eflags=0x202;request(PTRACE_SETREGS,child.pid,nullptr,&regs);
  user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);fp.cwd=c.at("before_fp")[0];fp.swd=c.at("before_fp")[1];fp.mxcsr=c.at("before_fp")[2];fp.ftw=0;std::memset(fp.st_space,0,sizeof(fp.st_space));const auto depth=c.at("live_x87").size();const auto top=(fp.swd>>11u)&7u;for(unsigned n=0;n<depth;++n){const auto b=bytes(c.at("live_x87")[n]);check(b.size()==10,"Invalid declared extended input");std::memcpy(reinterpret_cast<std::byte*>(fp.st_space)+n*16,b.data(),10);fp.ftw|=1u<<((top+n)&7u);}for(unsigned n=0;n<32;++n)fp.xmm_space[n]=0x6b000000u+c.at("id").get<unsigned>()*37u+n;request(PTRACE_SETFPREGS,child.pid,nullptr,&fp);user_fpregs_struct entry_fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&entry_fp);check(entry_fp.cwd==fp.cwd&&entry_fp.swd==fp.swd&&entry_fp.mxcsr==fp.mxcsr,"Original declared controls were not installed exactly");
  std::vector<std::byte>stack(12);const std::array<std::uint32_t,3>args{stop,c.at("record_filter"),c.at("slot")};for(unsigned n=0;n<3;++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(args[n]>>(k*8u));if(!c.value("caller",false)||c.value("install_root_stack",false))memory(child.pid,initial[4],stack);
  auto retained=[&](std::uint32_t p,unsigned n){for(const auto&b:c.at("banks")){const auto first=b.at("address").get<std::uint32_t>();if(p>=first&&std::uint64_t(p-first)+n<=b.at("before").get_ref<const std::string&>().size()/2)return true;}return false;};
  J reads=J::array(),writes=J::array(),entries=J::array(),clock_events=J::array(),caller_reads=J::array(),argument_reads=J::array(),return_reads=J::array(),call_packet=nullptr,entry_packet=nullptr,boundary=nullptr;bool entered=false;std::uint32_t entry_sp{};std::set<std::uint32_t>executed;unsigned steps{},read_count{},write_count{};bool finished{};
  const auto fault=c.value("fault",J::object());const std::string phase=fault.value("phase","");const auto ordinal=fault.value("ordinal",0u);
  for(;steps<300000;++steps){
   request(PTRACE_GETREGS,child.pid,nullptr,&regs);const auto pc=std::uint32_t(regs.rip);if(pc==c.value("stop_address",stop)){finished=true;break;}if(pc==c.at("caller_site").get<std::uint32_t>()){
    const auto at=sites.find(pc);check(at!=sites.end(),"Actual caller instruction is outside primary metadata");
    call_packet=J{{"gp",gp(regs)},{"arguments",hex(memory(child.pid,std::uint32_t(regs.rsp),8))},{"instruction_bytes",at->second.at("bytes")},{"eflags",std::uint32_t(regs.eflags)}};
   }
   if(pc==0x777e0u){
    check(!entered&&!call_packet.is_null(),"Timer caller did not execute its genuineCALL");entered=true;entry_sp=std::uint32_t(regs.rsp);user_fpregs_struct at{};request(PTRACE_GETFPREGS,child.pid,nullptr,&at);
    J xmm=J::array();for(unsigned n=0;n<8;++n)xmm.push_back(J::array({at.xmm_space[n*4],at.xmm_space[n*4+1],at.xmm_space[n*4+2],at.xmm_space[n*4+3]}));
    entry_packet=J{{"gp",gp(regs)},{"stack_hex",hex(memory(child.pid,entry_sp,12))},{"eflags",std::uint32_t(regs.eflags)},{"xmm",xmm},{"fp",J::array({at.cwd,at.swd,at.mxcsr})}};entries.push_back(entry_packet);
   }
   if(pc==0x77849u){
    check(entered&&std::uint32_t(regs.rsp)==entry_sp,"Original timer did not restore its trueRETframe");
    if(phase=="before_return_read"){boundary=J{{"phase",phase},{"instruction",pc},{"address",entry_sp},{"bytes",4}};break;}
    return_reads.push_back(J{{"instruction",pc},{"address",entry_sp},{"bytes",4},{"value",hex(memory(child.pid,entry_sp,4))},{"stores_before",writes.size()}});
   }
   if(metadata.at("required_owners").contains(std::to_string(pc))){boundary=J{{"phase","required_owner"},{"instruction",pc}};break;}// This is the actual platform-input boundary, after unchanged IAT read.
   // Publish only a real fresh sample or an explicit replay of that raw sample.
   // It is an external operand producer, never an original callee result/store.
   if(pc==0x33b595u){
    const auto cell=std::uint32_t(regs.rax);const auto index=clock_events.size();
    if(!c.value("clock_available",true)||cell!=0x60212000u){boundary=J{{"phase","required_clock_owner"},{"instruction",pc},{"cell",cell}};break;}
    if(phase=="clock_refresh"&&index==ordinal){boundary=J{{"phase",phase},{"instruction",pc},{"cell",cell}};break;}
    std::uint32_t value{};
    if(c.contains("clock_replay")){
     check(index<c.at("clock_replay").size(),"Raw clock replay exhausted before original volatile load");const auto&e=c.at("clock_replay").at(index);
     check(e.at("kind")=="Milliseconds"&&e.at("operand")==0&&e.at("instruction")==pc&&e.at("cell")==cell&&e.at("phase")=="before_volatile_load"&&e.at("engine_reads_before")==reads.size()&&e.at("engine_stores_before")==writes.size(),"Raw clock replay phase/authority differs");value=e.at("value");
    }else{timespec now{};check(clock_gettime(CLOCK_MONOTONIC,&now)==0,"Cannot sample actual platform monotonic milliseconds");value=std::uint32_t(std::uint64_t(now.tv_sec)*1000u+std::uint64_t(now.tv_nsec)/1000000u);}
    const auto before=hex(memory(child.pid,cell,4));std::array<std::byte,4>published{};for(unsigned k=0;k<4;++k)published[k]=std::byte(value>>(k*8u));memory(child.pid,cell,published);
    clock_events.push_back(J{{"kind","Milliseconds"},{"operand",0},{"instruction",pc},{"cell",cell},{"value",value},{"phase","before_volatile_load"},{"engine_reads_before",reads.size()},{"engine_stores_before",writes.size()},{"before",before},{"written",hex(published)}});
   }
   const auto it=sites.find(pc);check(it!=sites.end(),("Original session timer callers escaped authenticated owners at "+std::to_string(pc)).c_str());executed.insert(pc);J pending_reads=J::array(),pending_writes=J::array();bool stop_before{};
   for(const auto&o:it->second.at("operands")){if(it->second.contains("repeat_count_register")&&!gp(regs)[it->second.at("repeat_count_register").get<unsigned>()])continue;const auto p=address(o,gp(regs)),n=o.at("bytes").get<unsigned>();
    if(entered&&o.at("read").get<bool>()&&n==4&&(p==entry_sp+4u||p==entry_sp+8u)){
     argument_reads.push_back(J{{"instruction",pc},{"address",p},{"bytes",n},{"value",hex(memory(child.pid,p,n))},{"core_reads_before",reads.size()},{"core_stores_before",writes.size()}});continue;
    }
    if(!retained(p,n))continue;
    if(!entered&&o.at("read").get<bool>()){
     caller_reads.push_back(J{{"instruction",pc},{"address",p},{"bytes",n},{"value",hex(memory(child.pid,p,n))}});continue;
    }
    
    if(o.at("read").get<bool>()){if(phase=="before_read"&&read_count==ordinal){boundary=J{{"phase",phase},{"instruction",pc},{"address",p},{"bytes",n}};stop_before=true;break;}++read_count;pending_reads.push_back(J{{"instruction",pc},{"address",p},{"bytes",n},{"stores_before",writes.size()},{"value",hex(memory(child.pid,p,n))}});}
    if(o.at("write").get<bool>()){if(phase=="before_write"&&write_count==ordinal){boundary=J{{"phase",phase},{"instruction",pc},{"address",p},{"bytes",n}};stop_before=true;break;}pending_writes.push_back(J{{"instruction",pc},{"address",p},{"bytes",n},{"before",hex(memory(child.pid,p,n))}});}
   }
   if(stop_before){boundary["uncommitted_reads"]=pending_reads;break;}
   request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status),"Original session timer callers left without a boundary");
   if(WSTOPSIG(status)==SIGSEGV){
    check(c.value("actual_read_fault",false)||c.value("actual_write_fault",false),"Unexpected original participant fault");siginfo_t info{};request(PTRACE_GETSIGINFO,child.pid,nullptr,&info);const auto fault_address=std::uint32_t(reinterpret_cast<std::uintptr_t>(info.si_addr));bool found{};
    for(const auto&o:it->second.at("operands")){const auto at=address(o,gp(regs)),width=o.at("bytes").get<unsigned>();if(fault_address>=at&&std::uint64_t(fault_address)-at<width){boundary=J{{"phase",c.value("actual_read_fault",false)?"actual_read_fault":"actual_write_fault"},{"instruction",pc},{"address",at},{"bytes",width},{"uncommitted_reads",pending_reads}};found=true;break;}}check(found,"Physical participant fault operand not identified");break;
   }
   check(WSTOPSIG(status)==SIGTRAP,"Unexpected original session timer callers stop");for(const auto&r:pending_reads)reads.push_back(r);for(auto&w:pending_writes){w["written"]=hex(memory(child.pid,w.at("address").get<std::uint32_t>(),w.at("bytes").get<std::size_t>()));w.erase("bytes");writes.push_back(w);for(const auto &change:c.value("input_changes",J::array()))if(change.at("after_store")==write_count)memory(child.pid,change.at("address"),bytes(change.at("bytes")));if(phase=="after_write"&&write_count==ordinal)boundary=J{{"phase",phase},{"instruction",pc},{"address",w.at("address")}};++write_count;}
   if(!boundary.is_null())break;
  }
  check(finished||!boundary.is_null(),"Original session timer callers did not reach its declared boundary");if(c.contains("clock_replay"))check(clock_events.size()==c.at("clock_replay").size(),"Raw clock replay has unused events");if(!phase.empty())check(!finished&&boundary.at("phase")==phase,"Original session timer callers prefix did not stop");request(PTRACE_GETREGS,child.pid,nullptr,&regs);request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
  // Both timer owners are integer-only and preserve all live FP values.
  const unsigned pending=(((entry_fp.swd>>11u)&7u)-((fp.swd>>11u)&7u))&7u;
  check(pending+depth<=8,"Original session timer callers exhausted the admitted private x87 capacity");
  for(unsigned n=0;n<depth;++n)check(!std::memcmp(reinterpret_cast<const std::byte*>(fp.st_space)+(pending+n)*16,reinterpret_cast<const std::byte*>(entry_fp.st_space)+n*16,10),"Original session timer callers changed an older live x87 value");
  if(finished)check(fp.cwd==entry_fp.cwd,"Original session timer callers leaked its private CRT control word");
  J boundary_xmm=J::array();for(unsigned n=0;n<8;++n)boundary_xmm.push_back(J::array({fp.xmm_space[n*4],fp.xmm_space[n*4+1],fp.xmm_space[n*4+2],fp.xmm_space[n*4+3]}));J boundary_x87=J::array();for(unsigned n=0;n<depth;++n)boundary_x87.push_back(hex(std::span(reinterpret_cast<const std::byte*>(fp.st_space)+n*16,10)));check(std::equal(std::begin(fp.xmm_space),std::end(fp.xmm_space),std::begin(entry_fp.xmm_space)),"Original timer caller changed an XMM value");
  J banks=J::array();for(const auto&b:c.at("banks"))banks.push_back(J{{"address",b.at("address")},{"after",hex(memory(child.pid,b.at("address"),b.at("before").get_ref<const std::string&>().size()/2))}});
  result.push_back(J{{"id",c.at("id")},{"completed",finished},{"boundary",boundary},{"after_fp",J::array({fp.cwd,fp.swd,fp.mxcsr})},{"live_x87_unchanged",true},{"callee_entries",entries},{"clock_events",clock_events},{"caller_reads",caller_reads},{"argument_reads",argument_reads},{"return_reads",return_reads},{"call_packet",call_packet},{"entry_packet",entry_packet},{"boundary_xmm",boundary_xmm},{"boundary_x87",boundary_x87},{"boundary_eflags",std::uint32_t(regs.eflags)},{"pending_x87_temporaries",pending},{"boundary_gp",gp(regs)},{"result_bits",fp.xmm_space[0]},{"reads",reads},{"writes",writes},{"banks",banks},{"executed_sites",executed},{"original_instructions",steps}});total+=steps;
 }
 std::ofstream out(argv[4]);out<<J{{"cases",result},{"original_instructions",total},{"instruction_patches",J::array()},{"supplied_original_outcomes",false}}.dump()<<'\n';check(bool(out),"Original session timer callers report failed");std::cout<<result.size()<<" physical original session timer callers cases, "<<total<<" unchanged instructions\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
