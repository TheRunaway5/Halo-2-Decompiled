#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#ifdef HALO2_NATIVE_NEXT_EVENT_COMPILED_CALLERS
#include "halo2/death_cycle_runtime_tu15.hpp"
#endif
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

        aliases+=row.at("kind").get<std::string>().starts_with("alias-");
        ++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+": "+e.what());}
#ifdef HALO2_NATIVE_NEXT_EVENT_COMPILED_CALLERS
    need(cases==185 && called_total==0 && stops==7 && aliases==5,"Incomplete compiled effect next-event scope");
#else
    need(cases==185 && called_total==0 && stops==7 && aliases==5,"Incomplete physical original effect next-event scope");
#endif
    std::cout<<"PASS "<<cases<<" physical original effect next-event cases, "<<called_total<<" actual lower ABI controls\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
