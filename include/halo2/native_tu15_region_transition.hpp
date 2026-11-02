#pragma once
#include "halo2/native_tu15_object_regions.hpp"

namespace halo2 {
class NativeTu15RegionTransitionStorage {
public:
    virtual ~NativeTu15RegionTransitionStorage()=default;
    virtual std::uint32_t object_definition()=0;
    virtual std::uint32_t model(std::uint32_t definition)=0;
    virtual std::int16_t region_extent()=0;
    virtual void prepare_snapshot(std::int32_t count)=0;
    virtual std::uint32_t snapshot_word()=0;
    virtual void snapshot_word(std::uint32_t)=0;
    virtual void advance_snapshot_word()=0;
    virtual void prepare_snapshot_bytes(std::uint32_t count)=0;
    virtual std::uint8_t snapshot_byte()=0;
    virtual void snapshot_byte(std::uint8_t)=0;
    virtual void advance_snapshot_byte()=0;
    virtual std::uint8_t object_type()=0;
    virtual std::uint16_t attached_index()=0;
    virtual std::uint8_t flags()=0;
    // Each operation owns a genuine current callee and its real mutable
    // memory/ABI. No model, attachment, effect or physics result is supplied.
    virtual void refresh_link()=0;
    virtual void apply_model_state()=0;
    virtual void notify_model()=0;
    virtual void rebind_snapshot()=0;
    virtual void notify_object()=0;
};
// Complete BC720 data/control work. The original snapshots count/4 DWORDs
// then count%4 bytes, including its unchecked32-bit count conversion. Storage
// owns direction and each transfer/fault boundary; there is no libc memcpy,
// new count clamp, object admission, model fallback or manufactured result.
inline void native_tu15_region_transition(NativeTu15RegionTransitionStorage& storage) {
    const auto definition=storage.object_definition();
    if(storage.model(definition)==0xffffffffu)return;
    const auto count=native_tu15_object_region_count(storage.region_extent());
    storage.prepare_snapshot(count);
    for(auto remaining=std::uint32_t(count)>>2u;remaining;--remaining){
        const auto value=storage.snapshot_word();storage.snapshot_word(value);storage.advance_snapshot_word();
    }
    storage.prepare_snapshot_bytes(std::uint32_t(count)&3u);
    for(auto remaining=std::uint32_t(count)&3u;remaining;--remaining){
        const auto value=storage.snapshot_byte();storage.snapshot_byte(value);storage.advance_snapshot_byte();
    }
    if(storage.object_type()==6u && storage.attached_index()!=0xffffu && !(storage.flags()&2u))storage.refresh_link();
    storage.apply_model_state();storage.notify_model();storage.rebind_snapshot();storage.notify_object();
}
}
