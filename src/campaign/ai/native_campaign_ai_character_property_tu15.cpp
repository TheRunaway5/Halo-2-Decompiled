#include "halo2/native_campaign_ai_character_property_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void write(SourcePhysicsContactMemory &memory,std::uint32_t address,std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));
    memory.write(address,bytes);
}
std::uint32_t count_for(std::uint32_t entry) {
    switch(entry) {
    case 0x1ecc40u:return 0x34u;
    case 0x1ecd00u:return 0x5cu;
    case 0x1ecd40u:return 0xb4u;
    }
    throw std::invalid_argument("Native character property requires 1ECC40, 1ECD00 or 1ECD40");
}
}
namespace {
std::uint32_t walk(const SourcePhysicsContactMemory &memory,std::uint32_t tags,std::uint32_t count_offset,std::uint32_t &ecx) {
    for(;;) {
        const auto tag=read(memory,(ecx&0xffffu)*16u+tags+8u);ecx=tag;
        if(std::bit_cast<std::int32_t>(read(memory,tag+count_offset))>0)return read(memory,tag+count_offset+4u);
        ecx=read(memory,tag+8u);
        if(ecx==0xffffffffu)return 0;
    }
}
// 1ED700: actor ECX selects a character; [ESP+4] selects an exact object
// definition in its +E4/+E8 records. Keep each tag's count and table pinned,
// including the original signed-low-word loop index after INC EDI.
void object_property_call(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi) {
    const auto sp=abi.gp[4];
    const auto pool=read(memory,0x5020c8u);
    auto edx=read(memory,pool+0x44u);
    auto ecx=read(memory,edx+(abi.gp[1]&0xffffu)*0x888u+0x54u);
    write(memory,sp-4u,abi.gp[5]); // PUSH EBP precedes the argument read even for NONE.
    const auto definition=read(memory,sp+4u);
    std::uint32_t result{};
    if(ecx!=0xffffffffu) {
        const auto tags=read(memory,0x4f0414u);
        write(memory,sp-8u,abi.gp[3]);
        write(memory,sp-12u,abi.gp[6]);
        write(memory,sp-16u,abi.gp[7]);
        for(;;) {
            edx=read(memory,tags+(ecx&0xffffu)*16u+8u);
            const auto count=std::bit_cast<std::int32_t>(read(memory,edx+0xe4u));
            if(count>0) {
                const auto records=read(memory,edx+0xe8u);
                std::int32_t index{};
                bool found{};
                do {
                    ecx=records+std::uint32_t(index)*0xb4u;
                    if(read(memory,ecx+4u)==definition){result=ecx;found=true;break;}
                    index=std::bit_cast<std::int16_t>(std::uint16_t(std::uint32_t(index)+1u));
                } while(index<count);
                if(found)break;
            }
            ecx=read(memory,edx+8u);
            if(ecx==0xffffffffu)break;
        }
    }
    abi.gp[0]=result;abi.gp[1]=ecx;abi.gp[2]=edx;
    abi.gp[4]=sp+8u; // RET 4; all four saved registers retain their entry words.
}
}
std::uint32_t native_campaign_ai_character_property_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t datum,std::uint32_t count_offset,std::uint32_t &ecx) {
    ecx=datum;
    if(datum==0xffffffffu)return 0;
    const auto tags=read(memory,0x4f0414u);
    return walk(memory,tags,count_offset,ecx);
}
void native_campaign_ai_character_property_call_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi) {
    if(entry==0x1ed700u){object_property_call(memory,abi);return;}
    const auto count=count_for(entry);
    const auto sp=abi.gp[4],datum=abi.gp[1];
    // XOR EAX,EAX; CMP ECX,-1; JE ret: no global read and no ESI save.
    if(datum==0xffffffffu){abi.gp[0]=0;abi.gp[4]=sp+4u;return;}
    // MOV EDX,[4F0414] precedes PUSH ESI.
    const auto tags=read(memory,0x4f0414u);
    write(memory,sp-4u,abi.gp[6]);
    std::uint32_t ecx=datum;
    abi.gp[0]=walk(memory,tags,count,ecx);
    abi.gp[1]=ecx;abi.gp[2]=tags;
    // POP ESI restores the caller's saved word; ESI is never modified.
    abi.gp[4]=sp+4u;
}
bool native_campaign_ai_character_property_tu15_owns_call(const SourcePhysicsContactRuntimeCall &call) {
    return call.target==0x1ecd40u && call.instruction==0x270696u && !call.tail;
}
void native_campaign_ai_character_property_tu15_run(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi) {
    if(!native_campaign_ai_character_property_tu15_owns_call(call))
        throw std::invalid_argument("Native 1ECD40 requires its actual 270696 call");
    if(read(memory,abi.gp[4])!=0x27069bu)throw std::runtime_error("Native 1ECD40 lost its actual 27069B return");
    native_campaign_ai_character_property_call_tu15(memory,0x1ecd40u,abi);
}
}
