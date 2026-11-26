#include "physics/runtime/native_tu15_mopp_ray_decreasing_resume.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage=native_mopp_ray_decreasing_resume_detail;
using Route=stage::Continuation;
using IncreasingPhase=stage::IncreasingPhase;
constexpr std::uint32_t base=0x62000000u, stack=base+0x180u, cache=base+0x500u,
    code=base+0x700u, origin=base+0x900u, rays=base+0xb00u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};

enum class Exit { opcode, empty };
struct Result;
Exit model(WorldState &s,Result &result,bool native);

struct Fixture {
    std::uint32_t ray=rays, object=cache, constants=3, borrowed_constants{}, program=code, source=origin;
    std::uint32_t frame=stack, fraction=0x3f400000u, scale=0x3f000000u, plane=0xc2200000u, axis=1u;
    unsigned precision=3, rounding{}, depth=3, vector_variant{}, fault_half{}, mxcsr=0x1f80u;
    std::uint32_t fault_pc{}, deny_scalar{}, reenter_pc{};
    bool observer{}, trace{}, authentic=true, observer_throws{}, move_frame{};
    std::uint32_t left=0x3fc00000u,right=0x3f400000u;
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
    std::array<Route,3> routes{};
    std::uint32_t pc{};
    unsigned mxcsr{};
    Exit exit=Exit::opcode;
    std::string fault;
};

struct Bank final : SourcePhysicsContactMemory {
    const Fixture &f;
    Result &r;
    SourceDirectPageIndex index;
    std::array<std::byte,4096> unit{}, ones{};
    std::function<SourcePhysicsWorldRuntimeAbi()> snapshot;
    std::function<void()> on_write;
    unsigned fault_matches{};

    void map(std::uint32_t address,std::byte *bytes,bool writable) {
        auto &block=index.blocks[address>>22u];
        if(!block)block=std::make_unique<SourceDirectPageIndex::Block>();
        (*block)[(address>>12u)&1023u]={bytes,writable};
    }
    template<class T> void seed(std::uint32_t address,const T &value) {
        std::memcpy(r.data.data()+address-base,&value,sizeof(value));
    }
    Bank(const Fixture &fixture,Result &result):f(fixture),r(result) {
        direct_pages_=&index;
        map(base,r.data.data(),true);
        map(base+4096u,r.data.data()+4096u,true);
        const std::array uniform{1.0f,1.0f,1.0f,1.0f};
        std::memcpy(unit.data()+0xd0u,uniform.data(),4u);
        const auto epsilon=std::uint32_t{0x358637bdu};
        std::memcpy(unit.data()+0xe8u,&epsilon,4u);
        std::memcpy(ones.data()+0x500u,uniform.data(),16u);
        if(f.constants&1u)map(0x46a000u,unit.data(),false);
        if(f.constants&2u)map(0x41c000u,ones.data(),false);
        seed(f.frame+0x14u,17u);seed(f.frame+0x20u,f.plane);seed(f.frame+0x24u,4u);
        seed(f.frame+0x28u,f.object);seed(f.frame+0x3cu,f.axis);
        seed(f.frame+0x18u,f.left);seed(f.frame+0x2cu,f.right);
        seed(f.frame+0x30u,f.left);seed(f.frame+0x34u,f.right);
        seed(f.object+0x10u,f.program);
        auto vector=std::array{2.0f,-4.0f,6.0f,-8.0f};
        if(f.vector_variant!=0) {
            const unsigned bits=0x7f8010a1u+(f.vector_variant<<17u);
            vector={std::bit_cast<float>(bits),-0.0f,std::bit_cast<float>(0xffca9021u),0.0f};
        }
        seed(f.object+0x20u,vector);
        seed(f.object+0x30u,std::array{-1.0f,3.0f,-5.0f,7.0f});
        seed(f.object+0x54u,f.fraction);
        seed(f.program,std::array{-0.5f,0.125f,-0.25f,0.5f});
        seed(f.source,std::array{0.625f,-0.5f,0.375f,0.25f});
        seed(f.source+0x14u,f.scale);
    }
    void access(std::uint32_t address,std::size_t width,bool write)const {
        r.accesses.push_back({r.pc,address,width,write});
        r.states.push_back(snapshot());
    }
    std::optional<std::span<const std::byte>> try_read_view(
        std::uint32_t address,std::size_t size) const noexcept override {
        if((f.borrowed_constants&1u) && (address==0x46a0d0u||address==0x46a0e8u) && size==4u)
            return std::span<const std::byte>{unit}.subspan(address&4095u,size);
        if((f.borrowed_constants&2u) && address==0x41c500u && size==16u)
            return std::span<const std::byte>{ones}.subspan(0x500u,size);
        return std::nullopt;
    }
    void read(std::uint32_t address,std::span<std::byte> data)const override {
        if(const auto borrowed=try_read_view(address,data.size())) {
            std::memcpy(data.data(),borrowed->data(),data.size());
            return;
        }
        access(address,data.size(),false);
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected original read fault");
        const std::byte *source;
        if(address>>12u==0x46au) source=unit.data()+(address&4095u);
        else if(address>>12u==0x41cu) source=ones.data()+(address&4095u);
        else source=r.data.data()+address-base;
        std::memcpy(data.data(),source,data.size());
    }
    void read_vector(std::uint32_t address,std::span<std::byte,16> out) const override {
        if(const auto borrowed=try_read_view(address,out.size()))
            std::memcpy(out.data(),borrowed->data(),out.size());
        else SourcePhysicsContactMemory::read_vector(address,out);
    }
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> bytes)noexcept override {
        if(f.observer||r.pc==f.deny_scalar||bytes.size()!=4u||!direct_page(address,bytes.size()))return false;
        r.accesses.push_back({r.pc,address,bytes.size(),true});
        std::memcpy(r.data.data()+address-base,bytes.data(),bytes.size());
        return true;
    }
    void write(std::uint32_t address,std::span<const std::byte> bytes)override {
        if(try_callback_free_write(address,bytes))return;
        access(address,bytes.size(),true);
        if(r.pc==f.fault_pc && ++fault_matches==f.fault_half)
            throw std::runtime_error("Injected original store fault");
        std::memcpy(r.data.data()+address-base,bytes.data(),bytes.size());
        if(f.reenter_pc==r.pc)on_write();
    }
};

