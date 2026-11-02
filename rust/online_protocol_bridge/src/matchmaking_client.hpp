#pragma once
#include "rust/cxx.h"
#include <cstdint>

namespace halo2::online_protocol_bridge {
struct MatchmakingClientIdentity;
struct MatchmakingClientRequest;
struct MatchmakingClientResponse;
struct MatchmakingClientConnection;
struct MatchmakingClientAdmission;

// Native resource/IO observations only. Rust owns admission and transitions.
class NativeMatchmakingIo {
public:
  virtual ~NativeMatchmakingIo() = default;
  virtual std::uint64_t client_now_ms() const = 0;
  virtual bool client_cancelled(std::uint64_t version) const = 0;
  virtual MatchmakingClientIdentity client_identity(std::uint64_t version) const = 0;
  virtual MatchmakingClientResponse client_request(const MatchmakingClientRequest &) const = 0;
  virtual MatchmakingClientConnection client_connection(std::uint64_t version) const = 0;
  virtual bool client_install(std::uint64_t version, const MatchmakingClientAdmission &) const = 0;
  virtual void client_disconnect(std::uint64_t version) const = 0;
  virtual bool client_qos(std::uint64_t version, rust::Str endpoint, rust::Str bearer, bool loopback) const = 0;
};
} // namespace halo2::online_protocol_bridge
