#include "platform/native_tu15_async_order.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using State=physics_world_runtime_detail::WorldState;
static void record_async_child(State&);
#include "async_insert_reference.inc"
namespace {
constexpr std::array bases{0x4ef000u,0x61000000u,0x62000000u};
constexpr std::array sizes{4096u,16384u,8192u};
struct Access {std::uint32_t address,size,pc;bool write;};
struct Memory final:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,3> banks;
    mutable std::vector<Access> accesses;
    const std::uint32_t *cursor{};
    Memory(){for(unsigned i=0;i<3;++i)banks[i].resize(sizes[i]);}
    std::span<std::byte> view(std::uint32_t p,std::size_t n){
        for(unsigned i=0;i<3;++i)if(p>=bases[i]&&p-bases[i]<=sizes[i]&&n<=sizes[i]-(p-bases[i]))return std::span(banks[i]).subspan(p-bases[i],n);
        throw std::runtime_error("Async insertion escaped DATA banks");
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        accesses.push_back({p,std::uint32_t(out.size()),cursor?*cursor:0,false});std::ranges::copy(const_cast<Memory*>(this)->view(p,out.size()),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        accesses.push_back({p,std::uint32_t(in.size()),cursor?*cursor:0,true});std::ranges::copy(in,view(p,in.size()).begin());
    }
};
struct Store {std::uint32_t pc,address;std::vector<std::byte> before,after;};
thread_local std::vector<SourcePhysicsWorldRuntimeAbi> calls;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
}
static void record_async_child(State&s){calls.push_back(s.save());}
int main(int argc,char**argv)try{
    bool reference=false,accesses=false,canonical=false,shadow=false;
    for(int i=1;i<argc;++i){const std::string_view arg=argv[i];if(arg=="--reference")reference=true;
        else if(arg=="--accesses")accesses=true;else if(arg=="--canonical-sites")canonical=true;
        else if(arg=="--shadow-reference"){reference=true;shadow=true;}
        else throw std::runtime_error("Unknown insertion mode");}
    const auto word=[](){std::uint32_t v{};std::cin.read(reinterpret_cast<char*>(&v),4);if(!std::cin)throw std::runtime_error("Truncated insertion input");return v;};
    const auto emit=[](std::uint32_t v){std::cout.write(reinterpret_cast<const char*>(&v),4);};
    const auto abi=[&](const auto&output){for(auto v:output.gp)emit(v);emit(output.eflags);emit(output.floating_point->mxcsr);emit(output.x87_status);emit(output.floating_point->x87_control_word);
        for(auto&lane:output.xmm)for(auto v:lane)emit(v);};
    while(std::cin.peek()!=std::char_traits<char>::eof()){
        calls.clear();SourcePhysicsWorldRuntimeAbi input{};for(auto&v:input.gp)v=word();input.eflags=word();const auto mxcsr=word();
        input.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(word()),mxcsr};input.x87_status=std::uint16_t(word());
        for(auto&lane:input.xmm)for(auto&v:lane)v=word();
        Memory memory;for(auto&bank:memory.banks){std::cin.read(reinterpret_cast<char*>(bank.data()),bank.size());if(!std::cin)throw std::runtime_error("Truncated insertion bank");}
        std::vector<Store> stores;SourcePhysicsContactRecordObserver observer=[&](const auto&e){stores.push_back({e.instruction,e.address,e.before,e.written});};
        SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{},callbacks{};memory.cursor=&pc;
        services.instruction=[&](auto site){pc=site;++callbacks;};auto&policy=services.instruction_cursor;
        policy.position=&pc;policy.executor=std::this_thread::get_id();policy.callback_type=&services.instruction.target_type();
        policy.literal_service_sites=true;policy.observation_only_stores=true;policy.service_sites=service_sites;
        State s{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};s.load(input);
        if(reference){if(shadow)insert_shadow_reference::run_1232c0(s);else if(canonical)insert_canonical::run_122020(s);else insert_reference::run_122020(s);}
        else if(!native_async_order_detail::insert(s,canonical?0x122020u:0x1232c0u,[&](auto&child){
            record_async_child(child);if(!native_async_order_detail::compare(child,canonical?0x1a5080u:0x1ab330u))throw std::runtime_error("Native insertion comparator declined");
        }))throw std::runtime_error("Native insertion declined");
        if((reference&&!callbacks)||(!reference&&callbacks))throw std::runtime_error("Insertion callback routing differs");
        if(accesses){emit(memory.accesses.size());for(auto&e:memory.accesses){emit(e.address);emit(e.size);emit(e.write);emit(e.pc);}continue;}
        const auto output=s.save();emit(0);abi(output);
        for(auto&bank:memory.banks)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());
        emit(stores.size());for(auto&e:stores){emit(e.pc);emit(e.address);emit(e.after.size());
            for(unsigned i=0;i<4;++i)std::cout.put(i<e.before.size()?char(e.before[i]):'\0');
            for(unsigned i=0;i<4;++i)std::cout.put(i<e.after.size()?char(e.after[i]):'\0');}
        emit(calls.size());for(auto&call:calls)abi(call);
    }
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
