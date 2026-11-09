#include "halo2/native_tu15_main_heap_allocate.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <iostream>
#include <map>
#include <random>

namespace halo2 {
using namespace physics_world_runtime_detail;
std::uint32_t canonical_branch(std::uint32_t value,SourcePhysicsContactImage) {
    switch(value){case 0:return 0;case 0x232c14:return 0x22e314;case 0x232c1a:return 0x22e31a;
    case 0x232d4e:return 0x22e44e;case 0x232d64:return 0x22e464;default:return value;}
}
namespace allocation_reference {
static void run_22e430(WorldState&);
#include "main_heap_allocate_reference.inc"
}
}
namespace {
using namespace halo2;
using Slot=NativeTu15MainHeapAllocationSlot;using Phase=NativeTu15MainHeapAllocationPhase;
constexpr std::uint32_t heap=0x10001000u,base=0x10003000u,stack=0x20000000u,sp=stack+0x1000u;
constexpr std::array<std::uint32_t,8> incoming{0xa1a1a1a1u,heap,0xc3c3c3c3u,0xb4b4b4b4u,sp,0xb5b5b5b5u,0xb6b6b6b6u,0xb7b7b7b7u};
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    mutable std::vector<std::pair<std::uint32_t,std::size_t>> reads;
    std::vector<SourcePhysicsContactRecordStore> stores;
    bool record{};std::function<void(Memory&,const SourcePhysicsContactRecordStore&)> mutate;
    auto at(std::uint32_t p,std::size_t n)const {
        auto i=banks.upper_bound(p);if(i==banks.begin())throw std::runtime_error("Unbacked allocation operand");--i;
        if(std::uint64_t(p-i->first)+n>i->second.size())throw std::runtime_error("Unbacked allocation extent");return i;
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        if(record)reads.emplace_back(p,out.size());const auto i=at(p,out.size());
        std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);
    }
    std::uint32_t word(std::uint32_t p)const {std::array<std::byte,4>b{};read(p,b);std::uint32_t v{};for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(8*n);return v;}
    void put(std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8*n));write(p,b);}
    SourcePhysicsContactRecordObserver observer(){return [&](const auto&e){stores.push_back(e);if(mutate)mutate(*this,e);};}
    void publish(std::uint32_t site,std::uint32_t p,std::uint32_t v){
        SourcePhysicsContactRecordStore e{site,p,{},{}};e.before.resize(4);read(p,e.before);
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8*n));write(p,b);
        e.written.assign(b.begin(),b.end());stores.push_back(e);if(mutate)mutate(*this,e);
    }
};
struct Frame:NativeTu15MainHeapAllocationFrame {
    Memory&m;std::uint32_t frame=sp-0x24u;
    explicit Frame(Memory&memory):m(memory){}
    std::uint32_t address(Slot slot)const {
        constexpr std::array offsets{0x10u,0x14u,0x18u,0x1cu,0x20u,0x28u,0x2cu};return frame+offsets.at(unsigned(slot));
    }
    std::uint32_t load(Slot slot)const override{return m.word(address(slot));}
    void store(Slot slot,std::uint32_t value,std::uint32_t site)override{m.publish(site,address(slot),value);}
};
Memory initial(std::uint32_t bytes,std::uint32_t memory_class,
    const std::vector<std::pair<std::uint32_t,std::uint32_t>>&records,std::uint32_t extent=0x1000u) {
    Memory m;m.banks.emplace(0x10000000u,std::vector<std::byte>(0x10000u));m.banks.emplace(stack,std::vector<std::byte>(0x2000u));
    constexpr std::array<std::uint8_t,17> classes{0,0,1,2,1,2,2,2,1,2,2,2,2,2,2,2,1};
    for(auto [at,raw,aligned]:{std::array{0x232c5cu,0x232c1au,0x232c14u},std::array{0x232d70u,0x232d4eu,0x232d64u}}){
        m.banks.emplace(at,std::vector<std::byte>(28));m.put(at,raw);m.put(at+4u,aligned);m.put(at+8u,0);
        for(unsigned n=0;n<classes.size();++n)m.banks.at(at)[11+n]=std::byte(classes[n]);
    }
    m.put(heap+0x18u,base);m.put(heap+0x1cu,extent);m.put(heap+0x20u,records.size());
    for(unsigned n=0;n<records.size();++n){m.put(base+extent-(n+1u)*8u,records[n].first);m.put(base+extent-(n+1u)*8u+4u,records[n].second);}
    for(unsigned n=0;n<0x80u;n+=4u)m.put(sp-0x60u+n,0x5a000000u+n);
    m.put(sp,0x63000000u);m.put(sp+4u,memory_class);m.put(sp+8u,bytes);m.put(sp+12u,0x87654321u);return m;
}
struct Outcome {std::uint32_t result{},owner{};bool residual{},fault{};};
Outcome reference(Memory&m) {
    auto observe=m.observer();SourcePhysicsContactRuntimeServices services;
    services.required_owner=[](auto&,const auto&call,auto&,const auto&){throw call.target;};
    WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    SourcePhysicsContactRuntimeAbi abi{};abi.gp=incoming;abi.eflags=0xad7;s.load(abi);
    try{allocation_reference::run_22e1d0(s);return {s.gp[0],0,false,false};}
    catch(std::uint32_t owner){return {0,owner,true,false};}
    catch(const std::runtime_error&){return {0,0,false,true};}
}
Outcome native(Memory&m) {
    Frame frame(m);auto observe=m.observer();NativeTu15MainHeapAllocationPhases phases;
    phases.observe=[&](auto phase,const auto&){
        if(phase==Phase::begin){m.publish(0x232ad3u,sp-24u,incoming[3]);m.publish(0x232ad4u,sp-28u,incoming[5]);m.publish(0x232ad5u,sp-32u,incoming[6]);}
        else if(phase==Phase::count)m.publish(0x232ae1u,sp-36u,incoming[7]);
        else if(phase==Phase::returning){for(auto p:{sp-36u,sp-32u,sp-28u,sp-24u,sp})(void)m.word(p);}
    };
    NativeTu15MainHeapAllocationServices services;
    services.probe_gap=[&](const auto&request,std::uint32_t allocator,std::uint32_t memory_class,auto&sink){
        const auto p=frame.frame;
        m.publish(0x232b46u,p-4u,memory_class);m.publish(0x232b47u,p-8u,allocator);m.publish(0x232b4eu,p-12u,0x232b53u);
        NativeTu15MainHeapAllocationPhases child;
        child.observe=[&](auto phase,const auto&){
            if(phase==Phase::gap_begin)m.publish(0x232d30u,p-16u,allocator);
            else if(phase==Phase::gap_capacity)m.publish(0x232d39u,p-20u,request.cursor);
            else if(phase==Phase::gap_returning){for(auto word:{p-20u,p-16u,p-12u})(void)m.word(word);}
        };
        return native_tu15_probe_main_heap_gap(m,request,{[&]{return m.word(p-8u);},[&]{return m.word(p-4u);}},sink,child,observe);
    };
    try{const auto r=native_tu15_allocate_main_heap_with_phases(m,heap,frame,services,phases,observe);
        return {r.allocation,r.residual.owner,r.residual.required,false};}
    catch(const std::runtime_error&){return {0,0,false,true};}
}
void compare(std::string name,Memory input,bool read_pass) {
    Memory a=input,b=input;a.record=b.record=read_pass;
    const auto ar=reference(a),br=native(b);
    check(ar.result==br.result&&ar.residual==br.residual&&ar.owner==br.owner&&ar.fault==br.fault,name+": result/residual differs");
    check(a.banks==b.banks,name+": DATA/caller banks differ");
    check(a.stores==b.stores,name+": ordered publications differ");
    if(read_pass&&a.reads!=b.reads){
        unsigned n{};while(n<a.reads.size()&&n<b.reads.size()&&a.reads[n]==b.reads[n])++n;
        throw std::runtime_error(name+": ordered read mismatch at "+std::to_string(n)+
            " source="+(n<a.reads.size()?std::to_string(a.reads[n].first):"end")+
            " native="+(n<b.reads.size()?std::to_string(b.reads[n].first):"end"));
    }
}
}
int main()try {
    unsigned count{};const auto test=[&](std::string name,Memory m){for(bool reads:{false,true}){compare(name,m,reads);++count;}};
    for(auto memory_class:{0u,1u,2u,4u,8u,16u})for(auto bytes:{0u,1u,15u,16u,17u,0x100u,0x1000u,0xffffffffu}) {
        test("empty",initial(bytes,memory_class,{}));
        test("early-hole",initial(bytes,memory_class,{{0x200u,0x80u},{0x500u,0x100u}}));
        test("middle-hole",initial(bytes,memory_class,{{0,0x80u},{0x200u,0x100u},{0x500u,0x80u}}));
        test("tail",initial(bytes,memory_class,{{0,0x100u},{0x100u,0x100u}}));
    }
    for(auto memory_class:{3u,17u,0xffffffffu})test("explicit-invalid-alignment",initial(16,memory_class,{}));
    for(auto raw_count:{0xffffffffu,0x80000000u,0x7fffffffu}){auto m=initial(16,2,{});m.put(heap+0x20u,raw_count);test("signed-count",m);}
    for(auto at:{heap+0x18u,heap+0x1cu,heap+0x20u,sp-0x14u,sp-0x10u,sp-0xcu,sp-8u,sp-4u,sp+4u,sp+8u}) {
        auto m=initial(16,2,{{0,5u},{0x100u,0x40u}});
        m.mutate=[at](auto&memory,const auto&event){if(event.instruction==0x232b55u)memory.put(at,0);};
        test("published-frame-feedback",m);
    }
    for(auto extent:{0x80u,0xc0u,0x100u,0x120u}) {
        auto m=initial(16,2,{});const auto aliased_base=sp-0x100u;
        m.put(heap+0x18u,aliased_base);m.put(heap+0x1cu,extent);m.put(heap+0x20u,2);
        m.put(aliased_base+extent-8u,0x40u);m.put(aliased_base+extent-4u,16u);
        m.put(aliased_base+extent-16u,0x80u);m.put(aliased_base+extent-12u,16u);
        test("ledger-caller-frame-alias",m);
    }
    for(auto memory_class:{0u,2u,8u})for(auto bytes:{1u,17u,0x100u,0x2000u}) {
        auto a=initial(bytes,memory_class,{{0,5u},{0x100u,0x40u}}),b=a;
        const auto expected=reference(a);NativeTu15MainHeapAllocationStats stats;
        const auto actual=native_tu15_allocate_main_heap(b,heap,bytes,memory_class,b.observer(),&stats);
        check(!expected.fault&&!expected.residual&&expected.result==actual,"Typed value caller result differs");
        check(a.banks.at(0x10000000u)==b.banks.at(0x10000000u),"Typed value caller changed heap DATA");
        check(stats.allocations==1,"Typed value caller bypassed shared DATA core");++count;
    }
    std::cout<<"Native main-heap allocation DATA: "<<count<<" source-reference transactions, full banks and ordered reads/stores\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
