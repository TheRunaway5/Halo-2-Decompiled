#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_broadphase_bounds_leaves.hpp"
#include <fstream>
#include <iostream>
#include <random>

namespace halo2 {
using namespace physics_world_runtime_detail;
#include "broadphase_bounds_leaf_bodies.inc"
}
namespace {
using namespace halo2;
struct Memory final : SourcePhysicsContactMemory {
    static inline unsigned current_entry{},current_sample{};
    static constexpr std::uint32_t base=0x1000000u;
    std::vector<std::byte> bytes=std::vector<std::byte>(65536);
    mutable std::vector<std::pair<std::uint32_t,std::size_t>> reads;
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        reads.emplace_back(p,out.size());
        if(p>=0x46a0d0u && p+out.size()<=0x46a0d4u) {
            const std::uint32_t one=0x3f800000u;
            std::memcpy(out.data(),reinterpret_cast<const std::byte *>(&one)+p-0x46a0d0u,out.size());return;
        }
        if(p<base || p-base+out.size()>bytes.size()) {
            std::cerr<<"entry="<<current_entry<<" sample="<<current_sample<<" read="<<std::hex<<p<<std::dec<<'\n';
            throw std::runtime_error("read outside fixture");
        }
        std::memcpy(out.data(),bytes.data()+p-base,out.size());
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        if(p<base || p-base+in.size()>bytes.size())throw std::runtime_error("write outside fixture");
        std::memcpy(bytes.data()+p-base,in.data(),in.size());
    }
    void word(std::uint32_t p,std::uint32_t x,unsigned width=4){std::memcpy(bytes.data()+p-base,&x,width);}
    void real(std::uint32_t p,float x){word(p,std::bit_cast<std::uint32_t>(x));}
};
struct Result {
    std::array<std::uint32_t,8> gp{};
    std::array<std::array<std::byte,16>,8> xmm{};
    unsigned flags{},mxcsr{},fp_count{},fp_top{},fp_status{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    friend bool operator==(const Result &,const Result &)=default;
};
template<class Routine> Result evaluate(Memory &memory,Routine routine,
    const std::array<std::uint32_t,8> &gp,unsigned flags,unsigned mxcsr,unsigned depth,unsigned top,std::uint16_t control) {
    Result result;SourcePhysicsContactRuntimeServices services;
    SourcePhysicsContactRecordObserver observe=[&](const auto &store){result.stores.push_back(store);};
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    s.gp=gp;s.flags.load(flags);s.fp.count=depth;s.fp.top=top;s.fp.status=0x200u;
    for(unsigned i=0;i<depth;++i)s.fp.lanes[i]=static_cast<long double>(i+1u);
    for(unsigned i=0;i<8;++i)s.xmm[i]=_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));
    asm volatile("fninit\n\tfldcw %0"::"m"(control):"memory");
    _mm_setcsr(mxcsr);routine(s);
    result.mxcsr=_mm_getcsr();result.flags=s.flags.bits();result.gp=s.gp;
    result.fp_count=s.fp.count;result.fp_top=s.fp.top;result.fp_status=s.save().x87_status&0x23fu;
    for(unsigned i=0;i<8;++i)std::memcpy(result.xmm[i].data(),&s.xmm[i],16);
    return result;
}
}
int main(int argc,char **argv) {
    try {
        std::mt19937 random(0x2ea050u);unsigned cases=0;
        const std::array<std::uint32_t,12> edges={0u,0x80000000u,0x3f800000u,0xbf800000u,
            1u,0x80000001u,0x7f7fffffu,0x00800000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u};
        for(unsigned entry=0;entry<2;++entry)for(unsigned sample=0;sample<2048;++sample) {
            Memory::current_entry=entry;Memory::current_sample=sample;
            Memory original;for(auto &byte:original.bytes)byte=std::byte(random());
            const auto sp=Memory::base+0x9000u+sample%16u;
            const auto handle=Memory::base+0x1000u,broad=Memory::base+0x2000u,nodes=Memory::base+0x3000u;
            const std::array<std::uint32_t,3> arrays={Memory::base+0x4000u,Memory::base+0x4200u,Memory::base+0x4400u};
            const unsigned index=sample%16u;const auto node=nodes+index*16u;
            for(unsigned axis=0;axis<3;++axis) {
                original.word(broad+76u+axis*12u,arrays[axis]);
                const auto scale=sample<128u?edges[(sample+axis)%edges.size()]:
                    std::bit_cast<std::uint32_t>(float(1u+random()%100000u)/257.0f);
                original.word(broad+48u+axis*4u,scale);
                original.word(broad+16u+axis*4u,sample<128u?edges[(sample+axis+3u)%edges.size()]:
                    std::bit_cast<std::uint32_t>(float(std::int32_t(random()%100000u)-50000)/123.0f));
                for(unsigned edge=0;edge<128;++edge)original.word(arrays[axis]+edge*4u,random()&65535u,2);
            }
            original.real(broad+28u,(sample&1u)?-0.0f:0.0f);
            original.word(broad+64u,nodes);original.word(handle,index);
            for(unsigned i=0;i<6;++i)original.word(node+i*2u,random()%128u,2);
            const std::array<std::uint32_t,8> outputs={Memory::base+0x5000u,node,broad,arrays[0],
                (sp-64u)&~15u,(sp-48u)&~15u,(sp-32u)&~15u,(sp+32u)&~15u};
            original.word(sp,0x103f000u);original.word(sp+4u,entry==0?node:handle);
            original.word(sp+8u,outputs[sample%outputs.size()]);
            std::array<std::uint32_t,8> gp;for(auto &value:gp)value=random();gp[4]=sp;gp[1]=broad;
            const auto input_bytes=original.bytes;auto native=original;
            const unsigned flags=random()&0x8d5u,mxcsr=0x1f80u|((sample%4u)<<13u)|((sample&4u)?0x8040u:0u);
            const std::array<std::uint16_t,3> precisions={0x007fu,0x027fu,0x037fu};
            const std::uint16_t control=precisions[(sample/4u)%3u]|((sample%4u)<<10u);
            const auto depth=sample%4u,top=sample%8u;
            const auto expected=entry==0?evaluate(original,reference::run_2ea050,gp,flags,mxcsr,depth,top,control):
                evaluate(original,reference::run_2eb0c0,gp,flags,mxcsr,depth,top,control);
            const auto actual=entry==0?evaluate(native,production::run_2ea050,gp,flags,mxcsr,depth,top,control):
                evaluate(native,production::run_2eb0c0,gp,flags,mxcsr,depth,top,control);
            if(argc==2 && sample<128u) {
                std::ofstream stream(std::string(argv[1])+"/hardware-"+std::to_string(entry)+".bin",std::ios::binary|std::ios::app);
                const auto write=[&](const auto &value){stream.write(reinterpret_cast<const char *>(&value),sizeof(value));};
                write(gp);write(flags);write(mxcsr);
                for(unsigned i=0;i<8;++i){const auto x=_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));write(x);}
                stream.write(reinterpret_cast<const char *>(input_bytes.data()),input_bytes.size());
                write(actual.gp);write(actual.flags);write(actual.mxcsr);write(actual.xmm);
                stream.write(reinterpret_cast<const char *>(native.bytes.data()),native.bytes.size());
                std::ofstream fp(std::string(argv[1])+"/hardware-fp-"+std::to_string(entry)+".bin",std::ios::binary|std::ios::app);
                fp.write(reinterpret_cast<const char *>(&control),2);const std::uint16_t status=actual.fp_status;
                fp.write(reinterpret_cast<const char *>(&status),2);
            }
            const bool same_reads=std::getenv("HALO2_VERIFY_MATRIX_LEAVES") || original.reads==native.reads;
            if(expected!=actual || original.bytes!=native.bytes || !same_reads) {
                std::cerr<<"broadphase bounds mismatch entry="<<entry<<" sample="<<sample
                    <<" gp="<<(expected.gp==actual.gp)<<" flags="<<(expected.flags==actual.flags)
                    <<" xmm="<<(expected.xmm==actual.xmm)<<" mxcsr="<<(expected.mxcsr==actual.mxcsr)
                    <<" stores="<<(expected.stores==actual.stores)<<" memory="<<(original.bytes==native.bytes)
                    <<" reads="<<same_reads<<" fp="<<std::hex<<expected.fp_status<<'/'<<actual.fp_status<<std::dec<<'\n';
                return 1;
            }
            ++cases;
        }
        std::cout<<"{\"passed\":true,\"cases\":"<<cases<<",\"entries\":[\"2EA050\",\"2EB0C0\"]}\n";
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
