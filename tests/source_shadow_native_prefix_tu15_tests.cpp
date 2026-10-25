#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_shadow_native_prefix_tu15.hpp"
#include "rendering/shadows/source_shadow_native_prefix_tu15_internal.hpp"
#include "helpers/source_shadow_prefix_2e6810_reference_20261023.hpp"
#include "helpers/source_shadow_prefix_2e66b2_reference_20261023.hpp"
#include "helpers/source_shadow_prefix_2e6a50_reference_20261023.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cerrno>
#include <cfenv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <thread>

using namespace halo2;
using Json=nlohmann::json;
using State=physics_contact_runtime_detail::State;
namespace {
struct Prefix {
    std::uint32_t entry,last,resume;
    unsigned representative_contexts,full_contexts;
};
constexpr std::array<Prefix,3> prefixes{{
    {0x2e66b2,0x2e6711,0x2e6714,16,2523},
    {0x2e6810,0x2e686f,0x2e6872,128,128},
    {0x2e6a50,0x2e6aaf,0x2e6ab2,16,2431}}};
const Prefix *active_prefix=&prefixes[1];
bool run_native_prefix(State &state) {
    if(active_prefix->entry==0x2e66b2)return source_shadow_native_prefix_tu15_detail::run_2e66b2(state);
    if(active_prefix->entry==0x2e6a50)return source_shadow_native_prefix_tu15_detail::run_2e6a50(state);
    return source_shadow_native_prefix_tu15_detail::run_2e6810(state);
}
void run_literal_prefix(State &state) {
    if(active_prefix->entry==0x2e66b2)return test::shadow_prefix_2e66b2_reference(state);
    if(active_prefix->entry==0x2e6a50)return test::shadow_prefix_2e6a50_reference(state);
    test::shadow_prefix_2e6810_reference(state);
}
std::string phase="setup";
void need(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
struct Environment {
    std::fenv_t saved{};int error=errno;
    Environment(){need(std::fegetenv(&saved)==0,"save test FP environment");}
    ~Environment(){std::fesetenv(&saved);errno=error;}
};
std::vector<std::byte> hex(const std::string &text) {
    need(text.size()%2==0,"partial fixture byte");std::vector<std::byte> out;
    for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return out;
}
SourcePhysicsContactRuntimeAbi abi(const Json &value) {
    SourcePhysicsContactRuntimeAbi out{};out.gp=value.at("gp").get<decltype(out.gp)>();
    out.eflags=value.at("eflags");out.fs_base=value.at("fs_base");
    out.x87_status=value.at("x87_status");
    need(value.at("x87_tags")==65535,"prefix fixture has live unmodeled x87 slots");
    out.floating_point=SourcePhysicsFactoryFloatingPoint{value.at("x87_control"),value.at("mxcsr")};
    for(unsigned i=0;i<8;++i) {
        const auto text=value.at("xmm").at(i).get<std::string>();need(text.size()==32,"partial fixture XMM");
        for(unsigned lane=0;lane<4;++lane)out.xmm[i][lane]=std::stoul(text.substr(24-8*lane,8),nullptr,16);
    }
    return out;
}
struct Input {
    SourcePhysicsContactRuntimeAbi entry,expected;
    std::map<std::uint32_t,std::byte> bytes;
    std::array<std::uint32_t,9> addresses{};
};
Input input(const Json &context) {
    Input out{abi(context.at("entry")),abi(context.at("exit"))};
    const auto &reads=context.at("original_128_bit_reads");need(reads.size()==9,"prefix fixture operand coverage");
    unsigned ordinal{};
    for(const auto &read:reads) {
        const auto address=read.at("address").get<std::uint32_t>();out.addresses[ordinal++]=address;
        const auto data=hex(read.at("hex"));need(data.size()==16,"prefix fixture operand size");
        for(unsigned i=0;i<16;++i) {
            const auto [at,inserted]=out.bytes.emplace(address+i,data[i]);
            need(inserted || at->second==data[i],"fixture aliases have different bytes");
        }
    }
    return out;
}
void mount(SourceRuntimeMemory &bank,const Input &in,bool separate=false,int omit=-1) {
    if(separate) {
        std::set<std::uint32_t> done;
        for(unsigned i=0;i<9;++i)if(int(i)!=omit && done.insert(in.addresses[i]).second) {
            std::array<std::byte,16> bytes{};
            for(unsigned n=0;n<16;++n)bytes[n]=in.bytes.at(in.addresses[i]+n);
            bank.mount(in.addresses[i],bytes,SourceRuntimeMemory::Access::ReadWrite,"prefix operand");
        }
    } else {
        const auto first=in.bytes.begin()->first,last=in.bytes.rbegin()->first;
        need(std::uint64_t(last)-first<65536,"fixture operand span unexpectedly large");
        std::vector<std::byte> data(std::size_t(last-first)+1,std::byte{0xa5});
        for(const auto &[address,value]:in.bytes)data[address-first]=value;
        bank.mount(first,data,SourceRuntimeMemory::Access::ReadWrite,"completed prefix frame");
    }
}
// Explicit test-only capability adapter. Counts real bridge calls independently
// of the native helper's counters; it introduces no source callbacks or writes.
struct Memory final:SourcePhysicsContactMemory {
    SourceRuntimeMemory &bank;bool capable;
    mutable unsigned vectors{},capabilities{};
    mutable std::vector<std::uint32_t> reads;
    explicit Memory(SourceRuntimeMemory &b,bool c=true):bank(b),capable(c){reads.reserve(16);}
    void read(std::uint32_t p,std::span<std::byte> out)const override{bank.read(p,out);}
    void write(std::uint32_t p,std::span<const std::byte> in)override{bank.write(p,in);}
    void read_vector(std::uint32_t p,std::span<std::byte,16> out)const override {
        ++vectors;reads.push_back(p);bank.read_vector(p,out);
    }
    std::optional<std::span<const std::byte>> try_read_view(std::uint32_t p,std::size_t n)const noexcept override {
        ++capabilities;return capable?bank.try_read_view(p,n):std::nullopt;
    }
};
enum class Delivery {none,cursor,trace,second_trace,nonliteral,unknown,wrong_type,wrong_executor,other_sites};
struct Result {
    SourcePhysicsContactRuntimeAbi value{};
    std::fenv_t environment{};int error{};
    bool admitted{},failed{};
    unsigned vectors{},capabilities{};
    std::uint32_t cursor{};
    std::vector<std::uint32_t> callbacks,reads;
    std::vector<std::byte> alias_bank;
    std::uint64_t declined_numeric{};
};
Result run(const Input &in,bool candidate,Delivery delivery=Delivery::none,
    unsigned rounding=0,unsigned denormals=0,unsigned sticky=0,bool separate=false,int omit=-1,bool capable=true,
    std::optional<std::uint32_t> cursor_address=std::nullopt) {
    Environment restore;SourceRuntimeMemory bank;
    std::shared_ptr<std::vector<std::uint32_t>> words;
    std::uint32_t bank_first{};
    if(cursor_address) {
        need(!separate && omit<0,"cursor alias requires complete shared backing");
        bank_first=std::min(in.bytes.begin()->first,*cursor_address);
        const auto last=std::max(std::uint64_t(in.bytes.rbegin()->first),std::uint64_t(*cursor_address)+3);
        need(last-bank_first<65536 && (*cursor_address-bank_first)%4==0,"cursor alias backing extent/alignment");
        words=std::make_shared<std::vector<std::uint32_t>>((last-bank_first+4)/4,0xa5a5a5a5u);
        auto bytes=std::as_writable_bytes(std::span(*words));
        for(const auto &[address,value]:in.bytes)bytes[address-bank_first]=value;
        bank.mount(bank_first,std::make_shared<SourceRuntimeMemoryBacking>(words,bytes),
            SourceRuntimeMemory::Access::ReadWrite,"actual uint32 cursor/source backing");
    } else mount(bank,in,separate,omit);
    Memory memory(bank,capable);
    SourcePhysicsContactRuntimeServices services;
    std::vector<std::uint32_t> callbacks;callbacks.reserve(32);
    std::uint32_t cursor=0x12345678;
    auto *position=cursor_address?&words->at((*cursor_address-bank_first)/4):&cursor;
    *position=cursor;
    bool first=delivery==Delivery::trace,second=delivery==Delivery::second_trace;
    std::array<std::uint32_t,3> sites{0x38846e,0x21f590,0x33b595};
    if(delivery==Delivery::other_sites)sites[0]=active_prefix->entry;
    if(delivery!=Delivery::none)services.instruction=[&](std::uint32_t pc){
        *position=pc;
        if(delivery!=Delivery::cursor || first || second)callbacks.push_back(pc);
    };
    if(delivery!=Delivery::none && delivery!=Delivery::unknown) {
        services.instruction_cursor={position,&first,&second,sites,std::this_thread::get_id(),
            &services.instruction.target_type(),true};
        if(delivery==Delivery::wrong_executor)services.instruction_cursor.executor=std::thread::id{};
        if(delivery==Delivery::wrong_type)services.instruction_cursor.callback_type=&typeid(int);
        if(delivery==Delivery::nonliteral)services.instruction_cursor.literal_service_sites=false;
    }
    SourcePhysicsContactRecordObserver observer;
    State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    auto initial=in.entry;
    initial.floating_point=SourcePhysicsFactoryFloatingPoint{
        std::uint16_t(0x027f|(rounding<<10)),std::uint32_t(0x1f80|(rounding<<13)|denormals|sticky)};
    state.load(initial);errno=EDOM;
    Result out;
    const auto prior_numeric=source_shadow_native_prefix_tu15_stats().declined_numeric;
    try {
        if(candidate)out.admitted=run_native_prefix(state);
        if(!out.admitted)run_literal_prefix(state);
    } catch(const std::out_of_range &) {out.failed=true;}
    out.error=errno;need(std::fegetenv(&out.environment)==0,"capture complete host FP environment");
    out.value=state.save();out.vectors=memory.vectors;out.capabilities=memory.capabilities;
    out.cursor=*position;out.callbacks=std::move(callbacks);out.reads=std::move(memory.reads);
    if(words) {
        const auto bytes=std::as_bytes(std::span(*words));out.alias_bank.assign(bytes.begin(),bytes.end());
    }
    out.declined_numeric=source_shadow_native_prefix_tu15_stats().declined_numeric-prior_numeric;
    // No source writes are legal in this block, including refused admissions.
    for(const auto &[address,value]:in.bytes) {
        if(cursor_address && address>=*cursor_address && std::uint64_t(address)<std::uint64_t(*cursor_address)+4)continue;
        if(omit>=0 && address>=in.addresses[omit] && address<in.addresses[omit]+16)continue;
        need(bank.view(address,1)[0]==value,"prefix mutated source memory");
    }
    return out;
}
void equal(const Result &a,const Result &b) {
    need(a.failed==b.failed,"native/reference fault mismatch");
    if(a.value.gp!=b.value.gp || a.value.xmm!=b.value.xmm || a.value.eflags!=b.value.eflags) {
        std::cerr<<phase<<std::hex;
        for(unsigned r=0;r<8;++r) {
            if(a.value.gp[r]!=b.value.gp[r])std::cerr<<" gp"<<r<<"="<<a.value.gp[r]<<" reference="<<b.value.gp[r];
            for(unsigned l=0;l<4;++l)if(a.value.xmm[r][l]!=b.value.xmm[r][l])
                std::cerr<<" xmm"<<r<<"["<<l<<"]="<<a.value.xmm[r][l]<<" reference="<<b.value.xmm[r][l];
        }
        if(a.value.eflags!=b.value.eflags)std::cerr<<" flags="<<a.value.eflags<<" reference="<<b.value.eflags;
        std::cerr<<std::dec<<'\n';
    }
    need(a.value.gp==b.value.gp && a.value.xmm==b.value.xmm && a.value.eflags==b.value.eflags,
        "native/reference complete GP/XMM/flags mismatch");
    need(a.value.fs_base==b.value.fs_base && a.value.x87_depth==b.value.x87_depth &&
        a.value.x87_status==b.value.x87_status && a.value.active_mmx.has_value()==b.value.active_mmx.has_value(),
        "native/reference x87/MMX ownership changed");
    for(unsigned n=0;n<a.value.x87_depth;++n)
        need(std::memcmp(&a.value.x87[n],&b.value.x87[n],10)==0,"native/reference logical x87 value changed");
    if(a.value.active_mmx)need(a.value.active_mmx->physical==b.value.active_mmx->physical &&
        a.value.active_mmx->known_mask==b.value.active_mmx->known_mask,"native/reference MMX alias bytes changed");
    need(a.value.floating_point->x87_control_word==b.value.floating_point->x87_control_word &&
        a.value.floating_point->mxcsr==b.value.floating_point->mxcsr,
        "native/reference source FP mismatch");
    need(a.error==b.error && std::memcmp(&a.environment,&b.environment,sizeof(a.environment))==0,
        "native/reference complete host FP/errno mismatch");
    need(a.cursor==b.cursor && a.callbacks==b.callbacks,"native/reference instruction delivery mismatch");
    need(a.alias_bank==b.alias_bank,"native/reference cursor/source backing mismatch");
}
}
int main(int argc,char **argv)try {
    need(argc>=2,"provide original prefix corpus");
    bool require_native=false,expect_reference=false,quick=false,full_corpus=false;
    for(int i=2;i<argc;++i) {
        const std::string arg=argv[i];
        if(arg=="--require-native")require_native=true;
        else if(arg=="--expect-reference")expect_reference=true;
        else if(arg=="--quick")quick=true;
        else if(arg=="--full-corpus")full_corpus=true;
        else throw std::runtime_error("unknown prefix test argument");
    }
    std::ifstream file(argv[1]);Json corpus;file>>corpus;
    const auto found=std::find_if(prefixes.begin(),prefixes.end(),[&](const auto &prefix){return prefix.entry==corpus.at("entry");});
    need(found!=prefixes.end(),"unqualified original prefix entry");active_prefix=&*found;
    need(corpus.at("schema")=="H2_TU15_ORIGINAL_PLANE_PREFIX_V1" &&
        corpus.at("contexts").size()==(full_corpus?active_prefix->full_contexts:active_prefix->representative_contexts) &&
        corpus.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        corpus.at("last_pc")==active_prefix->last && corpus.at("resume_pc")==active_prefix->resume,
        "original prefix corpus identity/coverage");
    source_reset_shadow_native_prefix_tu15_stats();
    const auto admission=[&](const Result &trial) {
        if(expect_reference)need(!trial.admitted,"reference policy admitted native prefix");
        else if(require_native)need(trial.admitted && trial.vectors==0 && trial.capabilities==3,
            "ordinary qualified cursor failed native admission");
    };
    std::uint64_t cases{},admitted{},bridges_removed{},numeric_declines{};unsigned accepted{},rejected{};
    for(const auto &context:corpus.at("contexts")) {
        phase="original "+context.at("case").dump();
        const auto in=input(context);const auto literal=run(in,false),native=run(in,true);
        equal(native,literal);
        admission(native);
        need(native.value.gp==in.expected.gp && native.value.xmm==in.expected.xmm &&
            native.value.eflags==in.expected.eflags,"original TU1.5 numerical mismatch");
        need(literal.vectors==9,"literal control did not execute nine real vector bridges");
        if(native.admitted) {
            ++admitted;need(native.vectors==0 && native.capabilities==3,"native bridge reduction was not real");
            bridges_removed+=literal.vectors;
        }
        ++cases;if(native.value.eflags&0x40)++accepted;else ++rejected;
        if(!quick)for(unsigned rounding=0;rounding<4;++rounding)
            for(const auto denormals:{0u,0x40u,0x8000u,0x8040u})for(const auto sticky:{0u,0x20u}) {
                phase="FP "+context.at("case").dump()+" rounding="+std::to_string(rounding)+
                    " denormals="+std::to_string(denormals)+" sticky="+std::to_string(sticky);
                const auto control=run(in,false,Delivery::cursor,rounding,denormals,sticky);
                const auto trial=run(in,true,Delivery::cursor,rounding,denormals,sticky);
                admission(trial);
                equal(trial,control);++cases;admitted+=trial.admitted;
                if(trial.admitted){need(trial.vectors==0 && control.vectors==9,"cursor route bridge reduction");bridges_removed+=9;}
            }
    }
    const auto sample=input(corpus.at("contexts").front());
    // Cursor publication is an observable uint32 write even without callbacks.
    // A caller can place that object in the source backing; retain every
    // literal PC write before its operand load, including all three spans.
    for(const auto address:sample.addresses) {
        phase="cursor/source overlap="+std::to_string(address);
        const auto control=run(sample,false,Delivery::cursor,0,0,0,false,-1,true,address);
        const auto trial=run(sample,true,Delivery::cursor,0,0,0,false,-1,true,address);
        equal(trial,control);
        need(!trial.admitted && trial.vectors==9 && trial.reads==control.reads,
            "cursor/source alias bypassed ordered literal execution");++cases;
    }
    for(const auto boundary:{0xffffffc0u,0xffffffe0u,0xfffffff8u}) {
        auto changed=sample;changed.entry.gp[4]=boundary;
        phase="wrapped stack boundary="+std::to_string(boundary);
        const auto control=run(changed,false,Delivery::cursor),trial=run(changed,true,Delivery::cursor);
        equal(trial,control);need(trial.failed && !trial.admitted,"wrapped stack changed original failure prefix");++cases;
        changed=sample;changed.entry.gp[2]=boundary;
        phase="wrapped point boundary="+std::to_string(boundary);
        const auto point_control=run(changed,false,Delivery::cursor),point_trial=run(changed,true,Delivery::cursor);
        equal(point_trial,point_control);need(point_trial.failed && !point_trial.admitted,"wrapped point changed original failure prefix");++cases;
    }
    // Bit patterns exercise operand order, NaN payload propagation, signed
    // zero, underflow, overflow and denormal handling beyond retail inputs.
    // The frozen literal block is the host FP-state control; original Unicorn
    // sticky flags do not stand in for the physical processor's MXCSR.
    if(!quick) {
        constexpr std::array<std::uint32_t,16> patterns{
            0x00000000,0x80000000,0x00000001,0x80000001,
            0x007fffff,0x807fffff,0x00800000,0x80800000,
            0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,
            0x7fc12345,0xffc54321,0x7f812345,0xff854321};
        for(unsigned seed=0;seed<patterns.size();++seed) {
            auto edge=sample;
            for(unsigned operand=0;operand<9;++operand)for(unsigned lane=0;lane<4;++lane) {
                const auto bits=patterns[(seed+operand*3+lane)%patterns.size()];
                for(unsigned n=0;n<4;++n)edge.bytes[edge.addresses[operand]+lane*4+n]=std::byte(bits>>(n*8));
            }
            for(unsigned reg=0;reg<8;++reg)for(unsigned lane=0;lane<4;++lane)
                edge.entry.xmm[reg][lane]=patterns[(seed+reg*5+lane*3)%patterns.size()];
            for(unsigned rounding=0;rounding<4;++rounding)
                for(const auto denormals:{0u,0x40u,0x8000u,0x8040u})for(const auto sticky:{0u,0x20u}) {
                    phase="edge seed="+std::to_string(seed)+" rounding="+std::to_string(rounding)+
                        " denormals="+std::to_string(denormals)+" sticky="+std::to_string(sticky);
                    const auto control=run(edge,false,Delivery::cursor,rounding,denormals,sticky);
                    const auto trial=run(edge,true,Delivery::cursor,rounding,denormals,sticky);
                    bool nan=false;
                    for(const auto reg:{0u,1u,5u,7u})for(const auto bits:control.value.xmm[reg])
                        nan|=(bits&0x7fffffffu)>0x7f800000u;
                    if(nan) {
                        need(!trial.admitted && trial.vectors==control.vectors,
                            "NaN result did not retain literal vector reads");
                        if(require_native && !expect_reference)
                            need(trial.declined_numeric==1,"NaN result did not take the explicit numeric decline");
                    } else admission(trial);
                    numeric_declines+=trial.declined_numeric;
                    equal(trial,control);++cases;admitted+=trial.admitted;
                    if(trial.admitted){need(trial.vectors==0 && control.vectors==9,"edge route bridge reduction");bridges_removed+=9;}
                }
        }
    }
    // Every later missing operand must retain the original partial SIMD state,
    // instruction cursor and attempted-read prefix after admission declines.
    for(int omit=0;omit<9;++omit) {
        phase="omitted operand="+std::to_string(omit);
        const auto control=run(sample,false,Delivery::cursor,0,0,0,true,omit);
        const auto trial=run(sample,true,Delivery::cursor,0,0,0,true,omit);
        equal(trial,control);need(trial.failed && !trial.admitted && trial.reads==control.reads,
            "missing-operand decline changed original failure prefix");++cases;
    }
    for(const auto mode:{Delivery::cursor,Delivery::trace,Delivery::second_trace,Delivery::nonliteral,Delivery::unknown,Delivery::wrong_type,
            Delivery::wrong_executor,Delivery::other_sites}) {
        phase="instruction delivery="+std::to_string(unsigned(mode));
        const auto control=run(sample,false,mode),trial=run(sample,true,mode);equal(trial,control);++cases;
        if(mode==Delivery::cursor)admission(trial);
        if(mode==Delivery::cursor && trial.admitted){++admitted;bridges_removed+=9;}
        if(mode!=Delivery::cursor)need(!trial.admitted,"observable instruction delivery was bypassed");
    }
    const auto split=run(sample,true,Delivery::cursor,0,0,0,true);
    equal(split,run(sample,false,Delivery::cursor,0,0,0,true));need(!split.admitted,"cross-region capability admitted");
    const auto custom=run(sample,true,Delivery::cursor,0,0,0,false,-1,false);
    equal(custom,run(sample,false,Delivery::cursor,0,0,0,false,-1,false));
    need(!custom.admitted,"generic memory without a capability admitted");cases+=2;
    auto live_x87=sample;
    phase="live logical x87";
    live_x87.entry.x87_depth=4;live_x87.entry.x87_status=0x2001;
    live_x87.entry.x87={1.0L,-0.0L,-13.25L,0x1p-1000L};
    const auto live_control=run(live_x87,false,Delivery::cursor);
    const auto live_trial=run(live_x87,true,Delivery::cursor);
    admission(live_trial);
    equal(live_trial,live_control);++cases;admitted+=live_trial.admitted;
    if(live_trial.admitted)bridges_removed+=9;
    {
        phase="unmasked live MXCSR refusal";
        Environment restore;SourceRuntimeMemory bank;mount(bank,sample);Memory memory(bank);
        SourcePhysicsContactRuntimeServices services;SourcePhysicsContactRecordObserver observer;
        State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(sample.entry);
        const auto masked=_mm_getcsr();_mm_setcsr(masked&~0x80u);
        Result before,after;before.value=state.save();errno=EDOM;before.error=errno;
        need(std::fegetenv(&before.environment)==0,"capture unmasked environment before refusal");
        const auto numeric_before=source_shadow_native_prefix_tu15_stats().declined_numeric;
        const auto used=run_native_prefix(state);
        after.error=errno;need(std::fegetenv(&after.environment)==0,"capture unmasked environment after refusal");
        after.value=state.save();_mm_setcsr(masked);
        need(!used && memory.vectors==0,"unmasked source FP executed native arithmetic or literal reads");
        const auto declined=source_shadow_native_prefix_tu15_stats().declined_numeric-numeric_before;
        if(require_native && !expect_reference)need(declined==1,"unmasked source FP missed numeric refusal");
        equal(after,before);numeric_declines+=declined;++cases;
    }
    const auto stats=source_shadow_native_prefix_tu15_stats();
    need(stats.attempts==stats.admitted+stats.declined_reference+stats.declined_instruction+stats.declined_view+stats.declined_numeric,
        "native admission accounting incomplete");
    need(stats.declined_numeric==numeric_declines,"numeric decline accounting incomplete");
    need(stats.admitted==admitted,"public native admission counter disagrees with executed helper");
    std::cout<<"PREFIX_STATS attempts="<<stats.attempts<<" admitted="<<stats.admitted
        <<" declined_reference="<<stats.declined_reference<<" declined_instruction="<<stats.declined_instruction
        <<" declined_view="<<stats.declined_view<<" declined_numeric="<<stats.declined_numeric<<'\n';
    need(accepted && rejected,"original branch outcome coverage absent");
    std::cout<<"NUMERICAL_PASS contexts="<<corpus.at("contexts").size()<<" cases="<<cases<<" admitted="<<admitted
        <<" vector_bridges_removed="<<bridges_removed<<" numeric_declines="<<numeric_declines
        <<" accept="<<accepted<<" reject="<<rejected<<'\n';
    if(expect_reference)need(admitted==0,"reference mode executed native kernel");
    if(require_native)need(admitted>0 && bridges_removed>0,"NATIVE_REQUIRED: reference policy has zero native admissions");
    return 0;
}catch(const std::exception &error){std::cerr<<phase<<": "<<error.what()<<'\n';return 1;}
