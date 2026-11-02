#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15PendingResponseStatus { Idle, Waiting, Due };
struct NativeTu15PendingResponseStep {
    std::uint16_t word{},countdown{};
    std::uint8_t response{};
    NativeTu15PendingResponseStatus status{};
};
// D8330's per-section pending word. Zero countdown is inactive, including a
// nonzero low-three-bit response index. A live countdown decrements once;
// only its transition to zero requests the existing eligibility/response owner.
// No seconds, frame rate, eligibility result or effect/damage result is supplied.
constexpr NativeTu15PendingResponseStep native_tu15_pending_response_step(std::uint16_t word) {
    const auto count=std::uint16_t(word>>3);
    const auto response=std::uint8_t(word&7u);
    if(!count)return {word,count,response,NativeTu15PendingResponseStatus::Idle};
    const auto next=std::uint16_t(((count-1u)<<3)|response);
    return {next,count,response,count==1?NativeTu15PendingResponseStatus::Due:NativeTu15PendingResponseStatus::Waiting};
}
}
