#pragma once
#include "native_tu15_effect_slot_scan.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include <optional>

namespace halo2::native_effect_transform_product_detail {
inline bool reference_mode(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_EFFECT_TRANSFORM_PRODUCT");return p&&*p;}();
    return enabled||native_tu15_leaves::reference_mode();
}
struct UnchangedFlags {};
using Machine=native_tu15_leaves::Machine<UnchangedFlags>;
struct SseEnvironment {
    SourcePhysicsWorldRuntimeAbi&abi;unsigned host;
    explicit SseEnvironment(SourcePhysicsWorldRuntimeAbi&a):abi(a),host(_mm_getcsr()){
        _mm_setcsr(abi.floating_point->mxcsr);
    }
    ~SseEnvironment(){abi.floating_point->mxcsr=_mm_getcsr();_mm_setcsr(host);}
};
// Only transport the complete retained ABI at observable boundaries. The shared
// matrix kernel owns the row composition, aliases and exact 4/8-byte accesses.
struct Access {
    SourcePhysicsContactMemory&memory;SourcePhysicsWorldRuntimeAbi&abi;
    const SourcePhysicsContactRecordObserver&observe;Machine&machine;
    std::uint32_t*position;std::optional<SseEnvironment>environment;
    void publish(){
        abi.gp=machine.gp;
        for(unsigned n=0;n<8;++n)std::memcpy(abi.xmm[n].data(),&machine.x[n],16);
        if(environment)abi.floating_point->mxcsr=_mm_getcsr();
    }
    void at(std::uint32_t pc){publish();if(position)*position=pc;}
    void begin_arithmetic(std::uint32_t pc){at(pc);environment.emplace(abi);}
    std::uint32_t rd(std::uint32_t pc,std::uint32_t address,unsigned bytes){
        at(pc);std::array<std::byte,4>storage{};memory.read(address,std::span(storage).first(bytes));
        std::uint32_t value{};for(unsigned n=0;n<bytes;++n)value|=std::to_integer<unsigned>(storage[n])<<(8u*n);
        return value;
    }
    __m128 half(std::uint32_t pc,std::uint32_t address,__m128 value,unsigned offset){
        at(pc);std::array<std::byte,8>storage{};memory.read(address,storage);
        std::memcpy(reinterpret_cast<std::byte*>(&value)+offset,storage.data(),8);return value;
    }
    void st(std::uint32_t pc,std::uint32_t address,unsigned bytes,std::uint64_t value){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(bytes)};
        if(observe){event.before.resize(bytes);memory.read(address,event.before);}
        for(unsigned n=0;n<bytes;++n)event.written[n]=std::byte(value>>(8u*n));
        memory.write(address,event.written);if(observe)observe(event);
    }
};
inline bool try_native(SourcePhysicsContactMemory&memory,SourcePhysicsWorldRuntimeAbi&abi,
    const SourcePhysicsWorldRuntimeServices&services,const SourcePhysicsContactRecordObserver&observe){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference_mode()||native_tu15_leaves::verify_mode()||abi.active_mmx||!abi.floating_point||
       (abi.floating_point->mxcsr&0x1f80u)!=0x1f80u||(abi.floating_point->mxcsr&~0xffffu)||
       !native_effect_slot_scan_detail::admitted_protocol(services,observe))return false;
    Machine machine{};machine.gp=abi.gp;
    for(unsigned n=0;n<8;++n)std::memcpy(&machine.x[n],abi.xmm[n].data(),16);
    Access access{memory,abi,observe,machine,services.instruction?services.instruction_cursor.position:nullptr,{}};
    try{native_tu15_leaves::kernel_145560(access,machine);}
    catch(...){access.publish();throw;}
    access.publish();return true;
#endif
}
} // namespace halo2::native_effect_transform_product_detail
