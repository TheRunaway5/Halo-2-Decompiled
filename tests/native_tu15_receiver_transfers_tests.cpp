#include "rendering/native_tu15_view_reset.hpp"
#include "rendering/native_tu15_view_setup.hpp"
#include "halo2/source_execution_inputs.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace halo2;
using namespace native_receiver_transfers;
using State=physics_world_runtime_detail::WorldState;
constexpr std::uint32_t root=0x600ed000u,camera=root+0x1000u;
constexpr std::uint32_t view_bank=0x558000u,camera_buffer=0x55c000u;
constexpr std::uint32_t reset_entry=root-0x14u,setup_entry=root-0x38u;
constexpr std::uint32_t origin=root+0x800u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};

void need(bool condition,const char *message) {
    if(!condition)throw std::runtime_error(message);
}

void put(SourceRuntimeMemory &bank,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    bank.write(address,std::as_bytes(std::span(&value,1)).first(width));
}

enum class Policy { Ordinary,GuardFault,MemoryObserver,StoreObserver,Tape,WrongFrame,Overlap,Backward };

struct Fault {};
struct Outcome {
    SourcePhysicsWorldRuntimeAbi abi;
    SourceRuntimeMemory::Checkpoint bank;
    std::vector<std::array<std::uint32_t,3>> observed;
    std::uint64_t write_count;
    std::uint32_t pc;
    bool fault;
};

