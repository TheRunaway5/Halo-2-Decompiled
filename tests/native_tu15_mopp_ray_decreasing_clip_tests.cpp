#include "physics/runtime/native_tu15_mopp_ray_decreasing_clip.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage=native_mopp_ray_decreasing_clip_detail;
using Route=stage::Continuation;
constexpr std::uint32_t base=0x62000000u,stack=base+0x180u,rays=base+0x500u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};

struct Fixture {
    bool after{}, observer{}, observer_fault{}, trace{}, authentic=true;
    std::uint32_t sp=stack, ray=rays, a=0xbf800000u,b=0x40000000u,epsilon=0u;
    unsigned constants=3, borrowed_constants{}, depth=3, precision=3, rounding{}, fault_half=1;
    std::uint32_t deny{},fault_pc{},reenter_pc{};
};
struct Access {
    std::uint32_t pc,address;
    std::size_t width;
    bool write;
    bool operator==(const Access &)const=default;
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;
    std::array<std::byte,8192> data{};
    std::vector<Access> accesses;
    std::vector<SourcePhysicsWorldRuntimeAbi> states;
    std::vector<SourcePhysicsContactRecordStore> observed;
    std::vector<std::uint32_t> instructions;
    std::uint32_t pc{};
    Route route=Route::original;
    std::string fault;
};

struct Bank final : SourcePhysicsContactMemory {
    const Fixture &f;
    Result &r;
    SourceDirectPageIndex index;
    std::array<std::byte,4096> epsilon{},one{};
    std::function<SourcePhysicsWorldRuntimeAbi()> snapshot;
    std::function<void()> on_write;
    unsigned faults{};

