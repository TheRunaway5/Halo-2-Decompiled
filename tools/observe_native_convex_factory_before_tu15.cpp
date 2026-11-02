// Offline physical comparison. The original runs unchanged on the CPU; the
// bound native constructor receives typed BEFORE data and independently owned
// scratch. Neither implementation receives the other's results.
#include "halo2/native_tu15_convex_agent_binding.hpp"
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>

namespace {
using namespace halo2;
using J=nlohmann::json;
void check(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
long request(__ptrace_request op,pid_t pid,void* address=nullptr,void* data=nullptr){
    errno=0;const auto value=ptrace(op,pid,address,data);
    if(value==-1&&errno)throw std::runtime_error(strerror(errno));return value;
}
std::string hex(std::span<const std::byte> bytes){
    constexpr char digits[]="0123456789abcdef";std::string value;
    for(auto byte:bytes){auto n=std::to_integer<unsigned>(byte);value+=digits[n>>4];value+=digits[n&15];}return value;
}
struct Child {
    pid_t pid=-1;int status{};
    ~Child(){if(pid>0){kill(pid,SIGKILL);waitpid(pid,&status,0);}}
    void wait(){check(waitpid(pid,&status,0)==pid,"Original wait failed");check(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP,"Original instruction fault, signal="+std::to_string(WSTOPSIG(status)));}
    user_regs_struct regs(){user_regs_struct r{};request(PTRACE_GETREGS,pid,nullptr,&r);return r;}
    user_fpregs_struct fp(){user_fpregs_struct f{};request(PTRACE_GETFPREGS,pid,nullptr,&f);return f;}
    void step(){request(PTRACE_SINGLESTEP,pid);wait();}
    void start(const char* exe){
        pid=fork();check(pid>=0,"Cannot fork original");
        if(!pid){const auto fd=open("/dev/null",O_WRONLY);dup2(fd,1);close(fd);if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(exe,exe,nullptr);_exit(127);}
        wait();request(PTRACE_SETOPTIONS,pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,pid);wait();step();
        check(regs().rip==0x2e4eb0u,"Original typed caller did not enter2E4EB0");
    }
    std::vector<std::byte> read(std::uint32_t address,std::size_t size){
        std::vector<std::byte> bytes(size);iovec local{bytes.data(),size},remote{reinterpret_cast<void*>(std::uintptr_t(address)),size};
        check(process_vm_readv(pid,&local,1,&remote,1,0)==std::ptrdiff_t(size),"Cannot read original bank at "+std::to_string(address));return bytes;
    }
};
J fp_words(const user_fpregs_struct& fp){return J::array({fp.cwd,fp.swd,fp.mxcsr});}
std::array<std::uint32_t,8> gp(const user_regs_struct&r){return{std::uint32_t(r.rax),std::uint32_t(r.rcx),std::uint32_t(r.rdx),std::uint32_t(r.rbx),std::uint32_t(r.rsp),std::uint32_t(r.rbp),std::uint32_t(r.rsi),std::uint32_t(r.rdi)};}
struct Banks:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    mutable J reads=J::array();J writes=J::array();bool tracing=true;
    auto region(std::uint32_t p,std::size_t n)const{
        auto i=banks.upper_bound(p);check(i!=banks.begin(),"Native unmapped operand "+std::to_string(p));--i;
        check(std::uint64_t(p-i->first)+n<=i->second.size(),"Native operand crosses bank at "+std::to_string(p));return i;
    }
    bool persistent(std::uint32_t p,std::size_t n)const{
        if(p>=0x4000000u&&std::uint64_t(p)+n<=0x4010000u)return false;
        auto i=banks.upper_bound(p);if(i==banks.begin())return false;--i;return std::uint64_t(p-i->first)+n<=i->second.size();
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override{
        auto i=region(p,out.size());std::copy_n(i->second.begin()+(p-i->first),out.size(),out.begin());
        if(tracing&&persistent(p,out.size()))reads.push_back(J::array({p,out.size()}));
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override{
        auto i=region(p,in.size());auto& target=banks.at(i->first);const auto off=p-i->first;
        if(tracing&&persistent(p,in.size()))writes.push_back(J::array({p,hex(std::span(target).subspan(off,in.size())),hex(in)}));
        std::copy(in.begin(),in.end(),target.begin()+off);
    }
    void read_vector(std::uint32_t p,std::span<std::byte,16> out)const override{read(p,out);}
};
void compare_ledger(const J& expected,const J& actual,const char* label,J& report){
    report[std::string(label)+"_original_count"]=expected.size();report[std::string(label)+"_native_count"]=actual.size();
    if(expected==actual)return;
    std::size_t n{};while(n<std::min(expected.size(),actual.size())&&expected[n]==actual[n])++n;
    report["ledger_mismatch"]=J{{"kind",label},{"index",n},{"original",n<expected.size()?expected[n]:J()},{"native",n<actual.size()?actual[n]:J()}};
    throw std::runtime_error(std::string("Ordered persistent ")+label+" mismatch");
}
}
int main(int argc,char** argv){
    J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false},{"captured_original_gp_frame",false},{"persistent_bank_waivers",J::array()}};
    try{
        check(argc==6,"Supply original ELF, BEFORE manifest, metadata, scratch poison, report");
        std::ifstream manifest_file(argv[2]),metadata_file(argv[3]);const auto manifest=J::parse(manifest_file),metadata=J::parse(metadata_file);
        check(manifest.at("schema")=="native-tu15-contact-agent-factory-before-v1"&&!manifest.at("expected_after_consumed").get<bool>()&&!manifest.at("physics_result_supplied").get<bool>(),"Requires authentic BEFORE schema");
        check(manifest.at("input").at("owner")==0x2e4eb0u&&manifest.at("input").at("preserved_x87_depth")==0,"Requires declared convex empty-x87 input");
        Banks memory;std::uint64_t total{};
        const auto directory=std::filesystem::path(argv[2]).parent_path();
        for(const auto& region:manifest.at("regions")){
            const auto address=region.at("address").get<std::uint32_t>(),size=region.at("size").get<std::uint32_t>();
            check(!(address<0x4010000u&&0x4000000u<std::uint64_t(address)+size),"Native private DATA overlaps captured bank");
            std::ifstream file(directory/region.at("before_file").get<std::string>(),std::ios::binary);std::vector<std::byte> bytes(size);
            file.read(reinterpret_cast<char*>(bytes.data()),size);check(file.gcount()==size&&file.peek()==EOF,"Incorrect BEFORE bank size");
            check(memory.banks.emplace(address,std::move(bytes)).second,"Duplicate BEFORE bank");total+=size;
        }
        for(auto i=memory.banks.begin();i!=memory.banks.end();++i){auto next=std::next(i);check(next==memory.banks.end()||std::uint64_t(i->first)+i->second.size()<=next->first,"Overlapping BEFORE banks");}
        const auto poison=std::stoul(argv[4],nullptr,0);check(poison<=255,"Scratch poison out of range");
        memory.banks.emplace(0x4000000u,std::vector<std::byte>(0x10000,std::byte(poison)));
        std::map<std::uint32_t,J> sites;for(const auto&site:metadata.at("sites"))check(sites.emplace(site.at("instruction"),site).second,"Duplicate original site");
        Child original;original.start(argv[1]);const auto input_fp=original.fp();
        const auto& declared_fp=manifest.at("input").at("floating_point");
        check(fp_words(input_fp)==J::array({declared_fp.at("x87_control_word"),declared_fp.at("x87_status"),declared_fp.at("mxcsr")})&&input_fp.ftw==0,"Original FP differs from typed BEFORE");
        for(const auto& [address,bytes]:memory.banks)if(address!=0x4000000u)check(original.read(address,bytes.size())==bytes,"Original input bank differs from BEFORE");
        const auto& table=metadata.at("immutable_dispatch_table");
        const auto table_address=table.at("address").get<std::uint32_t>(),table_size=table.at("bytes").get<std::uint32_t>();
        check(hex(original.read(table_address,table_size))==table.at("hex").get<std::string>(),"Original immutable dispatch table differs from pinned XBE");
        J reads=J::array(),writes=J::array(),control_reads=J::array();std::set<std::uint32_t> executed;unsigned instructions{};
        for(;instructions<100000;++instructions){
            const auto r=original.regs();const auto pc=std::uint32_t(r.rip);if(pc==0x1ccacau)break;
            const auto site=sites.find(pc);check(site!=sites.end(),"Original escaped authenticated bodies pc="+std::to_string(pc));executed.insert(pc);const auto registers=gp(r);J pending=J::array();
            for(const auto& op:site->second.at("operands")){
                auto address=op.at("displacement").get<std::uint32_t>();const auto base=op.at("base").get<int>(),index=op.at("index").get<int>();
                if(base>=0)address+=registers.at(base);if(index>=0)address+=registers.at(index)*op.at("scale").get<std::uint32_t>();
                const auto size=op.at("bytes").get<std::uint32_t>();if(!memory.persistent(address,size))continue;
                if(site->second.at("immutable_control_dispatch").get<bool>()){
                    check(op.at("read").get<bool>()&&!op.at("write").get<bool>()&&address>=table_address&&std::uint64_t(address)+size<=std::uint64_t(table_address)+table_size,"Original switch escaped its authenticated immutable table");
                    const auto value=hex(original.read(address,size));check(value==table.at("hex").get<std::string>().substr((address-table_address)*2u,size*2u),"Original switch-table operand changed");
                    control_reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",size},{"value",value}});continue;
                }
                if(op.at("read").get<bool>())reads.push_back(J::array({address,size}));
                if(op.at("write").get<bool>())pending.push_back(J::array({address,hex(original.read(address,size)),size}));
            }
            original.step();for(const auto& store:pending){const auto p=store[0].get<std::uint32_t>(),n=store[2].get<std::uint32_t>();writes.push_back(J::array({p,store[1],hex(original.read(p,n))}));}
        }
        check(original.regs().rip==0x1ccacau,"Original instruction limit");
        report.update(J{{"original_completed",true},{"original_instructions",instructions},{"executed_sites",executed},{"original_return",std::uint32_t(original.regs().rax)},{"original_after_fp",fp_words(original.fp())},{"captured_bank_bytes",total},{"captured_bank_count",manifest.at("regions").size()},{"scratch_poison",poison},{"authenticated_original_control_dispatch_reads",control_reads},{"native_control_lowering","Native C++ branches; immutable original switch-table reads checked separately"}});
        NativeTu15PhysicsFrameFp fp{input_fp.cwd,input_fp.swd,input_fp.mxcsr};
        const auto args=manifest.at("input").at("arguments").get<std::array<std::uint32_t,4>>();NativeTu15ConvexAgentBindingActivity activity;
        const auto result=native_tu15_create_bound_convex_contact_agent(memory,{[&]{return args[0];},[&]{return args[1];},[&]{return args[3];}},
            {0x4002000u,0x4002100u,0x4002300u,0x4002400u,0x4002500u,0x4002600u},fp,0,{},&activity);
        report.update(J{{"native_completed",true},{"native_return",result.identity},{"native_after_fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"native_activity",J{{"completed",activity.completed},{"pool_allocations",activity.pool_allocations},{"class_queries",activity.class_queries},{"centers",activity.centers},{"supports",activity.supports},{"encodings",activity.encodings}}}});
        check(result.identity==std::uint32_t(original.regs().rax),"Returned agent mismatch");check(report["native_after_fp"]==report["original_after_fp"],"Final FP mismatch");
        for(const auto& [address,bytes]:memory.banks)if(address!=0x4000000u){const auto expected=original.read(address,bytes.size());if(expected!=bytes){std::size_t off{};while(off<bytes.size()&&bytes[off]==expected[off])++off;report["first_bank_mismatch"]=J{{"address",address+off},{"original",std::to_integer<unsigned>(expected[off])},{"native",std::to_integer<unsigned>(bytes[off])}};throw std::runtime_error("Complete persistent bank mismatch");}}
        report["complete_banks_match"]=true;
        compare_ledger(writes,memory.writes,"writes",report);compare_ledger(reads,memory.reads,"reads",report);
        report["passed"]=true;
    }catch(const std::exception& error){report["error"]=error.what();}
    if(argc==6){std::ofstream file(argv[5]);file<<report.dump(2)<<'\n';if(!file)return 2;}
    std::cout<<report.value("error",std::string("Complete banks, FP, return and persistent operands match"))<<'\n';return report.at("passed").get<bool>()?0:1;
}