Outcome run(bool setup,unsigned count,Policy policy,bool grouped) {
    SourceRuntimeMemory bank;
    using Access=SourceRuntimeMemory::Access;
    bank.mount(view_bank,std::vector<std::byte>(0x6000,std::byte{0x5a}),Access::ReadWrite,"secondary view and queues");
    bank.mount(0x600ec000u,std::vector<std::byte>(0x4000,std::byte{0x76}),Access::ReadWrite,"original caller stack");
    bank.mount(0x46a0e4u,std::vector<std::byte>(4),Access::ReadWrite,"original float constant");
    bank.mount(0x474c24u,std::vector<std::byte>(2),Access::ReadWrite,"original view tag");
    put(bank,0x46a0e4u,0x3f800000u);
    put(bank,0x474c24u,0x2266u,2);
    for(unsigned n=0;n<6u*111u;++n)put(bank,camera+n*4u,0x3e000000u+n*3u);
    put(bank,origin+0,0x3f800000u);
    put(bank,origin+4,0xbf800000u);
    put(bank,origin+8,0x00000000u);
    for(unsigned n=0;n<4;++n) {
        const auto queue=0x55b000u+n*0x20u;
        put(bank,secondary_view+0xcu+n*4u,queue);
    }
    put(bank,secondary_view+0xa5cu,count,2);
    put(bank,secondary_view,policy==Policy::Overlap?camera:camera_buffer);

    SourcePhysicsContactRuntimeAbi abi{};
    abi.gp={0x44u,0x256u,secondary_view,count,setup?setup_entry:reset_entry,0x77u,0x88u,0x99u};
    abi.eflags=policy==Policy::Backward?0x602u:0x202u;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};
    abi.x87[0]=0.75L;
    abi.x87_depth=1;
    if(setup) {
        constexpr std::array args{0x134c3eu,secondary_view,1u,camera,0x33u,0u,origin,0x3f000000u,1u,0x3d000000u};
        for(unsigned i=0;i<args.size();++i)put(bank,setup_entry+4u*i,args[i]);
    } else put(bank,reset_entry,0x134c4du);

    std::uint32_t pc{};
    SourcePhysicsContactRuntimeServices services;
    services.instruction=[](auto){throw std::runtime_error("Unexpected literal service");};
    services.instruction_cursor={&pc,nullptr,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    std::vector<std::array<std::uint32_t,3>> observed;
    SourcePhysicsContactRecordObserver store_observer;
    if(policy==Policy::StoreObserver)store_observer=[&](const auto &store) {
        observed.push_back({store.instruction,store.address,std::uint32_t(store.written.size())});
    };

    const int owner{};
    const auto fault_address=setup?camera_buffer+12u:secondary_view+0x21cu+8u;
    if(policy==Policy::GuardFault)source_runtime_register_write_guard(bank,&owner,[&](auto at,auto bytes) {
        if(at<=fault_address && fault_address-at<bytes)throw Fault{};
    });
    if(policy==Policy::MemoryObserver)source_runtime_register_write_observer(bank,&owner,[&](auto at,auto bytes) {
        observed.push_back({pc,at,std::uint32_t(bytes)});
    });
    SourceRuntimeWriteSerial serial(bank,SourceRuntimeWriteSerial::Visibility::CallBoundary);
    SourceRuntimeWriteSerial continuation(bank,SourceRuntimeWriteSerial::Visibility::CallBoundary);
    SourceExecutionInputs tape;
    std::optional<SourceExecutionInputBinding> tape_binding;
    if(policy==Policy::Tape)tape_binding.emplace(bank,tape);

    State state{bank,store_observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(abi);
    bool fault=false;
    {
        std::optional<Scope> scope;
        if(grouped && !reference())scope.emplace(bank,serial,services,policy==Policy::WrongFrame?root-4u:root);
        try {
            const auto unexpected_child=[](auto&&...) {throw std::runtime_error("Unexpected view setup child");};
            need(setup?native_view_setup_detail::bridge(state,unexpected_child):native_view_reset_detail::bridge(state),
                 "Native view adapter refused normal test services");
        } catch(const Fault &) {fault=true;}
    }
    need(!Scope::current(bank,services),"Receiver scope survived the source call");
    need(serial.load()==continuation.load(),"Exact serial subscribers diverged");
    tape_binding.reset();
    tape.finish();
    if(policy==Policy::GuardFault)source_runtime_unregister_write_guard(bank,&owner);
    if(policy==Policy::MemoryObserver)source_runtime_unregister_write_observer(bank,&owner);
    return {state.save(),bank.checkpoint(),std::move(observed),serial.load(),pc,fault};
}

void compare(bool setup,unsigned count,Policy policy) {
    const auto expected=run(setup,count,policy,false);
    const auto actual=run(setup,count,policy,true);
    need(actual.fault==expected.fault && actual.fault==(policy==Policy::GuardFault),"Fault prefix differs");
    need(actual.bank==expected.bank,"Complete retained bytes differ");
    need(actual.write_count==expected.write_count && actual.write_count>0,"Logical write count differs");
    need(actual.pc==expected.pc && actual.observed==expected.observed,"Instruction or callback order differs");
    const auto &a=actual.abi,&b=expected.abi;
    // The REP state preserves all flags. AF remains undefined after specific
    // shift/logical instructions in the original setup and reset bodies.
    need(a.gp==b.gp && (a.eflags&~0x10u)==(b.eflags&~0x10u) && a.xmm==b.xmm &&
         a.floating_point->mxcsr==b.floating_point->mxcsr && a.x87_status==b.x87_status &&
         a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
         a.x87_depth==b.x87_depth && a.x87[0]==b.x87[0],"Final or fault ABI differs");
}

void compare_camera_clear(std::uint32_t site,unsigned row,Policy policy,bool eligible=true) {
    constexpr std::uint32_t bank_base=0x600ec000u;
    constexpr std::size_t bank_size=0x4000u;
    SourceRuntimeMemory bank;
    bank.mount(bank_base,std::vector<std::byte>(bank_size,std::byte{0x76}),
        SourceRuntimeMemory::Access::ReadWrite,"original camera caller stack");
    constexpr std::array args{0x60300000u,0x01001230u,root+0x2f00u,camera,root+0x2900u};
    for(unsigned n=0;n<args.size();++n)put(bank,root+n*4u,args[n]);
    need(Scope::owns_camera_root(bank,root),"Original camera caller frame refused");
    put(bank,root+12u,camera+4u);
    need(!Scope::owns_camera_root(bank,root),"Mismatched camera root output admitted");
    put(bank,root+12u,camera);

    auto raw=bank.view(bank_base,bank_size);
    std::array<std::byte,bank_size> expected;
    std::memcpy(expected.data(),raw.data(),raw.size());
    const auto address=camera+row*camera_bytes;
    std::uint32_t cursor=site;
    SourcePhysicsContactRuntimeServices services;
    services.instruction=[&](auto pc){cursor=pc;};
    services.instruction_cursor={&cursor,nullptr,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    bool observed=false;
    SourcePhysicsContactRecordObserver observer;
    if(policy==Policy::StoreObserver)observer=[&](const auto&){observed=true;};
    const int owner{};
    if(policy==Policy::MemoryObserver)source_runtime_register_write_observer(bank,&owner,[&](auto,auto){observed=true;});
    if(policy==Policy::GuardFault)source_runtime_register_write_guard(bank,&owner,[&](auto,auto){observed=true;throw Fault{};});
    SourceRuntimeWriteSerial serial(bank,SourceRuntimeWriteSerial::Visibility::CallBoundary);
    State state{bank,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    SourcePhysicsContactRuntimeAbi abi{};
    abi.gp={0u,camera_words,0x1234u,0x55u,root-(policy==Policy::WrongFrame?0x14cu:0x148u),0x77u,0x88u,address};
    abi.eflags=policy==Policy::Backward?0xed7u:0xad7u;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};
    state.load(abi);
    const auto before=state.save();
    const bool admit=eligible && policy==Policy::Ordinary && !reference();
    {
        std::optional<Scope> scope;
        if(!reference())scope.emplace(bank,serial,services,root,Scope::Root::Camera);
        need(clear_camera(state,site)==admit,"Camera REP admission differs");
    }
    if(policy==Policy::MemoryObserver)source_runtime_unregister_write_observer(bank,&owner);
    if(policy==Policy::GuardFault)source_runtime_unregister_write_guard(bank,&owner);
    need(!Scope::current(bank,services) && !observed && cursor==site,"Camera REP scope or observer differs");
    auto gp=before.gp;
    if(admit) {
        auto *destination=expected.data()+(address-bank_base);
        auto *start=destination;
        std::uint64_t count=camera_words;
        asm volatile("cld; rep stosl":"+D"(destination),"+c"(count):"a"(0u):"memory","cc");
        gp[7]+=std::uint32_t(destination-start);
        gp[1]=std::uint32_t(count);
    }
    const auto after=state.save();
    need(serial.load()==(admit?camera_words:0u),"Camera REP serial differs");
    need(std::memcmp(bank.view(bank_base,bank_size).data(),expected.data(),bank_size)==0,"Camera REP full bank differs");
    need(after.gp==gp && after.eflags==before.eflags && after.xmm==before.xmm &&
         after.x87_status==before.x87_status && after.floating_point->mxcsr==before.floating_point->mxcsr,
         "Camera REP publication differs");
}
} // namespace

int main() try {
    compare(false,0,Policy::Ordinary);
    compare(false,1,Policy::Ordinary);
    compare(false,512,Policy::Ordinary);
    compare(true,0,Policy::Ordinary);
    compare(true,1,Policy::Ordinary);
    compare(true,6,Policy::Ordinary);
    for(const auto policy:{Policy::GuardFault,Policy::MemoryObserver,Policy::StoreObserver,
            Policy::Tape,Policy::WrongFrame,Policy::Backward}) {
        compare(false,12,policy);
        compare(true,1,policy);
    }
    compare(true,1,Policy::Overlap);
    std::cout<<"19 receiver transfer comparisons passed\n";
    for(unsigned row=0;row<6;++row)compare_camera_clear(0xc3e51u,row,Policy::Ordinary);
    compare_camera_clear(0xc45d4u,0,Policy::Ordinary);
    compare_camera_clear(0xc3e51u,6,Policy::Ordinary,false);
    compare_camera_clear(0xc45d4u,1,Policy::Ordinary,false);
    for(const auto policy:{Policy::Backward,Policy::WrongFrame,Policy::GuardFault,Policy::MemoryObserver,Policy::StoreObserver})
        compare_camera_clear(0xc3e51u,0,policy);
    std::cout<<"14 camera REP comparisons passed\n";
    return 0;
} catch(const std::exception &error) {
    std::cerr<<error.what()<<'\n';
    return 1;
}
