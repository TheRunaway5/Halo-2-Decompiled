#include "platform/native_tu15_async_order.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using State=physics_world_runtime_detail::WorldState;
#include "async_order_reference.inc"
namespace {
constexpr std::array bases{0x61000000u,0x62000000u};
constexpr std::array sizes{16384u,8192u};
struct Access {std::uint32_t address,size,pc;bool write;};
struct Memory final:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,2> banks;
    mutable std::vector<Access> accesses;
    const std::uint32_t *cursor{};
    Memory(){for(unsigned i=0;i<2;++i)banks[i].resize(sizes[i]);}
    std::span<std::byte> view(std::uint32_t p,std::size_t n){
        for(unsigned i=0;i<2;++i)if(p>=bases[i]&&p-bases[i]<=sizes[i]&&n<=sizes[i]-(p-bases[i]))
            return std::span(banks[i]).subspan(p-bases[i],n);
        throw std::runtime_error("Async ordering escaped DATA banks");
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        accesses.push_back({p,std::uint32_t(out.size()),cursor?*cursor:0,false});
        std::ranges::copy(const_cast<Memory*>(this)->view(p,out.size()),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        accesses.push_back({p,std::uint32_t(in.size()),cursor?*cursor:0,true});std::ranges::copy(in,view(p,in.size()).begin());
    }
};
struct Store {std::uint32_t pc,address;std::vector<std::byte> before,after;};
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
int guard_tests(){
    unsigned refused{};
    for(unsigned kind=0;kind<8;++kind){
        Memory memory;SourcePhysicsContactRecordObserver observer=[](const auto&){};
        const auto before=memory.banks;SourcePhysicsWorldRuntimeServices services;
        std::uint32_t pc{};bool trace1=kind==4,trace2=kind==5;
        services.instruction=[&](auto site){pc=site;};
        auto&policy=services.instruction_cursor;policy.position=&pc;policy.first_trace=&trace1;policy.second_trace=&trace2;
        policy.executor=std::this_thread::get_id();policy.callback_type=&services.instruction.target_type();
        policy.literal_service_sites=true;policy.observation_only_stores=true;policy.service_sites=service_sites;
        if(kind==1)policy.position=nullptr;if(kind==2)policy.executor={};if(kind==3)policy.callback_type=&typeid(int);
        if(kind==6)policy.observation_only_stores=false;if(kind==7)policy.service_sites={};
        State s{memory,observer,services,kind==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};input.gp[4]=0x62001000;s.load(input);
        if(native_async_order_detail::compare(s,0x1a5080u)||memory.banks!=before||s.gp!=input.gp||pc)
            throw std::runtime_error("Async diagnostic refusal modified source state");
        ++refused;
    }
    std::cout<<"{\"refusals\":"<<refused<<"}\n";return 0;
}
}
int main(int argc,char**argv)try{
    bool reference=false,accesses=false,canonical_sites=false,cursor_mode=false;
    for(int i=1;i<argc;++i){const std::string_view arg=argv[i];if(arg=="--reference")reference=true;
        else if(arg=="--accesses")accesses=true;else if(arg=="--canonical-sites")canonical_sites=true;
        else if(arg=="--cursor")cursor_mode=true;else if(arg=="--guard-tests")return guard_tests();
        else throw std::runtime_error("Unknown async probe mode");}
    const auto word=[](){std::uint32_t v{};std::cin.read(reinterpret_cast<char*>(&v),4);if(!std::cin)throw std::runtime_error("Truncated async input");return v;};
    const auto emit=[](std::uint32_t v){std::cout.write(reinterpret_cast<const char*>(&v),4);};
    while(std::cin.peek()!=std::char_traits<char>::eof()){
        SourcePhysicsWorldRuntimeAbi input{};for(auto&v:input.gp)v=word();input.eflags=word();const auto mxcsr=word();
        input.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(word()),mxcsr};input.x87_status=std::uint16_t(word());
        for(auto&lane:input.xmm)for(auto&v:lane)v=word();
        Memory memory;for(auto&bank:memory.banks){std::cin.read(reinterpret_cast<char*>(bank.data()),bank.size());if(!std::cin)throw std::runtime_error("Truncated async bank");}
        std::vector<Store> stores;SourcePhysicsContactRecordObserver observer=[&](const auto&e){stores.push_back({e.instruction,e.address,e.before,e.written});};
        SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{},callbacks{};
        // A store observer needs a live literal-observation witness even when
        // this run only compares values and does not emit its cursor stream.
        if(cursor_mode||!reference){
            if(cursor_mode)memory.cursor=&pc;services.instruction=[&](auto site){pc=site;++callbacks;};
            auto&policy=services.instruction_cursor;policy.position=&pc;policy.executor=std::this_thread::get_id();
            policy.callback_type=&services.instruction.target_type();policy.literal_service_sites=true;
            policy.observation_only_stores=true;policy.service_sites=service_sites;
        }
        State s{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};s.load(input);
        if(reference){if(canonical_sites)async_canonical::run_1a5080(s);else async_reference::run_1a5080(s);}
        else if(!native_async_order_detail::compare(s,canonical_sites?0x1a5080u:0x1ab330u))throw std::runtime_error("Native async ordering declined");
        if(cursor_mode&&((reference&&!callbacks)||(!reference&&callbacks)||pc!=(canonical_sites?0x1a5109u:0x1ab3b9u)))
            throw std::runtime_error("Async cursor or callback routing differs");
        if(accesses){emit(memory.accesses.size());for(auto&e:memory.accesses){emit(e.address);emit(e.size);emit(e.write);emit(e.pc);}continue;}
        const auto output=s.save();emit(0);for(auto v:output.gp)emit(v);emit(output.eflags);emit(output.floating_point->mxcsr);emit(output.x87_status);emit(output.floating_point->x87_control_word);
        for(auto&lane:output.xmm)for(auto v:lane)emit(v);
        for(auto&bank:memory.banks)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());
        emit(stores.size());for(auto&e:stores){emit(e.pc);emit(e.address);emit(e.after.size());
            for(unsigned i=0;i<4;++i)std::cout.put(i<e.before.size()?char(e.before[i]):'\0');
            for(unsigned i=0;i<4;++i)std::cout.put(i<e.after.size()?char(e.after[i]):'\0');}
    }
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
