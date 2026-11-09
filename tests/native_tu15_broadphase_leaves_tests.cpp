#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_broadphase_leaves.hpp"
#include <fstream>
#include <iostream>
#include <random>

namespace halo2 {
using namespace physics_world_runtime_detail;
#include "broadphase_leaf_bodies.inc"
}
namespace {
using namespace halo2;
struct Memory final : SourcePhysicsContactMemory {
    static constexpr std::uint32_t base=0x1000000u;
    std::vector<std::byte> bytes=std::vector<std::byte>(65536);
    mutable std::vector<std::pair<std::uint32_t,std::size_t>> reads;
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        if(p<base || p-base+out.size()>bytes.size())throw std::runtime_error("read outside fixture");
        reads.emplace_back(p,out.size());std::memcpy(out.data(),bytes.data()+p-base,out.size());
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        if(p<base || p-base+in.size()>bytes.size())throw std::runtime_error("write outside fixture");
        std::memcpy(bytes.data()+p-base,in.data(),in.size());
    }
    void word(std::uint32_t p,std::uint32_t x,unsigned width=4){std::memcpy(bytes.data()+p-base,&x,width);}
};
struct Result {
    std::array<std::uint32_t,8> gp{};
    std::array<std::array<std::byte,16>,8> xmm{};
    unsigned flags{},mxcsr{},fp_count{},fp_top{};
    bool active_mmx{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    friend bool operator==(const Result &,const Result &)=default;
};
template<class Routine> Result evaluate(Memory &memory,Routine routine,
    const std::array<std::uint32_t,8> &gp,unsigned flags,unsigned mxcsr,unsigned depth,unsigned top) {
    Result result;SourcePhysicsContactRuntimeServices services;
    SourcePhysicsContactRecordObserver observe=[&](const auto &store){result.stores.push_back(store);};
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    s.gp=gp;s.flags.load(flags);s.other_flags=flags&1024u;
    s.fp.count=depth;s.fp.top=top;
    for(unsigned i=0;i<depth;++i)s.fp.lanes[i]=static_cast<long double>(i+1u);
    for(unsigned i=0;i<8;++i)s.xmm[i]=_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));
    _mm_setcsr(mxcsr);routine(s);result.mxcsr=_mm_getcsr();result.flags=s.flags.bits();result.gp=s.gp;
    result.fp_count=s.fp.count;result.fp_top=s.fp.top;result.active_mmx=bool(s.active_mmx);
    for(unsigned i=0;i<8;++i)std::memcpy(result.xmm[i].data(),&s.xmm[i],16);
    return result;
}
}
int main(int argc,char **argv) {
    try {
        std::mt19937 random(0x2e9df0u);unsigned cases=0;
        for(unsigned entry=0;entry<2;++entry)for(unsigned sample=0;sample<1024;++sample) {
            Memory original;
            for(auto &byte:original.bytes)byte=std::byte(random());
            const std::uint32_t nodes=Memory::base+0x3000u,header=Memory::base+0x2000u;
            const auto sp=Memory::base+0x8000u+sample%16u;
            const auto count=entry==0?sample%64u:2u+sample%62u;
            const auto first=count>1?sample%(count-1u):0u;
            const auto second=count>1?first+1u+(sample/4u)%(count-first-1u):0u;
            const auto bounds=(sample&4u)?nodes+((count?count:1u)-1u)*16u:Memory::base+0x1000u;
            original.word(sp,0x103f000u);
            original.word(sp+4u,entry==0?nodes:first);
            original.word(sp+8u,entry==0?count:second);
            original.word(sp+12u,bounds);
            original.word(header,nodes);original.word(header+4u,count);
            for(unsigned i=0;i<6;++i)original.word(bounds+i*2u,(sample<64?sample+i:random())&65535u,2);
            std::array<std::uint32_t,8> gp;
            for(auto &value:gp)value=random();gp[4]=sp;gp[1]=header;
            const auto input_bytes=original.bytes;
            auto native=original;
            const unsigned flags=(random()&0x8d5u)|((sample&2u)?1024u:0u);
            const unsigned mxcsr=0x1f80u|((sample%4u)<<13u)|((sample&4u)?0x8040u:0u);
            const auto depth=sample%4u,top=sample%8u;
            const auto expected=entry==0?evaluate(original,reference::run_2e9df0,gp,flags,mxcsr,depth,top):
                evaluate(original,reference::run_2e9fd0,gp,flags,mxcsr,depth,top);
            const auto actual=entry==0?evaluate(native,production::run_2e9df0,gp,flags,mxcsr,depth,top):
                evaluate(native,production::run_2e9fd0,gp,flags,mxcsr,depth,top);
            if(argc==2 && sample<128u) {
                std::ofstream stream(std::string(argv[1])+"/hardware-"+std::to_string(entry)+".bin",std::ios::binary|std::ios::app);
                const auto write=[&](const auto &value){stream.write(reinterpret_cast<const char *>(&value),sizeof(value));};
                write(gp);write(flags);write(mxcsr);
                for(unsigned i=0;i<8;++i){const auto x=_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));write(x);}
                stream.write(reinterpret_cast<const char *>(input_bytes.data()),input_bytes.size());
                write(actual.gp);write(actual.flags);write(actual.mxcsr);write(actual.xmm);
                stream.write(reinterpret_cast<const char *>(native.bytes.data()),native.bytes.size());
            }
            const bool same_reads=std::getenv("HALO2_VERIFY_MATRIX_LEAVES") || original.reads==native.reads;
            if(expected!=actual || original.bytes!=native.bytes || !same_reads) {
                std::cerr<<"broadphase mismatch entry="<<entry<<" sample="<<sample
                    <<" gp="<<(expected.gp==actual.gp)<<" flags="<<(expected.flags==actual.flags)
                    <<" xmm="<<(expected.xmm==actual.xmm)<<" mxcsr="<<(expected.mxcsr==actual.mxcsr)
                    <<" stores="<<(expected.stores==actual.stores)<<" memory="<<(original.bytes==native.bytes)
                    <<" reads="<<same_reads<<" fp="<<expected.fp_count<<'/'<<actual.fp_count<<' '
                    <<expected.fp_top<<'/'<<actual.fp_top<<'\n';return 1;
            }
            ++cases;
        }
        std::cout<<"{\"passed\":true,\"cases\":"<<cases<<",\"entries\":[\"2E9DF0\",\"2E9FD0\"]}\n";
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
