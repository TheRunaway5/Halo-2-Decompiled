#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#ifdef HALO2_NATIVE_NEXT_EVENT_COMPILED_CALLERS
#include "halo2/death_cycle_runtime_tu15.hpp"
#endif
#include <algorithm>
#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>
using namespace halo2;using Json=nlohmann::json;
namespace {
constexpr std::uint32_t stack=0x2010000;
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s) {
    std::vector<std::byte> bytes;for(unsigned n=0;n<s.size();n+=2)bytes.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return bytes;
}
std::array<std::uint32_t,4> lanes(const Json &s) {
    const auto bytes=unhex(s);std::array<std::uint32_t,4> result{};need(bytes.size()==16,"Invalid XMM reference");
    std::memcpy(result.data(),bytes.data(),16);return result;
}
void load_fp(SourcePhysicsWorldRuntimeAbi &a,const Json &fp) {
    a.floating_point=SourcePhysicsFactoryFloatingPoint{fp.at("cw"),fp.at("mxcsr")};a.x87_status=fp.at("status");a.x87_depth=fp.at("stack").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto raw=unhex(fp.at("stack")[n]);need(raw.size()==10,"Invalid extended reference");std::memcpy(&a.x87[n],raw.data(),10);}
    for(unsigned n=0;n<8;++n)a.xmm[n]=lanes(fp.at("xmm")[n]);
}
void check_fp(const SourcePhysicsWorldRuntimeAbi &a,const Json &fp) {
    need(a.floating_point && a.floating_point->x87_control_word==fp.at("cw") && a.floating_point->mxcsr==fp.at("mxcsr"),"Different original FP controls/sticky flags");
    if(a.x87_depth!=fp.at("stack").size() || a.x87_status!=fp.at("status"))throw std::runtime_error("Different original x87 depth/status: actual "+std::to_string(a.x87_depth)+"/"+std::to_string(a.x87_status)+" expected "+std::to_string(fp.at("stack").size())+"/"+std::to_string(fp.at("status").get<unsigned>()));
    for(unsigned n=0;n<a.x87_depth;++n){const auto raw=unhex(fp.at("stack")[n]);need(!std::memcmp(&a.x87[n],raw.data(),10),"Different original live extended lane");}
    for(unsigned n=0;n<8;++n)need(a.xmm[n]==lanes(fp.at("xmm")[n]),"Different original XMM");
}
void check_abi(const SourcePhysicsWorldRuntimeAbi &a,const Json &expected,std::uint32_t mask) {
    if(a.gp!=expected.at("gp").get<decltype(a.gp)>())throw std::runtime_error("Different original GP: actual "+Json(a.gp).dump()+" expected "+expected.at("gp").dump());
    need(!((a.eflags^expected.at("eflags").get<std::uint32_t>())&mask),"Different defined original flags");check_fp(a,expected.at("fp"));
}
void check_stack(const SourcePhysicsContactMemory &m,const Json &raw) {
    const auto expected=unhex(raw);std::vector<std::byte> actual(expected.size());m.read(stack-64,actual);need(actual==expected,"Different original live/dead stack");
}
#ifndef HALO2_NATIVE_NEXT_EVENT_COMPILED_CALLERS
struct InjectedAccessStop {};

class FrontierMemory final : public SourcePhysicsContactMemory {
    sound_test::Memory &backing;
    unsigned stop;
    std::function<void()> completed;
    mutable unsigned accesses{};

    void begin_access() const {
        if (++accesses == stop) throw InjectedAccessStop{};
    }

public:
    FrontierMemory(sound_test::Memory &backing, unsigned stop, std::function<void()> complete)
        : backing(backing), stop(stop), completed(std::move(complete)) {}

    void read(std::uint32_t address,std::span<std::byte> bytes) const override {
        begin_access();
        backing.read(address,bytes);
        if (completed) completed();
    }

    void write(std::uint32_t address,std::span<const std::byte> bytes) override {
        begin_access();
        backing.write(address,bytes);
        if (completed) completed();
    }
};

void check_memory(const sound_test::Memory &actual,const sound_test::Memory &expected) {
    need(actual.pages.size()==expected.pages.size(),"Different next-event memory extent");
    for (const auto &[page,bytes]:actual.pages)
        need(*bytes==*expected.pages.at(page),"Different next-event memory page");
}

std::string replay(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,
                   const SourcePhysicsWorldRuntimeServices &services) {
    try {
        native_weapon_effect_dispatch_tu15_run(memory,0x17c020u,abi,services);
    } catch(const std::out_of_range &error) {
        return error.what();
    }
    return {};
}

