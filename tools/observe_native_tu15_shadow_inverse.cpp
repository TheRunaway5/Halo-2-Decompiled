// Offline unchanged complete C3DC0 caller/frame observer. Baselines never
// change executing state; separately declared liveness controls only perturb
// already-returned dead transport fields or one owed result as a red control.
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
namespace {
J fp_state(const user_fpregs_struct &fp) {
 J values=J::array();unsigned occupied{},top=(fp.swd>>11u)&7u;
 for(unsigned n=0;n<8;++n){values.push_back(hex(std::span(reinterpret_cast<const std::byte*>(fp.st_space)+n*16,10)));if(fp.ftw&(1u<<((top+n)&7u)))occupied|=1u<<n;}
 return J{{"fp",J::array({fp.cwd,fp.swd,fp.mxcsr})},{"x87",J{{"occupied",occupied},{"values",values}}}};
}
J xmm_state(const user_fpregs_struct &fp){J out=J::array();for(unsigned n=0;n<8;++n)out.push_back(J::array({fp.xmm_space[n*4],fp.xmm_space[n*4+1],fp.xmm_space[n*4+2],fp.xmm_space[n*4+3]}));return out;}
}
int main(int argc,char **argv)try {
 check(argc==5,"Supply original ELF, metadata, parent inputs, output");std::ifstream mi(argv[2]),ci(argv[3]);const auto meta=J::parse(mi),cases=J::parse(ci);
 std::map<std::uint32_t,J> sites;for(const auto &s:meta.at("sites"))check(sites.emplace(s.at("instruction"),s).second,"Duplicate original site");J result=J::array();
 for(const auto &c:cases) {
  Child child;child.pid=fork();check(child.pid>=0,"Cannot fork original shadow caller");if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(argv[1],argv[1],nullptr);_exit(127);}
  int status{};auto wait=[&]{check(waitpid(child.pid,&status,0)==child.pid,"Original wait failed");};wait();check(WIFSTOPPED(status),"Original exec missing");request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original bridge missing");
  for(const auto &b:c.at("banks"))memory(child.pid,b.at("address"),bytes(b.at("before")));
  user_regs_struct regs{};request(PTRACE_GETREGS,child.pid,nullptr,&regs);const auto initial=c.at("gp").get<std::array<std::uint32_t,8>>();regs.rax=initial[0];regs.rcx=initial[1];regs.rdx=initial[2];regs.rbx=initial[3];regs.rsp=initial[4];regs.rbp=initial[5];regs.rsi=initial[6];regs.rdi=initial[7];regs.rip=0xc3dc0;regs.eflags=0x202;request(PTRACE_SETREGS,child.pid,nullptr,&regs);
  user_fpregs_struct fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);fp.cwd=c.at("before_fp")[0];fp.swd=c.at("before_fp")[1];fp.mxcsr=c.at("before_fp")[2];fp.ftw=0;std::memset(fp.st_space,0,sizeof(fp.st_space));const unsigned top=(fp.swd>>11u)&7u;for(unsigned n=0;n<c.at("live_x87").size();++n){const auto raw=bytes(c.at("live_x87")[n]);std::memcpy(reinterpret_cast<std::byte*>(fp.st_space)+n*16,raw.data(),10);fp.ftw|=1u<<((top+n)&7u);}for(unsigned n=0;n<32;++n)fp.xmm_space[n]=0x6b000000u+c.at("id").get<unsigned>()*37u+n;request(PTRACE_SETFPREGS,child.pid,nullptr,&fp);
  const auto phase=c.value("fault",J::object()).value("phase",std::string{});const unsigned ordinal=c.value("fault",J::object()).value("ordinal",0u),wanted=c.value("wanted_leaf",~0u);const auto control=c.value("control",std::string("none"));
  J frames=J::array(),trace=J::array(),parent_writes=J::array(),boundary=nullptr,current;bool active{},completed{};unsigned frame_count{},reads{},writes{},steps{},normal{},early{},next_loop{};std::uint32_t record{},leaf_sp{};
  auto finish_frame=[&](bool done) {
   request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);current["completed"]=done;current["boundary"]=boundary;current["after_fp"]=fp_state(fp).at("fp");current["after_x87"]=fp_state(fp).at("x87");
   current["banks"][0]["after"]=hex(memory(child.pid,record,0x1bc));current["banks"][1]["after"]=hex(memory(child.pid,0x46a0d0,4));current["after_gp"]=gp(regs);current["after_xmm"]=xmm_state(fp);frames.push_back(current);active=false;
  };
  for(;steps<300000;++steps) {
   request(PTRACE_GETREGS,child.pid,nullptr,&regs);const auto pc=std::uint32_t(regs.rip);if(pc==meta.at("stop")){completed=true;break;}
   if(pc==0xc487e && active) {
    finish_frame(true);
    if(control!="none") {
     const auto before=current.at("gp").get<std::array<std::uint32_t,8>>();
     if(control=="owed_data") {auto raw=memory(child.pid,before[0],4);raw[0]^=std::byte{1};memory(child.pid,before[0],raw);}
     else {
      const bool taint=control=="taint";check(taint||control=="opaque","Unknown declared liveness control");regs.rax=taint?0xdead1001:before[0];regs.rcx=taint?0xdead2002:before[1];regs.rdx=taint?0xdead3003:before[2];regs.eflags=(regs.eflags&~0x8d5u)|(taint?0x895u:current.at("eflags").get<unsigned>()&0x8d5u)|2u;request(PTRACE_SETREGS,child.pid,nullptr,&regs);
      request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);for(unsigned n=0;n<6;++n)for(unsigned k=0;k<4;++k)fp.xmm_space[n*4+k]=taint?0x7f812345u+n*16+k:current.at("xmm")[n][k].get<std::uint32_t>();request(PTRACE_SETFPREGS,child.pid,nullptr,&fp);
     }
    }
   }
   if(pc==0x144090u) {
    check(!trace.empty()&&trace.back()==0xc4879u,"Inverse frame not reached through actual original CALL");check(!active,"Nested original inverse");request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);record=std::uint32_t(regs.rax)-4u;leaf_sp=std::uint32_t(regs.rsp);const auto ret=memory(child.pid,leaf_sp,4);check(hex(ret)=="7e480c00","Original CALL did not push genuine C487E");check(std::uint32_t(regs.rcx)==record+0x38u,"Original adjacent frame differs");
    current=J{{"frame_index",frame_count++},{"gp",gp(regs)},{"xmm",xmm_state(fp)},{"eflags",regs.eflags},{"before_fp",fp_state(fp).at("fp")},{"before_x87",fp_state(fp).at("x87")},{"record",record},{"real_return",hex(ret)},{"banks",J::array({J{{"address",record},{"before",hex(memory(child.pid,record,0x1bc))}},J{{"address",0x46a0d0},{"before",hex(memory(child.pid,0x46a0d0,4))}}})},{"reads",J::array()},{"writes",J::array()},{"accesses",J::array()}};active=true;reads=writes=0;
   }
   const auto it=sites.find(pc);check(it!=sites.end(),("Original shadow caller escaped authenticated graph at"+std::to_string(pc)).c_str());trace.push_back(pc);if(pc==0x166cafu)++normal;if(pc==0x16779eu)++early;if(pc==0xc3e1cu)++next_loop;
   J pending_reads=J::array(),pending_writes=J::array();bool stop{};auto operands=it->second.at("operands");
   if(active&&(pc==0x1441afu||pc==0x1441bau))operands.push_back(J{{"base",4},{"index",-1},{"scale",1},{"displacement",0},{"bytes",4},{"read",true},{"write",false}});
   for(const auto &o:operands) {
    const auto at=address(o,gp(regs)),n=o.at("bytes").get<unsigned>();const bool retained=active&&((at>=record&&std::uint64_t(at)+n<=std::uint64_t(record)+0x1bc)||(at==0x46a0d0&&n==4)||(at==leaf_sp&&n==4));
    if(retained&&o.at("read").get<bool>()) {if(current.at("frame_index")==wanted&&phase=="before_read"&&reads==ordinal){boundary=J{{"phase",phase},{"instruction",pc},{"address",at},{"bytes",n}};stop=true;break;}++reads;pending_reads.push_back(J{{"instruction",pc},{"address",at},{"bytes",n}});}
    if(retained&&o.at("write").get<bool>()) {if(current.at("frame_index")==wanted&&phase=="before_write"&&writes==ordinal){boundary=J{{"phase",phase},{"instruction",pc},{"address",at},{"bytes",n}};stop=true;break;}pending_writes.push_back(J{{"instruction",pc},{"address",at},{"before",hex(memory(child.pid,at,n))},{"bytes",n}});}
   }
   if(stop)break;
   request(PTRACE_SINGLESTEP,child.pid);wait();check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,("Unexpected original shadow signal "+std::to_string(WSTOPSIG(status))+" at"+std::to_string(pc)).c_str());
   if(active) {
    for(const auto &e:pending_reads){current["reads"].push_back(e);current["accesses"].push_back(J::array({"read",e.at("address"),e.at("bytes")}));}
    for(auto e:pending_writes){e["written"]=hex(memory(child.pid,e.at("address").get<std::uint32_t>(),e.at("bytes").get<std::size_t>()));e.erase("bytes");current["writes"].push_back(e);current["accesses"].push_back(J::array({"write",e.at("address"),e.at("written").get<std::string>().size()/2}));if(current.at("frame_index")==wanted&&phase=="after_write"&&writes==ordinal)boundary=J{{"phase",phase},{"instruction",pc},{"address",e.at("address")}};++writes;}
   }
   // Persistent parent stores (source private stack is deliberately excluded).
   for(const auto &o:it->second.at("operands"))if(o.at("write").get<bool>()) {
    const auto at=address(o,gp(regs)),n=o.at("bytes").get<unsigned>();bool keep=at==0x48c7a6u;
    for(const auto &b:c.at("banks"))if(at>=b.at("address").get<unsigned>()&&std::uint64_t(at)+n<=std::uint64_t(b.at("address").get<unsigned>())+b.at("before").get_ref<const std::string&>().size()/2)keep=true;
    if(keep)parent_writes.push_back(J::array({pc,at,hex(memory(child.pid,at,n))}));
   }
   if(!boundary.is_null())break;
  }
  request(PTRACE_GETREGS,child.pid,nullptr,&regs);if(active)finish_frame(false);check(completed||!boundary.is_null(),"Original shadow parent did not complete");request(PTRACE_GETFPREGS,child.pid,nullptr,&fp);J after=J::array();for(const auto &b:c.at("banks"))after.push_back(J{{"address",b.at("address")},{"after",hex(memory(child.pid,b.at("address"),b.at("before").get_ref<const std::string&>().size()/2))}});
  result.push_back(J{{"id",c.at("id")},{"name",c.at("name")},{"control",control},{"completed",completed},{"frames",frames},{"trace",trace},{"parent_writes",parent_writes},{"banks",after},{"fp",fp_state(fp)},{"gp",gp(regs)},{"eflags",regs.eflags},{"normal",normal},{"returns166c90",early},{"next_loop",next_loop},{"instructions",steps},{"global48c7a6",hex(memory(child.pid,0x48c7a6,1))}});
 }
 std::ofstream out(argv[4]);out<<J{{"cases",result},{"instruction_patches",J::array()},{"supplied_original_outcomes",false}}.dump()<<'\n';check(bool(out),"Cannot retain original observer result");return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
