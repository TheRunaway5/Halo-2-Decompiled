#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include <iostream>
#include <fstream>
#include <random>
#include <tuple>

namespace halo2 {
using namespace physics_world_runtime_detail;
#include "collision_matrix_leaf_bodies.inc"
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
    void word(std::uint32_t p,std::uint32_t value){std::memcpy(bytes.data()+p-base,&value,4);}
};
struct Result {
    std::array<std::uint32_t,8> gp{};
    std::array<std::array<std::byte,16>,8> xmm{};
    unsigned flags{},mxcsr{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    friend bool operator==(const Result &,const Result &)=default;
};
template<class Routine> Result evaluate(Memory &mem,Routine routine,
    const std::array<std::uint32_t,8> &gp,unsigned flags,unsigned mxcsr) {
    Result result;
    SourcePhysicsContactRuntimeServices services;
    SourcePhysicsContactRecordObserver observe=[&](const auto &store){result.stores.push_back(store);};
    WorldState s{mem,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    s.gp=gp;s.flags.load(flags);
    for(unsigned i=0;i<8;++i)s.xmm[i]=_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));
    _mm_setcsr(mxcsr);routine(s);result.mxcsr=_mm_getcsr();
    result.gp=s.gp;result.flags=s.flags.bits();
    for(unsigned i=0;i<8;++i)std::memcpy(result.xmm[i].data(),&s.xmm[i],16);
    return result;
}
}

int main(int argc,char **argv) {
    try {
        std::mt19937 random(0x2e44d0u);
        constexpr std::array<std::uint32_t,12> edge={0u,0x80000000u,0x3f800000u,0xbf800000u,
            1u,0x80000001u,0x007fffffu,0x7f7fffffu,0x7f800000u,0xff800000u,0x7fc01234u,0x7f801234u};
        unsigned cases=0;
        for(unsigned entry=0;entry<2;++entry)for(unsigned sample=0;sample<768;++sample) {
            Memory original;
            for(auto &b:original.bytes)b=std::byte(random());
            const std::uint32_t a=Memory::base+0x1000,b=Memory::base+0x1100;
            const std::array<std::uint32_t,8> destinations={Memory::base+0x1200,a,b,a+16,b+16,a-16,b-16,Memory::base+0x7f80};
            const auto out=destinations[sample%destinations.size()];
            for(unsigned i=0;i<32;++i) {
                const auto value=sample<192?edge[(sample+i)%edge.size()]:
                    std::bit_cast<std::uint32_t>(float(std::int32_t(random()%200000)-100000)/128.0f);
                original.word(a+i*4,value);
                original.word(b+i*4,value^((i&1u)?0x80000000u:0u));
            }
            const auto sp=Memory::base+0x8000u+(sample%4u)*4u;
            original.word(sp,0x103f000u);original.word(sp+4,a);original.word(sp+8,b);
            std::array<std::uint32_t,8> gp;
            for(auto &g:gp)g=random();gp[1]=out;gp[4]=sp;
            auto native=original;
            const unsigned flags=random()&0x8d5u;
            const unsigned mxcsr=0x1f80u|((sample%4u)<<13u)|((sample&4u)?0x8040u:0u);
            const auto input_bytes=original.bytes;
            const auto expected=entry==0?
                evaluate(original,sample<192?reference::run_2e44d0:legacy::run_2e44d0,gp,flags,mxcsr):
                evaluate(original,sample<192?reference::run_2e46e0:legacy::run_2e46e0,gp,flags,mxcsr);
            const auto actual=entry==0?
                evaluate(native,production::run_2e44d0,gp,flags,mxcsr):
                evaluate(native,production::run_2e46e0,gp,flags,mxcsr);
            if(argc==2 && sample<192) {
                std::ofstream stream(std::string(argv[1])+"/hardware-"+std::to_string(entry)+".bin",std::ios::binary|std::ios::app);
                const auto write=[&](const auto &value){stream.write(reinterpret_cast<const char *>(&value),sizeof(value));};
                write(gp);write(flags);write(mxcsr);
                for(unsigned i=0;i<8;++i) {
                    const auto x=_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));write(x);
                }
                stream.write(reinterpret_cast<const char *>(input_bytes.data()),input_bytes.size());
                write(actual.gp);write(actual.flags);write(actual.mxcsr);write(actual.xmm);
                stream.write(reinterpret_cast<const char *>(native.bytes.data()),native.bytes.size());
            }
            const bool same_reads=std::getenv("HALO2_VERIFY_MATRIX_LEAVES") || original.reads==native.reads;
            if(expected!=actual || original.bytes!=native.bytes || !same_reads) {
                std::cerr<<"collision matrix mismatch entry="<<entry<<" sample="<<sample
                    <<" gp="<<(expected.gp==actual.gp)<<" flags="<<(expected.flags==actual.flags)
                    <<" xmm="<<(expected.xmm==actual.xmm)<<" mxcsr="<<(expected.mxcsr==actual.mxcsr)
                    <<" stores="<<(expected.stores==actual.stores)<<" memory="<<(original.bytes==native.bytes)
                    <<" reads="<<(original.reads==native.reads)<<'\n';
                for(unsigned i=0;i<8;++i)if(expected.xmm[i]!=actual.xmm[i]) {
                    std::array<std::uint32_t,4> e{},a{};
                    std::memcpy(e.data(),expected.xmm[i].data(),16);std::memcpy(a.data(),actual.xmm[i].data(),16);
                    std::cerr<<"xmm"<<i<<std::hex;for(unsigned lane=0;lane<4;++lane)std::cerr<<" "<<e[lane]<<":"<<a[lane];std::cerr<<std::dec<<'\n';
                }return 1;
            }
            ++cases;
        }
        std::cout<<"{\"passed\":true,\"cases\":"<<cases<<",\"entries\":[\"2E44D0\",\"2E46E0\"]}\n";
        return 0;
    } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
