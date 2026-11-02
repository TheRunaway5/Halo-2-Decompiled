// Independent original304350 caller→2F36F0/query/resolver study. Declares only
// its ordinary ECX/four formals and scalar FP over authenticated BEFORE DATA.
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <array>
#include <algorithm>
#include <span>
#include <vector>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <iostream>
namespace {
using J=nlohmann::json;
void require(bool value,const char*why){if(!value)throw std::runtime_error(why);}
long request(__ptrace_request op,pid_t pid,void*a=nullptr,void*b=nullptr){errno=0;const auto n=ptrace(op,pid,a,b);if(n==-1&&errno)throw std::runtime_error(strerror(errno));return n;}
struct Child {
 pid_t pid=-1;int status{};
 ~Child(){if(pid>0){kill(pid,SIGKILL);waitpid(pid,&status,0);}}
 void wait(){require(waitpid(pid,&status,0)==pid&&WIFSTOPPED(status),"Original child exited before its boundary");}
 user_regs_struct regs(){user_regs_struct r{};request(PTRACE_GETREGS,pid,nullptr,&r);return r;}
 user_fpregs_struct fp(){user_fpregs_struct f{};request(PTRACE_GETFPREGS,pid,nullptr,&f);return f;}
 void debug(unsigned n,unsigned long value){request(PTRACE_POKEUSER,pid,reinterpret_cast<void*>(offsetof(user,u_debugreg)+n*sizeof(unsigned long)),reinterpret_cast<void*>(value));}
 void start(const char*exe){pid=fork();require(pid>=0,"Cannot create original entry process");if(!pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(exe,exe,nullptr);_exit(127);}wait();request(PTRACE_SETOPTIONS,pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,pid);wait();require(WSTOPSIG(status)==SIGTRAP,"Original FS/entry scaffold failed");}
 std::uint32_t go(std::initializer_list<std::uint32_t>targets){require(targets.size()<=4,"Too many hardware entry boundaries");unsigned index{},control{};for(auto p:targets){debug(index,p);control|=1u<<(2*index++);}debug(6,0);debug(7,control);request(PTRACE_CONT,pid);wait();const auto r=regs();debug(7,0);if(WSTOPSIG(status)!=SIGTRAP||std::find(targets.begin(),targets.end(),std::uint32_t(r.rip))==targets.end())throw std::runtime_error("Original caller stopped before declared child: pc="+std::to_string(r.rip)+" signal="+std::to_string(WSTOPSIG(status)));return r.rip;}
 std::vector<std::byte>read(std::uint32_t p,std::size_t n){std::vector<std::byte>b(n);iovec l{b.data(),n},r{reinterpret_cast<void*>(std::uintptr_t(p)),n};require(process_vm_readv(pid,&l,1,&r,1,0)==std::ptrdiff_t(n),"Cannot read original typed DATA");return b;}
 void write(std::uint32_t p,std::span<const std::byte>b){iovec l{const_cast<std::byte*>(b.data()),b.size()},r{reinterpret_cast<void*>(std::uintptr_t(p)),b.size()};require(process_vm_writev(pid,&l,1,&r,1,0)==std::ptrdiff_t(b.size()),"Cannot initialize declared original formals");}
 std::uint32_t word(std::uint32_t p,unsigned n=4){const auto b=read(p,n);std::uint32_t value{};for(unsigned i=0;i<n;++i)value|=std::to_integer<unsigned>(b[i])<<(8*i);return value;}
 void put(std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));write(p,b);}
};
J fp(Child&c){auto f=c.fp();return J::array({f.cwd,f.swd,f.mxcsr});}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string out;for(auto byte:b){auto n=std::to_integer<unsigned>(byte);out+=d[n>>4];out+=d[n&15];}return out;}
J shape(Child&c,std::uint32_t p){const auto table=c.word(p);return J{{"shape",p},{"table",table},{"class_getter",c.word(table+0x14)},{"query_owner",c.word(table+0x30)},{"resolve_owner",c.word(table+0x34)}};}
}
int main(int argc,char**argv){J report{{"passed",false},{"native_qualification",false},{"instruction_patches",J::array()},{"captured_gp_used",false},{"physics_outcomes_supplied",false}};try{
 require(argc==4,"Supply original ELF, typed semantic input and report");std::ifstream stream(argv[2]);const auto input=J::parse(stream);J profiles=J::array();
 for(unsigned mode=0;mode<4;++mode){Child c;c.start(argv[1]);constexpr std::uint32_t sp=0x7200f000,stop=0x70000080;const std::array<std::uint32_t,4>arguments{input.at("first"),input.at("second"),input.at("configuration"),input.at("output")};c.put(sp,stop);for(unsigned i=0;i<4;++i)c.put(sp+4+4*i,arguments[i]);auto r=c.regs();r.rax=r.rbx=r.rdx=r.rsi=r.rdi=r.rbp=0;r.rcx=input.at("agent");r.rsp=sp;r.rip=0x304350;r.eflags&=~0x400u;request(PTRACE_SETREGS,c.pid,nullptr,&r);auto f=c.fp();f.cwd=input.at("floating_point").at("x87_control_word");f.swd=input.at("floating_point").at("x87_status");f.mxcsr=input.at("floating_point").at("mxcsr");if(mode){f.cwd=(f.cwd&~0xc00u)|(mode<<10);f.mxcsr=(f.mxcsr&~0x6000u)|(mode<<13);}f.ftw=0;std::memset(f.st_space,0,sizeof(f.st_space));request(PTRACE_SETFPREGS,c.pid,nullptr,&f);J row{{"mode",mode},{"declared_before_fp",fp(c)},{"profiling_cursor",c.word(0x48c974)},{"profiling_limit",c.word(0x48c978)}};
 require(c.go({0x2f36f0,stop})==0x2f36f0,"Original producer did not call candidate owner");r=c.regs();const auto candidate_sp=std::uint32_t(r.rsp);const auto first=c.word(candidate_sp+4),second=c.word(candidate_sp+8),context=c.word(candidate_sp+12),factory=c.word(candidate_sp+16);require(r.rcx==input.at("agent")&&first==input.at("first")&&context==input.at("configuration")&&factory==0x2f3140u,"Genuine source candidate arguments differ");row["candidate_entry"]={{"producer",std::uint32_t(r.rcx)},{"first",first},{"second",second},{"context",context},{"factory",factory},{"floating_point",fp(c)},{"second_payload_hex12",hex(c.read(second,12))}};row["query_shape_before"]=shape(c,c.word(second));row["resolver_collection_before"]=shape(c,c.word(c.word(second)+12));
 auto boundary=c.go({0x1f6d10,0x3043e0});row["first_boundary"]=boundary;
 if(boundary==0x1f6d10u){r=c.regs();const auto bounds=c.word(r.rsp+4),keys=c.word(r.rsp+8);row["query_entry"]={{"shape",std::uint32_t(r.rcx)},{"bounds",bounds},{"bounds_hex32",hex(c.read(bounds,32))},{"keys",keys},{"header_hex12",hex(c.read(keys,12))},{"floating_point",fp(c)}};boundary=c.go({0x1f7c10,0x3043e0});row["after_query_boundary"]=boundary;row["source_key_count"]=c.word(keys+4);
  if(boundary==0x1f7c10u){r=c.regs();const auto return_address=c.word(r.rsp),key=c.word(r.rsp+4),output=c.word(r.rsp+8);row["resolver_entry"]={{"key",key},{"output",output},{"return_address",return_address},{"floating_point",fp(c)}};c.go({return_address});r=c.regs();row["resolver_return"]={{"shape",std::uint32_t(r.rax)},{"output_hex",hex(c.read(output,80))},{"shape_dispatch",shape(c,r.rax)},{"floating_point",fp(c)}};boundary=c.go({0x2f3140,0x3043e0});row["after_resolver_boundary"]=boundary;
   if(boundary==0x2f3140u){r=c.regs();const auto second_payload=c.word(r.rsp+8),dispatch=c.word(c.word(r.rsp+12));row["factory_entry"]={{"first",c.word(r.rsp+4)},{"second",second_payload},{"second_shape",c.word(second_payload)},{"second_class_getter",c.word(c.word(c.word(second_payload))+0x14)},{"dispatch",dispatch},{"floating_point",fp(c)}};
    const auto caller_return=c.word(r.rsp),first_payload=c.word(r.rsp+4),first_owner=c.word(c.word(c.word(first_payload))+0x14),second_owner=c.word(c.word(c.word(second_payload))+0x14);
    c.go({first_owner});r=c.regs();auto leaf_return=c.word(r.rsp);c.go({leaf_return});const auto first_class=std::uint32_t(c.regs().rax);
    c.go({second_owner});r=c.regs();leaf_return=c.word(r.rsp);c.go({leaf_return});const auto second_class=std::uint32_t(c.regs().rax);row["actual_class_returns"]={first_class,second_class};
    const auto selected=c.word(dispatch+0x18cu+((first_class<<5u)+second_class)*4u);row["actual_selected_factory"]=selected;c.go({selected});r=c.regs();row["selected_factory_formals"]={c.word(r.rsp+4),c.word(r.rsp+8),c.word(r.rsp+12),c.word(r.rsp+16)};c.go({caller_return});r=c.regs();const auto created=std::uint32_t(r.rax),table=c.word(created);row["created_agent"]={{"identity",created},{"table",table},{"process_owner",c.word(table+0x10)},{"release_owner",c.word(table+0x14)},{"query_owner",c.word(table+0x20)},{"floating_point",fp(c)}};
}
  }
 }
 profiles.push_back(row);}
 report.update(J{{"passed",true},{"declared_source_caller_abi","304350 ECX=typed agent, four ordinary stack formals; empty x87 plus actual FP0/independent rounding1..3"},{"genuine_source_boundaries",profiles},{"no_gp_or_expected_values_feed_native_kernel",true}});
}catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}std::ofstream output(argv[3]);output<<report.dump(2)<<'\n';return report.at("passed").get<bool>()?0:1;}
