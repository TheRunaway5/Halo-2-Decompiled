#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_execution_inputs.hpp"
#include <array>
#include <bit>
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
void native_write_lease(){
    using Reason=SourceRuntimeNativeWriteRefusal;
    SourceRuntimeMemory bank;
    bank.mount(0x10000,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"native scalar block");
    const std::array<SourceRuntimeNativeWriteRange,2> ranges{{{0x10100,12},{0x10800,4}}};
    const std::array<std::byte,4> word{std::byte{1},std::byte{2},std::byte{3},std::byte{4}};
    int guard_owner,observer_owner;unsigned guards{},observed{};
    SourceRuntimeWriteSerial serial(bank);
    {
        SourceRuntimeNativeWriteLease lease(bank,ranges);need(bool(lease)&&lease.refusal()==Reason::None,"Plain native write block refused");
        SourceRuntimeNativeWriteLease nested(bank,ranges);need(!nested&&nested.refusal()==Reason::PolicyHold,"Nested policy lease admitted or misattributed");
        need(refused([&]{SourceRuntimePolicyChange change(bank);}),"Native lease permitted a policy change");
        bool foreign_refused{};
        std::thread worker([&]{foreign_refused=refused([&]{source_runtime_register_write_guard(bank,&guard_owner,[](auto,auto){});});});worker.join();
        need(foreign_refused,"Native lease admitted a foreign policy publisher");
        bank.write(0x10100,word);
    }
    need(serial.load()==1,"Native write serial did not count the actual store");
    source_runtime_register_write_guard(bank,&guard_owner,[&](auto,auto){++guards;throw std::logic_error("guard");},0x80000000ull);
    {
        SourceRuntimeNativeWriteLease lease(bank,ranges);need(bool(lease),"Inert physical guard blocked ordinary native writes");
        bank.write(0x10100,word);need(guards==0,"Native write invoked an inert guard");
    }
    source_runtime_unregister_write_guard(bank,&guard_owner);
    source_runtime_register_write_guard(bank,&guard_owner,[&](auto,auto){++guards;throw std::logic_error("guard");});
    {SourceRuntimeNativeWriteLease lease(bank,ranges);need(!lease&&lease.refusal()==Reason::Guard&&guards==0,"Native admission executed or bypassed an access guard");}
    bool original_rejected{};try{bank.write(0x10100,word);}catch(const std::logic_error&){original_rejected=true;}
    need(original_rejected&&guards==1,"Literal write guard lost ownership after native refusal");
    source_runtime_unregister_write_guard(bank,&guard_owner);
    source_runtime_register_write_observer(bank,&observer_owner,[&](auto,auto){++observed;});
    {SourceRuntimeNativeWriteLease lease(bank,ranges);need(!lease&&lease.refusal()==Reason::Observer&&observed==0,"Native admission consumed an arbitrary observer");}
    const std::array<const void*,1> allowed{{&observer_owner}};
    {SourceRuntimeNativeWriteLease lease(bank,ranges,allowed);need(bool(lease),"Explicit semantic observer refused");bank.write(0x10100,word);}
    need(observed==1,"Allowed observer did not receive the native write");
    source_runtime_unregister_write_observer(bank,&observer_owner);
    source_runtime_register_write_observer(bank,&observer_owner,[&](auto,auto){
        SourceRuntimeNativeWriteLease recursive(bank,ranges,allowed);need(!recursive&&recursive.refusal()==Reason::Recursion,"Policy callback recursively acquired a native lease");});
    bank.write(0x10100,word);source_runtime_unregister_write_observer(bank,&observer_owner);
    {SourceRuntimePolicyChange change(bank);SourceRuntimeNativeWriteLease lease(bank,ranges);need(!lease&&lease.refusal()==Reason::PolicyChange,"Native lease overtook an admitted policy change");}
    {SourceRuntimePolicyHold hold(bank);SourceRuntimeNativeWriteLease lease(bank,ranges);need(!lease&&lease.refusal()==Reason::PolicyHold,"Native lease overtook a publication hold");}
    {SourceExecutionInputs inputs;{SourceExecutionInputBinding binding(bank,inputs);SourceRuntimeNativeWriteLease lease(bank,ranges);need(!lease&&lease.refusal()==Reason::PolicyChange,"Native lease ignored a recording/replay admission");}inputs.finish();}
    const std::array<SourceRuntimeNativeWriteRange,1> overflow{{{0xfffffffeu,4}}};
    {SourceRuntimeNativeWriteLease lease(bank,overflow);need(!lease&&lease.refusal()==Reason::Range,"Wrapping native write range admitted");}
    {SourceRuntimeNativeWriteLease lease(bank,{});need(!lease&&lease.refusal()==Reason::Range,"Empty native write block admitted");}
    {SourceRuntimeNativeWriteLease lease(bank,ranges);need(bool(lease),"Native policy refusal leaked its admission");}
}

