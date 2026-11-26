#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#ifdef HALO2_NATIVE_ADVANCE_COMPILED_CALLERS
#include "halo2/physics_live_support_tu15.hpp"
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
    need(a.x87_depth==fp.at("stack").size() && a.x87_status==fp.at("status"),"Different original x87 depth/status");
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
#ifndef HALO2_NATIVE_ADVANCE_COMPILED_CALLERS
struct RejectedRmwWrite {};

class RejectWrite final : public SourcePhysicsContactMemory {
    sound_test::Memory &backing;
    unsigned writes_until_stop;

public:
    RejectWrite(sound_test::Memory &backing, unsigned writes_until_stop)
        : backing(backing), writes_until_stop(writes_until_stop) {}

    void read(std::uint32_t address, std::span<std::byte> bytes) const override {
        backing.read(address, bytes);
    }
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        if (--writes_until_stop == 0)
            throw RejectedRmwWrite{};
        backing.write(address, bytes);
    }
};
#endif
}
int main(int argc,char **argv)try {
#ifdef HALO2_NATIVE_ADVANCE_COMPILED_CALLERS
    need(argc==3,"Supply original effect-advance reference and caller module");const std::string module=argv[2];
    need(module=="support" || module=="death","Unknown compiled effect-advance caller");
#else
    need(argc==2,"Supply original effect-advance reference");
#endif
    std::ifstream in(argv[1]);const auto reference=Json::parse(in);
    need(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    need(reference.at("schema")=="H2_TU15_WEAPON_EFFECT_ADVANCE_REFERENCE_V1","Wrong advance reference schema");
    unsigned cases{},called_total{},stops{},aliases{};
#ifndef HALO2_NATIVE_ADVANCE_COMPILED_CALLERS
    bool late_trace_checked{}, rejected_write_checked{};
#endif
    for(const auto &row:reference.at("rows"))try {
        const auto entry=row.at("entry").get<std::uint32_t>();
#ifdef HALO2_NATIVE_ADVANCE_COMPILED_CALLERS
        need(entry==0x17c140u,"Advance fixture has another native entry");
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
            need(pc==row.at("trace_base").get<std::uint32_t>()+offset,"Different original effect-advance instruction trace");
        };
        services.required_owner=[&](auto &,const auto &call,auto &abi,const auto &) {
            need(called<row.at("calls").size(),"Different actual lower call count");const auto &expected=row.at("calls").at(called++);
            need(!call.tail && call.target==expected.at("target") && call.instruction==m.get32(abi.gp[4])-5u,"Different actual lower call/return ABI");
            check_abi(abi,expected.at("before"),expected.at("flags_mask"));check_stack(m,expected.at("before").at("stack"));
            const auto &returned=expected.at("returned");abi.gp=returned.at("gp").template get<decltype(abi.gp)>();abi.eflags=returned.at("eflags");load_fp(abi,returned.at("fp"));++called_total;
        };
        bool stopped{};std::string error;
        try {
#ifdef HALO2_NATIVE_ADVANCE_COMPILED_CALLERS
            if(module=="support")source_run_physics_live_support_tu15(m,entry,a,*a.floating_point,services,observe);
            else source_run_death_cycle_runtime_tu15(m,entry,a,*a.floating_point,services,observe);
#else
            native_weapon_effect_dispatch_tu15_run(m,entry,a,services,observe);
#endif
        }catch(const std::out_of_range &e){stopped=true;error=e.what();}
        need(stopped==!row.at("fault").empty(),"Different original effect-advance read-stop outcome");
        if(stopped) {
            need(error.starts_with("unmapped ") && std::stoul(error.substr(9),nullptr,16)==row.at("fault")[0].at("address").get<std::uint32_t>(),
                "Different original effect-advance read-stop address");++stops;
        }
        need(stored==row.at("stores").size() && called==row.at("calls").size() && traced==trace.size(),"Different final original store/call/trace count");
        check_abi(a,row.at("expected"),row.at("flags_mask"));check_stack(m,row.at("expected").at("stack"));
        need(a.fs_base==before.fs_base && !a.active_mmx,"Unrelated source FS/MMX changed");
        for(const auto &[page,bytes]:m.pages)need(*bytes==*expected_memory.pages.at(page),"Different final original memory page");
#ifndef HALO2_NATIVE_ADVANCE_COMPILED_CALLERS
        auto native_memory=initial.clone();
        auto native_abi=before;
        unsigned native_calls{};
        std::optional<std::uint32_t> trace_after;
        std::vector<std::uint32_t> resumed;
        SourcePhysicsWorldRuntimeServices native_services;
        native_services.required_owner=[&](auto &,const auto &call,auto &abi,const auto &) {
            const auto &expected=row.at("calls").at(native_calls++);
            need(!call.tail && call.target==expected.at("target") && call.instruction==native_memory.get32(abi.gp[4])-5u,
                "Different unobserved effect child ABI");
            check_abi(abi,expected.at("before"),expected.at("flags_mask"));
            check_stack(native_memory,expected.at("before").at("stack"));
            const auto &returned=expected.at("returned");
            abi.gp=returned.at("gp").template get<decltype(abi.gp)>();abi.eflags=returned.at("eflags");load_fp(abi,returned.at("fp"));
            if(!late_trace_checked && !trace_after && row.at("calls").size()>1 && !stopped) {
                trace_after=call.instruction;
                native_services.instruction=[&](auto pc){resumed.push_back(pc);};
            }
        };
        std::string native_error;
        try {native_weapon_effect_dispatch_tu15_run(native_memory,entry,native_abi,native_services);}
        catch(const std::out_of_range &e){native_error=e.what();}
        need(native_error==error && native_calls==called,"Different unobserved effect fault or child count");
        check_abi(native_abi,row.at("expected"),row.at("flags_mask"));
        check_stack(native_memory,row.at("expected").at("stack"));
        need(native_abi.fs_base==before.fs_base && !native_abi.active_mmx,"Unobserved effect changed FS/MMX");
        need(native_memory.pages.size()==m.pages.size(),"Different effect memory extent");
        for(const auto &[page,bytes]:m.pages)
            need(*bytes==*native_memory.pages.at(page),"Different unobserved effect memory page");
        if(trace_after) {
            std::vector<std::uint32_t> suffix;
            bool after{};
            for(unsigned n=0;n<trace.size();n+=2) {
                const auto offset=std::to_integer<unsigned>(trace[n])|(std::to_integer<unsigned>(trace[n+1])<<8u);
                const auto pc=row.at("trace_base").get<std::uint32_t>()+offset;
                if(after)suffix.push_back(pc);
                after=after || pc==*trace_after;
            }
            need(after && !suffix.empty() && suffix.front()==*trace_after+5u && resumed==suffix,
                "Different Effect trace after real child return");
            late_trace_checked=true;
        }
        if(!rejected_write_checked) {
            const auto &stores=row.at("stores");
            const auto rmw=std::find_if(stores.begin(),stores.end(),[](const auto &event){return event[0]==0x17c284u;});
            if(rmw!=stores.end()) {
                const auto stop=unsigned(rmw-stores.begin()+1);
                auto traced_memory=initial.clone(), silent_memory=initial.clone();
                RejectWrite traced_write(traced_memory,stop), silent_write(silent_memory,stop);
                auto traced_abi=before, silent_abi=before;
                std::optional<std::uint32_t> prewrite_flags;
                SourcePhysicsWorldRuntimeServices traced_services;
                traced_services.instruction=[&](auto pc){if(pc==0x17c284u)prewrite_flags=traced_abi.eflags;};
                const auto rejected=[&](auto &memory,auto &state,const auto &policy) {
                    try {native_weapon_effect_dispatch_tu15_run(memory,entry,state,policy);}
                    catch(const RejectedRmwWrite &){return true;}
                    return false;
                };
                need(rejected(traced_write,traced_abi,traced_services) &&
                    rejected(silent_write,silent_abi,SourcePhysicsWorldRuntimeServices{}),"Missing rejected event flag write");
                need(prewrite_flags && traced_abi.eflags==*prewrite_flags && silent_abi.eflags==*prewrite_flags &&
                    traced_abi.gp==silent_abi.gp,"Faulting OR changed its original input register state");
                for(const auto &[page,bytes]:traced_memory.pages)
                    need(*bytes==*silent_memory.pages.at(page),"Faulting OR changed the native write prefix");
                rejected_write_checked=true;
            }
        }
        if(!row.at("calls").empty()) {
            const auto untouched=initial.clone();
            auto stopped=before;bool missing{};try{native_weapon_effect_dispatch_tu15_run(initial,entry,stopped,{});}catch(const std::runtime_error &){missing=true;}
            need(missing,"Missing actual lower owner was replaced");check_abi(stopped,row.at("calls")[0].at("before"),row.at("calls")[0].at("flags_mask"));
            check_stack(initial,row.at("calls")[0].at("before").at("stack"));
            if(row.at("calls").size()>1) {
                auto second_memory=untouched.clone();
                auto second=before;unsigned entered{};SourcePhysicsWorldRuntimeServices partial;
                partial.required_owner=[&](auto &,const auto &,auto &abi,const auto &) {
                    if(entered++)throw std::runtime_error("Missing second real owner");
                    const auto &returned=row.at("calls")[0].at("returned");abi.gp=returned.at("gp").template get<decltype(abi.gp)>();abi.eflags=returned.at("eflags");load_fp(abi,returned.at("fp"));
                };
                bool missing_second{};try{native_weapon_effect_dispatch_tu15_run(second_memory,entry,second,partial);}catch(const std::runtime_error &){missing_second=true;}
                need(missing_second && entered==2,"Missing second owner acquired a fabricated result");
                check_abi(second,row.at("calls")[1].at("before"),row.at("calls")[1].at("flags_mask"));check_stack(second_memory,row.at("calls")[1].at("before").at("stack"));
            }
        }
#endif
        aliases+=row.at("kind").get<std::string>().starts_with("alias-");
        ++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+": "+e.what());}
#ifdef HALO2_NATIVE_ADVANCE_COMPILED_CALLERS
    need(cases==142 && called_total==845 && stops==7 && aliases==2,"Incomplete compiled effect-advance scope");
#else
    need(cases==142 && called_total==845 && stops==7 && aliases==2,"Incomplete physical original effect-advance scope");
    need(late_trace_checked,"Missing resumed diagnostic trace after an Effect child");
    need(rejected_write_checked,"Missing rejected event flag write coverage");
#endif
    std::cout<<"PASS "<<cases<<" physical original effect-advance cases, "<<called_total<<" actual lower ABI controls\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
