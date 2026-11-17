#pragma once
#include "halo2/native_tu15_codec_sparse_decode_kernel.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_row_copy_frame.hpp"
#include "physics/runtime/native_tu15_codec_scale_frame.hpp"
#include "physics/runtime/native_tu15_codec_scale_row_frame.hpp"
#include "physics/runtime/native_tu15_codec_translation_frame.hpp"
#include "physics/runtime/native_tu15_packed_rotation_frame.hpp"

namespace halo2::native_codec_sparse_decode_detail {
using native_codec_rotation_interpolation_detail::Noop;
using native_codec_rotation_interpolation_detail::Io;

template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::before_;
    Scope& scope_;

    static std::uint32_t site(unsigned lane,std::uint32_t offset) {
        return 0x284d70u+lane*0x80u+offset;
    }
    static unsigned track(unsigned lane){return lane==0?6u:7u;}
    static unsigned remaining(unsigned lane){return lane==0?7u:6u;}
    std::uint32_t load(std::uint32_t instruction,std::uint32_t address,unsigned width=4u) {
        at(instruction);return word(address,width);
    }
    void store(std::uint32_t instruction,std::uint32_t address,std::uint32_t value) {
        Base::store(instruction,address,value,[]{});
    }
    bool signed_less()const{return s_.flags.sf!=s_.flags.of;}

