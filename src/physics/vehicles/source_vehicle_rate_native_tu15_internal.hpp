#pragma once
#include "physics/vehicles/physics_vehicle_runtime_internal.hpp"
#include "physics/vehicles/native_vehicle_rate_math_tu15.hpp"
#include "halo2/source_vehicle_native_rates.hpp"
#include "halo2/native_vehicle_rate_consumer_tu15.hpp"
#if __has_include("physics/runtime/physics_instruction_remap_tu15.hpp")
#include "physics/runtime/physics_instruction_remap_tu15.hpp"
#elif __has_include("physics_instruction_remap_tu15.hpp")
#include "physics_instruction_remap_tu15.hpp"
#endif
namespace halo2 {struct PhysicsInstructionRemapTu15;}
#include <atomic>

namespace halo2::physics_vehicle_runtime_detail {
inline std::atomic<std::uint64_t> rate_acceleration_calls{},rate_approach_calls{},rate_cursor_calls{};

// All 94 original instruction sites in the two complete scalar bodies. A
// mapped callback must retain every branch's original lookup/fault behavior.
inline constexpr auto rate_instruction_sites=std::to_array<std::uint32_t>({
    0x208d80u,0x208d86u,0x208d89u,0x208d8cu,0x208d8du,0x208d90u,0x208d96u,0x208d9au,
    0x208d9du,0x208d9fu,0x208da1u,0x208da4u,0x208da8u,0x208dacu,0x208dafu,0x208db3u,
    0x208db7u,0x208dbdu,0x208dbfu,0x208dc2u,0x208dc6u,0x208dc9u,0x208dcbu,0x208dcfu,
    0x208dd3u,0x208dd5u,0x208dd9u,0x208ddcu,0x208ddfu,0x208de1u,0x208de7u,0x208de9u,
    0x208dedu,0x208df5u,0x208dfbu,0x208dffu,0x208e02u,0x208e04u,0x208e08u,0x208e0cu,
    0x208e0fu,0x208e13u,0x208e16u,0x208e18u,0x208e1cu,0x208e1eu,0x208e21u,0x208e23u,
    0x208e27u,0x208e2du,0x208e31u,0x208e33u,0x208e37u,0x208e3bu,0x208e43u,0x208e49u,
    0x208e4du,0x208e52u,0x208e56u,0x208e59u,0x208e5bu,0x208e5fu,0x208e63u,0x208e70u,
    0x208e74u,0x208e77u,0x208e78u,0x208e7au,0x208e7cu,0x208e80u,0x208e83u,0x208e89u,
    0x208e8au,0x208e8bu,0x208e90u,0x208e95u,0x208e98u,0x208e9au,0x208e9du,0x208e9fu,
    0x208ea3u,0x208ea4u,0x208ea8u,0x208ea9u,0x208eaeu,0x208eb2u,0x208eb5u,0x208eb7u,
    0x208ebbu,0x208ebdu,0x208ebeu,0x208ec1u,0x208ec3u,0x208ec4u});

template<class State>
[[noreturn]] void vehicle_rate_native_refuse(const State&s,const char*why){
    native_vehicle_rate_observation_refused_tu15(s.memory);throw std::runtime_error(why);
}
template<class State,class Select>
bool vehicle_rate_native_eligible(const State&s,const Select&select,bool approach) {
    if(const auto*policy=native_vehicle_rate_consumer_policy_tu15(s.memory)) {
        if(s.image!=SourcePhysicsContactImage::TitleUpdate15)
            vehicle_rate_native_refuse(s,"Native live vehicle rate scope requires TU1.5");
        if((policy->first_trace&&*policy->first_trace)||(policy->second_trace&&*policy->second_trace)||
           (s.observe&&!policy->observation_only_stores))
            vehicle_rate_native_refuse(s,"Native live vehicle rate scope refuses full instruction or mutable observation");
        const auto stack=s.gp[4],below=approach?16u:0u;
        if(stack<below||stack-below<policy->caller_stack_begin||
           std::uint64_t(stack)+12u>policy->caller_stack_end||
           s.gp[1]>0xfffffffcu||s.gp[2]>0xfffffff0u)
            vehicle_rate_native_refuse(s,"Native live vehicle rate scope requires ordinary retained caller storage");
        // Payload inputs remain lazy. No speculative view/read can consume a
        // source guard or hide its original partial-store/fault prefix.
        return true;
    }
    if(!s.services.instruction)return true;
    // Preserve modern cursor/remap admission exactly when those capabilities
    // actually exist. Old frozen states reach native kernels through their
    // explicit native world scope, never by importing a new machine state.
    if constexpr(requires{s.instruction_cursor;s.literal_service_sites;
                          s.memory.try_read_view(s.gp[4],std::size_t(4));}) {
        auto*cursor=s.instruction_cursor;
        if(!s.literal_service_sites||!cursor) {
            const auto*mapped=s.services.instruction.template target<PhysicsInstructionRemapTu15>();
            if(!mapped||!mapped->covers(rate_instruction_sites,
                s.image==SourcePhysicsContactImage::TitleUpdate15?0x4900u:0u))return false;
            cursor=mapped->live_cursor();
        }
        if(!cursor||(s.observe&&!cursor->observation_only_stores)||
           (cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))return false;
        const auto stack=s.gp[4];
        if(stack<(approach?16u:0u)||stack>0xffffffffu-11u||
           s.gp[1]>0xfffffffcu||s.gp[2]>0xfffffff0u)return false;
        return s.memory.try_read_view(stack-(approach?16u:0u),approach?28u:12u)&&
            s.memory.try_read_view(s.gp[1],4)&&s.memory.try_read_view(s.gp[2],16)&&
            s.memory.try_read_view(select(0x465030u,s.image),4);
    }
    return false; // Offline full-machine/reference observation only.
}
template<class State>
void vehicle_rate_native_site(State&s,std::uint32_t site) {
    if(const auto*policy=native_vehicle_rate_consumer_policy_tu15(s.memory)) {
        if((policy->first_trace&&*policy->first_trace)||(policy->second_trace&&*policy->second_trace)||
           (s.observe&&!policy->observation_only_stores))
            vehicle_rate_native_refuse(s,"Native live vehicle rate observation changed during a scalar call");
        policy->publish_phase(site+(s.image==SourcePhysicsContactImage::TitleUpdate15?0x4900u:0u));
        return;
    }
    if(s.services.instruction){
        if constexpr(requires{s.instruction_literal(site);})s.instruction_literal(site);
        else s.services.instruction(site);
    }
}
inline void vehicle_rate_native_write(VehicleState&s,std::uint32_t site,
    std::uint32_t address,unsigned size,std::uint64_t value) {
    vehicle_rate_native_site(s,site);
    // This canonical observer protocol is already translated by the actual
    // TU1.5 composition. Translating this event twice would corrupt its owner.
    w(s.memory,s.observe,site,address,size,value);
}
inline void vehicle_rate_native_push(VehicleState&s,std::uint32_t site,std::uint32_t value) {
    vehicle_rate_native_site(s,site);s.push(site,value);
}
inline void vehicle_rate_native_publication_complete(VehicleState&s,std::uint16_t status) {
    s.fp.pop();s.fp.status=std::uint16_t((s.fp.status&~0x200u)|(status&0x200u));
}
// Publish a real FSTP as one instruction: destination rejection keeps the
// pre-instruction FP state, while a store observer sees the committed pop/C1.
// The observer is outside the memory transaction and cannot undo that store.
inline void vehicle_rate_publish_extended(VehicleState&s,std::uint32_t site,
    std::uint32_t address,long double value) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto before_flags=source_x87_exception_flags();
    std::uint32_t bits;std::uint16_t status;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        :"=m"(bits),"=a"(status):"m"(value):"memory","st");
    SourcePhysicsContactRecordStore event{site,address,{}, {}};
    std::array<std::byte,4>written{};
    for(unsigned i=0;i<4;++i)written[i]=std::byte(bits>>(i*8u));
    try {
        if(s.observe){event.before.resize(4);s.memory.read(address,event.before);event.written.assign(written.begin(),written.end());}
        s.memory.write(address,written);
    }catch(...){source_load_x87_exception_flags(before_flags);throw;}
    vehicle_rate_native_publication_complete(s,status);
    if(s.observe)s.observe(event);
