#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_execution_inputs.hpp"
#include <future>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>

using namespace halo2;
namespace {
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>bool refused(F &&f){try{f();return false;}catch(const std::logic_error &e){
    return std::string_view(e.what()).starts_with("Native publication");}}

void input_binding(){
    SourceRuntimeMemory bank;
    SourceExecutionInputs plain,tracked;tracked.track_calls();
    bool plain_refused{},tracked_refused{};
    {
        SourceRuntimePolicyHold held(bank);
        plain_refused=refused([&]{SourceExecutionInputBinding binding(bank,plain);});
        tracked_refused=refused([&]{SourceExecutionInputBinding binding(bank,tracked);});
        need(!source_execution_inputs(bank),"Rejected tape binding left TLS state behind");
    }
    // The actual same tapes must remain usable: refusal cannot close, poison
    // or partially bind them while publication owns the bank.
    {SourceExecutionInputBinding binding(bank,plain);need(source_execution_inputs(bank)==&plain,"Plain tape did not resume");}
    {SourceExecutionInputBinding binding(bank,tracked);need(source_execution_inputs(bank)==&tracked,"Tracked tape did not resume");}
    plain.finish();tracked.finish();
    need(plain_refused && tracked_refused,"Held bank admitted an execution tape");
    // A foreign TLS binding is invisible to the acquiring thread's TLS map.
    // Its actual retained policy participation must still refuse publication.
    std::promise<void> entered,release;auto released=release.get_future();
    std::thread worker([&]{SourceExecutionInputs inputs;{
        SourceExecutionInputBinding binding(bank,inputs);entered.set_value();released.wait();}inputs.finish();});
    entered.get_future().wait();
    const bool rejected=refused([&]{SourceRuntimePolicyHold held(bank);});
    release.set_value();worker.join();
    need(rejected,"Publication ignored a foreign-thread live tape");
    SourceRuntimePolicyHold after(bank);
}

struct Retirement {
    SourceRuntimeMemory &bank;bool &refused_publication;
    Retirement(SourceRuntimeMemory &bank,bool &refused_publication):bank(bank),refused_publication(refused_publication){}
    ~Retirement(){refused_publication=refused([&]{SourceRuntimePolicyHold held(bank);});}
};
void backing_policy(){
    constexpr std::size_t bytes=1024*1024;
    SourceRuntimeMemory bank;
    bank.mount(0x20000000u,std::vector<std::byte>(bytes,std::byte{0x39}),SourceRuntimeMemory::Access::ReadWrite,"actual restored policy owner");
    const auto original=bank.checkpoint();bank.restore(original);
    const auto address=bank.native_address(0x20000000u,bytes);
    bool retirement_refused{};unsigned original_calls{},replacement_calls{};
    auto lifetime=std::make_shared<Retirement>(bank,retirement_refused);std::weak_ptr<Retirement> old=lifetime;
    auto backing=bank.prepare_snapshot_backing(original.regions.front(),4096,
        [lifetime,&original_calls](auto,auto){++original_calls;});
    need(bool(backing),"Required actual aligned restored snapshot was not eligible");
    backing.reset();lifetime.reset();
    need(!old.expired(),"Actual installed backing policy did not retain its capture");
    bool rejected{};
    {
        SourceRuntimePolicyHold held(bank);
        rejected=refused([&]{auto incoming=bank.prepare_snapshot_backing(original.regions.front(),4096,
            [&](auto,auto){++replacement_calls;});});
        need(!old.expired(),"Held backing replacement retired the installed policy capture");
    }
    need(rejected,"Held bank replaced an eligible same-backing write policy");
    bank.mutable_view(0x20000000u,1);
    need(original_calls==1 && !replacement_calls,"Rejected replacement changed actual write policy");
    auto incoming=bank.prepare_snapshot_backing(original.regions.front(),4096,[&](auto,auto){++replacement_calls;});
    need(bool(incoming) && old.expired() && retirement_refused,
        "Policy admission ended before outgoing capture retirement");
    bank.mutable_view(0x20000000u,1);
    need(original_calls==1 && replacement_calls==1 && bank.native_address(0x20000000u,bytes)==address && bank.checkpoint()==original,
        "Admitted policy replacement changed native address or complete bytes");
}

void registry(){
    SourceRuntimeMemory bank,other;int owner,added;
    using Register=void(*)(const SourceRuntimeMemory &,const void *,SourceRuntimeWriteGuard);
    using Unregister=void(*)(const SourceRuntimeMemory &,const void *);
    const Register install[]={source_runtime_register_write_guard,source_runtime_register_write_observer,
        source_runtime_register_topology_guard,source_runtime_register_topology_observer};
    const Unregister remove[]={source_runtime_unregister_write_guard,source_runtime_unregister_write_observer,
        source_runtime_unregister_topology_guard,source_runtime_unregister_topology_observer};
    bool retirement_refused{};
    auto lifetime=std::make_shared<Retirement>(bank,retirement_refused);std::weak_ptr<Retirement> weak=lifetime;
    for(auto fn:install)fn(bank,&owner,[lifetime](auto,auto){});
    lifetime.reset();
    for(auto fn:install){
        bool incoming_retirement_refused{},duplicate_refused{};
        auto incoming=std::make_shared<Retirement>(bank,incoming_retirement_refused);
        std::weak_ptr<Retirement> incoming_weak=incoming;
        try{fn(bank,&owner,[incoming=std::move(incoming)](auto,auto){});}
        catch(const std::logic_error &){duplicate_refused=true;}
        need(duplicate_refused && incoming_weak.expired() && incoming_retirement_refused && !weak.expired(),
            "Duplicate registration retired incoming captures outside policy admission or under registry lock");
    }
    {
        SourceRuntimePolicyHold held(bank);
        need(refused([&]{SourceRuntimePolicyHold duplicate(bank);}),"Nested policy hold accepted");
        for(unsigned i=0;i<4;++i){
            need(refused([&]{install[i](bank,&added,[](auto,auto){});}),"Held policy registry admitted a new callback");
            need(refused([&]{remove[i](bank,&owner);}),"Held policy registry removed a callback");
            need(!weak.expired(),"Held policy registry retired outgoing callbacks");
            install[i](other,&added,[](auto,auto){});remove[i](other,&added);
        }
        source_runtime_validate_write(bank,0x1000u,4);
        source_runtime_validate_topology(bank,0x1000u,4);
        source_runtime_observe_topology(bank,0x1000u,4);
    }
    for(auto fn:remove)fn(bank,&owner);
    need(weak.expired() && retirement_refused,
        "Registry admission ended before outgoing capture retirement");
}

void active_admission(){
    SourceRuntimeMemory bank;
    {
        SourceRuntimePolicyChange first(bank),second(bank);
        need(refused([&]{SourceRuntimePolicyHold held(bank);}),"Publication overtook an admitted policy change");
        need(refused([&]{SourceRuntimePolicyHold held(first);}),"Upgrade overtook a second admitted policy change");
    }
    {
        SourceRuntimePolicyChange installing(bank);
        SourceRuntimePolicyHold held(installing);
        need(refused([&]{SourceRuntimePolicyChange change(bank);}),"Atomic upgrade left an admission gap");
        need(refused([&]{SourceRuntimePolicyHold again(installing);}),"Consumed admission upgraded twice");
        SourceRuntimePolicyChange releasing(held);
        need(refused([&]{SourceRuntimePolicyChange again(held);}),"Consumed policy hold downgraded twice");
        bool foreign_refused{};
        std::thread worker([&]{foreign_refused=refused([&]{SourceRuntimePolicyHold next(bank);});});worker.join();
        need(foreign_refused,"Atomic downgrade admitted a competing publisher before retirement");
        // Actual ordinary guards can be retired while this admitted release
        // keeps competing publication out of the whole native teardown.
        int owner;bool retirement_refused{};
        auto lifetime=std::make_shared<Retirement>(bank,retirement_refused);
        source_runtime_register_write_guard(bank,&owner,[lifetime](auto,auto){});lifetime.reset();
        source_runtime_unregister_write_guard(bank,&owner);
        need(retirement_refused,"Downgraded release ended before actual guard-capture retirement");
    }
    {
        SourceRuntimePolicyHold held(bank);bool foreign_refused{};
        std::thread worker([&]{foreign_refused=refused([&]{SourceRuntimePolicyChange change(bank);});});worker.join();
        need(foreign_refused,"Foreign policy admission entered held bank");
    }
    std::promise<void> entered,release;auto released=release.get_future();
    std::thread worker([&]{SourceRuntimePolicyChange change(bank);entered.set_value();released.wait();});
    entered.get_future().wait();const bool rejected=refused([&]{SourceRuntimePolicyHold held(bank);});
    release.set_value();worker.join();
    need(rejected,"Publication overtook an active foreign policy admission");
    SourceRuntimePolicyHold after(bank);
}
}
int main(){
    unsigned failed{};
    for(const auto &[name,run]:std::initializer_list<std::pair<const char *,void(*)()>>{
        {"input_binding",input_binding},{"backing_policy",backing_policy},{"registry",registry},{"active_admission",active_admission}}){
        try{run();std::cout<<"PASS "<<name<<'\n';}
        catch(const std::exception &e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}
    }
    return failed?1:0;
}
