#pragma once
#include "halo2/discord_login.hpp"
#include "halo2/online_service.hpp"

namespace halo2 {
struct SteamLoginOptions {
  std::string origin = default_online_origin;
  bool allow_loopback_http{};
  std::chrono::milliseconds timeout = std::chrono::minutes(5);
};
SteamLoginOptions steam_login_environment();
// Browser OpenID verified by the account service, then a private S256-bound
// handoff. No Steam password, Web API key or Steamworks app ID enters the game.
class SteamLogin {
public:
  SteamLogin(DiscordLogin::OpenBrowser, SteamLoginOptions = {}, DiscordLogin::Http = {});
  ~SteamLogin();
  void start();
  void cancel();
  DiscordLoginSnapshot snapshot() const;
  std::optional<std::string> authorization() const;
  bool link_discord(std::string private_discord_token);
  DiscordLinkSnapshot discord_link() const;
  void cancel_discord_link();
private:
  friend class DiscordLogin;
  struct Credential { std::string bearer, subject; };
  std::optional<Credential> credential() const;
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
