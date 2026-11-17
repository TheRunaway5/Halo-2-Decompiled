#pragma once

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_model_secondary_data_frame.hpp"
#include "physics/runtime/native_tu15_codec_sparse_decode_frame.hpp"
#include "physics/runtime/native_tu15_codec_transform_publication_frame.hpp"

namespace halo2::native_codec_model_secondary_transform_detail {
using native_codec_model_first_selector_detail::Boundary;
using native_codec_model_first_selector_detail::Stop;
using native_codec_model_first_selector_detail::Noop;
using native_codec_model_first_selector_detail::Io;

template<class State,class Before>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_integer_fp_detail::Scope<State>>> {
    using Scope=native_integer_fp_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::at;using Base::word;using Base::before_;using Base::io;
    Scope& scope_;

    std::uint32_t load(std::uint32_t instruction,std::uint32_t address,unsigned width=4u) {
        at(instruction);return word(address,width);
    }
    std::uint32_t signed_half(std::uint32_t instruction,std::uint32_t address) {
        const auto raw=std::uint16_t(load(instruction,address,2u));
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(raw)));
    }
    std::uint32_t signed_byte(std::uint32_t instruction,std::uint32_t address) {
        const auto raw=std::uint8_t(load(instruction,address,1u));
        return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(raw)));
    }
    void store(std::uint32_t instruction,std::uint32_t address,std::uint32_t value) {
        Base::store(instruction,address,value,[]{});
    }
    void store_channel(std::uint32_t value) {
        constexpr std::uint32_t pc=0x283a31u,address=0x510f38u;
        at(pc);
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
        const std::array bytes{std::byte(value)};
        io([&]{s_.memory.write(address,bytes);});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
    void align_track_count() {
        auto& count=s_.gp[1];
        count=s_.flags.plus(count,31u,4u);
        count=s_.flags.right(count,3u,4u,true);
        count=s_.flags.logic(count&0xfffffffcu,4u);
    }

public:
    Frame(State& state,Before& before,Scope& scope):Base{state,before,{&scope}},scope_(scope){}

    void transform_if_selected() {
        auto& gp=s_.gp;
        const auto mode=load(0x283953u,0x510f30u);
        s_.flags.minus(mode,gp[3],4u);
        if(!s_.flags.zf)return;

        gp[1]=load(0x28395bu,gp[4]+24u);
        gp[2]=load(0x28395fu,gp[4]+28u);
        gp[0]=load(0x283963u,gp[4]+16u);
        Base::push(0x283967u,gp[5]);
        Base::push(0x283968u,gp[1]);
        Base::push(0x283969u,gp[2]);
        Base::push(0x28396au,gp[7]);
        Base::push(0x28396bu,gp[0]);
        Base::push(0x28396cu,0x283971u);
        scope_.child([&]{native_codec_transform_publication_detail::run_private_complete(s_,std::ref(before_));});

        at(0x283971u);
        s_.flags.logic(gp[0]&255u,1u);
        if(!s_.flags.zf)store(0x283975u,0x510f30u,4u);
    }

    void publish_second_table() {
        auto& gp=s_.gp;
        gp[1]=load(0x28397fu,gp[6]+4u);
        gp[0]=signed_half(0x283982u,gp[1]+6u);
        gp[5]=load(0x283986u,gp[6]);
        s_.set(2u,load(0x283988u,gp[4]+40u,1u),1u,0u);
        gp[1]=s_.flags.logic(0u,4u);
        gp[0]=s_.flags.plus(gp[0],gp[5],4u);
        store(0x283990u,0x510f60u,gp[0]);

        gp[0]=load(0x283995u,gp[0],1u);
        s_.flags.minus(gp[2]&255u,gp[3]&255u,1u);
        s_.set(1u,!s_.flags.zf,1u,0u);
        gp[0]*=5u;
        gp[0]=native_mopp_aabb_abi_detail::shifted_immediate<3>(s_,gp[0]);
        gp[1]*=3u;
        gp[1]=0x48c33cu+gp[0]+gp[1]*4u;
        gp[0]=load(0x2839adu,gp[1]);
        store(0x2839afu,0x510f64u,gp[0]);
        gp[0]=load(0x2839b4u,gp[1]+4u);
        gp[1]=load(0x2839b7u,gp[1]+8u);
        store(0x2839bau,0x510f6cu,gp[1]);
        store(0x2839c0u,0x510f68u,gp[0]);
    }

    void publish_second_streams() {
        auto& gp=s_.gp;
        gp[0]=load(0x2839c5u,gp[6]+4u);
        gp[1]=signed_half(0x2839c8u,gp[0]+6u);
        gp[3]=load(0x2839ccu,gp[0]+12u);
        gp[0]=signed_byte(0x2839cfu,gp[0]);
        gp[1]=s_.flags.plus(gp[1],gp[3],4u);
        gp[1]=s_.flags.plus(gp[1],gp[0],4u);
        gp[1]=s_.flags.plus(gp[1],load(0x2839d6u,gp[6]),4u);
        store(0x2839d8u,0x510f70u,gp[1]);

        gp[1]=load(0x2839deu,gp[6]+8u,1u);
        gp[0]=load(0x2839e2u,gp[6]+4u);
        gp[7]=signed_half(0x2839e5u,gp[0]+6u);
        gp[3]=load(0x2839e9u,gp[0]+12u);
        gp[0]=signed_byte(0x2839ecu,gp[0]);
        gp[5]=load(0x2839efu,gp[6]);
        align_track_count();
        gp[1]=s_.flags.plus(gp[1],gp[7],4u);
        gp[1]=s_.flags.plus(gp[1],gp[3],4u);
        gp[1]=s_.flags.plus(gp[1],gp[0],4u);
        gp[1]=s_.flags.plus(gp[1],gp[5],4u);
        store(0x283a02u,0x510f74u,gp[1]);

        gp[1]=load(0x283a08u,gp[6]+8u,1u);
        gp[0]=load(0x283a0cu,gp[6]+4u);
        gp[7]=signed_half(0x283a0fu,gp[0]+6u);
        gp[3]=load(0x283a13u,gp[0]+12u);
        gp[0]=signed_byte(0x283a16u,gp[0]);
        align_track_count();
        gp[1]=gp[7]+gp[1]*2u;
        gp[1]=s_.flags.plus(gp[1],gp[3],4u);
        gp[1]=s_.flags.plus(gp[1],gp[0],4u);
        gp[1]=s_.flags.plus(gp[1],load(0x283a29u,gp[6]),4u);
        store(0x283a2bu,0x510f78u,gp[1]);
        store_channel(gp[2]&255u);
    }

    Stop second_selector() {
        Base::push(0x283a37u,0x283a3cu);
        return native_codec_model_first_selector_detail::select_after_push(s_,before_,scope_);
    }
    void decode_selected() {
        scope_.child([&]{native_codec_sparse_decode_detail::run_private(s_,std::ref(before_));});
    }
};

template<class State,class Before=Noop>Stop run_private(State& state,Before before={}) {
    native_integer_fp_detail::Scope scope{state};
    native_codec_model_secondary_data_detail::Frame gate{state,before,scope};
    if(gate.has_secondary_data()) {
        Frame view{state,before,scope};
        view.transform_if_selected();
        view.publish_second_table();
        view.publish_second_streams();
        const auto stop=view.second_selector();
        if(stop.kind==Boundary::tail_entry) {
            if(stop.instruction!=0x284d30u)return stop;
            view.decode_selected();
        }
    }
    gate.finish_parent();
    return {Boundary::returned,0x283a3fu};
}
} // namespace halo2::native_codec_model_secondary_transform_detail
#endif
