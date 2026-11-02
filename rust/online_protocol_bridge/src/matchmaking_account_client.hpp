#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct AccountClientIdentity;
struct MatchmakingClientRequest;
struct MatchmakingClientResponse;
struct EosIdentity;
// Actual native mutex, clock, login, Curl, entropy and official-provider IO.
// The account/party controller and its sanitized snapshot belong to Rust.
class NativeAccountClientIo {
public:
  virtual ~NativeAccountClientIo()=default;
  virtual void account_lock()const=0;
  virtual void account_unlock()const=0;
  virtual std::int64_t account_now_ns()const=0;
  virtual AccountClientIdentity account_identity()const=0;
  virtual MatchmakingClientResponse account_request(const MatchmakingClientRequest &)const=0;
  virtual bool account_cancelled()const=0;
  virtual void account_reset_cancellation()const=0;
  virtual void account_cancel()const=0;
  virtual void account_wake()const=0;
  virtual rust::Vec<std::uint8_t> account_entropy()const=0;
  virtual bool account_eos_profiles()const=0;
  virtual EosIdentity account_eos_identity(rust::Str)const=0;
};
}
