#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/source_effect_light_view_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <algorithm>
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
    std::function<void(std::uint32_t)> after_read;
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        bytes.read(at,out);
        if(after_read)after_read(at);
    }
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
void reopen_expected_trace(SourcePhysicsWorldRuntimeServices &services,std::span<const std::byte> trace,
                           std::uint32_t site,unsigned occurrence,std::uint32_t successor,
                           bool has_budget,std::uint32_t budget_at,
                           std::vector<std::uint32_t> &suffix,std::vector<std::uint32_t> &resumed,
                           bool child_trace_embedded=false) {
    bool after{};
    for(std::size_t n=0;n<trace.size();n+=4) {
        std::uint32_t pc{};
        std::memcpy(&pc,trace.data()+n,4);
        if(after) {
            if(child_trace_embedded && pc!=successor)continue;
            child_trace_embedded=false;
            suffix.push_back(pc);
        }
        else if(pc==site && --occurrence==0)after=true;
    }
    need(after && !suffix.empty() && suffix.front()==successor,"Different physical frame trace successor");
    services.instruction=[&,has_budget,budget_at](std::uint32_t pc) {
        if(has_budget && resumed.size()==suffix.size() && pc==budget_at)throw ObservationStop{};
        resumed.push_back(pc);
    };
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
#ifdef HALO2_NATIVE_EFFECT_SYSTEMS_DIRECT_ONLY
    need(controlled && mode=="direct","The focused kernel fixture requires a direct controlled route");
#endif
    need(controlled || reference.at("schema")=="H2_TU15_WEAPON_EFFECT_FRAME_REFERENCE_V2","Wrong physical frame schema");
    need(!controlled || !slot_only,"Particle-frame controls have no slot-only route");
    {
        need(native_weapon_effect_dispatch_tu15_owns_entry(0x179df0u),"Native effect frame owner is not registered");
        need(native_weapon_effect_dispatch_tu15_owns_entry(0x16ea60u),"Native effect slot scan is not registered");
        if(particle)need(native_weapon_effect_dispatch_tu15_owns_entry(0x177830u),"Native particle frame owner is not registered");
        if(systems)need(native_weapon_effect_dispatch_tu15_owns_entry(0x179ed0u),"Native effect systems owner is not registered");
    }
    unsigned cases{},stops{},bounded{},returned{},calls{},mmx{},quiet_slot_rows{},quiet_slot_faults{},quiet_child_slot_faults{};
    bool reopened_systems_trace{};
    bool reopened_particle_scan{};
    constexpr std::array particle_sites{0x1778a1u,0x1778b2u,0x1778bfu};
    std::array<bool,particle_sites.size()> reopened_particle_calls{};
    unsigned reopened_particle_prefixes{};
#ifndef HALO2_NATIVE_EFFECT_SYSTEMS_DIRECT_ONLY
    constexpr std::array frame_sites{0x179dfcu,0x179e51u};
    std::array<bool,frame_sites.size()> reopened_frame_calls{};
    bool reopened_frame_scan{};
    unsigned reopened_frame_prefixes{};
#endif
    std::map<std::uint32_t,unsigned> lower_calls;
    for(const auto &row:reference.at("rows"))try {
        const auto entry=row.at("entry").get<std::uint32_t>();
        if(slot_only && entry!=0x16ea60u)continue;
        const bool parent=entry==0x179df0u && !controlled;
        const bool slot=entry==0x16ea60u && !controlled;
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
        const auto lower_site=[&](const Json &expected) {
            const auto target=expected.at("target").get<std::uint32_t>();
            if(systems)return expected.at("instruction").get<std::uint32_t>();
            if(particle)return target==0x1778d0u?0x1778a1u:target==0x17de10u?0x1778b2u:0x1778bfu;
            return target==0x16ea60u?0x179dfcu:target==0x179ed0u?0x179e51u:0x179ec8u;
        };
        services.required_owner=[&](auto &memory,const auto &call,auto &abi,const auto &o) {
            if(traced==trace.size() && has_budget && call.target==budget_at)throw ObservationStop{};
            // The composed reference executes this real lookup inside179ED0.
            // Its instructions and stores remain in the physical trace; it has
            // no supplied result or separate outer-frame boundary record.
            if(!controlled && call.target==0x178070u) {
                need(call.instruction==0x179f12u && !call.tail,"Different composed system lookup call");
#ifndef HALO2_NATIVE_EFFECT_SYSTEMS_DIRECT_ONLY
                source_run_effect_particle_frame_tu15(memory,call.target,abi,*abi.floating_point,services,o);
                return;
#endif
            }
            need(called<row.at("calls").size(),"Different physical frame lower-call count");const auto &expected=row.at("calls").at(called++);
            need(call.target==expected.at("target") && call.tail==expected.at("tail"),"Different physical frame actual lower target/tail");
            need(call.instruction==lower_site(expected),"Different physical frame lower-call instruction");
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
#ifndef HALO2_NATIVE_EFFECT_SYSTEMS_DIRECT_ONLY
            std::exception_ptr error;
            try{source_run_effect_particle_frame_tu15(memory,call.target,abi,*abi.floating_point,services,o);}
            catch(...){error=std::current_exception();}
            check_abi(abi,expected.at("returned"),expected.at("returned_flags_mask"));check_stack(memory,stack,expected.at("returned").at("stack"));
            need(bool(error)==(expected.value("fault",false) || expected.value("interrupted",false)),"Different physical frame actual lower outcome");
            if(error)std::rethrow_exception(error);
#endif
        };
        Memory silent;
        if((controlled || parent || slot) && mode=="direct") {
            silent.bytes=m.bytes.clone();
            silent.readonly=m.readonly;
            silent.extent=m.extent;
        }
        bool fault{},interrupted{};std::string error;
        try {
            if(mode=="direct")native_weapon_effect_dispatch_tu15_run(m,entry,a,services,observe);
#ifndef HALO2_NATIVE_EFFECT_SYSTEMS_DIRECT_ONLY
            else if(mode=="light")source_run_effect_light_view_tu15(m,entry,a,*a.floating_point,services,observe);
            else if(mode=="ai")source_run_ai_frame_tu15(m,entry,a,*a.floating_point,services,observe);
            else if(mode=="combat")source_run_combat_action_tu15(m,entry,a,*a.floating_point,services,observe);
            else source_run_effect_particle_frame_tu15(m,entry,a,*a.floating_point,services,observe);
#endif
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
        if(slot && mode=="direct") {
            auto native_abi=before;
            std::string silent_error;
            try {native_weapon_effect_dispatch_tu15_run(silent,entry,native_abi,{});}
            catch(const std::out_of_range &e){silent_error=e.what();}
            need(silent_error==error,"Different quiet slot scan memory fault");
            check_abi(native_abi,row.at("expected"),row.at("flags_mask"));
            check_stack(silent,stack,row.at("expected").at("stack"));
            need(native_abi.fs_base==before.fs_base && silent.bytes.pages.size()==expected_memory.pages.size(),
                 "Different quiet slot scan FS or memory extent");
            if(native_abi.active_mmx)need(native_abi.active_mmx->known_mask==0xa5,"Quiet slot scan changed MMX knowledge");
            for(const auto &[page,bytes]:expected_memory.pages)
                need(*bytes==*silent.bytes.pages.at(page),"Different quiet slot scan final memory");
            ++quiet_slot_rows;
            quiet_slot_faults+=fault;
        }
        if(controlled && mode=="direct") {
            SourcePhysicsWorldRuntimeServices silent_services;
            auto native_abi=before;
            unsigned native_calls{};
            std::vector<std::uint32_t> suffix,resumed;
            std::map<std::uint32_t,unsigned> occurrences;
            const auto reopen=[&](std::uint32_t site,unsigned occurrence,std::uint32_t successor) {
                reopen_expected_trace(silent_services,trace,site,occurrence,successor,has_budget,budget_at,suffix,resumed);
            };
            if(has_budget) {
                silent.after_read=[&](std::uint32_t address) {
                    if(address==0x51df74u && !silent_services.instruction) {
                        reopen(0x177837u,1u,0x17783du);
                        ++reopened_particle_prefixes;
                    }
                };
            } else if(particle && row.at("kind")=="base" && row.at("n")==1) {
                const auto pool=silent.bytes.get32(0x51df74u);
                const auto bitmap=silent.bytes.get32(pool+0x48u);
                silent.after_read=[&,bitmap](std::uint32_t address) {
                    if(address==bitmap && !silent_services.instruction) {
                        reopen(0x177866u,1u,0x177869u);
                        reopened_particle_scan=true;
                    }
                };
            }
            silent_services.required_owner=[&](auto &memory,const auto &call,auto &abi,const auto &) {
                const auto &expected=row.at("calls").at(native_calls++);
                need(call.target==expected.at("target") && call.tail==expected.at("tail") &&
                    call.instruction==lower_site(expected),"Different native frame child target/site");
                check_abi(abi,expected.at("before"),expected.at("flags_mask"));
                check_stack(memory,stack,expected.at("before").at("stack"));
                for(const auto &write:expected.at("memory_writes")) {
                    const auto prior=unhex(write[1]),value=unhex(write[2]);
                    std::vector<std::byte> actual(prior.size());
                    memory.read(write[0],actual);
                    need(actual==prior,"Different native frame declared child preimage");
                    memory.write(write[0],value);
                }
                const auto &result=expected.at("returned");
                abi.gp=result.at("gp").template get<decltype(abi.gp)>();
                abi.eflags=result.at("eflags");load_fp(abi,result.at("fp"));
                check_stack(memory,stack,result.at("stack"));
                const auto occurrence=++occurrences[call.instruction];
                if(fault || silent_services.instruction)return;
                if(systems && !reopened_systems_trace && call.instruction==0x179f5du) {
                    reopen(0x179f5du,occurrence,0x179f60u);
                    reopened_systems_trace=true;
                } else if(particle) {
                    const auto found=std::find(particle_sites.begin(),particle_sites.end(),call.instruction);
                    if(found==particle_sites.end())return;
                    auto &checked=reopened_particle_calls[found-particle_sites.begin()];
                    if(!checked) {
                        reopen(call.instruction,occurrence,call.instruction+5u);
                        checked=true;
                    }
                }
            };
            std::string silent_error;bool silent_interrupted{};
            try {native_weapon_effect_dispatch_tu15_run(silent,entry,native_abi,silent_services);}
            catch(const std::out_of_range &e){silent_error=e.what();}
            catch(const ObservationStop &){silent_interrupted=true;}
            need(silent_error==error && silent_interrupted==has_budget && native_calls==called,
                "Different native frame fault or child count");
            check_abi(native_abi,row.at("expected"),row.at("flags_mask"));
            check_stack(silent,stack,row.at("expected").at("stack"));
            need(native_abi.fs_base==before.fs_base && silent.bytes.pages.size()==m.bytes.pages.size(),
                "Different native frame FS or memory extent");
            for(const auto &[page,bytes]:m.bytes.pages)
                need(*bytes==*silent.bytes.pages.at(page),"Different native frame final memory");
            need(suffix==resumed,"Different native frame trace after reopening");
        }
#ifndef HALO2_NATIVE_EFFECT_SYSTEMS_DIRECT_ONLY
        if(parent && mode=="direct") {
            auto native_abi=before;
            SourcePhysicsWorldRuntimeServices silent_services;
            unsigned native_calls{};
            std::map<std::uint32_t,unsigned> occurrences;
            std::vector<std::uint32_t> suffix,resumed;
            bool first_slot_returned{};
            const auto reopen=[&](std::uint32_t site,unsigned occurrence,std::uint32_t successor,bool child_trace_embedded=false) {
                reopen_expected_trace(silent_services,trace,site,occurrence,successor,has_budget,budget_at,suffix,resumed,
                                      child_trace_embedded);
            };
            if(has_budget) {
                silent.after_read=[&](std::uint32_t address) {
                    if(address==0x4f720cu && !silent_services.instruction) {
                        reopen(0x179df1u,1u,0x179df7u);
                        ++reopened_frame_prefixes;
                    }
                };
            } else if(row.at("kind")=="base" && row.at("n")==14) {
                const auto pool=silent.bytes.get32(0x4f720cu);
                const auto bitmap=silent.bytes.get32(pool+0x48u);
                silent.after_read=[&,bitmap](std::uint32_t address) {
                    if(address==bitmap && first_slot_returned && !silent_services.instruction) {
                        reopen(0x179e91u,1u,0x179e94u);
                        reopened_frame_scan=true;
                    }
                };
            }
            silent_services.required_owner=[&](auto &memory,const auto &call,auto &abi,const auto &o) {
                if(call.target==0x178070u) {
                    need(call.instruction==0x179f12u && !call.tail,"Different native effect system lookup");
                    source_run_effect_particle_frame_tu15(memory,call.target,abi,*abi.floating_point,silent_services,o);
                    return;
                }
                const auto &expected=row.at("calls").at(native_calls++);
                need(call.target==expected.at("target") && call.tail==expected.at("tail") &&
                    call.instruction==lower_site(expected),"Different native effect frame child target/site");
                check_abi(abi,expected.at("before"),expected.at("flags_mask"));
                check_stack(memory,stack,expected.at("before").at("stack"));
                std::exception_ptr stopped;
                quiet_child_slot_faults+=call.target==0x16ea60u && expected.value("fault",false) && !silent_services.instruction;
                try{source_run_effect_particle_frame_tu15(memory,call.target,abi,*abi.floating_point,silent_services,o);}
                catch(...){stopped=std::current_exception();}
                check_abi(abi,expected.at("returned"),expected.at("returned_flags_mask"));
                check_stack(memory,stack,expected.at("returned").at("stack"));
                need(bool(stopped)==(expected.value("fault",false) || expected.value("interrupted",false)),
                     "Different native effect frame child outcome");
                if(stopped)std::rethrow_exception(stopped);
                if(call.target==0x16ea60u)first_slot_returned=true;
                const auto occurrence=++occurrences[call.instruction];
                if(fault || silent_services.instruction || row.at("n")==14)return;
                const auto found=std::find(frame_sites.begin(),frame_sites.end(),call.instruction);
                if(found==frame_sites.end())return;
                auto &checked=reopened_frame_calls[found-frame_sites.begin()];
                if(!checked) {
                    reopen(call.instruction,occurrence,call.instruction+5u,true);
                    checked=true;
                }
            };
            std::string silent_error;bool interrupted_silent{};
            try {native_weapon_effect_dispatch_tu15_run(silent,entry,native_abi,silent_services);}
            catch(const std::out_of_range &e){silent_error=e.what();}
            catch(const ObservationStop &){interrupted_silent=true;}
            need(silent_error==error && interrupted_silent==has_budget && native_calls==called,
                 "Different native effect frame fault or child count");
            check_abi(native_abi,row.at("expected"),row.at("flags_mask"));
            check_stack(silent,stack,row.at("expected").at("stack"));
            need(native_abi.fs_base==before.fs_base && silent.bytes.pages.size()==m.bytes.pages.size(),
                 "Different native effect frame FS or memory extent");
            for(const auto &[page,bytes]:m.bytes.pages)
                need(*bytes==*silent.bytes.pages.at(page),"Different native effect frame final memory");
            need(suffix==resumed,"Different native effect frame trace after reopening");
        }
#endif
        bounded+=interrupted;returned+=!interrupted && !fault;calls+=row.at("calls").size();mmx+=row.at("fp").contains("mmx");++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+" ("+row.at("kind").get<std::string>()+"/"+row.at("n").dump()+"): "+e.what());}
    if(systems)need(cases==193 && returned==172 && stops==21 && bounded==0 && mmx==38 && calls==1933,"Incomplete physical systems-owner controls");
    else if(particle)need(cases==118 && returned==103 && stops==13 && bounded==2 && mmx==23 && calls==136,"Incomplete physical particle-frame controls");
    else if(slot_only)need(cases==150 && returned==145 && stops==5 && bounded==0 && mmx==21 && calls==0,"Incomplete physical slot caller scope");
    else need(cases==327 && returned==304 && stops==20 && bounded==3 && mmx==46 && calls==532,"Incomplete physical frame scope");
    std::cout<<"PASS "<<cases<<" physical unchanged frame cases: "<<returned<<" returns, "<<stops<<" memory faults, "<<bounded<<" bounded prefixes, "<<mmx<<" MMX callers\n";
    std::cout<<"Actual lower-call boundaries: "<<calls<<", native route observed "<<lower_calls[0x16ea60u]<<"/"<<lower_calls[0x179ed0u]<<"/"<<lower_calls[0x177830u]<<"\n";
    if(systems && mode=="direct")need(reopened_systems_trace,"Missing native trace reopen after a virtual owner");
    if(particle && mode=="direct")need(reopened_particle_scan && reopened_particle_prefixes==bounded &&
        std::all_of(reopened_particle_calls.begin(),reopened_particle_calls.end(),[](bool checked){return checked;}),
        "Missing particle frame trace reopen after a bitmap read, child, or bounded prefix");
#ifndef HALO2_NATIVE_EFFECT_SYSTEMS_DIRECT_ONLY
    if(!controlled && mode=="direct")need(reopened_frame_scan && reopened_frame_prefixes==bounded &&
        std::all_of(reopened_frame_calls.begin(),reopened_frame_calls.end(),[](bool checked){return checked;}),
        "Missing effect frame trace reopen after a bitmap read, child or bounded prefix");
    if(!controlled && mode=="direct") {
        need(quiet_slot_rows==150 && quiet_slot_faults==5 && quiet_child_slot_faults!=0,"Incomplete quiet slot scan fault coverage");
        std::cout<<"Quiet slot scan checked "<<quiet_slot_rows<<" standalone rows with "<<quiet_slot_faults
                 <<" faults and "<<quiet_child_slot_faults<<" faults inside the actual parent composition\n";
    }
#endif
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
