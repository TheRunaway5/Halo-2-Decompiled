#include "sword_online_socket_fixture.hpp"
#include "halo2/frontend.hpp"
#include "halo2/online_match_session.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <thread>

#if defined(__linux__)
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

namespace halo2::test {
namespace {
using Json=nlohmann::json;
using Clock=std::chrono::steady_clock;
using namespace std::chrono_literals;
constexpr std::size_t maximum_datagram=1400,maximum_ipc=32*1024,maximum_queue=2048;
enum : unsigned char {send_packet=1,received_packet=2,status_packet=3,loaded_packet=4,stop_packet=5};
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
const char *environment(const char *key){const auto value=std::getenv(key);need(value&&*value,"Incomplete owned ONLINE socket fixture environment");return value;}
std::filesystem::path private_file(const char *name) {
  const auto input=std::filesystem::path(environment(name));
  const auto owner=std::filesystem::canonical(environment("HALO2_TEST_SCRATCH"));
  const auto path=std::filesystem::canonical(input),relative=path.lexically_relative(owner);
  need(input.is_absolute() && !std::filesystem::is_symlink(input) && !relative.empty() &&
      *relative.begin()!=".." && std::filesystem::is_regular_file(path) && std::filesystem::file_size(path)<=maximum_ipc,
      "ONLINE fixture descriptor must be a bounded private file inside managed scratch");
  return path;
}
struct Descriptor {unsigned self{};std::string room,token;std::vector<OnlineAdmission> roster;};
Descriptor descriptor(const std::filesystem::path &path) {
  Json value;{std::ifstream input(path);need(bool(input),"Cannot read private ONLINE socket fixture descriptor");input>>value;}
  Descriptor result;result.self=value.at("slot").get<unsigned>();result.room=value.at("id").get<std::string>();
  result.token=value.at("token").get<std::string>();
  for(const auto &row:value.at("players")) {
    OnlineAdmission player;player.slot=result.roster.size();player.name=row.at("name").get<std::string>();
    player.team=row.at("team").get<int>();player.peer=row.at("peer").get<unsigned>();player.guest=row.at("guest").get<bool>();
    const auto id=row.at("identity").get<std::string>();need(id.size()==16,"Invalid private ONLINE presentation identity");
    for(unsigned n=0;n<8;++n)player.identity[n]=std::byte(std::stoul(id.substr(n*2,2),nullptr,16));
    result.roster.push_back(std::move(player));
  }
  need(result.self<2 && result.roster.size()==2 && result.roster[0].peer==0 && result.roster[1].peer==1 &&
      !result.roster[0].guest && !result.roster[1].guest,"Owned sword transport requires two actual admitted physical owners");
  return result;
}
std::shared_ptr<OnlineMatchSession> make_session(const std::string &origin,const Descriptor &d) {
  need(origin.starts_with("http://127.0.0.1:"),"Owned sword transport must use its private loopback coordinator");
  OnlineMatchTransportOptions transport;transport.direct_udp=true;
  return std::make_shared<OnlineMatchSession>(origin,d.room,d.token,d.self,d.roster,true,false,false,transport);
}
Json variant_json(std::uint64_t fingerprint,const UiVariant &variant) {
  Json writes=Json::array();for(const auto &[id,value]:variant.values)writes.push_back({id,value});
  return {{"fingerprint",fingerprint},{"name",variant.name},{"engine",variant.engine},{"writes",writes}};
}
void load_session(OnlineMatchSession &session,const Json &value) {
  UiVariant variant{value.at("name").get<std::string>(),value.at("engine").get<unsigned>(),{}};
  for(const auto &row:value.at("writes"))variant.values.append(row.at(0).get<unsigned>(),row.at(1).get<int>());
  session.loaded(value.at("fingerprint").get<std::uint64_t>(),variant);
}
Json state_json(const OnlineMatchSession &session,std::uint64_t sent,std::uint64_t received) {
  Json quality=Json::array();for(const auto &row:session.host_candidates())quality.push_back({{"peer",row.peer},
      {"upload_bytes_per_second",row.minimum_upload_bytes_per_second},{"rtt_ms",row.mean_rtt_ms},{"loss_per_mille",row.worst_loss_per_mille}});
  return {{"self",session.self()},{"host",session.host()},{"phase",unsigned(session.phase())},
      {"error",session.error()},{"direct_udp",session.direct_udp()},{"master_connected",session.master_connected()},
      {"sent",sent},{"received",received},{"quality",quality},{"rejected_direct_dispatches",session.rejected_direct_dispatches()}};
}
std::vector<std::byte> frame(unsigned char kind,std::span<const std::byte> bytes={},unsigned peer=0) {
  need(bytes.size()+2<=maximum_ipc,"Owned socket IPC frame exceeds its finite bound");
  std::vector<std::byte> result{std::byte(kind),std::byte(peer)};result.insert(result.end(),bytes.begin(),bytes.end());return result;
}
std::vector<std::byte> json_frame(unsigned char kind,const Json &value) {
  const auto bytes=value.dump();return frame(kind,std::as_bytes(std::span(bytes.data(),bytes.size())));
}
Json parse_frame(std::span<const std::byte> bytes) {
  need(bytes.size()>=2,"Truncated owned socket IPC frame");
  return Json::parse(reinterpret_cast<const char*>(bytes.data()+2),reinterpret_cast<const char*>(bytes.data()+bytes.size()));
}
#if defined(__linux__)
void nonblocking(int fd) {const auto flags=fcntl(fd,F_GETFL);need(flags>=0 && fcntl(fd,F_SETFL,flags|O_NONBLOCK)==0,"Cannot bind owned nonblocking socket IPC");}
bool send_frame(int fd,std::span<const std::byte> bytes) {
  const auto result=::send(fd,bytes.data(),bytes.size(),MSG_DONTWAIT|MSG_NOSIGNAL);
  if(result<0 && (errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR))return false;
  need(result==std::ptrdiff_t(bytes.size()),"Owned ONLINE socket worker IPC send failed");return true;
}
bool receive_frame(int fd,std::vector<std::byte> &bytes) {
  std::array<std::byte,maximum_ipc> buffer;const auto result=::recv(fd,buffer.data(),buffer.size(),MSG_DONTWAIT|MSG_TRUNC);
  if(result<0 && (errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR))return false;
  need(result>0 && std::size_t(result)<=buffer.size(),"Owned ONLINE socket worker IPC closed or exceeded its packet bound");
  bytes.assign(buffer.begin(),buffer.begin()+result);return true;
}
void enqueue(std::deque<std::vector<std::byte>> &queue,std::vector<std::byte> bytes) {
  need(queue.size()<maximum_queue,"Owned ONLINE socket IPC queue exceeded its finite bound");queue.push_back(std::move(bytes));
}
#endif
}

struct SwordOnlineSocketPair::Impl :std::enable_shared_from_this<Impl> {
  struct Counters {std::uint64_t sent{},received{},sent_bytes{},received_bytes{};};
  struct Endpoint final:MultiplayerDatagrams {
    std::weak_ptr<Impl> owner;unsigned slot;
    Endpoint(std::shared_ptr<Impl> p,unsigned s):owner(p),slot(s){}
    void send(std::span<const std::byte> bytes,unsigned peer)override {
      const auto p=owner.lock();need(bool(p),"Owned ONLINE socket endpoint outlived its fixture");p->send(slot,bytes,peer);
    }
    bool receive(std::vector<std::byte> &bytes,unsigned &peer)override {
      const auto p=owner.lock();need(bool(p),"Owned ONLINE socket endpoint outlived its fixture");return p->receive(slot,bytes,peer);
    }
  };
  Descriptor local,remote;
  std::shared_ptr<OnlineMatchSession> session;
  std::array<std::shared_ptr<MultiplayerDatagrams>,2> endpoints;
  mutable std::mutex mutex;
  std::deque<std::vector<std::byte>> outgoing,incoming;
  Json child_state;std::string failure;
  std::array<Counters,2> counts;
  unsigned elected_host=16;
  int fd=-1;
#if defined(__linux__)
  pid_t child=-1;
#endif
  std::jthread worker;
  ~Impl() {
#if defined(__linux__)
    worker.request_stop();if(worker.joinable())worker.join();
    if(fd>=0){const auto stop=frame(stop_packet);::send(fd,stop.data(),stop.size(),MSG_DONTWAIT|MSG_NOSIGNAL);}
    if(child>0) {
      const auto until=Clock::now()+2s;int status{};
      while(waitpid(child,&status,WNOHANG)==0 && Clock::now()<until)std::this_thread::sleep_for(10ms);
      if(waitpid(child,&status,WNOHANG)==0){kill(child,SIGTERM);std::this_thread::sleep_for(50ms);}
      if(waitpid(child,&status,WNOHANG)==0){kill(child,SIGKILL);while(waitpid(child,&status,0)<0&&errno==EINTR){}}
    }
    if(fd>=0)close(fd);
#endif
  }
  void start() {
#if defined(__linux__)
    local=descriptor(private_file("HALO2_SWORD_ONLINE_DESCRIPTOR_0"));
    remote=descriptor(private_file("HALO2_SWORD_ONLINE_DESCRIPTOR_1"));
    need(local.self==0 && remote.self==1 && local.room==remote.room,"Owned ONLINE descriptors disagree about their admitted room");
    for(unsigned n=0;n<2;++n)need(local.roster[n].identity==remote.roster[n].identity &&
        local.roster[n].name==remote.roster[n].name && local.roster[n].team==remote.roster[n].team,
        "Owned ONLINE descriptors disagree about their genuine roster");
    const std::string origin=environment("HALO2_SWORD_ONLINE_ORIGIN");
    int sockets[2];need(socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0,sockets)==0,"Cannot create owned remote process socket bridge");
    fd=sockets[0];
    // exec creates the remote RTC/OpenSSL/session runtime independently. Never
    // fork an existing world or the parent's RTC/provider worker threads.
    constexpr int child_fd=198;
    const std::string executable=std::filesystem::read_symlink("/proc/self/exe").string();
    const std::string private_path=private_file("HALO2_SWORD_ONLINE_DESCRIPTOR_1").string();
    std::array<std::string,5> arguments{executable,"--sword-online-socket-worker",origin,private_path,std::to_string(child_fd)};
    std::array<char*,6> argv{};for(unsigned n=0;n<arguments.size();++n)argv[n]=arguments[n].data();
    posix_spawn_file_actions_t actions;need(posix_spawn_file_actions_init(&actions)==0,"Cannot prepare owned remote exec");
    posix_spawn_file_actions_adddup2(&actions,sockets[1],child_fd);
    posix_spawn_file_actions_addclose(&actions,sockets[0]);
    if(sockets[1]!=child_fd)posix_spawn_file_actions_addclose(&actions,sockets[1]);
    const auto spawned=posix_spawn(&child,executable.c_str(),&actions,nullptr,argv.data(),environ);
    posix_spawn_file_actions_destroy(&actions);close(sockets[1]);
    need(spawned==0,"Cannot exec the owned remote ONLINE socket process");nonblocking(fd);
    worker=std::jthread([this](std::stop_token stop){io(stop);});
    session=make_session(origin,local);
    for(unsigned n=0;n<2;++n)endpoints[n]=std::make_shared<Endpoint>(shared_from_this(),n);
#else
    throw std::runtime_error("Owned process ONLINE sword socket qualification currently requires Linux");
#endif
  }
  void io(std::stop_token stop) {
#if defined(__linux__)
    try {
      while(!stop.stop_requested()) {
        {
          std::lock_guard lock(mutex);
          for(unsigned n=0;n<256 && !outgoing.empty();++n) {
            if(!send_frame(fd,outgoing.front()))break;outgoing.pop_front();
          }
        }
        std::vector<std::byte> packet;
        for(unsigned n=0;n<256 && receive_frame(fd,packet);++n) {
          need(packet.size()>=2,"Truncated remote ONLINE socket worker message");const auto type=std::to_integer<unsigned>(packet[0]);
          if(type==received_packet) {std::lock_guard lock(mutex);enqueue(incoming,std::move(packet));}
          else if(type==status_packet) {auto value=parse_frame(packet);std::lock_guard lock(mutex);child_state=std::move(value);}
          else throw std::runtime_error("Unexpected owned remote ONLINE socket worker message");
        }
        pollfd item{fd,POLLIN,0};poll(&item,1,2);
      }
    }catch(const std::exception &error){std::lock_guard lock(mutex);failure=error.what();}
#endif
  }
  void health()const {
    need(bool(session),"Owned local ONLINE socket session was not constructed");
    if(session->phase()==OnlineMatchSession::Phase::Failed)throw std::runtime_error("Local ONLINE session failed: "+session->error());
    std::lock_guard lock(mutex);
    if(!failure.empty())throw std::runtime_error(failure);
    if(!child_state.empty() && child_state.at("phase").get<unsigned>()==unsigned(OnlineMatchSession::Phase::Failed))
      throw std::runtime_error("Remote ONLINE session failed: "+child_state.at("error").get<std::string>());
  }
  void send(unsigned slot,std::span<const std::byte> bytes,unsigned peer) {
    health();need(slot<2 && peer==1u-slot && !bytes.empty() && bytes.size()<=maximum_datagram,
        "Native sword packet exceeds its actual admitted ONLINE endpoint/size");
    if(slot==0)session->send(bytes,peer);
#if defined(__linux__)
    else {std::lock_guard lock(mutex);enqueue(outgoing,frame(send_packet,bytes,peer));}
#endif
    ++counts[slot].sent;counts[slot].sent_bytes+=bytes.size();
  }
  bool receive(unsigned slot,std::vector<std::byte> &bytes,unsigned &peer) {
    health();need(slot<2,"Invalid owned ONLINE endpoint");
    if(slot==0) {if(!session->receive(bytes,peer))return false;}
    else {
      std::lock_guard lock(mutex);if(incoming.empty())return false;
      auto packet=std::move(incoming.front());incoming.pop_front();peer=std::to_integer<unsigned>(packet[1]);
      bytes.assign(packet.begin()+2,packet.end());
    }
    need(peer==1u-slot && !bytes.empty() && bytes.size()<=maximum_datagram,"Actual ONLINE receive changed admitted peer or native packet bound");
    ++counts[slot].received;counts[slot].received_bytes+=bytes.size();return true;
  }
};

