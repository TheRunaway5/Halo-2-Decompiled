#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/source_effect_light_view_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <cstring>
#include <exception>
#include <fstream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
struct ObservationStop final {};
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s) {
    std::vector<std::byte> result;result.reserve(s.size()/2);
    for(std::size_t i=0;i<s.size();i+=2)result.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
    return result;
}
std::array<std::uint32_t,4> lanes(const Json &value) {
    const auto raw=unhex(value);need(raw.size()==16,"Invalid physical XMM file");
    std::array<std::uint32_t,4> out;std::memcpy(out.data(),raw.data(),16);return out;
}
struct Memory final:SourcePhysicsContactMemory {
    sound_test::Memory bytes;std::uint32_t readonly{},extent{};
    void read(std::uint32_t at,std::span<std::byte> out)const override {bytes.read(at,out);}
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        if(std::uint64_t(at)<std::uint64_t(readonly)+extent && std::uint64_t(readonly)<std::uint64_t(at)+in.size()) {
            char text[40];std::snprintf(text,sizeof text,"readonly %08x",std::max(at,readonly));throw std::out_of_range(text);
        }
        bytes.write(at,in);
    }
};
void load_fp(SourcePhysicsWorldRuntimeAbi &a,const Json &fp) {
    a.active_mmx.reset();
    a.floating_point=SourcePhysicsFactoryFloatingPoint{fp.at("cw"),fp.at("mxcsr")};
    a.x87_depth=fp.at("stack").size();a.x87_status=fp.at("status");
    for(unsigned n=0;n<a.x87_depth;++n) {
        const auto raw=unhex(fp.at("stack")[n]);need(raw.size()==10,"Invalid physical extended lane");
        std::memcpy(&a.x87[n],raw.data(),10);
    }
    for(unsigned n=0;n<8;++n)a.xmm[n]=lanes(fp.at("xmm")[n]);
    if(fp.contains("mmx")) {
        a.active_mmx.emplace();a.active_mmx->known_mask=0xa5;
        for(unsigned n=0;n<8;++n) {
            const auto raw=unhex(fp.at("mmx")[n]);need(raw.size()==10,"Invalid physical MMX lane");
            std::memcpy(a.active_mmx->physical[n].data(),raw.data(),10);
        }
    }
}
void check_fp(const SourcePhysicsWorldRuntimeAbi &a,const Json &fp) {
    need(a.floating_point && a.floating_point->x87_control_word==fp.at("cw") &&
        a.floating_point->mxcsr==fp.at("mxcsr"),"Different physical frame FP controls");
    need(a.x87_depth==fp.at("stack").size() && a.x87_status==fp.at("status"),"Different physical frame x87 depth/status");
    for(unsigned n=0;n<a.x87_depth;++n) {
        const auto raw=unhex(fp.at("stack")[n]);need(!std::memcmp(&a.x87[n],raw.data(),10),"Different physical frame extended lane");
    }
    for(unsigned n=0;n<8;++n)need(a.xmm[n]==lanes(fp.at("xmm")[n]),"Different physical frame XMM");
    need(bool(a.active_mmx)==fp.contains("mmx"),"Different physical frame MMX mode");
    if(a.active_mmx)for(unsigned n=0;n<8;++n) {
        const auto raw=unhex(fp.at("mmx")[n]);need(!std::memcmp(a.active_mmx->physical[n].data(),raw.data(),10),"Different physical frame MMX bytes");
    }
}
void check_abi(const SourcePhysicsWorldRuntimeAbi &a,const Json &expected,std::uint32_t mask) {
    if(a.gp!=expected.at("gp").get<decltype(a.gp)>())
        throw std::runtime_error("Different physical frame GP: actual "+Json(a.gp).dump()+" expected "+expected.at("gp").dump());
    need(!((a.eflags^expected.at("eflags").get<std::uint32_t>())&mask),"Different defined physical frame flags");
    check_fp(a,expected.at("fp"));
}
void check_stack(const SourcePhysicsContactMemory &m,std::uint32_t stack,const Json &expected) {
    const auto raw=unhex(expected);std::vector<std::byte> out(raw.size());m.read(stack-512,out);
    need(raw==out,"Different physical frame live/dead stack");
}
}
int main(int argc,char **argv)try {
    need(argc==3,"Supply physical frame/pool reference and caller route");const std::string mode=argv[2];
    const bool slot_only=mode=="light" || mode=="ai" || mode=="combat";
    need(mode=="direct" || mode=="frame" || slot_only,"Unknown frame route");
    std::ifstream in(argv[1]);const auto reference=Json::parse(in);
    need(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    const bool systems=reference.at("schema")=="H2_TU15_WEAPON_EFFECT_SYSTEMS_REFERENCE_V1";
    const bool particle=reference.at("schema")=="H2_TU15_WEAPON_PARTICLE_FRAME_REFERENCE_V1";
    const bool controlled=systems || particle;
    need(controlled || reference.at("schema")=="H2_TU15_WEAPON_EFFECT_FRAME_REFERENCE_V2","Wrong physical frame schema");
    need(!controlled || !slot_only,"Particle-frame controls have no slot-only route");
    {
        need(native_weapon_effect_dispatch_tu15_owns_entry(0x179df0u),"Native effect frame owner is not registered");
        need(native_weapon_effect_dispatch_tu15_owns_entry(0x16ea60u),"Native effect slot scan is not registered");
        if(particle)need(native_weapon_effect_dispatch_tu15_owns_entry(0x177830u),"Native particle frame owner is not registered");
        if(systems)need(native_weapon_effect_dispatch_tu15_owns_entry(0x179ed0u),"Native effect systems owner is not registered");
    }
    unsigned cases{},stops{},bounded{},returned{},calls{},mmx{};
    std::map<std::uint32_t,unsigned> lower_calls;
    for(const auto &row:reference.at("rows"))try {
        const auto entry=row.at("entry").get<std::uint32_t>();
        if(slot_only && entry!=0x16ea60u)continue;
        Memory m;m.readonly=row.at("readonly")[0];m.extent=row.at("readonly")[1];
        for(const auto &r:row.at("regions"))m.bytes.map(r[0],r[1]);
        m.bytes.map(0x3000000,4096); // Original CPU return-boundary backing.
        const auto stack=row.at("stack_base").get<std::uint32_t>();m.bytes.map(stack-512,1024);
        for(const auto &v:row.at("memory")) {
            const auto at=v[0].get<std::uint32_t>(),value=v[1].get<std::uint32_t>();
            if(v[2]==1)m.bytes.put8(at,value);else if(v[2]==2)m.bytes.put16(at,value);else m.bytes.put32(at,value);
        }
        m.bytes.write(stack-512,unhex(row.at("initial_stack")));auto expected_memory=m.bytes.clone();
        SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=row.at("eflags");
        a.fs_base=0x76543210;load_fp(a,row.at("fp"));const auto before=a;
        const auto trace=unhex(row.at("trace"));std::size_t traced{};unsigned stored{},called{};
        const auto has_budget=!row.at("boundary").empty();
        const auto budget_at=has_budget?row.at("boundary").at("next_instruction").get<std::uint32_t>():0u;
        SourcePhysicsContactRecordObserver observe=[&](const auto &event) {
            need(stored<row.at("stores").size(),"Different physical frame store count");const auto &expected=row.at("stores").at(stored++);
            std::uint32_t value{};for(unsigned n=0;n<event.written.size();++n)value|=std::to_integer<std::uint32_t>(event.written[n])<<(8*n);
            if(event.instruction!=expected[0] || event.address!=expected[1] || event.written.size()!=expected[2] || value!=expected[3])
                throw std::runtime_error("Different physical frame store: "+std::to_string(event.instruction)+"/"+std::to_string(event.address));
            std::vector<std::byte> previous(event.written.size());expected_memory.read(event.address,previous);
            need(previous==event.before,"Different physical frame store preimage");expected_memory.write(event.address,event.written);
        };
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](std::uint32_t pc) {
            if(traced==trace.size() && has_budget && pc==budget_at)throw ObservationStop{};
            need(traced+4<=trace.size(),"Different physical frame trace extent");std::uint32_t expected{};
            std::memcpy(&expected,trace.data()+traced,4);traced+=4;
            if(pc!=expected)throw std::runtime_error("Different physical frame trace: "+std::to_string(pc)+" expected "+std::to_string(expected));
        };
        services.required_owner=[&](auto &memory,const auto &call,auto &abi,const auto &o) {
            if(traced==trace.size() && has_budget && call.target==budget_at)throw ObservationStop{};
            // The composed reference executes this real lookup inside179ED0.
            // Its instructions and stores remain in the physical trace; it has
            // no supplied result or separate outer-frame boundary record.
            if(!controlled && call.target==0x178070u) {
                need(call.instruction==0x179f12u && !call.tail,"Different composed system lookup call");
                source_run_effect_particle_frame_tu15(memory,call.target,abi,*abi.floating_point,services,o);
                return;
            }
            need(called<row.at("calls").size(),"Different physical frame lower-call count");const auto &expected=row.at("calls").at(called++);
            need(call.target==expected.at("target") && call.tail==expected.at("tail"),"Different physical frame actual lower target/tail");
            const auto site=systems?expected.at("instruction").get<std::uint32_t>():particle?(call.target==0x1778d0u?0x1778a1u:call.target==0x17de10u?0x1778b2u:0x1778bfu):
                call.target==0x16ea60u?0x179dfcu:call.target==0x179ed0u?0x179e51u:0x179ec8u;
            need(call.instruction==site,"Different physical frame lower-call instruction");
            check_abi(abi,expected.at("before"),expected.at("flags_mask"));check_stack(memory,stack,expected.at("before").at("stack"));
            ++lower_calls[call.target];
            if(controlled) {
                for(const auto &write:expected.at("memory_writes")) {
                    const auto prior=unhex(write[1]),value=unhex(write[2]);std::vector<std::byte> actual(prior.size());
                    memory.read(write[0],actual);need(actual==prior,"Different declared lower bank-write preimage");
                    memory.write(write[0],value);expected_memory.write(write[0],value);
                }
                abi.gp=expected.at("returned").at("gp").template get<decltype(abi.gp)>();
                abi.eflags=expected.at("returned").at("eflags");load_fp(abi,expected.at("returned").at("fp"));
                check_stack(memory,stack,expected.at("returned").at("stack"));return;
            }
            std::exception_ptr error;
            try{source_run_effect_particle_frame_tu15(memory,call.target,abi,*abi.floating_point,services,o);}
            catch(...){error=std::current_exception();}
            check_abi(abi,expected.at("returned"),expected.at("returned_flags_mask"));check_stack(memory,stack,expected.at("returned").at("stack"));
            need(bool(error)==(expected.value("fault",false) || expected.value("interrupted",false)),"Different physical frame actual lower outcome");
            if(error)std::rethrow_exception(error);
        };
        bool fault{},interrupted{};std::string error;
        try {
            if(mode=="direct")native_weapon_effect_dispatch_tu15_run(m,entry,a,services,observe);
            else if(mode=="light")source_run_effect_light_view_tu15(m,entry,a,*a.floating_point,services,observe);
            else if(mode=="ai")source_run_ai_frame_tu15(m,entry,a,*a.floating_point,services,observe);
            else if(mode=="combat")source_run_combat_action_tu15(m,entry,a,*a.floating_point,services,observe);
            else source_run_effect_particle_frame_tu15(m,entry,a,*a.floating_point,services,observe);
        }catch(const std::out_of_range &e){fault=true;error=e.what();}
        catch(const ObservationStop &){interrupted=true;}
        if(fault!=!row.at("fault").empty() || interrupted!=has_budget)throw std::runtime_error("Different physical frame return/fault/observation outcome: fault="+std::to_string(fault)+" interrupted="+std::to_string(interrupted)+" error="+error);
        if(fault) {
            const auto &f=row.at("fault")[0];const auto prefix=f.at("kind")=="write"?"readonly ":"unmapped ";
            need(error.starts_with(prefix) && std::stoul(error.substr(9),nullptr,16)==f.at("address").get<std::uint32_t>(),"Different physical frame fault address");++stops;
        }
        need(stored==row.at("stores").size() && traced==trace.size(),"Different physical frame final store/trace extent");
        need(called==row.at("calls").size(),"Different physical frame final lower-call extent");
        check_abi(a,row.at("expected"),row.at("flags_mask"));check_stack(m,stack,row.at("expected").at("stack"));
        need(a.fs_base==before.fs_base,"Unrelated physical frame FS changed");
        if(a.active_mmx)need(a.active_mmx->known_mask==0xa5,"Unrelated physical frame MMX knowledge changed");
        need(m.bytes.pages.size()==expected_memory.pages.size(),"Different physical frame final memory extent");
        for(const auto &[page,bytes]:m.bytes.pages)need(*bytes==*expected_memory.pages.at(page),"Different physical frame final memory page");
        bounded+=interrupted;returned+=!interrupted && !fault;calls+=row.at("calls").size();mmx+=row.at("fp").contains("mmx");++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+" ("+row.at("kind").get<std::string>()+"/"+row.at("n").dump()+"): "+e.what());}
    if(systems)need(cases==193 && returned==172 && stops==21 && bounded==0 && mmx==38 && calls==1933,"Incomplete physical systems-owner controls");
    else if(particle)need(cases==118 && returned==103 && stops==13 && bounded==2 && mmx==23 && calls==136,"Incomplete physical particle-frame controls");
    else if(slot_only)need(cases==150 && returned==145 && stops==5 && bounded==0 && mmx==21 && calls==0,"Incomplete physical slot caller scope");
    else need(cases==327 && returned==304 && stops==20 && bounded==3 && mmx==46 && calls==532,"Incomplete physical frame scope");
    std::cout<<"PASS "<<cases<<" physical unchanged frame cases: "<<returned<<" returns, "<<stops<<" memory faults, "<<bounded<<" bounded prefixes, "<<mmx<<" MMX callers\n";
    std::cout<<"Actual lower-call boundaries: "<<calls<<", native route observed "<<lower_calls[0x16ea60u]<<"/"<<lower_calls[0x179ed0u]<<"/"<<lower_calls[0x177830u]<<"\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
