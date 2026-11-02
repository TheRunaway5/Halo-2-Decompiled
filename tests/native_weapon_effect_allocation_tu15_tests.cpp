#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#ifdef HALO2_NATIVE_ALLOCATION_COMPILED_CALLERS
#include "halo2/physics_live_support_tu15.hpp"
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
    a.active_mmx.reset();
    if(fp.contains("mmx")) {
        SourcePhysicsMmxAlias mmx{};mmx.known_mask=255;
        for(unsigned n=0;n<8;++n){const auto raw=unhex(fp.at("mmx")[n]);need(raw.size()==10,"Invalid MMX reference");std::memcpy(mmx.physical[n].data(),raw.data(),10);}
        a.active_mmx=mmx;
    }
    for(unsigned n=0;n<a.x87_depth;++n){const auto raw=unhex(fp.at("stack")[n]);need(raw.size()==10,"Invalid extended reference");std::memcpy(&a.x87[n],raw.data(),10);}
    for(unsigned n=0;n<8;++n)a.xmm[n]=lanes(fp.at("xmm")[n]);
}
void check_fp(const SourcePhysicsWorldRuntimeAbi &a,const Json &fp) {
    need(a.floating_point && a.floating_point->x87_control_word==fp.at("cw") && a.floating_point->mxcsr==fp.at("mxcsr"),"Different original FP controls/sticky flags");
    need(a.x87_depth==fp.at("stack").size() && a.x87_status==fp.at("status"),"Different original x87 depth/status");
    for(unsigned n=0;n<a.x87_depth;++n){const auto raw=unhex(fp.at("stack")[n]);need(!std::memcmp(&a.x87[n],raw.data(),10),"Different original live extended lane");}
    for(unsigned n=0;n<8;++n)need(a.xmm[n]==lanes(fp.at("xmm")[n]),"Different original XMM");
    need(a.active_mmx.has_value()==fp.contains("mmx"),"Different original active MMX state");
    if(a.active_mmx) {
        need(a.active_mmx->known_mask==255,"Different original MMX known mask");
        for(unsigned n=0;n<8;++n){const auto raw=unhex(fp.at("mmx")[n]);need(!std::memcmp(a.active_mmx->physical[n].data(),raw.data(),10),"Different original physical MMX lane");}
    }
}
void check_abi(const SourcePhysicsWorldRuntimeAbi &a,const Json &expected,std::uint32_t mask) {
    need(a.gp==expected.at("gp").get<decltype(a.gp)>(),"Different original GP");
    need(!((a.eflags^expected.at("eflags").get<std::uint32_t>())&mask),"Different defined original flags");check_fp(a,expected.at("fp"));
}
void check_stack(const SourcePhysicsContactMemory &m,const Json &raw) {
    const auto expected=unhex(raw);std::vector<std::byte> actual(expected.size());m.read(stack-96,actual);need(actual==expected,"Different original live/dead stack");
}
}

