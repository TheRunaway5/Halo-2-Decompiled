#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <cstring>
#include <fstream>
#include <set>
#include <immintrin.h>
#include <nlohmann/json.hpp>
#ifdef HALO2_NATIVE_PARENT_COMPOSED_CALLERS
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include "halo2/physics_vehicle_motion.hpp"
#endif
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
    if(a.gp!=expected.at("gp").get<decltype(a.gp)>())std::cerr<<"actual GP="<<Json(a.gp)<<" expected="<<expected.at("gp")<<'\n';
    need(a.gp==expected.at("gp").get<decltype(a.gp)>(),"Different original GP");
    need(!((a.eflags^expected.at("eflags").get<std::uint32_t>())&mask),"Different defined original flags");check_fp(a,expected.at("fp"));
}
void check_stack(const SourcePhysicsContactMemory &m,const Json &raw) {
    const auto expected=unhex(raw);std::vector<std::byte> actual(expected.size());m.read(stack-512,actual);need(actual==expected,"Different original live/dead stack");
}
struct Memory final:SourcePhysicsContactMemory {
    sound_test::Memory backing;
    std::set<std::uint32_t> write_only_sites,read_modify_write_sites;
    std::uint32_t pc{};
    bool capture{};
    mutable std::vector<std::array<std::uint32_t,3>> reads;
    void read(std::uint32_t at,std::span<std::byte> bytes)const override {
        // Store observers read preimages at write-only source instructions.
        // These are instrumentation reads, not original operand reads.
        if(capture && !write_only_sites.contains(pc))reads.push_back({pc,at,std::uint32_t(bytes.size())});
        backing.read(at,bytes);
    }
    void write(std::uint32_t at,std::span<const std::byte> bytes)override {std::array<std::byte,1> checked;for(unsigned n=0;n<bytes.size();++n)backing.read(at+n,checked);backing.write(at,bytes);}
};
}

