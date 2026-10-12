#include "halo2/discord_activity_inbox.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <utility>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
struct Fixture {
  std::chrono::steady_clock::time_point now{};
  bool blocked{},join_allowed=true;
  unsigned accepts{},joins{};
  DiscordActivityInbox::Complete done;
  std::string joined_secret;
  DiscordActivityInbox inbox{[this](std::string secret) {
    ++joins;joined_secret=std::move(secret);return join_allowed;
  },[this]{return blocked?std::string{"Leave your current party first."}:std::string{};},[this]{return now;}};
  std::string receive(std::string key="111111111111111111:222222222222222222",std::string name="Duplicate") {
    inbox.received(std::move(key),std::move(name),true,[this](auto complete){++accepts;done=std::move(complete);});
    return inbox.snapshot().invitations.back().handle;
  }
};
}
int main() {
  try {
    const std::string secret="h2p1_"+std::string(43,'x');
    {
      Fixture f;const auto handle=f.receive();
      check(f.accepts==0 && f.joins==0,"Receiving an invite automatically accepted it");
      const auto snapshot=f.inbox.snapshot();
      check(snapshot.invitations.size()==1 && snapshot.invitations[0].name=="Duplicate" &&
        snapshot.invitations[0].handle.find("111111111111111111")==std::string::npos &&
        snapshot.invitations[0].handle.find("222222222222222222")==std::string::npos,"SDK IDs escaped into a menu handle");
      check(!f.inbox.accept("unknown") && f.inbox.accept(handle) && !f.inbox.accept(handle),"Stale or duplicate accept was submitted");
      check(f.accepts==1 && f.joins==0 && f.inbox.snapshot().busy,"Acceptance skipped the SDK result");
      const auto repeated=f.done;f.done(true,secret);repeated(true,secret);
      check(f.joins==1 && f.joined_secret==secret && !f.inbox.snapshot().busy && f.inbox.snapshot().invitations.empty(),
        "SDK acceptance duplicated the private join handoff");
      check(f.inbox.snapshot().message=="Joining ONLINE party...","Receipt claimed completed membership");
      check(f.inbox.snapshot().message.find(secret)==std::string::npos,"Join secret escaped into presentation");
    }
    {
      Fixture f;const auto first=f.receive("first","Duplicate");const auto second=f.receive("second","Duplicate");
      f.inbox.received("first","Renamed",true,[&](auto done){++f.accepts;f.done=std::move(done);});
      const auto rows=f.inbox.snapshot().invitations;
      check(rows.size()==2 && rows[0].handle==first && rows[1].handle==second && rows[0].name=="Renamed",
        "Duplicate names or SDK updates changed invitation ownership");
      check(f.inbox.accept(second),"Second duplicate invitation could not be selected");f.done(false,{});
      check(f.joins==0 && !f.inbox.snapshot().busy && f.inbox.snapshot().invitations[1].status=="Try accepting again.",
        "SDK rejection consumed an invite or hid its retry status");
      check(f.inbox.accept(second),"Rejected SDK invitation could not be retried");f.done(true,secret);
      check(f.joins==1 && f.inbox.snapshot().invitations[0].handle==first,"Retry accepted another duplicate row");
    }
    {
      Fixture f;const auto handle=f.receive();f.blocked=true;
      check(!f.inbox.accept(handle) && !f.accepts,"Current party/match gate reached the SDK");
      check(f.inbox.snapshot().invitations[0].status=="Cannot accept right now.","Blocked action lost its bounded UI status");
      f.blocked=false;check(f.inbox.accept(handle),"Unblocked invite was refused");f.blocked=true;f.done(true,secret);
      check(!f.joins && !f.inbox.snapshot().busy,"Party/match change during SDK acceptance started a join");
    }
    for(unsigned cancellation=0;cancellation<3;++cancellation) {
      Fixture f;const auto handle=f.receive();check(f.inbox.accept(handle),"Cancellation fixture was not started");
      if(cancellation==0)f.inbox.received("111111111111111111:222222222222222222","Duplicate",false,{});
      if(cancellation==1)check(f.inbox.dismiss(handle),"Explicit dismissal failed");
      if(cancellation==2)f.inbox.reset();
      f.done(true,secret);
      check(!f.joins && !f.inbox.snapshot().busy && f.inbox.snapshot().invitations.empty(),"Late SDK result revived a cancelled invitation");
      check(!f.inbox.accept(handle),"Removed SDK handle remained actionable");
    }
    {
      Fixture f;const auto handle=f.receive();check(f.inbox.accept(handle),"Timeout fixture was not started");
      f.now+=std::chrono::seconds(31);f.inbox.pump();f.done(true,secret);
      check(!f.joins && !f.inbox.snapshot().busy && f.inbox.snapshot().invitations[0].status=="Invitation timed out.",
        "Timed-out SDK callback initiated a join");
    }
    {
      Fixture f;const auto handle=f.receive();f.join_allowed=false;
      check(f.inbox.accept(handle),"Rejected-handoff fixture was not started");f.done(true,secret);
      check(f.joins==1 && f.inbox.snapshot().invitations.size()==1 && f.inbox.snapshot().invitations[0].status=="Invitation unavailable.",
        "Private join rejection was displayed as successful membership");
    }
    {
      Fixture f;const auto first=f.receive();check(f.inbox.accept(first),"Capacity fixture was not started");
      for(unsigned i=0;i<100;++i)f.receive("bounded-"+std::to_string(i));
      f.done(true,secret);
      check(f.inbox.snapshot().invitations.size()==100 && !f.inbox.accept(first) && !f.joins,
        "Capacity eviction retained SDK authority or grew an unbounded archive");
    }
    {
      Fixture f;const auto old=f.receive();f.inbox.reset();const auto fresh=f.receive();
      Fixture other;const auto other_handle=other.receive();
      check(old!=fresh && fresh!=other_handle && !f.inbox.accept(old),"Inbox reset or replacement recycled a stale handle");
      f.inbox.received("name-control",std::string(130,'W')+"\n\t",true,[](auto){});
      const auto name=f.inbox.snapshot().invitations.back().name;
      check(name.size()==128 && name.find('\n')==std::string::npos,"Untrusted invite name escaped its display bound");
      f.inbox.received("empty-name","\n\t",true,[](auto){});
      check(f.inbox.snapshot().invitations.back().name=="Discord player","Empty invite name lacks a privacy-safe fallback");
    }
    {
      DiscordActivityInbox::Complete done;unsigned joins{};
      {
        DiscordActivityInbox inbox([&](auto){++joins;return true;},[]{return std::string{};});
        inbox.received("private-message","Sender",true,[&](auto callback){done=std::move(callback);});
        check(inbox.accept(inbox.snapshot().invitations.front().handle),"Destroyed-owner fixture was not started");
      }
      done(true,secret);check(!joins,"Destroyed SDK owner accepted a late invitation");
    }
    {
      Fixture f;f.inbox.received("throwing-sdk","Sender",true,[](auto){throw std::runtime_error("private SDK detail");});
      check(f.inbox.accept(f.inbox.snapshot().invitations.front().handle) && !f.inbox.snapshot().busy,
        "SDK exception stranded an acceptance operation");
      check(f.inbox.snapshot().message.find("private SDK detail")==std::string::npos,"Arbitrary SDK error leaked into the UI");
    }
    std::cout<<"PASS: explicit SDK acceptance, private opaque rows, invalidation, duplicate names/callbacks, timeout, bounds and owner cleanup. No Discord messages sent.\n";
    return 0;
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
