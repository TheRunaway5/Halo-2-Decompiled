// Offline physical i386 geometry observer. All changes precede the original
// entry and contain declared inputs only; no instruction or outcome is supplied.
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
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
#include <sstream>
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
std::uint32_t address(const J&o,const std::array<std::uint32_t,8>&r){std::uint32_t p=o.at("displacement");const int base=o.at("base"),index=o.at("index");if(base>=0)p+=r.at(base);if(index>=0)p+=r.at(index)*o.at("scale").get<std::uint32_t>();return p;}
}
int main(int argc,char**argv){try{
 check(argc==5,"Supply original ELF, operand metadata, declared cases, output");std::ifstream mi(argv[2]),ci(argv[3]);const auto metadata=J::parse(mi),cases=J::parse(ci);std::map<std::uint32_t,J>sites;for(const auto&s:metadata.at("sites"))check(sites.emplace(s.at("instruction"),s).second,"Duplicate original instruction");const auto default_stop=metadata.at("stop").get<std::uint32_t>();J result=J::array();std::size_t total{};
 for(const auto&c:cases){const auto stop=c.value("stop_pc",default_stop);
  Child child;child.pid=fork();check(child.pid>=0,"Cannot fork original geometry process");if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
  int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Original geometry wait failed");};wait();check(WIFSTOPPED(status),"Missing original exec stop");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original declared-input bridge failed");
  for(const auto&b:c.at("banks"))memory(child.pid,b.at("address"),bytes(b.at("before")));
  // Declared platform input: use the same real Linux mprotect owner as the
  // import bridge to mark actual original allocation pages read-only before
  // the original source entry. No source instruction or outcome is replaced.
  if(c.value("initial_read_only",false)) {
   user_regs_struct setup{};request(PTRACE_GETREGS,child.pid,nullptr,&setup);
   setup.rip=0x800000b6u;setup.rsp=0x200e000u;setup.eflags=0x202;
   std::array<std::uint32_t,4> args{default_stop,0x81000001u,0x1fffu,2u};
   std::vector<std::byte> frame(16);for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)frame[n*4+k]=std::byte(args[n]>>(k*8u));
   memory(child.pid,setup.rsp,frame);request(PTRACE_SETREGS,child.pid,nullptr,&setup);
   for(unsigned n=0;n<100;++n) {
    request(PTRACE_GETREGS,child.pid,nullptr,&setup);if(std::uint32_t(setup.rip)==default_stop)break;
    request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original platform-input page protection failed");
   }
   request(PTRACE_GETREGS,child.pid,nullptr,&setup);check(std::uint32_t(setup.rip)==default_stop&&setup.rax==0,"Original platform-input mprotect did not complete");
  }
  const auto initial=c.at("gp").get<std::array<std::uint32_t,8>>();user_regs_struct regs{};request(PTRACE_GETREGS,child.pid,nullptr,&regs);regs.rax=initial[0];regs.rcx=initial[1];regs.rdx=initial[2];regs.rbx=initial[3];regs.rsp=initial[4];regs.rbp=initial[5];regs.rsi=initial[6];regs.rdi=initial[7];regs.rip=c.at("entry").get<std::uint32_t>();regs.eflags=0x202;request(PTRACE_SETREGS,child.pid,nullptr,&regs);
  user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);fp.cwd=c.at("before_fp")[0];fp.swd=c.at("before_fp")[1];fp.mxcsr=c.at("before_fp")[2];fp.ftw=0;std::memset(fp.st_space,0,sizeof(fp.st_space));const auto depth=c.at("live_x87").size();const auto top=(fp.swd>>11u)&7u;for(unsigned n=0;n<depth;++n){const auto b=bytes(c.at("live_x87")[n]);check(b.size()==10,"Invalid declared extended input");std::memcpy(reinterpret_cast<std::byte*>(fp.st_space)+n*16,b.data(),10);fp.ftw|=1u<<((top+n)&7u);}for(unsigned n=0;n<32;++n)fp.xmm_space[n]=0x6b000000u+c.at("id").get<unsigned>()*37u+n;request(PTRACE_SETFPREGS,child.pid,nullptr,&fp);user_fpregs_struct entry_fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&entry_fp);check(entry_fp.cwd==fp.cwd&&entry_fp.swd==fp.swd&&entry_fp.mxcsr==fp.mxcsr,"Original declared controls were not installed exactly");
  std::vector<std::byte>stack(12);const std::array<std::uint32_t,3>args{default_stop,c.at("matrix"),c.at("source")};for(unsigned n=0;n<3;++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(args[n]>>(k*8u));memory(child.pid,initial[4],stack);
  auto retained=[&](std::uint32_t p,unsigned n){for(const auto&b:c.at("banks")){const auto first=b.at("address").get<std::uint32_t>();if(p>=first&&std::uint64_t(p-first)+n<=b.at("before").get_ref<const std::string&>().size()/2)return true;}return false;};
  J reads=J::array(),writes=J::array(),services=J::array(),events=J::array(),boundary=nullptr;std::set<std::uint32_t>executed;unsigned steps{},read_count{},write_count{};bool finished{};
  const auto fault=c.value("fault",J::object());const std::string phase=fault.value("phase","");const auto ordinal=fault.value("ordinal",0u);
  for(;steps<20000;++steps){
   request(PTRACE_GETREGS,child.pid,nullptr,&regs);const auto pc=std::uint32_t(regs.rip);if(pc==stop){if(c.contains("required_owner")){
    std::uint32_t identity{};
    if(pc==0x12fd40u)identity=std::uint32_t(regs.rsi);
    else{const auto actual_argument=memory(child.pid,std::uint32_t(regs.rsp)+4u,4);for(unsigned k=0;k<4;++k)identity|=std::to_integer<unsigned>(actual_argument[k])<<(k*8u);}
    check(identity==c.at("required_identity").get<std::uint32_t>(),"Original required-owner argument differs from declared profile");
    boundary=J{{"phase","required_owner"},{"entry",pc},{"identity",identity}};
   }else finished=true;break;}
   if(pc==0x800000b6u) {
    const auto words=memory(child.pid,std::uint32_t(regs.rsp),16);auto word=[&](unsigned n){std::uint32_t v{};for(unsigned k=0;k<4;++k)v|=std::to_integer<unsigned>(words[n*4+k])<<(k*8u);return v;};
    services.push_back(J::array({word(1),word(2),word(3)}));events.push_back(J::array({"protect",word(1),word(2),word(3)}));
    const auto continuation=word(0);for(unsigned n=0;n<100;++n){request(PTRACE_GETREGS,child.pid,nullptr,&regs);if(std::uint32_t(regs.rip)==continuation)break;request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Actual original import mprotect failed");}
    request(PTRACE_GETREGS,child.pid,nullptr,&regs);check(std::uint32_t(regs.rip)==continuation&&regs.rax==0,"Actual original import returned host mprotect failure");
    continue;
   }
   const auto it=sites.find(pc);check(it!=sites.end(),"Original resource retirement escaped authenticated source bodies");executed.insert(pc);J pending_reads=J::array(),pending_writes=J::array();bool stop_before{};
   for(const auto&o:it->second.at("operands")){
    // PTRACE also stops once on a zero-count REP instruction. Such an
    // instruction has an authenticated operand but performs no memory access.
    if(std::uint32_t(regs.rcx)==0&&(it->second.at("bytes")=="f3ab"||it->second.at("bytes")=="f3aa"))continue;
    const auto p=address(o,gp(regs)),n=o.at("bytes").get<unsigned>();if(!retained(p,n))continue;
    if(o.at("read").get<bool>()){if(phase=="before_read"&&read_count==ordinal){boundary=J{{"phase",phase},{"instruction",pc},{"address",p},{"bytes",n}};stop_before=true;break;}++read_count;pending_reads.push_back(J{{"instruction",pc},{"address",p},{"bytes",n}});}
    if(o.at("write").get<bool>()){if(phase=="before_write"&&write_count==ordinal){boundary=J{{"phase",phase},{"instruction",pc},{"address",p},{"bytes",n}};stop_before=true;break;}pending_writes.push_back(J{{"instruction",pc},{"address",p},{"bytes",n},{"before",hex(memory(child.pid,p,n))}});}
   }
   if(stop_before)break;
   request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status),"Original geometry left without a boundary");
   if(WSTOPSIG(status)==SIGSEGV){
    if(c.value("actual_read_fault",false)) {
     const auto operands=it->second.at("operands");check(operands.size()==1&&operands[0].at("read").get<bool>()&&!operands[0].at("write").get<bool>(),"Original fault is not the declared scalar read");
     boundary=J{{"phase","actual_read_fault"},{"instruction",pc},{"address",address(operands[0],gp(regs))},{"bytes",operands[0].at("bytes")}};break;
    }
    if(!c.value("alignment_fault",false)){std::cerr<<"Original resource input fault: "<<c.at("name")<<" pc="<<std::hex<<pc<<std::dec<<"\n";throw std::runtime_error("Unexpected original resource input fault");}boundary=J{{"phase","alignment_fault"},{"instruction",pc}};break;}
   check(WSTOPSIG(status)==SIGTRAP,"Unexpected original geometry stop");for(const auto&r:pending_reads){reads.push_back(r);events.push_back(J::array({"read",r.at("address"),r.at("bytes")}));}for(auto&w:pending_writes){w["written"]=hex(memory(child.pid,w.at("address").get<std::uint32_t>(),w.at("bytes").get<std::size_t>()));w.erase("bytes");writes.push_back(w);events.push_back(J::array({"write",w.at("address"),w.at("written")}));if(phase=="after_write"&&write_count==ordinal)boundary=J{{"phase",phase},{"instruction",pc},{"address",w.at("address")}};++write_count;}
   if(!boundary.is_null())break;
  }
  check(finished||!boundary.is_null(),"Original geometry did not reach its declared boundary");if(!phase.empty())check(!finished&&boundary.at("phase")==phase,"Original geometry prefix did not stop");request(PTRACE_GETREGS,child.pid,nullptr,&regs);request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);check(fp.cwd==entry_fp.cwd&&fp.swd==entry_fp.swd&&fp.ftw==entry_fp.ftw&&!std::memcmp(fp.st_space,entry_fp.st_space,sizeof(fp.st_space)),"Original geometry unexpectedly changed live x87 values/status");
  J banks=J::array();for(const auto&b:c.at("banks"))banks.push_back(J{{"address",b.at("address")},{"after",hex(memory(child.pid,b.at("address"),b.at("before").get_ref<const std::string&>().size()/2))}});
  J permissions=J::array();for(unsigned k=0;k<4;++k){
   std::ifstream maps("/proc/"+std::to_string(child.pid)+"/maps");std::string line;bool found{},read_only{};const auto page=0x81000000u+k*4096u;
   while(std::getline(maps,line)){std::istringstream input(line);std::string range,mode;input>>range>>mode;const auto dash=range.find('-');if(dash==std::string::npos)continue;const auto begin=std::stoull(range.substr(0,dash),nullptr,16),end=std::stoull(range.substr(dash+1),nullptr,16);if(page>=begin&&page<end){found=true;read_only=mode.at(1)!='w';break;}}
   check(found,"Original actual physical page is not mapped");permissions.push_back(read_only?1:0);
  }
  result.push_back(J{{"id",c.at("id")},{"completed",finished},{"boundary",boundary},{"after_fp",J::array({fp.cwd,fp.swd,fp.mxcsr})},{"live_x87_unchanged",true},{"boundary_gp",gp(regs)},{"reads",reads},{"writes",writes},{"services",services},{"events",events},{"physical_read_only",permissions},{"banks",banks},{"executed_sites",executed},{"original_instructions",steps}});total+=steps;
 }
 std::ofstream out(argv[4]);out<<J{{"cases",result},{"original_instructions",total},{"instruction_patches",J::array()},{"supplied_original_outcomes",false}}.dump()<<'\n';check(bool(out),"Original geometry report failed");std::cout<<result.size()<<" physical original geometry cases, "<<total<<" unchanged instructions\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
