// Test-only lift delivery extension of vehicle_motion_math_hardware_trace_20261011.cpp.
// It observes the unchanged i386 bridge, original CALL/RET, and child DATA operands.
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
struct Instruction {std::uint32_t canonical;bool repeated;std::vector<Write>writes,reads;};
std::uint32_t address(const user_regs_struct& r,const Write& operand){
 return gp(r,operand.base)+gp(r,operand.index)*operand.scale+operand.displacement;
}
J registers(const user_regs_struct& r){
 J out=J::array();for(int i=0;i<8;++i)out.push_back(gp(r,i));return out;
}
struct Delivery {std::uint32_t callsite,entry,continuation,store_site;bool angular;};
constexpr Delivery deliveries[]={{0x1db370,0x1d92c0,0x1db375,0x3239f7,false},
                                 {0x1db64f,0x1d9100,0x1db654,0x323a07,true}};
int main(int argc,char**argv){try{
 check(argc==6,"Supply ELF, input, output, metadata and trace paths");
 std::ifstream m(argv[4]);const auto metadata=J::parse(m);std::map<std::uint32_t,Instruction>instructions;
 for(const auto&row:metadata.at("instructions")){Instruction i{row.at("canonical"),row.at("repeated"),{}, {}};for(const auto&w:row.at("writes"))i.writes.push_back({w.at("base"),w.at("index"),w.at("scale"),w.at("displacement"),w.at("width")});for(const auto&w:row.at("reads"))i.reads.push_back({w.at("base"),w.at("index"),w.at("scale"),w.at("displacement"),w.at("width")});check(instructions.emplace(row.at("actual").get<std::uint32_t>(),i).second,"Duplicate hardware site");}
 const auto input=metadata.at("input_address").get<std::uint32_t>();const auto child=fork();check(child>=0,"fork failed");
 if(!child){const auto in=open(argv[2],O_RDONLY),out=open(argv[3],O_CREAT|O_TRUNC|O_WRONLY,0600);if(in<0||out<0||dup2(in,0)<0||dup2(out,1)<0)_exit(125);close(in);close(out);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
 int status=0;check(waitpid(child,&status,0)==child&&WIFSTOPPED(status),"Missing initial hardware stop");J results=J::object();std::size_t count=0;
 request(PTRACE_SETOPTIONS,child,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
 auto wait=[&](){check(waitpid(child,&status,0)==child,"Hardware wait failed");if(WIFSTOPPED(status))check(WSTOPSIG(status)==SIGTRAP,"Original hardware execution fault");else check(WIFEXITED(status)&&WEXITSTATUS(status)==0,"Hardware harness failed");};
 request(PTRACE_CONT,child);wait();
 while(WIFSTOPPED(status)){
  const auto id=read_word(child,input);const auto key=std::to_string(id);if(!results.contains(key))results[key]=J{{"id",id},{"instructions",J::array()},{"writes",J::array()},{"velocity_deliveries",J::array()}};auto &result=results[key];
  // The bridge INT3 precedes only its own entry jump, outside original code.
  request(PTRACE_SINGLESTEP,child);wait();auto r=regs(child);J active;
  while(instructions.contains(std::uint32_t(r.rip))){
   const auto actual=std::uint32_t(r.rip);
   const auto&i=instructions.at(actual);
   const auto step=result["instructions"].size();
   result["instructions"].push_back(i.canonical);J stores=J::array();
   if(!active.is_null()&&actual==active.at("continuation")){
    const auto entry_sp=active.at("caller_sp").get<std::uint32_t>();
    check(std::uint32_t(r.rsp)==entry_sp+(active.at("store_site")==0x323a07?4u:0u),"Velocity RET stack delta differs");
    active["return_step"]=step;active["return_sp"]=std::uint32_t(r.rsp);
    active["return_gp"]=registers(r);active["return_eflags"]=std::uint32_t(r.eflags);
    result["velocity_deliveries"].push_back(std::move(active));active=nullptr;
   }
   for(const auto& delivery:deliveries)if(actual==delivery.callsite){
    check(active.is_null(),"Nested lift velocity handoff differs");
    const auto sp=std::uint32_t(r.rsp),vector=gp(r,delivery.angular?1:2);
    active=J{{"callsite",delivery.callsite},{"entry",delivery.entry},{"continuation",delivery.continuation},
     {"store_site",delivery.store_site},{"call_step",step},{"caller_sp",sp},{"caller_gp",registers(r)},
     {"caller_eflags",std::uint32_t(r.eflags)},{"component",delivery.angular?read_word(child,sp):gp(r,6)},
     {"slot",gp(r,delivery.angular?0:1)},{"vector",vector},{"vector_hex",read_hex(child,vector,12)},
     {"reads",J::array()}};
   }
   if(!active.is_null()&&actual!=active.at("callsite")&&(!i.repeated||std::uint32_t(r.rcx))){
    for(const auto&operand:i.reads){
     const auto p=address(r,operand);
     active["reads"].push_back(J{{"step",step},{"instruction",actual},{"address",p},{"raw",read_hex(child,p,operand.width)}});
    }
   }
   if(!i.repeated||std::uint32_t(r.rcx))for(const auto&w:i.writes){
    const auto p=address(r,w);
    for(unsigned off=0;off<w.width;){const auto size=std::min(8u,w.width-off);stores.push_back(J{{"instruction",i.canonical},{"step",step},{"address",p+off},{"before",read_hex(child,p+off,size)},{"size",size}});off+=size;}
   }
   request(PTRACE_SINGLESTEP,child);wait();check(WIFSTOPPED(status),"Original escaped without capture");
   for(auto&s:stores){s["written"]=read_hex(child,s.at("address"),s.at("size"));s.erase("size");result["writes"].push_back(std::move(s));}
   const auto previous=std::uint32_t(r.rip);const auto repeated_count=std::uint32_t(r.rcx);
   r=regs(child);++count;
   if(!active.is_null()&&actual==active.at("callsite"))
    check(std::uint32_t(r.rip)==active.at("entry")&&std::uint32_t(r.rsp)+4u==active.at("caller_sp"),"Velocity CALL target or stack delta differs");
   // Match the runtime observer's entry/iteration/completion convention.
   // Architectural SINGLESTEP executes REP once per iteration instead.
   if(i.repeated&&repeated_count&&std::uint32_t(r.rip)!=previous)result["instructions"].push_back(i.canonical);
  }
  check(active.is_null(),"Original velocity delivery did not return to its caller");
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
