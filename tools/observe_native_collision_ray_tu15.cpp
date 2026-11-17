// Offline physical i386 observer for the TU1.5 collision ray 16C620 replay.
// The unchanged original .text runs single-stepped under ptrace from the
// declared entry. Inputs (declared 64-byte lines, registers, FP words, the
// stack argument block) are installed before the entry; no instruction is
// patched and no callee outcome is supplied. Every executed PC must belong to
// the authenticated owner bodies; every memory operand must lie inside a
// declared line (strict reads and writes), so the replay proves that the
// declared input set is complete for the original.
// Output per case: completion, EAX..EDI at the stop, x87 CW/SW and MXCSR,
// each declared line after the call, instruction count and any undeclared
// operand.
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <csignal>
#include <array>
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
#include <unordered_map>
#include <vector>
namespace {
using J=nlohmann::json;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
long request(__ptrace_request op,pid_t p,void*a=nullptr,void*b=nullptr){errno=0;const auto n=ptrace(op,p,a,b);if(n==-1&&errno)throw std::runtime_error(strerror(errno));return n;}
struct Child{pid_t pid=-1;~Child(){if(pid>0){kill(pid,SIGKILL);int status{};waitpid(pid,&status,0);}}};
std::vector<std::byte> bytes(const std::string&s){check(s.size()%2==0,"Invalid declared input bytes");std::vector<std::byte>b;b.reserve(s.size()/2);for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
std::vector<std::byte> memory(pid_t p,std::uint32_t address,std::size_t size){std::vector<std::byte>b(size);iovec local{b.data(),size},remote{reinterpret_cast<void*>(std::uintptr_t(address)),size};check(process_vm_readv(p,&local,1,&remote,1,0)==std::ptrdiff_t(size),"Cannot read original storage");return b;}
void memory(pid_t p,std::uint32_t address,std::span<const std::byte>b){iovec local{const_cast<std::byte*>(b.data()),b.size()},remote{reinterpret_cast<void*>(std::uintptr_t(address)),b.size()};check(process_vm_writev(p,&local,1,&remote,1,0)==std::ptrdiff_t(b.size()),"Cannot install declared original input storage");}
std::array<std::uint32_t,8> gp(const user_regs_struct&r){return{std::uint32_t(r.rax),std::uint32_t(r.rcx),std::uint32_t(r.rdx),std::uint32_t(r.rbx),std::uint32_t(r.rsp),std::uint32_t(r.rbp),std::uint32_t(r.rsi),std::uint32_t(r.rdi)};}
struct Operand{int base,index;std::uint32_t scale,displacement,bytes;};
struct Site{std::vector<Operand> operands;};
}
int main(int argc,char**argv){try{
 check(argc==5,"Supply original ELF, operand metadata, declared cases, output");
 std::ifstream mi(argv[2]),ci(argv[3]);const auto metadata=J::parse(mi);
 std::unordered_map<std::uint32_t,Site> sites;
 for(const auto&s:metadata.at("sites")){Site site;for(const auto&o:s.at("operands"))site.operands.push_back({o.at("base"),o.at("index"),o.at("scale"),o.at("displacement"),o.at("bytes")});check(sites.emplace(s.at("instruction").get<std::uint32_t>(),site).second,"Duplicate original instruction");}
 const auto stop=metadata.at("stop").get<std::uint32_t>();
 std::ofstream out(argv[4]);std::string line;std::uint64_t total{};std::size_t count{};
 while(std::getline(ci,line)){
  if(line.empty())continue;const auto c=J::parse(line);
  Child child;child.pid=fork();check(child.pid>=0,"Cannot fork original process");
  if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
  int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Original wait failed");};
  wait();check(WIFSTOPPED(status),"Missing original exec stop");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
  request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original declared-input bridge failed");
  std::map<std::uint32_t,std::vector<std::byte>> lines;   // declared 64-byte lines
  for(const auto&l:c.at("lines")){auto b=bytes(l.at(1).get<std::string>());check(b.size()==64,"Declared line is not 64 bytes");memory(child.pid,l.at(0).get<std::uint32_t>(),b);lines.emplace(l.at(0).get<std::uint32_t>(),std::move(b));}
  const auto declared=[&](std::uint32_t p,std::uint32_t n){for(auto l=std::uint64_t(p)&~63ull;l<std::uint64_t(p)+n;l+=64)if(!lines.count(std::uint32_t(l)))return false;return true;};
  const auto initial=c.at("gp").get<std::array<std::uint32_t,8>>();
  user_regs_struct regs{};request(PTRACE_GETREGS,child.pid,nullptr,&regs);
  regs.rax=initial[0];regs.rcx=initial[1];regs.rdx=initial[2];regs.rbx=initial[3];regs.rsp=initial[4];regs.rbp=initial[5];regs.rsi=initial[6];regs.rdi=initial[7];
  regs.rip=c.at("entry").get<std::uint32_t>();regs.eflags=0x202u;request(PTRACE_SETREGS,child.pid,nullptr,&regs);
  user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
  fp.cwd=c.at("fp")[0];fp.swd=c.at("fp")[1];fp.mxcsr=c.at("fp")[2];fp.ftw=0;std::memset(fp.st_space,0,sizeof(fp.st_space));
  for(unsigned n=0;n<32;++n)fp.xmm_space[n]=0x6b000000u+c.at("id").get<unsigned>()*37u+n;
  request(PTRACE_SETFPREGS,child.pid,nullptr,&fp);
  // Return address and the six stdcall arguments at ESP.
  std::vector<std::uint32_t> args{stop};for(const auto&w:c.at("args"))args.push_back(w.get<std::uint32_t>());
  std::vector<std::byte> stack(args.size()*4);for(std::size_t n=0;n<args.size();++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(args[n]>>(k*8u));
  check(declared(initial[4],std::uint32_t(stack.size())),"Argument block is not declared");
  memory(child.pid,initial[4],stack);
  for(std::size_t n=0;n<stack.size();++n){auto&l=lines.at((initial[4]+std::uint32_t(n))&~63u);l[(initial[4]+n)&63u]=stack[n];}
  J undeclared=J::array();std::set<std::uint32_t> executed;std::uint64_t steps{};bool finished{};
  const std::uint64_t max_steps=c.value("max_steps",20000000ull);
  for(;steps<max_steps;++steps){
   request(PTRACE_GETREGS,child.pid,nullptr,&regs);const auto pc=std::uint32_t(regs.rip);
   if(pc==stop){finished=true;break;}
   const auto it=sites.find(pc);
   if(it==sites.end()){std::cerr<<"Original escaped authenticated bodies pc="<<std::hex<<pc<<std::dec<<"\n";throw std::runtime_error("Original escaped authenticated owner bodies");}
   executed.insert(pc);
   const auto r=gp(regs);
   for(const auto&o:it->second.operands){
    std::uint32_t p=o.displacement;if(o.base>=0)p+=r[o.base];if(o.index>=0)p+=r[o.index]*o.scale;
    if(!declared(p,o.bytes)&&undeclared.size()<16)undeclared.push_back(J::array({pc,p,o.bytes}));
   }
   request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status),"Original left without a boundary");
   if(WSTOPSIG(status)!=SIGTRAP){std::cerr<<"Original fault signal "<<WSTOPSIG(status)<<" pc="<<std::hex<<pc<<std::dec<<"\n";break;}
  }
  request(PTRACE_GETREGS,child.pid,nullptr,&regs);request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
  J after=J::array();for(const auto&[address,before]:lines)after.push_back(J::array({address,hex(memory(child.pid,address,64))}));
  out<<J{{"id",c.at("id")},{"completed",finished},{"gp",gp(regs)},{"fp",J::array({fp.cwd,fp.swd,fp.mxcsr})},{"steps",steps},
      {"executed_sites",executed.size()},{"executed",J(std::vector<std::uint32_t>(executed.begin(),executed.end()))},{"undeclared",undeclared},{"lines",after}}.dump()<<'\n';
  total+=steps;++count;
 }
 check(bool(out),"Original report failed");std::cout<<count<<" physical original 16C620 cases, "<<total<<" unchanged instructions\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
