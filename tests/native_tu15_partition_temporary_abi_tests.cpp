#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"
#include <fstream>
#include <iostream>
#include <random>

namespace halo2 {
using namespace physics_world_runtime_detail;
struct ObservedWorldState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs=true;
};
struct ResidualBoundary {};
struct CallRecord {
    std::uint32_t target{},site{};
    std::array<std::uint32_t,8> gp{};
    unsigned flags{};
    friend bool operator==(const CallRecord &,const CallRecord &)=default;
};
inline std::vector<CallRecord> *calls;
void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    calls->push_back({target,site,s.gp,s.flags.bits()});throw ResidualBoundary{};
}
#include "partition_temporary_abi_bodies.inc"
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
    std::array<std::uint32_t,8> gp{};
    std::array<std::array<std::byte,16>,8> xmm{};
    unsigned flags{},mxcsr{},fp_count{},fp_top{},fp_status{};
    bool boundary{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<CallRecord> calls;
    friend bool operator==(const Result &,const Result &)=default;
};
template<class Routine> Result evaluate(Memory &memory,Routine routine,
    const std::array<std::uint32_t,8> &gp,unsigned flags,unsigned mxcsr,unsigned depth,unsigned top,std::uint16_t control) {
    Result result;SourcePhysicsContactRuntimeServices services;
    SourcePhysicsContactRecordObserver observe=[&](const auto &store){result.stores.push_back(store);};
    ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    s.gp=gp;s.flags.load(flags);s.fp.count=depth;s.fp.top=top;s.fp.status=0x200u;s.fs_base=Memory::base+0xf000u;
    for(unsigned i=0;i<depth;++i)s.fp.lanes[i]=static_cast<long double>(i+1u);
    for(unsigned i=0;i<8;++i)s.xmm[i]=_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));
    asm volatile("fninit\n\tfldcw %0"::"m"(control):"memory");_mm_setcsr(mxcsr);
    calls=&result.calls;
    try{routine(s);}catch(const ResidualBoundary&){result.boundary=true;}
    result.mxcsr=_mm_getcsr();result.flags=s.flags.bits();result.gp=s.gp;
    result.fp_count=s.fp.count;result.fp_top=s.fp.top;result.fp_status=s.save().x87_status&0x23fu;
    for(unsigned i=0;i<8;++i)std::memcpy(result.xmm[i].data(),&s.xmm[i],16);
    return result;
}
}
int main(int argc,char **argv) {
    try {
        std::mt19937 random(0x2eba20u);unsigned cases=0,normal=0,boundaries=0,hardware_cases=0;
        native_partition_temporary_detail::activity()={};
        for(unsigned entry=0;entry<2;++entry)for(unsigned sample=0;sample<768;++sample) {
            Memory original;for(auto &byte:original.bytes)byte=std::byte(random());
            const auto sp=Memory::base+0x9000u+sample%16u;
            const auto header=entry==0&&sample%7u==0u?sp+4u:Memory::base+0x2000u;
            const auto profile=sample%4u;
            const std::array<std::uint32_t,6> counts={0u,1u,3u,31u,0x40000000u,127u};
            const auto count=counts[(sample/4u)%counts.size()];
            const auto table=Memory::base+0x1100u;
            original.word(Memory::arena,table);original.word(Memory::arena+8u,Memory::base+0x6000u);
            original.word(Memory::arena+12u,entry==0&&profile==1?0u:0x10000u);
            original.word(Memory::arena+20u,profile==1?Memory::base+0x5000u:Memory::base+0x4000u);
            original.word(table+0x24u,0x2e10a0u);original.word(table+0x28u,0x2e1110u);
            original.word(table+0x14u,0x2e0eb0u);
            original.word(header,Memory::base+0x7000u);original.word(header+4u,count);
            original.word(header+8u,(entry==1&&profile==2?0u:0x80000000u)|count);
            original.word(header+12u,Memory::base+0x5000u);original.word(header+16u,count);
            original.word(sp,0x103f000u);original.word(sp+4u,count);original.word(Memory::base+0xf000u,0xdeadbeefu);
            std::array<std::uint32_t,8> gp;for(auto &value:gp)value=random();gp[4]=sp;gp[1]=header;
            const auto input_bytes=original.bytes;auto native=original;
            const unsigned flags=random()&0x8d5u,mxcsr=0x1f80u|((sample%4u)<<13u)|((sample&4u)?0x8040u:0u);
            const std::uint16_t control=0x027fu|((sample%4u)<<10u);
            const auto expected=entry==0?evaluate(original,reference::run_2e7da0,gp,flags,mxcsr,sample%4u,sample%8u,control):
                evaluate(original,reference::run_2e8090,gp,flags,mxcsr,sample%4u,sample%8u,control);
            const auto actual=entry==0?evaluate(native,production::run_2e7da0,gp,flags,mxcsr,sample%4u,sample%8u,control):
                evaluate(native,production::run_2e8090,gp,flags,mxcsr,sample%4u,sample%8u,control);
            if(argc==2 && sample<128u && !actual.boundary) {
                std::ofstream stream(std::string(argv[1])+"/hardware-"+std::to_string(entry)+".bin",std::ios::binary|std::ios::app);
                const auto write=[&](const auto &value){stream.write(reinterpret_cast<const char *>(&value),sizeof(value));};
                write(gp);write(flags);write(mxcsr);
                for(unsigned i=0;i<8;++i){const auto x=_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));write(x);}
                stream.write(reinterpret_cast<const char *>(input_bytes.data()),input_bytes.size());
                write(actual.gp);write(actual.flags);write(actual.mxcsr);write(actual.xmm);
                stream.write(reinterpret_cast<const char *>(native.bytes.data()),native.bytes.size());
                std::ofstream fp(std::string(argv[1])+"/hardware-fp-"+std::to_string(entry)+".bin",std::ios::binary|std::ios::app);
                fp.write(reinterpret_cast<const char *>(&control),2);const std::uint16_t status=actual.fp_status;
                fp.write(reinterpret_cast<const char *>(&status),2);++hardware_cases;
            }
            if(expected!=actual || original.bytes!=native.bytes || original.reads!=native.reads) {
                std::cerr<<"temporary ABI mismatch entry="<<entry<<" sample="<<sample
                    <<" gp="<<(expected.gp==actual.gp)<<" flags="<<(expected.flags==actual.flags)
                    <<" xmm="<<(expected.xmm==actual.xmm)<<" mxcsr="<<(expected.mxcsr==actual.mxcsr)
                    <<" stores="<<(expected.stores==actual.stores)<<" memory="<<(original.bytes==native.bytes)
                    <<" reads="<<(original.reads==native.reads)<<" calls="<<(expected.calls==actual.calls)<<'\n';
                for(unsigned i=0;i<std::min(expected.stores.size(),actual.stores.size());++i)
                    if(expected.stores[i]!=actual.stores[i]) {
                        const auto &a=expected.stores[i],&b=actual.stores[i];
                        std::cerr<<"store="<<i<<std::hex<<" pc "<<a.instruction<<'/'<<b.instruction<<" address "<<a.address<<'/'<<b.address;
                        for(unsigned j=0;j<std::min(a.written.size(),b.written.size());++j)std::cerr<<' '<<std::to_integer<unsigned>(a.written[j])<<'/'<<std::to_integer<unsigned>(b.written[j]);
                        std::cerr<<std::dec<<'\n';break;
                    }
                return 1;
            }
            ++cases;if(actual.boundary)++boundaries;else ++normal;
        }
        const auto stats=native_partition_temporary_detail::activity();
        std::cout<<"{\"passed\":true,\"cases\":"<<cases<<",\"normal_returns\":"<<normal
            <<",\"residual_boundaries\":"<<boundaries<<",\"hardware_cases\":"<<hardware_cases
            <<",\"native_constructors\":"<<stats.constructors<<",\"native_destructors\":"<<stats.destructors
            <<",\"arena_allocation_residuals\":"<<stats.arena_allocations<<",\"arena_release_residuals\":"<<stats.arena_releases
            <<",\"owned_release_residuals\":"<<stats.owned_releases<<"}\n";
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
