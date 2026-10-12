#include "halo2/discord_login.hpp"
#include "halo2/discord_social.hpp"
#include <chrono>
#include <iostream>
#include <thread>
using namespace halo2;
int main() {
  try {
    DiscordLoginOptions options;options.social_presence=true;
    auto browser=[](const std::string &url){std::cout<<"AUTHORIZE "<<url<<std::endl;return true;};
    auto identity=std::make_shared<DiscordLogin>(browser,options);
    DiscordSocial friends(identity,browser,options);
    if(!friends.snapshot().available)throw std::runtime_error("Official Social SDK is unavailable");
    identity->start();const auto deadline=std::chrono::steady_clock::now()+std::chrono::minutes(5);
    bool enabled=false;
    while(std::chrono::steady_clock::now()<deadline) {
      const auto login=identity->snapshot();
      if(login.status==DiscordLoginStatus::Failed)throw std::runtime_error(login.message);
      if(login.identity && !enabled) {friends.enable();enabled=true;}
      friends.pump();const auto state=friends.snapshot();
      if(state.ready) {
        std::size_t count{},playing{},incoming{};
        for(const auto &f:state.friends) {
          if(f.kind==DiscordFriendKind::Friend)++count;
          if(f.presence==DiscordFriendPresence::Playing)++playing;
          if(f.kind==DiscordFriendKind::Incoming)++incoming;
          if(f.handle.find(login.identity->id)!=std::string::npos)throw std::runtime_error("Private identity leaked into handle");
        }
        friends.disconnect();identity->cancel();
        if(!friends.snapshot().friends.empty() || identity->authorization())throw std::runtime_error("Sign-out retained private data");
        std::cout<<"PASS real Discord Social SDK authenticated the same account; friends="<<count<<" playing="<<playing
                 <<" incoming="<<incoming<<"; no names, IDs or tokens logged; sign-out cleared state"<<std::endl;
        return 0;
      }
      if(enabled && !state.connecting)throw std::runtime_error(state.message);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    throw std::runtime_error("Discord authorization timed out");
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
