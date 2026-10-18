#include "halo2/source_sound_duration_tu15.hpp"
#include "halo2/mcc_sound_tag_layout.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
constexpr std::uint32_t sound=0x600280u;
struct GuardMemory final:SourcePhysicsContactMemory {
    std::array<std::byte,3> codec;
    mutable unsigned reads{};
    unsigned writes{};
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        ++reads;
        if(at<sound+3u || std::uint64_t(at)+out.size()>sound+6u)
            throw std::runtime_error("Duration guard read outside codec inputs");
        std::copy_n(codec.begin()+std::ptrdiff_t(at-sound-3u),out.size(),out.begin());
    }
    void write(std::uint32_t,std::span<const std::byte>)override {
        ++writes;throw std::runtime_error("Duration guard wrote source memory");
    }
};
void check(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
}
int main()try {
    unsigned cases{};
    const auto one=[&](unsigned rate,unsigned encoding,unsigned compression,unsigned reads) {
        GuardMemory memory;
        memory.codec={std::byte(rate),std::byte(encoding),std::byte(compression)};
        unsigned instructions{},stores{};
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto){++instructions;};
        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp={0x11223344u,0x22334455u,0x33445566u,0x44556677u,
            0x700000u,0x55667788u,sound,0x66778899u};
        for(unsigned i=0;i<abi.xmm.size();++i)
            for(unsigned j=0;j<abi.xmm[i].size();++j)abi.xmm[i][j]=0x3f800000u+i*16u+j;
        abi.eflags=0x2d7u;abi.fs_base=0x60200000u;
        abi.x87[0]=3.25L;abi.x87[1]=-7.5L;abi.x87_depth=2;abi.x87_status=0x3200u;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};
        const auto before=abi;
        bool rejected{};
        try {
            source_run_sound_duration_tu15(memory,abi,*abi.floating_point,services,
                [&](const auto&){++stores;});
        }catch(const MccSoundLayoutRequirement& e) {
            rejected=e.need()==MccSoundLayoutNeed::NativeCodecDuration;
        }
        check(rejected,"Unsupported duration codec did not reject at admission");
        check(memory.reads==reads && !memory.writes && !instructions && !stores,
            "Duration codec rejection entered or wrote the original body");
        check(abi.gp==before.gp && abi.xmm==before.xmm && abi.eflags==before.eflags &&
            abi.fs_base==before.fs_base && abi.x87_depth==before.x87_depth &&
            abi.x87_status==before.x87_status && !abi.active_mmx &&
            abi.floating_point->x87_control_word==before.floating_point->x87_control_word &&
            abi.floating_point->mxcsr==before.floating_point->mxcsr,
            "Duration codec rejection changed the caller ABI");
        for(unsigned i=0;i<abi.x87_depth;++i)
            check(std::memcmp(&abi.x87[i],&before.x87[i],10)==0,
                "Duration codec rejection changed a live x87 value");
        ++cases;
    };
    for(unsigned rate=3;rate<256;++rate)one(rate,0,0,1);
    for(unsigned encoding=3;encoding<256;++encoding)one(0,encoding,0,2);
    for(unsigned compression=5;compression<256;++compression)one(0,0,compression,3);
    one(3,0,5,1);one(3,1,5,1);one(3,3,5,1);one(255,255,255,1);
    std::cout<<"{\"codec_rejections_before_body_or_write\":"<<cases<<"}\n";
}catch(const std::exception& e) {
    std::cerr<<e.what()<<'\n';return 1;
}
