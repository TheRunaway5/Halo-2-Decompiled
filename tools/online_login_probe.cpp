// Interactive operator check using the same Steam OpenID and account clients as the game.
// Only the public authorization URL and aggregate result are normally written.
// An explicit confirmation-file option is for a private, disposable fixture.
#include "halo2/online_account.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;
int main(int argc, char **argv) {
  if(argc<3 || argc%2==0){std::cerr<<"Usage: online_login_probe authorization-url-file screen-name [--steam-friends expected-count] [--confirmation-file private-path]\n";return 2;}
  bool friends{};unsigned long expected{};std::string confirmation_file;
  for(int i=3;i<argc;i+=2){
    if(std::string(argv[i])=="--steam-friends"){friends=true;expected=std::stoul(argv[i+1]);}
    else if(std::string(argv[i])=="--confirmation-file")confirmation_file=argv[i+1];
    else return 2;
  }
  bool wrote_confirmation{};
  bool requested_friends{};
  auto login = DiscordLogin::steam([&](const std::string &url) {
    std::ofstream file(argv[1]); file << url; return bool(file);
  });
  OnlineAccount account(login, online_account_environment());
  login->start();
  std::cout << "Waiting for Steam authorization in the browser.\n" << std::flush;
  auto deadline = std::chrono::steady_clock::now()+std::chrono::minutes(5);
  while (std::chrono::steady_clock::now()<deadline) {
    auto status=login->snapshot();
    const std::string code_prefix="Enter this code in your browser after Steam sign-in: ";
    if(!confirmation_file.empty() && !wrote_confirmation && status.message.starts_with(code_prefix)) {
      auto code=status.message.substr(code_prefix.size(),12);
      if(code.size()!=12 || !std::all_of(code.begin(),code.end(),[](char c){return (c>='0'&&c<='9')||(c>='A'&&c<='F');}))return 1;
      std::ofstream file(confirmation_file);
      std::filesystem::permissions(confirmation_file,std::filesystem::perms::owner_read|std::filesystem::perms::owner_write,std::filesystem::perm_options::replace);
      file<<code;if(!file)return 1;wrote_confirmation=true;
    }
    if (status.status==DiscordLoginStatus::Failed) { std::cerr<<status.message<<'\n';return 1; }
    if (status.identity) {
      account.connect(argv[2]); auto s=account.snapshot();
      if (s.authenticated && !s.busy) {
        if(friends && !requested_friends) {account.refresh_steam_friends();requested_friends=true;continue;}
        if(friends && s.steam_friends_state=="loading")continue;
        if(friends && (s.steam_friends_state!="ready" || s.steam_friends.size()!=expected)) {
          std::cerr<<"Steam friend response did not admit the expected bounded row count: "<<s.steam_friends_state<<" / "<<s.steam_friends.size()<<'\n';return 1;
        }
        if(friends)std::cout<<"PASS native Steam friend reply count "<<s.steam_friends.size()<<".\n";
        std::cout<<"PASS native Steam OpenID handoff, ONLINE account session, private clan snapshot and sign-out. Clan membership: "<<(s.clan ? "present" : "none")<<".\n";
        account.sign_out(); login->cancel(); return 0;
      }
      if (!s.busy && !s.message.empty() && s.message!="Sign in with Steam.") { std::cerr<<s.message<<'\n';return 1; }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  std::cerr<<"Sign-in timed out.\n";return 1;
}