    void map(std::uint32_t address,std::byte *data,bool writable) {
        auto &block=index.blocks[address>>22u];
        if(!block)block=std::make_unique<SourceDirectPageIndex::Block>();
        (*block)[(address>>12u)&1023u]={data,writable};
    }
    template<class T> void put(std::uint32_t address,const T &value) const {
        std::memcpy(r.data.data()+address-base,&value,sizeof(value));
    }
    Bank(const Fixture &fixture,Result &result):f(fixture),r(result) {
        direct_pages_=&index;
        map(base,r.data.data(),true);map(base+4096u,r.data.data()+4096u,true);
        const std::array ones{1.0f,1.0f,1.0f,1.0f};
        std::memcpy(one.data()+0x500u,ones.data(),16u);
        std::memcpy(epsilon.data()+0xe8u,&f.epsilon,4u);
        if(f.constants&1u)map(0x46a000u,epsilon.data(),false);
        if(f.constants&2u)map(0x41c000u,one.data(),false);
        put(f.ray,std::array{0x7fc42017u,0x41900000u,0x7f813403u,0x80000000u});
        put(f.ray+0x10u,std::array{0x00000001u,0x41400000u,0x7fc01204u,0u});
        put(f.sp+(f.after?0x30u:0x18u),f.a);put(f.sp+(f.after?0x34u:0x2cu),f.b);
    }
    void access(std::uint32_t address,std::size_t width,bool write)const {
        r.accesses.push_back({r.pc,address,width,write});r.states.push_back(snapshot());
    }
    std::optional<std::span<const std::byte>> try_read_view(
        std::uint32_t address,std::size_t size) const noexcept override {
        if((f.borrowed_constants&1u) && address==0x46a0e8u && size==4u)
            return std::span<const std::byte>{epsilon}.subspan(0xe8u,size);
        if((f.borrowed_constants&2u) && address==0x41c500u && size==16u)
            return std::span<const std::byte>{one}.subspan(0x500u,size);
        return std::nullopt;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(const auto borrowed=try_read_view(address,out.size())) {
            std::memcpy(out.data(),borrowed->data(),out.size());
            return;
        }
        access(address,out.size(),false);
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected decreasing-clip read fault");
        const auto *source=address>>12u==0x46au?epsilon.data()+(address&4095u):
            address>>12u==0x41cu?one.data()+(address&4095u):r.data.data()+address-base;
        std::memcpy(out.data(),source,out.size());
    }
    void read_vector(std::uint32_t address,std::span<std::byte,16> out) const override {
        if(const auto borrowed=try_read_view(address,out.size()))
            std::memcpy(out.data(),borrowed->data(),out.size());
        else SourcePhysicsContactMemory::read_vector(address,out);
    }
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> in)noexcept override {
        if(f.observer||r.pc==f.deny||in.size()!=4u||!direct_page(address,in.size()))return false;
        r.accesses.push_back({r.pc,address,in.size(),true});
        std::memcpy(r.data.data()+address-base,in.data(),in.size());
        return true;
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        if(try_callback_free_write(address,in))return;
        access(address,in.size(),true);
        if(r.pc==f.fault_pc&&++faults==f.fault_half)throw std::runtime_error("Injected decreasing-clip store fault");
        std::memcpy(r.data.data()+address-base,in.data(),in.size());
        if(r.pc==f.reenter_pc)on_write();
    }
};

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
void exact_before(WorldState &s,Route route) {
    switch(route) { case Route::original: goto L_original; case Route::ratio: goto L_ratio; case Route::reload_fraction: goto L_reload_fraction; case Route::read_one: goto L_read_one; case Route::done: return; default: throw std::logic_error("Wrong clip test route"); }
L_original: s.instruction_literal(0x30b022u); s.fp.push(ext(f(r(s.memory,(0x18u+s.gp[4]),4))));
s.instruction_literal(0x30b026u); halo2::native_mopp_ray_slab_bounds_detail::wide_arithmetic<true>(s,r(s.memory,(0x2cu+s.gp[4]),4));
s.instruction_literal(0x30b02au); s.fp.comparison(ext(f(r(s.memory,(0x46a0e8u),4))),1);
s.instruction_literal(0x30b030u); s.set(0,s.fp.word(),2,0);
s.instruction_literal(0x30b032u); s.flags.logic((s.reg(0,1,8))&(0x5u),1);
s.instruction_literal(0x30b035u); if(s.flags.pf)goto L30b06f;
L_ratio: s.instruction_literal(0x30b037u); s.fp.push(ext(f(r(s.memory,(0x2cu+s.gp[4]),4))));
s.instruction_literal(0x30b03bu); halo2::native_mopp_ray_slab_bounds_detail::wide_arithmetic<false>(s,r(s.memory,(0x18u+s.gp[4]),4));
s.instruction_literal(0x30b03fu); halo2::native_mopp_ray_decreasing_clip_detail::reverse_divide(s,r(s.memory,(0x2cu+s.gp[4]),4));
s.instruction_literal(0x30b043u); w(s.memory,s.observe,0x30b043u,(0x2cu+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_reload_fraction: s.instruction_literal(0x30b047u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x2cu+s.gp[4]),4)));
s.instruction_literal(0x30b04du); s.xmm[0]=_mm_shuffle_ps(s.xmm[0],s.xmm[0],0);
s.instruction_literal(0x30b051u); s.xmm[3]=s.xmm[0];
s.instruction_literal(0x30b054u); s.xmm[3]=halo2::native_tu15_leaves::ordered_mul_ps(s.xmm[3],s.xmm[2]);
L_read_one: s.instruction_literal(0x30b057u); s.xmm[2]=v(s.memory,(0x41c500u));
s.instruction_literal(0x30b05eu); s.xmm[2]=_mm_sub_ps(s.xmm[2],s.xmm[0]);
s.instruction_literal(0x30b061u); s.xmm[2]=halo2::native_tu15_leaves::ordered_mul_ps(s.xmm[2],s.xmm[1]);
s.instruction_literal(0x30b064u); s.xmm[2]=halo2::native_tu15_leaves::ordered_add_ps(s.xmm[2],s.xmm[3]);
s.instruction_literal(0x30b067u); vw(s.memory,s.observe,0x30b067u,(0x90u+s.gp[4]),s.xmm[2]);
L30b06f: return;
}

