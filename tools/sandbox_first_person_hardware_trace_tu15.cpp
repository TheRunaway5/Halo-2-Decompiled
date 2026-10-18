// Test-only single-step observer. A declared component boundary redirects the
// stopped CPU to capture before that instruction; original bytes stay intact.
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
using J=nlohmann::json;
void check(bool x,const char*s){if(!x)throw std::runtime_error(s);}
long request(__ptrace_request op,pid_t pid,void*a=nullptr,void*b=nullptr){errno=0;const auto r=ptrace(op,pid,a,b);if(r==-1&&errno)throw std::runtime_error(std::string("ptrace: ")+strerror(errno));return r;}
std::string read_hex(pid_t pid,std::uint32_t p,unsigned n){static constexpr char d[]="0123456789abcdef";std::string out;for(unsigned k=0;k<n;){const auto at=std::uintptr_t(p+k)&~7ul;const auto word=std::uint64_t(request(PTRACE_PEEKDATA,pid,reinterpret_cast<void*>(at)));for(unsigned j=(p+k)&7u;j<8&&k<n;++j,++k){const auto b=(word>>(j*8))&255;out+=d[b>>4];out+=d[b&15];}}return out;}
std::uint32_t word(pid_t pid,std::uint32_t p){const auto h=read_hex(pid,p,4);std::uint32_t x=0;for(unsigned n=0;n<4;++n)x|=std::stoul(h.substr(n*2,2),nullptr,16)<<(n*8);return x;}
void put_word(pid_t pid,std::uint32_t p,std::uint32_t value){const auto at=std::uintptr_t(p)&~7ul;const auto shift=(p&7u)*8u;check((p&7u)<=4,"Unaligned capture cell");auto old=std::uint64_t(request(PTRACE_PEEKDATA,pid,reinterpret_cast<void*>(at)));old=(old&~(0xffffffffull<<shift))|(std::uint64_t(value)<<shift);request(PTRACE_POKEDATA,pid,reinterpret_cast<void*>(at),reinterpret_cast<void*>(old));}
user_regs_struct regs(pid_t pid){user_regs_struct r{};request(PTRACE_GETREGS,pid,nullptr,&r);return r;}
std::uint32_t gp(const user_regs_struct&r,int n){switch(n){case 0:return r.rax;case 1:return r.rcx;case 2:return r.rdx;case 3:return r.rbx;case 4:return r.rsp;case 5:return r.rbp;case 6:return r.rsi;case 7:return r.rdi;default:return 0;}}
struct Write{int base,index,scale;std::uint32_t displacement,width;bool fs;};
Write memory_operand(const J&w){return {w.at("base"),w.at("index"),w.at("scale"),w.at("displacement"),w.at("width"),w.at("fs")};}
std::uint32_t location(const Write&w,const user_regs_struct&r,std::uint32_t fs_base){return gp(r,w.base)+gp(r,w.index)*w.scale+w.displacement+(w.fs?fs_base:0u);}
std::uint32_t xmm_low(pid_t pid,unsigned index){check(index<8,"Invalid source XMM index");user_fpregs_struct fp{};request(PTRACE_GETFPREGS,pid,nullptr,&fp);return fp.xmm_space[index*4];}
struct Instruction{std::uint32_t canonical;bool repeated;std::vector<Write>writes,reads;J estimate;bool timestamp,packet;};
int main(int argc,char**argv){try{
 check(argc==6,"Supply ELF, input, output, metadata and trace paths");std::ifstream in(argv[4]);const auto metadata=J::parse(in);std::map<std::uint32_t,Instruction>instructions;
 for(const auto&row:metadata.at("instructions")){Instruction i{row.at("canonical"),row.at("repeated"),{},{},row.at("estimate"),row.at("timestamp"),row.at("packet")};for(const auto&w:row.at("writes"))i.writes.push_back(memory_operand(w));for(const auto&w:row.at("reads"))i.reads.push_back(memory_operand(w));check(instructions.emplace(row.at("actual").get<std::uint32_t>(),i).second,"Duplicate original site");}
 const auto input=metadata.at("input_address").get<std::uint32_t>();const auto capture=metadata.at("capture_address").get<std::uint32_t>();const auto done=metadata.at("done_address").get<std::uint32_t>();
 const auto child=fork();check(child>=0,"fork failed");if(!child){const auto fd=open(argv[2],O_RDONLY),out=open(argv[3],O_CREAT|O_TRUNC|O_WRONLY,0600);if(fd<0||out<0||dup2(fd,0)<0||dup2(out,1)<0)_exit(125);close(fd);close(out);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
 int status{};check(waitpid(child,&status,0)==child&&WIFSTOPPED(status),"Missing initial stop");request(PTRACE_SETOPTIONS,child,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
 bool allow_read_fault=false;
 auto wait=[&](){check(waitpid(child,&status,0)==child,"Hardware wait failed");if(WIFSTOPPED(status))check(WSTOPSIG(status)==SIGTRAP||(allow_read_fault&&WSTOPSIG(status)==SIGSEGV),"Original hardware execution fault");else check(WIFEXITED(status)&&WEXITSTATUS(status)==0,"Hardware harness failed");};
 J results=J::array();std::size_t count=0;request(PTRACE_CONT,child);wait();
 while(WIFSTOPPED(status)){
  const auto id=word(child,input);const auto&limits=metadata.at("cases").at(std::to_string(id));const auto boundary=limits.at("boundary").get<std::uint32_t>();const auto return_address=limits.at("return_address").get<std::uint32_t>();const auto fs_base=limits.at("fs_base").get<std::uint32_t>();
  std::map<std::uint32_t,std::uint64_t>retained;for(const auto&range:limits.at("retained_ranges")){const auto p=range.at(0).get<std::uint32_t>();retained.emplace(p,std::uint64_t(p)+range.at(1).get<unsigned>());}
  const auto fault=limits.value("unmapped_read_fault",J{});
  bool read_fault_observed=false;
  auto require_retained=[&](std::uint32_t p,unsigned width,std::uint32_t site){const auto end=std::uint64_t(p)+width;auto it=retained.upper_bound(p);if(it==retained.begin()||std::prev(it)->second<end){std::ostringstream why;why<<"Original hardware data access outside retained input at "<<std::hex<<site<<" address "<<p<<" width "<<width;throw std::runtime_error(why.str());}};
  J row{{"id",id},{"instructions",J::array()},{"writes",J::array()},{"estimates",J::array()},{"timestamps",J::array()},{"packets",J::array()}};request(PTRACE_SINGLESTEP,child);wait();check(WIFSTOPPED(status),"Missing entry jump stop");auto r=regs(child);
  while((std::uint32_t(r.rip)!=boundary&&std::uint32_t(r.rip)!=return_address)||
        (!fault.is_null()&&!read_fault_observed&&std::uint32_t(r.rip)==fault.at("instruction").get<std::uint32_t>())){
   const auto it=instructions.find(std::uint32_t(r.rip));check(it!=instructions.end(),"Original reached an undeclared owner");const auto&i=it->second;row["instructions"].push_back(i.canonical);J stores=J::array();
   if(!i.repeated||std::uint32_t(r.rcx)){
    for(const auto&w:i.reads){
     const auto p=location(w,r,fs_base);
     if(!fault.is_null()&&i.canonical==fault.at("instruction").get<std::uint32_t>()&&p==4&&w.width==4){
      check(fault.at("address")==4&&fault.at("width")==4,"Unsupported declared read fault");
      check(retained.upper_bound(p)==retained.begin(),"Fault address is retained");
     }else require_retained(p,w.width,i.canonical);
    }
    for(const auto&w:i.writes){const auto p=location(w,r,fs_base);require_retained(p,w.width,i.canonical);for(unsigned off=0;off<w.width;){const auto n=std::min(8u,w.width-off);stores.push_back(J{{"instruction",i.canonical},{"address",p+off},{"before",read_hex(child,p+off,n)},{"size",n}});off+=n;}}
   }
   J estimate_input;
   if(!i.estimate.is_null()){
    const auto lanes=i.estimate.value("lanes",1u);check(lanes==1||lanes==4,"Invalid estimate width");
    user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child,nullptr,&fp);J values=J::array();
    for(unsigned lane=0;lane<lanes;++lane){
     if(i.estimate.contains("source_xmm")){const auto index=i.estimate.at("source_xmm").get<unsigned>();check(index<8,"Invalid source XMM");values.push_back(fp.xmm_space[index*4+lane]);}
     else{const auto p=location(memory_operand(i.estimate.at("source_memory")),r,fs_base)+lane*4;require_retained(p,4,i.canonical);values.push_back(word(child,p));}
    }
    estimate_input=lanes==1?values[0]:values;
   }
   if(i.packet){require_retained(std::uint32_t(r.rsp)+4,4,i.canonical);const auto p=word(child,std::uint32_t(r.rsp)+4);require_retained(p,128,i.canonical);row["packets"].push_back(J{{"input",p},{"destination",std::uint32_t(r.rcx)},{"input_hex",read_hex(child,p,128)}});}
   const auto old=std::uint32_t(r.rip),repetitions=std::uint32_t(r.rcx);
   allow_read_fault=!fault.is_null()&&i.canonical==fault.at("instruction").get<std::uint32_t>();
   request(PTRACE_SINGLESTEP,child);wait();check(WIFSTOPPED(status),"Original escaped capture");
   if(WSTOPSIG(status)==SIGSEGV){
    check(allow_read_fault&&stores.empty(),"Unexpected original hardware fault");
    r=regs(child);siginfo_t info{};request(PTRACE_GETSIGINFO,child,nullptr,&info);
    check(std::uint32_t(r.rip)==old&&reinterpret_cast<std::uintptr_t>(info.si_addr)==4&&info.si_code==SEGV_MAPERR,"Declared read did not raise its actual unmapped-page fault");
    row["read_fault"]=J{{"instruction",old},{"address",4},{"signal",SIGSEGV},{"code",info.si_code},{"instruction_retired",false}};
    allow_read_fault=false;read_fault_observed=true;++count;break;
   }
   check(!allow_read_fault,"Declared unmapped read unexpectedly succeeded");
   allow_read_fault=false;
   for(auto&s:stores){s["written"]=read_hex(child,s.at("address"),s.at("size"));s.erase("size");row["writes"].push_back(std::move(s));}r=regs(child);++count;
   if(!i.estimate.is_null()){
    const auto lanes=i.estimate.value("lanes",1u),index=i.estimate.at("destination").get<unsigned>();check(index<8,"Invalid destination XMM");
    user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child,nullptr,&fp);J values=J::array();for(unsigned lane=0;lane<lanes;++lane)values.push_back(fp.xmm_space[index*4+lane]);
    row["estimates"].push_back(J{{"instruction",i.canonical},{"input",estimate_input},{"output",lanes==1?values[0]:values}});
   }
   if(i.timestamp)row["timestamps"].push_back(J{{"instruction",i.canonical},{"output",(std::uint64_t(std::uint32_t(r.rdx))<<32)|std::uint32_t(r.rax)}});
   if(i.repeated&&repetitions&&std::uint32_t(r.rip)!=old)row["instructions"].push_back(i.canonical);
   check(count<10000000,"Original single-step bound exceeded");
  }
  check(fault.is_null()||read_fault_observed,"Declared unmapped read fault was not observed");
  row["exit_entry"]=std::uint32_t(r.rip);put_word(child,done,std::uint32_t(r.rip));r.rip=capture;request(PTRACE_SETREGS,child,nullptr,&r);results.push_back(std::move(row));request(PTRACE_CONT,child);wait();
 }
 std::ofstream out(argv[5]);out<<J{{"cases",results},{"single_stepped_original_instructions",count}}.dump()<<'\n';std::cout<<"Hardware original instructions "<<count<<" cases "<<results.size()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