    void child(std::uint32_t target) {
        namespace rows=native_codec_row_copy_detail;
        switch(target) {
        case 0x296180u: {
            native_packed_rotation_detail::Frame view{s_,before_,scope_};
            native_packed_rotation_detail::decode(view);
            return;
        }
        case 0x2961f0u: {
            native_codec_translation_detail::Frame view{s_,before_,scope_};
            native_codec_translation_detail::decode(view);
            return;
        }
        case 0x296220u: {
            native_codec_scale_detail::Frame view{s_,before_,scope_};
            native_codec_scale_detail::decode(view);
            return;
        }
        case 0x2963d0u: {
            rows::Frame<rows::Row::quaternion,State,Before> view{s_,before_,scope_};
            rows::decode(view);
            return;
        }
        case 0x296410u: {
            rows::Frame<rows::Row::translation,State,Before> view{s_,before_,scope_};
            rows::decode(view);
            return;
        }
        case 0x2cec60u: {
            native_codec_scale_row_detail::Frame view{s_,before_,scope_};
            native_codec_scale_detail::decode(view);
            return;
        }
        default:
            throw std::logic_error("Unqualified TU1.5 sparse decoder child target");
        }
    }

public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}},scope_(scope){}

    void begin() {
        auto& gp=s_.gp;
        Base::push(0x284d30u,gp[5]);
        gp[5]=gp[4];
        gp[4]=s_.flags.logic(gp[4]&0xfffffff0u,4u);
        gp[4]=s_.flags.minus(gp[4],20u,4u);
        gp[1]=load(0x284d39u,0x510f3cu);
        gp[0]=load(0x284d3fu,0x510f7cu);
        gp[2]=load(0x284d44u,0x510f70u);
        Base::push(0x284d4au,gp[3]);
        Base::push(0x284d4bu,gp[6]);
        gp[6]=s_.flags.logic(0u,4u);
        s_.flags.minus(gp[1],gp[6],4u);
        Base::push(0x284d50u,gp[7]);
        store(0x284d51u,gp[4]+28u,gp[1]);
        store(0x284d55u,0x510f94u,gp[6]);
        store(0x284d5bu,0x510f98u,gp[6]);
        store(0x284d61u,0x510f9cu,gp[6]);
        store(0x284d67u,0x510fa0u,gp[0]);
    }
    void begin_lane(unsigned lane) {
        auto& gp=s_.gp;
        if(lane==1) {
            gp[2]=load(0x284dd6u,0x510f7cu);
            gp[7]=s_.flags.logic(0u,4u);
            s_.flags.logic(gp[1],4u);
            store(0x284de0u,0x510fa0u,gp[2]);
            gp[2]=load(0x284de6u,0x510f74u);
        } else {
            gp[0]=load(0x284e56u,0x510f7cu);
            gp[2]=load(0x284e5bu,0x510f78u);
            gp[7]=s_.flags.logic(0u,4u);
            s_.flags.logic(gp[1],4u);
            store(0x284e65u,0x510fa0u,gp[0]);
        }
    }
    bool has_tracks()const{return !s_.flags.zf&&!signed_less();}

    bool load_bitmap(unsigned lane) {
        auto& gp=s_.gp;
        gp[3]=load(site(lane,0u),gp[2],1u);
        gp[2]=s_.flags.increment(gp[2],4u,false);
        s_.flags.logic(gp[3],4u);
        store(site(lane,6u),gp[4]+24u,gp[2]);
        return !s_.flags.zf;
    }
    void skip_bitmap(unsigned lane) {
        auto& gp=s_.gp;
        gp[0]=load(site(lane,0xcu),0x510fa0u);
        gp[track(lane)]=s_.flags.plus(gp[track(lane)],8u,4u);
        gp[0]=s_.flags.plus(gp[0],256u,4u);
        store(site(lane,0x19u),0x510fa0u,gp[0]);
    }
    bool begin_bits(unsigned lane) {
        auto& gp=s_.gp;
        auto& index=gp[track(lane)];
        gp[0]=index+8u;
        s_.flags.minus(gp[0],gp[1],4u);
        if(!s_.flags.zf&&!signed_less())gp[0]=gp[1];
        s_.flags.minus(index,gp[0],4u);
        if(!signed_less())return false;
        gp[0]=s_.flags.minus(gp[0],index,4u);
        gp[remaining(lane)]=gp[0];
        index=s_.flags.plus(index,gp[0],4u);
        return true;
    }
    bool bit_set(){s_.flags.logic(s_.gp[3]&1u,1u);return !s_.flags.zf;}

    void decode_track(unsigned lane) {
        auto& gp=s_.gp;
        const auto call=site(lane,0x37u);
        const auto returned=site(lane,0x3du);
        const auto target=load(call,0x510f64u+lane*4u);
        const auto stack_slot=gp[4]-4u;
        Base::store(call,stack_slot,returned,[&]{gp[4]=stack_slot;},true);
        scope_.child([&]{child(target);});

        gp[0]=load(returned,0x510f94u+lane*4u);
        gp[1]=load(site(lane,0x42u),gp[4]+28u);
        gp[0]=s_.flags.increment(gp[0],4u,false);
        store(site(lane,0x47u),0x510f94u+lane*4u,gp[0]);
    }
    void finish_bit(unsigned lane) {
        auto& gp=s_.gp;
        gp[0]=load(site(lane,0x4cu),0x510fa0u);
        gp[0]=s_.flags.plus(gp[0],32u,4u);
        gp[3]=s_.flags.right(gp[3],1u,4u,true);
        gp[remaining(lane)]=s_.flags.increment(gp[remaining(lane)],4u,true);
        store(site(lane,0x57u),0x510fa0u,gp[0]);
    }
    bool more_bits()const{return !s_.flags.zf;}
    void reload_bitmap(unsigned lane){auto& gp=s_.gp;gp[2]=load(site(lane,0x5eu),gp[4]+24u);}
    bool more_tracks(unsigned lane) {
        s_.flags.minus(s_.gp[track(lane)],s_.gp[1],4u);return signed_less();
    }
    void finish() {
        auto& gp=s_.gp;
        gp[7]=Base::pop(0x284ed6u);
        gp[6]=Base::pop(0x284ed7u);
        gp[3]=Base::pop(0x284ed8u);
        gp[4]=gp[5];
        gp[5]=Base::pop(0x284edbu);
        (void)Base::pop(0x284edcu);
    }
};

template<class State,class Before=Noop>void run_private(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};
    Frame view{state,before,scope};
    native_codec_sparse_decode_detail::decode(view);
}
} // namespace halo2::native_codec_sparse_decode_detail
#endif
