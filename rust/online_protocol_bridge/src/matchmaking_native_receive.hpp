#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct NativeReceiveSession;
enum class NativeReceiveTarget : std::uint8_t;
// Real lock/owner/history IO only. Holds no Rust Session borrow across callbacks.
// A native RAII lock must also cover exceptional Rust/CXX unwinding.
class NativeReceiveIo {
public:
    virtual ~NativeReceiveIo()=default;
    virtual void receive_lock()const=0;
    virtual void receive_unlock()const=0;
    virtual NativeReceiveSession receive_session()const=0;
    virtual bool receive_supported(NativeReceiveTarget)const=0;
    virtual void receive_pin(NativeReceiveTarget)const=0;
    virtual bool receive_present(NativeReceiveTarget)const=0;
    virtual std::uint32_t receive_epoch(NativeReceiveTarget)const=0;
    virtual bool receive_binding_present()const=0;
    virtual std::uint32_t receive_binding_epoch()const=0;
    virtual std::uint64_t receive_history_len()const=0;
    virtual std::uint32_t receive_history_epoch(std::uint64_t)const=0;
    virtual void receive_history_packet(std::uint64_t,std::uint32_t,rust::Slice<const std::uint8_t>)const=0;
    virtual void receive_packet(NativeReceiveTarget,std::uint32_t,rust::Slice<const std::uint8_t>)const=0;
    virtual void receive_abort()const=0;
    virtual void receive_progress()const=0;
    virtual void receive_fence()const=0;
    virtual bool receive_general(std::uint32_t,std::uint32_t,rust::Slice<const std::uint8_t>)const=0;
    virtual void receive_observe_host()const=0;
    virtual void receive_refresh()const=0;
};
}
