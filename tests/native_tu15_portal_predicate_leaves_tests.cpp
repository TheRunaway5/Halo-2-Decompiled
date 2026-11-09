#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_portal_predicate_leaves.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <random>

namespace halo2 {
using namespace physics_world_runtime_detail;
std::uint32_t sign_extend(std::uint32_t x,unsigned n){const auto shift=32u-8u*n;return std::uint32_t(std::int32_t(x<<shift)>>shift);}
#include "portal_predicate_leaf_bodies.inc"
}
namespace {
using namespace halo2;
struct Memory final : SourcePhysicsContactMemory {
    static constexpr std::uint32_t base=0x1000000u;
    std::vector<std::byte> bytes=std::vector<std::byte>(65536);
    std::map<std::uint32_t,std::byte> globals;
    mutable std::vector<std::pair<std::uint32_t,std::size_t>> reads;
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        reads.emplace_back(p,out.size());
        if(p>=base && p-base+out.size()<=bytes.size())std::memcpy(out.data(),bytes.data()+p-base,out.size());
        else for(unsigned i=0;i<out.size();++i)out[i]=globals.at(p+i);
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        if(p<base || p-base+in.size()>bytes.size())throw std::runtime_error("write outside fixture");
        std::memcpy(bytes.data()+p-base,in.data(),in.size());
    }
    void word(std::uint32_t p,std::uint32_t x,unsigned width=4) {
        if(p>=base && p-base+width<=bytes.size())std::memcpy(bytes.data()+p-base,&x,width);
        else for(unsigned i=0;i<width;++i)globals[p+i]=std::byte(x>>(i*8u));
    }
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
    const std::array<std::uint32_t,8> &gp,unsigned flags,unsigned mxcsr,unsigned depth,unsigned top,std::uint16_t control,float radius) {
    Result result;SourcePhysicsContactRuntimeServices services;
    SourcePhysicsContactRecordObserver observe=[&](const auto &store){result.stores.push_back(store);};
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    s.gp=gp;s.flags.load(flags);s.fp.count=depth;s.fp.top=top;s.fp.status=0x200u;
    for(unsigned i=0;i<depth;++i)s.fp.lanes[i]=static_cast<long double>(i+1u);
    for(unsigned i=0;i<8;++i)s.xmm[i]=_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));
    s.xmm[0]=_mm_set_ss(radius);
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
        std::mt19937 random(0x14cea0u);unsigned cases=0;
        const std::array<std::uint32_t,12> edges={0u,0x80000000u,0x3f800000u,0xbf800000u,
            1u,0x80000001u,0x7f7fffffu,0x00800000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u};
        constexpr std::array<unsigned,18> projection={2,1,0,1,2,0,0,2,1,2,0,1,1,0,2,0,1,2};
        for(unsigned entry=0;entry<3;++entry)for(unsigned sample=0;sample<768;++sample) {
            Memory original;for(auto &byte:original.bytes)byte=std::byte(random());
            const auto sp=Memory::base+0x9000u+sample%16u;
            const auto point=Memory::base+0x1000u,structure=Memory::base+0x2000u;
            const auto collision=Memory::base+0x2100u,planes=Memory::base+0x2200u;
            const auto global_collision=Memory::base+0x2300u,global_planes=Memory::base+0x2400u;
            const auto portal=Memory::base+0x3000u,vertices=Memory::base+0x4000u;
            for(unsigned i=0;i<projection.size();++i)original.word(0x44ac8cu+i*2u,projection[i],2);
            original.word(0x4ecc0cu,global_collision);original.word(global_collision+12u,global_planes);
            original.word(structure+24u,collision);original.word(collision+12u,planes);
            original.word(structure+96u,portal);
            const unsigned axis=sample%3u;const float sign=(sample&1u)?-1.0f:1.0f;
            const float radius=sample<96u?std::bit_cast<float>(edges[sample%edges.size()]):float(1u+sample%16u)/8.0f;
            for(unsigned i=0;i<4;++i) {
                original.real(planes+i*4u,i==axis?sign:0.0f);
                original.real(global_planes+i*4u,i==axis?sign:0.0f);
                if(i<3)original.real(point+i*4u,float(std::int32_t(sample%(7u+i))-3)/4.0f);
            }
            original.word(portal+4u,0);for(unsigned i=0;i<3;++i)original.real(portal+8u+i*4u,0.0f);
            original.real(portal+20u,4.0f);
            const unsigned count=sample%17u==0?0u:(sample%13u==0?128u:4u);
            original.word(portal+28u,count);original.word(portal+32u,vertices);
            for(unsigned i=0;i<128;++i)for(unsigned component=0;component<3;++component) {
                const auto corner=i%4u;
                const float value=component==axis?0.0f:((corner==0u||corner==3u)==(component==(axis+1u)%3u)?-1.0f:1.0f);
                original.real(vertices+i*12u+component*4u,value);
            }
            if(entry==0) {
                for(unsigned i=0;i<3;++i)original.word(point+i*4u,
                    sample<96u?edges[(sample+i)%edges.size()]:std::bit_cast<std::uint32_t>(float(std::int32_t(random()%10000u)-5000)/64.0f));
            }else if(entry==1) {
                const std::array<std::pair<float,float>,4> square={{{-1,-1},{1,-1},{1,1},{-1,1}}};
                for(unsigned i=0;i<4;++i){original.real(vertices+i*8u,square[i].first);original.real(vertices+i*8u+4u,square[i].second);}
                if(sample<96u)original.word(vertices+(sample%8u)*4u,edges[sample%edges.size()]);
                if(sample%11u==0)original.word(vertices+8u,0xbf800000u);
            }else if(sample%23u==0) {
                original.real(planes,0.707106769f);original.real(planes+4u,0.707106769f);
                original.real(planes+8u,1e-10f);original.real(point,1e9f);original.real(point+4u,-1e9f);
                original.real(point+8u,1.0f);original.real(portal+20u,2e9f);
            }
            original.word(sp,0x103f000u);original.word(sp+4u,entry==1?(sample%19u==0?0xffffu:4u):std::bit_cast<std::uint32_t>(radius));
            std::array<std::uint32_t,8> gp;for(auto &value:gp)value=random();gp[4]=sp;
            if(entry==0)gp[0]=(sample&4u)?sp-16u:point;
            if(entry==1){gp[1]=vertices;gp[7]=point;}
            if(entry==2){gp[0]&=0xffff0000u;gp[1]=point;gp[2]=structure;}
            const auto input_bytes=original.bytes;auto native=original;
            const unsigned flags=random()&0x8d5u,mxcsr=0x1f80u|((sample%4u)<<13u)|((sample&4u)?0x8040u:0u);
            const std::array<std::uint16_t,3> precisions={0x007fu,0x027fu,0x037fu};
            const std::uint16_t control=precisions[(sample/4u)%3u]|((sample%4u)<<10u);
            const auto depth=sample%3u,top=sample%8u;
            const auto expected=entry==0?evaluate(original,reference::run_123210,gp,flags,mxcsr,depth,top,control,radius):
                entry==1?evaluate(original,reference::run_243450,gp,flags,mxcsr,depth,top,control,radius):
                evaluate(original,reference::run_14cea0,gp,flags,mxcsr,depth,top,control,radius);
            const auto actual=entry==0?evaluate(native,production::run_123210,gp,flags,mxcsr,depth,top,control,radius):
                entry==1?evaluate(native,production::run_243450,gp,flags,mxcsr,depth,top,control,radius):
                evaluate(native,production::run_14cea0,gp,flags,mxcsr,depth,top,control,radius);
            if(argc==2 && sample<128u) {
                std::ofstream stream(std::string(argv[1])+"/hardware-"+std::to_string(entry)+".bin",std::ios::binary|std::ios::app);
                const auto write=[&](const auto &value){stream.write(reinterpret_cast<const char *>(&value),sizeof(value));};
                write(gp);write(flags);write(mxcsr);
                for(unsigned i=0;i<8;++i){const auto x=i==0?_mm_set_ss(radius):_mm_set_ps(float(i+4),float(i+3),float(i+2),float(i+1));write(x);}
                stream.write(reinterpret_cast<const char *>(input_bytes.data()),input_bytes.size());
                write(actual.gp);write(actual.flags);write(actual.mxcsr);write(actual.xmm);
                stream.write(reinterpret_cast<const char *>(native.bytes.data()),native.bytes.size());
                std::ofstream fp(std::string(argv[1])+"/hardware-fp-"+std::to_string(entry)+".bin",std::ios::binary|std::ios::app);
                fp.write(reinterpret_cast<const char *>(&control),2);const std::uint16_t status=actual.fp_status;
                fp.write(reinterpret_cast<const char *>(&status),2);
            }
            const bool same_reads=std::getenv("HALO2_VERIFY_MATRIX_LEAVES") || original.reads==native.reads;
            if(expected!=actual || original.bytes!=native.bytes || !same_reads) {
                std::cerr<<"portal predicate mismatch entry="<<entry<<" sample="<<sample
                    <<" gp="<<(expected.gp==actual.gp)<<" flags="<<(expected.flags==actual.flags)
                    <<" xmm="<<(expected.xmm==actual.xmm)<<" mxcsr="<<(expected.mxcsr==actual.mxcsr)
                    <<" stores="<<(expected.stores==actual.stores)<<" memory="<<(original.bytes==native.bytes)
                    <<" reads="<<same_reads<<" fp="<<std::hex<<expected.fp_status<<'/'<<actual.fp_status<<std::dec<<'\n';
                for(unsigned i=0;i<8;++i)if(expected.gp[i]!=actual.gp[i])std::cerr<<"gp"<<i<<' '<<std::hex<<expected.gp[i]<<'/'<<actual.gp[i]<<std::dec<<'\n';
                return 1;
            }
            ++cases;
        }
        std::cout<<"{\"passed\":true,\"cases\":"<<cases<<",\"entries\":[\"123210\",\"243450\",\"14CEA0\"]}\n";
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
