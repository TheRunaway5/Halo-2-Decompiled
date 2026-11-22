#pragma once

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_simplex_candidate_abi.hpp"
#endif
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_campaign_ai_path_edge_ring {
inline constexpr std::uint32_t stack_base=0x60000000u;
inline constexpr std::uint32_t workspace_bytes=0xcbcu, call_depth=0x44u;
inline constexpr std::uint32_t local_store_begin=0x28u;
inline constexpr std::uint32_t row_base=0xbcu, row_count=64u, row_bytes=0x30u;
static_assert(row_base + row_count*row_bytes == workspace_bytes);

// This one writable view proves that locals, arguments and the row array
// have distinct offsets in the same committed backing. The parent return
// begins at workspace_bytes and lies outside every admitted store.
inline bool owns_frame(const SourcePhysicsContactMemory &memory,
                       std::uint32_t sp,std::uint32_t limit) noexcept {
    if(sp<stack_base || (sp&3u) || std::uint64_t(sp)+workspace_bytes+4u>limit)return false;
    const auto frame=memory.try_write_extent(sp,workspace_bytes+4u);
    if(!frame)return false;
    std::uint32_t return_site{},rows{};
    std::memcpy(&return_site,frame->data()+0x44u,4u);
    std::memcpy(&rows,frame->data()+0x50u,4u);
    return return_site==0x27b44cu && rows==sp+row_base;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// This continuation starts inside the generated 27BD30. The callers' frame
// and the generated 27C3E7 epilogue still own all actual saves and restores.
// It does not admit the separately observed row-64 publication.
template<class State> class BoundaryRing {
    using Word = std::uint32_t;
    enum Register { eax, ecx, edx, ebx, esp, ebp, esi, edi };
    State &s;
    std::array<Word,8> &g;
    Word &cursor;

    Word read(Word pc, Word address, unsigned width=4) {
        cursor=pc;
        return physics_step_detail::r(s.memory,address,width);
    }
    void store(Word pc, Word address, unsigned width, Word value) {
        cursor=pc;
        physics_step_detail::w(s.memory,s.observe,pc,address,width,value);
    }
    Word slot(unsigned offset) const { return g[esp]+offset; }
    void difference(Word load_pc, Word sub_pc, Word store_pc, unsigned component) {
        using physics_step_detail::f;
        s.xmm[0]=_mm_set_ss(f(read(load_pc,g[esi]+component)));
        const auto first=_mm_set_ss(f(read(sub_pc,g[edx]+component)));
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        // The original scalar destination owns NaN priority and upper lanes.
        asm volatile("subss %1,%0":"+x"(s.xmm[0]):"x"(first):"memory");
#else
        s.xmm[0]=_mm_sub_ss(s.xmm[0],first);
#endif
        store(store_pc,g[eax]+0x20u+component,4,physics_step_detail::u(_mm_cvtss_f32(s.xmm[0])));
    }
    void append_row() {
        g[edi]=read(0x27c320u,slot(0x50));
        const auto row=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(g[ebp])));
        // The row store can fault after SHL and before ADD replaces its flags.
        // Capture the immediate-four hardware encoding, including undefined AF.
        g[eax]=native_simplex_candidate_detail::shift4(s,std::uint32_t(row)*3u);
        store(0x27c32du,g[eax]+g[edi]+0xcu,4,g[esi]);
        g[esi]=read(0x27c331u,slot(0x10));
        g[eax]=s.flags.plus(g[eax],g[edi],4);
        store(0x27c337u,g[eax],4,g[edx]);
        store(0x27c339u,g[eax]+8u,2,0xffffu);
        s.set(edx,read(0x27c33fu,g[esi]+g[edx]*8u,2),2);
        store(0x27c343u,g[eax]+4u,2,s.reg(edx,2));
        g[edx]=s.flags.logic(0,4);
        s.set(edx,read(0x27c349u,g[ecx]+4u,1),1);
        store(0x27c34cu,g[eax]+0x2du,1,0);
        store(0x27c350u,g[eax]+0x2eu,1,0);
        g[edx]=s.flags.right(g[edx],7,4,false);
        s.set(edx,s.flags.logic(s.reg(edx,1)&1u,1),1);
        store(0x27c35au,g[eax]+0x2cu,1,s.reg(edx,1));

        g[ebx]=read(0x27c35du,g[ebx]+0x2cu);
        g[edx]=read(0x27c360u,g[ecx],2);
        g[esi]=read(0x27c363u,g[ecx]+2u,2);
        g[esi]=g[ebx]+g[esi]*12u;
        g[edx]=g[ebx]+g[edx]*12u;
        store(0x27c373u,slot(0x54),4,g[esi]);
        g[esi]=g[eax]+0x10u;
        g[edi]=g[edx];
        g[ebx]=read(0x27c37cu,g[edi]);
        store(0x27c37eu,g[esi],4,g[ebx]);
        g[ebx]=read(0x27c380u,g[edi]+4u);
        store(0x27c383u,g[esi]+4u,4,g[ebx]);
        g[edi]=read(0x27c386u,g[edi]+8u);
        g[ebx]=read(0x27c389u,slot(0x48));
        store(0x27c38du,g[esi]+8u,4,g[edi]);
        s.set(esi,read(0x27c390u,slot(0x14),2),2);
        g[edi]=read(0x27c395u,slot(0x28));
        store(0x27c399u,g[eax]+0x1cu,2,s.reg(esi,2));
        g[esi]=read(0x27c39du,slot(0x54));
        difference(0x27c3a1u,0x27c3a5u,0x27c3a9u,0);
        difference(0x27c3aeu,0x27c3b3u,0x27c3b8u,4);
        difference(0x27c3bdu,0x27c3c2u,0x27c3c7u,8);
        s.set(eax,read(0x27c3ccu,slot(0x4c),1),1);
        g[ebp]=s.flags.increment(g[ebp],4,false);
    }
public:
    BoundaryRing(State &state, Word &position):s(state),g(s.gp),cursor(position){}

    bool run() {
        if(g[ebp]>=0x40u)return false;
        g[edi]=read(0x27c2b2u,slot(0x10));
        g[eax]=read(0x27c2b6u,slot(0x18));
        s.flags.logic(read(0x27c2bau,g[edi]+g[eax]*8u,1)&1u,1);
        if(s.flags.zf) {
            s.flags.logic(s.reg(ebp,2),2);
            if(!s.flags.zf)return true;
            g[ecx]=read(0x27c2c9u,slot(0x4c));
            s.flags.minus(read(0x27c2cdu,g[ecx]+2u,2),0xffffu,2);
            if(!s.flags.zf)return true;
        }
        g[esi]=read(0x27c2d8u,g[edi]+g[eax]*8u+4u);
        g[edi]+=g[eax]*8u+4u;
        store(0x27c2e0u,slot(0x28),4,g[edi]);
        for(;;) {
            g[edx]=read(0x27c2e4u,g[ebx]+0xcu);
            g[eax]=read(0x27c2e7u,slot(0x18));
            g[ecx]=s.flags.shift(g[esi],4,4);
            g[ecx]=s.flags.plus(g[ecx],g[edx],4);
            g[edx]=read(0x27c2f2u,g[ecx]+0xeu,2);
            s.flags.minus(g[eax],g[edx],4);
            s.set(eax,s.flags.zf,1);
            g[edx]=s.flags.logic(0,4);
            s.flags.logic(s.reg(eax,1),1);
            s.set(edx,s.flags.zf,1);
            store(0x27c302u,slot(0x4c),1,s.reg(eax,1));
            g[edx]=read(0x27c306u,g[ecx]+0xcu+g[edx]*2u,2);
            s.flags.minus(g[edx],0xffffffffu,4);
            if(!s.flags.zf) {
                s.flags.minus(g[edx],0xffffu,4);
                if(!s.flags.zf)append_row();
            }
            s.flags.minus(s.reg(ebp,2),0x40u,2);
            if(s.flags.zf)return true;
            g[eax]=s.reg(eax,1);
            g[esi]=read(0x27c3dau,g[ecx]+8u+g[eax]*2u,2);
            s.flags.minus(g[esi],read(0x27c3dfu,g[edi]),4);
            if(s.flags.zf)return true;
        }
    }
};
#endif

