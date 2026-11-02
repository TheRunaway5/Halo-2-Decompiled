#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct MatchmakingQosOpen;
struct MatchmakingQosFrame;
struct MatchmakingQosSent;
struct NativeSessionFlush;
struct NativeSessionPacket;
enum class NativeSessionSendKind : std::uint8_t;
class NativeMatchSessionIo {
public:
    virtual ~NativeMatchSessionIo()=default;
    virtual std::uint64_t session_now_ms()const=0;
    virtual bool session_cancelled()const=0;
    virtual void session_wait(std::uint32_t)const=0;
    virtual void session_open(const MatchmakingQosOpen &)const=0;
    virtual MatchmakingQosFrame session_receive()const=0;
    virtual MatchmakingQosSent session_send(NativeSessionSendKind,rust::Slice<const std::uint8_t>)const=0;
    virtual void session_close()const=0;
    virtual rust::String session_text(rust::Slice<const std::uint8_t>)const=0;
    virtual void session_datagram(std::uint8_t,std::uint32_t,rust::Slice<const std::uint8_t>)const=0;
    virtual void session_pump()const=0;
    virtual NativeSessionFlush session_flush()const=0;
    virtual bool session_packet_current(const NativeSessionPacket &)const=0;
    virtual bool session_failure(bool,rust::Str)const=0;
};
}