bool SwordOnlineSocketPair::requested()noexcept {
  return std::getenv("HALO2_SWORD_ONLINE_ORIGIN") || std::getenv("HALO2_SWORD_ONLINE_DESCRIPTOR_0") ||
      std::getenv("HALO2_SWORD_ONLINE_DESCRIPTOR_1");
}
SwordOnlineSocketPair::SwordOnlineSocketPair():impl_(std::make_shared<Impl>()) {
  impl_->start();
  wait_for_io([&] {
    if(impl_->session->phase()!=OnlineMatchSession::Phase::Loading)return false;
    std::lock_guard lock(impl_->mutex);
    if(impl_->child_state.empty() || impl_->child_state.at("phase").get<unsigned>()!=unsigned(OnlineMatchSession::Phase::Loading))return false;
    const auto host=impl_->session->host();need(host<2 && impl_->child_state.at("host").get<unsigned>()==host,
        "Actual ONLINE endpoints disagree about their measured elected host");
    if(impl_->session->host_candidates().size()!=2 || impl_->child_state.at("quality").size()!=2)return false;
    impl_->elected_host=host;return true;
  },"actual ICE/DTLS/SCTP measurement, room admission and host election",45s);
  std::cout<<"Source actual ONLINE socket pair: host_slot="<<host_slot()<<" remote_slot="<<other_slot()
    <<" process_endpoints=2 master_relay=0 external_account=fixture\n";
}
SwordOnlineSocketPair::~SwordOnlineSocketPair()=default;
unsigned SwordOnlineSocketPair::host_slot()const{return impl_->elected_host;}
unsigned SwordOnlineSocketPair::other_slot()const{return 1u-host_slot();}
const std::vector<OnlineAdmission>& SwordOnlineSocketPair::roster()const{return impl_->local.roster;}
std::shared_ptr<MultiplayerDatagrams> SwordOnlineSocketPair::endpoint(unsigned slot)const {
  need(slot<2,"Invalid owned ONLINE socket endpoint selection");return impl_->endpoints[slot];
}
void SwordOnlineSocketPair::loaded(std::uint64_t fingerprint,const UiVariant &variant) {
  impl_->session->loaded(fingerprint,variant);
#if defined(__linux__)
  {std::lock_guard lock(impl_->mutex);enqueue(impl_->outgoing,json_frame(loaded_packet,variant_json(fingerprint,variant)));}
#endif
  wait_for_io([&]{if(impl_->session->phase()!=OnlineMatchSession::Phase::Running)return false;
    std::lock_guard lock(impl_->mutex);return !impl_->child_state.empty() &&
      impl_->child_state.at("phase").get<unsigned>()==unsigned(OnlineMatchSession::Phase::Running);},
      "matching native content/variant and actual room Running admission",45s);
}
void SwordOnlineSocketPair::health()const{impl_->health();}
void SwordOnlineSocketPair::wait_for_io(const std::function<bool()>& ready,const char *stage,std::chrono::milliseconds timeout) {
  need(timeout.count()>0 && timeout<=60s,"Owned ONLINE I/O wait requires a finite at-most60-second bound");
  const auto until=Clock::now()+timeout;
  while(true) {health();if(ready())return;
    if(Clock::now()>=until)throw std::runtime_error(std::string("Actual ONLINE socket deadline: ")+stage);
    std::this_thread::sleep_for(2ms);
  }
}
void SwordOnlineSocketPair::write_report(const std::filesystem::path &path)const {
  health();Json endpoints=Json::array();
  for(unsigned n=0;n<2;++n){const auto &row=impl_->counts[n];endpoints.push_back({{"slot",n},{"submitted_packets",row.sent},
      {"received_packets",row.received},{"submitted_bytes",row.sent_bytes},{"received_bytes",row.received_bytes}});}
  Json child;{std::lock_guard lock(impl_->mutex);child=impl_->child_state;}
  const auto value=Json{{"schema","H2_TU15_OWNED_ONLINE_SOCKET_V1"},{"host",host_slot()},{"endpoints",endpoints},
      {"local",state_json(*impl_->session,impl_->counts[0].sent,impl_->counts[0].received)},{"remote",child},
      {"actual_process_endpoints",2},{"gameplay_master_relay",false},{"transport","OnlineMatchSession/OnlinePeerTransport/ICE/DTLS/SCTP/UDP"},
      {"native_client_codec","parent executor; remote session sockets in exec child"},{"external_account_authentication","local verification fixture"},
      {"network_scope","same-host sockets; no WAN, NAT traversal or network impairment claim"}};
  need(value.dump().size()<=maximum_ipc,"Owned ONLINE socket report exceeded its finite bound");
  std::ofstream output(path);output<<value.dump(2)<<'\n';need(bool(output),"Cannot write bounded ONLINE socket evidence report");
}

