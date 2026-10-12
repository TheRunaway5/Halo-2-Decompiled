// Test-only Linux ptrace observer for the unmodified paired i386 instructions.
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
using J=nlohmann::json;
void check(bool x,const char*s){if(!x)throw std::runtime_error(s);}
long request(__ptrace_request op,pid_t pid,void*a=nullptr,void*b=nullptr){errno=0;const auto r=ptrace(op,pid,a,b);if(r==-1&&errno)throw std::runtime_error(std::string("ptrace: ")+strerror(errno));return r;}
std::string read_hex(pid_t pid,std::uint32_t p,unsigned n){static constexpr char digits[]="0123456789abcdef";std::string result;for(unsigned k=0;k<n;){const auto address=std::uintptr_t(p+k)&~7ul;const auto word=std::uint64_t(request(PTRACE_PEEKDATA,pid,reinterpret_cast<void*>(address)));for(unsigned j=(p+k)&7u;j<8&&k<n;++j,++k){const auto b=(word>>(j*8))&255;result+=digits[b>>4];result+=digits[b&15];}}return result;}
std::uint32_t read_word(pid_t pid,std::uint32_t p){const auto h=read_hex(pid,p,4);std::uint32_t x=0;for(unsigned k=0;k<4;++k)x|=std::stoul(h.substr(k*2,2),nullptr,16)<<(k*8);return x;}
user_regs_struct regs(pid_t pid){user_regs_struct r{};request(PTRACE_GETREGS,pid,nullptr,&r);return r;}
std::uint32_t gp(const user_regs_struct&r,int n){switch(n){case 0:return r.rax;case 1:return r.rcx;case 2:return r.rdx;case 3:return r.rbx;case 4:return r.rsp;case 5:return r.rbp;case 6:return r.rsi;case 7:return r.rdi;default:return 0;}}
struct Write {std::int32_t base,index,scale;std::uint32_t displacement,width;};
struct Instruction {std::uint32_t canonical;bool repeated;std::vector<Write>writes;};
int main(int argc,char**argv){try{
 check(argc==6,"Supply ELF, input, output, metadata and trace paths");
 std::ifstream m(argv[4]);const auto metadata=J::parse(m);std::map<std::uint32_t,Instruction>instructions;
 for(const auto&row:metadata.at("instructions")){Instruction i{row.at("canonical"),row.at("repeated"),{}};for(const auto&w:row.at("writes"))i.writes.push_back({w.at("base"),w.at("index"),w.at("scale"),w.at("displacement"),w.at("width")});check(instructions.emplace(row.at("actual").get<std::uint32_t>(),i).second,"Duplicate hardware site");}
 const auto input=metadata.at("input_address").get<std::uint32_t>();const auto child=fork();check(child>=0,"fork failed");
 if(!child){const auto in=open(argv[2],O_RDONLY),out=open(argv[3],O_CREAT|O_TRUNC|O_WRONLY,0600);if(in<0||out<0||dup2(in,0)<0||dup2(out,1)<0)_exit(125);close(in);close(out);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
 int status=0;check(waitpid(child,&status,0)==child&&WIFSTOPPED(status),"Missing initial hardware stop");J results=J::object();std::size_t count=0;
 request(PTRACE_SETOPTIONS,child,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
 auto wait=[&](){check(waitpid(child,&status,0)==child,"Hardware wait failed");if(WIFSTOPPED(status))check(WSTOPSIG(status)==SIGTRAP,"Original hardware execution fault");else check(WIFEXITED(status)&&WEXITSTATUS(status)==0,"Hardware harness failed");};
 request(PTRACE_CONT,child);wait();
 while(WIFSTOPPED(status)){
  const auto id=read_word(child,input);const auto key=std::to_string(id);if(!results.contains(key))results[key]=J{{"id",id},{"instructions",J::array()},{"writes",J::array()}};auto &result=results[key];
  // The bridge INT3 precedes only its own entry jump, outside original code.
  request(PTRACE_SINGLESTEP,child);wait();auto r=regs(child);
  while(instructions.contains(std::uint32_t(r.rip))){
   const auto&i=instructions.at(std::uint32_t(r.rip));result["instructions"].push_back(i.canonical);J stores=J::array();
   if(!i.repeated||std::uint32_t(r.rcx))for(const auto&w:i.writes){
    const auto address=gp(r,w.base)+gp(r,w.index)*w.scale+w.displacement;
    for(unsigned off=0;off<w.width;){const auto size=std::min(8u,w.width-off);stores.push_back(J{{"instruction",i.canonical},{"address",address+off},{"before",read_hex(child,address+off,size)},{"size",size}});off+=size;}
   }
   request(PTRACE_SINGLESTEP,child);wait();check(WIFSTOPPED(status),"Original escaped without capture");
   for(auto&s:stores){s["written"]=read_hex(child,s.at("address"),s.at("size"));s.erase("size");result["writes"].push_back(std::move(s));}
   const auto previous=std::uint32_t(r.rip);const auto repeated_count=std::uint32_t(r.rcx);
   r=regs(child);++count;
   // Match the runtime observer's entry/iteration/completion convention.
   // Architectural SINGLESTEP executes REP once per iteration instead.
   if(i.repeated&&repeated_count&&std::uint32_t(r.rip)!=previous)result["instructions"].push_back(i.canonical);
  }
  const auto row=metadata.at("setups").find(key);std::vector<J>setup;
  if(std::uint32_t(r.rip)==0x100f000u&&(r.rax&255u)&&row!=metadata.at("setups").end()&&result.value("stages",0)==0){
   for(const auto&s:*row)setup.push_back(J{{"instruction",0},{"address",s.at("address")},{"before",read_hex(child,s.at("address"),4)},{"written",s.at("written")}});
  }
  result["stages"]=result.value("stages",0)+1;
  request(PTRACE_CONT,child);wait();
  if(!setup.empty()){
   check(WIFSTOPPED(status)&&read_word(child,input)==id,"Missing second original call");for(auto&s:setup){check(read_hex(child,s.at("address"),4)==s.at("written").get<std::string>(),"Hardware call setup differs");result["writes"].push_back(std::move(s));}
  }
 }
 J output=J::array();for(auto&[key,value]:results.items())output.push_back(std::move(value));std::ofstream out(argv[5]);out<<J{{"cases",output},{"single_stepped_original_instructions",count}}.dump()<<'\n';std::cout<<"Hardware original instructions "<<count<<" cases "<<output.size()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
