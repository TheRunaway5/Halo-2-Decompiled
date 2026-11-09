#include "platform/native_tu15_tick_conversion.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#ifdef HALO2_TICK_WITH_OWNER_DISPATCH
#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"
#endif
#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>
using namespace halo2;
using State=physics_world_runtime_detail::WorldState;
namespace {
struct Memory final:SourcePhysicsContactMemory {
    std::vector<std::byte> bank=std::vector<std::byte>(8192);
    struct Read {std::uint32_t pc,address,size;};
    mutable std::vector<Read> reads;
    const std::uint32_t *cursor{};
    std::span<std::byte> view(std::uint32_t p,std::size_t n){
        if(p<0x62000000u||p-0x62000000u>bank.size()||n>bank.size()-(p-0x62000000u))throw std::runtime_error("Tick arithmetic escaped stack bank");
        return std::span(bank).subspan(p-0x62000000u,n);
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {reads.push_back({cursor?*cursor:0,p,std::uint32_t(out.size())});std::ranges::copy(const_cast<Memory*>(this)->view(p,out.size()),out.begin());}
    void write(std::uint32_t p,std::span<const std::byte> in)override {std::ranges::copy(in,view(p,in.size()).begin());}
};
struct Store {std::uint32_t pc,address;std::vector<std::byte> before,after;};
struct Call {std::uint32_t entry;SourcePhysicsWorldRuntimeAbi abi;};
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
int guards(){
    unsigned refused{};
    for(unsigned kind=0;kind<12;++kind){
        Memory memory;std::uint32_t pc{};bool trace1=kind==4,trace2=kind==5;
        SourcePhysicsContactRecordObserver observer=[](const auto&){};SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto site){pc=site;};auto&p=services.instruction_cursor;
        p.position=&pc;p.executor=std::this_thread::get_id();p.callback_type=&services.instruction.target_type();
        p.literal_service_sites=true;p.observation_only_stores=true;p.service_sites=service_sites;p.first_trace=&trace1;p.second_trace=&trace2;
        if(kind==1)p.position=nullptr;if(kind==2)p.executor={};if(kind==3)p.callback_type=&typeid(int);
        if(kind==6)p.observation_only_stores=false;if(kind==7)p.service_sites={};
        State s{memory,observer,services,kind==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};input.gp[4]=0x62001000u;input.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        if(kind==8)input.x87_depth=7;if(kind==9)input.active_mmx=SourcePhysicsMmxAlias{};s.load(input);
        const auto before=memory.bank;std::uint16_t cw{};asm volatile("fnstcw %0":"=m"(cw));
        if(kind==10){const auto unmasked=std::uint16_t(cw&~1u);asm volatile("fldcw %0"::"m"(unmasked));}
        const bool accepted=native_tick_conversion_detail::run(s,kind==11?0x123u:0x22a5c0u);
        asm volatile("fldcw %0"::"m"(cw));
        if(accepted||memory.bank!=before||s.gp!=input.gp||!memory.reads.empty()||pc)throw std::runtime_error("Tick refusal modified input state");
        ++refused;
    }
    std::cout<<"{\"refusals\":"<<refused<<"}\n";return 0;
}
int reference_guard(){
    Memory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
    State s{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};
    input.gp[4]=0x62001000u;input.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};s.load(input);
    for(auto entry:{0x22a5c0u,0x32ae70u,0x32acd0u}){
        if(native_tick_conversion_detail::run(s,entry))throw std::runtime_error("Tick reference control admitted native route");
#ifdef HALO2_TICK_WITH_OWNER_DISPATCH
        if(native_allocator_owner_detail::dispatch(s,entry,[](auto&,auto,auto,auto){throw std::runtime_error("Reference guard called owner");}))throw std::runtime_error("Tick reference control admitted dispatcher");
#endif
    }
    if(!memory.reads.empty()||s.gp!=input.gp)throw std::runtime_error("Tick reference refusal changed state");
    std::cout<<"{\"refusals\":3}\n";return 0;
}
}
int main(int argc,char**argv)try{
    bool reads_only=false,owner_dispatch=false;
    for(int i=1;i<argc;++i){const std::string_view arg=argv[i];if(arg=="--reads")reads_only=true;
        else if(arg=="--owner-dispatch")owner_dispatch=true;else if(arg=="--guard-tests")return guards();
        else if(arg=="--reference-guard")return reference_guard();
        else throw std::runtime_error("Unknown tick probe mode");}
    const auto word=[](){std::uint32_t v{};std::cin.read(reinterpret_cast<char*>(&v),4);if(!std::cin)throw std::runtime_error("Truncated tick input");return v;};
    const auto emit=[](std::uint32_t v){std::cout.write(reinterpret_cast<const char*>(&v),4);};
    const auto abi=[&](const auto&a){for(auto v:a.gp)emit(v);emit(a.eflags);emit(a.floating_point->mxcsr);emit(a.x87_status);emit(a.floating_point->x87_control_word);
        for(auto&lane:a.xmm)for(auto v:lane)emit(v);};
    while(std::cin.peek()!=std::char_traits<char>::eof()){
        const auto entry=word();SourcePhysicsWorldRuntimeAbi input{};for(auto&v:input.gp)v=word();input.eflags=word();const auto mxcsr=word();
        input.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(word()),mxcsr};input.x87_status=std::uint16_t(word());
        for(auto&lane:input.xmm)for(auto&v:lane)v=word();input.x87_depth=word();
        for(unsigned i=0;i<input.x87_depth;++i){std::array<std::byte,10> raw{};std::cin.read(reinterpret_cast<char*>(raw.data()),10);std::memcpy(&input.x87[i],raw.data(),10);}
        Memory memory;std::cin.read(reinterpret_cast<char*>(memory.bank.data()),memory.bank.size());if(!std::cin)throw std::runtime_error("Truncated tick bank");
        std::vector<Store> stores;std::vector<Call> calls;
        SourcePhysicsContactRecordObserver observer=[&](const auto&e){stores.push_back({e.instruction,e.address,e.before,e.written});};
        SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{},callbacks{};memory.cursor=&pc;
        services.instruction=[&](auto site){pc=site;++callbacks;};auto&policy=services.instruction_cursor;
        policy.position=&pc;policy.executor=std::this_thread::get_id();policy.callback_type=&services.instruction.target_type();
        policy.literal_service_sites=true;policy.observation_only_stores=true;policy.service_sites=service_sites;
        State s{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};s.load(input);
        if(owner_dispatch){
#ifdef HALO2_TICK_WITH_OWNER_DISPATCH
            const auto before=native_allocator_owner_detail::activity().tick_entries[entry];
            if(!native_allocator_owner_detail::dispatch(s,entry,[](auto&,auto,auto,auto){throw std::runtime_error("Tick dispatcher requested an external outcome");}))throw std::runtime_error("Native tick owner dispatcher declined");
            if(native_allocator_owner_detail::activity().tick_entries[entry]!=before+1)throw std::runtime_error("Tick native owner activity differs");
#else
            throw std::runtime_error("Tick owner dispatcher not built");
#endif
        }else if(entry==0x22a5c0u){if(!native_tick_conversion_detail::convert(s,[&](auto&v,auto target){calls.push_back({target,v.save()});}))throw std::runtime_error("Tick conversion declined");}
        else if(entry==0x32ae70u)native_tick_conversion_detail::signed_divide(s);
        else if(entry==0x32acd0u)native_tick_conversion_detail::multiply(s);
        else throw std::runtime_error("Unknown tick arithmetic entry");
        if(callbacks)throw std::runtime_error("Native tick arithmetic emitted instruction callbacks");
        if(reads_only){emit(memory.reads.size());for(auto&read:memory.reads){emit(read.pc);emit(read.address);emit(read.size);}continue;}
        const auto result=s.save();emit(0);abi(result);emit(result.x87_depth);
        for(unsigned i=0;i<result.x87_depth;++i)std::cout.write(reinterpret_cast<const char*>(&result.x87[i]),10);
        std::cout.write(reinterpret_cast<const char*>(memory.bank.data()),memory.bank.size());
        emit(stores.size());for(auto&e:stores){emit(e.pc);emit(e.address);emit(e.after.size());
            for(unsigned i=0;i<4;++i)std::cout.put(i<e.before.size()?char(e.before[i]):'\0');
            for(unsigned i=0;i<4;++i)std::cout.put(i<e.after.size()?char(e.after[i]):'\0');}
        emit(calls.size());for(auto&call:calls){emit(call.entry);abi(call.abi);emit(call.abi.x87_depth);
            for(unsigned i=0;i<call.abi.x87_depth;++i)std::cout.write(reinterpret_cast<const char*>(&call.abi.x87[i]),10);}
    }
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