Exit exact_resume(WorldState &s,std::array<Route,3> &routes,bool native) {
L30b0d1: { using namespace halo2::native_mopp_ray_decreasing_resume_detail; if(native) switch((routes[0]=decreasing_resume<Phase::fraction>(s))) { case Continuation::original: break; case Continuation::read_fraction: goto L_native_decreasing_30b0d5; case Continuation::shorten: goto L30b0e9; case Continuation::advance: goto L30b16e; default: throw std::logic_error("Invalid MOPP decreasing resume continuation"); } } s.instruction_literal(0x30b0d1u); s.set(1,r(s.memory,(0x28u+s.gp[4]),4),4,0);
L_native_decreasing_30b0d5: s.instruction_literal(0x30b0d5u); s.fp.push(ext(f(r(s.memory,(0x54u+s.gp[1]),4))));
s.instruction_literal(0x30b0d8u); s.fp.comparison(ext(f(r(s.memory,(0x46a0d0u),4))),1);
s.instruction_literal(0x30b0deu); s.set(0,s.fp.word(),2,0);
s.instruction_literal(0x30b0e0u); s.flags.logic((s.reg(0,1,8))&(0x5u),1);
s.instruction_literal(0x30b0e3u); if(s.flags.pf)goto L30b16e;
L30b0e9: { using namespace halo2::native_mopp_ray_decreasing_resume_detail; if(native) switch((routes[1]=decreasing_resume<Phase::shorten>(s))) { case Continuation::original: break; case Continuation::read_end: goto L_native_decreasing_30b0ec; case Continuation::read_one: goto L_native_decreasing_30b0f0; case Continuation::read_code_pointer: goto L_native_decreasing_30b0f7; case Continuation::read_code: goto L_native_decreasing_30b0fa; case Continuation::save_fraction: goto L_native_decreasing_30b0fd; case Continuation::refill_fraction: goto L_native_decreasing_30b101; case Continuation::read_start: goto L_native_decreasing_30b111; case Continuation::read_scale: goto L_native_decreasing_30b125; case Continuation::read_short_end: goto L_native_decreasing_30b128; case Continuation::save_scale: goto L_native_decreasing_30b12c; case Continuation::refill_scale: goto L_native_decreasing_30b130; case Continuation::read_axis: goto L_native_decreasing_30b136; case Continuation::read_translation: goto L_native_decreasing_30b14b; case Continuation::read_scaled_end: goto L_native_decreasing_30b14e; case Continuation::branch_axis: goto L_native_decreasing_30b159; case Continuation::read_endpoint: goto L_native_decreasing_30b15b; case Continuation::advance: goto L30b16e; case Continuation::empty: goto L30b6af; default: throw std::logic_error("Invalid MOPP decreasing resume continuation"); } } s.instruction_literal(0x30b0e9u); s.set(0,r(s.memory,(0x54u+s.gp[1]),4),4,0);
L_native_decreasing_30b0ec: s.instruction_literal(0x30b0ecu); s.xmm[2]=v(s.memory,(0x30u+s.gp[1]));
L_native_decreasing_30b0f0: s.instruction_literal(0x30b0f0u); s.xmm[4]=v(s.memory,(0x41c500u));
L_native_decreasing_30b0f7: s.instruction_literal(0x30b0f7u); s.set(2,r(s.memory,(0x10u+s.gp[1]),4),4,0);
L_native_decreasing_30b0fa: s.instruction_literal(0x30b0fau); s.xmm[1]=v(s.memory,(0x0u+s.gp[2]));
L_native_decreasing_30b0fd: s.instruction_literal(0x30b0fdu); w(s.memory,s.observe,0x30b0fdu,(0x74u+s.gp[4]),4,s.reg(0,4,0));
L_native_decreasing_30b101: s.instruction_literal(0x30b101u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x74u+s.gp[4]),4)));
s.instruction_literal(0x30b107u); s.xmm[0]=_mm_shuffle_ps(s.xmm[0],s.xmm[0],0);
s.instruction_literal(0x30b10bu); s.xmm[3]=s.xmm[0];
s.instruction_literal(0x30b10eu); s.xmm[3]=halo2::native_tu15_leaves::ordered_mul_ps(s.xmm[3],s.xmm[2]);
L_native_decreasing_30b111: s.instruction_literal(0x30b111u); s.xmm[2]=v(s.memory,(0x20u+s.gp[1]));
s.instruction_literal(0x30b115u); s.xmm[4]=_mm_sub_ps(s.xmm[4],s.xmm[0]);
s.instruction_literal(0x30b118u); s.xmm[4]=halo2::native_tu15_leaves::ordered_mul_ps(s.xmm[4],s.xmm[2]);
s.instruction_literal(0x30b11bu); s.xmm[4]=halo2::native_tu15_leaves::ordered_add_ps(s.xmm[4],s.xmm[3]);
s.instruction_literal(0x30b11eu); s.xmm[4]=_mm_sub_ps(s.xmm[4],s.xmm[1]);
s.instruction_literal(0x30b121u); vw(s.memory,s.observe,0x30b121u,(0x10u+s.gp[7]),s.xmm[4]);
L_native_decreasing_30b125: s.instruction_literal(0x30b125u); s.set(0,r(s.memory,(0x14u+s.gp[3]),4),4,0);
L_native_decreasing_30b128: s.instruction_literal(0x30b128u); s.xmm[1]=v(s.memory,(0x10u+s.gp[7]));
L_native_decreasing_30b12c: s.instruction_literal(0x30b12cu); w(s.memory,s.observe,0x30b12cu,(0x78u+s.gp[4]),4,s.reg(0,4,0));
L_native_decreasing_30b130: s.instruction_literal(0x30b130u); s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x78u+s.gp[4]),4)));
L_native_decreasing_30b136: s.instruction_literal(0x30b136u); s.set(0,r(s.memory,(0x3cu+s.gp[4]),4),4,0);
s.instruction_literal(0x30b13au); s.flags.minus(s.reg(0,4,0),0x3u,4);
s.instruction_literal(0x30b13du); s.xmm[2]=s.xmm[0];
s.instruction_literal(0x30b140u); s.xmm[2]=_mm_shuffle_ps(s.xmm[2],s.xmm[0],0);
s.instruction_literal(0x30b144u); s.xmm[2]=halo2::native_tu15_leaves::ordered_mul_ps(s.xmm[2],s.xmm[1]);
s.instruction_literal(0x30b147u); vw(s.memory,s.observe,0x30b147u,(0x10u+s.gp[7]),s.xmm[2]);
L_native_decreasing_30b14b: s.instruction_literal(0x30b14bu); s.xmm[0]=v(s.memory,(0x0u+s.gp[3]));
L_native_decreasing_30b14e: s.instruction_literal(0x30b14eu); s.xmm[1]=v(s.memory,(0x10u+s.gp[7]));
s.instruction_literal(0x30b152u); s.xmm[1]=_mm_sub_ps(s.xmm[1],s.xmm[0]);
s.instruction_literal(0x30b155u); vw(s.memory,s.observe,0x30b155u,(0x10u+s.gp[7]),s.xmm[1]);
L_native_decreasing_30b159: s.instruction_literal(0x30b159u); if(s.flags.sf==s.flags.of)goto L30b16e;
L_native_decreasing_30b15b: s.instruction_literal(0x30b15bu); s.fp.push(ext(f(r(s.memory,(0x10u+s.gp[7]+s.gp[0]*4u),4))));
s.instruction_literal(0x30b15fu); s.fp.comparison(ext(f(r(s.memory,(0x20u+s.gp[4]),4))),1);
s.instruction_literal(0x30b163u); s.set(0,s.fp.word(),2,0);
s.instruction_literal(0x30b165u); s.flags.logic((s.reg(0,1,8))&(0x41u),1);
s.instruction_literal(0x30b168u); if(s.flags.zf)goto L30b6af;
L30b16e: { using namespace halo2::native_mopp_ray_decreasing_resume_detail; if(native) switch((routes[2]=decreasing_resume<Phase::advance>(s))) { case Continuation::original: break; case Continuation::read_delta: goto L_native_decreasing_30b172; case Continuation::opcode: goto L30ae02; default: throw std::logic_error("Invalid MOPP decreasing resume continuation"); } } s.instruction_literal(0x30b16eu); s.set(0,r(s.memory,(0x24u+s.gp[4]),4),4,0);
L_native_decreasing_30b172: s.instruction_literal(0x30b172u); s.set(0,s.flags.minus(s.reg(0,4,0),r(s.memory,(0x14u+s.gp[4]),4),4),4,0);
s.instruction_literal(0x30b176u); s.set(6,s.flags.plus(s.reg(6,4,0),s.reg(0,4,0),4),4,0);
s.instruction_literal(0x30b178u); goto L30ae02;
L30ae02: return Exit::opcode;
L30b6af: return Exit::empty;
}
Exit model(WorldState &s,Result &r,bool native) { return exact_resume(s,r.routes,native); }

