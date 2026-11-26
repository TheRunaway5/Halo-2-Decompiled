#pragma once

#include "physics/runtime/native_tu15_mopp_ray_scale_tail.hpp"

namespace halo2::native_mopp_ray_split_bytes_detail {

inline bool reference_split_bytes() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_SPLIT_BYTES");
        return (value&&*value) || native_mopp_ray_traversal_detail::reference() ||
            native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline bool reference_diagonal_split_bytes() noexcept {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_MOPP_RAY_DIAGONAL_SPLIT_BYTES");
        return value && *value;
    }();
    return reference;
}

template<bool Diagonal=false,class State,class Spill>
std::uint32_t split_bytes(State &s,Spill spill) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference_split_bytes() || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       s.observe || s.fp.count>6u || !native_raw_x87_detail::supported(s))return 0;
    if constexpr(Diagonal)
        if(reference_diagonal_split_bytes())return 0;
    const auto *cursor=native_mopp_ray_scale_tail_detail::admitted_cursor(s);
    if(s.services.instruction ? !cursor : s.instruction_cursor!=nullptr)return 0;
    using namespace physics_step_detail;
    auto &g=s.gp;
    const auto read=[&](std::uint32_t address,auto &value) {
        return native_raw_x87_detail::direct_read(s.memory,address,std::as_writable_bytes(std::span{&value,1}));
    };
    const auto publish=[&](std::uint32_t pc) {if(cursor)*cursor->position=pc;};
    const auto store=[&](std::uint32_t pc,unsigned offset,std::uint32_t value,bool pop,unsigned expected_depth) {
        publish(pc);
        const auto address=g[4]+offset;
        const bool direct=s.memory.try_callback_free_write(address,std::as_bytes(std::span{&value,1}));
        if(!direct)w(s.memory,s.observe,pc,address,4u,value);
        if(pop)s.fp.pop();
        return direct || (s.fp.count==expected_depth && native_raw_x87_detail::supported(s) &&
            native_mopp_ray_scale_tail_detail::provider_continuation(s,cursor));
    };

    const auto depth=s.fp.count;
    std::uint8_t byte;
    std::uint32_t word;
    if constexpr(Diagonal) {
        if(!read(g[6]+1u,byte))return 0;
        g[0]=s.flags.minus(byte,0xaau,4);
        g[2]=g[0]*3u;
        if(!read(g[6]+2u,byte))return 0x30acd2u;
        g[0]=byte;
        if(!store(0x30acd6u,0x14u,g[2],false,depth))return 0x30acdau;
        g[0]=s.flags.minus(g[0],0xaau,4)*3u;
        if(!read(g[4]+0x14u,word))return 0x30ace2u;
        s.fp.push(static_cast<long double>(si(word)));
        if(!store(0x30ace6u,0x14u,g[0],false,depth+1u))return 0x30aceau;
        publish(0x30aceau);
        if(!store(0x30aceau,0x20u,spill(s.fp.st(0),s.fp.status),true,depth))return 0x30aceeu;
        if(!read(g[4]+0x14u,word))return 0x30aceeu;
        s.fp.push(static_cast<long double>(si(word)));
        publish(0x30acf2u);
        if(!store(0x30acf2u,0x1cu,spill(s.fp.st(0),s.fp.status),true,depth))return 0x30acf6u;

        constexpr std::array load_sites{0x30acf6u,0x30acfeu};
        constexpr std::array sub_sites{0x30acf8u,0x30acfbu,0x30ad01u,0x30ad04u};
        for(unsigned endpoint=0;endpoint<2u;++endpoint) {
            const auto address=g[7]+16u*endpoint;
            if(!read(address,word))return load_sites[endpoint];
            s.fp.push(ext(f(word)));
            for(unsigned axis=0;axis<2u;++axis) {
                if(!read(address+4u*(axis+1u),word))return sub_sites[2u*endpoint+axis];
                s.fp.st(0)=sub(s.fp.st(0),ext(f(word)));
            }
        }
        publish(0x30ad07u);
        return 0x30adc9u;
    }
    if(!read(g[6]+1u,byte))return 0;
    g[1]=byte;
    if(!read(g[6]+2u,byte))return 0x30ad10u;
    g[2]=byte;
    if(!store(0x30ad14u,0x14u,g[1],false,depth))return 0x30ad18u;
    g[6]=s.flags.plus(g[6],3u,4);
    g[0]=s.flags.plus(g[0],0xffffffe0u,4);
    if(!read(g[4]+0x14u,word))return 0x30ad1eu;
    s.fp.push(static_cast<long double>(si(word)));
    g[1]=s.flags.logic(0,4);
    if(!store(0x30ad24u,0x3cu,g[0],false,depth+1u))return 0x30ad28u;
    if(!store(0x30ad28u,0x24u,g[1],false,depth+1u))return 0x30ad2cu;
    publish(0x30ad2cu);
    if(!store(0x30ad2cu,0x1cu,spill(s.fp.st(0),s.fp.status),false,depth+1u))return 0x30ad30u;
    if(!store(0x30ad30u,0x14u,g[2],false,depth+1u))return 0x30ad34u;
    if(!read(0x46a0d0u,word))return 0x30ad34u;
    s.fp.st(0)=add(s.fp.st(0),ext(f(word)));
    publish(0x30ad3au);
    if(!store(0x30ad3au,0x20u,spill(s.fp.st(0),s.fp.status),true,depth))return 0x30ad3eu;
    if(!read(g[7]+g[0]*4u,word))return 0x30ad3eu;
    s.fp.push(ext(f(word)));
    if(!read(g[7]+16u+g[0]*4u,word))return 0x30ad41u;
    s.fp.push(ext(f(word)));
    publish(0x30ad45u);
    return 0x30ade0u;
#else
    (void)s;(void)spill;
    return 0;
#endif
}

} // namespace halo2::native_mopp_ray_split_bytes_detail
