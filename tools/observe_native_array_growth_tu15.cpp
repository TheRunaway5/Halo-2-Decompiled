// Offline physical original-code observer. Single steps the unchanged i386
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
  Child child;child.pid=fork();check(child.pid>=0,"Cannot create original array process");if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
  int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Original array wait failed");};wait();check(WIFSTOPPED(status),"Missing original exec stop");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original array input setup failed");
  J before=J::array();for(const auto&r:m.at("regions"))before.push_back(J{{"address",r.at("address")},{"before",hex(memory(child.pid,r.at("address"),r.at("bytes")))}});
  request(PTRACE_SINGLESTEP,child.pid);wait();
  J writes=J::array(),reads=J::array(),estimates=J::array(),allocator_calls=J::array();user_regs_struct regs{};unsigned count=0,fault_count=0;bool declared_fault=false;std::set<unsigned> applied;std::set<std::uint32_t> executed;
  for(;count<100000;++count){
    check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original array execution fault");request(PTRACE_GETREGS,child.pid,nullptr,&regs);const auto pc=std::uint32_t(regs.rip);if(pc==stop)break;
    const auto site=sites.find(pc);check(site!=sites.end(),"Original array escaped authenticated complete bodies");executed.insert(pc);const auto registers=gp(regs);J pending=J::array();
    if(pc==0x149da0u||pc==0x233250u||pc==0x149e00u||pc==0x233420u){
      const bool releasing=pc==0x149e00u||pc==0x233420u;
      const auto argument=[&](unsigned n){const auto raw=memory(child.pid,registers[4]+4u+n*4u,4);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(raw[i])<<(i*8u);return value;};
      J call{{"owner",pc},{"receiver",registers[1]},{"bytes",argument(releasing?1:0)},{"memory_class",argument(releasing?2:1)}};if(releasing)call["data"]=argument(0);allocator_calls.push_back(std::move(call));
    }
    const bool repeated_empty=(pc==0x2e2e43u||pc==0x2e2e4au)&&registers[1]==0;
    if(m.contains("fault")&&!repeated_empty){for(const auto&a:site->second.at("operands")){
      const auto&f=m.at("fault");const auto p=operand(a,registers),n=a.at("bytes").get<std::uint32_t>();
      if(p==f.at("address")&&n==f.at("bytes")&&a.at(f.value("write",false)?"write":"read").get<bool>()&&++fault_count==f.at("occurrence")){declared_fault=true;break;}
    }}
    if(declared_fault)break;
    if(!repeated_empty)for(const auto&a:site->second.at("operands")){
      const auto p=operand(a,registers),n=a.at("bytes").get<std::uint32_t>();if(!retained(p,n))continue;
      if(a.at("read").get<bool>())reads.push_back(J{{"instruction",pc},{"address",p},{"bytes",n}});
      if(a.at("write").get<bool>())pending.push_back(J{{"instruction",pc},{"address",p},{"before",hex(memory(child.pid,p,n))},{"bytes",n}});
    }
    request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original array instruction fault");
    for(auto&a:pending){a["written"]=hex(memory(child.pid,a.at("address"),a.at("bytes")));a.erase("bytes");writes.push_back(a);}

    unsigned mutation_index=0;for(const auto&mutation:m.value("mutations",J::array())){
      if(pc==mutation.at("instruction")&&!pending.empty()&&applied.insert(mutation_index).second){
        const auto p=mutation.at("address").get<std::uint32_t>();const auto raw=mutation.at("written").get<std::string>();
        for(unsigned off=0;off<raw.size()/2;){const auto at=std::uintptr_t(p+off)&~7ull;auto word=std::uint64_t(request(PTRACE_PEEKDATA,child.pid,reinterpret_cast<void*>(at)));
          for(unsigned lane=(p+off)&7u;lane<8&&off<raw.size()/2;++lane,++off)word=(word&~(255ull<<(lane*8u)))|(std::stoull(raw.substr(off*2,2),nullptr,16)<<(lane*8u));
          request(PTRACE_POKEDATA,child.pid,reinterpret_cast<void*>(at),reinterpret_cast<void*>(word));
        }
      }
      ++mutation_index;
    }

  }
  check(declared_fault||std::uint32_t(regs.rip)==stop,"Original array stop not reached");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);
  J banks=J::array();for(const auto&r:m.at("regions"))banks.push_back(J{{"address",r.at("address")},{"after",hex(memory(child.pid,r.at("address"),r.at("bytes")))}});
  std::ofstream output(argv[3]);output<<J{{"initial_banks",before},{"declared_fault",declared_fault},{"return_word",std::uint32_t(regs.rax)},{"mutations_applied",applied.size()},{"executed_sites",executed},{"original_instructions",count},{"after_fp",J::array({fp.cwd,fp.swd,fp.mxcsr})},{"writes",writes},{"reads",reads},{"allocator_calls",allocator_calls},{"banks",banks}}.dump()<<'\n';check(bool(output),"Original array observation output failed");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
