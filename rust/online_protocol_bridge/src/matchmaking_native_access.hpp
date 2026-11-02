#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct NativePostgamePreferences;
struct NativeSessionAudio;
class NativeSnapshotViewIo {
public:
    virtual ~NativeSnapshotViewIo()=default;
    virtual bool snapshot_view_common()const=0;
    virtual bool snapshot_view_supported()const=0;
    virtual void snapshot_view_select()const=0;
};
// Real audio resources and mutex only. Rust owns every phase/order decision.
// No mutable Rust Session borrow may span these callbacks. Native RAII must
// release the mutex on unexpected unwinding as well as ordinary IO failures.
class NativePostgameIo {
public:
    virtual ~NativePostgameIo()=default;
    virtual bool postgame_pin_audio()const=0;
    virtual std::uint32_t postgame_phase()const=0;
    virtual void postgame_enable(bool)const=0;
    virtual bool postgame_focused()const=0;
    virtual void postgame_focus(bool)const=0;
    virtual void postgame_radio(bool,bool)const=0;
    virtual std::uint16_t postgame_blocks()const=0;
    virtual void postgame_set_blocks(std::uint16_t)const=0;
    virtual NativePostgamePreferences postgame_preferences()const=0;
    virtual rust::Vec<NativeSessionAudio> postgame_outgoing()const=0;
    virtual void postgame_lock()const=0;
    virtual void postgame_unlock()const=0;
    virtual void postgame_fence()const=0;
    virtual rust::Vec<NativeSessionAudio> postgame_exchange(const NativePostgamePreferences &,rust::Vec<NativeSessionAudio>)const=0;
    virtual void postgame_deliver(rust::Vec<NativeSessionAudio>)const=0;
};
}