Exit model_increasing(WorldState &s,Result &outcome,bool native,IncreasingPhase phase) {
    auto &route=outcome.routes[0];
    if(native) {
        switch(phase) {
        case IncreasingPhase::before_child:route=stage::increasing_resume<IncreasingPhase::before_child>(s);break;
        case IncreasingPhase::after_child:route=stage::increasing_resume<IncreasingPhase::after_child>(s);break;
        case IncreasingPhase::fraction:route=stage::increasing_resume<IncreasingPhase::fraction>(s);break;
        }
    }
    if(route==Route::advance)return Exit::opcode;
    if(route==Route::shorten)return Exit::empty;
    if(phase==IncreasingPhase::fraction) {
        if(route==Route::original) {
            s.instruction_literal(0x30af7cu);s.gp[1]=r(s.memory,s.gp[4]+0x28u);
        }
        s.instruction_literal(0x30af80u);s.fp.push(ext(f(r(s.memory,s.gp[1]+0x54u))));
        s.instruction_literal(0x30af83u);s.fp.comparison(ext(f(r(s.memory,0x46a0d0u))),1);
        s.instruction_literal(0x30af89u);s.set(0,s.fp.word(),2,0);
        s.instruction_literal(0x30af8bu);s.flags.logic(s.reg(0,1,8)&5u,1);
        s.instruction_literal(0x30af8eu);
    } else {
        const bool before=phase==IncreasingPhase::before_child;
        const auto pcs=before?std::array{0x30aec9u,0x30aecdu,0x30aed1u,0x30aed7u,0x30aed9u,0x30aedcu}:
                             std::array{0x30af2du,0x30af31u,0x30af35u,0x30af3bu,0x30af3du,0x30af40u};
        s.instruction_literal(pcs[0]);s.fp.push(ext(f(r(s.memory,s.gp[4]+(before?0x30u:0x18u)))));
        s.instruction_literal(pcs[1]);s.fp.st(0)=mul(s.fp.st(0),ext(f(r(s.memory,s.gp[4]+(before?0x34u:0x2cu)))));
        s.instruction_literal(pcs[2]);s.fp.comparison(ext(f(r(s.memory,0x46a0e8u))),1);
        s.instruction_literal(pcs[3]);s.set(0,s.fp.word(),2,0);
        s.instruction_literal(pcs[4]);s.flags.logic(s.reg(0,1,8)&5u,1);
        s.instruction_literal(pcs[5]);
    }
    return s.flags.pf?Exit::opcode:Exit::empty;
}

