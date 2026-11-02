#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct NativeSessionSignal;
struct NativeSessionPacket;
struct NativeSessionDirect;
struct NativePeerSample;
struct NativePeerDatagram;
enum class NativePeerPoll : std::uint8_t;
class NativeMatchPeersIo {
public:
    virtual ~NativeMatchPeersIo()=default;
    virtual bool peers_cancelled()const=0;
    virtual void peers_wait(std::uint32_t)const=0;
    virtual bool peers_ready(std::uint8_t)const=0;
    virtual bool peers_failed(std::uint8_t)const=0;
    virtual rust::Vec<NativeSessionSignal> peers_signals()const=0;
    virtual void peers_signal(const NativeSessionSignal &)const=0;
    virtual NativePeerSample peers_sample(std::uint16_t)const=0;
    virtual NativePeerDatagram peers_receive()const=0;
    virtual void peers_datagram(std::uint32_t,std::uint32_t,rust::Slice<const std::uint8_t>)const=0;
    virtual void peers_rejected()const=0;
    virtual rust::Vec<NativeSessionPacket> peers_drain()const=0;
    virtual void peers_send_media(const NativeSessionPacket &,rust::Slice<const std::uint8_t>)const=0;
    virtual void peers_send(std::uint32_t,rust::Slice<const std::uint8_t>)const=0;
    virtual rust::Vec<NativePeerDatagram> peers_poll(NativePeerPoll,std::uint32_t,std::uint16_t,std::uint32_t)const=0;
    virtual void peers_drive()const=0;
    virtual std::uint32_t peers_agreement_generations()const=0;
    virtual bool peers_finish(const NativeSessionDirect &)const=0;
    virtual void peers_failure()const=0;
};
}
