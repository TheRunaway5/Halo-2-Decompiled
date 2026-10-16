#include "native_tu15_sound_registered_event_fixture.hpp"
#include "halo2/source_runtime_sound_timed_notifications.hpp"
#include <poll.h>
#include <sys/timerfd.h>
namespace {
using Timed=SourceRuntimeSoundTimedNotifications;
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string out;out.reserve(bytes.size()*2);
    for(const auto b:bytes){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}return out;
}
Json snapshot(const SourceRuntimeMemory& m) {
    Json rows=Json::array();m.visit_committed([&](auto p,auto bytes){rows.push_back({{"address",p},{"hex",hex(bytes)},{"readonly",m.is_read_only(p,bytes.size())}});});return rows;
}
Json run(const Json& reference,bool interrupt_callback=false) {
    EventFixture e(reference);e.frontend.reset();e.frontend=std::make_unique<Frontend>(e.f.memory,e.reset,e.dma,1);
    for(unsigned i=0;i<2;++i)e.frontend->bind(64+i,0x19010000+i*0x200,e.leases[i]);
    mount_native_tu15_sound_event_port_data(e.f.memory);mount_native_tu15_sound_timed_notification_data(e.f.memory);
    SourceRuntimeSoundEventPort port(e.f.memory,e.reset,*e.frontend,e.output);
    const auto registration=[&](std::uint32_t site,std::uint32_t target,std::uint32_t returned,std::initializer_list<std::uint32_t> args){
        e.frame(0,args);e.f.abi.gp[3]=0x8000006b;put(e.f.memory,stack,returned);
        check(port.try_call(e.f.memory,{site,target,false},e.f.abi),"Timed fixture EventPort resource registration was not owned");
    };
    registration(0x3893bd,0x8000006b,0x3893bf,{apu+0x4c8,0x38918c,apu});
    registration(0x3893c8,0x80000071,0x3893ce,{apu+0x4e8,0});
    {
        Frontend foreign_frontend(e.f.memory,e.reset,e.dma,1);
        rejected([&]{Timed wrong_queue(e.f.memory,e.reset,foreign_frontend,port);});
    }
    Timed timed(e.f.memory,e.reset,*e.frontend,port);
    check(timed.state().timer_descriptor==port.state().timer_descriptor&&timed.state().native_owner.value>0xffffffffu,
        "Timed notifications duplicated their actual shared timer or truncated their native owner");
    e.frame(0,{apu+0x510,0x38901d,apu});e.f.abi.gp[3]=0x8000006b;put(e.f.memory,stack,0x3893dd);
    const auto dpc_before=hex(e.f.memory.view(apu+0x510,28));e.f.abi.gp[3]=0;
    rejected([&]{timed.try_call(e.f.memory,{0x3893db,0x8000006b,false},e.f.abi);});
    check(!timed.state().dpc_initialized&&hex(e.f.memory.view(apu+0x510,28))==dpc_before,"Wrong original DPC call register changed native/source registration");
    e.f.abi.gp[3]=0x8000006b;
    check(timed.try_call(e.f.memory,{0x3893db,0x8000006b,false},e.f.abi),"Actual native second DPC registration failed");
    Json operations=Json::array();
    const auto operation=[&](std::uint32_t entry,std::uint32_t channel,std::initializer_list<std::uint32_t> args,bool dpc=false) {
        e.frame(channel,args);Json row{{"entry",entry},{"input_gp",e.f.abi.gp},{"input_flags",e.f.abi.eflags},{"fs",e.f.abi.fs_base},{"before",snapshot(e.f.memory)}};
        Json platform=Json::array();const Timed::PlatformObserver observer=[&](const auto& p){platform.push_back({{"instruction",p.instruction},{"target",p.target},{"system_time",p.system_time},{"has_boolean_result",p.has_boolean_result},{"boolean_result",p.boolean_result}});};
        if(dpc)check(timed.dispatch_dpc(e.f.abi,{0x27f,0x1f80},e.f.services,e.f.observe,observer),"Actual timed deferred callback did not execute");
        else timed.source_notification(entry,e.f.abi,{0x27f,0x1f80},e.f.services,e.f.observe,observer);
        row["instructions"]=e.f.instructions;row["stores"]=e.f.stores;row["platform"]=platform;row["gp"]=e.f.abi.gp;row["flags"]=e.f.abi.eflags;row["after"]=snapshot(e.f.memory);operations.push_back(std::move(row));
    };
    const auto arm=[&](std::uint32_t channel,std::int64_t relative) {
        const auto value=std::uint64_t(relative);operation(0x38cbb9,channel,{4,std::uint32_t(value),std::uint32_t(value>>32),0});
        check(timed.state().armed&&timed.state().due_time>116444736000000000ull,"Original timed wrapper did not arm the actual shared timer");
    };
    const auto expire=[&] {
        pollfd fd{timed.state().timer_descriptor,POLLIN,0};check(poll(&fd,1,2000)==1&&(fd.revents&POLLIN),"Actual kernel sound timer did not expire");
        check(timed.collect_due()&&!timed.state().armed,"Actual kernel timer expiration was not consumed");
        const auto queued=e.frontend->pending_dpc();check(queued&&queued->routine==0x38901d&&queued->source_dpc==apu+0x510&&queued->native.value>0xffffffffu,
            "Actual kernel expiry did not enqueue its native64 callback in the existing frontend queue");
    };
    const auto duplicate=[&](bool queued) {
        const auto before=e.frontend->pending_dpc();std::unique_ptr<Timed> other;
        rejected([&]{other=std::make_unique<Timed>(e.f.memory,e.reset,*e.frontend,port);});other.reset();
        const auto after=e.frontend->pending_dpc();
        check(bool(before)==bool(after)&&(!before||before->identity==after->identity),"Rejected duplicate timer owner cancelled the first owner's actual queued receipt");
        if(!queued){itimerspec remaining{};check(timerfd_gettime(timed.state().timer_descriptor,&remaining)==0&&
            (remaining.it_value.tv_sec||remaining.it_value.tv_nsec),"Rejected duplicate timer owner cancelled the first owner's actual armed kernel timer");}
    };
    check(!timed.collect_due()&&!e.frontend->pending_dpc(),"Unarmed timer fabricated an actual source callback");
    arm(0x19010000,-300000);duplicate(false);expire();duplicate(true);operation(0x38901d,0,{apu+0x510,apu,0,0},true);
    check(!e.frontend->pending_dpc()&&timed.state().completed==1,"Whole original timed thunk did not retire its actual queued callback");
    // A pending native expiry can be cancelled by the complete original
    // wrapper before its callback executes. No inspection-only dequeue occurs.
    arm(0x19010000,-100000);expire();const auto before_cancel=e.frontend->state().cancelled_dpcs;
    operation(0x38cbe8,0x19010000,{4});
    check(!e.frontend->pending_dpc()&&e.frontend->state().cancelled_dpcs==before_cancel+1&&timed.state().completed==1,
        "Original timed cancel lost its actual queued callback receipt or invented completion");
    // Removing the first of two actual source records re-arms the same timer
    // for the remaining record through388C3D.
    arm(0x19010000,-10000000);arm(0x19010200,-20000000);
    operation(0x38cbe8,0x19010000,{4});check(timed.state().armed,"Head cancellation did not retain the remaining actual native timer");
    operation(0x38cbe8,0x19010200,{4});check(!timed.state().armed&&!timed.collect_due(),"Cancelled kernel timer retained a fabricated expiry");
    // Zero-time requests really execute the original rejection path.
    operation(0x38cbb9,0x19010000,{4,0,0,0});check(!timed.state().armed&&!e.f.abi.gp[0],"Original zero-time path armed an actual timer");
    bool foreign=false,foreign_fence=false;const auto control=e.reset.control_lease();
    std::thread thread([&]{try{timed.state();}catch(const std::exception&){foreign=true;}
        try{control.fence_retirement();}catch(const std::exception&){foreign_fence=true;}});thread.join();
    check(foreign&&foreign_fence,"Native timed owner read mutable state from a foreign thread");
    const auto commands=e.reset.state().completed_commands;e.frame(0x19010000,{3,0,0,0});
    rejected([&]{timed.source_notification(0x38cbb9,e.f.abi,{0x27f,0x1f80},e.f.services);});
    check(e.reset.state().completed_commands==commands,"Foreign notification index mutated actual source/control resources");
    // Reject unsupported playing semantics and foreign kernel timer state
    // before any source wrapper instruction or actual timer operation.
    e.frame(0x19010000,{4,0,0,0});put(e.f.memory,apu+0x50c,1);
    rejected([&]{timed.source_notification(0x38cbb9,e.f.abi,{0x27f,0x1f80},e.f.services);});put(e.f.memory,apu+0x50c,0);
    e.f.memory.write(0x19010012,std::array{std::byte{3}});
    rejected([&]{timed.source_notification(0x38cbb9,e.f.abi,{0x27f,0x1f80},e.f.services);});
    e.f.memory.write(0x19010012,std::array{std::byte{1}});
    e.f.memory.write(0x1901000c,std::array{std::byte{65},std::byte{}});
    rejected([&]{timed.source_notification(0x38cbb9,e.f.abi,{0x27f,0x1f80},e.f.services);});
    e.f.memory.write(0x1901000c,std::array{std::byte{64},std::byte{}});
    put(e.f.memory,apu+0x188,0);rejected([&]{timed.source_notification(0x38cbb9,e.f.abi,{0x27f,0x1f80},e.f.services);});put(e.f.memory,apu+0x188,0x19010000);
    check(!timed.state().armed&&e.reset.state().completed_commands==commands,"Rejected foreign timer/playing path changed actual resources");
    if(interrupt_callback) {
        arm(0x19010000,-100000);expire();e.frame(0,{apu+0x510,apu,0,0});
        const auto completed_before=e.frontend->state().completed_dpcs;bool stopped=false;auto services=e.f.services;
        services.instruction=[&](auto pc){if(pc==0x38d200){stopped=true;throw std::runtime_error("declared interruption before authentic index4 virtual callback");}};
        rejected([&]{timed.dispatch_dpc(e.f.abi,{0x27f,0x1f80},services,e.f.observe);});
        check(stopped&&timed.state().faulted&&!timed.state().armed&&port.state().faulted&&
            e.frontend->state().queued_dpcs==1&&e.frontend->state().completed_dpcs==completed_before,
            "Interrupted original timed callback fabricated completion or lost actual retained deferred work");
        check(!(std::to_integer<unsigned>(e.f.memory.view(0x19010110,1)[0])&2)&&
            hex(e.f.memory.view(0x19010012,1))=="01","Interrupted original timed callback rolled back source admission or invented playing state");
        timed.retire_generation();port.retire_generation();e.retire();return Json{{"interrupted_callback",true},{"queued_diagnostic_retained",true}};
    }
    Json result{{"fill",reference.at("case").at("fill")},{"operations",std::move(operations)},{"actual_expirations",timed.state().expirations},{"completed",timed.state().completed},
        {"same_event_port_timer",true},{"foreign_thread_rejected",foreign},{"queued_expiry_cancelled_without_callback",true},{"exclusive_timer_owner_verified",true}};
    timed.retire_generation();port.retire_generation();e.retire();return result;
}
void foreign_destructor(const Json& reference) {
    EventFixture e(reference);mount_native_tu15_sound_event_port_data(e.f.memory);
    SourceRuntimeSoundEventPort port(e.f.memory,e.reset,*e.frontend,e.output);
    e.frame(0,{apu+0x4e8,0});put(e.f.memory,stack,0x3893ce);
    check(port.try_call(e.f.memory,{0x3893c8,0x80000071,false},e.f.abi),"Foreign destruction fixture lacks its actual shared timer");
    auto timed=std::make_unique<Timed>(e.f.memory,e.reset,*e.frontend,port);
    std::set_terminate([]{std::_Exit(77);});std::thread other([owner=std::move(timed)]()mutable{owner.reset();});other.join();
    throw std::runtime_error("Foreign timed owner destructor bypassed its creating-thread contract");
}
}
int main(int argc,char** argv){try{
    if(argc==3&&std::string(argv[1])=="--foreign-destructor-probe") {
        std::ifstream input(argv[2]);Json reference;input>>reference;foreign_destructor(reference.at("cases").at(0));return 0;
    }
    check(argc==3,"Supply registered prerequisite reference and unique native output report");
    std::ifstream input(argv[1]);Json reference;input>>reference;Json cases=Json::array();
    for(const auto& row:reference.at("cases"))cases.push_back(run(row));
    const auto interrupted=run(reference.at("cases").at(0),true);
    std::ofstream output(argv[2]);output<<Json{{"passed",true},{"interruption_guard",interrupted},{"scope","Actual native shared timer/clock and unchanged whole original inactive index4 timed notification execution; original differential follows separately"},{"cases",std::move(cases)}}.dump(2)<<'\n';
    check(bool(output),"Failed to preserve actual native timed input/report");
    std::cout<<"Actual shared timerfd expiry, whole source38901D virtual callback, queued cancellation and source head re-arm passed for three fills\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
