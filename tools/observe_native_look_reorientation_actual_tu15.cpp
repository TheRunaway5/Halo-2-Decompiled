// Offline genuine typed18A2A0 comparison. The existing physical child/bank
// adapter is prepended. Original ABI assembly is confined to this oracle.
struct ReorientationMemory:Memory {
    using Memory::Memory;bool track{};mutable J ledger=J::array();
    void read(std::uint32_t p,std::span<std::byte>b)const override {
        Memory::read(p,b);if(track)ledger.push_back(J::array({p,b.size()}));
    }
};
std::string reorientation_hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string out;for(auto value:b){const auto n=std::to_integer<unsigned>(value);out+=d[n>>4];out+=d[n&15];}return out;}
void start_reorientation(Child&child,const char*exe,std::uint32_t index,std::uint32_t forward,unsigned poison){
    child.pid=fork();check(child.pid>=0,"Cannot fork reorientation original");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(exe,exe,nullptr);_exit(127);}
    child.wait();request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);child.wait();check(WSTOPSIG(child.status)==SIGTRAP,"Reorientation original setup failed");
    auto r=child.regs();r.rax=forward;r.rcx=index;r.rdx=0x13579bdfu^poison;r.rbx=0x2468ace1u^poison;r.rbp=0x31415926u^poison;r.rsi=0x57721566u^poison;r.rdi=0x16180339u^poison;r.eflags=2;request(PTRACE_SETREGS,child.pid,nullptr,&r);
}
}
int main(int argc,char**argv){J report{{"passed",false},{"profiles",J::array()},{"captured_original_gp_abi",false},{"supplied_original_outcomes",false}};try{
    check(argc==5,"Supply original ELF/typed reorientation/metadata/report");std::ifstream input(argv[2]),metadata_file(argv[3]);const auto capture=J::parse(input),metadata=J::parse(metadata_file);const auto&typed=capture.at("input");const auto index=typed.at("control_index").get<std::uint32_t>(),forward=typed.at("forward").get<std::uint32_t>();
    std::map<std::uint32_t,J>sites;for(const auto&s:metadata.at("sites"))sites.emplace(s.at("instruction"),s);
    for(unsigned profile=0;profile<4;++profile){
        Child original;start_reorientation(original,argv[1],index,forward,profile);auto before_fp=original.fp();if(profile){before_fp.cwd=(before_fp.cwd&~0xc00u)|(profile<<10u);before_fp.mxcsr=(before_fp.mxcsr&~0x6000u)|(profile<<13u);request(PTRACE_SETFPREGS,original.pid,nullptr,&before_fp);}check(before_fp.ftw==0,"Actual reorientation needs empty source x87");
        struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
        const auto retained=[&](std::uint32_t p,std::size_t n){return std::any_of(banks.begin(),banks.end(),[&](const auto&b){return p>=b.address&&std::uint64_t(p-b.address)+n<=b.before.size();});};
        J reads=J::array(),stores=J::array();original.step();unsigned instructions{};
        for(;;){const auto regs=original.regs();const auto pc=std::uint32_t(regs.rip);if(pc==0x5000000u)break;check(++instructions<64,"Reorientation original exceeded complete body");const auto found=sites.find(pc);check(found!=sites.end(),"Reorientation escaped authenticated body");const auto registers=gp(regs);J pending=J::array();for(const auto&op:found->second.at("operands")){auto p=op.at("displacement").get<std::uint32_t>();const auto base=op.at("base").get<int>(),lane=op.at("index").get<int>();if(base>=0)p+=registers.at(base);if(lane>=0)p+=registers.at(lane)*op.at("scale").get<std::uint32_t>();const auto n=op.at("bytes").get<std::uint32_t>();if(!retained(p,n))continue;if(op.at("read").get<bool>())reads.push_back(J::array({p,n}));if(op.at("write").get<bool>())pending.push_back(J{{"instruction",pc},{"address",p},{"bytes",n},{"before",reorientation_hex(original.read(p,n))}});}original.step();for(auto event:pending){event["written"]=reorientation_hex(original.read(event.at("address"),event.at("bytes")));event.erase("bytes");stores.push_back(event);}}
        check(std::uint32_t(original.regs().rsp)==0x0200f004u,"Original18A2A0 RET0 differs");const auto after_fp=original.fp();for(auto&b:banks)b.after=original.read(b.address,b.before.size());
        J passes=J::array();
        for(bool read_pass:{false,true}){
            Child native;start_reorientation(native,argv[1],index,forward,profile^0x55aa);for(const auto&b:banks)check(native.read(b.address,b.before.size())==b.before,"Independent actual BEFORE differs");ReorientationMemory memory(native);memory.track=read_pass;J writes=J::array();SourcePhysicsContactRecordObserver observe;if(!read_pass)observe=[&](const auto&s){writes.push_back(J{{"instruction",s.instruction},{"address",s.address},{"before",reorientation_hex(s.before)},{"written",reorientation_hex(s.written)}});};
            NativeTu15PhysicsFrameFp fp{before_fp.cwd,before_fp.swd,before_fp.mxcsr};NativeTu15LookReorientationBindingActivity activity;
            native_tu15_reorient_bound_look_controller(memory,index,forward,{fp.control_word,fp.status,fp.mxcsr,0},true,observe,&activity);memory.track=false;
            check(activity.attempts==1&&activity.completed==1&&activity.refusals==0&&activity.reorientation.calls==1&&activity.reorientation.completed==1&&activity.reorientation.refusals==0,"Native reorientation phase counts differ");
            const auto&actual=read_pass?memory.ledger:writes;const auto&expected=read_pass?reads:stores;check(actual==expected,std::string("Native reorientation ordered ")+(read_pass?"read":"store")+" ledger differs "+actual.dump()+" / "+expected.dump());
            std::size_t compared{},different{};J first=J::array();for(const auto&b:banks){const auto actual_bytes=native.read(b.address,b.after.size());compared+=actual_bytes.size();for(std::size_t k=0;k<actual_bytes.size();++k)if(actual_bytes[k]!=b.after[k]){++different;if(first.size()<16)first.push_back(J{{"address",b.address+k},{"native",std::to_integer<unsigned>(actual_bytes[k])},{"original",std::to_integer<unsigned>(b.after[k])}});}}
            const auto actual_fp=J::array({fp.control_word,fp.status,fp.mxcsr}),expected_fp=fp_words(after_fp);const bool passed=different==0&&actual_fp==expected_fp;passes.push_back(J{{"passed",passed},{"read_pass",read_pass},{"compared_bytes",compared},{"captured_bytes_excluded",0},{"different_bytes",different},{"first_differences",first},{"original_fp",expected_fp},{"native_fp",actual_fp},{"yaw_wraps",activity.reorientation.yaw_wraps}});check(passed,"Native actual reorientation persistent bytes/FP differ");
        }
        report["profiles"].push_back(J{{"passed",true},{"actual_input_fp",profile==0},{"declared_rounding_control",profile},{"index",index},{"forward",forward},{"before_fp",fp_words(before_fp)},{"ordered_reads",reads.size()},{"ordered_stores",stores.size()},{"passes",passes}});
    }
    report["passed"]=true;
}catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';return report.at("passed").get<bool>()?0:1;
}
