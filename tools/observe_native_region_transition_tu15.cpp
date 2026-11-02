// Offline physical BC720 oracle. Each side starts from the same actual BEFORE
// and executes its own unchanged original callees on its own current banks.
#include "gameplay/objects/source_region_transition_native_tu15_internal.hpp"
#include "gameplay/objects/source_model_region_state_native_tu15_internal.hpp"
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;using namespace halo2::physics_world_runtime_detail;using J=nlohmann::json;
struct PrefixStop {};
void check(bool yes,const std::string& why){if(!yes)throw std::runtime_error(why);}
long request(__ptrace_request op,pid_t pid,void* address=nullptr,void* data=nullptr){
    errno=0;const auto result=ptrace(op,pid,address,data);
    if(result==-1&&errno)throw std::runtime_error(strerror(errno));return result;
}
std::array<std::uint32_t,8> gp(const user_regs_struct& r){return {
    std::uint32_t(r.rax),std::uint32_t(r.rcx),std::uint32_t(r.rdx),std::uint32_t(r.rbx),
    std::uint32_t(r.rsp),std::uint32_t(r.rbp),std::uint32_t(r.rsi),std::uint32_t(r.rdi)};}
void set_gp(user_regs_struct& r,const std::array<std::uint32_t,8>& a){
    r.rax=a[0];r.rcx=a[1];r.rdx=a[2];r.rbx=a[3];r.rsp=a[4];r.rbp=a[5];r.rsi=a[6];r.rdi=a[7];
}
unsigned live_x87(const user_fpregs_struct& f){
    const auto top=(f.swd>>11u)&7u;unsigned count{};
    while(count<8u&&(f.ftw&(1u<<((top+count)&7u))))++count;
    unsigned mask{};for(unsigned n=0;n<count;++n)mask|=1u<<((top+n)&7u);
    check(mask==f.ftw&&count<8u,"Sparse/full x87 or active MMX needs a separate explicit physical owner");return count;
}
std::string raw_extended(const void* data){
    constexpr char digits[]="0123456789abcdef";std::string text;const auto* bytes=static_cast<const unsigned char*>(data);
    for(unsigned n=0;n<10;++n){text+=digits[bytes[n]>>4u];text+=digits[bytes[n]&15u];}return text;
}
J fp_input(const user_fpregs_struct& f){
    // Only live logical80-bit values are ABI. Preserve their real physical
    // TOP/tags; never invent dead words or debugger instruction/data pointers.
    const auto count=live_x87(f);J values=J::array();
    for(unsigned n=0;n<count;++n)values.push_back(raw_extended(reinterpret_cast<const unsigned char*>(f.st_space)+n*16u));
    std::array<std::array<std::uint32_t,4>,8> xmm{};std::memcpy(xmm.data(),f.xmm_space,128);
    return J{{"control",f.cwd},{"status",f.swd},{"mxcsr",f.mxcsr},{"tag",f.ftw},{"xmm",xmm},{"live_x87",values}};
}
struct Child {
    pid_t pid=-1;int status{};
    ~Child(){if(pid>0){kill(pid,SIGKILL);waitpid(pid,&status,0);}}
    void wait(){check(waitpid(pid,&status,0)==pid,"Physical child wait failed");check(WIFSTOPPED(status),"Original code left without a return boundary");}
    void debug(unsigned n,unsigned long value){request(PTRACE_POKEUSER,pid,
        reinterpret_cast<void*>(offsetof(user,u_debugreg)+n*sizeof(unsigned long)),reinterpret_cast<void*>(value));}
    user_regs_struct regs(){user_regs_struct r{};request(PTRACE_GETREGS,pid,nullptr,&r);return r;}
    user_fpregs_struct fp(){user_fpregs_struct f{};request(PTRACE_GETFPREGS,pid,nullptr,&f);return f;}
    void go(std::uint32_t target){
        debug(0,target);debug(6,0);debug(7,1);request(PTRACE_CONT,pid);wait();const auto r=regs();debug(7,0);
        check(WSTOPSIG(status)==SIGTRAP&&r.rip==target,"Physical original failed before "+std::to_string(target)+" at "+std::to_string(r.rip)+" signal "+std::to_string(WSTOPSIG(status)));
    }
    void step(){request(PTRACE_SINGLESTEP,pid);wait();check(WSTOPSIG(status)==SIGTRAP,"Physical original instruction fault at "+std::to_string(regs().rip));}
    void start(const char* exe){
        pid=fork();check(pid>=0,"Cannot fork original process");
        if(!pid){const int fd=open("/dev/null",O_WRONLY);dup2(fd,1);close(fd);
            if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(exe,exe,nullptr);_exit(127);}
        wait();request(PTRACE_SETOPTIONS,pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));
        request(PTRACE_CONT,pid);wait();check(WSTOPSIG(status)==SIGTRAP,"Actual input bridge failed");go(0xbc720u);
    }
    std::vector<std::byte> read(std::uint32_t address,std::size_t size){
        std::vector<std::byte> data(size);iovec local{data.data(),size},remote{reinterpret_cast<void*>(std::uintptr_t(address)),size};
        check(process_vm_readv(pid,&local,1,&remote,1,0)==std::ptrdiff_t(size),"Cannot read original bank "+std::to_string(address)+" bytes "+std::to_string(size));return data;
    }
    void write(std::uint32_t address,std::span<const std::byte> data){
        iovec local{const_cast<std::byte*>(data.data()),data.size()},remote{reinterpret_cast<void*>(std::uintptr_t(address)),data.size()};
        check(process_vm_writev(pid,&local,1,&remote,1,0)==std::ptrdiff_t(data.size()),"Cannot write current original bank "+std::to_string(address));
    }
    std::uint32_t word(std::uint32_t address){const auto data=read(address,4);std::uint32_t result{};std::memcpy(&result,data.data(),4);return result;}
};
struct Memory final:SourcePhysicsContactMemory {
    Child& child;std::uint32_t pc{};mutable J accesses=J::array();J stores=J::array();
    explicit Memory(Child& c):child(c){}
    void read(std::uint32_t address,std::span<std::byte> data)const override {
        accesses.push_back(J::array({pc,"read",address,data.size()}));
        const auto original=child.read(address,data.size());std::copy(original.begin(),original.end(),data.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> data)override {
        accesses.push_back(J::array({pc,"write",address,data.size()}));
        std::uint32_t value{};for(unsigned i=0;i<data.size();++i)value|=std::to_integer<unsigned>(data[i])<<(8u*i);
        stores.push_back(J::array({pc,address,data.size(),value}));child.write(address,data);
    }
    void read_vector(std::uint32_t address,std::span<std::byte,16> data)const override{read(address,data);}
};
SourcePhysicsContactRuntimeAbi abi(Child& child,std::uint32_t fs){
    const auto r=child.regs();const auto f=child.fp();fp_input(f);
    SourcePhysicsContactRuntimeAbi a{};a.gp=gp(r);a.eflags=r.eflags&~0x10000u;a.fs_base=fs;
    a.floating_point=SourcePhysicsFactoryFloatingPoint{f.cwd,f.mxcsr};a.x87_status=f.swd;
    a.x87_depth=live_x87(f);
    for(unsigned n=0;n<a.x87_depth;++n)std::memcpy(&a.x87[n],reinterpret_cast<const unsigned char*>(f.st_space)+n*16u,10);
    std::memcpy(a.xmm.data(),f.xmm_space,128);return a;
}
J semantic(const SourcePhysicsContactRuntimeAbi& a){
    check(a.x87_depth<8u&&!a.active_mmx,"Unsupported full x87/MMX state at actual parent boundary");
    J values=J::array();unsigned tag{};const auto top=(a.x87_status>>11u)&7u;
    for(unsigned n=0;n<a.x87_depth;++n){values.push_back(raw_extended(&a.x87[n]));tag|=1u<<((top+n)&7u);}
    return J{{"control",a.floating_point->x87_control_word},{"status",a.x87_status},
        {"mxcsr",a.floating_point->mxcsr},{"tag",tag},{"xmm",a.xmm},{"live_x87",values}};
}
void install(Child& child,const SourcePhysicsContactRuntimeAbi& a,std::uint32_t entry){
    semantic(a);auto r=child.regs();set_gp(r,a.gp);r.rip=entry;r.eflags=a.eflags&~0x10000u;
    request(PTRACE_SETREGS,child.pid,nullptr,&r);auto f=child.fp();
    f.cwd=a.floating_point->x87_control_word;f.swd=a.x87_status;f.mxcsr=a.floating_point->mxcsr;
    f.ftw=0;const auto top=(a.x87_status>>11u)&7u;
    for(unsigned n=0;n<a.x87_depth;++n){f.ftw|=1u<<((top+n)&7u);std::memcpy(reinterpret_cast<unsigned char*>(f.st_space)+n*16u,&a.x87[n],10);}
    std::memcpy(f.xmm_space,a.xmm.data(),128);request(PTRACE_SETFPREGS,child.pid,nullptr,&f);
}
void finish_service(Child& child,std::uint32_t after,std::uint32_t expected_sp){
    // A nested transition may use the same return PC. Observe its genuine
    // completion too; stop only at this actual call's stack boundary.
    for(unsigned nested=0;nested<256;++nested){child.go(after);if(std::uint32_t(child.regs().rsp)==expected_sp)return;child.step();}
    throw std::runtime_error("Original service exceeded nested return budget");
}
unsigned argument_bytes(std::uint32_t target){switch(target){
    case 0x1ce340:case 0x17e060:case 0xb9390:return 0;
    case 0x1704c0:return 40;case 0x1ce0e0:return 4;
    default:throw std::runtime_error("Undeclared original BC720 callee");}}
void apply_model_component_inputs(Child& child,std::uint32_t sp,const J& inputs){
    auto put=[&](std::uint32_t address,std::uint32_t value,unsigned width){
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        child.write(address,std::span(bytes).first(width));
    };
    const auto formals=inputs.value("formal_words",J::object());
    for(const auto& [index,value]:formals.items()){
        const auto number=std::stoul(index);check(number<10,"Invalid declared model formal");put(sp+4u+number*4u,value,4);
    }
    if(inputs.contains("model_count")||inputs.contains("mapping_byte")){
        const auto model=child.word(child.word(0x4f0414u)+(child.word(sp+8u)&65535u)*16u+8u);
        if(inputs.contains("model_count"))put(model+0x70u,inputs.at("model_count"),4);
        if(inputs.contains("mapping_byte")){
            const auto variant=child.word(sp+12u),region=child.word(sp+16u);
            check(variant!=0xffffffffu&&region<16u,"Invalid declared model mapping input");
            put(child.word(model+0x54u)+variant*0x38u+region+4u,inputs.at("mapping_byte"),1);
        }
    }
    if(inputs.contains("assert_authored_effect")){
        const auto& expected=inputs.at("assert_authored_effect");const auto tags=child.word(0x4f0414u);
        const auto model_datum=child.word(sp+8u),variant=child.word(sp+12u),region=child.word(sp+16u);
        check(model_datum==expected.at("model")&&variant==expected.at("variant")&&region==expected.at("region"),"Authored model effect case uses another model/variant/region");
        const auto model=child.word(tags+(model_datum&65535u)*16u+8u);
        const auto definition=child.word(model+0x54u)+variant*0x38u;
        const auto mapped=std::to_integer<unsigned>(child.read(definition+region+4u,1)[0]);check(mapped<128,"Authored model effect mapping is absent");
        const auto region_data=child.word(definition+0x18u)+mapped*20u;
        check(child.word(region_data+8u)>0,"Authored model effect region has no permutation");
        const auto permutation=child.word(region_data+0xcu);check(child.word(permutation+0xcu)>0,"Authored model effect permutation has no state");
        const auto state=child.word(permutation+0x10u);const auto state_word=child.word(state+4u);
        check((state_word>>16u)==expected.at("state")&&child.word(state+0xcu)==expected.at("effect")&&
            child.word(state+0x10u)==expected.at("parameter"),"Authored model effect state data changed");
        const auto effect=child.word(state+0xcu),row=tags+(effect&65535u)*16u;
        check(child.word(row)==0x65666665u&&child.word(row+4u)==effect&&child.word(row+8u)&&child.word(row+12u),"Authored model effect tag is not resident");
    }
    if(inputs.contains("duplicate_permutations")){
        const auto count=inputs.at("duplicate_permutations").get<unsigned>(),arena=inputs.at("declared_arena").get<std::uint32_t>();
        check(count>1&&count<=32,"Invalid declared permutation copy count");
        const auto model=child.word(child.word(0x4f0414u)+(child.word(sp+8u)&65535u)*16u+8u);
        const auto variant=child.word(model+0x54u)+child.word(sp+12u)*0x38u,region=child.word(sp+16u);
        check(region<16,"Invalid declared copied permutation region");const auto mapped=std::to_integer<unsigned>(child.read(variant+region+4u,1)[0]);
        check(mapped<128,"Copied authored permutation mapping is absent");const auto definition=child.word(variant+0x18u)+mapped*20u;
        check(child.word(definition+8u)>0,"No authored permutation to copy");const auto bytes=child.read(child.word(definition+0xcu),32);
        for(unsigned n=0;n<count;++n)child.write(arena+n*32u,bytes);
        put(definition+8u,count,4);put(definition+0xcu,arena,4);
        put(child.word(sp+40u)+region*8u,255u,1);
    }
}
J trace_model_owner(Child& child,const J& metadata,std::uint32_t after,std::uint32_t after_sp,J& report){
    J result{{"entry",0x1704c0},{"entry_gp",gp(child.regs())},{"entry_fp",fp_input(child.fp())},
        {"trace",J::array()},{"calls",J::array()},{"accesses",J::array()},{"stores",J::array()},{"instruction_patches",J::array()},
        {"supplied_original_outcomes",false},{"semantic_native_model_kernel_verified",false}};
    J arguments=J::array();const auto sp=std::uint32_t(child.regs().rsp);
    for(unsigned i=0;i<10;++i)arguments.push_back(child.word(sp+4u+i*4u));result["arguments"]=arguments;
    const auto random=child.word(0x4f3cd8u);result["random_address"]=random;result["random_before"]=child.word(random);
    unsigned flags_mask=0x8d5u;
    for(unsigned count=0;;++count){
        check(count<16384,"Original model-owner trace budget exceeded");const auto r=child.regs();const auto pc=std::uint32_t(r.rip);
        if(pc==after&&std::uint32_t(r.rsp)==after_sp)break;
        report["active_model_pc"]=pc;const auto& row=metadata.at(std::to_string(pc));
        const auto mnemonic=row.at("mnemonic").get<std::string>();
        result["trace"].push_back(J{{"pc",pc},{"gp",gp(r)},{"flags",unsigned(r.eflags)&flags_mask},{"mask",flags_mask}});
        std::vector<std::pair<std::uint32_t,unsigned>> pending;
        auto access=[&](std::uint32_t address,unsigned width,bool write){result["accesses"].push_back(J::array({pc,write?"write":"read",address,width}));if(write)pending.emplace_back(address,width);};
        for(const auto& operand:row.at("memory")){
            const auto regs=gp(r);const auto base=operand.at("base").get<int>(),index=operand.at("index").get<int>();
            const auto address=std::uint32_t((base>=0?regs[base]:0)+(index>=0?regs[index]*operand.at("scale").get<unsigned>():0)+operand.at("displacement").get<std::int64_t>());
            const auto kind=operand.at("access").get<unsigned>();check(kind==1||kind==2,"Unexpected model-owner memory read-modify-write");access(address,operand.at("width"),kind==2);
        }
        if(mnemonic=="push"||mnemonic=="call")access(std::uint32_t(r.rsp)-4,4,true);
        if(mnemonic=="pop"||mnemonic.starts_with("ret"))access(r.rsp,4,false);
        child.step();
        for(auto [address,width]:pending){const auto data=child.read(address,width);std::uint32_t value{};std::memcpy(&value,data.data(),width);result["stores"].push_back(J::array({pc,address,width,value}));}
        if(row.at("mnemonic")=="call"){
            J call{{"site",pc},{"target",row.at("target")},{"entry_gp",gp(child.regs())},{"entry_fp",fp_input(child.fp())},
                {"entry_flags",unsigned(child.regs().eflags)&flags_mask},{"entry_flags_mask",flags_mask}};
            report["active_model_service"]=call;
            finish_service(child,pc+row.at("size").get<unsigned>(),std::uint32_t(r.rsp)+row.at("return_bytes").get<unsigned>());
            call["return_gp"]=gp(child.regs());call["return_fp"]=fp_input(child.fp());call["return_flags"]=unsigned(child.regs().eflags)&0x8d5u;result["calls"].push_back(call);flags_mask=0x8d5u;
        }else if(mnemonic=="imul")flags_mask=0x801u;
        else if(mnemonic=="and"||mnemonic=="test"||mnemonic=="xor"||mnemonic=="or")flags_mask=0x8c5u;
        else if(mnemonic=="shr"||mnemonic=="sar"||mnemonic=="shl")flags_mask=0xc5u;
        else if(mnemonic=="cmp"||mnemonic=="sub"||mnemonic=="add"||mnemonic=="inc")flags_mask=0x8d5u;
    }
    result["return_gp"]=gp(child.regs());result["return_fp"]=fp_input(child.fp());result["return_flags"]=unsigned(child.regs().eflags)&0x8d5u;result["random_after"]=child.word(random);result["completed_original_return"]=true;return result;
}
SourcePhysicsContactRuntimeAbi run_native_model_owner(Child& child,const SourcePhysicsContactRuntimeAbi& initial,
    const J& expected,const J& metadata,J& report){
    Memory memory(child);const SourcePhysicsWorldRuntimeServices services{};const SourcePhysicsContactRecordObserver observer{};
    WorldState state(memory,observer,services,SourcePhysicsContactImage::TitleUpdate15);state.load(initial);
    J trace=J::array(),calls=J::array();std::size_t index{};
    auto site=[&](std::uint32_t pc){
        memory.pc=pc;check(index<expected.at("trace").size(),"Native model emitted an extra instruction");
        const auto& row=expected.at("trace").at(index++);const auto mask=row.at("mask").get<unsigned>();const auto saved=state.save();
        J actual{{"pc",pc},{"gp",saved.gp},{"flags",saved.eflags&mask},{"mask",mask}};trace.push_back(actual);
        if(actual!=row){report["original_model_instruction"]=row;report["native_model_instruction"]=actual;throw std::runtime_error("Native model instruction ABI differs");}
        return pc;
    };
    auto invoke=[&](std::uint32_t pc,std::uint32_t target){
        const auto before=state.save();const auto after=child.word(before.gp[4]);const auto& original_call=expected.at("calls").at(calls.size());
        const auto mask=original_call.at("entry_flags_mask").get<unsigned>();
        J call{{"site",pc},{"target",target},{"entry_gp",before.gp},{"entry_fp",semantic(before)},
            {"entry_flags",before.eflags&mask},{"entry_flags_mask",mask}};
        report["active_native_model_service"]=call;install(child,before,target);
        finish_service(child,after,before.gp[4]+4u+metadata.at(std::to_string(pc)).at("return_bytes").get<unsigned>());
        const auto result=abi(child,before.fs_base);state.load(result);
        call["return_gp"]=result.gp;call["return_fp"]=semantic(result);call["return_flags"]=result.eflags&0x8d5u;calls.push_back(call);
        if(call!=original_call){report["expected_model_service"]=original_call;report["actual_model_service"]=call;throw std::runtime_error("Native model original-service boundary differs");}
    };
    model_region_state_native_tu15(state,site,invoke);const auto result=state.save();
    report["native_model_owner"]=J{{"passed",index==expected.at("trace").size()&&calls==expected.at("calls")&&
            memory.accesses==expected.at("accesses")&&memory.stores==expected.at("stores")&&
            J(result.gp)==expected.at("return_gp")&&semantic(result)==expected.at("return_fp")&&
            (result.eflags&0x8d5u)==expected.at("return_flags").get<unsigned>()},
        {"trace",trace},{"calls",calls},{"ordered_accesses_equal",memory.accesses==expected.at("accesses")},
        {"ordered_stores_equal",memory.stores==expected.at("stores")},{"return_gp",result.gp},{"return_fp",semantic(result)}};
    if(memory.accesses!=expected.at("accesses")){report["original_model_accesses"]=expected.at("accesses");report["native_model_accesses"]=memory.accesses;}
    if(memory.stores!=expected.at("stores")){report["original_model_stores"]=expected.at("stores");report["native_model_stores"]=memory.stores;}
    check(report.at("native_model_owner").at("passed"),"Native model complete instruction/access/store/ABI comparison differs");return result;
}
}

