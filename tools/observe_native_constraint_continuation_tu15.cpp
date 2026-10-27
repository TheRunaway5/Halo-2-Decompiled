// Physical full original constraint call and genuine solver-return observer. Single steps the unchanged i386
// executable, records declared original data operands, and never supplies a
// math result, replaces an instruction, or resumes past an unowned boundary.
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <vector>
namespace {
using J=nlohmann::json;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
long request(__ptrace_request op,pid_t child,void*a=nullptr,void*b=nullptr){errno=0;auto n=ptrace(op,child,a,b);if(n==-1&&errno)throw std::runtime_error(strerror(errno));return n;}
struct Child{pid_t pid=-1;~Child(){if(pid>0){kill(pid,SIGKILL);int status{};waitpid(pid,&status,0);}}};
std::string hex(const std::vector<std::byte>&b){const char*d="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
std::vector<std::byte> memory(pid_t child,std::uint32_t p,std::size_t n){std::vector<std::byte>b(n);iovec local{b.data(),n},remote{reinterpret_cast<void*>(std::uintptr_t(p)),n};check(process_vm_readv(child,&local,1,&remote,1,0)==std::ptrdiff_t(n),"Cannot read original operand storage");return b;}
std::array<std::uint32_t,8> gp(const user_regs_struct&r){return{std::uint32_t(r.rax),std::uint32_t(r.rcx),std::uint32_t(r.rdx),std::uint32_t(r.rbx),std::uint32_t(r.rsp),std::uint32_t(r.rbp),std::uint32_t(r.rsi),std::uint32_t(r.rdi)};}
std::uint32_t operand(const J&a,const std::array<std::uint32_t,8>&r){auto p=a.at("displacement").get<std::uint32_t>();const auto base=a.at("base").get<int>(),index=a.at("index").get<int>();if(base>=0)p+=r.at(base);if(index>=0)p+=r.at(index)*a.at("scale").get<std::uint32_t>();return p;}
}
int main(int argc,char**argv){try{
  check(argc==5,"Supply ELF, authenticated operand metadata, output, stop boundary");std::ifstream input(argv[2]);const auto m=J::parse(input);const auto stop=std::stoul(argv[4],nullptr,0);
  std::map<std::uint32_t,J> sites;for(const auto&s:m.at("sites"))check(sites.emplace(s.at("instruction"),s).second,"Duplicate original instruction descriptor");
  auto retained=[&](std::uint32_t p,std::uint32_t n){for(const auto&r:m.at("regions")){const auto first=r.at("address").get<std::uint32_t>();if(p>=first&&std::uint64_t(p-first)+n<=r.at("bytes").get<std::uint32_t>())return true;}return false;};
  Child child;child.pid=fork();check(child.pid>=0,"Cannot create original pose process");if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
  int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Original pose wait failed");};wait();check(WIFSTOPPED(status),"Missing original exec stop");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original pose input setup failed");request(PTRACE_SINGLESTEP,child.pid);wait();
  J writes=J::array(),reads=J::array(),estimates=J::array(),service=nullptr;user_regs_struct regs{};unsigned count=0;
  for(;count<10000;++count){
    check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original pose execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);const auto pc=std::uint32_t(regs.rip);
    if(pc==0x3264e7&&service.is_null()){
      user_fpregs_struct state{};request(PTRACE_GETFPREGS,child.pid,nullptr,&state);
      const auto root_stack=std::uint32_t(regs.rsp)+0x10u;
      const auto locals=memory(child.pid,root_stack,64),arguments=memory(child.pid,std::uint32_t(regs.rbp)+8u,36);
      J local_words=J::array(),argument_words=J::array(),snapshot_banks=J::array();
      for(unsigned i=0;i<16;++i){std::uint32_t value;std::memcpy(&value,locals.data()+i*4,4);local_words.push_back(value);}
      for(unsigned i=0;i<9;++i){std::uint32_t value;std::memcpy(&value,arguments.data()+i*4,4);argument_words.push_back(value);}
      for(const auto&r:m.at("regions"))snapshot_banks.push_back(J{{"address",r.at("address")},{"after",hex(memory(child.pid,r.at("address"),r.at("bytes")))}});
      service=J{{"instruction",pc},{"gp",gp(regs)},{"fp",J::array({state.cwd,state.swd,state.mxcsr})},{"x87_tag",state.ftw},{"root_local_words",local_words},{"argument_words",argument_words},{"banks",snapshot_banks},{"read_index",reads.size()},{"write_index",writes.size()},{"original_instructions",count}};
    }
    if(pc==stop)break;
    const auto site=sites.find(pc);check(site!=sites.end(),"Original pose escaped authenticated complete bodies");const auto registers=gp(regs);J pending=J::array();
    for(const auto&a:site->second.at("operands")){
      const auto p=operand(a,registers),n=a.at("bytes").get<std::uint32_t>();if(!retained(p,n))continue;
      if(a.at("read").get<bool>())reads.push_back(J{{"instruction",pc},{"address",p},{"bytes",n}});
      if(a.at("write").get<bool>())pending.push_back(J{{"instruction",pc},{"address",p},{"before",hex(memory(child.pid,p,n))},{"bytes",n}});
    }
    std::uint32_t estimate_input{};if(pc==0x1e1042){user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);estimate_input=fp.xmm_space[0];}
    request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original pose instruction fault");
    for(auto&a:pending){a["written"]=hex(memory(child.pid,a.at("address"),a.at("bytes")));a.erase("bytes");writes.push_back(a);}
    if(pc==0x1e1042){user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);estimates.push_back(J::array({estimate_input,fp.xmm_space[4]}));}
  }
  check(std::uint32_t(regs.rip)==stop,"Original pose stop not reached");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
  J banks=J::array();for(const auto&r:m.at("regions"))banks.push_back(J{{"address",r.at("address")},{"after",hex(memory(child.pid,r.at("address"),r.at("bytes")))}});
  J stack=J::array();const auto stack_bytes=memory(child.pid,std::uint32_t(regs.rsp),40);for(unsigned n=0;n<10;++n){std::uint32_t value{};std::memcpy(&value,stack_bytes.data()+n*4,4);stack.push_back(value);}
  J packet=nullptr;if(m.contains("boundary_packet_stack_slot")){const auto p=stack.at(m.at("boundary_packet_stack_slot").get<unsigned>()).get<std::uint32_t>();packet=hex(memory(child.pid,p,16));}
  std::ofstream output(argv[3]);output<<J{{"service_return",service},{"boundary_gp",gp(regs)},{"boundary_stack",stack},{"boundary_packet",packet},{"original_instructions",count},{"after_fp",J::array({fp.cwd,fp.swd,fp.mxcsr})},{"writes",writes},{"reads",reads},{"rsqrt_estimates",estimates},{"banks",banks}}.dump()<<'\n';check(bool(output),"Original pose observation output failed");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
