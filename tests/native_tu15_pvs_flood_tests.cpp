// Native TU1.5 PVS closure (1532A0/170C80/170FB0/44680) against the
// unmodified translated bodies, extracted by tools/validate_native_pvs_flood_tu15.py
// into pvs_flood_bodies.inc. Seeded fixtures author complete cluster/portal
// worlds, stack frames and caller registers; every case compares the ordered
// store events (pc, address, preimage, bytes), final bank, registers, flags and
// instruction cursor. Prints a JSON summary with branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_pvs_flood.hpp"
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace original {
std::uint32_t sign_extend(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unverified sign-extension width");
}
#include "pvs_flood_bodies.inc"
}
namespace {
struct Memory : SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::shared_ptr<std::array<std::byte,4096>>> pages;
    std::array<std::byte,4096> &page(std::uint32_t at,bool create) {
        auto found=pages.find(at>>12);
        if(found==pages.end()) {
            if(!create){char t[64];std::snprintf(t,sizeof t,"Access escaped the authored fixture at %x",at);throw std::runtime_error(t);}
            found=pages.emplace(at>>12,std::make_shared<std::array<std::byte,4096>>()).first;
        }
        return *found->second;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(std::size_t n=0;n<out.size();++n)out[n]=const_cast<Memory*>(this)->page(at+std::uint32_t(n),false)[(at+n)&4095u];
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        for(std::size_t n=0;n<in.size();++n)page(at+std::uint32_t(n),false)[(at+n)&4095u]=in[n];
    }
    void put(std::uint32_t at,std::uint32_t value,unsigned width=4) {
        for(unsigned n=0;n<width;++n)page(at+n,true)[(at+n)&4095u]=std::byte(value>>(8*n));
    }
    Memory clone()const {
        Memory copy;for(const auto &[k,v]:pages)copy.pages.emplace(k,std::make_shared<std::array<std::byte,4096>>(*v));return copy;
    }
    bool same(const Memory &other)const {
        if(pages.size()!=other.pages.size())return false;
        for(const auto &[k,v]:pages){const auto at=other.pages.find(k);if(at==other.pages.end()||*at->second!=*v)return false;}
        return true;
    }
};

struct Case { Memory memory; SourcePhysicsWorldRuntimeAbi abi{}; };

// One authored world. `shape` selects structural edge cases.
Case author(std::mt19937 &rng,unsigned shape) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t n){return std::uint32_t(rng()%n);};
    std::uint32_t clusters=1+pick(48);
    if(shape%11==3)clusters=1+pick(512);
    if(shape%11==5)clusters=32*(1+pick(16));
    if(shape==7)clusters=0;
    const std::uint32_t words=(clusters+31)/32;
    const std::uint32_t portals=clusters?pick(clusters*3+1):0;
    const std::uint32_t pwords=(portals+31)/32;
    constexpr std::uint32_t bsp=0x10000000u,rows=0x10100000u,cluster_table=0x10200000u,
        portal_table=0x10300000u,lists=0x10400000u,open_bits=0x10500000u,scenario=0x10600000u,
        flags_vector=0x10700000u,records=0x10800000u,destination=0x10900000u,stack=0x20010000u;
    const bool second=pick(4)!=0;
    const std::uint32_t row_bytes=clusters*words*4u;
    m.put(0x4ecc18u,bsp);m.put(0x4f54f4u,scenario);m.put(0x505e7cu,open_bits);
    m.put(bsp+0x54u,second?2u*row_bytes+pick(2)*4u:row_bytes-(row_bytes&&pick(2)?0u:0u));
    m.put(bsp+0x58u,rows);m.put(bsp+0x5cu,portals);m.put(bsp+0x60u,portal_table);
    m.put(bsp+0x9cu,clusters);m.put(bsp+0xa0u,cluster_table);
    const auto density=1+pick(8);
    for(std::uint32_t r=0;r<(second?2u:1u)*clusters;++r)
        for(std::uint32_t w=0;w<words;++w) {
            std::uint32_t bits=0;for(unsigned b=0;b<32;++b)if(pick(10)<density)bits|=1u<<b;
            if(shape%5==1)bits|=0xffffffffu*(pick(3)==0);
            m.put(rows+(r*words+w)*4u,bits);
        }
    // Pad one row past the plane for out-of-plane reads by unflagged rows.
    for(std::uint32_t w=0;w<words+4;++w)m.put(rows+(2*clusters*words+w)*4u,std::uint32_t(rng()));
    std::uint32_t list=lists;
    // Records exist for every bit a vector word can carry (padding bits
    // past the cluster count are iterated by the original too).
    for(std::uint32_t k=0;k<std::max(words*32u,1u);++k) {
        const auto record=cluster_table+k*0xb0u;
        m.put(record+0x80u,std::uint32_t(rng())&0xffu,1);
        if(shape%3==0)m.put(record+0x80u,2u|(std::uint32_t(rng())&0xf0u),1);
        std::uint32_t count=portals?pick(6):0;
        if(pick(9)==0)count=std::uint32_t(-std::int32_t(pick(3)));
        m.put(record+0x8cu,count);m.put(record+0x90u,list);
        for(std::uint32_t n=0;n<std::max<std::int32_t>(std::int32_t(count),0);++n){m.put(list,pick(portals),2);list+=2;}
        list+=2;
    }
    for(std::uint32_t p=0;p<portals;++p) {
        const auto record=portal_table+p*36u;
        m.put(record,pick(clusters),2);m.put(record+2u,pick(clusters),2);
        m.put(record+0x18u,(pick(4)?4u:0u)|(std::uint32_t(rng())&0xf3u),1);
    }
    for(std::uint32_t w=0;w<std::max(pwords,1u);++w)m.put(open_bits+w*4u,shape%4==2?0xffffffffu:std::uint32_t(rng()));
    // Scenario cluster list: count, bit vector, stride and records.
    std::uint32_t scenario_count=1+pick(70);
    if(shape==9)scenario_count=0;
    const std::uint32_t stride=0x30u;
    m.put(scenario+0x24u,stride);m.put(scenario+0x38u,scenario_count);
    m.put(scenario+0x44u,shape==10?0u:records);m.put(scenario+0x48u,flags_vector);
    for(std::uint32_t w=0;w<(scenario_count+31)/32+1;++w) {
        std::uint32_t bits=0;for(unsigned b=0;b<32;++b)if(pick(6)==0)bits|=1u<<b;
        m.put(flags_vector+w*4u,bits);
    }
    for(std::uint32_t i=0;i<scenario_count;++i) {
        const auto record=records+i*stride;
        m.put(record+0x28u,pick(5)==0?0xffffu:pick(30),2);
        // An empty cluster set still floods cluster 0 (zero-word vectors).
        m.put(record+0x2au,!clusters?0u:pick(6)==0?0xffffu:pick(clusters),2);
    }
    if(shape==10)for(std::uint32_t i=0;i<scenario_count;++i){m.put(i*stride+0x28u,pick(2)?0xffffu:1u,2);m.put(i*stride+0x2au,clusters?pick(clusters):0xffffu,2);}
    for(std::uint32_t w=0;w<16;++w)m.put(destination+w*4u,std::uint32_t(rng()));
    // Stack: the frames below ESP plus both arguments.
    for(std::uint32_t at=stack-0x400u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x13b63au);m.put(stack+4u,destination);m.put(stack+8u,pick(2)?1u:(std::uint32_t(rng())&0xffffff00u));
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[4]=stack;c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    return c;
}

struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{};bool admitted{};
};
Result execute(Memory &memory,const SourcePhysicsWorldRuntimeAbi &abi,bool native) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(native) {
        services.instruction=[](auto){throw std::runtime_error("Native PVS closure delivered a translated instruction");};
        auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {
        if(native)result.admitted=native_pvs_flood_detail::bridge(s,[](auto &v){original::run_1532a0(v);});
        else original::run_1532a0(s);
    } catch(const std::exception &e) {
        throw std::runtime_error(std::string(e.what())+(native?" (native":" (translated")+" cursor "+std::to_string(result.cursor)+")");
    }
    result.abi=s.save();return result;
}
}

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    std::mt19937 rng(0x1532a0u);
    std::uint64_t stores{},calls{},refused{};
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n%12);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native=execute(right,c.abi,true);
        if(!native.admitted){++refused;continue;}
        const auto reference=execute(left,c.abi,false);
        ++calls;stores+=reference.stores.size();
        if(native.stores!=reference.stores) {
            std::size_t at=0;while(at<native.stores.size()&&at<reference.stores.size()&&native.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"case %u store mismatch at %zu of %zu/%zu",n,at,native.stores.size(),reference.stores.size());
            if(at<native.stores.size()&&at<reference.stores.size())
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",native.stores[at].instruction,native.stores[at].address,
                    reference.stores[at].instruction,reference.stores[at].address);
            std::fprintf(stderr,"\n");return 1;
        }
        if(native.abi.gp!=reference.abi.gp||native.abi.eflags!=reference.abi.eflags||native.abi.xmm!=reference.abi.xmm||
           native.abi.x87_status!=reference.abi.x87_status||native.cursor!=reference.cursor||!left.same(right)) {
            std::fprintf(stderr,"case %u register/flag/bank/cursor mismatch\n",n);return 1;
        }
    }
    // Admission refusals: other image, direction flag, traces, oversized vectors.
    {
        auto c=author(rng,0);SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
        WorldState s{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};s.load(c.abi);
        if(native_pvs_flood_detail::bridge(s,[](auto&){}))throw std::runtime_error("MapPack11 admitted");
        WorldState d{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};auto abi=c.abi;abi.eflags|=0x400u;d.load(abi);
        if(native_pvs_flood_detail::bridge(d,[](auto&){}))throw std::runtime_error("Direction flag admitted");
        c.memory.put(0x10000000u+0x9cu,513);
        WorldState e{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};e.load(c.abi);
        if(native_pvs_flood_detail::bridge(e,[](auto&){}))throw std::runtime_error("Oversized cluster vector admitted");
        services.instruction=[](auto){};
        WorldState f{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};c.memory.put(0x10000000u+0x9cu,5);f.load(c.abi);
        if(native_pvs_flood_detail::bridge(f,[](auto&){}))throw std::runtime_error("Full instruction callbacks admitted");
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"refused\":%llu,\"ordered_stores\":%llu,\"coverage\":%llu,\"branches\":%u}\n",cases,
        (unsigned long long)calls,(unsigned long long)refused,(unsigned long long)stores,
        (unsigned long long)native_pvs_flood_detail::coverage().load(),unsigned(native_pvs_flood_detail::branch_count));
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