int main(int argc,char** argv){
    J report{{"passed",false},{"source_entry",0xbc720},{"captured_native_after_used",false},
        {"supplied_original_outcomes",false},{"instruction_patches",J::array()},
        {"remaining_services_execute_original_cpu",true},{"production_native_only_closure",false}};
    try {
        check(argc==5,"Supply physical ELF, BEFORE manifest, instruction metadata and report");
        std::ifstream input(argv[2]),metadata_input(argv[3]);const auto capture=J::parse(input),metadata=J::parse(metadata_input);
        const auto fs=capture.at("before").at("fs_base").get<std::uint32_t>();
        const auto stop_before=metadata.value("stop_before",0u);
        const auto model_inputs=metadata.value("model_component_inputs",J::object());
        report["declared_model_component_inputs"]=model_inputs;
        report["complete_parent_return"]=stop_before==0;
        report["declared_stop_before"]=stop_before;
        auto patch_before=[&](Child& child){for(const auto& patch:metadata.value("before_patches",J::array())){
            const auto width=patch.at("width").get<unsigned>(),value=patch.at("value").get<std::uint32_t>();
            check(width<=4,"Invalid declared BEFORE width");std::array<std::byte,4> data{};
            for(unsigned i=0;i<width;++i)data[i]=std::byte(value>>(8u*i));child.write(patch.at("address"),std::span(data).first(width));
        }};
        report["declared_before_patches"]=metadata.value("before_patches",J::array());
        Child original;original.start(argv[1]);patch_before(original);const auto entry=original.regs();const auto initial_fp=fp_input(original.fp());
        const auto entry_sp=std::uint32_t(entry.rsp),return_pc=original.word(entry_sp);
        check(return_pc==metadata.at("return_address"),"Actual caller differs from metadata");
        report["actual_entry_gp"]=gp(entry);report["actual_entry_fp"]=initial_fp;
        report["arguments"]=J::array({original.word(entry_sp+4),original.word(entry_sp+8),original.word(entry_sp+12),original.word(entry_sp+16)});
        struct Bank{std::uint32_t address;std::vector<std::byte> after;};std::vector<Bank> banks;
        J trace=J::array(),accesses=J::array(),stores=J::array(),calls=J::array();unsigned flags_mask=0x8d5u;
        report["phase"]="unchanged_original_parent";
        for(unsigned instructions=0;;++instructions){
            check(instructions<4096,"Unbounded original BC720 parent");const auto r=original.regs();const auto pc=std::uint32_t(r.rip);if(pc==return_pc||(stop_before&&pc==stop_before))break;
            report["active_parent_pc"]=pc;const auto& row=metadata.at("instructions").at(std::to_string(pc));const auto mnemonic=row.at("mnemonic").get<std::string>();
            trace.push_back(J{{"pc",pc},{"gp",gp(r)},{"flags",unsigned(r.eflags)&flags_mask},{"mask",flags_mask}});
            std::vector<std::pair<std::uint32_t,unsigned>> pending;
            auto access=[&](std::uint32_t address,unsigned width,bool write){accesses.push_back(J::array({pc,write?"write":"read",address,width}));if(write)pending.emplace_back(address,width);};
            if(mnemonic.starts_with("rep")){
                if(std::uint32_t(r.rcx)){const unsigned width=pc==0xbc797u?4:1;access(r.rsi,width,false);access(r.rdi,width,true);}
            } else for(const auto& operand:row.at("memory")){
                const auto regs=gp(r);const auto base=operand.at("base").get<int>(),index=operand.at("index").get<int>();
                const auto address=std::uint32_t((base>=0?regs[base]:0)+(index>=0?regs[index]*operand.at("scale").get<unsigned>():0)+operand.at("displacement").get<std::int64_t>());
                const auto kind=operand.at("access").get<unsigned>();check(kind==1||kind==2,"Unexpected parent read-modify-write operand");access(address,operand.at("width"),kind==2);
            }
            if(mnemonic=="push"||mnemonic=="call")access(std::uint32_t(r.rsp)-4,4,true);
            if(mnemonic=="pop"||mnemonic.starts_with("ret"))access(r.rsp,4,false);
            original.step();
            for(auto [address,width]:pending){const auto data=original.read(address,width);std::uint32_t value{};std::memcpy(&value,data.data(),width);stores.push_back(J::array({pc,address,width,value}));}
            if(mnemonic=="call"){
                const auto target=row.at("target").get<std::uint32_t>(),after=pc+row.at("size").get<unsigned>();
                J call{{"site",pc},{"target",target},{"before_gp",gp(original.regs())},{"before_fp",fp_input(original.fp())}};
                report["active_service"]=call;
                if(target==0x1704c0u)apply_model_component_inputs(original,original.regs().rsp,model_inputs);
                if(target==0x1704c0u&&metadata.contains("model_instructions"))
                    report["original_model_owner"]=trace_model_owner(original,metadata.at("model_instructions"),after,std::uint32_t(r.rsp)+argument_bytes(target),report);
                else finish_service(original,after,std::uint32_t(r.rsp)+argument_bytes(target));
                call["after_gp"]=gp(original.regs());call["after_fp"]=fp_input(original.fp());call["after_flags"]=unsigned(original.regs().eflags)&0x8d5u;calls.push_back(call);flags_mask=0x8d5u;
                report["completed_original_services"]=calls;
            }else if(mnemonic=="imul")flags_mask=0x801u;
            else if(mnemonic=="and"||mnemonic=="test"||mnemonic=="xor"||mnemonic=="or")flags_mask=0x8c5u;
            else if(mnemonic=="shr"||mnemonic=="sar"||mnemonic=="shl")flags_mask=0xc5u;
            else if(mnemonic=="cmp"||mnemonic=="sub"||mnemonic=="add")flags_mask=0x8d5u;
        }
        const auto expected=abi(original,fs);report["original_final_gp"]=expected.gp;
        for(const auto& row:capture.at("regions")){const auto address=row.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,row.at("size"))});}
        for(const auto& row:metadata.value("additional_comparison_regions",J::array())){const auto address=row.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,row.at("size"))});}
        Child native;native.start(argv[1]);patch_before(native);Memory memory(native);const SourcePhysicsWorldRuntimeServices services{};const SourcePhysicsContactRecordObserver observer{};
        WorldState state(memory,observer,services,SourcePhysicsContactImage::TitleUpdate15);state.load(abi(native,fs));
        J native_trace=J::array(),native_calls=J::array();std::size_t trace_index{};std::uint32_t previous_pc{};bool previous_copy{};
        report["phase"]="native_parent_with_unchanged_original_services";
        auto site=[&](std::uint32_t pc){
            memory.pc=pc;const bool copy=pc==0xbc797u||pc==0xbc79eu;
            if(stop_before&&pc==stop_before)throw PrefixStop{};
            // The generated source observer emits a final zero-count REP
            // cursor. Physical single-step emits only the actual iterations.
            // Keep the legitimate initial zero-count instruction in both.
            if(copy&&previous_copy&&previous_pc==pc&&state.gp[1]==0)return pc;
            previous_pc=pc;previous_copy=copy;
            check(trace_index<trace.size(),"Native parent emitted an extra source instruction");const auto& row=trace.at(trace_index++);
            const auto saved=state.save();const auto mask=row.at("mask").get<unsigned>();
            J current{{"pc",pc},{"gp",saved.gp},{"flags",saved.eflags&mask},{"mask",mask}};native_trace.push_back(current);
            if(current!=row){report["expected_instruction"]=row;report["native_instruction"]=current;throw std::runtime_error("Native parent instruction ABI differs");}
            return pc;
        };
        auto invoke=[&](std::uint32_t pc,std::uint32_t target){
            const auto before=state.save();const auto after=native.word(before.gp[4]);
            J call{{"site",pc},{"target",target},{"before_gp",before.gp},{"before_fp",semantic(before)}};
            report["active_native_service"]=call;
            if(target==0x1704c0u)apply_model_component_inputs(native,before.gp[4],model_inputs);
            SourcePhysicsContactRuntimeAbi result;
            if(target==0x1704c0u&&metadata.value("native_model_owner",false))
                result=run_native_model_owner(native,before,report.at("original_model_owner"),metadata.at("model_instructions"),report);
            else {
                install(native,before,target);finish_service(native,after,before.gp[4]+4u+argument_bytes(target));result=abi(native,fs);
            }
            state.load(result);
            call["after_gp"]=result.gp;call["after_fp"]=semantic(result);call["after_flags"]=result.eflags&0x8d5u;native_calls.push_back(call);
            check(native_calls.size()<=calls.size()&&call==calls.at(native_calls.size()-1),"Native original-service boundary differs");
        };
        bool prefix_stopped=false;
        try{region_transition_native_tu15(state,site,invoke);}catch(const PrefixStop&){prefix_stopped=true;}
        check(prefix_stopped==bool(stop_before),"Native parent did not honor the declared observation boundary");const auto actual=state.save();
        std::size_t different{},compared{};J differences=J::array();
        for(const auto& bank:banks){const auto data=native.read(bank.address,bank.after.size());compared+=data.size();for(std::size_t i=0;i<data.size();++i)if(data[i]!=bank.after[i]){++different;if(differences.size()<64)differences.push_back(J{{"address",bank.address+i},{"native",std::to_integer<unsigned>(data[i])},{"original",std::to_integer<unsigned>(bank.after[i])}});}}
        const auto final_mask=flags_mask|0x400u;
        report.update(J{{"passed",actual.gp==expected.gp&&semantic(actual)==semantic(expected)&&((actual.eflags^expected.eflags)&final_mask)==0&&
                trace_index==trace.size()&&memory.accesses==accesses&&memory.stores==stores&&native_calls==calls&&different==0},
            {"native_final_gp",actual.gp},{"final_fp_equal",semantic(actual)==semantic(expected)},
            {"final_flags_equal",((actual.eflags^expected.eflags)&final_mask)==0},{"parent_instructions",trace.size()},
            {"ordered_accesses_equal",memory.accesses==accesses},{"ordered_stores_equal",memory.stores==stores},
            {"service_calls_equal",native_calls==calls},{"compared_bytes",compared},{"different_bytes",different},{"first_differences",differences},
            {"calls",calls},{"parent_trace",trace},{"native_parent_trace",native_trace}});
        if(memory.accesses!=accesses){report["original_accesses"]=accesses;report["native_accesses"]=memory.accesses;}
        if(memory.stores!=stores){report["original_stores"]=stores;report["native_stores"]=memory.stores;}
    }catch(const std::exception& error){report["error"]=error.what();std::cerr<<error.what()<<'\n';}
    if(argc>=5){std::ofstream output(argv[4]);output<<report.dump(2)<<'\n';}
    std::cout<<J{{"passed",report.at("passed")},{"phase",report.value("phase","")},{"error",report.value("error","")},{"different_bytes",report.value("different_bytes",0u)}}.dump()<<'\n';
    return report.at("passed").get<bool>()?0:1;
}
