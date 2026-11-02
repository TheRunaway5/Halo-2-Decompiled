#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct NativeSessionPacket;
struct NativeApiAudioBinding;
// Genuine native mutex, resource, buffer, exception and worker IO only. Rust
// drives each public API. Standalone audio sync/fence already holds the outer
// generation lock. A native exception_ptr preserves the original exception
// object through the Rust Result boundary. No Session borrow spans callbacks.
class NativeSessionApiIo {
public:
    virtual ~NativeSessionApiIo()=default;
    virtual void api_lock()const=0;
    virtual void api_unlock()const=0;
    virtual void api_fence()const=0;
    virtual void api_send(std::uint32_t,rust::Slice<const std::uint8_t>)const=0;
    virtual rust::Vec<NativeSessionPacket> api_receive()const=0;
    virtual void api_receive_peer(std::uint32_t)const=0;
    virtual void api_receive_bytes(rust::Slice<const std::uint8_t>)const=0;
    virtual bool api_audio_present()const=0;
    virtual bool api_audio_resume_allowed()const=0;
    virtual void api_audio_resume()const=0;
    virtual bool api_audio_ready()const=0;
    virtual std::uint64_t api_audio_generation()const=0;
    virtual std::uint64_t api_audio_fence()const=0;
    virtual void api_apply_audio_fence(std::uint64_t)const=0;
    virtual void api_voice(std::uint32_t,bool)const=0;
    virtual void api_preferences(std::uint32_t,bool,std::uint16_t)const=0;
    virtual void api_collect_voices()const=0;
    virtual NativeApiAudioBinding api_audio_binding()const=0;
    virtual void api_install_audio()const=0;
    virtual void api_bind_audio()const=0;
    virtual void api_close_presence()const=0;
    virtual bool api_pin_audio()const=0;
    virtual void api_close_audio()const=0;
    virtual void api_unpin_audio()const=0;
    virtual void api_stop_worker()const=0;
    virtual bool api_worker_joinable()const=0;
    virtual void api_join_worker()const=0;
};
}
