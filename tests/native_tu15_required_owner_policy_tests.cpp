#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool value,const char*why){if(!value)throw std::runtime_error(why);}
struct Memory:SourcePhysicsContactMemory {
    unsigned operations{};
    void read(std::uint32_t,std::span<std::byte>)const override{throw std::runtime_error("Policy touched source memory");}
    void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Policy touched source memory");}
};
struct Forwarded{};
struct Impl {
    const std::thread::id main_thread;
    bool first_person_phase_active{},weapon_effect_phase_active{},raise{};
    unsigned calls{};Memory*memory{};SourcePhysicsContactRuntimeAbi*abi{};const SourcePhysicsContactRecordObserver*observer{};
    explicit Impl(std::thread::id thread=std::this_thread::get_id()):main_thread(thread){}
    void required(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&call,
        SourcePhysicsContactRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o){
        check(&m==memory&&&a==abi&&&o==observer&&call.instruction==0x178102u&&call.target==0x22aca0u&&!call.tail,"World forwarding ABI/observer identity differs");
        ++calls;a.gp[0]=0x12345678u;if(raise)throw Forwarded{};
    }
#include "world_required_owner.inc"
};
SourcePhysicsContactRuntimeServices services(Impl&world){
    SourcePhysicsContactRuntimeServices s;s.required_owner=Impl::WorldRequiredOwner{&world};
    s.required_owner_policy={&s.required_owner.target_type(),world.main_thread,&world,&Impl::WorldRequiredOwner::matches};return s;
}
}
int main()try{
    using native_required_owner_policy_detail::preserves_instruction_policy;
    unsigned checks{};Impl world,other,wrong_thread{std::thread::id{}};Memory memory;SourcePhysicsContactRuntimeAbi abi{};SourcePhysicsContactRecordObserver observer;
    world.memory=&memory;world.abi=&abi;world.observer=&observer;
    const auto good=services(world);
    auto accept=[&](const auto&s,const char*why){check(preserves_instruction_policy(s),why);++checks;};
    auto refuse=[&](const auto&s,const char*why){check(!preserves_instruction_policy(s),why);++checks;};
    accept(good,"Exact immutable World owner refused");auto copied=good;accept(copied,"Immutable World copy refused");auto moved=std::move(copied);accept(moved,"Immutable World move refused");
    auto s=good;s.required_owner_policy={};refuse(s,"Uncertified owner admitted");
    s=good;s.required_owner_policy.callback_type=&typeid(int);refuse(s,"Wrong callback type admitted");
    s=good;s.required_owner_policy.callback_type=nullptr;refuse(s,"Missing callback type admitted");
    s=good;s.required_owner_policy.executor={};refuse(s,"Wrong executor admitted");
    s=good;s.required_owner_policy.owner=&other;refuse(s,"Different owner admitted");
    s=good;s.required_owner_policy.owner=nullptr;refuse(s,"Missing owner admitted");
    s=good;s.required_owner_policy.matches=nullptr;refuse(s,"Missing matcher admitted");
    s=good;s.required_owner=Impl::WorldRequiredOwner{&other};refuse(s,"Copied certificate with another World admitted");
    s=good;s.required_owner=[original=good.required_owner](auto&m,const auto&c,auto&a,const auto&o){original(m,c,a,o);};refuse(s,"Wrapped World owner admitted");
    s=good;s.required_owner=[](auto&,const auto&,auto&,const auto&){};refuse(s,"Replaced owner admitted");
    s=services(wrong_thread);s.required_owner_policy.executor=std::this_thread::get_id();refuse(s,"Spoofed World executor admitted");
    world.first_person_phase_active=true;refuse(good,"Active trace admitted");world.first_person_phase_active=false;
    world.weapon_effect_phase_active=true;refuse(good,"Active trace admitted");world.weapon_effect_phase_active=false;accept(good,"Restored World trace refused");
    bool worker_refused{};std::thread worker([&]{auto local=good;local.required_owner_policy.executor=std::this_thread::get_id();worker_refused=!preserves_instruction_policy(local);});worker.join();check(worker_refused,"Cross-executor World admitted");++checks;
    s=good;s.required_owner={};accept(s,"Provider-free original stop refused");
    check(!world.calls&&!memory.operations,"Policy matching executed provider or memory");
    good.required_owner(memory,{0x178102u,0x22aca0u,false},abi,observer);check(world.calls==1&&abi.gp[0]==0x12345678u,"World forwarding changed live ABI");++checks;
    world.raise=true;bool raised{};try{good.required_owner(memory,{0x178102u,0x22aca0u,false},abi,observer);}catch(const Forwarded&){raised=true;}
    check(raised&&world.calls==2,"World forwarding changed exception envelope");++checks;
    std::cout<<nlohmann::json{{"passed",true},{"checks",checks},{"actual_world_struct",true},{"numerical_world_child_qualification",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