void exact_after(WorldState &s,Route route) {
    switch(route) { case Route::original: goto L_original; case Route::ratio: goto L_ratio; case Route::reload_fraction: goto L_reload_fraction; case Route::read_start: goto L_read_start; case Route::done: return; default: throw std::logic_error("Wrong clip test route"); }
L_original: s.instruction_literal(0x30b082u); s.fp.push(ext(f(r(s.memory,(0x30u+s.gp[4]),4))));
s.instruction_literal(0x30b086u); halo2::native_mopp_ray_slab_bounds_detail::wide_arithmetic<true>(s,r(s.memory,(0x34u+s.gp[4]),4));
s.instruction_literal(0x30b08au); s.fp.comparison(ext(f(r(s.memory,(0x46a0e8u),4))),1);
s.instruction_literal(0x30b090u); s.set(0,s.fp.word(),2,0);
s.instruction_literal(0x30b092u); s.flags.logic((s.reg(0,1,8))&(0x5u),1);
s.instruction_literal(0x30b095u); if(s.flags.pf)goto L30b0d1;
L_ratio: s.instruction_literal(0x30b097u); s.fp.push(ext(f(r(s.memory,(0x34u+s.gp[4]),4))));
s.instruction_literal(0x30b09bu); s.xmm[1]=v(s.memory,(0x10u+s.gp[7]));
s.instruction_literal(0x30b09fu); halo2::native_mopp_ray_slab_bounds_detail::wide_arithmetic<false>(s,r(s.memory,(0x30u+s.gp[4]),4));
s.instruction_literal(0x30b0a3u); s.xmm[3]=v(s.memory,(0x41c500u));
s.instruction_literal(0x30b0aau); halo2::native_mopp_ray_decreasing_clip_detail::reverse_divide(s,r(s.memory,(0x34u+s.gp[4]),4));
s.instruction_literal(0x30b0aeu); w(s.memory,s.observe,0x30b0aeu,(0x30u+s.gp[4]),4,native_single_spill(s.fp.st(0),s.fp.status));s.fp.pop();
L_reload_fraction: s.instruction_literal(0x30b0b2u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x30u+s.gp[4]),4)));
s.instruction_literal(0x30b0b8u); s.xmm[0]=_mm_shuffle_ps(s.xmm[0],s.xmm[0],0);
s.instruction_literal(0x30b0bcu); s.xmm[2]=s.xmm[0];
s.instruction_literal(0x30b0bfu); s.xmm[2]=halo2::native_tu15_leaves::ordered_mul_ps(s.xmm[2],s.xmm[1]);
L_read_start: s.instruction_literal(0x30b0c2u); s.xmm[1]=v(s.memory,(0x0u+s.gp[7]));
s.instruction_literal(0x30b0c5u); s.xmm[3]=_mm_sub_ps(s.xmm[3],s.xmm[0]);
s.instruction_literal(0x30b0c8u); s.xmm[3]=halo2::native_tu15_leaves::ordered_mul_ps(s.xmm[3],s.xmm[1]);
s.instruction_literal(0x30b0cbu); s.xmm[3]=halo2::native_tu15_leaves::ordered_add_ps(s.xmm[3],s.xmm[2]);
s.instruction_literal(0x30b0ceu); vw(s.memory,s.observe,0x30b0ceu,(0x0u+s.gp[7]),s.xmm[3]);
L30b0d1: return;
}


