#pragma once
#include "rust/cxx.h"
#include <memory>

namespace halo2::online_protocol_bridge {
struct EosConfig;
struct EosEvent;
struct EosSession;
struct EosInviteInfo;
bool sdk_process_shutdown()noexcept;
class RawEos {
public:
  explicit RawEos(const EosConfig &);
  ~RawEos();
  void sdk_device();
  void sdk_login();
  void sdk_external_login(rust::Str token);
  rust::String sdk_id_token()const;
  void sdk_create_user();
  void sdk_socket(rust::Str name, bool force_relay);
  rust::Vec<EosEvent> sdk_poll();
  bool sdk_accept(rust::Str user);
  bool sdk_send(rust::Str user, rust::Slice<const std::uint8_t> bytes, bool delayed);
  void sdk_session_create(const EosSession &,std::uint32_t capacity);
  void sdk_session_invite(rust::Str user);
  void sdk_session_query();
  void sdk_session_register(rust::Str user);
  EosInviteInfo sdk_session_info(rust::Str invite)const;
  void sdk_session_join(rust::Str invite);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
std::unique_ptr<RawEos> new_raw_eos(const EosConfig &);
bool eos_sdk_available() noexcept;
}