int sword_online_socket_worker_main(int argc,char **argv) {
  if(argc<2 || std::string_view(argv[1])!="--sword-online-socket-worker")return -1;
  try {
#if defined(__linux__)
    need(argc==5,"Malformed owned ONLINE socket child invocation");
    need(prctl(PR_SET_PDEATHSIG,SIGTERM)==0 && getppid()!=1,
        "Owned ONLINE socket child lost its parent lifecycle");
    const auto selected=private_file("HALO2_SWORD_ONLINE_DESCRIPTOR_1");
    need(std::filesystem::canonical(argv[3])==selected,"Owned ONLINE child descriptor differs from its private admission");
    const auto d=descriptor(selected);need(d.self==1,"Owned ONLINE child must own admitted endpoint1");
    const auto fd=std::stoi(argv[4]);need(fd==198,"Owned ONLINE child received a foreign private IPC descriptor");nonblocking(fd);
    auto session=make_session(argv[2],d);std::deque<std::vector<std::byte>> outgoing;
    std::uint64_t sent{},received{};auto next_status=Clock::now();const auto deadline=Clock::now()+15min;
    while(Clock::now()<deadline) {
      if(session->phase()==OnlineMatchSession::Phase::Failed) {
        send_frame(fd,json_frame(status_packet,state_json(*session,sent,received)));
        throw std::runtime_error("Actual remote ONLINE session failed: "+session->error());
      }
      std::vector<std::byte> message;
      for(unsigned n=0;n<256 && receive_frame(fd,message);++n) {
        need(message.size()>=2,"Truncated private ONLINE child command");
        switch(std::to_integer<unsigned>(message[0])) {
        case send_packet:
          need(std::to_integer<unsigned>(message[1])==0 && message.size()>2 && message.size()<=maximum_datagram+2,
              "Invalid admitted remote ONLINE child datagram");
          session->send(std::span(message).subspan(2),0);++sent;break;
        case loaded_packet:load_session(*session,parse_frame(message));break;
        case stop_packet:close(fd);return 0;
        default:throw std::runtime_error("Unknown private ONLINE socket child command");
        }
      }
      std::vector<std::byte> bytes;unsigned peer{};
      for(unsigned n=0;n<256 && session->receive(bytes,peer);++n) {
        need(peer==0 && !bytes.empty() && bytes.size()<=maximum_datagram,"Remote ONLINE receive violated actual admission");
        enqueue(outgoing,frame(received_packet,bytes,peer));++received;
      }
      if(Clock::now()>=next_status) {enqueue(outgoing,json_frame(status_packet,state_json(*session,sent,received)));next_status=Clock::now()+25ms;}
      for(unsigned n=0;n<256 && !outgoing.empty();++n) {if(!send_frame(fd,outgoing.front()))break;outgoing.pop_front();}
      pollfd item{fd,POLLIN,0};poll(&item,1,2);
    }
    throw std::runtime_error("Owned remote ONLINE socket worker exceeded its15-minute lifetime");
#else
    throw std::runtime_error("Owned ONLINE socket worker requires Linux");
#endif
  }catch(const std::exception &error){std::cerr<<"Owned ONLINE socket worker: "<<error.what()<<'\n';return 1;}
}
} // namespace halo2::test
