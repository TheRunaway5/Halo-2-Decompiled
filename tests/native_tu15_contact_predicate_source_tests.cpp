#include "physics/runtime/native_tu15_contact_predicate_source_frame.hpp"
#if defined(HALO2_CONTACT_PREDICATE_BOUND_TEST)
#include "physics/runtime/native_tu15_contact_predicate_source_owner.hpp"
#endif
#include <algorithm>
#include <bit>
#include <iostream>
#include <map>
#include <set>
#include <string_view>

namespace {
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using namespace halo2::native_contact_predicate_detail;
#include "contact_predicate_source_reference.inc"

constexpr std::uint32_t sp=0x61000120u, first=0x61002000u, second=0x61003000u,
    settings=0x61004000u, output=0x61005000u;
constexpr std::uint32_t bits(float x) {return std::bit_cast<std::uint32_t>(x);}
struct Stop {unsigned event;};
struct Packet {
    std::array<std::uint32_t,8> gp;
    std::array<std::array<std::uint32_t,4>,8> xmm;
    std::array<std::array<std::byte,10>,8> x87, mmx;
    std::uint32_t flags, fs_base, cursor, mxcsr;
    std::uint16_t cw, status;
    unsigned depth, mmx_mask;
    bool has_mmx;
    friend bool operator==(const Packet&,const Packet&)=default;
};
struct Event {
    char kind;
    std::uint32_t address, pc;
    std::vector<std::byte> before, data;
    Packet abi;
    friend bool operator==(const Event&,const Event&)=default;
};
struct Memory final : SourcePhysicsContactMemory {
    mutable std::vector<Event> events;
    mutable std::map<std::uint32_t,std::byte> bytes;
    std::function<Packet()> packet;
    std::function<void(unsigned,char,std::uint32_t)> feedback;
    unsigned stop=~0u;
    std::byte get(std::uint32_t p)const {
        const auto found=bytes.find(p);
        return found==bytes.end()?std::byte{}:found->second;
    }
    void put(std::uint32_t p,std::uint32_t x) {
        for(unsigned n=0;n<4;++n)bytes[p+n]=std::byte(x>>(n*8u));
    }
    unsigned event(char kind,std::uint32_t address,std::uint32_t pc=0,
        std::span<const std::byte> before={},std::span<const std::byte> data={})const {
        const auto ordinal=unsigned(events.size());
        events.push_back({kind,address,pc,{before.begin(),before.end()},{data.begin(),data.end()},packet()});
        if(ordinal==stop)throw Stop{ordinal};
        return ordinal;
    }
    void acquire(char kind,std::uint32_t p,std::span<std::byte> out)const {
        for(unsigned n=0;n<out.size();++n)out[n]=get(p+n);
        const auto ordinal=event(kind,p,0,{},out);
        if(feedback)feedback(ordinal,kind,p);
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {acquire('r',p,out);}
    void read_vector(std::uint32_t p,std::span<std::byte,16> out)const override {acquire('v',p,out);}
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        const auto ordinal=event('w',p,0,{},in);
        for(unsigned n=0;n<in.size();++n)bytes[p+n]=in[n];
        if(feedback)feedback(ordinal,'w',p);
    }
};
enum class Policy {
    normal, visible, install, trace, clear, rebind, observer_install,
    observer_clear, registers, logical_stack, host, scratch, foreign,
    entry_unknown, entry_clear, entry_rebind, entry_thread, entry_sites,
    entry_format, entry_position, entry_mmx, entry_full, entry_status
};
struct Case {
    unsigned seed, event=~0u, stop=~0u;
    bool quiet, observer;
    Policy policy=Policy::normal;
};
struct Result {
    Packet abi;
    std::vector<Event> events;
    std::map<std::uint32_t,std::byte> bank;
    unsigned stop, route;
};
Packet capture(const WorldState& s,std::uint32_t cursor) {
    const auto abi=s.save();
    Packet packet{abi.gp,abi.xmm,{},{},abi.eflags,abi.fs_base,cursor,
        abi.floating_point->mxcsr,abi.floating_point->x87_control_word,
        abi.x87_status,abi.x87_depth,0,abi.active_mmx.has_value()};
    for(unsigned n=0;n<abi.x87_depth;++n)std::memcpy(packet.x87[n].data(),&abi.x87[n],10);
    if(abi.active_mmx){packet.mmx=abi.active_mmx->physical;packet.mmx_mask=abi.active_mmx->known_mask;}
    return packet;
}

template<SourceFormat Format,class Run> Result execute(Case row,Run run,bool candidate) {
    physics_step_detail::FpScope environment{{0x37fu,0x1f80u}};
    Memory m;m.stop=row.stop;
    const auto a=first+((row.seed%13u)==9u?4u:0u);
    const auto b=second;
    const auto output_address=row.seed%13u==7u?sp:output;
    const auto settings_address=row.seed%13u==8u?sp-20u:settings;
    m.put(sp,0x63000000u);m.put(sp+4u,output_address);m.put(sp+8u,a);
    m.put(sp+12u,b);m.put(sp+16u,settings_address);
    m.put(0x41d018u,0xbdcccccdu);
    m.put(b+12u,bits(row.seed%13u==0u?-1.f:0.f));
    m.put(a+16u,bits(1.f));m.put(b+20u,bits(1.f));
    m.put(settings_address+16u,bits(0.5f));
    m.put(b,bits(row.seed%13u==2u?-2.f:2.f));
    if(row.seed%13u==1u)m.put(b+16u,bits(1.f));
    if(row.seed%13u==3u){m.put(a+16u,0x7fc12345u);m.put(b+16u,0xffc54321u);}
    if(row.seed%13u==4u){m.put(a,0x7f812345u);m.put(b,0xffc54321u);}
    if(row.seed%13u==5u){m.put(a+28u,0x7fc12345u);m.put(b+28u,0xffc54321u);}
    if(row.seed%13u==6u){m.put(a+16u,bits(1.f));m.put(a+20u,bits(1.f));m.put(a+24u,bits(1.f));
        m.put(b+16u,bits(33554432.f));m.put(b+20u,bits(-33554432.f));m.put(b+24u,bits(1.f));}
    if(row.seed%13u==11u){m.put(a+12u,0x7fc01234u);m.put(b+12u,0xffc01239u);m.put(a+28u,0xffc04567u);}

    SourcePhysicsWorldRuntimeServices services;
    SourcePhysicsContactRecordObserver observer;
    std::uint32_t cursor=0x51420000u;bool trace=row.policy==Policy::visible;
    const auto callback=[&](std::uint32_t pc){m.event('c',pc);cursor=pc;};
    const auto rebound=[&](std::uint32_t pc){m.event('b',pc);cursor=pc;};
    const auto observe=[&](const auto& e){
        const auto ordinal=m.event('o',e.address,e.instruction,e.before,e.written);
        if(m.feedback)m.feedback(ordinal,'o',e.address);
    };
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    constexpr std::array changed_sites{0x303800u,0x21f590u,0x33b595u};
    const bool entry_callback=row.policy>=Policy::entry_unknown&&row.policy<=Policy::entry_position;
    if(row.quiet||trace||entry_callback) {
        services.instruction=callback;
        if(row.policy!=Policy::entry_unknown)
            services.instruction_cursor={&cursor,&trace,nullptr,sites,std::this_thread::get_id(),
                &services.instruction.target_type(),true,true};
    }
    if(row.observer)observer=observe;
    WorldState state{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    SourcePhysicsContactRuntimeAbi abi{};
    abi.gp={0xa1111134u,2u,3u,4u,sp,0x6100b100u,0x6100b200u,7u};
    abi.eflags=0x202u|((row.seed&1)?0x8d5u:0x400u);abi.fs_base=0x2a000000u;
    abi.x87_depth=row.policy==Policy::entry_full?8u:row.seed%8u;
    abi.x87_status=((row.seed%8u)<<11u)|((row.seed&1u)?0x0200u:0u)|((row.seed&2u)?0x4100u:0u);
    for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n+0.125L;
    for(unsigned n=0;n<8;++n)for(unsigned lane=0;lane<4;++lane)
        abi.xmm[n][lane]=0x3e300000u+row.seed*31u+n*7u+lane;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{
        std::uint16_t(0x37fu|((row.seed%4u)<<10u)),
        0x1f80u|((row.seed%4u)<<13u)|((row.seed%2u)?0x8040u:0u)};
    if(row.policy==Policy::entry_mmx) {
        abi.x87_depth=abi.x87_status=0u;
        abi.active_mmx.emplace();abi.active_mmx->known_mask=0x5au;
        for(unsigned slot=0;slot<8u;++slot)for(unsigned byte=0;byte<10u;++byte)
            abi.active_mmx->physical[slot][byte]=std::byte(row.seed*17u+slot*11u+byte);
    }
    state.load(abi);
    // Mutate the real Services table after State cached its construction
    // witness. Refusal has to reach the complete retained body unchanged.
    if(row.policy==Policy::entry_clear)services.instruction={};
    if(row.policy==Policy::entry_rebind)services.instruction=rebound;
    if(row.policy==Policy::entry_thread)services.instruction_cursor.executor={};
    if(row.policy==Policy::entry_sites)services.instruction_cursor.service_sites=changed_sites;
    if(row.policy==Policy::entry_format)services.instruction_cursor.literal_service_sites=false;
    if(row.policy==Policy::entry_position)services.instruction_cursor.position=nullptr;
    if(row.policy==Policy::entry_status)state.fp.status|=0x8080u;
    m.packet=[&]{return capture(state,cursor);};
    const auto altered_normal=_mm_set_ps(0.f,0.f,0.f,-1.f);
    m.feedback=[&](unsigned n,char kind,std::uint32_t address){
        if(n!=row.event)return;
        switch(row.policy) {
        case Policy::install:services.instruction=callback;break;
        case Policy::trace:trace=true;break;
        case Policy::clear:services.instruction={};break;
        case Policy::rebind:services.instruction=rebound;break;
        case Policy::observer_install:observer=observe;break;
        case Policy::observer_clear:observer={};break;
        case Policy::registers:
            state.xmm[1]=altered_normal;state.gp[7]^=0x12345678u;
            if(kind=='r'&&address==sp+16u)state.gp[1]=b+0x20u;
            if(kind=='v'&&address==a)state.gp[1]=b+0x40u;
            break;
        case Policy::logical_stack:
            state.fp.status|=0x0200u;
            if(state.fp.count)state.fp.lanes[0]=-0.125L;
            break;
        case Policy::host:_mm_setcsr(0xbfc0u);break;
        case Policy::scratch:if(kind=='w')m.put(address,bits(-2.f));break;
        case Policy::foreign:throw SourceContinuation{0x303853u,&m};
        default:break;
        }
    };
    unsigned route=0,stop=~0u;
    try {
        if(candidate) {
#if defined(HALO2_CONTACT_PREDICATE_BOUND_TEST)
            auto& activity=native_contact_predicate_owner_detail::activity();
            activity={};
            run(state,~0u);
            route=activity.completed ? 1u : activity.last_next;
#else
            route=run_source_private<Format>(state);
            if(route!=1u)run(state,route);
#endif
        } else run(state,0u);
    } catch(const Stop& failure) {stop=failure.event;}
    catch(const SourceContinuation& error) {
        if(error.next!=0x303853u||error.owner!=&m||row.policy!=Policy::foreign)throw;
        stop=row.event;
    } catch(const std::runtime_error& error) {
        if(row.policy!=Policy::entry_full ||
           std::string_view(error.what())!="Required original solver x87 stack-fault owner unavailable")throw;
        stop=unsigned(m.events.size());
    }
    return {capture(state,cursor),std::move(m.events),std::move(m.bytes),stop,route};
}
struct Counts {
    unsigned paths{},events{},prefixes{},continuations{},native{},fallback{};
    std::set<std::uint32_t> next, traced;
};
void require(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}
template<SourceFormat Format,class Run> Counts compare(Run run) {
    Counts counts;
    bool force_reference=false;
#if defined(HALO2_CONTACT_PREDICATE_BOUND_TEST)
    force_reference=native_contact_predicate_owner_detail::force_reference();
#endif
    const auto same=[&](Case row,bool expect_native,bool expect_resume=false){
        const auto baseline=execute<Format>(row,run,false),actual=execute<Format>(row,run,true);
        const auto id=std::to_string(row.seed)+"/"+std::to_string(unsigned(row.policy))+"/"+std::to_string(row.event);
        require(actual.stop==baseline.stop,"predicate source stop differs "+id);
        require(actual.events.size()==baseline.events.size(),"predicate source event count differs "+id);
        for(unsigned n=0;n<actual.events.size();++n) {
            const auto& a=actual.events[n];const auto& b=baseline.events[n];
            if(a==b)continue;
            std::cerr<<std::hex<<"source boundary "<<a.kind<<" "<<a.address
                     <<" cursor "<<a.abi.cursor<<"/"<<b.abi.cursor
                     <<" FP "<<a.abi.status<<"/"<<b.abi.status
                     <<" MXCSR "<<a.abi.mxcsr<<"/"<<b.abi.mxcsr<<"\n";
            for(unsigned reg=0;reg<8u;++reg)for(unsigned lane=0;lane<4u;++lane)
                if(a.abi.xmm[reg][lane]!=b.abi.xmm[reg][lane])
                    std::cerr<<"xmm"<<reg<<"["<<lane<<"] "<<a.abi.xmm[reg][lane]
                             <<"/"<<b.abi.xmm[reg][lane]<<"\n";
            require(false,"predicate source event differs "+id+" "+std::to_string(n));
        }
        require(actual.abi==baseline.abi&&actual.bank==baseline.bank,"predicate source complete ABI/bank differs "+id);
        if(row.stop==~0u&&row.policy!=Policy::foreign) {
            if(force_reference)require(actual.route==0u,"predicate source reference control entered native path "+id);
            else if(expect_resume)require(actual.route>1u,"predicate source did not hand off "+id);
            else require(actual.route==(expect_native?1u:0u),"predicate source default/refusal differs "+id);
        }
        ++counts.paths;counts.events+=actual.events.size();counts.prefixes+=actual.stop!=~0u;
        counts.continuations+=actual.route>1u;counts.native+=actual.route==1u;
        if(actual.route>1u)counts.next.insert(actual.route);
        for(const auto& e:baseline.events)if(e.kind=='c'||e.kind=='b')counts.traced.insert(e.address);
        counts.fallback+=!expect_native&&!expect_resume;
        return baseline.events;
    };
    constexpr bool quiet=Format==SourceFormat::literal;
    for(unsigned seed=0;seed<104u;++seed)same({seed,~0u,~0u,quiet,bool(seed&1u)},true);
    for(unsigned seed:{0u,1u,2u,3u,4u,5u,10u,11u}) {
        Case row{seed,~0u,~0u,quiet,true};
        const auto events=same(row,true);
        for(unsigned n=0;n<events.size();++n) {
            row.policy=Policy::normal;row.event=~0u;
            row.stop=n;same(row,true);row.stop=~0u;row.event=n;
            if(n+1u<events.size()) {
                row.policy=quiet?Policy::trace:Policy::install;same(row,true,true);
                if(quiet){row.policy=Policy::clear;same(row,true,true);row.policy=Policy::rebind;same(row,true,true);}
            }
            for(auto policy:{Policy::registers,Policy::logical_stack,Policy::host,Policy::foreign}) {
                row.policy=policy;same(row,true);
            }
            if(events[n].kind=='w'){row.policy=Policy::scratch;same(row,true);}
        }
        same({seed,0,~0u,quiet,false,Policy::observer_install},true);
        same({seed,0,~0u,quiet,true,Policy::observer_clear},true);
        same({seed,~0u,~0u,true,true,Policy::visible},false);
        if(!quiet)same({seed,~0u,~0u,true,true},false);
    }
    for(unsigned seed:{0u,3u,7u,10u})
        for(auto policy:{Policy::entry_unknown,Policy::entry_clear,Policy::entry_rebind,Policy::entry_thread,
            Policy::entry_sites,Policy::entry_format,Policy::entry_position,Policy::entry_mmx,Policy::entry_full,Policy::entry_status})
            same({seed,~0u,~0u,quiet,true,policy},false);
    const std::set<std::uint32_t> next_expected{0x303801u,0x30380cu,0x30380fu,0x303810u,0x303816u,0x303824u,
        0x303828u,0x30382cu,0x30382fu,0x303853u,0x303857u,0x30385au,0x303864u,0x303867u,
        0x30388eu,0x303892u,0x303895u,0x30389fu,0x3038a2u,0x3038a3u,0x3038a6u,0x3038acu,0x3038afu,0x3038b0u,0x3038b3u};
    require(force_reference ? counts.next.empty() : counts.next==next_expected,
        "predicate retired continuation coverage incomplete");
    require(counts.traced.size()==60,"predicate traced original site coverage incomplete");
    return counts;
}
}
int main()try {
    const auto direct=[](WorldState& s,unsigned next){
#if defined(HALO2_CONTACT_PREDICATE_BOUND_TEST)
        if(next==~0u)return raw_bound::run_303800(s);
#endif
        if(next)raw_reference::resume_303800(s,next);else raw_reference::run_303800(s);
    };
    const auto literal=[](WorldState& s,unsigned next){
#if defined(HALO2_CONTACT_PREDICATE_BOUND_TEST)
        if(next==~0u)return prepared_bound::run_303800(s);
#endif
        if(next)prepared_reference::resume_303800(s,next);else prepared_reference::run_303800(s);
    };
    for(const auto [format, result] : std::array{
        std::pair{"direct",compare<SourceFormat::direct>(direct)},
        std::pair{"literal",compare<SourceFormat::literal>(literal)}}) {
        std::cout<<format<<" "<<result.paths<<" "<<result.events<<" "<<result.prefixes<<" "
                 <<result.continuations<<" "<<result.native<<" "<<result.fallback<<" "
                 <<result.next.size()<<" "<<result.traced.size()<<"\n";
    }
    return 0;
} catch(const std::exception& error) {
    std::cerr<<error.what()<<"\n";
    return 1;
}