// The build-local binder wraps only the actual CALL at 27B447. Other calls
// cannot enter the native suffix even with identical contents in the slots.
template<class State>
void call_from_search(State &s,void(*child)(State&)) {
    struct Caller {
        std::uint32_t &slot;
        const std::uint32_t prior;
        ~Caller() {slot=prior;}
    } caller{s.path_edge_ring_caller_sp,s.path_edge_ring_caller_sp};
    s.path_edge_ring_caller_sp=s.gp[4];
    child(s);
}

template<class State>
bool try_run(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto *cursor=s.immutable_cursor.cursor;
    const auto &run=s.services.run_ai_path_edge_ring;
    const auto sp=s.gp[4];
    if(!run || !cursor || s.image!=SourcePhysicsContactImage::TitleUpdate15 ||
       !s.path_edge_ring_caller_sp || s.path_edge_ring_caller_sp!=std::uint64_t(sp)+call_depth ||
       s.gp[5]>=row_count || s.active_mmx || s.observe ||
       (cursor->first_trace && *cursor->first_trace) ||
       (cursor->second_trace && *cursor->second_trace))return false;
    return run(s.services,s.memory,sp,&s,[](void *context,std::uint32_t &position) {
        BoundaryRing<State>(*static_cast<State*>(context),position).run();
    });
#else
    return false;
#endif
}
} // namespace halo2::native_campaign_ai_path_edge_ring