void check_same_fault_state(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    need(a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.fs_base==b.fs_base &&
         a.x87_depth==b.x87_depth && a.x87_status==b.x87_status && !a.active_mmx && !b.active_mmx &&
         a.floating_point && b.floating_point && a.floating_point->mxcsr==b.floating_point->mxcsr &&
         a.floating_point->x87_control_word==b.floating_point->x87_control_word,
         "Different next-event register or FP state at a rejected access");
    for (unsigned i=0;i<a.x87_depth;++i)
        need(!std::memcmp(&a.x87[i],&b.x87[i],10),"Different next-event live x87 input at a rejected access");
}

void verify_native_scope(const Json &row,const sound_test::Memory &initial,const sound_test::Memory &expected_memory,
                         const SourcePhysicsWorldRuntimeAbi &before,const std::string &error,
                         unsigned &resumes,unsigned &rejected) {
    auto quiet=initial.clone();auto quiet_abi=before;
    SourcePhysicsWorldRuntimeServices silent;
    need(replay(quiet,quiet_abi,silent)==error,"Different unobserved next-event fault");
    check_abi(quiet_abi,row.at("expected"),row.at("flags_mask"));
    check_stack(quiet,row.at("expected").at("stack"));
    need(quiet_abi.fs_base==before.fs_base && !quiet_abi.active_mmx,"Unobserved next event changed FS/MMX");
    check_memory(quiet,expected_memory);

    const auto kind=row.at("kind").get<std::string>();
    if (kind=="base" && row.at("n").get<unsigned>()>=18u) return;
    auto traced_memory=initial.clone();auto traced_abi=before;
    SourcePhysicsWorldRuntimeServices traced;
    std::vector<std::uint32_t> trace;
    std::vector<std::size_t> frontiers;
    traced.instruction=[&](auto pc){trace.push_back(pc);};
    FrontierMemory traced_access(traced_memory,0,[&]{frontiers.push_back(trace.size());});
    need(replay(traced_access,traced_abi,traced)==error,"Different traced next-event fault");
    SourcePhysicsWorldRuntimeServices forced_reference;
    forced_reference.instruction=[](auto){};

    for (unsigned ordinal=0;ordinal<frontiers.size();++ordinal) {
        auto reopened_memory=initial.clone();auto reopened_abi=before;
        SourcePhysicsWorldRuntimeServices reopened;
        std::vector<std::uint32_t> suffix;
        unsigned accesses{};
        FrontierMemory reopened_access(reopened_memory,0,[&]{
            if (accesses++==ordinal) reopened.instruction=[&](auto pc){suffix.push_back(pc);};
        });
        need(replay(reopened_access,reopened_abi,reopened)==error,"Different next-event fault after reopen");
        if (!std::ranges::equal(suffix,std::span(trace).subspan(frontiers[ordinal])))
            throw std::runtime_error("Different next-event trace successor at completed access "+std::to_string(ordinal+1));
        check_abi(reopened_abi,row.at("expected"),row.at("flags_mask"));
        check_memory(reopened_memory,expected_memory);
        ++resumes;

        auto stopped_traced=initial.clone(),stopped_silent=initial.clone();
        auto traced_state=before,silent_state=before;
        FrontierMemory traced_stop(stopped_traced,ordinal+1,{}),silent_stop(stopped_silent,ordinal+1,{});
        try {replay(traced_stop,traced_state,forced_reference);need(false,"Traced next-event accepted a rejected access");}
        catch(const InjectedAccessStop &) {}
        try {replay(silent_stop,silent_state,silent);need(false,"Native next-event accepted a rejected access");}
        catch(const InjectedAccessStop &) {}
        check_same_fault_state(traced_state,silent_state);
        check_memory(stopped_traced,stopped_silent);
        ++rejected;
    }
}
#endif
}
int main(int argc,char **argv)try {
#ifdef HALO2_NATIVE_NEXT_EVENT_COMPILED_CALLERS
    need(argc==3,"Supply original effect next-event reference and caller module");const std::string module=argv[2];
    need(module=="death","Unknown compiled effect next-event caller");
#else
    need(argc==2,"Supply original effect next-event reference");
#endif
    std::ifstream in(argv[1]);const auto reference=Json::parse(in);
    need(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    need(reference.at("schema")=="H2_TU15_WEAPON_EFFECT_NEXT_EVENT_REFERENCE_V1","Wrong next-event reference schema");
    unsigned cases{},called_total{},stops{},aliases{};
#ifndef HALO2_NATIVE_NEXT_EVENT_COMPILED_CALLERS
    unsigned resumed{},rejected{};
#endif
    for(const auto &row:reference.at("rows"))try {
        const auto entry=row.at("entry").get<std::uint32_t>();
#ifdef HALO2_NATIVE_NEXT_EVENT_COMPILED_CALLERS
        need(entry==0x17c020u,"Advance fixture has another native entry");
#endif
        sound_test::Memory m;m.map(stack-4096,8192);m.write(stack-64,unhex(row.at("initial_stack")));
        for(const auto &v:row.at("memory")) {
            const auto at=v[0].get<std::uint32_t>(),value=v[1].get<std::uint32_t>();
            if(v[2]==1)m.put8(at,value);else if(v[2]==2)m.put16(at,value);else m.put32(at,value);
        }
        SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=row.at("eflags");a.fs_base=0x76543210;load_fp(a,row.at("fp"));
        const auto before=a;auto initial=m.clone(),expected_memory=m.clone();unsigned stored{},called{},traced{};
        const auto trace=unhex(row.at("trace_offsets"));
        SourcePhysicsContactRecordObserver observe=[&](const auto &event) {
            need(stored<row.at("stores").size(),"Different original state store count");const auto &expected=row.at("stores").at(stored++);
            std::uint32_t value{};for(unsigned n=0;n<event.written.size();++n)value|=std::to_integer<std::uint32_t>(event.written[n])<<(8*n);
            need(event.instruction==expected[0] && event.address==expected[1] && event.written.size()==expected[2] && value==expected[3],"Different original state store");
            std::vector<std::byte> prior(event.written.size());expected_memory.read(event.address,prior);need(prior==event.before,"Different original store preimage");expected_memory.write(event.address,event.written);
        };
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc) {
            need(traced+2<=trace.size(),"Different original trace extent");const auto offset=std::to_integer<unsigned>(trace[traced])|(std::to_integer<unsigned>(trace[traced+1])<<8u);traced+=2;
            need(pc==row.at("trace_base").get<std::uint32_t>()+offset,"Different original effect next-event instruction trace");
        };
        services.required_owner=[&](auto &,const auto &call,auto &abi,const auto &) {
            need(called<row.at("calls").size(),"Different actual lower call count");const auto &expected=row.at("calls").at(called++);
            need(!call.tail && call.target==expected.at("target") && call.instruction==m.get32(abi.gp[4])-5u,"Different actual lower call/return ABI");
            check_abi(abi,expected.at("before"),expected.at("flags_mask"));check_stack(m,expected.at("before").at("stack"));
            const auto &returned=expected.at("returned");abi.gp=returned.at("gp").template get<decltype(abi.gp)>();abi.eflags=returned.at("eflags");load_fp(abi,returned.at("fp"));++called_total;
        };
        bool stopped{};std::string error;
        try {
#ifdef HALO2_NATIVE_NEXT_EVENT_COMPILED_CALLERS
            source_run_death_cycle_runtime_tu15(m,entry,a,*a.floating_point,services,observe);
#else
            native_weapon_effect_dispatch_tu15_run(m,entry,a,services,observe);
#endif
        }catch(const std::out_of_range &e){stopped=true;error=e.what();}
        need(stopped==!row.at("fault").empty(),"Different original effect next-event read-stop outcome");
        if(stopped) {
            need(error.starts_with("unmapped ") && std::stoul(error.substr(9),nullptr,16)==row.at("fault")[0].at("address").get<std::uint32_t>(),
                "Different original effect next-event read-stop address");++stops;
        }
        need(stored==row.at("stores").size() && called==row.at("calls").size() && traced==trace.size(),"Different final original store/call/trace count");
        check_abi(a,row.at("expected"),row.at("flags_mask"));check_stack(m,row.at("expected").at("stack"));
        need(a.fs_base==before.fs_base && !a.active_mmx,"Unrelated source FS/MMX changed");
        for(const auto &[page,bytes]:m.pages)need(*bytes==*expected_memory.pages.at(page),"Different final original memory page");
#ifndef HALO2_NATIVE_NEXT_EVENT_COMPILED_CALLERS
        verify_native_scope(row,initial,expected_memory,before,error,resumed,rejected);
#endif

        aliases+=row.at("kind").get<std::string>().starts_with("alias-");
        ++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+": "+e.what());}
#ifdef HALO2_NATIVE_NEXT_EVENT_COMPILED_CALLERS
    need(cases==185 && called_total==0 && stops==7 && aliases==5,"Incomplete compiled effect next-event scope");
#else
    need(cases==185 && called_total==0 && stops==7 && aliases==5,"Incomplete physical original effect next-event scope");
    need(resumed>1000 && resumed==rejected,"Incomplete next-event frontier scope");
    std::cout<<resumed<<" next-event trace reopenings and "<<rejected<<" rejected accesses\n";
#endif
    std::cout<<"PASS "<<cases<<" physical original effect next-event cases, "<<called_total<<" actual lower ABI controls\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
