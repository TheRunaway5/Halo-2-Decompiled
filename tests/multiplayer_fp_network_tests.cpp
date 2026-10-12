#include "halo2/animation_presentation.hpp"
#include "halo2/multiplayer_network.hpp"
#include <bit>
#include <iostream>
#include <set>
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
#ifndef _WIN32
struct Proxy {
  using Packet=std::vector<std::byte>;
  int fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
  sockaddr_in host{},client{};
  explicit Proxy(unsigned port) {
    check(fd>=0,"Create real UDP proxy");sockaddr_in bind_at{};
    bind_at.sin_family=AF_INET;bind_at.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    check(bind(fd,reinterpret_cast<sockaddr *>(&bind_at),sizeof(bind_at))==0,"Bind real UDP proxy");
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0)|O_NONBLOCK);
    host.sin_family=AF_INET;host.sin_addr.s_addr=htonl(INADDR_LOOPBACK);host.sin_port=htons(port);
  }
  ~Proxy(){if(fd>=0)close(fd);}
  unsigned port() const {
    sockaddr_in at{};socklen_t n=sizeof(at);
    check(getsockname(fd,reinterpret_cast<sockaddr *>(&at),&n)==0,"Read real UDP proxy port");return ntohs(at.sin_port);
  }
  std::vector<Packet> pump(bool drop_inputs=false) {
    std::vector<Packet> responses;
    for(unsigned n=0;n<256;++n) {
      Packet p(1401);sockaddr_in from{};socklen_t size=sizeof(from);
      const auto count=recvfrom(fd,p.data(),p.size(),0,reinterpret_cast<sockaddr *>(&from),&size);
      if(count<=0)break;p.resize(count);
      if(from.sin_port==host.sin_port && from.sin_addr.s_addr==host.sin_addr.s_addr)responses.push_back(std::move(p));
      else {
        client=from;
        if(!drop_inputs)check(sendto(fd,p.data(),p.size(),0,reinterpret_cast<const sockaddr *>(&host),sizeof(host))==count,"Forward actual client input");
      }
    }
    return responses;
  }
  void deliver(const std::vector<Packet> &packets,bool reverse,bool duplicate) {
    for(unsigned n=0;n<packets.size();++n) {
      const auto &p=packets[reverse?packets.size()-1-n:n];
      check(p.size()<=1400,"Clock replication exceeded UDP datagram bound");
      for(unsigned copy=0;copy<(duplicate && n==0?2u:1u);++copy)
        check(sendto(fd,p.data(),p.size(),0,reinterpret_cast<const sockaddr *>(&client),sizeof(client))==int(p.size()),"Forward actual snapshot fragment");
    }
  }
};
#endif
}
int main(int argc,char **argv) {
  try {
#ifdef _WIN32
    return 77;
#else
    if(argc!=4)throw FormatError("Supply original MAP SHARED_MAPS UPDATE_XBE");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
    const Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    const auto content=cache.load_content(0,true);auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});
    rules.weapon=cache.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
    const auto definition=cache.load_first_person_binding(rules.weapon,2);
    MultiplayerMatch server(content,0,cache.spawns(),rules),observed(content,0,cache.spawns(),rules);
    server.configure_replicated_first_person(definition);
    LanHost host(server,0,0x168866);Proxy proxy(host.port());
    LanClient client("127.0.0.1:"+std::to_string(proxy.port()),0x168866,4);
    std::array<Controls,16> commands{};std::array<Controls,4> local{};
    for(unsigned n=0;n<64 && !client.ready();++n) {
      client.send(local);proxy.pump();host.receive(commands);host.broadcast();
      proxy.deliver(proxy.pump(),true,true);client.receive();
    }
    check(client.ready() && client.slots().size()==4,"Join four real native UDP views");
    for(unsigned id=0;id<16;++id) {
      if(id>=4)check(server.join(std::string(80,'A'+id%26))==id,"Join full native UDP roster");
      check(observed.join(id<4?"LAN Player":std::string(80,'A'+id%26))==id,"Join corresponding source-observed roster");
      commands[id].yaw=server.unit(id).yaw;
      if(id<4)local[id].yaw=commands[id].yaw;
      // Explicit recent-damage state exercises the full16-player transport
      // through fade, fragment loss/reorder and retirement. Original report
      // values and genuine projectile producers have their separate suites.
      const DamageFeedbackState feedback{
          {float(id+1)/16.f,.75f,std::int8_t(id)},
          {float(16-id)/16.f,.5f,std::int8_t(id*3)}};
      const_cast<Unit &>(server.unit(id)).damage_feedback=feedback;
      const_cast<Unit &>(observed.unit(id)).damage_feedback=feedback;
    }
    const std::array<unsigned,4> ids{0,1,2,3};observed.configure_local_views(ids,definition);
    std::array<FirstPersonViewBinding,4> views{},origins{};
    std::vector<Proxy::Packet> stale;std::array<std::set<std::uint32_t>,4> activities;
    unsigned sampled{},fractional{},maximum_fragments{},cuts{};
    for(unsigned tick=0;tick<270;++tick) {
      local[0].melee=tick==40 || tick==50;
      local[0].fire=tick>=6 && tick<20;local[0].reload=tick==24;
      local[1].fire=tick<210;local[1].reload=false;
      local[2].fire=false;local[2].reload=false;
      local[3].fire=tick>=100 && tick<110;local[3].reload=tick==115;
      client.send(local);
      if(tick==40 || tick==50) {
        if(tick==50)proxy.pump(true);
        auto release=local;release[0].melee=false;client.send(release);
      }
      if(tick==24) {auto release=local;release[0].reload=false;client.send(release);}
      if(tick==115) {
        proxy.pump(true);auto release=local;release[3].reload=false;client.send(release);
      }
      proxy.pump();host.receive(commands);
      if(tick==24)check(commands[0].reload,"Coalesced short R press was lost before original reload owner");
      if(tick==25)check(!commands[0].reload,"Coalesced short R press was delivered twice");
      if(tick==115)check(commands[3].reload,"Lost R press packet was not recovered from its newer release");
      if(tick==116)check(!commands[3].reload,"Recovered short R press was delivered twice");
      if(tick==40 || tick==50)check(commands[0].melee,"Short/lost F press was not recovered from its newer release");
      if(tick==41 || tick==51)check(!commands[0].melee,"Recovered F press was delivered twice");
      for(unsigned id=1;id<4;++id)check(!commands[id].melee,"Melee delivery leaked to another participant");
      server.step(commands);observed.step(commands);host.broadcast();
      auto packets=proxy.pump();maximum_fragments=std::max(maximum_fragments,unsigned(packets.size()));
      const auto prior_tick=client.state().tick;
      if(tick==60) {stale=packets;client.receive();check(client.state().tick==prior_tick,"Lost snapshot published partial clock");continue;}
      proxy.deliver(packets,tick%2,tick%3==0);client.receive();
      check(client.state().tick==server.state().tick,"Complete clock snapshot did not recover after UDP loss/reorder");
      if(tick==61) {proxy.deliver(stale,true,true);client.receive();check(client.state().tick==server.state().tick,"Late clock snapshot rolled back rendered state");}
      for(unsigned id=0;id<16;++id) {
        const auto &received=client.state().players[id].damage_feedback;
        const auto &actual=server.unit(id).damage_feedback;
        const auto same=[](const auto &a,const auto &b) {
          return std::bit_cast<std::uint32_t>(a.visible)==std::bit_cast<std::uint32_t>(b.visible) &&
            std::bit_cast<std::uint32_t>(a.accumulated)==std::bit_cast<std::uint32_t>(b.accumulated) &&
            a.ticks==b.ticks;
        };
        check(same(received.shield,actual.shield)&&same(received.body,actual.body),
            "Full-roster damage feedback changed across fragment loss/reorder/duplicate");
      }
      for(unsigned id=0;id<4;++id) {
        const auto &player=client.state().players[id];const auto &actual=observed.first_person_view(id);
        observe_multiplayer_first_person(views[id],player,definition);
        if(player.source_unit==null_datum) {check(!player.first_person_clock.present,"Released unit retained native clock presence");continue;}
        check(apply_replicated_first_person_clock(origins[id],views[id],player.first_person_clock,definition),"Replicated source clock absent for current view");
        check(same_first_person_channel(first_person_channel_state(origins[id].hands[0]),
            first_person_channel_state(observed.first_person_clock_origin(id).hands[0])),"Dedicated presentation origin differs from actual local source clock");
        check(same_first_person_channel(first_person_channel_state(views[id].hands[0]),
            first_person_channel_state(actual.hands[0])),"Dedicated presentation output differs from actual local source clock");
        check(!server.unit(id).weapon.smg.hand_timer_registered && observed.unit(id).weapon.smg.hand_timer_registered,
            "Native clock replication installed a server gameplay hand-feedback owner");
        check(player.loaded==observed.unit(id).weapon.loaded && player.reserve==observed.unit(id).weapon.reserve &&
            player.health==observed.unit(id).vitality.body && player.shield==observed.unit(id).vitality.shield,
            "Native presentation clock changed ammunition or vitality");
        activities[id].insert(views[id].hands[0].activity);
        for(unsigned hz:{30,60,120,144,240}) {
          const unsigned first=(tick*hz+29)/30,last=((tick+1)*hz+29)/30;
          for(unsigned frame=first;frame<last;++frame) {
            const float fraction=float(double(frame)*30/double(hz)-double(tick));
            const auto before=first_person_channel_state(views[id].hands[0]);
            // The complete received pair itself proves a consecutive clock
            // interval, even when an older network snapshot was missed.
            const auto output=first_person_render_frame(origins[id],views[id],definition,fraction,true);
            check(same_first_person_channel(before,first_person_channel_state(views[id].hands[0])),"LAN render sampling mutated source clock/events");
            if(tick==61) {
              const auto same_interval=first_person_render_frame(observed.first_person_clock_origin(id),actual,definition,fraction,true);
              check(std::bit_cast<std::uint32_t>(output.frame)==std::bit_cast<std::uint32_t>(same_interval.frame),
                  "Recovered LAN pair sampled across a missing interval");
            }
            sampled++;fractional+=output.interpolated && fraction>0 && fraction<1 && output.frame!=before.frame;
            cuts+=!output.interpolated;
          }
        }
      }
    }
    check(maximum_fragments>=3 && sampled>10000 && fractional>1000 && cuts>0,"Full roster fragmentation and high-refresh LAN coverage missing");
    check(activities[0].contains(0x0b000065) && activities[0].contains(0x06000006) &&
        activities[1].contains(0x0c000064) && activities[2].size()==1 && activities[3].contains(0x0b000065),
        "Real independent fire/manual reload/empty reload/idle channels missing");
    local={};local[2].jump=local[2].fire=true;client.send(local);local[2].jump=local[2].fire=false;client.send(local);
    proxy.pump();host.receive(commands);
    check(commands[2].jump && commands[2].fire && !commands[0].jump && !commands[0].fire,
        "Coalesced Space/LMB presses lost or leaked between assigned views");
    host.receive(commands);check(!commands[2].jump && !commands[2].fire,"Coalesced Space/LMB press delivered twice");
    // A rendered client can drain several actual host ticks at once. Preserve
    // every complete accepted interval for animation/audio owners instead of
    // collapsing them to the most recent state. No authority tick is supplied.
    std::vector<MultiplayerState> delivered;
    for(unsigned batch=0;batch<3;++batch) {
      server.step(commands);observed.step(commands);host.broadcast();
      delivered.push_back(server.state());
      proxy.deliver(proxy.pump(),true,true);
    }
    client.receive();
    check(client.received_states().size()==delivered.size(),"Slow rendered client discarded complete host ticks");
    for(unsigned batch=0;batch<delivered.size();++batch) {
      const auto &received=client.received_states()[batch];
      check(received.tick==delivered[batch].tick,"Accepted native snapshot tick order changed");
      for(unsigned id=0;id<4;++id) {
        const auto &actual=delivered[batch].players[id].first_person_clock;
        const auto &copy=received.players[id].first_person_clock;
        check(copy.present==actual.present && same_first_person_channel(copy.origin,actual.origin) &&
            same_first_person_channel(copy.current,actual.current),"Batched delivery lost a real animation/event interval");
      }
    }
    client.receive();
    check(client.received_states().empty()&&client.state().tick==delivered.back().tick,
        "Empty native receive replayed already consumed animation ticks");
    auto invalid=client.state().players[0].first_person_clock;invalid.current.frame+=.5f;
    const auto saved=first_person_channel_state(views[0].hands[0]);bool rejected=false;
    try{apply_replicated_first_person_clock(origins[0],views[0],invalid,definition);}catch(const FormatError &){rejected=true;}
    check(rejected && same_first_person_channel(saved,first_person_channel_state(views[0].hands[0])),"Invalid clock pair partially changed retained view");
    std::cout<<"PASS actual native UDP:16 original-start participants,4 independent source SMG clocks, "<<sampled
        <<" 30/60/120/144/240Hz samples, "<<fractional<<" fractional frames; "<<maximum_fragments
        <<" bounded fragments with reorder/duplicate/drop/late recovery; gameplay hand-feedback owners unchanged\n";
    std::cout<<"LIMIT native presentation replicas from genuine source callbacks; not Xbox wire format, physical LAN or full body/action-manager parity\n";
#endif
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
