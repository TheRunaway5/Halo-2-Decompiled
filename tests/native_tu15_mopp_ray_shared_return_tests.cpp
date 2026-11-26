#include "physics/runtime/native_tu15_mopp_ray_opcode_resume.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace stage=native_mopp_ray_shared_return_detail;
namespace opcode=native_mopp_ray_opcode_resume_detail;
constexpr std::uint32_t base=0x62000000u,stack=base+0x180u,frame=base+0x580u,program=base+0x1200u;
constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
enum class Kind { opcode, rejected, local_exit, leaf_exit, alternate_exit };

struct Fixture {
    Kind kind=Kind::opcode;
    std::uint32_t sp=stack, fp=frame, instruction=0x45u, fault_pc{};
    unsigned depth=4;
    bool trace{}, authentic=true, device_code{}, deny_store{}, observer{}, observer_fault{};
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
    int route{};
    bool classify{};
    std::string fault;
};
struct Bank final : SourcePhysicsContactMemory {
    const Fixture &f;
    Result &r;
    SourceDirectPageIndex index;
    std::function<SourcePhysicsWorldRuntimeAbi()> snapshot;

    template<class T> void put(std::uint32_t address,const T &value) const {
        std::memcpy(r.data.data()+address-base,&value,sizeof(value));
    }
    Bank(const Fixture &fixture,Result &result):f(fixture),r(result) {
        direct_pages_=&index;
        auto &block=index.blocks[base>>22u];
        block=std::make_unique<SourceDirectPageIndex::Block>();
        (*block)[(base>>12u)&1023u]={r.data.data(),true};
        if(!f.device_code)(*block)[((base>>12u)&1023u)+1u]={r.data.data()+4096u,true};
        put(f.sp,std::array{0x3f800001u,0xab91e674u,0x65480021u});
        put(f.fp,std::array{0x61414243u,0x00207410u});
        put(program,std::byte(f.instruction));
    }
    void access(std::uint32_t address,std::size_t width,bool write)const {
        r.accesses.push_back({r.pc,address,width,write});r.states.push_back(snapshot());
        if(r.pc==f.fault_pc)throw std::runtime_error("Injected MOPP continuation provider fault");
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        access(address,out.size(),false);
        std::memcpy(out.data(),r.data.data()+address-base,out.size());
    }
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> in)noexcept override {
        if(f.deny_store||f.observer||!direct_page(address,in.size()))return false;
        r.accesses.push_back({r.pc,address,in.size(),true});
        std::memcpy(r.data.data()+address-base,in.data(),in.size());
        return true;
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        if(try_callback_free_write(address,in))return;
        access(address,in.size(),true);
        std::memcpy(r.data.data()+address-base,in.data(),in.size());
    }
};

bool exact_opcode(WorldState &s,opcode::Continuation route) {
    using opcode::Continuation;
    switch(route) {
    case Continuation::classify:return true;
    case Continuation::diagnostic:return false;
    case Continuation::store_default:goto L_store;
    case Continuation::read_opcode:goto L_read;
    case Continuation::original:break;
    }
s.instruction_literal(0x30ae00u); s.fp.st(0)=s.fp.st(0);s.fp.status&=~0x200u;s.fp.pop();
L_read: s.instruction_literal(0x30ae02u); s.set(0,r(s.memory,(0x0u+s.gp[6]),1),4,0);
s.instruction_literal(0x30ae05u); s.flags.minus(s.reg(0,4,0),0x6bu,4);
L_store: s.instruction_literal(0x30ae08u); w(s.memory,s.observe,0x30ae08u,(0x3cu+s.gp[4]),4,0x3e7u);
s.instruction_literal(0x30ae10u); return s.flags.cf||s.flags.zf;
}
void exact_x87_rejection(WorldState &s) {
s.instruction_literal(0x30b6abu); s.fp.st(0)=s.fp.st(0);s.fp.status&=~0x200u;s.fp.pop();
s.instruction_literal(0x30b6adu); s.fp.st(0)=s.fp.st(0);s.fp.status&=~0x200u;s.fp.pop();
}
void exact_local_exit(WorldState &s) {
s.instruction_literal(0x30b6afu); s.set(7,s.pop(),4,0);
s.instruction_literal(0x30b6b0u); s.set(6,s.pop(),4,0);
s.instruction_literal(0x30b6b1u); s.set(3,s.pop(),4,0);
s.instruction_literal(0x30b6b2u); s.set(4,s.reg(5,4,0),4,0);
s.instruction_literal(0x30b6b4u); s.set(5,s.pop(),4,0);
s.instruction_literal(0x30b6b5u); s.pop();s.gp[4]+=12u;return;
}
void exact_leaf_exit(WorldState &s) {
s.instruction_literal(0x30b6a2u); s.set(7,s.pop(),4,0);
s.instruction_literal(0x30b6a3u); s.set(6,s.pop(),4,0);
s.instruction_literal(0x30b6a4u); s.set(3,s.pop(),4,0);
s.instruction_literal(0x30b6a5u); s.set(4,s.reg(5,4,0),4,0);
s.instruction_literal(0x30b6a7u); s.set(5,s.pop(),4,0);
s.instruction_literal(0x30b6a8u); s.pop();s.gp[4]+=12u;return;
}
void exact_alternate_exit(WorldState &s) {
s.instruction_literal(0x30b668u); s.set(7,s.pop(),4,0);
s.instruction_literal(0x30b669u); s.set(6,s.pop(),4,0);
s.instruction_literal(0x30b66au); s.set(3,s.pop(),4,0);
s.instruction_literal(0x30b66bu); s.set(4,s.reg(5,4,0),4,0);
s.instruction_literal(0x30b66du); s.set(5,s.pop(),4,0);
s.instruction_literal(0x30b66eu); s.pop();s.gp[4]+=12u;return;
}


