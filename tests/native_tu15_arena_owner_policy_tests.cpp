#include "halo2/native_tu15_arena_data.hpp"
#include "halo2/physics_game_arena_native.hpp"
#include "physics/runtime/native_tu15_arena_schedule_policy.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_arena_publication.hpp"
#include <type_traits>
#include <atomic>
#include <cstring>
#include <iostream>
#include <optional>
#include <thread>
#if defined(HALO2_ARENA_SOURCE_ROUTES)
#include "physics/runtime/native_tu15_arena_initialize_owner.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include "halo2/source_runtime_write_protection.hpp"
#include <unistd.h>
#endif

namespace halo2 {
namespace {
thread_local std::optional<SourcePhysicsWorldRuntimeServices> captured;
thread_local const void*captured_owner{};
#if defined(HALO2_ARENA_SOURCE_ROUTES)
thread_local unsigned io_calls{},retained_statements{};
thread_local bool stop_at_retained{};
thread_local std::function<void(SourcePhysicsWorldRuntimeServices&)>before_parent;
thread_local SourcePhysicsGameArenaNative*expected_live_owner{};
thread_local bool expected_live_qualification{};
thread_local unsigned live_publications{};
thread_local unsigned nested_publications{};
thread_local const SourcePhysicsWorldRuntimeServices*published_table{};
thread_local bool check_callback_free_publication{};
struct RetainedEntryStop{};
#endif
}
// Added only to the owned disposable producer copy by the private validator.
void native_arena_policy_probe(const SourcePhysicsWorldRuntimeServices&s,const void*owner){captured=s;captured_owner=owner;}
#if defined(HALO2_ARENA_SOURCE_ROUTES)
void native_arena_policy_io(bool){++io_calls;if(check_callback_free_publication&&published_table&&(published_table->instruction||published_table->instruction_cursor.position||published_table->instruction_cursor.literal_service_sites))throw std::runtime_error("Callback-free published table changed through a registry alias");}
void native_arena_policy_before_parent(const SourcePhysicsWorldRuntimeServices&s){
    if(expected_live_owner){
        published_table=&s;
        const auto&memory=expected_live_owner->memory();
        if(native_arena_publication_detail::Borrow::qualified(memory,s)!=expected_live_qualification)throw std::runtime_error("Actual live Arena publication domain differs");
        if(expected_live_qualification){
            SourceRuntimeMemory foreign;if(native_arena_publication_detail::Borrow::qualified(foreign,s))throw std::runtime_error("Foreign Arena memory accepted in live scope");auto copy=s;if(native_arena_publication_detail::Borrow::qualified(memory,copy))throw std::runtime_error("Copied Arena table accepted in live scope");
            if(!nested_publications){
                ++nested_publications;SourcePhysicsWorldRuntimeAbi nested{};nested.gp[2]=0x56e258u;nested.gp[4]=0x6000e000u;nested.eflags=0x202;nested.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
                const std::array<std::byte,4>ret{std::byte{0},std::byte{0},std::byte{0},std::byte{0x63}};expected_live_owner->memory().write(nested.gp[4],ret);expected_live_owner->run(0x165700u,nested);
                if(!native_arena_publication_detail::Borrow::qualified(memory,s))throw std::runtime_error("Nested Arena publication lost exact outer table");
            }
        }
        ++live_publications;
    }
    if(before_parent)before_parent(const_cast<SourcePhysicsWorldRuntimeServices&>(s));
}
void native_arena_policy_retained(std::uint32_t){++retained_statements;if(stop_at_retained)throw RetainedEntryStop{};}
#endif
}
using namespace halo2;
static_assert(!std::is_copy_constructible_v<native_arena_publication_detail::Borrow>);
static_assert(!std::is_default_constructible_v<native_arena_publication_detail::Borrow>);
static_assert(!std::is_constructible_v<native_arena_publication_detail::Borrow,const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeServices&,const void*,bool(*)(const SourcePhysicsWorldRuntimeServices&,const void*)noexcept>);
namespace {
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
struct LowerStop{};
struct LowerOwner {
    const unsigned*const owner;
    void operator()(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
                    SourcePhysicsWorldRuntimeAbi&,const SourcePhysicsContactRecordObserver&)const {throw LowerStop{};}
    static bool matches(const SourcePhysicsWorldRuntimeServices::RequiredOwner&f,const void*expected)noexcept {
        const auto*actual=f.target<LowerOwner>();return actual&&actual->owner==expected;
    }
};
SourcePhysicsWorldRuntimeServices certificate(unsigned&owner){
    SourcePhysicsWorldRuntimeServices s;s.required_owner=LowerOwner{&owner};
    s.required_owner_policy={&s.required_owner.target_type(),std::this_thread::get_id(),&owner,&LowerOwner::matches};return s;
}
void put(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>raw{};for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(n*8));m.write(p,raw);}
#if defined(HALO2_ARENA_SOURCE_ROUTES)
bool same_abi(const SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsWorldRuntimeAbi&b){
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.fs_base!=b.fs_base||a.xmm!=b.xmm||
       a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||bool(a.active_mmx)!=bool(b.active_mmx)||
       bool(a.floating_point)!=bool(b.floating_point))return false;
    if(a.floating_point&&(a.floating_point->x87_control_word!=b.floating_point->x87_control_word||a.floating_point->mxcsr!=b.floating_point->mxcsr))return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;
    return true;
}
unsigned source_checks{},fallbacks{},native_warm{},reference_warm{};
void source_route(SourcePhysicsGameArenaNative&owner,SourceRuntimeMemory&bank,bool owner_expected,unsigned policy){
    auto services=*captured;std::uint32_t position=0x12345678;bool first{},second{};unsigned callbacks{},traced_callbacks{},observations{};
    constexpr std::array correct{0x38846eu,0x21f590u,0x33b595u};const std::array wrong{0x38846eu,0x21f590u,0x33b596u};
    SourcePhysicsContactRecordObserver observe;
    if(policy&&policy!=15){
        services.instruction=[&](auto pc){position=pc;++callbacks;if(first)++traced_callbacks;};
        services.instruction_cursor={&position,&first,&second,correct,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    }
    if(policy==9||policy==10)observe=[&](const auto&){++observations;};
    before_parent=[&](auto&live){switch(policy){
        case 2:live.instruction={};break;
        case 3:live.instruction=[&](auto){++callbacks;};break;
        case 4:live.instruction_cursor.position=nullptr;break;
        case 5:live.instruction_cursor.executor={};break;
        case 6:live.instruction_cursor.service_sites=wrong;break;
        case 7:first=true;break;
        case 8:live.instruction_cursor.literal_service_sites=false;break;
        case 9:live.instruction_cursor.observation_only_stores=false;break;
        case 11:live.instruction={};live.instruction_cursor={};break;
        case 12:second=true;break;
        default:break;
    }};
    const bool forced=std::getenv("HALO2_REFERENCE_ARENA_INITIALIZE")&&*std::getenv("HALO2_REFERENCE_ARENA_INITIALIZE");
    // Callback-bearing tables need a separate registry/cursor stability owner.
    // Exact memory identity alone cannot certify their mutable trace captures.
    const bool native_expected=owner_expected&&(policy==0||policy==15)&&!forced;
    const bool normal_reference=owner_expected&&(policy==0||policy==1||policy==10||policy==14||policy==15)&&forced;
    SourceRuntimeMemory foreign;SourcePhysicsContactMemory*memory=&owner.memory();
    if(policy==13){mount_native_tu15_arena_data(foreign);foreign.mount(0x60000000u,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"foreign policy caller");memory=&foreign;}
    const std::byte initialized{1};memory->write(0x4f0430u,std::span(&initialized,1));put(*memory,0x6000f000u,0x63000000u);
    struct Guard {
        SourceRuntimeMemory*bank{};
        ~Guard(){if(bank)source_runtime_unregister_write_guard(*bank,this);}
    }guard;
    bool external_rebound{};
    if(policy==14||policy==15){guard.bank=&bank;source_runtime_register_write_guard(bank,&guard,[&](auto address,auto){if(address==0x6000effcu){if(policy==14)first=true;else{services.instruction=[&](auto){++callbacks;};services.instruction_cursor={&position,&first,&second,correct,std::this_thread::get_id(),&services.instruction.target_type(),true,true};external_rebound=true;}}});}
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x11111111u,0x22222222u,0x33333333u,0x44444444u,0x6000f000u,0x55555555u,0x66666666u,0x77777777u};abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    const auto original=abi;const auto calls=native_arena_initialize_owner_detail::activity();io_calls=retained_statements=0;stop_at_retained=!normal_reference;bool stopped{};
    if(policy==0||policy==15){expected_live_owner=&owner;expected_live_qualification=owner_expected;}
    check_callback_free_publication=policy==15;
    try{if(policy==0||policy==15)owner.run(0x125280u,abi);else source_run_physics_game_arena_runtime(*memory,0x125280u,abi,SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},services,observe);}
    catch(const RetainedEntryStop&){stopped=true;}
    before_parent={};expected_live_owner=nullptr;check_callback_free_publication=false;published_table=nullptr;
    check(!native_arena_publication_detail::Borrow::qualified(owner.memory(),services),"Returned or throwing Arena publication remained active");
    const bool owns=native_arena_initialize_owner_detail::activity()==calls+1;
    if(policy==14)std::cerr<<"memory-policy-revocation first="<<first<<" native="<<owns<<" retained="<<retained_statements<<" traced_callbacks="<<traced_callbacks<<" io="<<io_calls<<'\n';
    if(policy==15){check(external_rebound&&services.instruction&&callbacks==0,"Actual registry rebind escaped or failed its separate table copy");std::cerr<<"callback-free-publication-alias external_rebound=1 actual_callbacks="<<callbacks<<" native="<<owns<<" retained="<<retained_statements<<'\n';}
    if(native_expected){
        check(!stopped&&owns&&retained_statements==0,"Genuine warm default failed to own complete source route");
        check(abi.gp[4]==original.gp[4]+4u&&abi.gp[1]==original.gp[1]&&abi.gp[0]==0x11111101u&&callbacks==0,"Genuine warm native continuation differs");++native_warm;
    }else if(normal_reference){
        check(!stopped&&!owns&&retained_statements==6,"Positive actual retained warm route did not execute all six statements");++reference_warm;
    }else{
        check(stopped&&!owns&&retained_statements==1,"Source fallback skipped first retained statement");
        check(io_calls==0&&callbacks==0&&observations==0&&same_abi(original,abi),"Refused source route changed actual ABI or performed source work");++fallbacks;
    }
    if(native_expected||normal_reference){auto expected=original;expected.gp[0]=0x11111101u;expected.gp[4]+=4u;check(same_abi(expected,abi),"Complete actual warm ABI differs");}
    ++source_checks;
}
nlohmann::json actual_cold_source(){
    using J=nlohmann::json;const auto scratch=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"));auto name=(scratch/"actual-cold-XXXXXX").string();check(::mkdtemp(name.data()),"Fresh cold owner directory unavailable");
    struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{name};
    SourceRuntimeMemory bank;mount_native_tu15_arena_data(bank);bank.mount(0x60000000u,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"actual cold parent caller");
    const bool uncontrolled=std::getenv("HALO2_ARENA_UNCONTROLLED_COLD")&&*std::getenv("HALO2_ARENA_UNCONTROLLED_COLD");
    const auto barrier=std::make_shared<native_arena_schedule_detail::WorkerJobBarrier>();
    auto schedule=uncontrolled?SourcePhysicsGameArenaSchedule{}:native_arena_schedule_detail::feedback_schedule(barrier);
    SourcePhysicsGameArenaNative owner{bank,{name,0x60100000u,{0x27f,0x1f80},schedule}};
    for(auto entry:{0x12cb70u,0x1221b0u}){SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=0x6000f000u;a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(owner.memory(),a.gp[4],0x63000000u);owner.run(entry,a);}
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x11111111u,0x22222222u,0x33333333u,0x44444444u,0x6000f000u,0x55555555u,0x66666666u,0x77777777u};abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(owner.memory(),abi.gp[4],0x63000000u);
    before_parent={};stop_at_retained=false;retained_statements=io_calls=0;const auto activity=native_arena_initialize_owner_detail::activity();owner.run(0x125280u,abi);const auto statements=retained_statements;const bool native=native_arena_initialize_owner_detail::activity()==activity+1;
    const bool forced=std::getenv("HALO2_REFERENCE_ARENA_INITIALIZE")&&*std::getenv("HALO2_REFERENCE_ARENA_INITIALIZE");check(forced?!native&&statements==1046573u:native&&statements==0u,"Actual cold source default/reference ownership differs");
    owner.stop();const auto resources=owner.ownership_checkpoint();check(resources.files.size()==2&&resources.physical.allocations.size()==1&&resources.synchronization.objects.size()==3,"Actual cold resource ownership differs");
    const auto digest=[](std::span<const std::byte>bytes){std::array<unsigned char,EVP_MAX_MD_SIZE>hash{};unsigned count{};check(EVP_Digest(bytes.data(),bytes.size(),hash.data(),&count,EVP_sha256(),nullptr)==1,"Cold bank digest failed");constexpr char hex[]="0123456789abcdef";std::string result;for(unsigned n=0;n<count;++n){result+=hex[hash[n]>>4];result+=hex[hash[n]&15];}return result;};
    const auto bank_directory=scratch/(forced?"source-reference-banks":"source-native-banks");std::filesystem::create_directories(bank_directory);
    J banks=J::array();bank.visit_committed([&](auto address,auto bytes){if(address>=0x60000000u&&address<0x60200000u)return;banks.push_back(J{{"address",address},{"bytes",bytes.size()},{"sha256",digest(bytes)}});std::ofstream raw(bank_directory/std::to_string(address),std::ios::binary);raw.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());check(bool(raw),"Owned source bank diagnostic failed");});
    J files=J::array();for(unsigned n=0;n<2;++n){const auto bytes=std::filesystem::file_size(std::filesystem::path(name)/("savegame"+std::to_string(n)+".bin"));check(bytes==4239360u,"Actual cold file task extent differs");files.push_back(bytes);}
    check(abi.gp[4]==0x6000f004u&&!abi.active_mmx&&abi.x87_depth==0,"Actual cold parent ABI incomplete");
    return J{{"native_default",native},{"retained_parent_statements",statements},{"declared_feedback_schedule",!uncontrolled},{"gp",abi.gp},{"eflags",abi.eflags},{"xmm",abi.xmm},{"fp",J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})},{"banks",banks},{"real_file_bytes",files},{"retired_worker",true}};
}
#endif
}
int main()try {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch&&*scratch,"Use disposable owner");
    const std::filesystem::path directory=std::filesystem::path(scratch)/"policy-files";std::filesystem::create_directory(directory);
    unsigned lower_identity{},foreign_identity{},checks{},refusals{};
    for(unsigned mode=0;mode<10;++mode){
        SourceRuntimeMemory bank;mount_native_tu15_arena_data(bank);bank.mount(0x60000000u,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"private Arena policy caller");
        auto lower=mode==0?SourcePhysicsWorldRuntimeServices{}:certificate(lower_identity);
        SourcePhysicsGameArenaSchedule schedule;
        if(mode==2)lower.required_owner=[](auto&,const auto&,auto&,const auto&){};
        if(mode==3)schedule.before_worker_wait=[](auto,auto){};
        if(mode==4)schedule.before_main_yield=[]{};
        const auto idle=std::make_shared<native_arena_schedule_detail::WorkerIdleBarrier>();
        if(mode==5)schedule.before_worker_wait=native_arena_schedule_detail::WorkerIdleBeforeWait{idle};
        if(mode==6)schedule.before_worker_wait=[idle](auto handle,auto){idle->before_wait(handle);};
        if(mode==7)lower.required_owner_policy.owner=&foreign_identity;
        if(mode==8||mode==9){const auto barrier=std::make_shared<native_arena_schedule_detail::WorkerJobBarrier>();schedule=native_arena_schedule_detail::feedback_schedule(barrier);if(mode==9)schedule.before_main_yield=native_arena_schedule_detail::WorkerJobBeforeYield{std::make_shared<native_arena_schedule_detail::WorkerJobBarrier>()};}
        SourcePhysicsGameArenaNative owner{bank,{directory,0x60100000u,{0x27f,0x1f80},schedule},lower};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[2]=0x56e258u;abi.gp[4]=0x6000f000u;abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(owner.memory(),abi.gp[4],0x63000000u);
        captured.reset();owner.run(0x165700u,abi);check(captured&&captured_owner,"Actual compiled producer was not observed");
        const bool expected=mode==0||mode==1||mode==5||mode==8;
        check(native_required_owner_policy_detail::preserves_instruction_policy(*captured)==expected,"Actual Arena producer policy domain differs");++checks;if(!expected)++refusals;
        check(!native_arena_publication_detail::Borrow::qualified(owner.memory(),*captured),"Retired Arena publication remained eligible");++checks;
        auto copied=*captured;check(native_required_owner_policy_detail::preserves_instruction_policy(copied)==expected,"Copied immutable dispatcher changed policy");++checks;
#if defined(HALO2_ARENA_SOURCE_ROUTES)
        if(mode==1){for(unsigned policy=0;policy<14;++policy)source_route(owner,bank,expected,policy);if(std::getenv("HALO2_ARENA_MEMORY_POLICY_REVOCATION")){source_route(owner,bank,expected,14);source_route(owner,bank,expected,15);}}
        else source_route(owner,bank,expected,0);
#endif
        if(expected){
            bool foreign_accepted=true;std::jthread foreign([&]{foreign_accepted=native_required_owner_policy_detail::preserves_instruction_policy(copied);});foreign.join();check(!foreign_accepted,"Foreign Arena parent executor accepted");++checks;
            auto wrapped=copied;wrapped.required_owner=[actual=copied.required_owner](auto&m,const auto&c,auto&a,const auto&o){actual(m,c,a,o);};check(!native_required_owner_policy_detail::preserves_instruction_policy(wrapped),"Wrapped Arena dispatcher accepted");++checks;
            wrapped=copied;wrapped.required_owner_policy.owner=&foreign_identity;check(!native_required_owner_policy_detail::preserves_instruction_policy(wrapped),"Foreign Arena capture accepted");++checks;
            wrapped=copied;wrapped.required_owner=LowerOwner{&lower_identity};check(!native_required_owner_policy_detail::preserves_instruction_policy(wrapped),"Rebound Arena callback retained stale policy");++checks;
            SourceRuntimeMemory other_memory;check(!native_arena_publication_detail::Borrow::qualified(other_memory,copied),"Foreign Arena memory accepted");++checks;
            SourceRuntimeMemory foreign_bank;bool actual_bank_refusal{};try{copied.required_owner(foreign_bank,{0,0,false},abi,{});}catch(const std::invalid_argument&e){actual_bank_refusal=std::string_view(e.what())=="Native arena startup shared bank owner differs";}check(actual_bank_refusal,"Named dispatcher bypassed the actual source bank guard");++checks;
            bool actual_frame_refusal{};try{copied.required_owner(owner.memory(),{0,0,false},abi,{});}catch(const std::invalid_argument&e){actual_frame_refusal=std::string_view(e.what())=="Original TU1.5 arena resource call frame differs";}check(actual_frame_refusal,"Named dispatcher bypassed the actual source frame guard");++checks;
        }
        owner.stop();
    }
#if defined(HALO2_ARENA_SOURCE_ROUTES)
    const auto cold=actual_cold_source();
#endif
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"refused_constructor_domains\":"<<refusals<<",\"actual_table_initializations\":10,\"production_policy_issued\":false";
#if defined(HALO2_ARENA_SOURCE_ROUTES)
    std::cout<<",\"source_route_checks\":"<<source_checks<<",\"untouched_fallbacks\":"<<fallbacks<<",\"actual_native_warm_returns\":"<<native_warm<<",\"actual_reference_warm_returns\":"<<reference_warm<<",\"exact_live_publications\":"<<live_publications<<",\"nested_publications\":"<<nested_publications<<",\"actual_cold_source\":"<<cold.dump();
#endif
    std::cout<<"}\n";
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
