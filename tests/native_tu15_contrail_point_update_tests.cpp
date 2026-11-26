#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_contrail_point_update.hpp"
#include "contrail_point_accounting.inc"

#include <algorithm>
#include <limits>

using namespace frame_misc_harness;

namespace fixture {
struct PointFrame : physics_world_runtime_detail::WorldState {
    using physics_world_runtime_detail::WorldState::WorldState;
    struct { const InstructionCursor *cursor{}; } immutable_cursor;
#include "contrail_point_fpu.inc"
};
using WorldState = PointFrame;

constexpr std::uint32_t stack=0x60004000u, contrail_array=0x61000000u, contrail_rows=0x61100000u;
constexpr std::uint32_t point_array=0x62000000u, point_rows=0x62100000u, tag_rows=0x63000000u;
constexpr std::uint32_t definition=0x63100000u, segments=0x63200040u, effect_tag=0x63300000u;
constexpr std::uint32_t none=0xffffffffu;

using Abi=SourcePhysicsWorldRuntimeAbi;

bool identical(const Abi &a,const Abi &b) {
    return same_abi(a,b) && a.fs_base==b.fs_base && a.floating_point && b.floating_point &&
        a.floating_point->mxcsr==b.floating_point->mxcsr && !a.active_mmx && !b.active_mmx;
}

struct Packet {
    char kind;
    std::uint32_t pc,address;
    std::size_t width;
    Abi abi;

    bool operator==(const Packet &o) const {
        return kind==o.kind && pc==o.pc && address==o.address && width==o.width && identical(abi,o.abi);
    }
};

struct Provider : Memory {
    using Snapshot=std::function<Packet(char,std::uint32_t,std::size_t)>;
    Snapshot snapshot;
    mutable std::vector<Packet> packets;
    std::vector<std::vector<std::byte>> writes;
    unsigned stop;
    bool change_fp;

    Provider(const Memory &source,unsigned fail_at,bool vary_fp)
        : Memory(source.clone()),stop(fail_at),change_fp(vary_fp) {}

    void visit(char kind,std::uint32_t address,std::size_t width) const {
        packets.push_back(snapshot(kind,address,width));
        const auto step=packets.size();
        if (change_fp) {
            const auto cw=std::uint16_t(0x037fu | ((step&3u)<<10u));
            asm volatile("fldcw %0"::"m"(cw):"memory");
            source_load_x87_exception_flags(std::uint16_t(step&0x3fu));
            _mm_setcsr(0x1f80u | (unsigned(step&3u)<<13u) | unsigned(step&0x3fu));
        }
        if (step==stop || step>2048u) throw std::runtime_error("Injected contrail provider fault");
    }

    void read(std::uint32_t a,std::span<std::byte> out) const override {
        visit('r',a,out.size());
        Memory::read(a,out);
    }

