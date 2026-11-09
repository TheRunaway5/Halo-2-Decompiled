#include "platform/native_tu15_async_promote.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/native_synchronization.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using State=physics_world_runtime_detail::WorldState;
static void record_promote_child(std::uint32_t,State&);
#include "async_promote_reference.inc"
namespace {
constexpr std::array bases{0x4ec000u,0x61000000u,0x62000000u};
constexpr std::array sizes{20480u,16384u,8192u};
struct Access {std::uint32_t address,size,pc;bool write;};
struct Memory final:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,3> banks;
    mutable std::vector<Access> accesses;const std::uint32_t* cursor{};
    Memory(){for(unsigned i=0;i<3;++i)banks[i].resize(sizes[i]);}
    std::span<std::byte> view(std::uint32_t p,std::size_t n){
        for(unsigned i=0;i<3;++i)if(p>=bases[i]&&p-bases[i]<=sizes[i]&&n<=sizes[i]-(p-bases[i]))return std::span(banks[i]).subspan(p-bases[i],n);
        throw std::runtime_error("Async promotion escaped DATA banks");
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        accesses.push_back({p,std::uint32_t(out.size()),cursor?*cursor:0,false});std::ranges::copy(const_cast<Memory*>(this)->view(p,out.size()),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        accesses.push_back({p,std::uint32_t(in.size()),cursor?*cursor:0,true});std::ranges::copy(in,view(p,in.size()).begin());
    }
};
struct Store {std::uint32_t pc,address;std::vector<std::byte> before,after;};
struct Phase {std::uint32_t target;SourcePhysicsWorldRuntimeAbi before,after;std::array<std::vector<std::byte>,3> banks;std::vector<Store> stores;};
thread_local std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>> calls;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
}
static void record_promote_child(std::uint32_t target,State&s){calls.emplace_back(target,s.save());}
int main(int argc,char**argv)try {
    bool reference=false,accesses=false;
    for(int i=1;i<argc;++i){const std::string_view arg=argv[i];if(arg=="--reference")reference=true;
        else if(arg=="--accesses")accesses=true;else throw std::runtime_error("Unknown promotion mode");}
    const auto word=[](){std::uint32_t value{};std::cin.read(reinterpret_cast<char*>(&value),4);if(!std::cin)throw std::runtime_error("Truncated promotion input");return value;};
    const auto emit=[](std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4);};
    const auto abi=[&](const auto& value){for(auto lane:value.gp)emit(lane);emit(value.eflags);emit(value.floating_point->mxcsr);emit(value.x87_status);emit(value.floating_point->x87_control_word);
        for(auto&lane:value.xmm)for(auto value:lane)emit(value);};
    const auto banks=[&](const auto& value){for(auto&bank:value)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());};
    const auto stores=[&](const auto& value){emit(value.size());for(auto&e:value){emit(e.pc);emit(e.address);emit(e.after.size());
        for(unsigned i=0;i<4;++i)std::cout.put(i<e.before.size()?char(e.before[i]):'\0');for(unsigned i=0;i<4;++i)std::cout.put(i<e.after.size()?char(e.after[i]):'\0');}};
    while(std::cin.peek()!=std::char_traits<char>::eof()) {
        calls.clear();SourcePhysicsWorldRuntimeAbi input{};for(auto&v:input.gp)v=word();input.eflags=word();const auto mxcsr=word();
        input.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(word()),mxcsr};input.x87_status=std::uint16_t(word());
        for(auto&lane:input.xmm)for(auto&v:lane)v=word();
        Memory memory;for(auto&bank:memory.banks){std::cin.read(reinterpret_cast<char*>(bank.data()),bank.size());if(!std::cin)throw std::runtime_error("Truncated promotion bank");}
        NativeSynchronization synchronization({0xf3000001u,0xf3000100ull});const auto native=synchronization.services();
        for(unsigned n=0;n<2;++n){const auto created=native.create_mutex(false);if(created.error||created.handle!=0xf3000001u+n)throw std::runtime_error("Native promotion mutex creation failed");}
        const auto checkpoint=synchronization.checkpoint();
        std::vector<Store> mutations;std::vector<Phase> phases;
        SourcePhysicsContactRecordObserver observer=[&](const auto&e){mutations.push_back({e.instruction,e.address,e.before,e.written});};
        SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{},callbacks{};memory.cursor=&pc;
        services.instruction=[&](auto site){pc=site;++callbacks;};auto&policy=services.instruction_cursor;
        policy.position=&pc;policy.executor=std::this_thread::get_id();policy.callback_type=&services.instruction.target_type();
        policy.literal_service_sites=true;policy.observation_only_stores=true;policy.service_sites=service_sites;
        services.required_owner=[&](auto&m,const auto&call,auto&child,const auto&) {
            if(call.target!=0x2e0478u&&call.target!=0x2e0342u)throw std::runtime_error("Unexpected promotion platform owner");
            calls.emplace_back(call.target,child);Phase phase{call.target,child,{},memory.banks,mutations};
            const auto stack=child.gp[4],handle=physics_step_detail::r(m,stack+4u);
            if(call.target==0x2e0478u) {
                if(physics_step_detail::r(m,stack+8u)!=0xffffffffu)throw std::runtime_error("Promotion wait is not infinite");
                const auto result=native.wait(handle,0xffffffffu);
                if(result.error||result.state!=NativeSynchronizationWaitState::Acquired)throw std::runtime_error("Actual native promotion wait did not acquire");
                (void)physics_step_detail::r(m,stack);child.gp[0]=0;child.gp[4]=stack+12u;
            } else {
                if(const auto error=native.release_mutex(handle))throw std::system_error(error);
                (void)physics_step_detail::r(m,stack);child.gp[0]=1;child.gp[4]=stack+8u;
            }
            phase.after=child;phases.push_back(std::move(phase));
        };
        State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
        if(reference)promote_reference::run_1236a0(state);
        else if(!native_async_promote_detail::promote(state,[&](auto& child){
            record_promote_child(0x1232c0u,child);
            if(!native_async_order_detail::insert(child,0x1232c0u,[&](auto&compare){
                record_promote_child(0x1ab330u,compare);
                if(!native_async_order_detail::compare(compare,0x1ab330u))throw std::runtime_error("Promotion comparator declined");
            }))throw std::runtime_error("Promotion insertion declined");
        }))throw std::runtime_error("Native promotion declined");
        if((reference&&!callbacks)||(!reference&&callbacks))throw std::runtime_error("Promotion instruction callback routing differs");
        if(synchronization.checkpoint()!=checkpoint)throw std::runtime_error("Promotion retained native mutex ownership");
        if(accesses){emit(memory.accesses.size());for(auto&e:memory.accesses){emit(e.address);emit(e.size);emit(e.write);emit(e.pc);}continue;}
        abi(state.save());banks(memory.banks);stores(mutations);emit(calls.size());for(auto&[target,child]:calls){emit(target);abi(child);}
        emit(phases.size());for(auto&phase:phases){emit(phase.target);abi(phase.before);abi(phase.after);banks(phase.banks);stores(phase.stores);}
    }
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
