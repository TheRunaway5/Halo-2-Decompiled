// Offline physical i386 campaign point observer. All changes precede the original
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
 check(argc==5,"Supply original ELF, operand metadata, declared cases, output");std::ifstream mi(argv[2]),ci(argv[3]);const auto metadata=J::parse(mi),cases=J::parse(ci);std::map<std::uint32_t,J>sites;for(const auto&s:metadata.at("sites"))check(sites.emplace(s.at("instruction"),s).second,"Duplicate original instruction");const auto stop=metadata.at("stop").get<std::uint32_t>();J result=J::array();std::size_t total{};
 for(const auto&c:cases){
  Child child;child.pid=fork();check(child.pid>=0,"Cannot fork original jump arc process");if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
  int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Original jump arc wait failed");};wait();check(WIFSTOPPED(status),"Missing original exec stop");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original declared-input bridge failed");
  for(const auto&b:c.at("banks"))memory(child.pid,b.at("address"),bytes(b.at("before")));
  const auto initial=c.at("gp").get<std::array<std::uint32_t,8>>();user_regs_struct regs{};request(PTRACE_GETREGS,child.pid,nullptr,&regs);regs.rax=initial[0];regs.rcx=initial[1];regs.rdx=initial[2];regs.rbx=initial[3];regs.rsp=initial[4];regs.rbp=initial[5];regs.rsi=initial[6];regs.rdi=initial[7];regs.rip=c.at("entry").get<std::uint32_t>();regs.eflags=0x202;request(PTRACE_SETREGS,child.pid,nullptr,&regs);
  user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);fp.cwd=c.at("before_fp")[0];fp.swd=c.at("before_fp")[1];fp.mxcsr=c.at("before_fp")[2];fp.ftw=0;std::memset(fp.st_space,0,sizeof(fp.st_space));const auto depth=c.at("live_x87").size();const auto top=(fp.swd>>11u)&7u;for(unsigned n=0;n<depth;++n){const auto b=bytes(c.at("live_x87")[n]);check(b.size()==10,"Invalid declared extended input");std::memcpy(reinterpret_cast<std::byte*>(fp.st_space)+n*16,b.data(),10);fp.ftw|=1u<<((top+n)&7u);}for(unsigned n=0;n<32;++n)fp.xmm_space[n]=0x6b000000u+c.at("id").get<unsigned>()*37u+n;request(PTRACE_SETFPREGS,child.pid,nullptr,&fp);user_fpregs_struct entry_fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&entry_fp);check(entry_fp.cwd==fp.cwd&&entry_fp.swd==fp.swd&&entry_fp.mxcsr==fp.mxcsr,"Original declared controls were not installed exactly");
  std::vector<std::uint32_t> args;
  if(c.at("entry").get<std::uint32_t>()==0xff1a0u)args={stop,c.at("speed_bits"),0x3f800000u,0,0,0,1,0,0,0,0,0};
  else args={stop,c.at("second"),c.at("secondary_first")};
  std::vector<std::byte>stack(args.size()*4);for(unsigned n=0;n<args.size();++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(args[n]>>(k*8u));memory(child.pid,initial[4],stack);
  auto retained=[&](std::uint32_t p,unsigned n){for(const auto&b:c.at("banks")){const auto first=b.at("address").get<std::uint32_t>();if(p>=first&&std::uint64_t(p-first)+n<=b.at("before").get_ref<const std::string&>().size()/2)return true;}return false;};
  J reads=J::array(),writes=J::array(),boundary=nullptr;std::set<std::uint32_t>executed;unsigned steps{},read_count{},write_count{};bool finished{};
  const auto fault=c.value("fault",J::object());const std::string phase=fault.value("phase","");const auto ordinal=fault.value("ordinal",0u);
  for(;steps<300000;++steps){
   request(PTRACE_GETREGS,child.pid,nullptr,&regs);const auto pc=std::uint32_t(regs.rip);if(pc==stop){finished=true;break;}const auto it=sites.find(pc);check(it!=sites.end(),"Original jump arc escaped authenticated leaf bodies");executed.insert(pc);J pending_reads=J::array(),pending_writes=J::array();bool stop_before{};
   for(const auto&o:it->second.at("operands")){const auto p=address(o,gp(regs)),n=o.at("bytes").get<unsigned>();if(!retained(p,n))continue;
    if(o.at("read").get<bool>()){if(phase=="before_read"&&read_count==ordinal){boundary=J{{"phase",phase},{"instruction",pc},{"address",p},{"bytes",n}};stop_before=true;break;}++read_count;pending_reads.push_back(J{{"instruction",pc},{"address",p},{"bytes",n}});}
    if(o.at("write").get<bool>()){if(phase=="before_write"&&write_count==ordinal){boundary=J{{"phase",phase},{"instruction",pc},{"address",p},{"bytes",n}};stop_before=true;break;}pending_writes.push_back(J{{"instruction",pc},{"address",p},{"bytes",n},{"before",hex(memory(child.pid,p,n))}});}
   }
   if(stop_before)break;
   request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status),"Original jump arc left without a boundary");
   if(WSTOPSIG(status)==SIGSEGV){
    check(c.value("actual_read_fault",false)||c.value("actual_write_fault",false),"Unexpected original jump arc fault");
    const auto operands=it->second.at("operands");check(operands.size()==1,"Original candidate fault has ambiguous operand");
    boundary=J{{"phase",c.value("actual_read_fault",false)?"actual_read_fault":"actual_write_fault"},{"instruction",pc},{"address",address(operands[0],gp(regs))},{"bytes",operands[0].at("bytes")}};break;
   }
   check(WSTOPSIG(status)==SIGTRAP,"Unexpected original jump arc stop");for(const auto&r:pending_reads)reads.push_back(r);for(auto&w:pending_writes){w["written"]=hex(memory(child.pid,w.at("address").get<std::uint32_t>(),w.at("bytes").get<std::size_t>()));w.erase("bytes");writes.push_back(w);if(phase=="after_write"&&write_count==ordinal)boundary=J{{"phase",phase},{"instruction",pc},{"address",w.at("address")}};++write_count;}
   if(!boundary.is_null())break;
  }
  check(finished||!boundary.is_null(),"Original jump arc did not reach its declared boundary");if(!phase.empty())check(!finished&&boundary.at("phase")==phase,"Original jump arc prefix did not stop");request(PTRACE_GETREGS,child.pid,nullptr,&regs);request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);const auto pending_depth=(((entry_fp.swd>>11u)&7u)-((fp.swd>>11u)&7u))&7u;
  check(fp.cwd==entry_fp.cwd&&pending_depth<=2,"Original jump arc pending arithmetic shape differs");
  J pending=J::array();int pending_kind=-1;std::string pending_family;
  if(pending_depth) {
   for(unsigned n=0;n<pending_depth;++n)pending.push_back(hex(std::span(reinterpret_cast<const std::byte*>(fp.st_space)+n*16,10)));
   const auto pc=boundary.at("instruction").get<std::uint32_t>();
   pending_family="arc";
   if(pc==0xff1db)pending_kind=0;
   else if(pc==0xff1e3||pc==0xff1e7||pc==0xff1f8)pending_kind=1;
   else if(pc==0xff27c)pending_kind=2;
   else if(pc==0xff3e2)pending_kind=3;
   else if(pc==0xff463)pending_kind=4;
   else if(pc>=0x30bd0u&&pc<0x30c34u)pending_kind=5;
   else if(pc==0xff589||pc==0xff598||pc==0xff5a2||pc==0xff5c9){pending_family="fallback";pending_kind=pc==0xff589?0:pc==0xff598?1:pc==0xff5a2?2:3;}
   else {std::cerr<<std::hex<<pc<<std::dec<<" pending "<<pending_depth<<"\n";throw std::runtime_error("Original jump arc has an unauthenticated pending boundary");}
  }
  for(unsigned n=0;n<depth;++n)if(std::memcmp(reinterpret_cast<const std::byte*>(fp.st_space)+(n+pending_depth)*16,reinterpret_cast<const std::byte*>(entry_fp.st_space)+n*16,10)) {
   std::cerr<<"Original arc caller mismatch "<<c.at("name")<<" depth="<<depth<<" pending="<<pending_depth<<" index="<<n<<" before="<<hex(std::span(reinterpret_cast<const std::byte*>(entry_fp.st_space)+n*16,10))<<" after="<<hex(std::span(reinterpret_cast<const std::byte*>(fp.st_space)+(n+pending_depth)*16,10))<<" sw="<<std::hex<<entry_fp.swd<<"/"<<fp.swd<<std::dec<<"\n";
   throw std::runtime_error("Original jump arc changed a caller x87 value");
  }
  if(finished)check(pending_depth==0,"Original completed jump arc retained private arithmetic");
  J banks=J::array();for(const auto&b:c.at("banks"))banks.push_back(J{{"address",b.at("address")},{"after",hex(memory(child.pid,b.at("address"),b.at("before").get_ref<const std::string&>().size()/2))}});
  const auto direction=finished&&c.at("entry").get<std::uint32_t>()==0xff1a0u?hex(memory(child.pid,c.at("private_direction"),12)):std::string{};
  result.push_back(J{{"direction_value",direction},{"id",c.at("id")},{"completed",finished},{"boundary",boundary},{"after_fp",J::array({fp.cwd,fp.swd,fp.mxcsr})},{"live_x87_unchanged",true},{"pending_extended",pending},{"pending_kind",pending_kind},{"pending_family",pending_family},{"boundary_gp",gp(regs)},{"result_bits",fp.xmm_space[0]},{"reads",reads},{"writes",writes},{"banks",banks},{"executed_sites",executed},{"original_instructions",steps}});total+=steps;
 }
 std::ofstream out(argv[4]);out<<J{{"cases",result},{"original_instructions",total},{"instruction_patches",J::array()},{"supplied_original_outcomes",false}}.dump()<<'\n';check(bool(out),"Original jump arc report failed");std::cout<<result.size()<<" physical original jump arc cases, "<<total<<" unchanged instructions\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
