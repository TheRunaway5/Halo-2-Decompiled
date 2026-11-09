#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_array_abi.hpp"
#include "physics/runtime/native_tu15_broadphase_remove.hpp"
#include "physics/runtime/native_tu15_broadphase_leaves.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>

namespace halo2 {
using namespace physics_world_runtime_detail;
struct ObservedWorldState : WorldState {using WorldState::WorldState;bool selected_observation_pcs=true;};
struct OwnerBoundary {};
struct CallRecord {
    std::uint32_t target{},site{};std::array<std::uint32_t,8> gp{};unsigned flags{};
    friend bool operator==(const CallRecord &,const CallRecord &)=default;
};
inline std::vector<CallRecord> *calls;
inline std::uint64_t translated_fallbacks{};
// Declared caller-owned allocator: it advances its real backing cursor and
// ownership accounting. Results are calculated from its live DATA, not given.
void declared_service(ObservedWorldState &s,std::uint32_t target,std::uint32_t site) {
    using namespace physics_step_detail;
    calls->push_back({target,site,s.gp,s.flags.bits()});auto &g=s.gp;
    if(target==0x70020000u) {
        g[2]=r(s.memory,g[4]+4u,4);g[0]=r(s.memory,g[1]+8u,4);
        const auto cursor=r(s.memory,g[1]+8u,4);
        w(s.memory,s.observe,0x70020008u,g[1]+8u,4,s.flags.plus(cursor,g[2],4));
        const auto active=r(s.memory,g[1]+36u,4);
        w(s.memory,s.observe,0x7002000bu,g[1]+36u,4,s.flags.plus(active,g[2],4));
        const auto count=r(s.memory,g[1]+28u,4);
        w(s.memory,s.observe,0x7002000eu,g[1]+28u,4,s.flags.increment(count,4,false));
        s.pop();g[4]+=8u;
    }else if(target==0x70020100u) {
        g[0]=r(s.memory,g[4]+4u,4);g[2]=r(s.memory,g[4]+8u,4);
        const auto active=r(s.memory,g[1]+36u,4);
        w(s.memory,s.observe,0x70020108u,g[1]+36u,4,s.flags.minus(active,g[2],4));
        const auto count=r(s.memory,g[1]+32u,4);
        w(s.memory,s.observe,0x7002010bu,g[1]+32u,4,s.flags.increment(count,4,false));
        s.pop();g[4]+=12u;
    }else throw OwnerBoundary{};
}
#include "broadphase_remove_bodies.inc"
}
namespace {
using namespace halo2;
struct Memory final : SourcePhysicsContactMemory {
    static constexpr std::uint32_t base=0x1000000u,arena=base+0x1000u;
    std::vector<std::byte> bytes=std::vector<std::byte>(65536);
    mutable std::vector<std::pair<std::uint32_t,std::size_t>> reads;
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        reads.emplace_back(p,out.size());
        if(p==0x48c938u && out.size()==4u){const std::uint32_t value=arena;std::memcpy(out.data(),&value,4);return;}
        if(p<base || p-base+out.size()>bytes.size())throw std::runtime_error("read outside fixture");
        std::memcpy(out.data(),bytes.data()+p-base,out.size());
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        if(p<base || p-base+in.size()>bytes.size())throw std::runtime_error("write outside fixture");
        std::memcpy(bytes.data()+p-base,in.data(),in.size());
    }
    void word(std::uint32_t p,std::uint32_t x){std::memcpy(bytes.data()+p-base,&x,4);}
};
struct Result {
    std::array<std::uint32_t,8> gp{};std::array<std::array<std::byte,16>,8> xmm{};
    unsigned flags{},other_flags{},mxcsr{},fp_count{},fp_top{},fp_status{},pc{};bool boundary{};
    std::vector<SourcePhysicsContactRecordStore> stores;std::vector<CallRecord> calls;
    friend bool operator==(const Result &,const Result &)=default;
};
template<class Routine> Result evaluate(Memory &memory,Routine routine,
    const std::array<std::uint32_t,8> &gp,unsigned flags,unsigned mxcsr,unsigned depth,unsigned top,std::uint16_t control) {
    Result result;SourcePhysicsContactRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){result.pc=pc;};
    const std::array<std::uint32_t,3> sites{0x38846eu,0x21f590u,0x33b595u};
    auto &cursor=services.instruction_cursor;cursor.position=&result.pc;
    cursor.executor=std::this_thread::get_id();cursor.callback_type=&services.instruction.target_type();
    cursor.literal_service_sites=true;cursor.observation_only_stores=true;cursor.service_sites=sites;
    SourcePhysicsContactRecordObserver observe=[&](const auto &store){result.stores.push_back(store);};
    ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    s.fs_base=Memory::base+0xff00u;s.gp=gp;s.flags.load(flags);s.other_flags=flags&1024u;s.fp.count=depth;s.fp.top=top;s.fp.status=0x200u;
    for(unsigned i=0;i<depth;++i)s.fp.lanes[i]=static_cast<long double>(i+1u);
    for(unsigned i=0;i<8;++i)s.xmm[i]=_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));
    asm volatile("fninit\n\tfldcw %0"::"m"(control):"memory");_mm_setcsr(mxcsr);calls=&result.calls;
    try{routine(s);}catch(const OwnerBoundary&){result.boundary=true;}
    result.mxcsr=_mm_getcsr();result.flags=s.flags.bits();result.other_flags=s.other_flags;result.gp=s.gp;
    result.fp_count=s.fp.count;result.fp_top=s.fp.top;result.fp_status=s.save().x87_status&0x23fu;
    for(unsigned i=0;i<8;++i)std::memcpy(result.xmm[i].data(),&s.xmm[i],16);return result;
}
}
int main(int argc,char **argv) {
    try {
        std::mt19937 random(0x2ec9b0u);unsigned cases=0,hardware_cases=0,boundaries=0;
        native_broadphase_remove_detail::activity()={};native_partition_temporary_detail::activity()={};native_array_abi_detail::activity()={};
        for(unsigned sample=0;sample<2304;++sample) {
            Memory original;for(auto &byte:original.bytes)byte=std::byte(random());
            constexpr auto base=Memory::base,broad=base+0x2000u,nodes=base+0x3000u,handles=base+0x7000u,output=base+0x2800u;
            const auto count=sample<32u?2u:2u+(sample/8u)%191u,index=1u+(sample%8u)%(count-1u),sp=base+0xf000u+sample%16u;
            const auto boundary_profile=sample>=2048u?1u+(sample-2048u)%4u:0u;
            const bool cells=(sample&2u)!=0&&!boundary_profile;
            original.word(Memory::arena,base+0x1100u);original.word(Memory::arena+8u,base+0xa000u);
            original.word(Memory::arena+12u,boundary_profile==1u?0u:0x10000u);original.word(Memory::arena+20u,boundary_profile==4u?base+0xa000u:base+0x9000u);
            original.word(Memory::arena+28u,0);original.word(Memory::arena+32u,0);original.word(Memory::arena+36u,0);
            original.word(base+0x1110u,boundary_profile==2u?0x149da0u:0x70020000u);original.word(base+0x1114u,boundary_profile==3u?0x149e00u:0x70020100u);
            original.word(base+0x1124u,0x2e10a0u);original.word(base+0x1128u,0x2e1110u);
            original.word(broad+0x40u,nodes);original.word(broad+0x44u,count);original.word(broad+0x48u,((sample&4u)?count-2u:count)|0x80000000u);
            original.word(broad+0x70u,cells);original.word(broad+0x74u,1);original.word(broad+0x78u,base+0x100u);
            for(unsigned node=0;node<count;++node) {original.word(handles+node*4u,node);original.word(nodes+node*16u+12u,handles+node*4u);}
            if(cells)original.word(nodes+16u+12u,1u);
            std::vector<unsigned> crossing;
            for(unsigned axis=0;axis<3;++axis) {
                struct Edge{unsigned coordinate,node;bool upper;};std::vector<Edge> edges{{0,0,false},{65535,0,true}};
                for(unsigned node=1;node<count;++node) {
                    unsigned low=2u+2u*unsigned(random()%30000u),high=low+2u*unsigned(random()%((65532u-low)/2u+1u));
                    if(boundary_profile){low=2u;high=65532u;}
                    if(cells&&node==1u){low=axis==0u?32768u:0u;high=axis==0u?32768u:65534u;}
                    if(cells&&axis==0u&&node!=1u&&low<=32768u&&high>=32768u)crossing.push_back(node);
                    edges.push_back({low,node,false});edges.push_back({high|1u,node,true});
                }
                std::sort(edges.begin(),edges.end(),[](const auto &a,const auto &b){return a.coordinate<b.coordinate;});
                const auto edge_data=base+0x4000u+axis*0x1000u;
                original.word(broad+0x4cu+axis*12u,edge_data);original.word(broad+0x50u+axis*12u,edges.size());original.word(broad+0x54u+axis*12u,edges.size()|0x80000000u);
                for(unsigned edge=0;edge<edges.size();++edge) {
                    const auto &item=edges[edge];original.word(edge_data+edge*4u,item.coordinate|(item.node<<16u));
                    const auto offset=axis==0u?8u:axis==1u?0u:2u;
                    const std::uint16_t packed=edge;std::memcpy(original.bytes.data()+nodes-base+item.node*16u+offset+(item.upper?axis==0u?2u:4u:0u),&packed,2);
                }
            }
            if(cells) {
                original.word(base+0x100u,1u);original.word(base+0x104u,base+0xc000u);original.word(base+0x108u,crossing.size());
                original.word(base+0x10cu,crossing.size()|0x80000000u);
                for(unsigned i=0;i<crossing.size();++i){const std::uint16_t node=crossing[i];std::memcpy(original.bytes.data()+0xc000u+i*2u,&node,2);}
            }
            original.word(output,base+0x8000u);original.word(output+4u,0);original.word(output+8u,boundary_profile?0u:((sample&1u)?0u:128u|0x80000000u));
            original.word(sp,0x103f000u);original.word(sp+4u,handles+index*4u);original.word(sp+8u,output);original.word(base+0xff00u,0xdeadbeefu);
            std::array<std::uint32_t,8> gp;for(auto &value:gp)value=random();gp[4]=sp;gp[1]=broad;
            const auto input_bytes=original.bytes;auto native=original;
            const unsigned flags=(random()&0x8d5u)|((sample>=16u&&sample<32u)?1024u:0u),mxcsr=0x1f80u|((sample%4u)<<13u)|((sample&4u)?0x8040u:0u);
            const std::uint16_t control=0x027fu|((sample%4u)<<10u);
            const auto expected=evaluate(original,reference::run_2ec9b0,gp,flags,mxcsr,sample%4u,sample%8u,control);
            const auto actual=evaluate(native,production::run_2ec9b0,gp,flags,mxcsr,sample%4u,sample%8u,control);
            if(expected!=actual || original.bytes!=native.bytes || original.reads!=native.reads) {
                std::cerr<<"remove mismatch sample="<<sample<<" gp="<<(expected.gp==actual.gp)<<" flags="<<(expected.flags==actual.flags)
                    <<" xmm="<<(expected.xmm==actual.xmm)<<" mxcsr="<<(expected.mxcsr==actual.mxcsr)<<" stores="<<(expected.stores==actual.stores)
                    <<" memory="<<(original.bytes==native.bytes)<<" reads="<<(original.reads==native.reads)<<" calls="<<(expected.calls==actual.calls)<<'\n';
                for(unsigned i=0;i<8;++i)if(expected.gp[i]!=actual.gp[i])std::cerr<<"gp"<<i<<' '<<std::hex<<expected.gp[i]<<'/'<<actual.gp[i]<<std::dec<<'\n';
                for(unsigned i=0;i<std::min(expected.stores.size(),actual.stores.size());++i)if(expected.stores[i]!=actual.stores[i]){
                    const auto &a=expected.stores[i],&b=actual.stores[i];std::cerr<<"store "<<i<<std::hex<<" pc "<<a.instruction<<'/'<<b.instruction<<" address "<<a.address<<'/'<<b.address<<std::dec<<'\n';break;}
                for(unsigned i=0;i<std::min(original.reads.size(),native.reads.size());++i)if(original.reads[i]!=native.reads[i]){
                    std::cerr<<"read "<<i<<std::hex<<' '<<original.reads[i].first<<'/'<<native.reads[i].first<<std::dec<<'\n';break;}
                return 1;
            }
            if(actual.boundary!=bool(boundary_profile))throw std::runtime_error("Missing actual-owner boundary prefix");
            if(actual.boundary)++boundaries;
            if(argc==2 && sample<2048u && (sample<32u||sample%16u==(sample/16u)%16u)) {
                std::ofstream stream(std::string(argv[1])+"/hardware-0.bin",std::ios::binary|std::ios::app);
                const auto write=[&](const auto &value){stream.write(reinterpret_cast<const char *>(&value),sizeof(value));};
                write(gp);write(flags);write(mxcsr);for(unsigned i=0;i<8;++i){const auto x=_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));write(x);}
                stream.write(reinterpret_cast<const char *>(input_bytes.data()),input_bytes.size());write(actual.gp);write(actual.flags);write(actual.mxcsr);write(actual.xmm);
                stream.write(reinterpret_cast<const char *>(native.bytes.data()),native.bytes.size());
                std::ofstream fp(std::string(argv[1])+"/hardware-fp-0.bin",std::ios::binary|std::ios::app);fp.write(reinterpret_cast<const char *>(&control),2);
                const std::uint16_t status=actual.fp_status;fp.write(reinterpret_cast<const char *>(&status),2);++hardware_cases;
            }
            ++cases;
        }
        const auto stats=native_broadphase_remove_detail::activity();const auto arrays=native_array_abi_detail::activity();const auto temps=native_partition_temporary_detail::activity();
        const auto expected_fallbacks=std::getenv("HALO2_REFERENCE_BROADPHASE_REMOVE")?cases:0u;
        if(translated_fallbacks!=expected_fallbacks)throw std::runtime_error("Unexpected translated body fallback in native closure");
        std::cout<<"{\"passed\":true,\"cases\":"<<cases<<",\"hardware_cases\":"<<hardware_cases<<",\"native_removals\":"<<stats.removals
            <<",\"normal_returns\":"<<cases-boundaries<<",\"actual_owner_boundaries\":"<<boundaries
            <<",\"pairs\":"<<stats.pairs<<",\"dense_moves\":"<<stats.dense_moves<<",\"cell_removals\":"<<stats.cell_removals
            <<",\"native_temporary_constructors\":"<<temps.constructors<<",\"native_temporary_destructors\":"<<temps.destructors
            <<",\"native_array_growths\":"<<arrays.growths<<",\"allocator_residuals\":"<<arrays.allocations+arrays.releases
            <<",\"translated_body_fallbacks\":"<<translated_fallbacks<<"}\n";
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
