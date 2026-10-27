#pragma once
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2::native_vehicle_rate_math_tu15 {
// Typed numerical quantities for the TU1.5 signed-rate controller. These
// operations use the caller's qualified MXCSR/x87 controls. They have no
// virtual registers, guest addresses, instruction cursor or memory service.
struct ScalarRate {
    float value;
    ScalarRate increased_by(ScalarRate step) const {
        return {_mm_cvtss_f32(_mm_add_ss(_mm_set_ss(value),_mm_set_ss(step.value)))};
    }
    ScalarRate decreased_by(ScalarRate step) const {
        return {_mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(value),_mm_set_ss(step.value)))};
    }
    ScalarRate normalized_by(ScalarRate step) const {
        return {_mm_cvtss_f32(_mm_div_ss(_mm_set_ss(value),_mm_set_ss(step.value)))};
    }
    ScalarRate scaled_by(ScalarRate step) const {
        return {_mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(value),_mm_set_ss(step.value)))};
    }
};

// Source rate deltas multiply |direction|, the authored coefficient and tick
// duration in extended precision before their single-precision publication.
// Keep the stages separate: callers can load each actual input at its original
// memory boundary, including when an input is unavailable or a store rejects.
struct RateDelta {
    long double extended;
    std::uint16_t operation_status{};
    static RateDelta magnitude(long double direction) {return {std::fabs(direction)};}
    void apply_coefficient(float coefficient) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        long double next;
        // The original uses FMUL m32 with the existing extended direction on
        // ST0. C++ may commute two NaNs or first extend the memory operand.
        asm volatile("fldt %2\n\tfmuls %3\n\tfnstsw %1\n\tfstpt %0"
            :"=m"(next),"=a"(operation_status):"m"(extended),"m"(coefficient):"memory","st");
        extended=next;
#else
        throw std::runtime_error("Exact native vehicle rate x87 multiplication unavailable");
#endif
    }
    void apply_seconds(float seconds) {apply_coefficient(seconds);}
    float published() const {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        float scalar;
        asm volatile("fldt %1\n\tfstps %0":"=m"(scalar):"m"(extended):"memory","st");
        return scalar;
#else
        throw std::runtime_error("Exact native vehicle rate single-precision publication unavailable");
#endif
    }
};

