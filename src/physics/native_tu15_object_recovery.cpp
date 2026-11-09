#include "halo2/native_tu15_object_recovery.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"

#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t p,unsigned width=4) {
    std::array<std::byte,4> bytes{};
    memory.read(p,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned i=0;i<width;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8u*i);
    return value;
}
void write(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<width;++i)bytes[i]=std::byte(value>>(8u*i));
    if(observe) {
        SourcePhysicsContactRecordStore event{pc,p,std::vector<std::byte>(width),
            {bytes.begin(),bytes.begin()+width}};
        memory.read(p,event.before);
        memory.write(p,std::span(bytes).first(width));
        observe(event);
    } else memory.write(p,std::span(bytes).first(width));
}
struct FlagState {
    SourcePhysicsWorldRuntimeAbi &abi;
    physics_contact_runtime_detail::Flags flags;
    explicit FlagState(SourcePhysicsWorldRuntimeAbi &abi):abi(abi){flags.load(abi.eflags);}
    ~FlagState(){abi.eflags=(abi.eflags&~0xcd5u)|flags.bits();}
};
}

void native_tu15_collect_object_recovery_candidates(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    auto &[eax,ecx,edx,ebx,esp,ebp,esi,edi]=abi.gp;
    if(call.target!=0xc1270u || call.instruction!=0xc14b8u || call.tail ||
        esp<0x600c0010u || esp>0x600fd5dcu || eax!=0xc1270u || edi!=0x44a670u ||
        read(memory,edi)!=0xc1270u || read(memory,edi+4u)!=0xc12f0u ||
        read(memory,esp)!=0xc14bau || read(memory,esp+4u)!=ebp || ebp>3u ||
        read(memory,esp+8u)!=esp+0x224u || ecx!=esp+0x224u || read(memory,esp+12u)!=0x2800u)
        throw std::runtime_error("C1270 requires its authenticated object recovery caller");
    const auto push=[&](auto pc,auto value) {
        esp-=4u;write(memory,observe,pc,esp,value);
    };
    const auto pop=[&]() {
        const auto value=read(memory,esp);esp+=4u;return value;
    };
    FlagState state(abi);auto &flags=state.flags;
    constexpr std::uint32_t none=0xffffffffu;

    eax=read(memory,0x4eabc4u);
    push(0xc1275u,esi);
    esi=read(memory,esp+12u);
    write(memory,observe,0xc127au,esi,0u);
    ecx=read(memory,eax+8u);
    flags.minus(ecx,none,4);
    if(!flags.zf) {
        push(0xc1288u,ebx);
        ebx=read(memory,0x4f3218u);
        push(0xc128fu,ebp);
        ebp=read(memory,0x4ecbd0u);
        push(0xc1296u,edi);
        do {
            edi=read(memory,esi);
            flags.minus(edi,0x800u,4);
            if(!flags.cf)break;
            eax=flags.logic(ecx&0xffffu,4);
            edx=eax+2u*eax;
            eax=read(memory,ebp+0x44u);
            eax=read(memory,eax+4u*edx+8u);
            bool append=true;
            flags.minus(read(memory,ebx+0xcu,1),4u,1);
            if(flags.zf) {
                flags.minus(read(memory,eax+0xd4u),none,4);
                edx=(edx&0xffffff00u)|std::uint32_t(!flags.zf);
                flags.logic(edx&0xffu,1);
                append=flags.zf;
            }
            if(append) {
                flags.minus(read(memory,eax+0x14u),none,4);
                if(flags.zf) {
                    write(memory,observe,0xc12d5u,esi+4u*edi+4u,ecx);
                    write(memory,observe,0xc12d9u,esi,flags.increment(read(memory,esi),4,false));
                }
            }
            ecx=read(memory,eax+0x1cu);
            flags.minus(ecx,none,4);
        } while(!flags.zf);
        edi=pop();ebp=pop();ebx=pop();
    }
    esi=pop();
    (void)pop();
    esp+=12u;
}

void native_tu15_finish_empty_object_recovery(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    auto &[eax,ecx,edx,ebx,esp,ebp,esi,edi]=abi.gp;
    if(call.target!=0xc12f0u || call.instruction!=0xc14deu || call.tail ||
        esp<0x600c0010u || esp>0x600fd5d0u || edi!=0x44a670u ||
        read(memory,edi)!=0xc1270u || read(memory,edi+4u)!=0xc12f0u ||
        read(memory,esp)!=0xc14e1u || read(memory,esp+4u)!=ebp || ebp>3u ||
        read(memory,esp+8u)!=esp+0x230u || ecx!=esp+0x230u ||
        read(memory,esp+12u)!=0x2800u || read(memory,esp+16u)!=esp+0x2eu ||
        eax!=esp+0x2eu || read(memory,esp+20u)!=esp+0x30u ||
        edx!=esp+0x30u || read(memory,esp+24u)!=0x200u)
        throw std::runtime_error("C12F0 requires its authenticated object recovery caller");
    FlagState state(abi);auto &flags=state.flags;
    esp-=4u;write(memory,observe,0xc12f0u,esp,ebp);
    esp-=4u;write(memory,observe,0xc12f1u,esp,edi);
    edi=read(memory,esp+0x10u);
    eax=read(memory,edi);
    ebp=flags.logic(0u,4);
    flags.logic(eax,4);
    if(!flags.zf && flags.sf==flags.of)
        throw std::runtime_error("C12F0 positive list requires native object eligibility and cleanup");
    eax=read(memory,edi);
    ecx=read(memory,esp+0x18u);
    flags.logic(eax,4);
    eax=(eax&0xffffff00u)|std::uint32_t(!flags.zf&&flags.sf==flags.of);
    write(memory,observe,0xc1386u,ecx,eax,1);
    edi=read(memory,esp);esp+=4u;
    eax=ebp;
    ebp=read(memory,esp);esp+=4u;
    (void)read(memory,esp);esp+=0x1cu;
}
}