void native_word_block(){
    using Visibility=SourceRuntimeWriteSerial::Visibility;
    constexpr std::uint32_t base=0x10000u,first=base+0xff8u;
    constexpr std::array<std::uint32_t,5> words{0x01234567u,0x89abcdefu,0u,0xff00aa55u,0x80000001u};
    constexpr std::byte poison{0xd7};
    SourceRuntimeMemory bank;
    bank.mount(base,std::vector<std::byte>(8192,poison),SourceRuntimeMemory::Access::ReadWrite,"native word block");
    SourceRuntimeWriteSerial serial(bank,Visibility::CallBoundary);
    if constexpr(std::endian::native!=std::endian::little) {
        need(!source_runtime_try_write_words(bank,first,words,serial) && serial.load()==0,"Native word block admitted host-endian bytes");
        return;
    }
    const auto initial=bank.checkpoint();
    const auto scalar=[&](std::uint32_t at,std::uint32_t value) {
        std::array<std::byte,4> bytes{};
        for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte(value>>(i*8u));
        bank.write(at,bytes);
    };
    for(unsigned i=0;i<words.size();++i)scalar(first+4u*i,words[i]);
    const auto expected=bank.checkpoint();
    bank.restore(initial);
    const auto before=serial.load();
    unsigned guard_calls{},observer_calls{};
    source_runtime_register_write_guard(bank,&guard_calls,[&](auto,auto){++guard_calls;},first+sizeof(words));
    source_runtime_register_write_observer(bank,&observer_calls,[&](auto,auto){++observer_calls;});
    need(!source_runtime_try_write_words(bank,first,words,serial) && bank.checkpoint()==initial &&
         serial.load()==before && !observer_calls && !guard_calls,"Unknown observer saw or admitted a bulk write");
    source_runtime_unregister_write_observer(bank,&observer_calls);
    source_runtime_register_write_observer(bank,&observer_calls,[&](auto address,auto) {
        if(address<first || address>=first+sizeof(words))++observer_calls;
    });
    const std::array<const void*,1> inert{{&observer_calls}};
    SourceRuntimeWriteSerial peer(bank,Visibility::CallBoundary);
    {
        SourceRuntimeWriteSerial extra(bank);
        need(!source_runtime_try_write_words(bank,first,words,serial,inert) && bank.checkpoint()==initial &&
             serial.load()==before && peer.load()==0 && extra.load()==0,
             "Native word block admitted an every-store subscriber");
    }
    SourceRuntimeMemory other;
    SourceRuntimeWriteSerial wrong(other,Visibility::CallBoundary);
    need(!source_runtime_try_write_words(bank,first,words,wrong,inert) && serial.load()==before,
         "Native word block admitted a serial from another bank");
    {
        SourceRuntimeMemory observed_bank;
        observed_bank.mount(first,std::vector<std::byte>(sizeof(words),poison),
            SourceRuntimeMemory::Access::ReadWrite,"every-store serial");
        SourceRuntimeWriteSerial ordinary(observed_bank);
        need(!source_runtime_try_write_words(observed_bank,first,words,ordinary) &&
             ordinary.load()==0 && observed_bank.view(first,1)[0]==poison,
             "A sole every-store subscriber admitted grouped writes");
    }
    {
        SourceExecutionInputs inputs;
        {SourceExecutionInputBinding binding(bank,inputs);
         need(!source_runtime_try_write_words(bank,first,words,serial,inert) && serial.load()==before,
              "Native word block admitted a bound execution tape");}
        inputs.finish();
    }
    need(source_runtime_try_write_words(bank,first,words,serial,inert) && bank.checkpoint()==expected &&
         serial.load()-before==words.size() && peer.load()==words.size() && !observer_calls && !guard_calls,
         "Native word block changed adjacent bytes, final word order or logical store count");
    source_runtime_unregister_write_observer(bank,&observer_calls);
    source_runtime_unregister_write_guard(bank,&guard_calls);

    bank.restore(initial);
    const auto before_fault=serial.load();
    const auto before_peer_fault=peer.load();
    constexpr auto stop=first+8u;
    source_runtime_register_write_guard(bank,&guard_calls,[&](auto address,auto) {
        ++guard_calls;
        if(address==stop)throw std::runtime_error("middle word guard");
    });
    need(!source_runtime_try_write_words(bank,first,words,serial) && bank.checkpoint()==initial &&
         serial.load()==before_fault && !guard_calls,"Bulk refusal lost the original guarded prefix");
    std::vector<std::uint32_t> observed;
    source_runtime_register_write_observer(bank,&observed,[&](auto address,auto width) {
        need(width==4,"Scalar continuation changed store width");observed.push_back(address);
    });
    bool rejected{};
    try{for(unsigned i=0;i<words.size();++i)scalar(first+4u*i,words[i]);}
    catch(const std::runtime_error& e){rejected=std::string_view(e.what())=="middle word guard";}
    need(rejected && guard_calls==3 && observed==std::vector<std::uint32_t>{first,first+4u} &&
         serial.load()-before_fault==2 && peer.load()-before_peer_fault==2 && bank.view(stop,1)[0]==poison,
         "Literal fallback changed the guarded prefix, observations or serial");
    source_runtime_unregister_write_observer(bank,&observed);
    source_runtime_unregister_write_guard(bank,&guard_calls);

    constexpr auto shared=0x20000u,split=0x30000u,reserved=0x40000u;
    auto backing=std::make_shared<std::array<std::uint32_t,8>>();
    bank.mount(shared,std::make_shared<SourceRuntimeMemoryBacking>(
        backing,std::as_writable_bytes(std::span(*backing))),SourceRuntimeMemory::Access::ReadWrite,"alias owner");
    bank.mount(split,std::vector<std::byte>(8),SourceRuntimeMemory::Access::ReadWrite,"first split owner");
    bank.mount(split+8u,std::vector<std::byte>(16),SourceRuntimeMemory::Access::ReadWrite,"second split owner");
    bank.reserve(reserved,8192,4096,"partly committed owner");
    bank.commit(reserved,std::vector<std::byte>(4096));
    const auto rejected_bytes=bank.checkpoint();
    const auto rejected_serial=serial.load();
    need(!source_runtime_try_write_words(bank,shared+4u,std::span(*backing).first(4),serial) &&
         !source_runtime_try_write_words(bank,split,words,serial) &&
         !source_runtime_try_write_words(bank,reserved+0xff8u,words,serial) &&
         !source_runtime_try_write_words(bank,0xfffffffcu,words,serial) &&
         !source_runtime_try_write_words(bank,first,{},serial) &&
         bank.checkpoint()==rejected_bytes && serial.load()==rejected_serial,
         "Alias, split owner, uncommitted extent, wrapping or empty input had a bulk write effect");
}
}
int main(){
    unsigned failed{};
    for(const auto &[name,run]:std::initializer_list<std::pair<const char *,void(*)()>>{
        {"input_binding",input_binding},{"backing_policy",backing_policy},{"registry",registry},{"active_admission",active_admission},
        {"native_write_lease",native_write_lease},{"native_word_block",native_word_block}}){
        try{run();std::cout<<"PASS "<<name<<'\n';}
        catch(const std::exception &e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}
    }
    return failed?1:0;
}