Result run(const Fixture &f,bool native) {
    Result r;r.pc=f.after?0x30b07du:0x30aec3u;r.accesses.reserve(24);Bank bank{f,r};
    SourcePhysicsWorldRuntimeServices services;
    auto trace=f.trace;
    services.instruction=[&](std::uint32_t pc){r.pc=pc;r.instructions.push_back(pc);};
    services.instruction_cursor={&r.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    if(!f.authentic)services.instruction_cursor={};
    SourcePhysicsContactRecordObserver observe;
    auto on_observe=[&](const auto &event) {
        r.observed.push_back(event);r.states.push_back(bank.snapshot());
        if(f.observer_fault)throw std::runtime_error("Injected decreasing-clip observer fault");
    };
    if(f.observer)observe=on_observe;
    bank.on_write=[&] {trace=true;observe=on_observe;};
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0xf2410000u,2u,1u,0x7100u,f.sp,0x2020u,0x7700u,f.ray};
    input.eflags=0x256u;input.x87_depth=f.depth;input.x87_status=0x3200u;
    for(unsigned i=0;i<f.depth;++i)input.x87[i]=0.125L*static_cast<long double>(i)+0.25L;
    input.xmm[1]={0x3f800000u,0x7f812345u,0x80000000u,0u};
    input.xmm[2]={0xc1800000u,0x7fc54321u,0u,0xbf800000u};
    const auto cw=std::uint16_t(0x7fu|(f.precision<<8u)|(f.rounding<<10u));
    input.floating_point=SourcePhysicsFactoryFloatingPoint{cw,0x1f80u};
    asm volatile("fnclex; fldcw %0"::"m"(cw));_mm_setcsr(0x1f80u|(f.rounding<<13u));
    WorldState state{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.snapshot=[&]{return state.save();};
    try {
        if(native)r.route=f.after?stage::decreasing_clip<true>(state):stage::decreasing_clip<false>(state);
        if(f.after)exact_after(state,r.route);else exact_before(state,r.route);
    } catch(const std::runtime_error &error) {r.fault=error.what();}
    r.abi=state.save();
    return r;
}
bool same(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_status!=b.x87_status||
       a.x87_depth!=b.x87_depth||a.floating_point->x87_control_word!=b.floating_point->x87_control_word||
       a.floating_point->mxcsr!=b.floating_point->mxcsr)return false;
    for(unsigned i=0;i<a.x87_depth;++i)if(std::memcmp(&a.x87[i],&b.x87[i],10u))return false;
    return true;
}
void compare(const Fixture &f,Route expected) {
    if(stage::reference_decreasing_clip())expected=Route::original;
    const auto a=run(f,false),b=run(f,true);
    if(a.pc!=b.pc||a.fault!=b.fault||a.data!=b.data||a.accesses!=b.accesses||
       a.observed!=b.observed||a.instructions!=b.instructions||!same(a.abi,b.abi)||
       a.states.size()!=b.states.size()||b.route!=expected) {
        std::cerr<<"Decreasing clip "<<std::hex<<f.after<<'/'<<f.fault_pc<<'/'<<f.deny<<" PC "<<a.pc<<'/'<<b.pc
                 <<" route "<<int(expected)<<'/'<<int(b.route)<<" x87 "<<a.abi.x87_status<<'/'<<b.abi.x87_status
                 <<" fault "<<a.fault<<'/'<<b.fault<<'\n';
        throw std::runtime_error("MOPP decreasing clip continuation, provider or ABI differs");
    }
    for(unsigned i=0;i<a.states.size();++i)
        if(!same(a.states[i],b.states[i]))throw std::runtime_error("Provider saw a different decreasing-clip x87 frame");
}
unsigned prove() {
    unsigned compared{};
    auto check=[&](const Fixture &f,Route expected){compare(f,expected);++compared;};
    for(const bool after:{false,true})for(const auto precision:{0u,2u,3u})for(unsigned rc=0;rc<4u;++rc) {
        Fixture f;f.after=after;f.precision=precision;f.rounding=rc;
        check(f,Route::done);
        auto skip=f;skip.a=0x3f800000u;check(skip,Route::done);
        for(const auto bits:{0u,0x80000000u,0x7fc12345u,0x7f821242u,1u,0x80000001u,0x7f800000u,0xff800000u}) {
            auto edge=f;edge.a=bits;check(edge,Route::done);
            edge=f;edge.b=bits;check(edge,Route::done);
        }
        auto refused=f;refused.deny=after?0x30b0aeu:0x30b043u;
        check(refused,stage::reference_decreasing_clip_provider()?Route::reload_fraction:Route::done);
        refused.reenter_pc=refused.deny;check(refused,Route::reload_fraction);
        refused.fault_pc=refused.deny;check(refused,Route::original);
        for(const auto half:{1u,2u}) {
            auto fault=f;fault.fault_pc=after?0x30b0ceu:0x30b067u;fault.fault_half=half;
            check(fault,Route::original);
        }
        auto dynamic=f;dynamic.reenter_pc=after?0x30b0ceu:0x30b067u;check(dynamic,Route::done);
        dynamic.observer_fault=true;check(dynamic,Route::original);
        auto observed=f;observed.observer=true;observed.observer_fault=true;check(observed,Route::original);
        auto trace=f;trace.trace=true;check(trace,Route::original);
        auto weak=f;weak.authentic=false;check(weak,Route::original);
        auto full=f;full.depth=8;check(full,Route::original);
        auto borrowed=f;borrowed.constants=0;borrowed.borrowed_constants=3;
        check(borrowed,Route::done);
        borrowed.a=0x3f800000u;check(borrowed,Route::done);
        borrowed.a=f.a;borrowed.borrowed_constants=1;
        borrowed.fault_pc=after?0x30b0a3u:0x30b057u;
        check(borrowed,after?Route::ratio:Route::read_one);
        borrowed.borrowed_constants=2;
        borrowed.fault_pc=after?0x30b08au:0x30b02au;
        check(borrowed,Route::original);
        auto no_one=f;no_one.constants=1;
        check(no_one,after?Route::ratio:Route::read_one);
        no_one.fault_pc=after?0x30b0a3u:0x30b057u;
        check(no_one,after?Route::ratio:Route::read_one);
        auto no_epsilon=f;no_epsilon.constants=2;
        no_epsilon.fault_pc=after?0x30b08au:0x30b02au;check(no_epsilon,Route::original);
        auto crossed=f;crossed.sp=base+(after?0xfccu:0xfdau);check(crossed,Route::original);
        if(after) {
            crossed=f;crossed.ray=base+0xff8u;check(crossed,Route::read_start);
            crossed.ray=base+0xfe8u;check(crossed,Route::ratio);
            auto alias=f;alias.ray=stack+0x20u;check(alias,Route::done);
        }
    }
    return compared;
}
}
int main() try {
    std::cout<<"30B022/30B082 decreasing clips: "<<prove()<<" exact continuation and provider comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}
