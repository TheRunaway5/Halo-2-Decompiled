#include "halo2/online_match_session.hpp"
#include "halo2/frontend.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>
#include <thread>
using namespace halo2;
using Json=nlohmann::json;
namespace {
const char *name(OnlineMatchSession::Phase phase) {
  switch(phase) {
    case OnlineMatchSession::Phase::Connecting:return "connecting";
    case OnlineMatchSession::Phase::Loading:return "loading";
    case OnlineMatchSession::Phase::Running:return "running";
    case OnlineMatchSession::Phase::Reviewing:return "reviewing";
    case OnlineMatchSession::Phase::Postgame:return "postgame";
    case OnlineMatchSession::Phase::AwaitingWorld:return "awaiting_world";
    case OnlineMatchSession::Phase::Migrating:return "migrating";
    case OnlineMatchSession::Phase::Rebinding:return "rebinding";
    default:return "failed";
  }
}
template<class F>void wait(OnlineMatchSession &session,F predicate) {
  const auto limit=std::chrono::steady_clock::now()+std::chrono::seconds(5);
  while(!predicate()) {
    if(session.phase()==OnlineMatchSession::Phase::Failed)throw FormatError(session.error());
    if(std::chrono::steady_clock::now()>limit)throw FormatError("Native continuation transport timed out");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
}
Json status(const OnlineMatchSession &session) {
  const auto state=session.continuity().value_or(OnlineContinuityState{});
  Json result={{"self",session.self()},{"phase",name(session.phase())},
      {"host",session.host()},{"epoch",state.epoch},{"members",state.members}};
  if(state.selected)result["tick"]=state.selected->tick;
  return result;
}
}
int main()try {
  std::string line;std::getline(std::cin,line);const auto config=Json::parse(line);
  const auto &descriptor=config.at("descriptor");std::vector<OnlineAdmission> roster;
  for(const auto &row:descriptor.at("players")) {
    OnlineAdmission player;player.slot=unsigned(roster.size());player.name=row.at("name");
    player.team=row.at("team");player.peer=row.at("peer");player.guest=row.at("guest");
    const auto identity=row.at("identity").get<std::string>();
    for(unsigned i=0;i<8;++i)player.identity[i]=std::byte(std::stoul(identity.substr(i*2,2),nullptr,16));
    roster.push_back(player);
  }
  auto make_session=[&] {
    if(descriptor.value("continuity",std::string{})=="complete-world-v1")
      return std::make_unique<OnlineMatchSession>(config.at("origin"),descriptor.at("id"),descriptor.at("token"),
          descriptor.at("slot"),std::move(roster),true,descriptor.at("team_mode").get<bool>(),true);
    return std::make_unique<OnlineMatchSession>(config.at("origin"),descriptor.at("id"),descriptor.at("token"),
        descriptor.at("slot"),std::move(roster),true,descriptor.at("team_mode").get<bool>());
  };
  auto retained=make_session();auto &session=*retained;
  wait(session,[&]{return session.phase()==OnlineMatchSession::Phase::Loading;});
  session.loaded(0x1234,UiVariant{"Slayer",0,{}});
  wait(session,[&]{return session.phase()==OnlineMatchSession::Phase::Running;});
  std::cout<<status(session).dump()<<std::endl;
  while(std::getline(std::cin,line)) {
    const auto command=Json::parse(line);const auto action=command.at("action").get<std::string>();
    if(action=="stop"){std::cout<<"{\"stopped\":true}"<<std::endl;break;}
    if(action=="wait") {
      const auto expected=command.at("phase").get<std::string>();
      wait(session,[&]{return name(session.phase())==expected;});
    } else if(action=="seal")session.seal_world(100,std::string(64,'a'));
    else if(action=="ack") {
      std::vector<OnlineWorldSeal> seals;
      wait(session,[&]{seals=session.world_seals();return !seals.empty();});
      for(const auto &seal:seals)session.acknowledge_world(seal);
    } else if(action=="accept")session.accept_migration(100,std::string(64,'a'));
    else if(action=="rebind")session.rebind_migration(100,std::string(64,'a'));
    else if(action=="reject-wrong-world") {
      bool rejected=false;try{session.accept_migration(100,std::string(64,'b'));}catch(const std::exception&){rejected=true;}
      if(!rejected)throw FormatError("Native client accepted an unvalidated replacement world");
    } else if(action=="send") {
      std::vector<std::byte> packet{std::byte{'H'},std::byte{'2'},std::byte{'N'},std::byte{'2'},
          std::byte(native_multiplayer_protocol&255),std::byte(native_multiplayer_protocol>>8),std::byte{1}};
      packet.insert(packet.end(),20,std::byte{'x'});session.send(packet,session.host());
    } else if(action=="receive") {
      std::vector<std::byte> packet;unsigned peer{};
      wait(session,[&]{return session.receive(packet,peer);});
      if(peer!=command.at("peer").get<unsigned>() || packet.size()!=27 || packet[6]!=std::byte{1} ||
          std::to_integer<unsigned>(packet[4])+(std::to_integer<unsigned>(packet[5])<<8)!=native_multiplayer_protocol)
        throw FormatError("Reestablished native route changed source or packet");
    } else if(action!="status")throw FormatError("Unknown native continuation test command");
    std::cout<<status(session).dump()<<std::endl;
  }
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
