#pragma once
#include "halo2/native_tu15_object_ratio_caller_extended_kernel.hpp"
#include "gameplay/objects/native_tu15_object_ratio_caller_middle_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_ratio_caller_detail {
template<class State,class Before> class ExtendedFrame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::store;using Base::push;
    Before& before_;
    native_raw_x87_detail::Scope<State>& fp_;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {at(pc);return word(address,width);}
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(pc,address))));
    }
    bool greater_float()const {return !s_.flags.cf&&!s_.flags.zf;}
    void keep_maximum() {
        s_.flags.scalar_compare(s_.xmm[2],s_.xmm[0],false);
        if(!greater_float())s_.xmm[2]=s_.xmm[0];
    }
    std::uint32_t definition_index(std::uint32_t value) {
        std::uint32_t result;
        asm volatile("shll $4,%%ecx; movl %%ecx,%%edx; seto %%cl; lahf; movb %%ah,%%ch"
            :"+c"(value),"=d"(result)::"eax","cc");
        s_.flags.load(((value>>8u)&0xd5u)|((value&1u)<<11u));return result;
    }
public:
    ExtendedFrame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},before_(before),fp_(fp) {}

    bool allow_extended() {
        auto& g=s_.gp;
        g[2]=read(0x104747u,0x4eabc4u);
        s_.set(1,read(0x10474du,g[2]+0x82u,1),1,0);
        s_.flags.logic(s_.reg(1,1,0),1);return s_.flags.zf;
    }
    void reload_definition() {
        auto& g=s_.gp;
        g[1]=read(0x10475bu,g[5]);
        g[1]=definition_index(s_.flags.logic(g[1]&0xffffu,4));
        g[3]=read(0x104767u,g[1]+g[0]+8u);
        store(0x10476bu,g[4]+0x10u,g[3],[]{});
    }
    ExtendedStage locate_initial_scan() {
        auto& g=s_.gp;
        g[2]=read(0x10476fu,g[3]+0x2d0u);
        g[6]=s_.flags.logic(0u,4);
        s_.flags.minus(g[2],4u,4);
        if(s_.flags.sf==s_.flags.of)return ExtendedStage::vector_maximum;
        s_.flags.minus(g[6],g[2],4);
        if(s_.flags.sf!=s_.flags.of)return ExtendedStage::scalar_maximum;
        return ExtendedStage::scan_records;
    }
    bool begin_record_scan() {
        auto& g=s_.gp;
        g[3]=read(0x1047e3u,g[3]+0x2c8u);
        g[7]=s_.flags.logic(0u,4);s_.flags.logic(g[3],4);
        store(0x1047edu,g[4]+0x1cu,g[3],[]{});
        if(s_.flags.zf||s_.flags.sf!=s_.flags.of)return false;
        g[1]=read(0x1047f3u,g[4]+0x10u);
        g[6]=read(0x1047f7u,g[1]+0x2ccu);
        g[6]=s_.flags.plus(g[6],0x28u,4);g[3]=g[5]+0x20cu;
        return true;
    }
    bool has_positive_weight() {
        s_.xmm[0]=scalar(0x104806u,s_.gp[6]-0xcu);
        s_.flags.scalar_compare(s_.xmm[0],s_.xmm[3],false);return greater_float();
    }
    void sample_record() {
        auto& g=s_.gp;
        g[0]=read(0x104810u,g[4]+0x18u);
        push(0x104814u,g[7]);push(0x104815u,0x10481au);
        native_object_record_ratio_detail::Frame<State,Before> child{s_,before_,fp_};
        native_object_record_ratio_detail::query(child);
        const auto scale=scalar(0x10481au,g[6]);
        asm volatile("mulss %1,%0":"+x"(s_.xmm[0]):"x"(scale));
        keep_maximum();
    }
    bool has_secondary_term() {
        s_.flags.minus(read(0x104826u,s_.gp[3],1),2u,1);return s_.flags.zf;
    }
    void add_secondary_term() {
        auto& g=s_.gp;
        s_.xmm[0]=s_.xmm[4];
        s_.xmm[0]=_mm_sub_ss(s_.xmm[0],scalar(0x10482eu,g[6]));
        const auto scale=scalar(0x104832u,g[5]+0x188u);
        asm volatile("mulss %1,%0":"+x"(s_.xmm[0]):"x"(scale));
        const auto weight=scalar(0x10483au,g[6]);
        asm volatile("addss %1,%0":"+x"(s_.xmm[0]):"x"(weight));
        keep_maximum();
    }
    bool advance_record() {
        auto& g=s_.gp;
        g[0]=read(0x104846u,g[4]+0x1cu);
        g[7]=s_.flags.increment(g[7],4,false);
        g[6]=s_.flags.plus(g[6],0x40u,4);g[3]=s_.flags.plus(g[3],0xcu,4);
        s_.flags.minus(g[7],g[0],4);return s_.flags.sf!=s_.flags.of;
    }
    void include_object_term() {
        auto& g=s_.gp;
        g[2]=read(0x104855u,g[4]+0x10u);
        s_.xmm[0]=scalar(0x104859u,g[5]+0x180u);
        const auto weight=scalar(0x104861u,g[2]+0x164u);
        asm volatile("mulss %1,%0":"+x"(s_.xmm[0]):"x"(weight));
        keep_maximum();
    }
    void publish_continuation(ExtendedStage stage) {at(static_cast<std::uint32_t>(stage));}
};

template<class State,class Before> void run_extended_record_path(State& state,Before before) {
    native_raw_x87_detail::Scope<State> fp{state};
    Setup<State,Before> setup{state,before,fp};setup.open_object();
    DispatchFrame<State,Before> entry{state,before,fp};
    const auto route=select_initial_route(entry);
    if(route!=Route::extended){entry.publish_continuation(route);return;}
    ExtendedFrame<State,Before> path{state,before,fp};
    const auto next=prepare_extended(path);
    MiddleFrame<State,Before> exit{state,before,fp};
    if(next==ExtendedStage::finish){exit.finish_result();return;}
    if(next!=ExtendedStage::scan_records){path.publish_continuation(next);return;}
    scan_extended_records(path);exit.clamp_result();exit.finish_result();
}
} // namespace halo2::native_object_ratio_caller_detail
#endif
