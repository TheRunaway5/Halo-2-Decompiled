// Interactive operator check using the same PKCE and account clients as the game.
// Only the public authorization URL and aggregate result are written to disk.
#include "halo2/online_account.hpp"
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;
int main(int argc, char **argv) {
  if (argc != 3) { std::cerr << "Usage: online_login_probe authorization-url-file screen-name\n"; return 2; }
  auto login = std::make_shared<DiscordLogin>([&](const std::string &url) {
    std::ofstream file(argv[1]); file << url; return bool(file);
  });
  OnlineAccount account(login, online_account_environment());
  login->start();
  std::cout << "Waiting for Discord authorization in the browser.\n" << std::flush;
  auto deadline = std::chrono::steady_clock::now()+std::chrono::minutes(5);
  while (std::chrono::steady_clock::now()<deadline) {
    auto status=login->snapshot();
    if (status.status==DiscordLoginStatus::Failed) { std::cerr<<status.message<<'\n';return 1; }
    if (status.identity) {
      account.connect(argv[2]); auto s=account.snapshot();
      if (s.authenticated && !s.busy) {
        std::cout<<"PASS live Discord PKCE, application/scope verification, HTTPS ONLINE account session, private clan snapshot and sign-out. Clan membership: "<<(s.clan ? "present" : "none")<<".\n";
        account.sign_out(); login->cancel(); return 0;
      }
      if (!s.busy && !s.message.empty() && s.message!="Sign in with Discord.") { std::cerr<<s.message<<'\n';return 1; }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  std::cerr<<"Sign-in timed out.\n";return 1;
}
