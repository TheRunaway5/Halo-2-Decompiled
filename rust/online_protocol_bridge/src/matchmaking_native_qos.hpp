#pragma once
#include "rust/cxx.h"
#include <cstdint>

namespace halo2::online_protocol_bridge {
struct MatchmakingQosOpen;
struct MatchmakingQosFrame;
struct MatchmakingQosSent;
enum class MatchmakingQosSendKind : std::uint8_t;

class NativeMatchmakingQosIo {
public:
  virtual ~NativeMatchmakingQosIo() = default;
  virtual std::uint64_t qos_now_ms() const = 0;
  virtual bool qos_cancelled() const = 0;
  virtual void qos_wait(std::uint32_t milliseconds) const = 0;
  virtual void qos_open(const MatchmakingQosOpen &) const = 0;
  virtual MatchmakingQosFrame qos_receive() const = 0;
  virtual MatchmakingQosSent qos_send(MatchmakingQosSendKind, rust::Slice<const std::uint8_t>) const = 0;
  virtual void qos_close() const = 0;
};
} // namespace halo2::online_protocol_bridge
