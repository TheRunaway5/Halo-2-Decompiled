#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <cstring>
#include <fstream>
#include <set>
#include <immintrin.h>
#include <nlohmann/json.hpp>
#ifdef HALO2_NATIVE_MARKER_MATRIX_COMPILED_CALLERS
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_sandbox_special_tu15.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include "halo2/source_damage_delivery_runtime.hpp"
#include "halo2/physics_vehicle_motion.hpp"
#endif
using namespace halo2;using Json=nlohmann::json;
namespace {
constexpr std::uint32_t stack=0x2010000;
struct OwnedVehicleLowerEntry {};
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
#ifdef HALO2_NATIVE_MARKER_MATRIX_COMPILED_CALLERS
    need(argc==3,"Supply original marker-matrix reference and module");const std::string module=argv[2];
    need(module=="support" || module=="ai" || module=="sandbox" || module=="device" || module=="damage" || module=="damage-selected" || module=="vehicle","Unknown marker-matrix module");
#else
    need(argc==2,"Supply original marker-matrix reference");
#endif
    std::ifstream in(argv[1]);const auto reference=Json::parse(in);need(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");need(native_weapon_effect_dispatch_tu15_owns_entry(0xbad60),"Native BAD60 owner is not registered");
    const auto run=[&](auto &memory,auto &abi,const auto &services,const SourcePhysicsContactRecordObserver &observe) {
#ifdef HALO2_NATIVE_MARKER_MATRIX_COMPILED_CALLERS
        const auto fp=abi.floating_point.value();
        if(module=="support")source_run_physics_live_support_tu15(memory,0xbad60,abi,fp,services,observe);
        else if(module=="ai")source_run_ai_frame_tu15(memory,0xbad60,abi,fp,services,observe);
        else if(module=="sandbox")source_run_sandbox_special_tu15(memory,0xbad60,abi,fp,services,observe);
        else if(module=="device")source_run_scene_device_constructor_tu15(memory,0xbad60,abi,fp,services,observe);
        else {
            constexpr std::uint32_t delta=0x1140,first=0xb9c20,last=0xb9dc5;auto forwarded=services;auto observer=observe;
            if(module!="damage-selected") {
                if(services.instruction)forwarded.instruction=[&](auto pc){services.instruction(pc>=first && pc<=last?pc+delta:pc);};
                if(observe)observer=[&](const auto &event){auto actual=event;if(actual.instruction>=first && actual.instruction<=last)actual.instruction+=delta;observe(actual);};
            }
            if(services.required_owner)forwarded.required_owner=[&](auto &m,const auto &call,auto &state,const auto &stores){auto actual=call;if(actual.instruction>=first && actual.instruction<=last)actual.instruction+=delta;services.required_owner(m,actual,state,stores);};
            if(module=="damage")source_run_damage_delivery_runtime(memory,first,abi,SourcePhysicsContactImage::TitleUpdate15,fp,forwarded,observer);
            else if(module=="damage-selected")source_run_damage_delivery_runtime_selected_observations(memory,first,abi,SourcePhysicsContactImage::TitleUpdate15,fp,forwarded,observer);
            else source_run_physics_vehicle_motion_runtime(memory,first,abi,SourcePhysicsContactImage::TitleUpdate15,fp,forwarded,observer);
        }
#else
        native_weapon_effect_dispatch_tu15_run(memory,0xbad60,abi,services,observe);
#endif
    };
    const auto returned=[](auto &abi,const Json &state){abi.gp=state.at("gp").template get<decltype(abi.gp)>();abi.eflags=state.at("eflags");load_fp(abi,state.at("fp"));};
    unsigned cases{},read_stops{},write_stops{},mmx{},aliases{},backward{},lower_calls{},missing_stops{},owned_vehicle_entries{};
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
        services.instruction=[&](auto pc){
#ifdef HALO2_NATIVE_MARKER_MATRIX_COMPILED_CALLERS
            // Vehicle composition owns these real callees before forwarding
            // unknown owners to the caller. Stop before their first instruction
            // and compare the actual pushed CALL boundary, without supplying a
            // return that this public API intentionally does not accept.
            if(module=="vehicle" && !row.at("calls").empty()) {
                const auto target=row.at("calls")[0].at("target").get<std::uint32_t>();
                if((pc==0xb9b90u && target==0xbacd0u) || (pc==0x1dc8f0u && target==0x1e11b0u) || (pc==0x1dc940u && target==0x1e1200u))throw OwnedVehicleLowerEntry{};
            }
#endif
            m.pc=pc;need(traced<row.at("trace").size(),"Different marker trace extent");const auto expected=row.at("trace").at(traced++).get<std::uint32_t>();if(pc!=expected)std::cerr<<"actual PC="<<std::hex<<pc<<" expected="<<expected<<std::dec<<'\n';need(pc==expected,"Different original marker PC");};
        services.required_owner=[&](auto &,const auto &call,auto &abi,const auto &){need(called<row.at("calls").size(),"Different marker lower count");const auto &expected=row.at("calls").at(called++);need(!call.tail && call.target==expected.at("target") && call.instruction==m.backing.get32(abi.gp[4])-5u,"Different marker lower call/return ABI");check_abi(abi,expected.at("before"),expected.at("flags_mask"));check_stack(m.backing,expected.at("before").at("stack"));for(const auto &write:expected.at("writes")){const auto at=write[0].template get<std::uint32_t>();const auto bytes=unhex(write[1]);m.backing.write(at,bytes);expected_memory.write(at,bytes);}returned(abi,expected.at("returned"));++lower_calls;};
        m.capture=true;bool stopped{},owned_entry{};std::string error;try{run(m,a,services,observe);}catch(const OwnedVehicleLowerEntry &){owned_entry=true;}catch(const std::out_of_range &e){stopped=true;error=e.what();}m.capture=false;
        if(owned_entry) {
            const auto &expected=row.at("calls")[0];need(called==0 && traced>0,"Vehicle lower owner accepted supplied component results");
            check_abi(a,expected.at("before"),expected.at("flags_mask"));check_stack(m.backing,expected.at("before").at("stack"));
            need(row.at("trace")[traced-1].get<std::uint32_t>()+5u==m.backing.get32(a.gp[4]),"Different real vehicle lower CALL word");
            std::set<std::uint32_t> prefix;for(unsigned n=0;n<traced;++n)prefix.insert(row.at("trace")[n].get<std::uint32_t>());Json expected_reads=Json::array();
            for(const auto &read:row.at("reads")){if(!prefix.contains(read[0].get<std::uint32_t>()))break;expected_reads.push_back(read);}
            need(Json(m.reads)==expected_reads,"Different vehicle lower-entry read prefix");
            for(const auto &[page,bytes]:m.backing.pages)need(*bytes==*expected_memory.pages.at(page),"Different vehicle lower-entry memory");
            need(a.fs_base==before.fs_base && _mm_getcsr()==host_mxcsr,"Vehicle lower entry changed FS or host SSE environment");
            ++owned_vehicle_entries;++cases;continue;
        }
        if(stopped && row.at("fault").empty())std::cerr<<"unexpected stop @ "<<std::hex<<m.pc<<std::dec<<": "<<error<<'\n';need(stopped==!row.at("fault").empty(),"Different marker fault outcome");
        if(stopped){const auto &fault=row.at("fault")[0];need(error.starts_with("unmapped ") && std::stoul(error.substr(9),nullptr,16)==fault.at("address").get<std::uint32_t>() && m.pc==fault.at("instruction"),"Different marker fault address/site");if(fault.at("access")=="read")++read_stops;else {++write_stops;if(m.read_modify_write_sites.contains(m.pc)){need(!m.reads.empty() && m.reads.back()==std::array<std::uint32_t,3>{m.pc,fault.at("operand_address"),fault.at("size")},"Missing faulting marker REP preimage");m.reads.pop_back();}}}
        need(stored==row.at("stores").size() && traced==row.at("trace").size() && called==row.at("calls").size(),"Different final marker trace/store/call counts");if(Json(m.reads)!=row.at("reads"))std::cerr<<"actual reads="<<Json(m.reads)<<" expected="<<row.at("reads")<<'\n';need(Json(m.reads)==row.at("reads"),"Different marker operand read order");
        check_abi(a,row.at("expected"),row.at("flags_mask"));check_stack(m.backing,row.at("expected").at("stack"));need(a.fs_base==before.fs_base,"Marker owner changed FS");for(const auto &[page,bytes]:m.backing.pages)need(*bytes==*expected_memory.pages.at(page),"Different marker final memory");need(_mm_getcsr()==host_mxcsr,"Marker owner leaked host SSE environment");
        struct MissingOwner{};for(unsigned unavailable=0;unavailable<row.at("calls").size();++unavailable){auto memory=initial.clone();auto partial_abi=before;unsigned entered{};SourcePhysicsWorldRuntimeServices partial;partial.required_owner=[&](auto &,const auto &call,auto &abi,const auto &){const auto &expected=row.at("calls").at(entered);need(call.target==expected.at("target"),"Different unavailable marker owner");if(entered++==unavailable)throw MissingOwner{};for(const auto &write:expected.at("writes"))memory.write(write[0],unhex(write[1]));returned(abi,expected.at("returned"));};bool missing{};try{run(memory,partial_abi,partial,{});}catch(const MissingOwner &){missing=true;}need(missing && entered==unavailable+1,"Unavailable marker owner was replaced");const auto &expected=row.at("calls").at(unavailable);check_abi(partial_abi,expected.at("before"),expected.at("flags_mask"));check_stack(memory,expected.at("before").at("stack"));++missing_stops;}
        mmx+=row.at("kind")=="active-mmx";aliases+=row.at("kind").get<std::string>().starts_with("alias-");backward+=(row.at("eflags").get<std::uint32_t>()&1024u)!=0;++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+": "+e.what());}
#ifdef HALO2_NATIVE_MARKER_MATRIX_COMPILED_CALLERS
    if(module=="vehicle") {
        need(cases==201 && owned_vehicle_entries==75 && lower_calls==0 && missing_stops==0,"Incomplete real vehicle owner boundary scope");
        std::cout<<"PASS 126 complete strict physical vehicle marker cases and 75 exact real-owned lower entry boundaries; no supplied lower results\n";return 0;
    }
#endif
    need(cases==201 && read_stops==17 && write_stops==16 && mmx==4 && aliases==18 && backward==13 && lower_calls==134 && missing_stops==134,"Incomplete marker-matrix scope");std::cout<<"PASS "<<cases<<" strict physical marker owner cases; "<<lower_calls<<" declared lower ABIs and unavailable boundaries\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