Result run(const Fixture &f,bool native,std::optional<IncreasingPhase> increasing={}) {
    Result r;r.accesses.reserve(32);r.pc=0x30b0ceu;
    Bank bank{f,r};
    SourcePhysicsWorldRuntimeServices services;
    auto trace=f.trace;
    services.instruction=[&](std::uint32_t pc){r.pc=pc;r.instructions.push_back(pc);};
    services.instruction_cursor={&r.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    if(!f.authentic)services.instruction_cursor={};
    SourcePhysicsContactRecordObserver observe;
    auto on_observe=[&](const auto &event) {
        r.observed.push_back(event);r.states.push_back(bank.snapshot());
        if(f.observer_throws)throw std::runtime_error("Injected observer fault");
    };
    if(f.observer)observe=on_observe;
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0x25300000u,2u,1u,f.source,f.frame,0x2020u,0x7800u,f.ray};
    input.eflags=0x256u;input.x87_depth=f.depth;input.x87_status=0x3200u;
    for(unsigned i=0;i<f.depth;++i)input.x87[i]=0.125L*static_cast<long double>(i)+0.25L;
    const auto cw=std::uint16_t(0x7fu|f.precision<<8u|f.rounding<<10u);
    input.floating_point=SourcePhysicsFactoryFloatingPoint{cw,f.mxcsr};
    asm volatile("fnclex; fldcw %0"::"m"(cw));
    _mm_setcsr(f.mxcsr);
    WorldState state{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.snapshot=[&]{return state.save();};
    bank.on_write=[&] {
        if(f.move_frame) {
            const auto frame=state.gp[4]-base;
            std::memcpy(r.data.data()+frame+0x100u,r.data.data()+frame,0x80u);
            state.gp[4]+=0x100u;
            bank.seed(state.gp[4]+(r.pc==0x30b0fdu?0x74u:0x78u),0x3e800000u);
        } else {
            trace=true;
            observe=on_observe;
        }
    };
    try { r.exit=increasing?model_increasing(state,r,native,*increasing):model(state,r,native); }
    catch(const std::runtime_error &e) { r.fault=e.what(); }
    r.abi=state.save();
    r.mxcsr=_mm_getcsr();
    return r;
}
bool same(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_status!=b.x87_status||a.x87_depth!=b.x87_depth||
       a.floating_point->x87_control_word!=b.floating_point->x87_control_word||
       a.floating_point->mxcsr!=b.floating_point->mxcsr)return false;
    for(unsigned i=0;i<a.x87_depth;++i)if(std::memcmp(&a.x87[i],&b.x87[i],10u))return false;
    return true;
}
void compare(const Fixture &f,std::size_t phase,Route route,std::optional<IncreasingPhase> increasing={}) {
    if((increasing?stage::reference_increasing_resume():stage::reference_decreasing_resume())||!f.authentic)
        route=Route::original;
    const auto a=run(f,false,increasing),b=run(f,true,increasing);
    if(a.pc!=b.pc||a.fault!=b.fault||a.data!=b.data||a.accesses!=b.accesses||a.exit!=b.exit||
       a.observed!=b.observed||a.instructions!=b.instructions||a.mxcsr!=b.mxcsr||
       !same(a.abi,b.abi)||a.states.size()!=b.states.size()||(b.fault.empty()&&b.routes[phase]!=route)) {
        std::cerr<<"Decreasing resume PC "<<std::hex<<a.pc<<'/'<<b.pc<<" route "
                 <<int(route)<<'/'<<int(b.routes[phase])<<" fault "<<a.fault<<'/'<<b.fault<<'\n';
        std::cerr<<"inputs "<<f.fraction<<'/'<<f.axis<<'/'<<f.precision<<'/'<<f.rounding<<'/'<<f.fault_pc
                 <<" data "<<(a.data==b.data)<<" accesses "<<a.accesses.size()<<'/'<<b.accesses.size()
                 <<" instructions "<<a.instructions.size()<<'/'<<b.instructions.size()
                 <<" observed "<<a.observed.size()<<'/'<<b.observed.size()<<" states "<<a.states.size()<<'/'<<b.states.size()
                 <<" eflags "<<a.abi.eflags<<'/'<<b.abi.eflags<<" x87 "<<a.abi.x87_status<<'/'<<b.abi.x87_status
                 <<" mxcsr "<<a.mxcsr<<'/'<<b.mxcsr<<" gp "<<(a.abi.gp==b.abi.gp)<<" xmm "<<(a.abi.xmm==b.abi.xmm)<<'\n';
        for(unsigned i=0;i<a.abi.xmm.size();++i)if(a.abi.xmm[i]!=b.abi.xmm[i]) {
            std::cerr<<"xmm "<<i<<':';
            for(unsigned lane=0;lane<4u;++lane)std::cerr<<' '<<a.abi.xmm[i][lane]<<'/'<<b.abi.xmm[i][lane];
            std::cerr<<'\n';
        }
        throw std::runtime_error("MOPP decreasing resume continuation, provider or ABI differs");
    }
    for(unsigned i=0;i<a.states.size();++i)
        if(!same(a.states[i],b.states[i]))throw std::runtime_error("Provider saw a different live MOPP frame");
}
unsigned prove_increasing() {
    unsigned count{};
    for(const auto phase:{IncreasingPhase::before_child,IncreasingPhase::after_child,IncreasingPhase::fraction})
        for(unsigned rounding=0;rounding<4u;++rounding) {
            Fixture f;f.rounding=rounding;
            for(const auto bits:{0u,0x80000000u,0xbf000000u,0x3f800000u,0x3f800001u,
                                 0x7f800000u,0x7fc12021u,0x7f814389u,0x00000001u}) {
                f.left=f.fraction=bits;
                const auto expected=run(f,false,phase).exit==Exit::opcode?Route::advance:Route::shorten;
                compare(f,0,expected,phase);++count;
                auto sparse=f;sparse.constants=0;sparse.borrowed_constants=1;
                compare(sparse,0,expected,phase);++count;
            }
            f.trace=true;compare(f,0,Route::original,phase);++count;
            f.trace=false;f.constants=0;
            const auto refused=phase==IncreasingPhase::fraction?Route::read_fraction:Route::original;
            compare(f,0,refused,phase);++count;
            f.fault_pc=phase==IncreasingPhase::fraction?0x30af83u:
                phase==IncreasingPhase::before_child?0x30aed1u:0x30af35u;
            compare(f,0,refused,phase);++count;
        }
    return count;
}
unsigned prove() {
    unsigned compared{};
    auto check=[&](const Fixture &f,std::size_t phase,Route route) {compare(f,phase,route);++compared;};
    for(const auto p:{0u,2u,3u})for(unsigned rc=0;rc<4u;++rc) {
        Fixture f;f.precision=p;f.rounding=rc;
        check(f,1,Route::empty);
        for(const auto bits:{0u,0x80000000u,0xbf000000u,0x3f800000u,0x3f800001u,
                             0x7f800000u,0x7fc12021u,0x7f814389u,0x00000001u}) {
            auto vary=f;vary.fraction=bits;vary.axis=999u;vary.vector_variant=rc;
            check(vary,2,Route::opcode);
        }
        for(const auto pc:{0x30b0fdu,0x30b12cu}) {
            auto refused=f;refused.deny_scalar=pc;
            const auto before=pc==0x30b0fdu?Route::save_fraction:Route::save_scale;
            const auto after=pc==0x30b0fdu?Route::refill_fraction:Route::refill_scale;
            const bool rollback=stage::reference_decreasing_provider();
            check(refused,1,rollback?before:Route::empty);
            refused.reenter_pc=pc;
            check(refused,1,rollback?before:after);
            refused.move_frame=true;
            const auto moved=run(refused,false).exit==Exit::empty?Route::empty:Route::advance;
            check(refused,1,rollback?before:moved);
            refused.move_frame=false;
            refused.fault_pc=pc;refused.fault_half=1;
            check(refused,1,Route::original);
        }
        for(const auto pc:{0x30b121u,0x30b147u,0x30b155u}) {
            for(const auto half:{1u,2u}) {
                auto fault=f;fault.fault_pc=pc;fault.fault_half=half;
                check(fault,1,Route::original);
            }
            auto dynamic=f;dynamic.reenter_pc=pc;
            check(dynamic,1,pc==0x30b121u?Route::read_scale:
                pc==0x30b147u?Route::read_translation:Route::branch_axis);
            dynamic.observer_throws=true;
            check(dynamic,1,Route::original);
        }
        auto observed=f;observed.observer=true;
        check(observed,1,Route::original);
        auto traced=f;traced.trace=true;
        check(traced,0,Route::original);
        auto unauthenticated=f;unauthenticated.authentic=false;
        check(unauthenticated,0,Route::original);
        auto sparse=f;sparse.constants=0u;sparse.borrowed_constants=3u;
        check(sparse,1,Route::empty);
        auto long_ray=sparse;long_ray.fraction=0x3f800000u;
        check(long_ray,0,Route::advance);
        auto scalar_only=sparse;scalar_only.borrowed_constants=1u;
        check(scalar_only,1,Route::read_one);
        scalar_only.fault_pc=0x30b0f0u;check(scalar_only,1,Route::read_one);
        auto vector_only=sparse;vector_only.borrowed_constants=2u;
        check(vector_only,0,Route::read_fraction);
        vector_only.fault_pc=0x30b0d8u;check(vector_only,0,Route::read_fraction);
        auto no_unit=f;no_unit.constants=2u;
        check(no_unit,0,Route::read_fraction);
        no_unit.fault_pc=0x30b0d8u;
        check(no_unit,0,Route::original);
        auto no_ones=f;no_ones.constants=1u;
        check(no_ones,1,Route::read_one);
        auto crossed=f;crossed.ray=base+0xfe8u;
        check(crossed,1,Route::read_short_end);
        auto code_crosses=f;code_crosses.program=base+0xff8u;
        check(code_crosses,1,Route::read_code);
        auto source_crosses=f;source_crosses.source=base+0xff8u;
        check(source_crosses,1,Route::read_translation);
        auto alias=f;alias.ray=stack+0x74u-0x10u;alias.axis=999u;
        check(alias,2,Route::opcode);
        alias=f;alias.ray=origin+4u;alias.axis=999u;
        check(alias,2,Route::opcode);
        auto full=f;full.depth=8;
        check(full,0,Route::original);
    }
    return compared;
}
}

int main() try {
    std::cout<<"MOPP ray resume: "<<prove()+prove_increasing()
             <<" exact continuation and provider comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}
