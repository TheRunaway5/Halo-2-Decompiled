#include "halo2/mcc_sound_duration_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
namespace {
constexpr std::uint32_t pcs[]{
#include "audio/mcc_sound_duration_pcs.inc"
};
void require(bool value,const char* message) {
    if(!value)throw FormatError(message);
}
bool overlaps(std::uint32_t address,std::size_t bytes,std::uint32_t other,unsigned count) {
    return std::uint64_t(address)<std::uint64_t(other)+count &&
        std::uint64_t(other)<std::uint64_t(address)+bytes;
}
struct Projection final:SourcePhysicsContactMemory {
    SourcePhysicsContactMemory &bank;
    const MccSoundTimingEntry &entry;
    const MccSoundOriginalTimingOperands &original;
    mutable unsigned guards{},codecs{},sizes{},returns{};
    unsigned instruction{},stores{};
    std::uint32_t pc{};
    Projection(SourcePhysicsContactMemory &bank,const MccSoundTimingEntry &entry,
        const MccSoundOriginalTimingOperands &original):bank(bank),entry(entry),original(original){}
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        const std::array values{original.sample_rate,original.encoding,original.compression};
        if(!instruction) {
            require(guards<3 && address==entry.esi+3u+guards && out.size()==1,
                "MCC duration guard read differs from the complete leaf");
            out[0]=std::byte(values[guards++]);return;
        }
        if(overlaps(address,out.size(),entry.esi+3u,3)) {
            constexpr std::uint32_t at_pc[]{0x21f5e6u,0x21f618u,0x21f623u};
            constexpr unsigned offsets[]{2,1,0};
            require(codecs<3 && pc==at_pc[codecs] && out.size()==1 &&
                address==entry.esi+3u+offsets[codecs],
                "MCC duration codec read has a foreign phase or extent");
            out[0]=std::byte(values[offsets[codecs++]]);return;
        }
        if(overlaps(address,out.size(),entry.esp+8u,4)) {
            require(!sizes && pc==0x21f627u && address==entry.esp+8u && out.size()==4,
                "MCC duration size read has a foreign phase or extent");
            for(unsigned i=0;i<4;++i)out[i]=std::byte(original.encoded_bytes>>(8*i));
            ++sizes;return;
        }
        if(pc==0x21f660u) {
            require(!returns && address==entry.esp && out.size()==4,
                "MCC duration return read has a foreign extent");
            bank.read(address,out);
            require(Bytes(out).u32(0)==entry.return_address,
                "MCC duration original return word changed during execution");
            ++returns;return;
        }
        bank.read(address,out);
    }
    void write(std::uint32_t address,std::span<const std::byte> bytes)override {
        constexpr std::uint32_t at_pc[]{0x21f5a1u,0x21f5c0u,0x21f5feu};
        const std::array at{entry.esp-4u,entry.esp+4u,entry.esp+4u};
        require(stores<3 && pc==at_pc[stores] && address==at[stores] && bytes.size()==4,
            "MCC duration leaf attempted a foreign source write");
        bank.write(address,bytes);++stores;
    }
    void next(std::uint32_t next_pc) {
        require(guards==3 && instruction<std::size(pcs) && next_pc==pcs[instruction],
            "MCC duration instruction sequence differs from the reviewed leaf");
        ++instruction;pc=next_pc;
    }
    void complete()const {
        require(guards==3 && codecs==3 && sizes==1 && returns==1 && stores==3 && instruction==std::size(pcs),
            "MCC duration leaf omitted an original read, write or instruction");
    }
};
}
void run_mcc_sound_duration_read_projection(SourcePhysicsContactMemory &bank,
    const MccSoundTimingEntry &entry,const MccSoundOriginalTimingOperands &original,
    SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe) {
    require(entry.pc==0x21f590u && entry.return_address==0x21f6f8u &&
        entry.stack[0]==entry.return_address && entry.stack[1]==entry.edi && entry.stack[2]==entry.eax &&
        entry.esp>=4u && entry.esp<=0xfffffff3u && entry.esi<=0xffffffeCu &&
        original.sample_rate<3 && original.encoding<3 && original.compression<5 && original.encoded_bytes,
        "MCC duration projection has invalid original operands or caller");
    require(abi.gp==std::array{entry.eax,entry.ecx,entry.edx,entry.ebx,entry.esp,
        abi.gp[5],entry.esi,entry.edi},"MCC duration projection lost its real entry registers");
    require(!overlaps(entry.esp-4u,16,entry.esi,20),"MCC duration stack aliases its sound root");
    std::array<std::byte,3> codec_before;bank.read(entry.esi+3u,codec_before);
    std::array<std::byte,12> stack_before;bank.read(entry.esp,stack_before);
    const Bytes stack(stack_before);
    for(unsigned i=0;i<3;++i)require(stack.u32(i*4)==entry.stack[i],"MCC duration entry stack changed");
    Projection projected(bank,entry,original);
    auto copied_services=services;
    copied_services.instruction=[&](std::uint32_t pc) {
        projected.next(pc);if(services.instruction)services.instruction(pc);
    };
    source_run_sound_duration_tu15(projected,abi,fp,copied_services,observe);
    projected.complete();
    require(abi.gp[4]==entry.esp+12u && abi.gp[7]==entry.edi,
        "MCC duration leaf lost its original return or saved register");
    std::array<std::byte,3> codec_after;bank.read(entry.esi+3u,codec_after);
    std::array<std::byte,4> size_after;bank.read(entry.esp+8u,size_after);
    require(codec_after==codec_before && std::ranges::equal(size_after,std::span(stack_before).subspan(8,4)),
        "MCC duration projection changed actual codec or encoded argument bytes");
}
} // namespace halo2