    void write(std::uint32_t a,std::span<const std::byte> in) override {
        writes.emplace_back(in.begin(),in.end());
        visit('w',a,in.size());
        Memory::write(a,in);
    }
};

void dispatch(PointFrame &s,std::uint32_t target,std::uint32_t pc,bool tail) {
    auto &memory=static_cast<const Provider&>(s.memory);
    memory.visit('c',target,0);
    if (tail || memory.packets.back().pc!=pc) throw std::runtime_error("Unexpected contrail call boundary");
    s.pop();
    switch (target) {
    case 0x32a660u:
        s.gp[4]-=s.gp[0];
        return;
    case 0x17ed80u: {
        constexpr long double rates[]{2.01L,3.19L,0.0L,-0.0L,0.0L,0x1p-146L,18.1L,0.0L};
        const auto curve=s.pop();
        const auto endpoint=s.pop();
        const auto rate=curve%8u;
        if (endpoint!=0 && endpoint!=2) throw std::runtime_error("Unexpected curve endpoint");
        if (rate==4u) s.fp.push(std::numeric_limits<long double>::quiet_NaN());
        else if (rate==7u) s.fp.push(std::numeric_limits<long double>::infinity());
        else s.fp.push(rates[rate]);
        s.gp[0]=0x17ed80u ^ curve;
        s.gp[2]=curve;
        return;
    }
    case 0x16e8a0u:
        s.gp[1]=s.gp[0];
        return;
    default:
        throw std::runtime_error("Unexpected contrail service");
    }
}

void run_219330(PointFrame &s) {
    static_cast<const Provider&>(s.memory).visit('c',0x219330u,0);
    s.pop();
    s.gp[4]+=40u;
    s.gp[0]=0xbc190845u;
    s.gp[3]=0x9d82e571u;
}

#include "contrail_point_owners.inc"

enum class Policy { sealed, unsealed, trace, second_trace, unqualified_required, unsafe_observer };
struct Observation {
    std::vector<std::uint32_t> callbacks;
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::string fault;
    Abi abi;
    std::uint32_t cursor{};
};

Observation run(Provider &memory,const Case &input,Policy policy,bool raw,bool observe) {
    Observation result;
    const bool trace=policy==Policy::trace, discovery=policy==Policy::second_trace;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc) { result.cursor=pc; result.callbacks.push_back(pc); };
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction_cursor={&result.cursor,&trace,&discovery,sites,std::this_thread::get_id(),
        &services.instruction.target_type(),true,policy!=Policy::unsafe_observer};
    if (policy==Policy::unqualified_required) services.required_owner=[](auto&&...) {};
    SourcePhysicsContactRecordObserver observer;
    if (observe) observer=[&](const auto &event) {
        result.stores.push_back(event);
        memory.visit('o',event.address,event.written.size());
    };
    FpScope scope(*input.abi.floating_point);
    PointFrame state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(input.abi);
    if (policy!=Policy::unsealed) state.immutable_cursor.cursor=state.instruction_cursor;
    memory.snapshot=[&](auto kind,auto address,auto width) { return Packet{kind,result.cursor,address,width,state.save()}; };
    try {
        if (raw) point_reference(state);
        else run_17f3d0(state);
    } catch (const std::exception &error) {
        result.fault=error.what();
    }
    result.abi=state.save();
    memory.snapshot={};
    return result;
}

Case make_case(std::mt19937 &random,unsigned n) {
    Case test;
    random_abi(random,test,stack);
    test.abi.fs_base=stack+n;
    auto &m=test.memory;
    for (unsigned offset=0;offset<0x1030;offset+=4) m.put(stack-offset,std::uint32_t(random()));
    m.put(stack,0x017ecccu);
    m.put(stack+4u,0x02340001u);
    constexpr std::uint32_t ticks[]{0x3e800000u,0x3f800000u,0x80000000u,0x00000001u,0x7fc12345u,0xff834567u};
    m.put(stack+8u,ticks[n%6u]);
    m.put(0x4f7214u,contrail_array);
    m.put(contrail_array+0x44u,contrail_rows);
    m.put(0x4f7210u,point_array);
    m.put(point_array+0x44u,point_rows);
    m.put(0x4f0414u,tag_rows);
    m.put(0x46a0e8u,0);
    m.put(0x46a0d0u,0x3f800000u);
    m.put(0x46a0ccu,0x40100000u);
    m.put(tag_rows+0x18u,definition);
    m.put(tag_rows+0x28u,effect_tag);
    const auto datum=contrail_rows+0x48u;
    m.put(datum+4u,0x74410001u);
    m.put(definition+0xe8u,n%4u==0u ? 1u : 3u);
    m.put(definition+0xecu,segments);
    for (unsigned record=0;record<3;++record) {
        const auto at=segments+record*0x40u;
        m.put(at,0x3f300000u); m.put(at+4u,n+record);
        m.put(at+8u,0x3f500000u); m.put(at+0xcu,n+record);
        m.put(at+0x14u,record==1u && n%2u ? none : 0x4f110002u);
        m.put(at+0x18u,0x3dccccccu);
        m.put(at+0x3cu,0x11000000u+record);
    }

    constexpr unsigned lengths[]{0,1,2,5};
    unsigned next=1;
    for (unsigned chain=0;chain<4;++chain) {
        const auto count=lengths[(n+chain)%4u];
        m.put(datum+0x30u+chain*2u,count,2);
        m.put(datum+0x38u+chain*4u,count ? (0x4ab30000u|next) : none);
        for (unsigned index=0;index<count;++index,++next) {
            const auto point=point_rows+next*0x38u;
            const auto flags=count==5u ? (index==0u ? 1u : index==4u ? 2u : 0u) : (n%5u ? 4u : 0u);
            m.put(point+2u,flags,1);
            m.put(point+3u,index==4u ? 1u : 0u,1);
            m.put(point+4u,index<2u ? 0x3e800000u : 0x3fc00000u);
            m.put(point+8u,index==3u ? 0u : index==2u && n%8u==0u ? 0x7fc45123u : 0x3f800000u);
            m.put(point+0xcu,0x3f980000u);
            m.put(point+0x34u,index+1u<count ? (0x4ab30000u|(next+1u)) : none);
        }
    }
    return test;
}

