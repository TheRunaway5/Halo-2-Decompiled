#pragma once
#include <cstdint>

namespace halo2 {
// Complete BC6C0's data work over the current constructed object's storage.
// There is deliberately no object validity/admission result or safe default.
class NativeTu15ObjectRegionStorage {
public:
    virtual ~NativeTu15ObjectRegionStorage()=default;
    virtual std::uint32_t object_body(std::uint32_t datum)=0;
    virtual std::int16_t descriptor_extent(std::uint32_t body)=0;
    virtual void publish_extent(std::int32_t)=0;
    virtual std::uint32_t active_base(std::uint32_t body)=0;
    // Read the actual optional output argument after the extent/base/count
    // work, before the final count store. This supplies input, not a result.
    virtual std::uint32_t states_output(std::int32_t count)=0;
    virtual void publish_count(std::int32_t)=0;
    virtual bool active_requested()=0;
    virtual void publish_active(std::uint32_t)=0;
    virtual bool cached_requested()=0;
    virtual std::uint32_t reread_count(bool for_states)=0;
    virtual void publish_cached(std::uint32_t)=0;
    virtual bool states_requested(std::uint32_t output)=0;
    virtual void publish_states(std::uint32_t output,std::uint32_t)=0;
};
struct NativeTu15ObjectRegionBank {
    std::uint32_t body{},active_base{};
    std::int32_t count{};
};
constexpr std::int32_t native_tu15_object_region_count(std::int16_t extent) {
    return std::int32_t(extent)/10;
}
inline NativeTu15ObjectRegionBank native_tu15_query_object_regions(
    NativeTu15ObjectRegionStorage& storage,std::uint32_t object) {
    const auto body=storage.object_body(object);
    const auto extent=storage.descriptor_extent(body);
    storage.publish_extent(extent);
    const auto base=storage.active_base(body);
    const auto count=native_tu15_object_region_count(extent);
    const auto states=storage.states_output(count);
    storage.publish_count(count);
    if(storage.active_requested())storage.publish_active(base);
    if(storage.cached_requested())storage.publish_cached(base+storage.reread_count(false));
    if(storage.states_requested(states))storage.publish_states(states,base+storage.reread_count(true)*2u);
    return {body,base,count};
}
}