Result run(const Fixture &f,bool native) {
    Result r;r.pc=0x30aaa7u;r.accesses.reserve(24);Bank bank{f,r};
    SourcePhysicsWorldRuntimeServices services;
    auto trace=f.trace;
    services.instruction=[&](std::uint32_t pc){r.pc=pc;r.instructions.push_back(pc);};
    services.instruction_cursor={&r.pc,&trace,nullptr,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    if(!f.authentic)services.instruction_cursor={};
    SourcePhysicsContactRecordObserver observe;
    if(f.observer)observe=[&](const auto &event) {
        r.observed.push_back(event);r.states.push_back(bank.snapshot());
        if(f.observer_fault)throw std::runtime_error("Injected MOPP continuation observer fault");
    };
    SourcePhysicsWorldRuntimeAbi input{};
    input.gp={0xf2410000u,2u,1u,0x7100u,f.sp,f.fp,program,0x7654u};
    input.eflags=0x256u;input.x87_depth=f.depth;input.x87_status=0x3204u;
    for(unsigned i=0;i<f.depth;++i)input.x87[i]=0.125L*static_cast<long double>(i)+0.25L;
    WorldState state{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    bank.snapshot=[&]{return state.save();};
    try {
        switch(f.kind) {
        case Kind::opcode: {
            const auto route=native?opcode::resume<true>(state):opcode::Continuation::original;
            r.route=int(route);r.classify=exact_opcode(state,route);break;
        }
        case Kind::rejected: {
            const auto route=native?stage::rejection(state):stage::Rejection::original;
            r.route=int(route);
            if(route==stage::Rejection::original)exact_x87_rejection(state);
            if(route!=stage::Rejection::complete)exact_local_exit(state);
            break;
        }
        case Kind::local_exit:
            r.route=native&&stage::frame<0x30b6b5u>(state);
            if(!r.route)exact_local_exit(state);
            break;
        case Kind::leaf_exit:
            r.route=native&&stage::frame<0x30b6a8u>(state);
            if(!r.route)exact_leaf_exit(state);
            break;
        case Kind::alternate_exit:
            r.route=native&&stage::frame<0x30b66eu>(state);
            if(!r.route)exact_alternate_exit(state);
            break;
        }
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
void compare(const Fixture &f,int expected) {
    if(stage::reference_shared_return())expected=0;
    const auto a=run(f,false),b=run(f,true);
    if(a.pc!=b.pc||a.fault!=b.fault||a.data!=b.data||a.accesses!=b.accesses||a.classify!=b.classify||
       a.observed!=b.observed||a.instructions!=b.instructions||!same(a.abi,b.abi)||
       a.states.size()!=b.states.size()||b.route!=expected) {
        std::cerr<<"MOPP continuation "<<int(f.kind)<<" PC "<<std::hex<<a.pc<<'/'<<b.pc
                 <<" route "<<expected<<'/'<<b.route<<" fault "<<a.fault<<'/'<<b.fault<<'\n';
        throw std::runtime_error("MOPP shared return or opcode continuation differs");
    }
    for(unsigned i=0;i<a.states.size();++i)
        if(!same(a.states[i],b.states[i]))throw std::runtime_error("Different provider-visible MOPP return frame");
}

unsigned prove() {
    unsigned compared{};
    auto check=[&](const Fixture &f,auto expected){compare(f,int(expected));++compared;};
    for(unsigned depth=0;depth<=8;++depth) {
        Fixture f;f.depth=depth;
        using Route=opcode::Continuation;
        for(const auto byte:{0u,0x45u,0x6bu,0x6cu,255u}) {
            f.instruction=byte;
            check(f,depth?(byte<=0x6bu?Route::classify:Route::diagnostic):Route::original);
        }
        auto provider=f;provider.device_code=true;
        check(provider,depth?Route::read_opcode:Route::original);
        provider.fault_pc=0x30ae02u;check(provider,depth?Route::read_opcode:Route::original);
        auto refused=f;refused.deny_store=true;
        check(refused,depth?Route::store_default:Route::original);
        refused.fault_pc=0x30ae08u;check(refused,depth?Route::store_default:Route::original);
        auto traced=f;traced.trace=true;check(traced,Route::original);
        auto weak=f;weak.authentic=false;check(weak,Route::original);
        auto observer=f;observer.observer=true;observer.observer_fault=true;check(observer,Route::original);

        using Rejection=stage::Rejection;
        for(const auto kind:{Kind::rejected,Kind::local_exit,Kind::leaf_exit,Kind::alternate_exit}) {
            f.kind=kind;
            const bool rejected=kind==Kind::rejected;
            check(f,rejected?(depth>=2u?int(Rejection::complete):0):1);
            auto split=f;split.sp=base+0xffeu;
            check(split,rejected&&depth>=2u?int(Rejection::frame):0);
            split=f;split.fp=base+0xffeu;
            check(split,rejected&&depth>=2u?int(Rejection::frame):0);
            split.fault_pc=kind==Kind::leaf_exit?0x30b6a7u:kind==Kind::alternate_exit?0x30b66du:0x30b6b4u;
            check(split,rejected&&depth>=2u?int(Rejection::frame):0);
            traced=f;traced.trace=true;check(traced,0);
            observer=f;observer.observer=true;
            check(observer,rejected?(depth>=2u?int(Rejection::complete):0):1);
        }
    }
    return compared;
}
}
int main() try {
    std::cout<<"MOPP shared returns: "<<prove()<<" exact continuation and fault comparisons\n";
    return 0;
} catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    return 1;
}