struct Stats {
    unsigned cases{},native{},retained{},faults{},packets{},stores{};
    std::map<std::uint32_t,unsigned> children;
};

unsigned compare(const Case &input,unsigned id,Stats &stats,bool forced,
                 Policy policy=Policy::sealed,bool observe=true,unsigned cut=0) {
    const bool native=policy==Policy::sealed && !forced;
    Provider left(input.memory,cut,id%3u==0u),right(input.memory,cut,id%3u==0u);
    const auto expected=run(left,input,policy,true,observe),actual=run(right,input,policy,false,observe);
    const auto mismatch=std::mismatch(left.packets.begin(),left.packets.end(),right.packets.begin(),right.packets.end());
    if (mismatch.first!=left.packets.end() || mismatch.second!=right.packets.end() || !left.same(right) ||
            left.writes!=right.writes || expected.stores!=actual.stores ||
            expected.cursor!=actual.cursor || expected.fault!=actual.fault || !identical(expected.abi,actual.abi) ||
            (native ? !actual.callbacks.empty() : expected.callbacks!=actual.callbacks)) {
        std::fprintf(stderr,"17F3D0 case %u cut %u packets %zu/%zu mismatch %zu cursor %x/%x faults [%s]/[%s]\n",
            id,cut,left.packets.size(),right.packets.size(),std::size_t(mismatch.first-left.packets.begin()),
            expected.cursor,actual.cursor,expected.fault.c_str(),actual.fault.c_str());
        if (mismatch.first!=left.packets.end() && mismatch.second!=right.packets.end())
            std::fprintf(stderr,"first packet %c:%x:%x / %c:%x:%x\n",mismatch.first->kind,mismatch.first->pc,
                mismatch.first->address,mismatch.second->kind,mismatch.second->pc,mismatch.second->address);
        std::exit(1);
    }
    if (actual.fault.empty()!=(!cut)) {
        std::fprintf(stderr,"Unexpected fixture stop in case %u at %u [%s]\n",id,cut,actual.fault.c_str());
        std::exit(1);
    }
    ++stats.cases;
    native ? ++stats.native : ++stats.retained;
    stats.faults+=!actual.fault.empty();
    stats.packets+=unsigned(left.packets.size());
    stats.stores+=unsigned(actual.stores.size());
    for (const auto &packet : left.packets)
        if (packet.kind=='c') ++stats.children[packet.pc];
    return unsigned(left.packets.size());
}
} // namespace fixture

int main(int argc,char **argv) {
    if (argc!=3) return 2;
    const auto cases=unsigned(std::stoul(argv[1]));
    const bool forced=std::string_view(argv[2])=="reference";
    std::mt19937 random(0x17f3d0u);
    fixture::Stats stats;
    for (unsigned n=0;n<cases;++n) {
        const auto input=fixture::make_case(random,n);
        const auto packets=fixture::compare(input,n,stats,forced,fixture::Policy::sealed,n%3u!=1u);
        if (n<2u) for (unsigned cut=1;cut<=packets;++cut)
            fixture::compare(input,n,stats,forced,fixture::Policy::sealed,n%3u!=1u,cut);
    }
    for (const auto policy : {fixture::Policy::unsealed,fixture::Policy::trace,fixture::Policy::second_trace,
                             fixture::Policy::unqualified_required,fixture::Policy::unsafe_observer})
        fixture::compare(fixture::make_case(random,9),9,stats,forced,policy);
    constexpr std::uint32_t calls[]{0x17f3d5u,0x17f4ebu,0x17f557u,0x17f600u,0x17f682u,0x17f6b4u};
    for (auto pc : calls) if (!stats.children.contains(pc)) {
        std::fprintf(stderr,"Missing original child call edge %06x\n",pc);
        return 1;
    }
    std::printf("{\"cases\":%u,\"native\":%u,\"retained\":%u,\"faults\":%u,\"packets\":%u,\"stores\":%u,\"call_edges\":{",
        stats.cases,stats.native,stats.retained,stats.faults,stats.packets,stats.stores);
    bool first=true;
    for (const auto &[pc, count] : stats.children) {
        std::printf("%s\"%06x\":%u",first?"":",",pc,count);
        first=false;
    }
    std::puts("}}");
}
