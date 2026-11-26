#include "physics/runtime/native_tu15_mopp_ray_scale_vectors.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage = native_mopp_ray_scale_vectors_detail;
using Route = stage::Continuation;
constexpr std::uint32_t base=0x62000000u,stack=base+0x280u,ray=base+0x400u,vars=base+0x600u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};

std::uint32_t native_single_spill(long double x,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    float result;std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        :"=m"(result),"=m"(actual):"m"(x):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
#else
    throw std::runtime_error("Required TU1.5 FSTP32 backend unavailable");
#endif
}

void retained(WorldState &s,Route route) {
    switch(route) { case Route::original: goto L_original; case Route::read_end: goto L_read_end; case Route::read_factor: goto L_read_factor; case Route::read_start: goto L_read_start; case Route::reload_factor: goto L_reload_factor; case Route::read_end_scaled: goto L_read_end_scaled; case Route::read_origin: goto L_read_origin; case Route::scale_tail:return; }
L_original: s.instruction_literal(0x30b3aeu); s.xmm[1]=v(s.memory,(0x60u+s.gp[4]));
s.instruction_literal(0x30b3b3u); s.xmm[0]=_mm_sub_ps(s.xmm[0],s.xmm[1]);
s.instruction_literal(0x30b3b6u); vw(s.memory,s.observe,0x30b3b6u,(0x0u+s.gp[7]),s.xmm[0]);
L_read_end: s.instruction_literal(0x30b3b9u); s.xmm[0]=v(s.memory,(0x10u+s.gp[7]));
s.instruction_literal(0x30b3bdu); w(s.memory,s.observe,0x30b3bdu,(0x14u+s.gp[4]),4,s.reg(2,4,0));
L_read_factor: s.instruction_literal(0x30b3c1u); s.fp.push(static_cast<long double>(si(r(s.memory,(0x14u+s.gp[4]),4))));
s.instruction_literal(0x30b3c5u); s.xmm[0]=_mm_sub_ps(s.xmm[0],s.xmm[1]);
s.instruction_literal(0x30b3c8u); vw(s.memory,s.observe,0x30b3c8u,(0x10u+s.gp[7]),s.xmm[0]);
L_read_start: s.instruction_literal(0x30b3ccu); s.xmm[2]=v(s.memory,(0x0u+s.gp[7]));
s.instruction_literal(0x30b3cfu); w(s.memory,s.observe,0x30b3cfu,(0x14u+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));
L_reload_factor: s.instruction_literal(0x30b3d3u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x14u+s.gp[4]),4)));
s.instruction_literal(0x30b3d9u); s.xmm[3]=s.xmm[0];
s.instruction_literal(0x30b3dcu); s.xmm[3]=_mm_shuffle_ps(s.xmm[3],s.xmm[0],0);
s.instruction_literal(0x30b3e0u); s.xmm[3]=native_tu15_leaves::ordered_mul_ps(s.xmm[3],s.xmm[2]);
s.instruction_literal(0x30b3e3u); vw(s.memory,s.observe,0x30b3e3u,(0x0u+s.gp[7]),s.xmm[3]);
L_read_end_scaled: s.instruction_literal(0x30b3e6u); s.xmm[2]=v(s.memory,(0x10u+s.gp[7]));
s.instruction_literal(0x30b3eau); s.xmm[3]=s.xmm[0];
s.instruction_literal(0x30b3edu); s.xmm[3]=_mm_shuffle_ps(s.xmm[3],s.xmm[0],0);
s.instruction_literal(0x30b3f1u); s.xmm[3]=native_tu15_leaves::ordered_mul_ps(s.xmm[3],s.xmm[2]);
s.instruction_literal(0x30b3f4u); vw(s.memory,s.observe,0x30b3f4u,(0x10u+s.gp[7]),s.xmm[3]);
L_read_origin: s.instruction_literal(0x30b3f8u); s.xmm[2]=v(s.memory,(0x0u+s.gp[3]));
s.instruction_literal(0x30b3fbu); s.xmm[2]=native_tu15_leaves::ordered_add_ps(s.xmm[2],s.xmm[1]);
s.instruction_literal(0x30b3feu); s.xmm[1]=s.xmm[0];
s.instruction_literal(0x30b401u); s.xmm[1]=_mm_shuffle_ps(s.xmm[1],s.xmm[0],0);
s.instruction_literal(0x30b405u); s.xmm[1]=native_tu15_leaves::ordered_mul_ps(s.xmm[1],s.xmm[2]);
s.instruction_literal(0x30b408u); vw(s.memory,s.observe,0x30b408u,(0x40u+s.gp[4]),s.xmm[1]);
}

