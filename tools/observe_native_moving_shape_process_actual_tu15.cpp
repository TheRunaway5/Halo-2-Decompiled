// Original-only ABI transport is provided by the authenticated offline adapter.
struct ProcessMemory:Memory {
    using Memory::Memory;
    static constexpr std::uint32_t private_data=0x06000000u;
    std::array<std::byte,4096>data{};bool track{};mutable J ledger=J::array();
    static bool owned(std::uint32_t p,std::size_t n){return p>=private_data&&std::uint64_t(p-private_data)+n<=4096;}
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        if(owned(p,b.size())){std::copy_n(data.begin()+p-private_data,b.size(),b.begin());return;}
        Memory::read(p,b);if(track)ledger.push_back(J::array({p,b.size()}));
    }
    void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        if(owned(p,b.size())){std::copy(b.begin(),b.end(),data.begin()+p-private_data);return;}
        Memory::write(p,b);
    }
};
std::string process_hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string out;for(auto value:b){const auto n=std::to_integer<unsigned>(value);out+=d[n>>4];out+=d[n&15];}return out;}
void start_process(Child&child,const char*exe,const J&input,unsigned poison){
    child.pid=fork();check(child.pid>=0,"Cannot fork actual PROCESS original");
    if(!child.pid){if(ptrace(PTRACE_TRACEME,0,nullptr,nullptr)<0)_exit(126);execl(exe,exe,nullptr);_exit(127);}
    child.wait();request(PTRACE_SETOPTIONS,child.pid,nullptr,reinterpret_cast<void*>(PTRACE_O_EXITKILL));request(PTRACE_CONT,child.pid);child.wait();check(WSTOPSIG(child.status)==SIGTRAP,"Actual PROCESS original setup failed");
    auto r=child.regs();r.rcx=input.at("agent").get<std::uint32_t>();r.rax=0x7ad92318u^poison;r.rdx=0x13579bdfu^poison;r.rbx=0x2468ace1u^poison;r.rbp=0x31415926u^poison;r.rsi=0x57721566u^poison;r.rdi=0x16180339u^poison;r.eflags=2;request(PTRACE_SETREGS,child.pid,nullptr,&r);
}
}
int main(int argc,char**argv){J report{{"passed",false},{"profiles",J::array()},{"captured_original_gp_abi",false},{"supplied_original_outcomes",false}};try{
    check(argc==5,"Supply original ELF/typed PROCESS/metadata/report");std::ifstream input(argv[2]),metadata_file(argv[3]);const auto capture=J::parse(input),metadata=J::parse(metadata_file);const auto&typed=capture.at("input");
    std::map<std::uint32_t,J>sites;for(const auto&s:metadata.at("sites"))sites.emplace(s.at("instruction"),s);
    for(unsigned profile=0;profile<4;++profile){
        Child original;start_process(original,argv[1],typed,profile);auto before_fp=original.fp();if(profile){before_fp.cwd=(before_fp.cwd&~0xc00u)|(profile<<10u);before_fp.mxcsr=(before_fp.mxcsr&~0x6000u)|(profile<<13u);request(PTRACE_SETFPREGS,original.pid,nullptr,&before_fp);}check(before_fp.ftw==0,"Actual PROCESS needs empty source x87");
        struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
        const auto retained=[&](std::uint32_t p,std::size_t n){return std::any_of(banks.begin(),banks.end(),[&](const auto&b){return p>=b.address&&std::uint64_t(p-b.address)+n<=b.before.size();});};
        J reads=J::array(),stores=J::array();original.step();unsigned instructions{};std::uint32_t stop_owner{};
        for(;;){const auto regs=original.regs();const auto pc=std::uint32_t(regs.rip);if(pc==0x5000000u)break;if(pc==0x309060u||pc==0x309150u||pc==0x2f8b60u){stop_owner=pc;break;}check(++instructions<4096,"Actual PROCESS exceeded authenticated bounded body");const auto found=sites.find(pc);check(found!=sites.end(),("Actual PROCESS escaped authenticated body at "+std::to_string(pc)).c_str());const auto registers=gp(regs);J pending=J::array();for(const auto&op:found->second.at("operands")){auto p=op.at("displacement").get<std::uint32_t>();const auto base=op.at("base").get<int>(),lane=op.at("index").get<int>();if(base>=0)p+=registers.at(base);if(lane>=0)p+=registers.at(lane)*op.at("scale").get<std::uint32_t>();const auto n=op.at("bytes").get<std::uint32_t>();if(!retained(p,n))continue;if(op.at("read").get<bool>())reads.push_back(J::array({p,n}));if(op.at("write").get<bool>())pending.push_back(J{{"instruction",pc},{"address",p},{"bytes",n},{"before",process_hex(original.read(p,n))}});}original.step();for(auto event:pending){event["written"]=process_hex(original.read(event.at("address"),event.at("bytes")));event.erase("bytes");stores.push_back(event);}}
        check(stop_owner!=0,"Actual PROCESS first child boundary absent");const auto boundary_regs=original.regs();const auto boundary_bytes=original.read(std::uint32_t(boundary_regs.rsp)+4u,stop_owner==0x2f8b60u?16u:12u);J boundary_args=J::array();for(unsigned n=0;n<boundary_bytes.size();n+=4){std::uint32_t value{};std::memcpy(&value,boundary_bytes.data()+n,4);boundary_args.push_back(value);}const auto after_fp=original.fp();for(auto&b:banks)b.after=original.read(b.address,b.before.size());
        J passes=J::array();
        for(bool read_pass:{false,true}){
            Child native;start_process(native,argv[1],typed,profile^0x55aa);for(const auto&b:banks)check(native.read(b.address,b.before.size())==b.before,"Independent actual PROCESS BEFORE differs");ProcessMemory memory(native);memory.data.fill(std::byte(profile*17u+0xa5u));memory.track=read_pass;J writes=J::array();SourcePhysicsContactRecordObserver observe;if(!read_pass)observe=[&](const auto&s){if(!ProcessMemory::owned(s.address,s.written.size()))writes.push_back(J{{"instruction",s.instruction},{"address",s.address},{"before",process_hex(s.before)},{"written",process_hex(s.written)}});};
            NativeTu15PhysicsFrameFp fp{before_fp.cwd,before_fp.swd,before_fp.mxcsr};NativeTu15MovingShapeProcessServices services;bool boundary_seen{};
            services.classify_shape=[&](auto,auto owner,auto&)->std::uint32_t{if(owner.identity==0xa9520u)return native_tu15_contact_child_class_23();if(owner.identity==0xab4b0u)return native_tu15_contact_child_class_24();const auto kind=native_tu15_activation_type(owner.identity);check(kind.has_value(),"Actual PROCESS unsupported class getter");return *kind;};
            const auto boundary=[&](std::uint32_t owner,std::uint32_t receiver,const J&args){check(owner==stop_owner&&receiver==std::uint32_t(boundary_regs.rcx)&&args==boundary_args,"Actual PROCESS real child DATA formals differ");boundary_seen=true;throw std::runtime_error("actual-process-child-boundary");};
            services.initialize_cache=[&](auto owner,auto cache,auto first,auto second,auto transform,auto&){boundary(owner,cache,J::array({first.identity,second.identity,transform}));};
            services.process_convex=[&](auto agent,auto first,auto second,auto config,auto output,auto&){boundary(0x2f8b60u,agent.identity,J::array({first.identity,second.identity,config.identity,output}));};
            NativeTu15MovingShapeProcessStats activity;try{native_tu15_process_moving_shape_contact(memory,{typed.at("agent")},{typed.at("first")},{typed.at("second")},{typed.at("configuration")},typed.at("output"),{ProcessMemory::private_data},fp,0,services,observe,&activity);check(false,"Actual PROCESS unexpectedly fullreturned");}catch(const std::runtime_error&e){if(std::string(e.what())!="actual-process-child-boundary")throw;}
            memory.track=false;check(boundary_seen&&activity.calls==1&&activity.completed==0&&activity.refusals==1,"Actual PROCESS bounded completion/activity differs");
            const auto&actual=read_pass?memory.ledger:writes;const auto&expected=read_pass?reads:stores;check(actual==expected,std::string("Native PROCESS ordered ")+(read_pass?"read":"store")+" differs "+actual.dump()+" / "+expected.dump());
            std::size_t compared{},different{};J first=J::array();for(const auto&b:banks){const auto actual_bytes=native.read(b.address,b.after.size());compared+=actual_bytes.size();for(std::size_t k=0;k<actual_bytes.size();++k)if(actual_bytes[k]!=b.after[k]){++different;if(first.size()<16)first.push_back(J{{"address",b.address+k},{"native",std::to_integer<unsigned>(actual_bytes[k])},{"original",std::to_integer<unsigned>(b.after[k])}});}}
            const auto actual_fp=J::array({fp.control_word,fp.status,fp.mxcsr}),expected_fp=fp_words(after_fp);const bool passed=different==0&&actual_fp==expected_fp;passes.push_back(J{{"passed",passed},{"read_pass",read_pass},{"compared_bytes",compared},{"captured_bytes_excluded",0},{"different_bytes",different},{"first_differences",first},{"original_fp",expected_fp},{"native_fp",actual_fp},{"classifications",activity.classifications},{"motion_vectors",activity.motion_vectors},{"cleared_vectors",activity.cleared_vectors}});check(passed,"Native actual PROCESS persistent bytes/FP differ");
        }
        report["profiles"].push_back(J{{"passed",true},{"actual_input_fp",profile==0},{"declared_rounding_control",profile},{"before_fp",fp_words(before_fp)},{"ordered_reads",reads.size()},{"ordered_stores",stores.size()},{"passes",passes},{"stop_owner",stop_owner},{"child_receiver",std::uint32_t(boundary_regs.rcx)},{"child_arguments",boundary_args}});
    }
    report["passed"]=true;
}catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';return report.at("passed").get<bool>()?0:1;
}
