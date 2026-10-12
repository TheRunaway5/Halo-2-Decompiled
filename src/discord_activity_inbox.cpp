#include "halo2/discord_activity_inbox.hpp"
#include <algorithm>
#include <atomic>
#include <utility>

namespace halo2 {
struct DiscordActivityInbox::Impl {
  struct Entry {std::string key;DiscordGameInvite view;Accept accept;};
  static std::atomic<std::uint64_t> owners;
  const std::uint64_t owner=++owners;
  std::uint64_t sequence{},epoch{};
  Join join;
  CanAccept can_accept;
  Now now;
  DiscordActivityInboxSnapshot state;
  std::vector<Entry> entries;
  std::string accepting;
  std::chrono::steady_clock::time_point deadline{};
  std::shared_ptr<bool> alive=std::make_shared<bool>(true);
  void changed() {
    state.invitations.clear();
    for(const auto &entry:entries)state.invitations.push_back(entry.view);
    ++state.revision;
  }
  void finish(std::string message,std::string label={}) {
    if(!label.empty())for(auto &entry:entries)if(entry.view.handle==accepting)entry.view.status=label;
    ++epoch;accepting.clear();state.busy=false;state.message=std::move(message);changed();
  }
  void erase(std::string handle) {
    std::erase_if(entries,[&](const auto &entry){return entry.view.handle==handle;});
    if(accepting==handle)finish("This Discord invitation is no longer available.");
    else changed();
  }
};
std::atomic<std::uint64_t> DiscordActivityInbox::Impl::owners{};
DiscordActivityInbox::DiscordActivityInbox(Join join,CanAccept can_accept,Now now)
  :impl_(std::make_unique<Impl>()) {
  impl_->join=std::move(join);impl_->can_accept=std::move(can_accept);
  impl_->now=now?std::move(now):Now{[]{return std::chrono::steady_clock::now();}};
}
DiscordActivityInbox::~DiscordActivityInbox(){reset();*impl_->alive=false;}
void DiscordActivityInbox::reset() {
  auto &p=*impl_;*p.alive=false;p.alive=std::make_shared<bool>(true);
  p.entries.clear();p.finish({});
}
DiscordActivityInboxSnapshot DiscordActivityInbox::snapshot() const{return impl_->state;}
void DiscordActivityInbox::received(std::string key,std::string name,bool valid,Accept accept) {
  auto &p=*impl_;if(key.empty() || key.size()>128)return;
  const auto found=std::find_if(p.entries.begin(),p.entries.end(),[&](const auto &entry){return entry.key==key;});
  if(!valid || !accept) {
    if(found!=p.entries.end())p.erase(found->view.handle);
    return;
  }
  if(name.size()>128)name.resize(128);
  std::erase_if(name,[](unsigned char c){return c<32 || c==127;});
  if(name.empty())name="Discord player";
  if(found!=p.entries.end()) {
    found->view.name=std::move(name);found->accept=std::move(accept);p.changed();return;
  }
  // Native UI policy: retain at most100 recent invitations, independently
  // of the SDK's lifetime. Never grow a private message/SDK-object archive.
  if(p.entries.size()==100) {const auto handle=p.entries.front().view.handle;p.erase(handle);}
  const auto handle="activity-"+std::to_string(p.owner)+"-"+std::to_string(++p.sequence);
  p.entries.push_back({std::move(key),{handle,std::move(name)},std::move(accept)});p.changed();
}
bool DiscordActivityInbox::dismiss(const std::string &handle) {
  auto &p=*impl_;
  if(std::none_of(p.entries.begin(),p.entries.end(),[&](const auto &entry){return entry.view.handle==handle;}))return false;
  p.erase(handle);return true;
}
bool DiscordActivityInbox::accept(const std::string &handle) {
  auto &p=*impl_;if(p.state.busy || !p.join || !p.can_accept)return false;
  const auto found=std::find_if(p.entries.begin(),p.entries.end(),[&](const auto &entry){return entry.view.handle==handle;});
  if(found==p.entries.end())return false;
  if(const auto reason=p.can_accept();!reason.empty()) {
    p.accepting=handle;p.finish(reason,"Cannot accept right now.");return false;
  }
  const auto action=found->accept;
  const auto epoch=++p.epoch;const auto weak=std::weak_ptr<bool>(p.alive);
  p.accepting=handle;p.state.busy=true;p.state.message="Accepting Discord invitation...";
  found->view.status="Accepting invitation...";
  p.deadline=p.now()+std::chrono::seconds(30);p.changed();
  try {
    action([&p,weak,epoch,handle](bool successful,std::string secret) {
      const auto live=weak.lock();if(!live || !*live || epoch!=p.epoch || !p.state.busy)return;
      if(!successful){p.finish("Discord could not accept this invitation. Try again.","Try accepting again.");return;}
      if(!p.can_accept().empty()) {p.finish("Return to ONLINE and leave your current party before accepting this invitation.","Cannot accept right now.");return;}
      bool joined{};
      try {joined=p.join(std::move(secret));}catch (...) {}
      if(!joined){p.finish("This Discord party invitation could not be accepted.","Invitation unavailable.");return;}
      // This acknowledges only the private join handoff. The account worker
      // supplies actual membership and failure status after server validation.
      std::erase_if(p.entries,[&](const auto &entry){return entry.view.handle==handle;});
      p.finish("Joining ONLINE party...");
    });
  } catch (...) {
    if(epoch==p.epoch && p.state.busy)p.finish("Discord could not accept this invitation. Try again.","Try accepting again.");
  }
  return true;
}
void DiscordActivityInbox::pump() {
  auto &p=*impl_;
  if(p.state.busy && p.now()>=p.deadline)p.finish("Discord invitation timed out. Try again.","Invitation timed out.");
}
} // namespace halo2
