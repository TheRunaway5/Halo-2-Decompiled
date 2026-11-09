#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <map>

namespace halo2 {
enum class NativeTu15LazyHeapSlot {start_padding,start_high,finish_padding,finish_high,retries};
class NativeTu15LazyHeapFrame {
public:
    virtual ~NativeTu15LazyHeapFrame()=default;
    virtual std::uint32_t load(NativeTu15LazyHeapSlot)const=0;
    virtual void store(NativeTu15LazyHeapSlot,std::uint32_t,std::uint32_t publication)=0;
};
struct NativeTu15LazyHeapServices {
    // The timestamp is an external input. Native production owns its real
    // clock; physical differential proofs may replay observed original reads.
    std::function<std::uint64_t(std::uint32_t site)> timestamp_counter;
    std::function<std::uint32_t(std::uint32_t bytes,NativeTu15PhysicsFrameFp&)> allocate; //12FE00
    std::function<void(NativeTu15PhysicsFrameFp&)> cleanup_first,cleanup_second,cleanup_third; //12EF10/60/12F0C0
    std::function<std::uint32_t(NativeTu15PhysicsFrameFp&)> milliseconds; //33B590
    std::function<void(NativeTu15PhysicsFrameFp&)> notify; //12EDF0
    // Actual22A5C0 owns conversion; the retained adapter compares its real
    // extended result with the live source timeout operand and retires it.
    std::function<bool(std::uint64_t ticks,NativeTu15PhysicsFrameFp&)> deadline_reached;
    std::function<std::uint32_t(std::uint32_t context,NativeTu15PhysicsFrameFp&)> yield_context; //404D10
    std::function<void(std::uint32_t current,NativeTu15PhysicsFrameFp&)> yield; //2DBF74
    std::function<void(bool aggressive,NativeTu15PhysicsFrameFp&)> trim; //1CD130
};
enum class NativeTu15LazyHeapPhase {binding,slow_begin,attempt_clock,pressure_binding,pressure_count,
    allocation,allocated,retry,clock_deadline,notification_clock,notification_value,
    elapsed_clock,elapsed_inputs,elapsed_delta,deadline,yield_binding,yield_process,yield_cursor,
    yield_bound,yield_fallback,backing,reduce_request,constructor_binding,constructor_inputs,
    constructor_size,publish_binding,returning};
struct NativeTu15LazyHeapPhaseData {
    std::uint32_t value{},bytes{},allocation{},pointer{},count{},low{},high{},start_low{},start_high{},
        previous{},delta_low{},delta_high{},adjustment{},object{},field{};
    bool slow{},deadline_reached{};
};
struct NativeTu15LazyHeapPhases {
    std::function<void(NativeTu15LazyHeapPhase,const NativeTu15LazyHeapPhaseData&)> observe;
};
struct NativeTu15LazyHeapStats {
    std::uint64_t calls{},initialized_returns{},attempts{},retries{},allocations{},notifications{},
        elapsed_queries{},yields{},constructions{},trims{},unbound_services{};
    std::map<std::uint32_t,std::uint64_t> unbound_owners;
};
// Whole actual149930 lazy emergency heap: native allocation retry policy,
// clock deadlines, ownership publication and source main-heap construction.
// Services are the distinct real owners; none supplies a guessed allocator
// outcome. Frame cells remain lazy so caller/header aliases keep their order.
std::uint32_t native_tu15_lazy_default_heap(SourcePhysicsContactMemory&,NativeTu15LazyHeapFrame&,
    NativeTu15PhysicsFrameFp&,const NativeTu15LazyHeapServices&,const NativeTu15LazyHeapPhases& = {},
    const SourcePhysicsContactRecordObserver& = {},NativeTu15LazyHeapStats* = nullptr);
} // namespace halo2