#else
    throw std::runtime_error("Native vehicle rate publication unavailable");
#endif
}
// These narrow literal primitives repair source FSTP C1 and FMUL m32 NaN
// operand order. They do not select the native scalar entry or record calls.
inline void vehicle_rate_publish_extended_reference(VehicleState&s,std::uint32_t site,std::uint32_t address) {
    vehicle_rate_publish_extended(s,site,address,s.fp.st(0));
}
inline void vehicle_rate_multiply_extended_reference(VehicleState&s,std::uint32_t address) {
    native_vehicle_rate_math_tu15::RateDelta value{s.fp.st(0)};
    value.apply_coefficient(f(r(s.memory,address,4)));
    s.fp.st(0)=value.extended;
    s.fp.status=std::uint16_t((s.fp.status&~0x200u)|(value.operation_status&0x200u));
}
inline native_vehicle_rate_math_tu15::ScalarRate rate_value(__m128 value) {
    return {_mm_cvtss_f32(value)};
}
inline __m128 rate_lane(native_vehicle_rate_math_tu15::ScalarRate value) {
    return _mm_set_ss(value.value);
}

// Transitional ABI adapter. All rate arithmetic and physics branch decisions
// live in the typed native algorithms; these views publish only compatibility
// state, ordered storage accesses and caller return effects. Unsupported full
// instruction observers retain the generated reference route.
using native_vehicle_rate_math_tu15::ScalarRate;
using native_vehicle_rate_math_tu15::RateComparison;
inline void rate_comparison_state(VehicleState &s,RateComparison result) {
    s.flags.cf=result.below;s.flags.zf=result.equal;s.flags.pf=result.unordered;
    s.flags.af=s.flags.sf=s.flags.of=false;
}
struct RateAccelerationAccess {
    VehicleState &s;
    const std::uint32_t value,rates,arguments,unit_address;
    ScalarRate load(std::uint32_t address)const {return {f(r(s.memory,address,4))};}
    ScalarRate direction()const {return load(arguments+4);}
    ScalarRate seconds()const {return load(arguments+8);}
    ScalarRate acceleration_coefficient()const {return load(rates+8);}
    ScalarRate deceleration_coefficient()const {return load(rates+12);}
    ScalarRate acceleration_step()const {return load(arguments+4);}
    ScalarRate deceleration_step()const {return load(arguments+8);}
    ScalarRate current()const {return load(value);}
    ScalarRate positive_limit()const {return load(rates);}
    ScalarRate negative_limit()const {return load(rates+4);}
    ScalarRate unit()const {return load(unit_address);}
    void put(std::uint32_t site,std::uint32_t address,ScalarRate next) {
        vehicle_rate_native_write(s,site,address,4,u(next.value));
    }
    void publish_acceleration(const native_vehicle_rate_math_tu15::RateDelta&next) {vehicle_rate_native_site(s,0x208da8u);vehicle_rate_publish_extended(s,0x208da8u,arguments+4,next.extended);}
    void publish_deceleration(const native_vehicle_rate_math_tu15::RateDelta&next) {vehicle_rate_native_site(s,0x208db3u);vehicle_rate_publish_extended(s,0x208db3u,arguments+8,next.extended);}
    void publish_increased(ScalarRate next) {put(0x208dfbu,value,next);}
    void publish_positive_limit(ScalarRate next) {put(0x208e08u,value,next);}
    void publish_decreased(ScalarRate next) {put(0x208e49u,value,next);}
    void publish_negative_acceleration(ScalarRate next) {put(0x208e2du,value,next);}
    void publish_final(ScalarRate next) {put(0x208e5fu,value,next);}
    void direction_tested(ScalarRate direction,RateComparison result) {
        s.xmm[1]=rate_lane(direction);s.xmm[0]=_mm_setzero_ps();rate_comparison_state(s,result);
        s.set(0,s.flags.bits()|2u,1,8);s.flags.logic(s.reg(0,1,8)&0x44u,1);
    }
    void extended_direction_loaded(long double next) {s.fp.push(next);}
    void direction_retested(RateComparison result) {rate_comparison_state(s,result);}
    void scaling_started(long double next) {s.fp.st(0)=next;s.fp.push(next);}
    void acceleration_scaled(long double next,std::uint16_t status) {s.fp.st(0)=next;s.fp.status=std::uint16_t((s.fp.status&~0x200u)|(status&0x200u));}
    void deceleration_scaled(long double next,std::uint16_t status) {s.fp.st(0)=next;s.fp.status=std::uint16_t((s.fp.status&~0x200u)|(status&0x200u));}
    void deceleration_loaded(ScalarRate next) {s.xmm[1]=rate_lane(next);}
    void reverse_candidate(ScalarRate next) {s.xmm[2]=rate_lane(next);}
    void reverse_compared(RateComparison result) {rate_comparison_state(s,result);}
    void current_for_increase(ScalarRate next) {s.xmm[0]=rate_lane(next);}
    void current_for_sign(ScalarRate next) {s.xmm[2]=rate_lane(next);}
    void sign_compared(RateComparison result) {rate_comparison_state(s,result);}
    void increase_origin(ScalarRate next) {s.xmm[0]=rate_lane(next);}
    void increased_value(ScalarRate next) {s.xmm[0]=rate_lane(next);}
    void increasing_normalized(ScalarRate next) {s.xmm[0]=rate_lane(next);}
    void positive_limit_compared(RateComparison result) {rate_comparison_state(s,result);}
    void positive_limit_loaded(ScalarRate next) {s.xmm[0]=rate_lane(next);}
    void current_for_decrease(ScalarRate next) {s.xmm[2]=rate_lane(next);}
    void braking_compared(RateComparison result) {rate_comparison_state(s,result);}
    void decreased_candidate(ScalarRate next) {s.xmm[2]=rate_lane(next);}
    void zero_current_compared(RateComparison result) {rate_comparison_state(s,result);}
    void current_for_negative_acceleration(ScalarRate next) {s.xmm[1]=rate_lane(next);}
    void negative_accelerated(ScalarRate next) {s.xmm[1]=rate_lane(next);}
    void current_for_negative_normalization(ScalarRate next) {s.xmm[2]=rate_lane(next);}
    void decreasing_normalized(ScalarRate next) {s.xmm[2]=rate_lane(next);}
    void negative_limit_loaded(ScalarRate next) {s.xmm[0]=rate_lane(next);}
    void negative_limit_current(ScalarRate next) {s.xmm[1]=rate_lane(next);}
    void negative_limit_compared(RateComparison result) {rate_comparison_state(s,result);}
    void final_rate_loaded(ScalarRate next) {s.xmm[0]=rate_lane(next);}
};
template<class Select>
void vehicle_rate_accelerate_native(VehicleState &s,const Select &select) {
    rate_acceleration_calls.fetch_add(1,std::memory_order_relaxed);
    const bool cursor=bool(s.services.instruction)||bool(native_vehicle_rate_consumer_policy_tu15(s.memory));
    if(cursor)rate_cursor_calls.fetch_add(1,std::memory_order_relaxed);
    native_vehicle_rate_call_tu15(s.memory,false,cursor);
    RateAccelerationAccess values{s,s.gp[1],s.gp[2],s.gp[4],select(0x465030u,s.image)};
    const auto result=native_vehicle_rate_math_tu15::accelerate_rate(values);
    vehicle_rate_native_site(s,result==native_vehicle_rate_math_tu15::AccelerationResult::positive_limit?0x208e0cu:0x208e63u);
    s.pop();s.gp[4]+=8;
}
template<class Select>
struct RateApproachAccess {
    VehicleState &s;
    const Select &select;
    ScalarRate load(std::uint32_t address)const {return {f(r(s.memory,address,4))};}
    ScalarRate current()const {return load(s.gp[1]);}
    ScalarRate target()const {return rate_value(s.xmm[3]);}
    void current_for_target(ScalarRate next) {s.xmm[0]=rate_lane(next);}
    void target_compared(RateComparison result) {rate_comparison_state(s,result);}
    void begin_approach() {vehicle_rate_native_push(s,0x208e77u,s.gp[3]);s.set(3,1u,1,0);}
    ScalarRate backward_seconds() {s.gp[0]=r(s.memory,s.gp[4]+12,4);return {f(s.gp[0])};}
    void backward_direction_started() {s.xmm[0]=_mm_setzero_ps();}
    ScalarRate backward_direction()const {return load(s.gp[4]+8);}
    void backward_direction_loaded(ScalarRate next) {s.xmm[0]=rate_lane(next);}
    void prepare_backward(ScalarRate seconds,ScalarRate direction) {
        vehicle_rate_native_push(s,0x208e89u,u(seconds.value));
        vehicle_rate_native_push(s,0x208e8au,s.gp[1]);
        vehicle_rate_native_write(s,0x208e8bu,s.gp[4],4,u(direction.value));
        vehicle_rate_native_push(s,0x208e90u,select(0x208e95u,s.image));
    }
    ScalarRate forward_seconds() {s.gp[0]=r(s.memory,s.gp[4]+12,4);return {f(s.gp[0])};}
    void prepare_forward_seconds(ScalarRate seconds) {vehicle_rate_native_push(s,0x208ea3u,u(seconds.value));}
    ScalarRate forward_direction() {s.gp[0]=r(s.memory,s.gp[4]+12,4);return {f(s.gp[0])};}
    void prepare_forward_direction(ScalarRate direction) {
        vehicle_rate_native_push(s,0x208ea8u,u(direction.value));
        vehicle_rate_native_push(s,0x208ea9u,select(0x208eaeu,s.image));
    }
    void advance() {vehicle_rate_accelerate_native(s,select);}
    void publish_target(ScalarRate next) {vehicle_rate_native_write(s,0x208eb7u,s.gp[1],4,u(next.value));}
};
template<class Select>
void vehicle_rate_approach_native(VehicleState &s,const Select &select) {
    rate_approach_calls.fetch_add(1,std::memory_order_relaxed);
    const bool cursor=bool(s.services.instruction)||bool(native_vehicle_rate_consumer_policy_tu15(s.memory));
    if(cursor)rate_cursor_calls.fetch_add(1,std::memory_order_relaxed);
    native_vehicle_rate_call_tu15(s.memory,true,cursor);
    RateApproachAccess<Select> values{s,select};
    const auto result=native_vehicle_rate_math_tu15::approach_rate(values);
    const bool advancing=result==native_vehicle_rate_math_tu15::ApproachResult::advancing;
    if(advancing)s.set(0,s.flags.logic(0,1),1,0);
    else s.set(0,s.reg(3,1,0),1,0);
    vehicle_rate_native_site(s,advancing?0x208ec3u:0x208ebdu);s.gp[3]=s.pop();
    vehicle_rate_native_site(s,advancing?0x208ec4u:0x208ebeu);s.pop();s.gp[4]+=8;
}
} // namespace halo2::physics_vehicle_runtime_detail