struct RateComparison {
    bool below,equal,unordered;
    bool above() const {return !below && !equal;}
};
inline RateComparison compare_rate(ScalarRate left,ScalarRate right,bool quiet=false) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char below,equal,unordered;
    const auto a=_mm_set_ss(left.value),b=_mm_set_ss(right.value);
    if(quiet)asm volatile("ucomiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2"
        :"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(a),"x"(b):"cc");
    else asm volatile("comiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2"
        :"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(a),"x"(b):"cc");
    return {bool(below),bool(equal),bool(unordered)};
#else
    throw std::runtime_error("Native TU1.5 scalar comparison backend unavailable");
#endif
}

// Optional semantic evaluation observations. Native value owners need none of
// these; the transitional caller adapter uses them to retain its observable
// register/FP postimage if a later input read or publication rejects.
struct RateEvaluationObserver {
    void direction_tested(ScalarRate,RateComparison) {}
    void extended_direction_loaded(long double) {}
    void direction_retested(RateComparison) {}
    void scaling_started(long double) {}
    void acceleration_scaled(long double,std::uint16_t) {}
    void deceleration_scaled(long double,std::uint16_t) {}
    void deceleration_loaded(ScalarRate) {}
    void reverse_candidate(ScalarRate) {}
    void reverse_compared(RateComparison) {}
    void current_for_increase(ScalarRate) {}
    void current_for_sign(ScalarRate) {}
    void sign_compared(RateComparison) {}
    void increase_origin(ScalarRate) {}
    void increased_value(ScalarRate) {}
    void increasing_normalized(ScalarRate) {}
    void positive_limit_compared(RateComparison) {}
    void positive_limit_loaded(ScalarRate) {}
    void current_for_decrease(ScalarRate) {}
    void braking_compared(RateComparison) {}
    void decreased_candidate(ScalarRate) {}
    void zero_current_compared(RateComparison) {}
    void current_for_negative_acceleration(ScalarRate) {}
    void negative_accelerated(ScalarRate) {}
    void current_for_negative_normalization(ScalarRate) {}
    void decreasing_normalized(ScalarRate) {}
    void negative_limit_loaded(ScalarRate) {}
    void negative_limit_current(ScalarRate) {}
    void negative_limit_compared(RateComparison) {}
    void final_rate_loaded(ScalarRate) {}
    void current_for_target(ScalarRate) {}
    void target_compared(RateComparison) {}
    void backward_direction_started() {}
    void backward_direction_loaded(ScalarRate) {}
};

enum class AccelerationResult {unchanged,advanced,positive_limit};
// Access supplies semantic values and publications. Each getter is evaluated
// where the algorithm consumes it, rather than eagerly snapshotting parameters:
// the caller can have aliased rate/parameter storage and rejecting accesses.
template<class Access>
AccelerationResult accelerate_rate(Access &a) {
    const ScalarRate zero{0};
    const auto direction=a.direction();
    const auto tested=compare_rate(direction,zero,true);
    a.direction_tested(direction,tested);
    if(tested.equal && !tested.unordered)return AccelerationResult::unchanged;

    const auto extended_direction=static_cast<long double>(a.direction().value);
    a.extended_direction_loaded(extended_direction);
    const auto direction_sign=compare_rate(direction,zero);
    a.direction_retested(direction_sign);
    const auto magnitude=RateDelta::magnitude(extended_direction).extended;
    a.scaling_started(magnitude);
    RateDelta acceleration{magnitude};
    acceleration.apply_coefficient(a.acceleration_coefficient().value);a.acceleration_scaled(acceleration.extended,acceleration.operation_status);
    acceleration.apply_seconds(a.seconds().value);a.acceleration_scaled(acceleration.extended,acceleration.operation_status);
    a.publish_acceleration(acceleration);
    RateDelta deceleration{magnitude};
    deceleration.apply_coefficient(a.deceleration_coefficient().value);a.deceleration_scaled(deceleration.extended,deceleration.operation_status);
    deceleration.apply_seconds(a.seconds().value);a.deceleration_scaled(deceleration.extended,deceleration.operation_status);
    a.publish_deceleration(deceleration);
    const auto braking=a.deceleration_step();a.deceleration_loaded(braking);

    if(direction_sign.above()) {
        const auto reverse=zero.decreased_by(braking);a.reverse_candidate(reverse);
        const auto reversed=compare_rate(reverse,a.current());a.reverse_compared(reversed);
        ScalarRate next;
        if(!reversed.below) {
            const auto current=a.current();a.current_for_increase(current);
            next=current.increased_by(braking);a.increased_value(next);
        } else {
            const auto current=a.current();a.current_for_sign(current);
            const auto sign=compare_rate(current,zero);a.sign_compared(sign);a.increase_origin(current);
            if(!sign.below) {
                next=current.increased_by(a.acceleration_step());a.increased_value(next);
            } else {
                auto relative=current.normalized_by(braking);a.increasing_normalized(relative);
                relative=relative.increased_by(a.unit());a.increasing_normalized(relative);
                next=relative.scaled_by(a.acceleration_step());a.increased_value(next);
            }
        }
        a.publish_increased(next);
        const auto limited=compare_rate(next,a.positive_limit());a.positive_limit_compared(limited);
        if(limited.above()) {
            const auto bound=a.positive_limit();a.positive_limit_loaded(bound);a.publish_positive_limit(bound);
            return AccelerationResult::positive_limit;
        }
        const auto current=a.current();a.final_rate_loaded(current);a.publish_final(current);
    } else {
        const auto current=a.current();a.current_for_decrease(current);
        const auto braking_test=compare_rate(current,braking);a.braking_compared(braking_test);
        if(!braking_test.below) {
            const auto next=current.decreased_by(braking);a.decreased_candidate(next);a.publish_decreased(next);
        } else {
            const auto sign=compare_rate(zero,a.current());a.zero_current_compared(sign);
            if(!sign.below) {
                const auto value=a.current();a.current_for_negative_acceleration(value);
                const auto next=value.decreased_by(a.acceleration_step());a.negative_accelerated(next);
                a.publish_negative_acceleration(next);
            } else {
                const auto value=a.current();a.current_for_negative_normalization(value);
                auto relative=value.normalized_by(braking);a.decreasing_normalized(relative);
                relative=relative.decreased_by(a.unit());a.decreasing_normalized(relative);
                const auto next=relative.scaled_by(a.acceleration_step());a.decreasing_normalized(next);
                a.publish_decreased(next);
            }
        }
        auto next=zero.decreased_by(a.negative_limit());a.negative_limit_loaded(next);
        const auto value=a.current();a.negative_limit_current(value);
        const auto limited=compare_rate(value,next);a.negative_limit_compared(limited);
        if(limited.above()){next=a.current();a.final_rate_loaded(next);}
        a.publish_final(next);
    }
    return AccelerationResult::advanced;
}

enum class ApproachResult {reached,advancing};
template<class Access>
ApproachResult approach_rate(Access &a) {
    const auto current=a.current();a.current_for_target(current);
    const auto target=a.target();
    auto comparison=compare_rate(current,target);a.target_compared(comparison);
    a.begin_approach();
    bool clamp=false;
    if(comparison.above()) {
        const auto seconds=a.backward_seconds();a.backward_direction_started();
        const auto direction=ScalarRate{0}.decreased_by(a.backward_direction());a.backward_direction_loaded(direction);
        a.prepare_backward(seconds,direction);a.advance();
        comparison=compare_rate(target,a.current());a.target_compared(comparison);clamp=true;
    } else {
        comparison=compare_rate(target,a.current());a.target_compared(comparison);
        if(comparison.above()) {
            const auto seconds=a.forward_seconds();a.prepare_forward_seconds(seconds);
            const auto direction=a.forward_direction();a.prepare_forward_direction(direction);a.advance();
            const auto next=a.current();a.current_for_target(next);
            comparison=compare_rate(next,target);a.target_compared(comparison);clamp=true;
        }
    }
    if(clamp && comparison.below)return ApproachResult::advancing;
    if(clamp)a.publish_target(target);
    return ApproachResult::reached;
}
} // namespace halo2::native_vehicle_rate_math_tu15
