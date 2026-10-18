#ifdef HALO2_REPLAY_AI_ACTOR_TU15
#include "halo2/source_ai_actor_tu15.hpp"
#elif defined(HALO2_REPLAY_OBJECT_MATERIAL_TU15)
#include "halo2/source_object_material_tu15.hpp"
#elif defined(HALO2_REPLAY_DAMAGE_FEEDBACK_TU15)
#include "halo2/source_damage_feedback_tu15.hpp"
#elif defined(HALO2_REPLAY_EFFECT_PARTICLE_FRAME_TU15)
#include "halo2/source_effect_particle_frame_tu15.hpp"
#elif defined(HALO2_REPLAY_EFFECT_PARTICLE_DRAW_TU15)
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/source_vehicle_director_tu15.hpp"
#elif defined(HALO2_REPLAY_VEHICLE_DIRECTOR_TU15)
#include "halo2/source_vehicle_director_tu15.hpp"
#elif defined(HALO2_REPLAY_RECORDED_ANIMATION_TU15)
#include "halo2/source_recorded_animation_tu15.hpp"
#elif defined(HALO2_REPLAY_LEVEL_HANDLES_NEW_MAP_TU15)
#include "halo2/source_level_handles_new_map_tu15.hpp"
#elif defined(HALO2_REPLAY_RESOURCE_CACHE_NEW_MAP_TU15)
#include "halo2/source_campaign_activation_tu15.hpp"
#include <sys/mman.h>
#elif defined(HALO2_REPLAY_CAMPAIGN_ACTIVATION_TU15)
#include "halo2/source_campaign_activation_tu15.hpp"
#ifdef HALO2_REPLAY_SUPPORT_RENDERER_TAIL_TU15
#include "halo2/source_renderer_tail_storage_tu15.hpp"
#endif
#elif defined(HALO2_REPLAY_OBJECT_FUNCTIONS_TU15)
#include "halo2/source_object_functions_tu15.hpp"
#else
#include "halo2/source_ai_frame_tu15.hpp"
#endif
#include "halo2/source_runtime_memory.hpp"
#ifdef HALO2_REPLAY_EFFECT_PARTICLE_SUPPORT_TU15
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#endif
#ifdef HALO2_REPLAY_DAMAGE_FEEDBACK_SUPPORT_TU15
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/physics_phantom_world_tu15.hpp"
#include "halo2/death_cycle_runtime_tu15.hpp"
#include "physics_live_entries_tu15.hpp"
#endif
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <xmmintrin.h>
using namespace halo2;
#ifdef HALO2_REPLAY_RESOURCE_CACHE_NEW_MAP_TU15
namespace halo2 {
void source_run_resource_cache_replay_support_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver&);
}
#endif
namespace {
#ifdef HALO2_REPLAY_AI_ACTOR_TU15
constexpr auto run_ai_consumer=source_run_ai_actor_tu15;
#elif defined(HALO2_REPLAY_OBJECT_MATERIAL_TU15)
constexpr auto run_ai_consumer=source_run_object_material_tu15;
#elif defined(HALO2_REPLAY_DAMAGE_FEEDBACK_TU15)
constexpr auto run_ai_consumer=source_run_damage_feedback_tu15;
#elif defined(HALO2_REPLAY_EFFECT_PARTICLE_FRAME_TU15)
constexpr auto run_ai_consumer=source_run_effect_particle_frame_tu15;
#elif defined(HALO2_REPLAY_EFFECT_PARTICLE_DRAW_TU15)
constexpr auto run_ai_consumer=source_run_effect_particle_draw_tu15;
#elif defined(HALO2_REPLAY_VEHICLE_DIRECTOR_TU15)
constexpr auto run_ai_consumer=source_run_vehicle_director_tu15;
#elif defined(HALO2_REPLAY_RECORDED_ANIMATION_TU15)
constexpr auto run_ai_consumer=source_run_recorded_animation_tu15;
#elif defined(HALO2_REPLAY_LEVEL_HANDLES_NEW_MAP_TU15)
constexpr auto run_ai_consumer=source_run_level_handles_new_map_tu15;
#elif defined(HALO2_REPLAY_RESOURCE_CACHE_NEW_MAP_TU15)
constexpr auto run_ai_consumer=source_run_campaign_activation_tu15;
#elif defined(HALO2_REPLAY_CAMPAIGN_ACTIVATION_TU15)
constexpr auto run_ai_consumer=source_run_campaign_activation_tu15;
#elif defined(HALO2_REPLAY_OBJECT_FUNCTIONS_TU15)
constexpr auto run_ai_consumer=source_run_object_functions_tu15;
#else
constexpr auto run_ai_consumer=source_run_ai_frame_tu15;
#endif
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class T>void write(std::ostream &out,const T &value){out.write(reinterpret_cast<const char*>(&value),sizeof(value));check(bool(out),"Cannot retain native AI frame replay");}
std::vector<std::byte> read_file(const std::filesystem::path &path){std::ifstream file(path,std::ios::binary|std::ios::ate);check(bool(file),"AI frame comparison file absent");const auto size=file.tellg();check(size>=0,"AI frame comparison size absent");std::vector<std::byte> bytes(static_cast<std::size_t>(size));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(file),"Cannot read AI frame comparison");return bytes;}
}
int main(int argc,char **argv)try {
    check(argc==4 || argc==5,"Supply ORIGINAL_CASE ENTRY OUTPUT [OBSERVABLE_WORD_ADDRESS]");const std::filesystem::path input=argv[1],out=argv[3];
    if(!std::filesystem::exists(input/"before.bin") || !std::filesystem::exists(input/"hardware/hardware-after.bin") || !std::filesystem::exists(input/"original-trace.bin"))return 77;
    std::filesystem::create_directories(out);
    SourceRuntimeMemory memory;std::ifstream bank(input/"before.bin",std::ios::binary);check(bool(bank),"Actual original AI frame input bank absent");unsigned regions{};
#ifdef HALO2_REPLAY_RESOURCE_CACHE_NEW_MAP_TU15
    std::map<std::uint32_t,std::shared_ptr<SourceRuntimeMemoryBacking>> physical_backings;
    unsigned protection_count=0;
    std::ofstream protections(out/"host-physical-protections.bin",std::ios::binary);
#endif
    for(std::uint32_t address,size;bank.read(reinterpret_cast<char*>(&address),4);){
        bank.read(reinterpret_cast<char*>(&size),4);check(bool(bank) && size && size<0x40000000u && ++regions<4096,"Invalid captured source range");
        std::vector<std::byte> bytes(size);bank.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(bank),"Truncated captured source range");
#ifdef HALO2_REPLAY_RESOURCE_CACHE_NEW_MAP_TU15
        if(address>=0x80000000u) {
            check(address==0x80061000u && size==0x30e4000u,"Captured physical allocation differs");
            auto *host=::mmap(nullptr,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
            check(host!=MAP_FAILED,"Actual native replay physical mmap failed");
            auto lifetime=std::shared_ptr<void>(host,[size](void *p){::munmap(p,size);});
            auto backing=std::make_shared<SourceRuntimeMemoryBacking>(std::move(lifetime),std::span(static_cast<std::byte*>(host),size));
            std::ranges::copy(bytes,backing->bytes().begin());physical_backings.emplace(address,backing);
            memory.mount(address,std::move(backing),SourceRuntimeMemory::Access::ReadWrite,"Actual native replay physical mmap");
        } else
#endif
        memory.mount(address,std::move(bytes),SourceRuntimeMemory::Access::ReadWrite,"Original source AI frame replay bank");
    }
    SourcePhysicsWorldRuntimeAbi abi{};std::ifstream caller(input/"native-input.bin",std::ios::binary);check(bool(caller),"Original AI frame caller ABI absent");
    caller.read(reinterpret_cast<char*>(abi.gp.data()),sizeof(abi.gp));caller.read(reinterpret_cast<char*>(abi.xmm.data()),sizeof(abi.xmm));caller.read(reinterpret_cast<char*>(&abi.eflags),4);caller.read(reinterpret_cast<char*>(&abi.fs_base),4);check(bool(caller),"Truncated original AI frame caller");
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};const auto entry=std::stoul(argv[2],nullptr,0);
    if(std::filesystem::exists(input/"native-fp-input.bin")) {
        std::ifstream fp_input(input/"native-fp-input.bin",std::ios::binary);
        SourcePhysicsFactoryFloatingPoint captured{};
        fp_input.read(reinterpret_cast<char*>(&abi.x87_depth),4);
        fp_input.read(reinterpret_cast<char*>(&abi.x87_status),2);
        fp_input.read(reinterpret_cast<char*>(&captured.x87_control_word),2);
        fp_input.read(reinterpret_cast<char*>(&captured.mxcsr),4);
        check(bool(fp_input) && abi.x87_depth==0,"Captured source caller requires an empty x87 stack");
        abi.floating_point=captured;
    }
    std::ofstream trace(out/"native-trace.bin",std::ios::binary),stores(out/"native-stores.bin",std::ios::binary);SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto p){write(trace,p);};
    // Same explicit CPU primitive policy as the real source world. This is
    // an actual SSE operation over the current operand, never a recorded
    // answer or a game-function return. Xbox lookup bits remain unverified.
    services.reciprocal_sqrt=[](std::uint32_t bits){return std::bit_cast<std::uint32_t>(
        _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));};
    services.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observe){
#ifdef HALO2_REPLAY_EFFECT_PARTICLE_DRAW_TU15
        if(source_effect_particle_frame_tu15_owns_entry(call.target)) {
            source_run_effect_particle_frame_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
        if(source_vehicle_director_tu15_owns_entry(call.target)) {
            source_run_vehicle_director_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
#endif
#ifdef HALO2_REPLAY_EFFECT_PARTICLE_SUPPORT_TU15
        if(source_combat_action_tu15_owns_entry(call.target)) {
            source_run_combat_action_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
        if(source_physics_live_support_tu15_owns_entry(call.target)) {
            source_run_physics_live_support_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
#endif
#ifdef HALO2_REPLAY_DAMAGE_FEEDBACK_SUPPORT_TU15
        if(source_damage_feedback_tu15_owns_entry(call.target)) {
            source_run_damage_feedback_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
        if(source_physics_live_support_tu15_owns_entry(call.target)) {
            source_run_physics_live_support_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
        if(source_death_cycle_runtime_tu15_owns_entry(call.target)) {
            source_run_death_cycle_runtime_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
        for(const auto &entry:physics_live_entries_tu15::entries)if(entry.actual==call.target) {
            if(entry.module==physics_live_entries_tu15::Module::World) {
                source_run_physics_world_runtime_selected_observations(bank,entry.canonical,state,
                    SourcePhysicsContactImage::TitleUpdate15,*state.floating_point,services,observe);return;
            }
            if(entry.module==physics_live_entries_tu15::Module::Phantom) {
                auto control=*state.floating_point;
                source_run_physics_phantom_world_tu15_runtime(bank,entry.actual,state,control,services,observe);
                state.floating_point=control;return;
            }
        }
#endif
#ifdef HALO2_REPLAY_SUPPORT_RENDERER_TAIL_TU15
        if(source_renderer_tail_storage_tu15_owns_entry(call.target)) {
            source_run_renderer_tail_storage_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
#endif
#ifdef HALO2_REPLAY_RESOURCE_CACHE_NEW_MAP_TU15
        if(call.target==0x2db4cau || call.target==0x32a4e0u) {
            source_run_resource_cache_replay_support_tu15(bank,call.target,state,*state.floating_point,services,observe);return;
        }
        if(call.instruction==0x2db4d1u && call.target==0x800000b6u && call.tail) {
            const auto word=[&](std::uint32_t p){std::uint32_t value=0;auto b=memory.view(p,4);for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return value;};
            const auto sp=state.gp[4],returned=word(sp),address=word(sp+4),size=word(sp+8),protection=word(sp+12);
            const auto base=word(0x4f2950u),index=protection_count;
            check(index<2 && returned==(index?0x12675du:0x126747u) && word(0x41b648u)==call.target &&
                address==base+(index?0x3be000u:0) && size==(index?0x40000u:0x3be000u) && protection==(index?0x404u:4u),"Actual physical-protection input differs");
            const auto first=address&~4095u,end=(address+size+4095u)&~4095u;
            auto found=physical_backings.upper_bound(first);check(found!=physical_backings.begin(),"Unowned physical request");--found;
            check(std::uint64_t(end)<=std::uint64_t(found->first)+found->second->bytes().size(),"Physical request exceeds its actual mmap");
            check(!::mprotect(found->second->bytes().data()+first-found->first,end-first,PROT_READ|PROT_WRITE),"Actual native replay mprotect failed");
            for(auto value:{returned,address,size,protection,first,end-first})write(protections,value);
            ++protection_count;state.gp[4]+=16;return;
        }
#endif
        std::ostringstream error;error<<"Unbound original AI frame owner "<<std::hex<<call.instruction<<" -> "<<call.target;throw std::runtime_error(error.str());};
    const auto dump=[&]{std::ofstream file(out/"native-after.bin",std::ios::binary);memory.visit_committed([&](auto p,auto bytes){write(file,p);write(file,std::uint32_t(bytes.size()));file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));});check(bool(file),"Cannot retain AI frame replay bank");};
    try {run_ai_consumer(memory,entry,abi,*abi.floating_point,services,[&](const auto &s){if(s.address>=0x60000000u && s.address<0x60200000u)return;write(stores,s.instruction);write(stores,s.address);write(stores,std::uint32_t(s.written.size()));stores.write(reinterpret_cast<const char*>(s.before.data()),std::streamsize(s.before.size()));stores.write(reinterpret_cast<const char*>(s.written.data()),std::streamsize(s.written.size()));});}
    catch(const std::exception &e){dump();std::ofstream(out/"failure.txt")<<e.what()<<'\n';throw;}
#ifdef HALO2_REPLAY_RESOURCE_CACHE_NEW_MAP_TU15
    check(protection_count==2,"Original resource-cache protections incomplete");protections.close();
    check(read_file(out/"host-physical-protections.bin")==read_file(input/"hardware/host-physical-protections.bin"),"Native real protection requests differ from independent hardware");
#endif
    trace.close();stores.close();dump();std::ofstream exit(out/"native-exit.bin",std::ios::binary);write(exit,abi.gp);write(exit,abi.xmm);write(exit,abi.eflags);write(exit,abi.fs_base);write(exit,abi.x87_depth);write(exit,abi.x87_status);write(exit,abi.floating_point->x87_control_word);write(exit,abi.floating_point->mxcsr);exit.close();
    const bool stepped=std::filesystem::exists(input/"hardware-step-reference.json");
    const auto trace_reference=stepped?input/"hardware/hardware-trace.bin":input/"original-trace.bin";
    const auto stores_reference=stepped?input/"hardware/hardware-stores.bin":input/"original-stores.bin";
    check(read_file(out/"native-trace.bin")==read_file(trace_reference),"Native AI frame instruction path differs");
    check(read_file(out/"native-stores.bin")==read_file(stores_reference),"Native AI frame ordered writes differ");
    auto expected=read_file(input/"hardware/hardware-exit.bin");check(expected.size()==180,"Hardware AI frame exit ABI size differs");
    expected[161]&=std::byte{0xfd};check(read_file(out/"native-exit.bin")==expected,"Native AI frame GP/XMM/FP exit differs from hardware");
    std::ifstream reference(input/"hardware/hardware-after.bin",std::ios::binary);std::size_t compared{};
    for(std::uint32_t address,size;reference.read(reinterpret_cast<char*>(&address),4);){reference.read(reinterpret_cast<char*>(&size),4);check(bool(reference),"Truncated hardware bank");std::vector<std::byte> bytes(size);reference.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(reference),"Truncated hardware source range");if(address>=0x60000000u && address<0x60200000u)continue;check(std::ranges::equal(memory.view(address,size),bytes),"Native AI frame retained bank differs from hardware");compared+=size;}
    if(argc==5) {
        const auto address=std::uint32_t(std::stoul(argv[4],nullptr,0));
        std::ifstream word_reference(input/"hardware/hardware-after.bin",std::ios::binary);bool found=false;
        for(std::uint32_t base,size;word_reference.read(reinterpret_cast<char*>(&base),4);) {
            word_reference.read(reinterpret_cast<char*>(&size),4);check(bool(word_reference),"Truncated observable-word region");
            std::vector<std::byte> bytes(size);word_reference.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(word_reference),"Truncated observable-word data");
            if(base<=address && std::uint64_t(address)+2<=std::uint64_t(base)+size) {
                check(std::ranges::equal(memory.view(address,2),std::span(bytes).subspan(address-base,2)),"Native observable caller WORD differs from hardware");
                std::ofstream word(out/"observable-word.bin",std::ios::binary);word.write(reinterpret_cast<const char*>(bytes.data()+address-base),2);check(bool(word),"Cannot retain observable caller WORD");found=true;break;
            }
        }
        check(found,"Observable caller WORD absent from hardware bank");
    }
    std::cout<<"PASS native complete source replay entry="<<std::hex<<entry<<std::dec<<"; exact trace, ordered writes, full hardware exit except IF and "<<compared<<" retained bytes; observable WORD checked="<<(argc==5)<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
