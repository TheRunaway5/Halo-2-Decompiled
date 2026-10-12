#include "halo2/discord_party_activity.hpp"
#include "halo2/online_account.hpp"
#include <algorithm>
#include <utility>

namespace halo2 {
struct DiscordPartyActivity::Impl {
  enum class Stage { Idle, Preparing, Publishing, Sending, AwaitingIdentity, Joining };
  std::shared_ptr<OnlineAccount> account;
  Publish publish;
  Send send;
  DiscordPartyActivityStatus state;
  Stage stage=Stage::Idle;
  bool playing{};
  std::string target, secret;
  std::uint64_t operation{}, epoch{};
  std::optional<DiscordPartyLink> cached_link;
  std::chrono::steady_clock::time_point deadline{};
  std::shared_ptr<bool> alive=std::make_shared<bool>(true);
  void status(bool busy,std::string message) {
    state.busy=busy;state.message=std::move(message);++state.revision;
  }
  void finish(std::string message) {
    ++epoch;stage=Stage::Idle;target.clear();secret.clear();operation=0;
    status(false,std::move(message));
  }
  std::optional<DiscordPartyLink> link() const {
    if(playing || !cached_link || !account)return {};
    const auto value=account->discord_party_link();
    if(!value || value->join_secret!=cached_link->join_secret ||
       value->party_id!=cached_link->party_id)return {};
    return value;
  }
};
DiscordPartyActivity::DiscordPartyActivity(std::shared_ptr<OnlineAccount> account,Publish publish,Send send)
  :impl_(std::make_unique<Impl>()) {
  impl_->account=std::move(account);impl_->publish=std::move(publish);impl_->send=std::move(send);
}
DiscordPartyActivity::~DiscordPartyActivity(){reset();*impl_->alive=false;}
void DiscordPartyActivity::reset() {
  auto &p=*impl_;*p.alive=false;p.alive=std::make_shared<bool>(true);
  p.cached_link.reset();p.finish({});
}
DiscordPartyActivityStatus DiscordPartyActivity::status() const{return impl_->state;}
std::optional<DiscordPartyLink> DiscordPartyActivity::link() const{return impl_->link();}
bool DiscordPartyActivity::invite(const std::string &subject) {
  auto &p=*impl_;
  if(p.state.busy || p.playing || !p.account || !p.publish || !p.send)return false;
  const auto operation=p.account->begin_discord_invite(subject);
  if(!operation){p.finish("Refresh your ONLINE party before inviting this friend.");return false;}
  ++p.epoch;p.operation=operation;p.target=subject;
  p.stage=Impl::Stage::Preparing;
  p.deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
  p.status(true,"Preparing party invitation...");return true;
}
bool DiscordPartyActivity::join(std::string secret) {
  auto &p=*impl_;
  if(p.state.busy || !p.account)return false;
  if(p.playing){p.finish("Return to ONLINE before accepting this party invitation.");return false;}
  if(secret.size()!=48 || !secret.starts_with("h2p1_") ||
     !std::all_of(secret.begin()+5,secret.end(),[](unsigned char v) {
       return (v>='A' && v<='Z') || (v>='a' && v<='z') || (v>='0' && v<='9') || v=='_' || v=='-';
     })) {p.finish("This Discord party invitation is unavailable.");return false;}
  if(p.account->snapshot().party){p.finish("Leave your current party before accepting this invitation.");return false;}
  ++p.epoch;p.secret=std::move(secret);p.stage=Impl::Stage::AwaitingIdentity;
  // The server remains the authority for expiry and recipient membership.
  // A cold launch may need the user's normal ONLINE sign-in first.
  p.deadline=std::chrono::steady_clock::now()+std::chrono::minutes(10);
  p.status(true,"Sign into ONLINE to accept the Discord party invitation.");return true;
}
void DiscordPartyActivity::pump(bool playing) {
  auto &p=*impl_;p.playing=playing;
  if(playing) {
    p.cached_link.reset();
    if(p.state.busy)p.finish("Return to ONLINE before using party invitations.");
    return;
  }
  if(!p.state.busy)return;
  if(std::chrono::steady_clock::now()>=p.deadline) {
    p.finish("Party invitation timed out. Refresh ONLINE and try again.");return;
  }
  if(p.stage==Impl::Stage::Publishing && !p.link()) {
    p.cached_link.reset();p.finish("Party changed before Discord could send the invitation. Try again.");return;
  }
  if(p.stage==Impl::Stage::AwaitingIdentity) {
    const auto account=p.account->snapshot();
    if(!account.authenticated || account.busy)return;
    if(account.party){p.finish("Leave your current party before accepting this invitation.");return;}
    p.operation=p.account->begin_discord_join(p.secret);
    p.secret.clear();
    if(!p.operation){p.finish("This Discord party invitation could not be accepted.");return;}
    p.stage=Impl::Stage::Joining;
    p.deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
    p.status(true,"Joining ONLINE party...");return;
  }
  if(p.stage!=Impl::Stage::Preparing && p.stage!=Impl::Stage::Joining)return;
  const auto value=p.account->discord_operation(p.operation);
  if(value && value->pending)return;
  if(!value || !value->successful) {
    p.finish("Discord party invitation unavailable. Refresh your ONLINE party and try again.");return;
  }
  if(p.stage==Impl::Stage::Joining){p.finish("Joined ONLINE party from Discord.");return;}
  if(!value->link){p.finish("Discord party invitation unavailable. Refresh ONLINE and try again.");return;}
  p.cached_link=value->link;p.stage=Impl::Stage::Publishing;
  p.status(true,"Updating Discord party presence...");
  const auto weak=std::weak_ptr<bool>(p.alive);const auto epoch=p.epoch;
  try {
    p.publish(*p.cached_link,[&p,weak,epoch](bool successful) {
      const auto live=weak.lock();if(!live || !*live || epoch!=p.epoch || p.stage!=Impl::Stage::Publishing)return;
      if(!successful || !p.link()) {
        p.finish("ONLINE invitation created; Discord presence could not be updated.");return;
      }
      p.stage=Impl::Stage::Sending;p.status(true,"Sending Discord party invitation...");
      try {
        p.send(p.target,[&p,weak,epoch](bool sent) {
          const auto current=weak.lock();if(!current || !*current || epoch!=p.epoch || p.stage!=Impl::Stage::Sending)return;
          p.finish(sent?"Discord party invitation sent.":"ONLINE invitation created; Discord could not send the invitation.");
        });
      } catch (...) {
        if(epoch==p.epoch && p.stage==Impl::Stage::Sending)
          p.finish("ONLINE invitation created; Discord could not send the invitation.");
      }
    });
  } catch (...) {
    if(epoch==p.epoch && p.stage==Impl::Stage::Publishing)
      p.finish("ONLINE invitation created; Discord presence could not be updated.");
  }
}
} // namespace halo2