int main(int argc,char **argv)try {
#ifdef HALO2_NATIVE_PARENT_COMPOSED_CALLERS
    need(argc==3,"Supply original composed marker-parent reference and module");const std::string module=argv[2];
    need(module=="support" || module=="ai" || module=="device" || module=="vehicle","Unknown marker-parent module");
#else
    need(argc==2,"Supply original composed marker-parent reference");
#endif
    std::ifstream in(argv[1]);const auto reference=Json::parse(in);need(reference.at("schema")=="H2_TU15_WEAPON_MARKER_PARENT_COMPOSED_REFERENCE_V1","Wrong composed parent reference");need(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");need(native_weapon_effect_dispatch_tu15_owns_entry(0xbacd0),"Native BACD0 owner is not registered");
    const auto run=[&](auto &memory,auto &abi,const auto &services,const SourcePhysicsContactRecordObserver &observe) {
#ifdef HALO2_NATIVE_PARENT_COMPOSED_CALLERS
        const auto fp=abi.floating_point.value();
        if(module=="support")source_run_physics_live_support_tu15(memory,0xbacd0,abi,fp,services,observe);
        else if(module=="ai")source_run_ai_frame_tu15(memory,0xbacd0,abi,fp,services,observe);
        else if(module=="device")source_run_scene_device_constructor_tu15(memory,0xbacd0,abi,fp,services,observe);
        else {
            constexpr std::uint32_t delta=0x1140,first=0xb9b90,last=0xb9c11;auto forwarded=services;auto observer=observe;
            if(services.instruction)forwarded.instruction=[&](auto pc){services.instruction(pc>=first && pc<=last?pc+delta:pc);};
            if(observe)observer=[&](const auto &event){auto actual=event;if(actual.instruction>=first && actual.instruction<=last)actual.instruction+=delta;observe(actual);};
            if(services.required_owner)forwarded.required_owner=[&](auto &m,const auto &call,auto &state,const auto &stores){auto actual=call;if(actual.instruction>=first && actual.instruction<=last)actual.instruction+=delta;services.required_owner(m,actual,state,stores);};
            source_run_physics_vehicle_motion_runtime(memory,first,abi,SourcePhysicsContactImage::TitleUpdate15,fp,forwarded,observer);
        }
#else
        native_weapon_effect_dispatch_tu15_run(memory,0xbacd0,abi,services,observe);
#endif
    };
    unsigned cases{},read_stops{},write_stops{},mmx{},aliases{},backward{},lower_calls{},lower_entries{},missing_stops{};
    for(const auto &row:reference.at("rows"))try {
        Memory m;m.write_only_sites=reference.at("write_only_sites").get<decltype(m.write_only_sites)>();m.read_modify_write_sites=reference.at("read_modify_write_sites").get<decltype(m.read_modify_write_sites)>();
        for(const auto &region:row.at("regions"))m.backing.map(region[0],region[1]);
        for(const auto &blob:row.at("blobs")){const auto at=blob[0].get<std::uint32_t>();const auto raw=unhex(blob[1]);m.backing.map(at,raw.size());m.backing.write(at,raw);}
        for(const auto &v:row.at("memory")){const auto at=v[0].get<std::uint32_t>(),value=v[1].get<std::uint32_t>();if(v[2]==1)m.backing.put8(at,value);else if(v[2]==2)m.backing.put16(at,value);else m.backing.put32(at,value);}
        auto initial=m.backing.clone(),expected_memory=m.backing.clone();SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=row.at("eflags");a.fs_base=0x76543210;load_fp(a,row.at("fp"));const auto before=a;const auto host_mxcsr=_mm_getcsr();unsigned stored{},traced{},called{};
        SourcePhysicsContactRecordObserver observe=[&](const auto &event){
            if(m.read_modify_write_sites.contains(event.instruction)){need(!m.reads.empty() && m.reads.back()==std::array<std::uint32_t,3>{event.instruction,event.address,std::uint32_t(event.written.size())},"Missing marker REP observer preimage");m.reads.pop_back();}
            need(stored<row.at("stores").size(),"Different marker store count");const auto &expected=row.at("stores").at(stored++);std::uint32_t value{};for(unsigned n=0;n<event.written.size();++n)value|=std::to_integer<std::uint32_t>(event.written[n])<<(8*n);
            need(event.instruction==expected[0] && event.address==expected[1] && event.written.size()==expected[2] && value==expected[3],"Different original marker store");std::vector<std::byte> prior(event.written.size());expected_memory.read(event.address,prior);need(prior==event.before,"Different marker preimage");expected_memory.write(event.address,event.written);
        };
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){m.pc=pc;need(traced<row.at("trace").size(),"Different marker trace extent");const auto expected=row.at("trace").at(traced++).get<std::uint32_t>();if(pc!=expected)std::cerr<<"actual PC="<<std::hex<<pc<<" expected="<<expected<<std::dec<<'\n';need(pc==expected,"Different original marker PC");};
        services.required_owner=[&](auto &memory,const auto &call,auto &abi,const auto &stores){
            need(called<row.at("calls").size(),"Different composed lower count");const auto &expected=row.at("calls").at(called++);++lower_entries;
            need(!call.tail && call.target==0xfc510u && call.target==expected.at("target") && call.instruction==m.backing.get32(abi.gp[4])-5u && !expected.at("controlled").get<bool>() && expected.at("writes").empty(),"Different real FC510 call boundary");
            check_abi(abi,expected.at("before"),expected.at("flags_mask"));check_stack(m.backing,expected.at("before").at("stack"));
            native_weapon_effect_dispatch_tu15_run(memory,call.target,abi,services,stores);
            check_abi(abi,expected.at("returned"),expected.at("returned_flags_mask"));check_stack(m.backing,expected.at("returned").at("stack"));++lower_calls;
        };
        m.capture=true;bool stopped{};std::string error;try{run(m,a,services,observe);}catch(const std::out_of_range &e){stopped=true;error=e.what();}m.capture=false;
        if(stopped && row.at("fault").empty())std::cerr<<"unexpected stop @ "<<std::hex<<m.pc<<std::dec<<": "<<error<<'\n';need(stopped==!row.at("fault").empty(),"Different marker fault outcome");
        if(stopped){const auto &fault=row.at("fault")[0];need(error.starts_with("unmapped ") && std::stoul(error.substr(9),nullptr,16)==fault.at("address").get<std::uint32_t>() && m.pc==fault.at("instruction"),"Different marker fault address/site");if(fault.at("access")=="read")++read_stops;else {++write_stops;if(m.read_modify_write_sites.contains(m.pc)){need(!m.reads.empty() && m.reads.back()==std::array<std::uint32_t,3>{m.pc,fault.at("operand_address"),fault.at("size")},"Missing faulting marker REP preimage");m.reads.pop_back();}}}
        need(stored==row.at("stores").size() && traced==row.at("trace").size() && called==row.at("calls").size(),"Different final marker trace/store/call counts");if(Json(m.reads)!=row.at("reads"))std::cerr<<"actual reads="<<Json(m.reads)<<" expected="<<row.at("reads")<<'\n';need(Json(m.reads)==row.at("reads"),"Different marker operand read order");
        check_abi(a,row.at("expected"),row.at("flags_mask"));check_stack(m.backing,row.at("expected").at("stack"));need(a.fs_base==before.fs_base,"Marker owner changed FS");for(const auto &[page,bytes]:m.backing.pages)need(*bytes==*expected_memory.pages.at(page),"Different marker final memory");need(_mm_getcsr()==host_mxcsr,"Marker owner leaked host SSE environment");
        struct MissingOwner{};for(unsigned unavailable=0;unavailable<row.at("calls").size();++unavailable){auto memory=initial.clone();auto partial_abi=before;unsigned entered{};SourcePhysicsWorldRuntimeServices partial;partial.required_owner=[&](auto &lower_memory,const auto &call,auto &abi,const auto &lower_stores){const auto &expected=row.at("calls").at(entered);need(call.target==expected.at("target"),"Different unavailable marker parent owner");if(entered++==unavailable)throw MissingOwner{};need(expected.at("writes").empty(),"Unexpected composed lower write control");native_weapon_effect_dispatch_tu15_run(lower_memory,call.target,abi,partial,lower_stores);};bool missing{};try{run(memory,partial_abi,partial,{});}catch(const MissingOwner &){missing=true;}need(missing && entered==unavailable+1,"Unavailable marker parent owner was replaced");const auto &expected=row.at("calls").at(unavailable);check_abi(partial_abi,expected.at("before"),expected.at("flags_mask"));check_stack(memory,expected.at("before").at("stack"));++missing_stops;}
        mmx+=row.at("kind")=="active-mmx";aliases+=row.at("kind").get<std::string>().starts_with("alias-");backward+=(row.at("eflags").get<std::uint32_t>()&1024u)!=0;++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+": "+e.what());}
    need(cases==238 && read_stops==14 && write_stops==2 && mmx==4 && aliases==10 && lower_entries==66 && lower_calls==63 && missing_stops==66,"Incomplete marker-parent scope");std::cout<<"PASS "<<cases<<" strict physical marker parent owner cases; "<<lower_calls<<" actual native FC510 returns, with 3 original callee faults and 66 unavailable boundaries\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