enum class Interrupt { none, write_fault, preimage_fault, observer_fault, trace, observer, rebind, move_frame };
enum class AccessKind { read,write };
struct Access {
    std::uint32_t pc,address;
    std::size_t width;
    AccessKind kind;
    bool operator==(const Access&)const=default;
};
struct Fixture {
    std::uint32_t fault_pc{},address{},ray_base{ray},stack_base{stack},variables{vars};
    Interrupt interrupt{};
    bool initial_observer{}, deny_scalar{}, no_direct_first{}, borrowed_first{};
    bool installed_preimage_fault{}, installed_observer_fault{};
    unsigned rounding{};
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;
    std::array<std::byte,8192> data{};
    std::vector<Access> accesses;
    std::vector<SourcePhysicsContactRecordStore> observed;
    std::vector<std::uint32_t> instructions;
    std::uint32_t pc{0x30b3aau};
    Route route{Route::original};
    std::string fault;
};
struct Bank final : SourcePhysicsContactMemory {
    const Fixture &fixture;
    Result &result;
    SourcePhysicsContactRecordObserver &observer;
    SourceDirectPageIndex index;
    std::function<void()> on_write;

    Bank(const Fixture &f,Result &r,SourcePhysicsContactRecordObserver &o)
        :fixture(f),result(r),observer(o) {
        direct_pages_=&index;
        auto &block=index.blocks[base>>22u];
        block=std::make_unique<SourceDirectPageIndex::Block>();
        if(!f.no_direct_first)(*block)[(base>>12u)&1023u]={r.data.data(),true};
        (*block)[((base>>12u)&1023u)+1u]={r.data.data()+4096u,true};
        const auto filled=_mm_set_ps(3.5f,-1.0f,9.0f,-0.5f);
        for(const auto address:{f.ray_base,f.ray_base+16u,f.variables,f.stack_base+0x60u})
            std::memcpy(r.data.data()+address-base,&filled,16u);
    }
    bool selected(std::uint32_t address)const {
        return result.pc==fixture.fault_pc&&address==fixture.address;
    }
    std::optional<std::span<const std::byte>> try_read_view(std::uint32_t address,std::size_t width)const noexcept override {
        if(!fixture.borrowed_first || address<base || std::uint64_t(address-base)+width>4096u)
            return std::nullopt;
        return std::span<const std::byte>{result.data}.subspan(address-base,width);
    }
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        if(const auto source=try_read_view(address,output.size())) {
            std::memcpy(output.data(),source->data(),output.size());
            return;
        }
        result.accesses.push_back({result.pc,address,output.size(),AccessKind::read});
        if((fixture.interrupt==Interrupt::preimage_fault&&selected(address)) ||
           (fixture.installed_preimage_fault&&observer&&result.pc==fixture.fault_pc&&address==fixture.address+8u))
            throw std::runtime_error("Read/preimage fault");
        std::memcpy(output.data(),result.data.data()+address-base,output.size());
    }
    void store(std::uint32_t address,std::span<const std::byte> input) {
        result.accesses.push_back({result.pc,address,input.size(),AccessKind::write});
        std::memcpy(result.data.data()+address-base,input.data(),input.size());
    }
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> input)noexcept override {
        if(input.size()!=4u||observer||(fixture.deny_scalar&&selected(address)))return false;
        store(address,input);
        return true;
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        if(try_callback_free_write(address,input))return;
        if(selected(address)&&fixture.interrupt==Interrupt::write_fault) {
            result.accesses.push_back({result.pc,address,input.size(),AccessKind::write});
            throw std::runtime_error("Write fault");
        }
        store(address,input);
        if(selected(address))on_write();
    }
};

