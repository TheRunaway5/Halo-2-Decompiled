#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_shadow_frame_tu15.hpp"
#include "halo2/source_shadow_native_prefix_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cfenv>
#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <thread>
#include <xmmintrin.h>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool value,const std::string &message) {
    if(!value)throw std::runtime_error(message);
}
std::vector<std::byte> hex(const std::string &text) {
    need(text.size()%2==0,"partial original fixture byte");
    std::vector<std::byte> out;out.reserve(text.size()/2);
    for(std::size_t i=0;i<text.size();i+=2)
        out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return out;
}
void patch(std::vector<std::byte> &data,const Json &patches) {
    for(const auto &p:patches) {
        const auto at=p.at("offset").get<std::size_t>();const auto value=hex(p.at("hex"));
        need(at<=data.size() && value.size()<=data.size()-at,"original fixture patch exceeds owner");
        std::copy(value.begin(),value.end(),data.begin()+at);
    }
}
std::array<std::byte,10> physical(const Json &state,unsigned index) {
    const auto mantissa=state.at("x87_physical").at(index).at(0).get<std::uint64_t>();
    const auto exponent=state.at("x87_physical").at(index).at(1).get<std::uint16_t>();
    std::array<std::byte,10> out{};
    for(unsigned i=0;i<8;++i)out[i]=std::byte(mantissa>>(i*8));
    out[8]=std::byte(exponent);out[9]=std::byte(exponent>>8);return out;
}
unsigned depth(const Json &state) {
    const auto tags=state.at("x87_tags").get<unsigned>();
    const auto top=(state.at("x87_status").get<unsigned>()>>11)&7u;
    unsigned count{};
    while(count<8 && ((tags>>(((top+count)&7u)*2))&3u)!=3u)++count;
    for(unsigned i=count;i<8;++i)
        need(((tags>>(((top+i)&7u)*2))&3u)==3u,"original fixture contains unsupported sparse logical x87 stack");
    return count;
}
SourcePhysicsWorldRuntimeAbi abi(const Json &state) {
    SourcePhysicsWorldRuntimeAbi out{};
    out.gp=state.at("gp").get<decltype(out.gp)>();out.eflags=state.at("eflags");
    out.fs_base=state.at("fs_base");out.x87_status=state.at("x87_status");
    out.x87_depth=depth(state);
    need(sizeof(long double)>=10,"original extended x87 fixture requires ten-byte values");
    const auto top=(out.x87_status>>11)&7u;
    for(unsigned i=0;i<out.x87_depth;++i) {
        const auto bytes=physical(state,(top+i)&7u);
        std::memcpy(&out.x87[i],bytes.data(),bytes.size());
    }
    out.floating_point=SourcePhysicsFactoryFloatingPoint{state.at("x87_control"),state.at("mxcsr")};
    for(unsigned i=0;i<8;++i) {
        const auto text=state.at("xmm").at(i).get<std::string>();
        need(text.size()==32,"partial original XMM fixture");
        for(unsigned lane=0;lane<4;++lane)
            out.xmm[i][lane]=std::stoul(text.substr(24-8*lane,8),nullptr,16);
    }
    return out;
}
struct Region {std::uint32_t address;std::vector<std::byte> initial,expected;};
struct Input {
    std::string name;
    SourcePhysicsWorldRuntimeAbi entry,expected;
    std::vector<Region> regions;
    std::vector<SourcePhysicsContactRecordStore> writes;
    std::vector<bool> committed;
    std::vector<std::uint32_t> pcs;
    bool fault{},hot{};
    std::array<std::uint64_t,3> prefix_entries{};
    std::uint32_t fault_pc{},fault_address{};
    std::size_t fault_size{};
    bool fault_write{};
};
Input input(const Json &value) {
    const auto &expected=value.at("expected");
    Input out{};out.name=value.at("name");out.entry=abi(value.at("entry"));
    out.expected=abi(expected.at("exit"));out.fault=!expected.at("completed").get<bool>();
    const auto &exact=expected.at("hot_exact_pc_counts");
    out.prefix_entries={exact.value("002e66b2",0u),exact.value("002e6810",0u),exact.value("002e6a50",0u)};
    out.hot=std::any_of(out.prefix_entries.begin(),out.prefix_entries.end(),[](auto n){return n>0;});
    out.pcs=value.at("original_instruction_pcs").get<decltype(out.pcs)>();
    for(const auto &region:value.at("initial_regions")) {
        std::vector<std::byte> bytes(region.at("size").get<std::size_t>(),
                                    std::byte(region.at("fill").get<unsigned>()));
        patch(bytes,region.at("patches"));
        out.regions.push_back({region.at("address"),bytes,bytes});
    }
    for(const auto &region:value.at("expected_changed_regions")) {
        const auto at=std::find_if(out.regions.begin(),out.regions.end(),[&](const auto &r){
            return r.address==region.at("address").get<std::uint32_t>();});
        need(at!=out.regions.end() && at->expected.size()==region.at("size").get<std::size_t>(),
             "original changed owner missing");
        patch(at->expected,region.at("patches_from_initial"));
    }
    for(const auto &event:value.at("original_ordered_accesses"))if(event.at("kind")=="write") {
        const auto size=event.at("size").get<unsigned>();
        need(size<=8,"original store requires an explicit width-normalization owner");
        SourcePhysicsContactRecordStore store{event.at("pc"),event.at("address"),
                                              hex(event.at("before")),{}};
        const auto bits=event.at("hook_value_low64").get<std::uint64_t>();
        for(unsigned i=0;i<size;++i)store.written.push_back(std::byte(bits>>(i*8)));
        need(store.before.size()==size,"original store preimage absent");
        out.writes.push_back(std::move(store));out.committed.push_back(event.at("next_instruction_observed"));
    }
    if(out.fault) {
        const auto &fault=expected.at("fault");need(!fault.is_null(),"original fault metadata absent");
        out.fault_pc=fault.at("abi").at("pc");out.fault_address=fault.at("address");
        out.fault_size=fault.at("size");const auto access=fault.at("access").get<unsigned>();
        need(access==19 || access==22,"unowned original fault access type");out.fault_write=access==22;
    }
    return out;
}
void mount(SourceRuntimeMemory &memory,const Input &in,const Json &value) {
    for(const auto &region:value.at("immutable_image_read_regions"))
        memory.mount(region.at("address"),hex(region.at("hex")),SourceRuntimeMemory::Access::ReadOnly,
                     "observed immutable original TU1.5 image input");
    const auto &readonly=value.at("initial_permissions").at("read_only_ranges");
    for(const auto &region:in.regions) {
        const auto end=std::uint64_t(region.address)+region.initial.size();
        std::vector<std::uint64_t> cuts{region.address,end};
        for(const auto &range:readonly) {
            const auto first=range.at("address").get<std::uint64_t>();
            const auto last=first+range.at("size").get<std::uint64_t>();
            if(first>region.address && first<end)cuts.push_back(first);
            if(last>region.address && last<end)cuts.push_back(last);
        }
        std::sort(cuts.begin(),cuts.end());cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());
        for(std::size_t i=1;i<cuts.size();++i) {
            bool protected_range=false;
            for(const auto &range:readonly) {
                const auto first=range.at("address").get<std::uint64_t>();
                const auto last=first+range.at("size").get<std::uint64_t>();
                protected_range|=cuts[i-1]>=first && cuts[i]<=last;
            }
            const auto offset=std::size_t(cuts[i-1]-region.address),size=std::size_t(cuts[i]-cuts[i-1]);
            memory.mount(std::uint32_t(cuts[i-1]),std::span(region.initial).subspan(offset,size),
                protected_range?SourceRuntimeMemory::Access::ReadOnly:SourceRuntimeMemory::Access::ReadWrite,
                "complete declared original standalone query workspace");
        }
    }
}
struct Failure {
    std::uint32_t pc{},address{};std::size_t size{};bool write{};
    bool operator==(const Failure &)const=default;
};
// The observer is a test of ordinary retained memory. It adds no source reads,
// callbacks, topology mutations, or FP work to the native capability admission.
struct Memory final:SourcePhysicsContactMemory {
    SourceRuntimeMemory bank;std::uint32_t &cursor;
    std::vector<SourcePhysicsContactRecordStore> writes;
    std::vector<bool> committed;
    mutable std::optional<Failure> failure;
    explicit Memory(std::uint32_t &pc):cursor(pc){}
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        try{bank.read(p,out);}catch(...){failure=Failure{cursor,p,out.size(),false};throw;}
    }
    void read_vector(std::uint32_t p,std::span<std::byte,16> out)const override {
        try{bank.read_vector(p,out);}catch(...){failure=Failure{cursor,p,out.size(),false};throw;}
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        SourcePhysicsContactRecordStore event{cursor,p,{},std::vector<std::byte>(in.begin(),in.end())};
        const auto before=bank.view(p,in.size());event.before.assign(before.begin(),before.end());
        writes.push_back(std::move(event));committed.push_back(false);
        try{bank.write(p,in);committed.back()=true;}
        catch(...){failure=Failure{cursor,p,in.size(),true};throw;}
    }
    std::optional<std::span<const std::byte>> try_read_view(std::uint32_t p,std::size_t size)const noexcept override {
        return bank.try_read_view(p,size);
    }
};
struct HostFp {
    unsigned mxcsr{};std::uint16_t control{},status{};
    bool operator==(const HostFp &)const=default;
};
HostFp host_fp() {
    HostFp out{};out.mxcsr=_mm_getcsr();
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(out.control),"=m"(out.status));
#else
    throw std::runtime_error("original x87 query test backend unavailable");
