#include "halo2/native_tu15_arena_data.hpp"
#include "halo2/physics_game_arena_native.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "physics/runtime/native_tu15_arena_schedule_policy.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "arena_world_bridge.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <x86intrin.h>

using namespace halo2;using J=nlohmann::json;
namespace {
constexpr SourcePhysicsFactoryFloatingPoint fp{0x27f,0x1f80};
constexpr std::uint32_t stack=0x600f0000u,tls=0x60200000u,native_tick_count=0x60212000u;
void check(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
void put(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8u*n));m.write(p,b);}
std::string digest(std::span<const std::byte>bytes){std::array<unsigned char,EVP_MAX_MD_SIZE>hash{};unsigned size{};check(EVP_Digest(bytes.data(),bytes.size(),hash.data(),&size,EVP_sha256(),nullptr)==1,"World Arena hash failed");constexpr char hex[]="0123456789abcdef";std::string text;for(unsigned n=0;n<size;++n){text+=hex[hash[n]>>4];text+=hex[hash[n]&15];}return text;}
#include "arena_world_clock.inc"
struct WorldReceiver {
    using Impl=WorldReceiver;
    SourceRuntimeMemory&bank;
    const std::thread::id main_thread=std::this_thread::get_id();
    std::atomic<bool>publication_held{};bool staging{};
    std::optional<std::pair<std::uint32_t,std::uint32_t>>nested_caller_frame;
    std::uint32_t last_instruction{};bool first_person_phase_active{},weapon_effect_phase_active{},sound_timing_active{};
    std::optional<int>canonical_cache;std::unique_ptr<SourceExecutionInputs>execution_inputs;
    std::shared_ptr<PhysicsLiveClockSource>clocks=std::make_shared<NativeWorldClock>();
    std::uint64_t tick_adjustment{},cycle_adjustment{};
    std::vector<std::uint32_t>first_person_phase_trace,weapon_effect_phase_trace;
    std::vector<SourceFirstPersonCpuInputTu15>first_person_phase_cpu,weapon_effect_phase_cpu;
    unsigned wider_required_calls{};
    void require_executor()const{if(std::this_thread::get_id()!=main_thread||publication_held.load(std::memory_order_acquire)||staging)throw FormatError("TU1.5 world requires its available retained executor");}
    void required(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,const SourcePhysicsContactRecordObserver&){++wider_required_calls;throw std::runtime_error("Cold Arena unexpectedly entered wider World required owner");}
#include "arena_world_producer.inc"
};
thread_local WorldReceiver*active_world{};
thread_local physics_world_runtime_detail::WorldState*parent{};
thread_local unsigned statements{},calls{},resumes{};
thread_local unsigned return_policy{},returned_policy_changes{};
thread_local std::uint32_t handoff_pc{};
thread_local J children=J::array();
J abi_packet(const SourcePhysicsWorldRuntimeAbi&a){J lanes=J::array();for(unsigned n=0;n<a.x87_depth;++n)lanes.push_back(digest(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));return J{{"gp",a.gp},{"flags",a.eflags},{"fs",a.fs_base},{"xmm",a.xmm},{"x87_count",a.x87_depth},{"x87_status",a.x87_status},{"x87_payload_sha256",lanes},{"cw",a.floating_point?a.floating_point->x87_control_word:0},{"mxcsr",a.floating_point?a.floating_point->mxcsr:0},{"mmx",bool(a.active_mmx)}};}
}
namespace halo2 {
bool arena_world_reference(){static const bool value=std::getenv("HALO2_ARENA_CONTINUATION_REFERENCE");return value;}
bool arena_world_eligible(const physics_world_runtime_detail::WorldState&s){
    const auto&c=s.services.instruction_cursor;std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));
    return active_world&&std::this_thread::get_id()==active_world->main_thread&&s.image==SourcePhysicsContactImage::TitleUpdate15&&
        !s.active_mmx&&s.fp.count<=8&&s.fp.top<8&&(cw&0x3f)==0x3f&&(_mm_getcsr()&0x1f80u)==0x1f80u&&
        native_required_owner_policy_detail::preserves_instruction_policy(s.services)&&!s.observe&&
        s.services.instruction&&s.instruction_cursor==&c&&s.literal_service_sites&&c.position==&active_world->last_instruction&&c.callback_type&&s.services.instruction.target_type()==*c.callback_type&&
        c.executor==std::this_thread::get_id()&&c.literal_service_sites&&c.service_sites.size()==3&&
        c.service_sites[0]==0x38846eu&&c.service_sites[1]==0x21f590u&&c.service_sites[2]==0x33b595u&&
        !(c.first_trace&&*c.first_trace)&&!(c.second_trace&&*c.second_trace);
}
void arena_world_parent(bool begin,physics_world_runtime_detail::WorldState&s){parent=begin?&s:nullptr;}
void arena_world_statement(std::uint32_t){++statements;}
void arena_world_native(bool resumed){if(resumed)++resumes;else++calls;}
void arena_world_continuation(std::uint32_t next){handoff_pc=next;}
void arena_world_child(bool before,std::uint32_t target,std::uint32_t site,const SourcePhysicsWorldRuntimeAbi&abi){
    children.push_back(J{{"before",before},{"target",target},{"site",site},{"abi",abi_packet(abi)}});
    const auto expected=return_policy==7u?0x1252a4u:return_policy==8u?0x1252bau:return_policy==9u?0x1252edu:return_policy==10u?0x125321u:0u;
    if(!before&&expected&&site==expected&&!returned_policy_changes&&std::this_thread::get_id()==active_world->main_thread){
        ++returned_policy_changes;active_world->first_person_phase_active=true;
    }
}
}
int main()try{
    const auto*scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch&&*scratch,"Use disposable World Arena owner");
    std::string files=(std::filesystem::path(scratch)/"world-arena-XXXXXX").string();check(::mkdtemp(files.data()),"Private World Arena directory unavailable");
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{files};
    SourceRuntimeMemory bank;mount_native_tu15_arena_data(bank);
    bank.mount(0x60000000u,std::vector<std::byte>(0x100000),SourceRuntimeMemory::Access::ReadWrite,"actual World main caller stack");
    bank.mount(tls,std::vector<std::byte>(0x2000),SourceRuntimeMemory::Access::ReadWrite,"actual World caller exception chain");put(bank,tls,0xffffffffu);
    bank.mount(native_tick_count,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"actual native World clock import");
    WorldReceiver world{bank};active_world=&world;
    const bool feedback=std::getenv("HALO2_ARENA_DECLARED_FEEDBACK");
    const auto barrier=std::make_shared<native_arena_schedule_detail::WorkerJobBarrier>();
    auto schedule=feedback?native_arena_schedule_detail::feedback_schedule(barrier):SourcePhysicsGameArenaSchedule{};
    auto arena=std::make_unique<SourcePhysicsGameArenaNative>(bank,SourcePhysicsGameArenaConfiguration{files,0x60110000u,fp,schedule},world.build_services());
    for(auto entry:{0x12cb70u,0x1221b0u}){auto a=world.frame({});arena->run(entry,a);}
    const unsigned policy=std::getenv("HALO2_ARENA_WORLD_POLICY")?std::stoul(std::getenv("HALO2_ARENA_WORLD_POLICY")):0u;
    check(policy<=10u,"Unknown declared World Arena frontier policy");return_policy=policy;returned_policy_changes=0;handoff_pc=0;
    unsigned repeat{},guard_changes{};
    struct Guard{SourceRuntimeMemory&bank;~Guard(){source_runtime_unregister_write_guard(bank,this);}}guard{bank};
    if(policy&&policy<=6u)source_runtime_register_write_guard(bank,&guard,[&](auto,auto){
        if(std::this_thread::get_id()!=world.main_thread||guard_changes)return;
        const auto pc=world.last_instruction;
        const bool matches=policy==1?pc==0x125280u:policy==2?pc==0x1252b8u&&++repeat==1u:policy==3?pc==0x1252b8u&&++repeat==0xff800u:policy==4?pc==0x1252d2u:policy==6?pc==0x1252adu:pc==0x125328u;
        if(matches){++guard_changes;world.first_person_phase_active=true;}
    });
    auto a=world.frame({});statements=calls=resumes=0;children=J::array();arena->run(0x125280u,a);arena->stop();
    const auto ownership=arena->ownership_checkpoint();check(ownership.files.size()==2&&ownership.physical.allocations.size()==1&&ownership.synchronization.objects.size()==3,"World Arena genuine resource graph differs");
    check(!world.wider_required_calls,"Wider World required outcome was supplied");
    const auto changes=guard_changes+returned_policy_changes;
    if(policy)check(changes==1,"Actual World trace-revocation guard was not reached");
    constexpr std::array<std::uint32_t,11>frontiers{0,0x125281u,0x1252b8u,0x1252b8u,0x1252d7u,0x12532eu,0x1252b3u,0x1252a9u,0x1252bfu,0x1252f2u,0x125326u};
    if(!arena_world_reference())check(calls==1&&resumes==unsigned(policy!=0)&&handoff_pc==frontiers[policy],"Private World Arena source ownership/frontier differs");
    J banks=J::array();bank.visit_committed([&](auto address,auto bytes){if(address>=0x60000000u&&address<0x60200000u)return;banks.push_back(J{{"address",address},{"bytes",bytes.size()},{"sha256",digest(bytes)}});});
    J file_hashes=J::array();for(unsigned n=0;n<2;++n){std::ifstream in(std::filesystem::path(files)/("savegame"+std::to_string(n)+".bin"),std::ios::binary);std::vector<char>bytes{std::istreambuf_iterator<char>(in),{}};check(bytes.size()==4239360u,"Real World savegame extent differs");file_hashes.push_back(J{{"bytes",bytes.size()},{"sha256",digest(std::as_bytes(std::span(bytes)))}});}
    std::cout<<J{{"passed",true},{"private_unbound",true},{"declared_feedback_schedule",feedback},{"policy",policy},{"actual_guard_changes",changes},{"parent_native_calls",calls},{"parent_handoffs",resumes},{"handoff_pc",handoff_pc},{"parent_retained_statements",statements},{"escaped_abi",abi_packet(a)},{"actual_children",children},{"banks",banks},{"files",file_hashes},{"real_worker_retired",true},{"wider_required_calls",world.wider_required_calls},{"last_instruction",world.last_instruction},{"first_trace_count",world.first_person_phase_trace.size()},{"first_trace_sha256",digest(std::as_bytes(std::span(world.first_person_phase_trace)))}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