Result run(const Fixture &fixture,bool native) {
    Result result;result.accesses.reserve(100);
    SourcePhysicsContactRecordObserver observe;
    SourcePhysicsWorldRuntimeServices services;
    bool trace{};
    services.instruction=[&](std::uint32_t at){result.pc=at;result.instructions.push_back(at);};
    auto rebound=[&](std::uint32_t at){result.pc=at;result.instructions.push_back(at|0x80000000u);};
    services.instruction_cursor={&result.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    const auto collector=[&](const SourcePhysicsContactRecordStore &store) {
        result.observed.push_back(store);
        if(store.instruction==fixture.fault_pc &&
           ((fixture.interrupt==Interrupt::observer_fault&&store.address==fixture.address) ||
            (fixture.installed_observer_fault&&store.address==fixture.address+8u)))
            throw std::runtime_error("Observer fault");
    };
    if(fixture.initial_observer)observe=collector;
    Bank bank{fixture,result,observe};
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0x1234a432u,1u,0xbc523417u,fixture.variables,fixture.stack_base,
        0x9abcdee0u,4u,fixture.ray_base};
    input.eflags=0x256u;input.x87_depth=2;input.x87_status=0x3200u;
    input.x87[0]=1.25L;input.x87[1]=-1.0L;
    input.xmm[0]={0x3f800000u,0xff852112u,0x7fc22222u,0x80000000u};
    const auto cw=std::uint16_t(0x37fu|fixture.rounding<<10u);
    input.floating_point=SourcePhysicsFactoryFloatingPoint{cw,0x1f80u|(fixture.rounding<<13u)};
    asm volatile("fnclex; fldcw %0"::"m"(cw));
    _mm_setcsr(input.floating_point->mxcsr);
    WorldState state{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.on_write=[&] {
        if(fixture.interrupt==Interrupt::trace)trace=true;
        if(fixture.interrupt==Interrupt::observer)observe=collector;
        if(fixture.interrupt==Interrupt::rebind)services.instruction=rebound;
        if(fixture.interrupt==Interrupt::move_frame) {
            state.gp[4]+=0x100u;
            const auto bits=std::uint32_t{0x40000000u};
            std::memcpy(result.data.data()+state.gp[4]-base+0x14u,&bits,sizeof(bits));
        }
    };
    try {
        if(native)result.route=stage::scale_vectors(state,native_single_spill);
        retained(state,result.route);
    } catch(const std::runtime_error &e) {
        result.fault=e.what();
    }
    result.abi=state.save();
    return result;
}
void compare(const Fixture &fixture,Route route) {
    const auto a=run(fixture,false),b=run(fixture,true);
    if(stage::reference_scale_vectors())route=Route::original;
    const auto &x=a.abi,&y=b.abi;
    if((b.fault.empty()&&b.route!=route)||a.fault!=b.fault||a.accesses!=b.accesses||
        a.observed!=b.observed||a.instructions!=b.instructions||a.pc!=b.pc||a.data!=b.data||
        x.gp!=y.gp||x.xmm!=y.xmm||x.eflags!=y.eflags||x.x87_status!=y.x87_status||
        x.x87_depth!=y.x87_depth||x.floating_point->mxcsr!=y.floating_point->mxcsr||
        x.floating_point->x87_control_word!=y.floating_point->x87_control_word) {
        std::cerr<<"fixture pc "<<std::hex<<fixture.fault_pc<<" address "<<fixture.address
                 <<" ends "<<a.pc<<"/"<<b.pc<<" routes "<<int(route)<<"/"<<int(b.route)<<'\n';
        throw std::runtime_error("Packed scale provider, ABI or retained continuation differs");
    }
    for(unsigned n=0;n<x.x87_depth;++n)
        if(std::memcmp(&x.x87[n],&y.x87[n],10u))throw std::runtime_error("Packed scale live x87 lane differs");
}
}

int main() try {
    constexpr std::array pcs{0x30b3b6u,0x30b3c8u,0x30b3e3u,0x30b3f4u,0x30b408u};
    constexpr std::array addresses{ray,ray+16u,ray,ray+16u,stack+0x40u};
    constexpr std::array next{Route::read_end,Route::read_start,Route::read_end_scaled,Route::read_origin,Route::scale_tail};
    unsigned compared{};
    for(unsigned rounding=0;rounding<4u;++rounding) {
        Fixture normal;normal.rounding=rounding;
        compare(normal,Route::scale_tail);++compared;
        for(unsigned i=0;i<pcs.size();++i) {
            for(unsigned half=0;half<2;++half) {
                auto f=normal;f.fault_pc=pcs[i];f.address=addresses[i]+half*8u;
                f.interrupt=Interrupt::write_fault;
                compare(f,next[i]);++compared;
                f.initial_observer=true;f.interrupt=Interrupt::preimage_fault;
                compare(f,Route::original);++compared;
                f.interrupt=Interrupt::observer_fault;
                compare(f,Route::original);++compared;
            }
            auto between=normal;between.fault_pc=pcs[i];between.address=addresses[i];
            for(const auto type:{Interrupt::trace,Interrupt::observer,Interrupt::rebind}) {
                between.interrupt=type;
                compare(between,next[i]);++compared;
            }
            between.interrupt=Interrupt::observer;
            between.installed_preimage_fault=true;
            compare(between,next[i]);++compared;
            between.installed_preimage_fault=false;between.installed_observer_fault=true;
            compare(between,next[i]);++compared;
        }
        for(unsigned i=0;i<2;++i) {
            auto f=normal;f.deny_scalar=true;f.fault_pc=i?0x30b3cfu:0x30b3bdu;
            f.address=stack+0x14u;
            const auto next_pc=i?Route::reload_factor:Route::read_factor;
            const auto continued=native_mopp_ray_scale_tail_detail::reference_scale_provider()?next_pc:Route::scale_tail;
            compare(f,continued);++compared;
            f.interrupt=Interrupt::move_frame;
            compare(f,continued);++compared;
            for(const auto type:{Interrupt::trace,Interrupt::observer,Interrupt::rebind}) {
                f.interrupt=type;
                compare(f,next_pc);++compared;
            }
            f.interrupt=Interrupt::write_fault;
            compare(f,next_pc);++compared;
        }
        auto indirect=normal;indirect.no_direct_first=true;
        compare(indirect,Route::original);++compared;
        indirect.borrowed_first=true;
        compare(indirect,native_mopp_ray_scale_tail_detail::reference_scale_provider()?Route::original:Route::scale_tail);
        ++compared;
        indirect.borrowed_first=false;
        indirect.interrupt=Interrupt::preimage_fault;indirect.fault_pc=0x30b3aeu;indirect.address=stack+0x64u;
        compare(indirect,Route::original);++compared;
    }
    std::cout<<"30B3AE packed scale: "<<compared<<" exact split-store, continuation and fault comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}