#endif
    return out;
}
void establish_caller_fp() {
    need(std::fesetenv(FE_DFL_ENV)==0,"establish masked test caller environment");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    // Local fesetenv restores IEEE exceptions but preserves current x87
    // condition codes. Pin those bits identically before each public call;
    // the original routine may change them in both native and literal modes.
    std::array<std::uint32_t,7> environment{};
    asm volatile("fnstenv %0":"=m"(environment)::"memory");
    environment[1]&=~0x4700u;
    asm volatile("fldenv %0"::"m"(environment):"memory");
#else
    throw std::runtime_error("original x87 caller setup backend unavailable");
#endif
}
struct Environment {
    std::fenv_t saved{};int error=errno;
    Environment(){need(std::fegetenv(&saved)==0,"save caller FP environment");}
    ~Environment(){std::fesetenv(&saved);errno=error;}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi value{};
    std::vector<SourcePhysicsContactRecordStore> writes,observed;
    std::vector<bool> committed;
    std::optional<Failure> failure;
    std::vector<std::uint32_t> pcs;
    SourceShadowNativePrefixTu15Stats stats{};
    HostFp host_before{},host_after{};
    int error{};std::uint32_t cursor{};
    double wall_seconds{},thread_cpu_seconds{};
    std::vector<std::vector<std::byte>> banks;
};
Result run(const Input &in,const Json &value,bool traced_literal,bool observer,bool measured=false) {
    Environment restore;std::uint32_t cursor{};Memory memory(cursor);mount(memory.bank,in,value);
    memory.writes.reserve(in.writes.size()+1);memory.committed.reserve(in.writes.size()+1);
    Result out;out.value=in.entry;out.pcs.reserve(in.pcs.size());out.observed.reserve(in.writes.size());
    SourcePhysicsWorldRuntimeServices services;
    bool first_trace=traced_literal,second_trace=false;
    const std::array<std::uint32_t,3> sites{0x38846e,0x21f590,0x33b595};
    services.instruction=[&](std::uint32_t pc){cursor=pc;if(traced_literal)out.pcs.push_back(pc);};
    services.instruction_cursor={&cursor,&first_trace,&second_trace,sites,std::this_thread::get_id(),
                                 &services.instruction.target_type(),true};
    services.required_owner=[](auto &,const auto &call,auto &,const auto &){
        std::ostringstream message;message<<"query reached an unqualified external owner at "
            <<std::hex<<call.instruction<<" -> "<<call.target;throw std::runtime_error(message.str());
    };
    SourcePhysicsContactRecordObserver observe;
    if(observer)observe=[&](const auto &store){out.observed.push_back(store);};
    establish_caller_fp();
    out.host_before=host_fp();errno=EDOM;
    const auto before=source_shadow_native_prefix_tu15_stats();
    std::chrono::steady_clock::time_point wall_before;
    timespec cpu_before{};
    if(measured) {
        need(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&cpu_before)==0,"read query thread CPU clock");
        wall_before=std::chrono::steady_clock::now();
    }
    try {
        source_run_shadow_frame_tu15(memory,0x2e79a0,out.value,*in.entry.floating_point,services,observe);
    } catch(const std::exception &) {
        // Only the declared natural memory fault may be caught. Missing source
        // callees, invalid fixture setup and unexpected diagnostics remain hard.
        need(memory.failure.has_value(),"query failed outside its declared memory owner");
    }
    out.error=errno;out.host_after=host_fp();
    if(measured) {
        const auto wall_after=std::chrono::steady_clock::now();timespec cpu_after{};
        need(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&cpu_after)==0,"read query thread CPU clock");
        out.wall_seconds=std::chrono::duration<double>(wall_after-wall_before).count();
        out.thread_cpu_seconds=double(cpu_after.tv_sec-cpu_before.tv_sec)+
                              double(cpu_after.tv_nsec-cpu_before.tv_nsec)*1e-9;
    }
    const auto after=source_shadow_native_prefix_tu15_stats();
    out.stats={after.attempts-before.attempts,after.admitted-before.admitted,
        after.declined_reference-before.declined_reference,after.declined_instruction-before.declined_instruction,
        after.declined_view-before.declined_view,after.declined_numeric-before.declined_numeric};
    out.failure=memory.failure;out.cursor=cursor;out.writes=std::move(memory.writes);
    out.committed=std::move(memory.committed);
    for(const auto &region:in.regions) {
        std::vector<std::byte> data(region.initial.size());memory.bank.read(region.address,data);
        out.banks.push_back(std::move(data));
    }
    return out;
}
bool same_x87(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    if(a.x87_depth!=b.x87_depth || a.active_mmx.has_value()!=b.active_mmx.has_value())return false;
    for(unsigned i=0;i<a.x87_depth;++i)if(std::memcmp(&a.x87[i],&b.x87[i],10)!=0)return false;
    return true;
}
bool restored_caller_controls(const Result &r) {
    return r.host_before.mxcsr==r.host_after.mxcsr && r.host_before.control==r.host_after.control &&
        ((r.host_before.status^r.host_after.status)&~0x4700u)==0;
}
void equal(const Result &candidate,const Result &literal,const std::string &name) {
    need(candidate.value.gp==literal.value.gp && candidate.value.xmm==literal.value.xmm &&
         candidate.value.eflags==literal.value.eflags && candidate.value.fs_base==literal.value.fs_base,
         name+": native/literal full GP/XMM/flags mismatch");
    need(same_x87(candidate.value,literal.value) && candidate.value.x87_status==literal.value.x87_status &&
         candidate.value.floating_point->x87_control_word==literal.value.floating_point->x87_control_word &&
         candidate.value.floating_point->mxcsr==literal.value.floating_point->mxcsr,
         name+": native/literal complete source FP mismatch");
    need(candidate.writes==literal.writes && candidate.committed==literal.committed &&
         candidate.observed==literal.observed && candidate.banks==literal.banks &&
         candidate.failure==literal.failure && candidate.cursor==literal.cursor,
         name+": native/literal memory/fault/observer/cursor prefix mismatch");
    // The public wrapper's local fesetenv restores controls/exception flags,
    // while x87 condition bits follow the literal routine. Both calls start
    // identically and must match their COMPLETE final status and source ABI.
    // The direct prefix separately requires full raw host fenv/errno equality.
    if(!(restored_caller_controls(candidate) && restored_caller_controls(literal) &&
         candidate.host_before==literal.host_before &&
         candidate.host_after==literal.host_after && candidate.error==literal.error)) {
        const auto show=[&](const char *role,const Result &r) {
            std::cerr<<name<<" "<<role<<std::hex<<" before(mxcsr,cw,sw)="<<r.host_before.mxcsr<<","
                <<r.host_before.control<<","<<r.host_before.status<<" after="<<r.host_after.mxcsr<<","
                <<r.host_after.control<<","<<r.host_after.status<<std::dec<<" errno="<<r.error<<'\n';
        };
        show("native",candidate);show("literal",literal);
    }
    need(restored_caller_controls(candidate) && restored_caller_controls(literal) &&
         candidate.host_before==literal.host_before &&
         candidate.host_after==literal.host_after && candidate.error==literal.error,
         name+": native/literal caller FP controls/status or errno changed");
}
std::string original_abi_difference(const SourcePhysicsWorldRuntimeAbi &actual,const Input &in,bool strict_x87_c1=false) {
    std::ostringstream difference;const auto &expected=in.expected;
    constexpr const char *names[]{"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
    for(unsigned i=0;i<8;++i)if(actual.gp[i]!=expected.gp[i])
        difference<<names[i]<<"="<<std::hex<<actual.gp[i]<<" expected="<<expected.gp[i]<<"; ";
    for(unsigned i=0;i<8;++i)if(actual.xmm[i]!=expected.xmm[i])difference<<"xmm"<<i<<"; ";
    // All arithmetic flags are recorded, including AF. No undefined-bit mask
    // silently weakens this whole-query numerical gate for these fixed cases.
    if(actual.eflags!=expected.eflags)difference<<"eflags="<<std::hex<<actual.eflags<<" expected="<<expected.eflags<<"; ";
    if(actual.fs_base!=expected.fs_base)difference<<"fs_base; ";
    if(!same_x87(actual,expected))difference<<"live x87/MMX; ";
    // The genuine2E6612 FSTP operand produces identical stored bits on this
    // CPU and Unicorn, but physical status0220 versus emulated0000. C1 and
    // sticky exceptions therefore lack emulator authority. Keep TOP/C0/C2/C3
    // and all numerical/control checks strict; full source status is always
    // compared native-versus-literal above. The explicit C1 probe stays red.
    const auto unqualified=0x3fu|(strict_x87_c1?0u:0x200u);
    if((actual.x87_status&~unqualified)!=(expected.x87_status&~unqualified))difference<<"x87 status="
        <<std::hex<<actual.x87_status<<" expected="<<expected.x87_status<<"; ";
    if(!actual.floating_point || actual.floating_point->x87_control_word!=expected.floating_point->x87_control_word ||
       (actual.floating_point->mxcsr&~0x3fu)!=(expected.floating_point->mxcsr&~0x3fu))difference<<"FP controls; ";
    return difference.str();
}
void original_memory(const Input &in,const Result &actual,bool observer) {
    need(actual.writes==in.writes && actual.committed==in.committed,
         in.name+": original ordered write attempts/partial commits differ");
    for(std::size_t i=0;i<in.regions.size();++i)
        need(actual.banks[i]==in.regions[i].expected,in.name+": original final workspace bytes differ");
    std::vector<SourcePhysicsContactRecordStore> committed;
    for(std::size_t i=0;i<in.writes.size();++i)if(in.committed[i])committed.push_back(in.writes[i]);
    need(actual.observed==(observer?committed:std::vector<SourcePhysicsContactRecordStore>{}),
         in.name+": committed source observer order/preimages differ");
    need(actual.failure.has_value()==in.fault,in.name+": original completion/fault boundary differs");
    if(in.fault)need(*actual.failure==Failure{in.fault_pc,in.fault_address,in.fault_size,in.fault_write},
                       in.name+": original fault PC/address/width/access differs");
    need(actual.cursor==in.pcs.back(),in.name+": original final/fault instruction cursor differs");
}
}
int main(int argc,char **argv)try {
    need(argc>=2,"provide the original whole-query fixture");
    bool require_native=false,expect_reference=false,require_original_fault_abi=false,full_corpus=false,
        diagnose_original_abi=false,require_original_x87_c1=false;
    unsigned repeat{};
    for(int i=2;i<argc;++i) {
        const std::string option=argv[i];
        if(option=="--require-native")require_native=true;
        else if(option=="--expect-reference")expect_reference=true;
        else if(option=="--require-original-fault-abi")require_original_fault_abi=true;
        else if(option=="--full-corpus")full_corpus=true;
        else if(option=="--diagnose-original-abi")diagnose_original_abi=true;
        else if(option=="--require-original-x87-c1")require_original_x87_c1=true;
        else if(option=="--repeat") {
            need(i+1<argc,"--repeat requires a bounded positive count");
            const std::string count=argv[++i];std::size_t consumed{};
            const auto number=std::stoul(count,&consumed);
            need(consumed==count.size() && number>0 && number<=10000,"--repeat must be1..10000");
            repeat=unsigned(number);
        }
        else throw std::runtime_error("unknown whole-query argument: "+option);
    }
    need(!(require_native && expect_reference),"conflicting native/reference admission requirements");
    std::ifstream file(argv[1]);need(bool(file),"original whole-query fixture absent");
    const auto fixture=Json::parse(file);
    need(fixture.at("schema")=="H2_TU15_ORIGINAL_WHOLE_PLANE_QUERY_V1" && fixture.at("entry")==0x2e79a0 &&
         fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
         fixture.at("input_fixture_sha256")=="1962360d46d6565d1c687b0444e66bb85d0cfc77de74ffcfccda438320cd2aae" &&
         fixture.at("cases").size()==(full_corpus?1403u:10u),"original TU1.5 query fixture identity/coverage");
    if(full_corpus)need(fixture.value("full_corpus",false) && fixture.at("expected_cases")==1403 &&
        fixture.at("normal_queries")==1398 && fixture.at("fault_queries")==5 &&
        fixture.at("authority_report_sha256")=="6cac7ea2f8a3db5c60dd3b6d8731185fd42a464dab0f4b780873b8bab3e97b91",
        "full original query corpus declaration differs");
    source_reset_shadow_native_prefix_tu15_stats();
    unsigned normal{},faults{},hot_cases{},native_hot_cases{},fault_abi_differences{},observer_pairs{},normal_abi_differences{};
    unsigned oracle_x87_c1_disagreements{};
    std::uint64_t qualified_admissions{},qualified_attempts{};
    std::array<std::uint64_t,3> expected_prefix_entries{};
    std::set<std::string> names;
    for(const auto &value:fixture.at("cases")) {
        const auto in=input(value);
        need(names.insert(in.name).second,"duplicate whole-query fixture case");
        for(const bool observer:{false,true}) {
            const auto literal=run(in,value,true,observer),candidate=run(in,value,false,observer);
            equal(candidate,literal,in.name);original_memory(in,candidate,observer);
            need(literal.pcs==in.pcs,in.name+": unchanged literal original instruction prefix differs");
            const auto difference=original_abi_difference(candidate.value,in,require_original_x87_c1);
            if(!observer && ((candidate.value.x87_status^in.expected.x87_status)&0x200u)) {
                ++oracle_x87_c1_disagreements;
                if(diagnose_original_abi || require_original_x87_c1)
                    std::cout<<"ORACLE_X87_C1_DISAGREEMENT case="<<in.name<<" actual="<<std::hex
                        <<candidate.value.x87_status<<" original_emulated="<<in.expected.x87_status<<std::dec<<'\n';
            }
            if(in.fault) {
                if(!observer && !difference.empty()) {
                    ++fault_abi_differences;
                    std::cout<<"ORIGINAL_FAULT_ABI_DIFFERENCE case="<<in.name<<" "<<difference<<'\n';
                }
            } else if(!difference.empty()) {
                if(!diagnose_original_abi)need(false,in.name+": original numerical ABI differs: "+difference);
                if(!observer) {
                    ++normal_abi_differences;
                    std::cout<<"ORIGINAL_NORMAL_ABI_DIFFERENCE case="<<in.name<<" "<<difference<<'\n';
                }
            }
            qualified_admissions+=candidate.stats.admitted;qualified_attempts+=candidate.stats.attempts;
            const auto expected_attempts=in.prefix_entries[0]+in.prefix_entries[1]+in.prefix_entries[2];
            need(candidate.stats.attempts==expected_attempts && literal.stats.attempts==expected_attempts,
                 in.name+": all-three original prefix entry coverage differs");
            if(require_native)need(candidate.stats.admitted==expected_attempts,
                in.name+": all-three ordinary prefix admissions are incomplete");
            if(expect_reference)need(candidate.stats.declined_reference==expected_attempts,
                in.name+": global reference policy did not refuse all three prefixes");
            for(unsigned i=0;i<3;++i)expected_prefix_entries[i]+=in.prefix_entries[i];
            if(!observer && in.hot) {
                ++hot_cases;native_hot_cases+=candidate.stats.admitted>0;
                std::cout<<"QUALIFIED_QUERY case="<<in.name<<" attempts="<<candidate.stats.attempts
                    <<" admitted="<<candidate.stats.admitted<<" reference="<<candidate.stats.declined_reference
                    <<" instruction="<<candidate.stats.declined_instruction<<" view="<<candidate.stats.declined_view
                    <<" numeric="<<candidate.stats.declined_numeric<<'\n';
            }
            if(observer)++observer_pairs;
        }
        if(in.fault)++faults;else ++normal;
    }
    const auto stats=source_shadow_native_prefix_tu15_stats();
    need(stats.attempts==stats.admitted+stats.declined_reference+stats.declined_instruction+
        stats.declined_view+stats.declined_numeric,"whole-query admission accounting incomplete");
    need(normal==(full_corpus?1398u:5u) && faults==5 && hot_cases>=2 && qualified_attempts>0,
         "whole-query coverage vanished");
    std::cout<<(normal_abi_differences?"DIFFERENTIAL_PASS original_normal=":"NUMERICAL_PASS original_normal=")
        <<normal<<" natural_fault_prefixes="<<faults
        <<" observer_pairs="<<observer_pairs<<" qualified_attempts="<<qualified_attempts
        <<" qualified_admitted="<<qualified_admissions<<" hot_cases="<<hot_cases
        <<" native_hot_cases="<<native_hot_cases<<" total_attempts="<<stats.attempts
        <<" total_admitted="<<stats.admitted<<" original_normal_abi_differences="<<normal_abi_differences
        <<" original_fault_abi_differences="<<fault_abi_differences
        <<" oracle_x87_c1_disagreements="<<oracle_x87_c1_disagreements<<'\n';
    std::cout<<"PREFIX_ENTRY_COVERAGE 2e66b2="<<expected_prefix_entries[0]<<" 2e6810="<<expected_prefix_entries[1]
        <<" 2e6a50="<<expected_prefix_entries[2]<<" qualified_attempts="<<qualified_attempts
        <<" qualified_admitted="<<qualified_admissions<<'\n';
    need(std::all_of(expected_prefix_entries.begin(),expected_prefix_entries.end(),[](auto n){return n>0;}),
         "all-three original prefix coverage absent");
    if(full_corpus)need(expected_prefix_entries==std::array<std::uint64_t,3>{5094,5122,5224},
        "full original all-three prefix entry totals differ");
    std::cout<<"ORACLE_LIMIT: emulated x87 C1/sticky flags lack physical authority; "
        "complete native/literal source status remains strict\n";
    if(fault_abi_differences)std::cout<<"ORIGINAL_FAULT_ABI_LIMITED: literal/native prefix parity is checked; "
        "full original fault-state acceptance remains open\n";
    if(expect_reference)need(qualified_admissions==0 && stats.admitted==0,"reference policy executed native prefix");
    if(require_native)need(native_hot_cases==hot_cases && qualified_admissions>0,
        "NATIVE_REQUIRED: every representative hot ordinary query must execute the native prefix");
    if(require_original_fault_abi)need(fault_abi_differences==0,
        "ORIGINAL_FAULT_ABI_REQUIRED: literal baseline does not match every original fault ABI");
    if(diagnose_original_abi)need(normal_abi_differences==0,
        "ORIGINAL_ABI_REQUIRED: differential passed, original normal ABI differences remain open");
    if(repeat) {
        double wall{},cpu{};std::uint64_t calls{},attempts{},admitted{};
        for(const auto &value:fixture.at("cases")) {
            const auto in=input(value);if(in.fault)continue;
            for(unsigned i=0;i<repeat;++i) {
                // Every call remounts its exact original banks and entry ABI.
                // Timing covers the public entry and raw host-state capture,
                // excluding reset, final-bank copies and assertions. Memory
                // write instrumentation remains present in both processes.
                const auto result=run(in,value,false,false,true);
                original_memory(in,result,false);
                need(original_abi_difference(result.value,in,require_original_x87_c1).empty(),in.name+": repeated original ABI differs");
                if(require_native && in.hot)need(result.stats.admitted>0,"repeated hot query did not admit native prefix");
                if(expect_reference)need(result.stats.admitted==0,"repeated reference query admitted native prefix");
                wall+=result.wall_seconds;cpu+=result.thread_cpu_seconds;++calls;
                attempts+=result.stats.attempts;admitted+=result.stats.admitted;
            }
        }
        std::cout<<"COMPONENT_TIMING calls="<<calls<<" repeat="<<repeat<<" wall_seconds="<<wall
            <<" thread_cpu_seconds="<<cpu<<" attempts="<<attempts<<" admitted="<<admitted
            <<" scope=instrumented_public_query_reset_excluded ordinary_fps=false\n";
    }
    return 0;
} catch(const std::exception &error) {
    std::cerr<<error.what()<<'\n';return 1;
}
