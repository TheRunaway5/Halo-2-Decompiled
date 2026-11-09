#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_lazy_heap_abi.hpp"
#include "platform/native_tu15_tick_conversion.hpp"
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <nlohmann/json.hpp>

namespace halo2 {
using namespace physics_world_runtime_detail;
void lazy_data_owner(WorldState&,std::uint32_t,std::uint32_t,bool);
void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){lazy_data_owner(s,target,site,tail);}
namespace lazy_data_reference {
static void run_33b590(WorldState&s){lazy_data_owner(s,0x33b590u,0,false);}
static void run_2dbf74(WorldState&s){lazy_data_owner(s,0x2dbf74u,0,false);}
static void run_1cd130(WorldState&s){lazy_data_owner(s,0x1cd130u,0,false);}
#include "lazy_heap_data_reference.inc"
}
}
namespace {
using namespace halo2;using State=physics_world_runtime_detail::WorldState;using J=nlohmann::json;
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    mutable std::vector<std::pair<std::uint32_t,std::size_t>> reads;
    std::vector<SourcePhysicsContactRecordStore> stores;
    bool recording{};std::function<void(Memory&,const SourcePhysicsContactRecordStore&)> feedback;
    auto locate(std::uint32_t p,std::size_t size)const {
        auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked lazy DATA operand "+std::to_string(p));--i;
        check(std::uint64_t(p-i->first)+size<=i->second.size(),"Unbacked lazy DATA extent "+std::to_string(p));return i;
    }
    void read(std::uint32_t p,std::span<std::byte>out)const override {
        if(recording)reads.emplace_back(p,out.size());const auto i=locate(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>in)override {
        const auto i=locate(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);
    }
    std::uint32_t word(std::uint32_t p)const {std::array<std::byte,4>b{};read(p,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8);return v;}
    void put(std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(n*8));write(p,b);}
    SourcePhysicsContactRecordObserver observer(){return [&](const auto&e){stores.push_back(e);if(feedback)feedback(*this,e);};}
};
// These are independent test owners, not production allocator outcomes. The
// arena and retained reservation are actual banks; freeing decreases a backed
// pin count and allocation succeeds only when the requested extent fits.
constexpr std::uint32_t arena=0x65000000u,ledger=0x64000000u,clock_cell=0x63000000u;
constexpr std::uint32_t manager=0x61000000u,context=0x61100000u,process=0x61200000u;
struct Fixture {
    std::string name;unsigned capacity=0x800000u,pins{},pressure=1,alignment{},depth{};
    std::uint32_t initial{},constructor=0x500000u,timeout=0x3f000000u,stack=0x62001000u,deadline{},eflags=0xad7u;
    std::uint64_t ticks=0x100000000ull,step=0x10000000ull;
    bool fallback{},negative_elapsed{};unsigned feedback_kind{},stop_owner{};
    std::uint16_t control=0x27f;std::uint32_t mxcsr=0x1f80;
};
struct Call {
    std::uint32_t owner{},site{};SourcePhysicsWorldRuntimeAbi abi;
};
struct StopOwner {std::uint32_t owner;};
bool same_abi(const SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsWorldRuntimeAbi&b) {
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_status!=b.x87_status||a.x87_depth!=b.x87_depth||a.active_mmx.has_value()!=b.active_mmx.has_value())return false;
    if(a.active_mmx||b.active_mmx)throw std::runtime_error("Lazy DATA seam does not admit active MMX aliases");
    if(a.floating_point->x87_control_word!=b.floating_point->x87_control_word||a.floating_point->mxcsr!=b.floating_point->mxcsr)return false;
    for(unsigned i=0;i<a.x87_depth;++i)if(std::memcmp(&a.x87[i],&b.x87[i],10))return false;return true;
}
struct Environment {
    Memory &m;const Fixture&f;std::vector<Call> calls;std::vector<std::uint32_t> real_clock_sites;
    std::map<std::uint32_t,unsigned> counts;std::uint64_t tick{};unsigned clock{},samples{};
    explicit Environment(Memory&memory,const Fixture&fixture):m(memory),f(fixture),tick(f.ticks){}
    std::uint64_t timestamp(){const auto result=tick;tick+=f.negative_elapsed?0x8000000000000000ull:f.step;++samples;check(samples<64,"Lazy timestamp failed to converge");return result;}
    void milliseconds(std::uint32_t pc){if(pc==0x33b595u){real_clock_sites.push_back(pc);clock+=73u;m.put(clock_cell,clock);}}
    void volatile_fp(State&s,std::uint32_t owner) {
        for(unsigned n=0;n<8;++n)s.xmm[n]=_mm_castsi128_ps(_mm_set_epi32(int(owner+n),int(owner^n),int(owner+n*3u),int(owner-n)));
        s.fp.status^=0x100u;
    }
    void invoke(State&s,std::uint32_t owner,std::uint32_t site,bool tail) {
        if(!site&&s.instruction_cursor&&s.instruction_cursor->position)site=*s.instruction_cursor->position;
        check(!tail,"Unexpected lazy DATA tail owner");calls.push_back({owner,site,s.save()});++counts[owner];
        if(f.stop_owner==owner)throw StopOwner{owner};
        if(owner==0x22a5c0u){check(native_tick_conversion_detail::convert(s),"Qualified tick converter refused DATA seam");return;}
        if(owner==0x33b590u){check(native_lazy_heap_detail::read_milliseconds(s),"Clock service owner refused DATA seam");return;}
        const auto input=s.gp[4];
        if(owner==0x12fe00u){
            const auto bytes=m.word(input+4u),available=m.word(ledger),blocked=m.word(ledger+4u),used=m.word(ledger+8u);
            s.gp[0]=!blocked&&bytes<=available-used?arena+f.alignment+used:0;
            if(s.gp[0])m.put(ledger+8u,used+bytes);
            s.gp[1]=bytes^0x159753u;s.gp[2]=available;volatile_fp(s,owner);s.flags.logic(s.gp[0],4);s.pop();s.gp[4]+=16u;return;
        }
        if(owner==0x12ef10u){const auto pins=m.word(ledger+4u);if(pins)m.put(ledger+4u,pins-1u);}
        else if(owner==0x12ef60u){m.put(ledger+12u,m.word(ledger+12u)+1u);}
        else if(owner==0x12f0c0u){m.put(ledger+16u,m.word(ledger+16u)+1u);}
        else if(owner==0x12edf0u){m.put(ledger+20u,m.word(ledger+20u)+1u);}
        else if(owner==0x404d10u){s.gp[2]=context;}
        else if(owner==0x2dbf74u){m.put(ledger+24u,m.word(ledger+24u)+1u);}
        else if(owner==0x1cd130u){
            check(m.word(input+4u)==1u,"Lazy trim lost actual aggressive argument");m.put(ledger+28u,m.word(ledger+28u)+1u);
            s.gp[0]=owner^0xabcdu;s.gp[1]=owner^0x1234u;s.gp[2]=owner^0x5678u;volatile_fp(s,owner);s.flags.logic(s.gp[0],4);s.pop();s.gp[4]+=4u;return;
        }
        else throw std::runtime_error("Unknown lazy DATA owner "+std::to_string(owner));
        s.gp[0]=0xabc00000u|owner;s.gp[1]=0xdef00000u|owner;
        if(owner!=0x404d10u)s.gp[2]=owner^0x12345678u;volatile_fp(s,owner);s.flags.logic(s.gp[0],4);s.pop();
    }
};
thread_local Environment* current{};
Memory initial(const Fixture&f) {
    Memory m;m.banks.emplace(0x410000u,std::vector<std::byte>(0x150000u));
    m.banks.emplace(manager,std::vector<std::byte>(0x1000));m.banks.emplace(context,std::vector<std::byte>(0x1000));
    m.banks.emplace(process+0x3240u,std::vector<std::byte>(16));m.banks.emplace(process+0x400700u,std::vector<std::byte>(16));
    m.banks.emplace(0x62000000u,std::vector<std::byte>(8192));m.banks.emplace(clock_cell,std::vector<std::byte>(4096));
    m.banks.emplace(ledger,std::vector<std::byte>(4096));m.banks.emplace(arena,std::vector<std::byte>(std::max(4096u,f.capacity+16u)));
    m.banks.emplace(0x600000u,std::vector<std::byte>(0x6000));
    m.banks.emplace(0x240000u,std::vector<std::byte>(0x10000));m.banks.emplace(0x100000u,std::vector<std::byte>(0x10000));
    m.put(0x486074u,f.initial);m.put(0x48605cu,f.constructor);m.put(0x4f2d34u,manager);m.put(manager+0x30u,f.pressure);
    m.put(0x4f2d50u,f.deadline);m.put(0x4f2d54u,0x1234);m.put(0x411568u,context);m.put(context+0x934u,process);
    m.put(process+0x3240u,0x01020304);m.put(process+0x3244u,f.fallback?0x01020304:0x50607080);m.put(process+0x400700u,0x11223344);
    m.put(0x41b774u,clock_cell);m.put(0x46a0d0u,f.timeout);m.put(ledger,f.capacity);m.put(ledger+4u,f.pins);m.put(f.stack,0x63001000);
    if(f.feedback_kind)m.feedback=[kind=f.feedback_kind,stack=f.stack](auto&memory,const auto&e){
        if(kind==1&&e.instruction==0x14999fu)memory.put(stack-20u,90u);
        if(kind==2&&e.instruction==0x14995eu)memory.put(stack-12u,0x7fffffffu);
        if(kind==3&&(e.instruction==0x149a9cu||e.instruction==0x149ab4u))memory.put(0x486074u,0xdeadbeefu);
        if(kind==4&&e.instruction==0x149a91u)memory.put(stack-36u,0xaabbccddu);
    };
    return m;
}
struct Outcome {SourcePhysicsWorldRuntimeAbi abi;std::vector<Call>calls;std::map<std::uint32_t,unsigned>counts;std::vector<std::uint32_t>clock_sites;std::uint32_t pc{};unsigned samples{},residual{};};
std::set<std::uint32_t> executed;
Outcome run(Memory&m,const Fixture&f,bool native,bool read_pass) {
    Environment environment(m,f);current=&environment;SourcePhysicsWorldRuntimeServices services;
    std::uint32_t pc{};services.instruction=[&](auto site){pc=site;if(!native)executed.insert(site);environment.milliseconds(site);};
    services.timestamp_counter=[&]{return environment.timestamp();};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto&policy=services.instruction_cursor;policy.position=&pc;policy.callback_type=&services.instruction.target_type();
    policy.executor=std::this_thread::get_id();policy.literal_service_sites=true;policy.observation_only_stores=true;policy.service_sites=sites;
    auto observer=read_pass?SourcePhysicsContactRecordObserver{}:m.observer();State s{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x11111111,0x22222222,0x33333333,0x44444444,f.stack,0x55555555,0x66666666,0x77777777};abi.eflags=f.eflags;
    for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=0x3f000001u+n*31u+k;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{f.control,f.mxcsr};abi.x87_depth=f.depth;abi.x87_status=0x4100;
    for(unsigned n=0;n<f.depth;++n)abi.x87[n]=n?0x1.123456789abcdefp-7L:0x1.000000000000001p+0L;
    s.load(abi);m.recording=read_pass;
    unsigned residual{};
    try {
        if(native)check(native_lazy_heap_detail::run(s,[](auto&state,auto target,auto site,bool tail){lazy_data_owner(state,target,site,tail);}),"Native lazy DATA refused admitted fixture");
        else lazy_data_reference::run_149930(s);
    }catch(const StopOwner&stop){residual=stop.owner;}
    m.recording=false;current=nullptr;return {s.save(),environment.calls,environment.counts,environment.real_clock_sites,pc,environment.samples,residual};
}
}
namespace halo2 {void lazy_data_owner(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){check(current!=nullptr,"No owning lazy DATA services");current->invoke(s,target,site,tail);}}
int main()try {
    std::vector<Fixture> fixtures;
    fixtures.push_back({.name="already-initialized",.initial=0x12345678});
    fixtures.push_back({.name="pressure-zero",.pressure=0});fixtures.push_back({.name="pressure-negative",.pressure=0xffffffffu});
    fixtures.push_back({.name="first-allocation"});fixtures.push_back({.name="cleanup-backed-release",.pins=2});
    fixtures.push_back({.name="no-notify-deadline",.pins=2,.deadline=0xffffffffu});
    fixtures.push_back({.name="halve-to-backed-capacity",.capacity=0x400000});
    fixtures.push_back({.name="permanently-pinned-deadline",.pins=1000});
    fixtures.push_back({.name="yield-current",.capacity=0,.step=0x08000000});
    fixtures.push_back({.name="yield-fallback",.capacity=0,.step=0x08000000,.fallback=true});
    fixtures.push_back({.name="ctor-null",.capacity=0,.pressure=0,.constructor=0xffefabc0u});
    fixtures.push_back({.name="unordered-timeout",.capacity=0,.timeout=0x7fc12345u});
    fixtures.push_back({.name="negative-elapsed",.capacity=0,.timeout=0,.negative_elapsed=true});
    for(unsigned alignment=1;alignment<16;++alignment)fixtures.push_back({.name="allocation-alignment-"+std::to_string(alignment),.alignment=alignment});
    for(unsigned kind=1;kind<5;++kind)fixtures.push_back({.name="observer-feedback-"+std::to_string(kind),.pins=2,.feedback_kind=kind});
    for(auto depth:{1u,4u})fixtures.push_back({.name="live-fp-"+std::to_string(depth),.capacity=0,.depth=depth});
    fixtures.push_back({.name="constructor-saved-register-alias",.stack=0x605460u});
    fixtures.push_back({.name="constructor-base-call-return-alias",.stack=0x486094u});
    fixtures.push_back({.name="retry-notification-global-alias",.pins=1,.stack=0x4f2d64u});
    fixtures.push_back({.name="start-high-constructor-base-alias",.stack=0x486068u});
    fixtures.push_back({.name="elapsed-lazy-frame-feedback",.capacity=0,.timeout=0,.feedback_kind=2});
    for(auto owner:{0x12fe00u,0x12ef10u,0x12ef60u,0x12f0c0u,0x33b590u,0x12edf0u,0x22a5c0u,0x404d10u,0x2dbf74u,0x1cd130u}) {
        Fixture f{.name="explicit-residual-"+std::to_string(owner),.capacity=0,.pins=1000,.stop_owner=owner};fixtures.push_back(f);
    }
    for(unsigned alignment=1;alignment<16;++alignment){Fixture f{.name="caller-stack-alignment-"+std::to_string(alignment),.stack=0x62001000u+alignment,.eflags=(alignment&1u)?0xed7u:0xad7u};fixtures.push_back(f);}
    for(auto precision:{0x7fu,0x27fu,0x37fu})for(unsigned rounding=0;rounding<4;++rounding){
        Fixture f{.name="fp-environment-"+std::to_string(precision)+"-"+std::to_string(rounding),.capacity=0,.depth=4};
        f.control=std::uint16_t(precision|(rounding<<10u));f.mxcsr=0x1fc1u|(rounding<<13u);fixtures.push_back(f);
    }
    unsigned transactions{},reads{},stores{},calls{},residuals{};std::map<std::uint32_t,unsigned>owners;J cases=J::array();
    for(auto f:fixtures)for(bool read_pass:{false,true}) {
        Memory a=initial(f),b=a;const auto expected=run(a,f,false,read_pass),actual=run(b,f,true,read_pass);
        check(expected.residual==actual.residual&&actual.residual==f.stop_owner,f.name+": real child boundary differs");
        check(same_abi(expected.abi,actual.abi),f.name+": final complete ABI differs");check(expected.pc==actual.pc,f.name+": final cursor differs");
        check(expected.samples==actual.samples&&expected.clock_sites==actual.clock_sites,f.name+": genuine clock observations differ");
        check(expected.calls.size()==actual.calls.size(),f.name+": actual child call count differs");
        for(unsigned n=0;n<expected.calls.size();++n){const auto&x=expected.calls[n];const auto&y=actual.calls[n];check(x.owner==y.owner&&x.site==y.site&&same_abi(x.abi,y.abi),f.name+": genuine child entry ABI differs at "+std::to_string(n)+" owner "+std::to_string(x.owner));}
        check(a.banks==b.banks,f.name+": complete DATA banks differ");check(a.stores==b.stores,f.name+": ordered stores differ");
        if(read_pass&&a.reads!=b.reads){unsigned n{};while(n<a.reads.size()&&n<b.reads.size()&&a.reads[n]==b.reads[n])++n;throw std::runtime_error(f.name+": ordered read differs at "+std::to_string(n));}
        ++transactions;reads+=a.reads.size();stores+=a.stores.size();calls+=expected.calls.size();for(const auto&[owner,count]:expected.counts)owners[owner]+=count;
        if(expected.residual)++residuals;
        if(!read_pass){
            if(f.name=="first-allocation")check(expected.counts.at(0x12fe00u)==1&&a.word(ledger+8u)==f.capacity,"Backed first-allocation branch missing");
            if(f.name=="cleanup-backed-release")check(expected.counts.at(0x12fe00u)==3&&expected.counts.at(0x12ef10u)==2,"Backed cleanup release branch missing");
            if(f.name=="halve-to-backed-capacity")check(a.word(ledger+8u)==0x400000u&&expected.counts.at(0x22a5c0u)>0,"Actual size reduction branch missing");
            if(f.name=="yield-current"||f.name=="yield-fallback")check(expected.counts.at(0x2dbf74u)>0,"Actual yield branch missing");
            if(f.name=="ctor-null")check(expected.abi.gp[0]==0,"Actual null-constructor branch missing");
            cases.push_back(J{{"name",f.name},{"residual",expected.residual},{"source_result",expected.abi.gp[0]},{"child_calls",expected.calls.size()},{"timestamp_samples",expected.samples}});
        }
    }
    std::cout<<J{{"passed",true},{"transactions",transactions},{"fixtures",fixtures.size()},{"residual_boundaries",residuals},{"ordered_reads",reads},{"ordered_stores",stores},{"child_entry_packets",calls},{"owners",owners},{"executed_parent_sites",executed},{"cases",cases},{"declared_test_service_owners",true},{"production_child_closure_qualified",false}}.dump()<<std::endl;return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
