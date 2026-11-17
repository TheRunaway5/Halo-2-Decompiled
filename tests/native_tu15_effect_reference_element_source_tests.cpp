#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "rendering/effects/native_tu15_effect_reference_element_admission.hpp"
#include "effect_reference_source_includes.inc"
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using namespace halo2::mopp_ray_frame_test;
WorldState*effect_reference_live{};
unsigned effect_reference_statements{},effect_reference_parent_statements{};bool effect_reference_stop{};
struct RetainedEntry{};
void effect_reference_statement(std::uint32_t entry){
    ++effect_reference_statements;if(entry==0x178070u||entry==0x13a6b0u)++effect_reference_parent_statements;
    if(const auto*p=std::getenv("HALO2_EFFECT_REFERENCE_FAIL_ON_RETAINED");p&&*p)throw std::runtime_error("native source ran retained statements");
    if(effect_reference_stop)throw RetainedEntry{};
}
#include "effect_reference_source_bodies.inc"
namespace {
struct ObservedMemory:Memory {
    std::function<J()>capture;mutable J packets=J::array();mutable std::optional<std::uint32_t>previous;
    std::optional<std::uint32_t>atomic_pc;unsigned atomic_ordinal{};mutable unsigned atomic_seen{};
    void before()const{
        if(previous==pc)return;
        packets.push_back(capture());previous=pc;
        if(atomic_pc==pc&&atomic_seen++==atomic_ordinal)throw Boundary{};
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{before();Memory::read(address,out);}
    void write(std::uint32_t address,std::span<const std::byte>value)override{before();Memory::write(address,value);}
};
SourcePhysicsWorldRuntimeAbi incoming(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");a.fs_base=0x5a556677u;
    a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto data=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],data.data(),10);}return a;
}
struct AccessorContext {
    ObservedMemory*memory;const SourcePhysicsWorldRuntimeServices*services;
    const SourcePhysicsContactRecordObserver*observe;unsigned*children;unsigned*calls;
};
// This exact fixture owner computes only the three childless accessors. Its
// implementation never changes instruction/cursor/trace policy. The witness
// binds that audited body and its actual context, including immutable copies.
struct ReadonlyAccessorOwner {
    AccessorContext*context;
    void operator()(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&call,
        SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o)const{
        auto&c=*context;check(&m==c.memory&&&o==c.observe&&!call.tail,"Actual required-owner identity differs");++*c.children;++*c.calls;
        check(call.target==0x22b790u||call.target==0x22aca0u||call.target==0x22b710u,"Source closure escaped the three explicit genuine accessor children");
        WorldState child{m,o,*c.services,SourcePhysicsContactImage::TitleUpdate15};child.load(a);auto*parent=effect_reference_live;effect_reference_live=&child;
        auto before=[](auto&,auto){};auto forbidden=[](auto&,auto,auto){throw std::runtime_error("Leaf accessor supplied an unexpected child");};
        try{native_effect_reference_element_detail::run_private(child,call.target,before,forbidden);}catch(...){a=child.save();effect_reference_live=parent;throw;}
        a=child.save();effect_reference_live=parent;
    }
    static bool matches(const SourcePhysicsContactRuntimeServices::RequiredOwner&callback,const void*owner)noexcept{
        const auto*body=callback.target<ReadonlyAccessorOwner>();return body&&body->context==owner;
    }
};
struct ChangedInstruction {
    std::uint32_t*pc;unsigned*count;J*log;
    void operator()(std::uint32_t value)const{*pc=value;++*count;log->push_back(value);}
};
struct MutatingAccessorOwner {
    ReadonlyAccessorOwner child;SourcePhysicsWorldRuntimeServices*services;
    bool*trace;bool*mutated;std::uint32_t*pc;unsigned*callbacks;J*log;std::size_t*packet_begin;unsigned mutation;
    void operator()(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&call,
        SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o)const{
        child(m,call,a,o);
        if(!*mutated)*packet_begin=child.context->memory->packets.size();
        *mutated=true;
        switch(mutation){
        case 0:services->instruction={};break;
        case 1:services->instruction=ChangedInstruction{pc,callbacks,log};break;
        case 2:*trace=true;break;
        case 3:services->instruction_cursor.position=nullptr;break;
        case 4:services->instruction_cursor.literal_service_sites=false;break;
        }
    }
};
unsigned guards(){
    unsigned count{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    for(auto route:effect_reference_routes)for(unsigned policy=0;policy<20;++policy){
        ObservedMemory memory;SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;bool trace{};unsigned callbacks{};
        memory.capture=[]()->J{throw std::runtime_error("Uncertified owner admitted");};
        services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};
        services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        SourcePhysicsWorldRuntimeAbi a{};a.gp={1,2,3,4,0x62000100,6,7,8};a.eflags=0x246;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};
        unsigned children{},calls{};AccessorContext owner{&memory,&services,&observe,&children,&calls},other=owner;
        if(policy>=12){services.required_owner=ReadonlyAccessorOwner{&owner};services.required_owner_policy={&services.required_owner.target_type(),std::this_thread::get_id(),&owner,&ReadonlyAccessorOwner::matches};}
        switch(policy){
        case 0:services.instruction=[](auto){};break;
        case 1:services.instruction_cursor.position=nullptr;break;
        case 2:services.instruction_cursor.callback_type=nullptr;break;
        case 3:services.instruction_cursor.executor={};break;
        case 4:services.instruction_cursor.literal_service_sites=false;break;
        case 5:trace=true;break;
        case 6:services.instruction_cursor.callback_type=&typeid(int);break;
        case 7:services.instruction_cursor.observation_only_stores=false;observe=[](const auto&){};break;
        case 8:services.instruction_cursor.first_trace=nullptr;services.instruction_cursor.second_trace=&trace;trace=true;break;
        case 9:services.instruction_cursor.service_sites={};break;
        case 10:a.x87_status|=0x8080;break;
        case 11:services.instruction={};break;
        case 12:services.required_owner_policy={};break;
        case 13:services.required_owner_policy.callback_type=&typeid(int);break;
        case 14:services.required_owner_policy.executor={};break;
        case 15:services.required_owner_policy.owner=&other;break;
        case 16:services.required_owner_policy.matches=nullptr;break;
        case 17:{auto original=services.required_owner;services.required_owner=[original](auto&m,const auto&c,auto&a,const auto&o){original(m,c,a,o);};break;}
        case 18:services.required_owner_policy.callback_type=nullptr;break;
        case 19:services.required_owner_policy.matches=+[](const SourcePhysicsContactRuntimeServices::RequiredOwner&,const void*)noexcept{return false;};break;
        }
        // Empty services without an observer are a valid native entry. Keep
        // an explicit unqualified observer to require unchanged fallback.
        if(policy==11)observe=[](const auto&){};
        WorldState before{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};before.load(a);const auto old=packet(before,0);
        effect_reference_stop=true;effect_reference_statements=0;bool refused{};
        try{route(memory,observe,services,a,0x178070u);}catch(const RetainedEntry&){refused=true;}
        effect_reference_stop=false;WorldState after{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};after.load(a);
        check(refused&&effect_reference_statements==1&&!callbacks&&memory.operations.empty()&&packet(after,0)==old,"Source refusal changed ABI or reached providers");++count;
    }
    return count;
}
}
int main(int argc,char**argv)try{
    if(argc==2&&std::string_view(argv[1])=="--guards"){std::cout<<J{{"passed",true},{"untouched_source_refusals",guards()}}.dump()<<'\n';return 0;}
    const bool empty=argc==3&&std::string_view(argv[1])=="--empty-services";
    const bool policies=argc==3&&std::string_view(argv[1])=="--provider-policy-mutations";
    check(argc==2||empty||policies,"Supply original effect-reference source fixture");std::ifstream input(argv[argc==3?2:1]);const auto fixture=J::parse(input);
    const bool forced=std::getenv("HALO2_REFERENCE_EFFECT_REFERENCE_ELEMENT");unsigned comparisons{},packets{},children{},statements{},flag_differences{};
    J policy_results=J::array();
    for(const auto&row:fixture.at("cases"))for(unsigned mutation=0;mutation<(policies?5u:1u);++mutation)for(unsigned route=0;route<effect_reference_routes.size();++route){
        if(!effect_reference_local_entries[route].contains(row.at("entry")))continue;
        if(policies&&row.at("id")!=3u)continue;
        if(policies&&effect_reference_external_entries[route].empty())continue;
        const auto phase=row.at("phase").get<std::string>();if(forced&&phase!="normal")continue;
        if(empty&&phase=="before_instruction")continue;
        ObservedMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.hostile=!forced&&!policies;
        if(phase=="before_read")memory.read_fault=row.at("native_fault_ordinal");
        if(phase=="before_write")memory.write_fault=row.at("native_fault_ordinal");
        if(phase=="before_instruction"){memory.atomic_pc=row.at("native_fault_instruction");memory.atomic_ordinal=row.at("native_fault_ordinal");}
        SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;bool trace{},mutated{};unsigned callbacks{},stores{},provider_calls{};J instruction_log=J::array();std::size_t packet_begin{};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        if(!empty){services.instruction=[&](auto pc){memory.pc=pc;++callbacks;if(mutated)instruction_log.push_back(pc);};services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};}
        if(phase=="after_write"){
            memory.observer_preimages=true;observe=[&](const auto&event){const auto&expected=row.at("writes").at(stores);
                check(event.instruction==expected.at("instruction")&&event.address==expected.at("address")&&hex(event.before)==expected.at("before").template get<std::string>()&&hex(event.written)==expected.at("written").template get<std::string>(),"Actual source store observer differs");
                if(stores++==row.at("native_fault_ordinal"))throw Boundary{};};
        }
        if(empty&&phase=="after_write")continue;
        memory.capture=[&]{check(effect_reference_live,"Missing actual source ABI owner");return packet(*effect_reference_live,memory.pc);};
        AccessorContext owner{&memory,&services,&observe,&children,&provider_calls};
        services.required_owner=ReadonlyAccessorOwner{&owner};
        services.required_owner_policy={&services.required_owner.target_type(),std::this_thread::get_id(),&owner,&ReadonlyAccessorOwner::matches};
        if(policies)services.required_owner=MutatingAccessorOwner{ReadonlyAccessorOwner{&owner},&services,&trace,&mutated,&memory.pc,&callbacks,&instruction_log,&packet_begin,mutation};
        else if(row.at("id").get<unsigned>()%2u){auto copy=services.required_owner;services.required_owner=copy;}
        auto a=incoming(row);const auto fs=a.fs_base;effect_reference_statements=0;effect_reference_parent_statements=0;bool stopped{};std::string exception;
        try{effect_reference_routes[route](memory,observe,services,a,row.at("entry"));}catch(const Boundary&){stopped=true;}
        catch(const std::exception&e){if(!policies)throw;stopped=true;exception=std::string(typeid(e).name())+":"+e.what();}
        const auto name=row.at("name").get<std::string>()+" source="+std::to_string(route)+(empty?" empty":"");
        if(policies){
            check(provider_calls&&mutated&&effect_reference_parent_statements,name+": uncertified mutating provider did not retain whole parent");
            WorldState result{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};result.load(a);
            J banks=J::array();for(const auto&bank:row.at("banks"))banks.push_back(hex(memory.banks.at(bank.at("address"))));
            J tail=J::array();for(std::size_t n=packet_begin;n<memory.packets.size();++n)tail.push_back(memory.packets[n]);
            policy_results.push_back(J{{"route",route},{"mutation",mutation},{"abi",packet(result,memory.pc)},{"reads",memory.architectural_reads()},{"writes",memory.writes},{"post_child_packets",tail},{"banks",banks},{"after_child_callbacks",instruction_log},{"required_entries",provider_calls},{"exception",exception},{"retained_parent",true}});continue;
        }
        check(stopped==(phase!="normal"),name+": completion differs");check(a.fs_base==fs,name+": FS base changed");
        unsigned expected_providers{};
        for(const auto&call:row.at("calls"))expected_providers+=effect_reference_external_entries[route].contains(call.at("entry"));
        if(effect_reference_external_entries[route].contains(row.at("entry")))--expected_providers;
        check(provider_calls==expected_providers,name+": genuine required-owner entry count differs");
        WorldState result{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};result.load(a);
        auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("boundary_live_x87")}};
        auto actual=packet(result,row.at("expected_pc"));
        if(forced&&actual["eflags"]!=expected["eflags"]){
            const auto difference=actual["eflags"].get<unsigned>()^expected["eflags"].get<unsigned>();
            check(!(difference&~0x810u),name+": retained defined flags differ");++flag_differences;actual["eflags"]=expected["eflags"];
        }
        check(actual==expected,name+": source boundary ABI differs actual="+actual.dump()+" expected="+expected.dump());
        auto reads=row.at("reads"),writes=row.at("writes"),calls=row.at("calls");
        if(empty){for(auto&read:reads)read["instruction"]=0u;for(auto&write:writes)write["instruction"]=0u;for(auto&call:calls)call["entry"]=0u;}
        check(memory.architectural_reads()==reads,name+": source ordered reads differ");check(memory.writes==writes,name+": source ordered writes differ");
        if(!forced){check(!effect_reference_statements&&!callbacks,name+": native source ran retained statements");
            if(!empty){check(memory.packets==calls,name+": source full packets differ");packets+=memory.packets.size();}}
        else{check(effect_reference_statements&&(route%2u?callbacks==0u:callbacks>0u),name+": positive reference did not execute actual retained owner/protocol");statements+=effect_reference_statements;}
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": source bank differs");++comparisons;
    }
    if(policies){check(policy_results.size()==50,"Expected five genuine mutating owners across ten externally calling source contexts");std::cout<<J{{"passed",true},{"provider_policy_rows",policy_results.size()},{"results",policy_results}}.dump()<<'\n';return 0;}
    std::cout<<J{{"passed",true},{"actual_source_transfers",comparisons},{"complete_pre_io_packets",packets},{"genuine_required_owner_entries",children},{"own_retained_statements",statements},{"retained_undefined_flag_differences",flag_differences},{"source_variants",effect_reference_routes.size()},{"production_binding",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
