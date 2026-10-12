#pragma once
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>

namespace halo2 {
// Native service replacement; this is not part of the recovered Xbox runtime.
struct DiscordIdentity {
  std::string id, username, display_name;
};
enum class DiscordLoginStatus { SignedOut, Waiting, Verifying, SignedIn, Failed };
struct DiscordLoginSnapshot {
  DiscordLoginStatus status = DiscordLoginStatus::SignedOut;
  std::optional<DiscordIdentity> identity;
  std::string message;
};
struct DiscordLoginOptions {
  std::string client_id = "1558522073272426519";
  unsigned callback_port = 2003;
  std::chrono::milliseconds timeout = std::chrono::minutes(5);
  bool social_presence{}; // Only the explicit Friends consent flow requests this.
  bool social_communication{}; // Game invitations require sdk.social_layer.
};
struct DiscordHttpRequest {
  std::string url, form, bearer;
};
struct DiscordHttpResponse {
  long status{};
  std::string body;
};
class DiscordLogin {
public:
  using OpenBrowser = std::function<bool(const std::string &)>;
  // Injectable transport for deterministic tests. Production uses HTTPS with
  // certificate verification and fixed Discord endpoints, without redirects.
  using Http = std::function<DiscordHttpResponse(const DiscordHttpRequest &, std::stop_token)>;
  explicit DiscordLogin(OpenBrowser, DiscordLoginOptions = {}, Http = {});
  ~DiscordLogin();
  DiscordLogin(const DiscordLogin &) = delete;
  DiscordLogin &operator=(const DiscordLogin &) = delete;
  void start();
  void cancel(); // Also signs out; credentials are never persisted.
  DiscordLoginSnapshot snapshot() const;
  // Private service handoff only. Never put this credential in UI snapshots,
  // preferences or logs. It expires with the existing Discord sign-in.
  std::optional<std::string> authorization() const;
  std::optional<std::string> social_authorization() const;
private:
  friend class OnlineAccount;
  struct ServiceIdentity {std::string bearer, subject;};
  std::optional<ServiceIdentity> service_identity() const;
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