int main(int argc,char **argv)try {
#ifdef HALO2_NATIVE_ALLOCATION_COMPILED_CALLERS
    need(argc==3,"Supply original allocation reference and caller module");const std::string module=argv[2];
    need(module=="support" || module=="death","Unknown compiled effect allocation caller");
#else
    need(argc==2,"Supply original allocation reference");
#endif
    std::ifstream in(argv[1]);const auto reference=Json::parse(in);
    need(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    unsigned cases{},called_total{},natural_total{},stops{},aliases{},mmx_cases{},missing_stops{};
    const auto words=[](auto &memory,const Json &values) {
        for(const auto &v:values) {
            const auto at=v[0].get<std::uint32_t>(),value=v[1].get<std::uint32_t>();
            if(v[2]==1)memory.put8(at,value);else if(v[2]==2)memory.put16(at,value);else memory.put32(at,value);
        }
    };
    const auto returned=[](auto &abi,const Json &state) {
        abi.gp=state.at("gp").template get<decltype(abi.gp)>();abi.eflags=state.at("eflags");load_fp(abi,state.at("fp"));
    };
    const auto run=[&](auto &memory,auto entry,auto &abi,const auto &services,const auto &observe) {
#ifdef HALO2_NATIVE_ALLOCATION_COMPILED_CALLERS
        if(module=="support")source_run_physics_live_support_tu15(memory,entry,abi,*abi.floating_point,services,observe);
        else source_run_death_cycle_runtime_tu15(memory,entry,abi,*abi.floating_point,services,observe);
#else
        native_weapon_effect_dispatch_tu15_run(memory,entry,abi,services,observe);
#endif
    };
    for(const auto &row:reference.at("rows"))try {
        constexpr auto entry=0x17afb0u;sound_test::Memory m;m.map(stack-4096,8192);m.write(stack-96,unhex(row.at("initial_stack")));
        for(const auto &blob:row.at("blobs")){const auto at=blob[0].get<std::uint32_t>();const auto raw=unhex(blob[1]);m.map(at,raw.size());m.write(at,raw);}
        words(m,row.at("memory"));const auto initial=m.clone();auto expected_memory=m.clone();
        SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=row.at("eflags");a.fs_base=0x76543210;load_fp(a,row.at("fp"));const auto before=a;
        unsigned stored{},called{},traced{};const auto trace=unhex(row.at("trace_offsets"));
        SourcePhysicsContactRecordObserver observe=[&](const auto &event) {
            if(stored>=row.at("stores").size())std::cerr<<"Unexpected store PC="<<std::hex<<event.instruction<<" address="<<event.address<<std::dec<<'\n';
            need(stored<row.at("stores").size(),"Different original allocation store count");const auto &expected=row.at("stores").at(stored++);
            std::uint32_t value{};for(unsigned n=0;n<event.written.size();++n)value|=std::to_integer<std::uint32_t>(event.written[n])<<(8*n);
            need(event.instruction==expected[0] && event.address==expected[1] && event.written.size()==expected[2] && value==expected[3],"Different original allocation store");
            std::vector<std::byte> prior(event.written.size());expected_memory.read(event.address,prior);need(prior==event.before,"Different original allocation preimage");expected_memory.write(event.address,event.written);
        };
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc) {
            need(traced+2<=trace.size(),"Different original allocation trace extent");const auto offset=std::to_integer<unsigned>(trace[traced])|(std::to_integer<unsigned>(trace[traced+1])<<8u);traced+=2;
            need(pc==entry+offset,"Different original composed allocation instruction trace");
        };
        services.required_owner=[&](auto &,const auto &call,auto &abi,const auto &) {
            need(called<row.at("calls").size(),"Different original composed allocation call count");const auto &expected=row.at("calls").at(called++);++called_total;
            need(!call.tail && call.target==expected.at("target") && call.instruction==m.get32(abi.gp[4])-5u,"Different actual allocation lower call/return ABI");
            check_abi(abi,expected.at("before"),expected.at("flags_mask"));check_stack(m,expected.at("before").at("stack"));
            if(expected.at("natural")) {
                ++natural_total;run(m,call.target,abi,services,observe);
                need(!expected.at("returned").is_null(),"Actual original child fault was replaced");check_abi(abi,expected.at("returned"),expected.at("returned_flags_mask"));check_stack(m,expected.at("returned").at("stack"));
            }else {
                words(m,expected.at("writes"));words(expected_memory,expected.at("writes"));returned(abi,expected.at("returned"));
            }
        };
        bool stopped{};std::string error;
        try{run(m,entry,a,services,observe);}catch(const std::out_of_range &e){stopped=true;error=e.what();}
        need(stopped==!row.at("fault").empty(),"Different original composed allocation stopped-access outcome");
        if(stopped){need(error.starts_with("unmapped ") && std::stoul(error.substr(9),nullptr,16)==row.at("fault")[0].at("address").get<std::uint32_t>(),"Different original allocation stop address");++stops;}
        need(stored==row.at("stores").size() && called==row.at("calls").size() && traced==trace.size(),"Different final composed allocation store/call/trace count");
        check_abi(a,row.at("expected"),row.at("flags_mask"));check_stack(m,row.at("expected").at("stack"));need(a.fs_base==before.fs_base,"Unrelated source FS changed");
        for(const auto &[page,bytes]:m.pages)need(*bytes==*expected_memory.pages.at(page),"Different final original allocation memory page");
#ifndef HALO2_NATIVE_ALLOCATION_COMPILED_CALLERS
        struct MissingOwner {};
        for(unsigned unavailable=0;unavailable<row.at("calls").size();++unavailable) {
            auto memory=initial.clone();auto stopped_abi=before;unsigned entered{};SourcePhysicsWorldRuntimeServices partial;
            partial.required_owner=[&](auto &,const auto &call,auto &abi,const auto &) {
                need(entered<row.at("calls").size(),"Unexpected partial allocation callee");const auto &expected=row.at("calls").at(entered);
                need(call.target==expected.at("target"),"Different partial allocation owner");
                if(entered++==unavailable)throw MissingOwner{};
                if(expected.at("natural"))native_weapon_effect_dispatch_tu15_run(memory,call.target,abi,partial);
                else {words(memory,expected.at("writes"));returned(abi,expected.at("returned"));}
            };
            bool missing{};try{native_weapon_effect_dispatch_tu15_run(memory,entry,stopped_abi,partial);}catch(const MissingOwner &){missing=true;}
            need(missing && entered==unavailable+1,"Missing composed allocation owner was replaced");const auto &expected=row.at("calls").at(unavailable);
            check_abi(stopped_abi,expected.at("before"),expected.at("flags_mask"));check_stack(memory,expected.at("before").at("stack"));++missing_stops;
        }
#endif
        const auto kind=row.at("kind").get<std::string>();aliases+=kind.starts_with("alias-");mmx_cases+=kind=="active-mmx";++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+": "+e.what());}
    need(cases==68 && called_total==177 && natural_total==61 && stops==12 && aliases==4 && mmx_cases==4,"Incomplete physical composed allocation scope");
#ifndef HALO2_NATIVE_ALLOCATION_COMPILED_CALLERS
    need(missing_stops==177,"Incomplete unavailable allocation owner scope");
#endif
    std::cout<<"PASS "<<cases<<" composed original allocation cases; "<<natural_total<<" actual child calls; "<<called_total-natural_total<<" controlled lower calls; "<<missing_stops<<" unavailable-owner stops\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

