#include "halo2/custom_slayer.hpp"
#include "halo2/frontend.hpp"
#include "halo2/online_match_session.hpp"
#include "halo2/online_peer_transport.hpp"
#include "halo2/online_voice_protocol.hpp"
#include "halo2/voice_origin.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/peer_migration_preparation.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <deque>
#include <iomanip>
#include <functional>
#include <mutex>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
using Json = nlohmann::json;
using Clock = std::chrono::steady_clock;
void require(bool ok) { if (!ok) throw std::runtime_error("ONLINE match connection failed or ended. Search again."); }
struct Frame { unsigned peer; std::vector<std::byte> data;std::uint32_t epoch{};std::uint64_t audio_generation{}; };
struct TerminalMatchError : std::runtime_error {using std::runtime_error::runtime_error;};
struct ControlPresence {std::atomic<bool> &active;~ControlPresence(){active=false;}};
struct EosHttpTransfer {
  std::string body;
  std::stop_token stop;
};
std::size_t receive_eos_auth(char *data,std::size_t size,std::size_t count,void *opaque) {
  auto &transfer=*static_cast<EosHttpTransfer *>(opaque);
  constexpr std::size_t maximum=16384;
  if(transfer.stop.stop_requested()||(count&&size>std::numeric_limits<std::size_t>::max()/count))return 0;
  const auto bytes=size*count;
  if(bytes>maximum-transfer.body.size())return 0;
  try{transfer.body.append(data,bytes);}catch(...){return 0;}
  return bytes;
}
int cancel_eos_auth(void *opaque,curl_off_t,curl_off_t,curl_off_t,curl_off_t) {
  return static_cast<EosHttpTransfer *>(opaque)->stop.stop_requested()?1:0;
}
OnlineEosAuthResponse request_eos_auth(const std::string &room_api,const std::string &bearer,
                                     const OnlineEosAuthRequest &request,std::stop_token stop) {
  OnlineEosAuthResponse response{};response.kind=request.kind;response.ticket=request.ticket;
  if(request.kind!=1&&request.kind!=2){response.status=400;return response;}
  try{
    std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> client(curl_easy_init(),curl_easy_cleanup);
    if(!client)return response;
    const auto authorization="Authorization: Bearer "+bearer;
    std::unique_ptr<curl_slist,decltype(&curl_slist_free_all)> headers(
        curl_slist_append(nullptr,authorization.c_str()),curl_slist_free_all);
    if(!headers||!curl_slist_append(headers.get(),"Content-Type: application/json"))return response;
    const auto url=room_api+(request.kind==1?"/eos-token":"/eos-proof");
    const auto body=request.kind==1?std::string("{}"):Json{{"token",request.token}}.dump();
    EosHttpTransfer transfer{{},stop};
    curl_easy_setopt(client.get(),CURLOPT_URL,url.c_str());
    curl_easy_setopt(client.get(),CURLOPT_HTTPHEADER,headers.get());
    curl_easy_setopt(client.get(),CURLOPT_POST,1L);
    curl_easy_setopt(client.get(),CURLOPT_POSTFIELDS,body.c_str());
    curl_easy_setopt(client.get(),CURLOPT_POSTFIELDSIZE,long(body.size()));
    curl_easy_setopt(client.get(),CURLOPT_PROTOCOLS_STR,"https,http");
    curl_easy_setopt(client.get(),CURLOPT_FOLLOWLOCATION,0L);
    curl_easy_setopt(client.get(),CURLOPT_PROXY,"");
    curl_easy_setopt(client.get(),CURLOPT_NOSIGNAL,1L);
    curl_easy_setopt(client.get(),CURLOPT_CONNECTTIMEOUT_MS,2000L);
    // The existing official proof verifier may wait up to twenty seconds.
    // This I/O runs on its own worker and remains cancellable during teardown.
    curl_easy_setopt(client.get(),CURLOPT_TIMEOUT_MS,request.kind==1?3000L:25000L);
    curl_easy_setopt(client.get(),CURLOPT_SSL_VERIFYPEER,1L);
    curl_easy_setopt(client.get(),CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(client.get(),CURLOPT_ACCEPT_ENCODING,"identity");
    curl_easy_setopt(client.get(),CURLOPT_HTTP_CONTENT_DECODING,0L);
    curl_easy_setopt(client.get(),CURLOPT_WRITEFUNCTION,receive_eos_auth);
    curl_easy_setopt(client.get(),CURLOPT_WRITEDATA,&transfer);
    curl_easy_setopt(client.get(),CURLOPT_NOPROGRESS,0L);
    curl_easy_setopt(client.get(),CURLOPT_XFERINFOFUNCTION,cancel_eos_auth);
    curl_easy_setopt(client.get(),CURLOPT_XFERINFODATA,&transfer);
    if(curl_easy_perform(client.get())!=CURLE_OK||stop.stop_requested())return response;
    long status{};curl_easy_getinfo(client.get(),CURLINFO_RESPONSE_CODE,&status);
    response.status=status>=0&&status<=999?std::uint16_t(status):0;
    if(status!=200)return response;
    std::vector<std::set<std::string>> keys;
    const auto value=Json::parse(transfer.body,[&](int depth,Json::parse_event_t event,Json &v){
      require(depth<=8);
      if(event==Json::parse_event_t::object_start)keys.emplace_back();
      if(event==Json::parse_event_t::key)require(!keys.empty()&&keys.back().insert(v.get<std::string>()).second);
      if(event==Json::parse_event_t::object_end)keys.pop_back();
      return true;
    });
    if(!value.is_object())return response;
    if(request.kind==1){
      if(value.size()!=2||!value.contains("token")||!value["token"].is_string()||
          !value.contains("expires_in")||!value["expires_in"].is_number_integer()||
          value["expires_in"]<1||value["expires_in"]>120)return response;
      response.token=value["token"].get<std::string>();
      if(response.token.empty()||response.token.size()>8192)return response;
      response.expires_in=value["expires_in"].get<std::uint32_t>();
    }else{
      if(value.size()!=1||!value.contains("eos_product_user_id")||!value["eos_product_user_id"].is_string())return response;
      response.user=value["eos_product_user_id"].get<std::string>();
      if(response.user.size()>64)return response;
    }
    response.ok=true;
  }catch(...){response.ok=false;response.token.clear();response.user.clear();response.expires_in=0;}
  return response;
}
bool valid_world_digest(const std::string &value) {
  return value.size()==64 && value.find_first_not_of("0123456789abcdef")==std::string::npos;
}
void put(std::vector<std::byte>& p,unsigned value,unsigned count){for(unsigned i=0;i<count;++i)p.push_back(std::byte(value>>(i*8)));}
unsigned get(std::span<const std::byte> p,unsigned at,unsigned count){require(at+count<=p.size());unsigned n{};for(unsigned i=0;i<count;++i)n|=std::to_integer<unsigned>(p[at+i])<<(i*8);return n;}
std::vector<std::byte> voice_header(unsigned type){std::vector<std::byte> p;put(p,0x324e3248,4);put(p,native_multiplayer_protocol,2);put(p,6,1);put(p,type,1);return p;}
std::string digest(std::span<const std::byte> bytes) {
  unsigned char hash[SHA256_DIGEST_LENGTH];require(SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),hash));
  constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(64);
  for(auto n:hash){result+=digits[n>>4];result+=digits[n&15];}return result;
}
std::uint64_t transfer_time(){return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();}
AuthorityTransferLimits source_transfer_limits(bool snapshot_candidates=false,bool survivor_selection=false){
  AuthorityTransferLimits limits;limits.features=authority_feature_source_replay|authority_feature_source_state|authority_feature_source_camera|authority_feature_source_lens_query|authority_feature_source_lens_draw|authority_feature_source_particle_collision|authority_feature_source_projectile_contacts|authority_feature_source_pipeline|authority_feature_source_host_continuation|authority_feature_source_common_cut;
  if(snapshot_candidates)limits.features|=authority_feature_source_snapshot_candidate;
  if(survivor_selection)limits.features|=authority_feature_survivor_selection;
  return limits;
}
online_protocol_bridge::MigrationPreparationCut preparation_cut(const AuthorityCutSeal &cut){
  online_protocol_bridge::MigrationPreparationCut out{};
  out.present=true;out.id=cut.id;out.tick=cut.tick;out.bytes=cut.bytes;
  std::ranges::transform(cut.digest,out.digest.begin(),[](auto byte){return std::to_integer<std::uint8_t>(byte);});
  return out;
}
}
struct OnlineMatchSession::Impl {
  unsigned self, host = 16;
  const std::vector<OnlineAdmission> roster;
  mutable std::mutex mutex;
  Phase phase = Phase::Connecting;
  std::string error, loaded;
  std::optional<std::uint64_t> native_fingerprint;
  std::string completion, confirmation;
  std::vector<std::byte> report;
  std::deque<Frame> incoming, outgoing;
  std::deque<std::string> continuation_outgoing;
  std::deque<OnlineWorldSeal> seals;
  const bool supports_continuity;
  const OnlineMatchTransportOptions transport_options;
  OnlineContinuityState continuity_state;
  AuthorityTransferDigest authority_session{};
  VoiceOrigin voice_origin;
  std::shared_ptr<PeerAuthorityTransfer> authority;
  std::shared_ptr<PeerSourceAdmissions> source_admissions;
  std::shared_ptr<PeerCommonCutAgreement> common_cut;
  std::shared_ptr<PeerSurvivorSelection> survivor_selection;
  bool selection_started{};
  std::unique_ptr<PeerMigrationPreparation> preparation_driver;
  std::shared_ptr<const unsigned char> preparation_identity=std::make_shared<const unsigned char>(0);
  std::shared_ptr<PeerSnapshotCandidate> snapshot_candidate;
  std::shared_ptr<PeerMeshQuality> mesh_quality;
  std::shared_ptr<PeerHostAgreement> agreement;
  std::optional<AuthorityTransferContext> agreement_context;
  struct AgreementGeneration {AuthorityTransferContext context;std::shared_ptr<PeerHostAgreement> owner;};
  std::vector<AgreementGeneration> agreement_history;
  unsigned agreement_cursor{};
  bool peer_started{},peer_fenced{};
  std::optional<std::vector<SourcePlayerAdmission>> continuity_admissions;
  std::deque<MatchVoicePacket> voices;
  std::shared_ptr<GameVoice> audio;
  std::uint64_t audio_generation{};
  std::array<bool,16> voice_enabled{};
  std::array<std::uint16_t,16> voice_mutes{};
  std::array<unsigned,16> voice_sequences{};
  std::uint16_t voice_members{};
  std::uint16_t voice_blocked{};
  Clock::time_point voice_preferences_at{};
  std::array<double,16> voice_credits{10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10};
  Clock::time_point voice_credit_at=Clock::now();
  std::atomic<bool> voice_focused{false};
  std::atomic<bool> control_online{false};
  std::atomic<std::uint16_t> direct_ready{0};
  std::atomic<std::uint64_t> rejected_direct{0};
  std::function<void()> postgame_voice_pump;
  std::jthread worker;
  const std::optional<bool> team_mode;
  Impl(std::string origin, std::string room, std::string bearer, unsigned s,
       std::vector<OnlineAdmission> r, bool local,std::optional<bool> teams,bool continuation,OnlineMatchTransportOptions transport) : self(s), roster(std::move(r)),supports_continuity(continuation),transport_options(std::move(transport)),voice_origin(room),team_mode(teams) {
    require(self < roster.size() && roster.size() >= 2 && roster.size() <= 16);
    require(!transport_options.eos || transport_options.direct_udp);
    require(!transport_options.peer_host_migration || (transport_options.direct_udp&&!supports_continuity));
    require(!transport_options.snapshot_candidates || (transport_options.direct_udp&&transport_options.peer_host_migration));
    require(!transport_options.survivor_cut_selection || transport_options.peer_host_migration);
    agreement_history.reserve(15); // Each committed generation removes one of at most sixteen PCs.
    require(online_admission_peer(roster[self])==self && !roster[self].guest);
    for(unsigned i=0;i<roster.size();++i) {
      const auto &player=roster[i];const auto peer=online_admission_peer(player);
      require(player.slot==i && peer<roster.size() && online_admission_peer(roster[peer])==peer && !roster[peer].guest);
      if(team_mode.value_or(true))require(player.team==roster[peer].team);
      require(std::count_if(roster.begin(),roster.end(),[&](const auto &p){return online_admission_peer(p)==peer;})<=4);
    }
    voice_members=std::uint16_t((1u<<roster.size())-1);
    continuity_state.members=voice_members;
    require(room.size() == 36 && room.find_first_not_of("0123456789abcdef-") == std::string::npos);
    require(SHA256(reinterpret_cast<const unsigned char *>(room.data()),room.size(),reinterpret_cast<unsigned char *>(authority_session.data())));
    if(transport_options.direct_udp){
      std::uint16_t physical=0;for(const auto &player:roster)if(online_admission_peer(player)==player.slot)physical|=std::uint16_t(1u<<player.slot);
      if(std::popcount(physical)>1)mesh_quality=std::make_shared<PeerMeshQuality>(self,
          AuthorityTransferContext{authority_session,0,0,physical,16});
    }
    require(bearer.size() >= 32 && bearer.size() <= 128 && bearer.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_") == std::string::npos);
    require(origin.starts_with("https://") || (local && origin.starts_with("http://127.0.0.1:")));
    require(origin.find_first_of("@?#\\\r\n\t ") == std::string::npos);
    const auto scheme = origin.find(':');
    const auto url = std::string(origin.starts_with("https:") ? "wss" : "ws") + origin.substr(scheme) + "/v1/matches/" + room + (transport_options.direct_udp?"/signal":"/relay");
    const auto room_api=origin+"/v1/matches/"+room;
    worker = std::jthread([this, url, room_api, bearer](std::stop_token stop) { run(url, room_api, bearer, stop); });
  }
  ~Impl() { worker.request_stop(); if (worker.joinable()) worker.join(); }
  static OnlineWorldSeal parse_seal(const Json &value) {
    require(value.is_object() && value.size()==3 && value.contains("epoch") &&
        value.contains("tick") && value.contains("digest") && value["epoch"].is_number_unsigned() &&
        value["epoch"].get<std::uint64_t>()<=UINT32_MAX && value["tick"].is_number_unsigned() && value["digest"].is_string());
    OnlineWorldSeal seal{value["epoch"].get<std::uint32_t>(),value["tick"].get<std::uint64_t>(),value["digest"].get<std::string>()};
    require(valid_world_digest(seal.digest));return seal;
  }
  bool epoch_packets()const{return supports_continuity||transport_options.peer_host_migration;}
  // Session mutex is held. Capability Hello state persists through transport
  // loss; direct readiness never removes a participant from this barrier.
  bool common_supported()const{
    if(!transport_options.direct_udp||!common_cut||!authority)return false;
    const auto context=common_cut->context();if(authority->context()!=context)return false;
    for(unsigned peer=0;peer<16;++peer)if(peer!=self&&(context.peers&(1u<<peer))&&
        !authority->feature_supported(peer,authority_feature_source_common_cut))return false;
    return true;
  }
  bool common_phase()const{return phase==Phase::Loading||phase==Phase::Running||phase==Phase::AwaitingWorld||phase==Phase::Migrating||phase==Phase::Rebinding;}
  bool native_control_phase()const{return phase==Phase::Loading||phase==Phase::Running||phase==Phase::Migrating||phase==Phase::Rebinding||
      (selection_started&&phase==Phase::AwaitingWorld);}
  bool selection_supported()const{
    if(!transport_options.survivor_cut_selection||!survivor_selection||!authority)return false;
    const auto context=survivor_selection->state().context;
    if(authority->context()!=context)return false;
    for(unsigned peer=0;peer<16;++peer)if(peer!=self&&(context.peers&(1u<<peer))&&
        !authority->feature_supported(peer,authority_feature_survivor_selection))return false;
    return true;
  }
  // One Session lock and one HostAgreement clock sample bind these facts.
  // Neither the snapshot nor its Rust request certifies native ownership.
  PeerMigrationPreparationObservation preparation_observation(std::uint64_t now)const{
    static_assert(unsigned(Phase::Connecting)==0&&unsigned(Phase::Loading)==1&&unsigned(Phase::Running)==2&&
        unsigned(Phase::Reviewing)==3&&unsigned(Phase::Postgame)==4&&unsigned(Phase::Failed)==5&&
        unsigned(Phase::AwaitingWorld)==6&&unsigned(Phase::Migrating)==7&&unsigned(Phase::Rebinding)==8);
    PeerMigrationPreparationObservation out;
    out.context=authority?std::optional{authority->context()}:std::nullopt;
    out.self=self;out.phase=std::uint8_t(phase);
    out.supported=peer_started&&agreement_context&&out.context==agreement_context&&selection_supported();
    out.selection_started=selection_started;
    const auto admitted=source_admissions?source_admissions->accepted():std::nullopt;
    if(admitted&&(!continuity_admissions||admitted->players==*continuity_admissions))out.admitted=admitted->context;
    if(agreement)out.agreement=agreement->state(now);
    if(survivor_selection)out.selection=survivor_selection->state();
    return out;
  }
  void preparation_terminal(){
    // Rust has rejected reuse of this preparation. This mechanical fence
    // preserves actual agreement decisions and never publishes native Ready.
    if(!peer_handoff_phase())return;
    if(!peer_fenced){peer_fenced=true;incoming.clear();outgoing.clear();fence_audio();}
    fence_peer_generation();
    if(phase==Phase::Running)phase=Phase::AwaitingWorld;
  }
  // Session lock serializes actual Common Abort, selection and Ready. These
  // metadata calls never create native freezes, leases or preparation owners.
  void selection_abort(){
    if(!survivor_selection||!common_cut)return;
    const auto common=common_cut->state();
    if(common.abort&&common.pending)survivor_selection->invalidate(common.pending->seal);
  }
  void selection_progress(){
    if(!survivor_selection)return;
    auto selected=survivor_selection->state();
    if(selected.selection_complete&&!selected.failed&&agreement){
      const auto learned=agreement->state(transfer_time());
      for(unsigned peer=0;peer<16;++peer)if((selected.survivors&(1u<<peer))&&learned.ready[peer])
        survivor_selection->observe_ready(peer,*learned.ready[peer]);
      selected=survivor_selection->state();
    }
    if(!selection_started)return;
    if(!peer_fenced){peer_fenced=true;incoming.clear();outgoing.clear();fence_audio();}
    if(selected.failed||!agreement||!agreement->state(transfer_time()).decision)phase=Phase::AwaitingWorld;
  }
  bool snapshot_supported()const{
    if(!transport_options.snapshot_candidates||!snapshot_candidate||!authority||!common_supported())return false;
    const auto context=snapshot_candidate->state().context;
    if(authority->context()!=context)return false;
    for(unsigned peer=0;peer<16;++peer)if(peer!=self&&(context.peers&(1u<<peer))&&
        !authority->feature_supported(peer,authority_feature_source_snapshot_candidate))return false;
    return true;
  }
  using CommonAction=bool(PeerCommonCutAgreement::*)(const PeerCommonCut &);
  bool common_action(const PeerCommonCut &cut,CommonAction action,bool retirement=false){
    std::lock_guard lock(mutex);
    if((phase!=Phase::Running&&!(retirement&&common_phase()))||!common_supported()||common_cut->context()!=cut.context)return false;
    const auto result=((*common_cut).*action)(cut);selection_abort();selection_progress();return result;
  }
  bool peer_handoff_phase()const{return peer_started&&(phase==Phase::Running||phase==Phase::AwaitingWorld||phase==Phase::Migrating||phase==Phase::Rebinding);}
  std::optional<unsigned> migration_candidate(std::uint64_t now)const{
    if(!peer_handoff_phase()||!agreement||!agreement_context||!mesh_quality||mesh_quality->context()!=*agreement_context)return {};
    const auto state=agreement->state(now);
    if(transport_options.survivor_cut_selection&&(!selection_supported()||
        !survivor_selection->state().selection_complete||survivor_selection->state().failed))return {};
    if((!state.suspected&&!state.fenced)||state.decision||!(state.electorate&(1u<<self)))return {};
    if(std::popcount(state.electorate)==1)return state.ready[self]?std::optional(self):std::nullopt;
    for(const auto &q:mesh_quality->migration_candidates(now))
      if((state.electorate&(1u<<q.peer))&&state.ready[q.peer])return q.peer;
    return {};
  }
  // Session mutex is held. Once a vote is accepted, no queued predecessor
  // gameplay or voice can reach a native consumer, even before a learner has
  // enough independent receipts to identify the successor.
  void fence_audio(){
    if(audio)audio_generation=audio->fence();
    voices.clear();
    std::erase_if(outgoing,[](const auto &p){return p.data.size()>6&&p.data[6]==std::byte{6};});
  }
  void sync_audio(){
    if(audio&&!peer_fenced&&(phase==Phase::Running||phase==Phase::Reviewing||phase==Phase::Postgame))
      (void)audio->resume(audio_generation);
  }
  void fence_peer_generation(){
    if(selection_started&&survivor_selection&&survivor_selection->state().failed){selection_progress();return;}
    if(!agreement||!peer_handoff_phase())return;
    const auto state=agreement->state(transfer_time());
    if(!state.fenced)return;
    if(!peer_fenced){peer_fenced=true;incoming.clear();outgoing.clear();fence_audio();}
    if(state.decision){
      if(!(state.decision->next.peers&(1u<<self))){phase=Phase::Failed;error="This player host was replaced by the surviving peer majority.";}
      else phase=self==state.decision->next.host?Phase::Migrating:Phase::Rebinding;
    }else phase=Phase::AwaitingWorld;
  }
  void control(const Json &data) {
    require(data.size()==(supports_continuity?3u:2u) && data["phase"].is_string());
    const auto name=data["phase"].get<std::string>();
    require(name=="connecting" || name=="loading" || name=="running" ||
        (supports_continuity && (name=="awaiting_world" || name=="migrating" || name=="rebinding")));
    std::lock_guard lock(mutex);
    // After initial admission this direct mode obtains host/epoch/membership
    // only from its local peer agreement, including across master reconnects.
    if(transport_options.peer_host_migration&&peer_started)return;
    const auto previous_host=host,previous_epoch=continuity_state.epoch;
    if(name!="connecting") {
      if(name=="awaiting_world"){require(data["host"].is_null());host=16;}
      else {
        require(data["host"].is_number_unsigned() && data["host"]<roster.size());
        host=data["host"].get<unsigned>();
        require(online_admission_peer(roster[host])==host && !roster[host].guest);
      }
    }
    if(supports_continuity) {
      require(data.contains("continuity"));const auto &value=data["continuity"];
      if(name=="connecting" || name=="loading")require(value.is_null());
      else {
        require(value.is_object() && value["epoch"].is_number_unsigned() &&
            value["epoch"].get<std::uint64_t>()<=UINT32_MAX && value["members"].is_number_unsigned() &&
            value["phase"]==name && value["host"]==data["host"]);
        const auto epoch=value["epoch"].get<std::uint32_t>(),members=value["members"].get<unsigned>();
        require(epoch>=continuity_state.epoch && !(members&~((1u<<roster.size())-1)) &&
            (members&(1u<<self)) && (host==16 || (members&(1u<<host))));
        for(const auto &player:roster)
          require(bool(members&(1u<<player.slot))==bool(members&(1u<<online_admission_peer(player))));
        std::optional<OnlineWorldSeal> selected;
        if(name=="migrating" || name=="rebinding") {
          require(value.size()==6);
          selected=parse_seal(Json{{"epoch",epoch},{"tick",value["tick"]},{"digest",value["digest"]}});
        } else require(value.size()==4);
        continuity_state={epoch,host,std::uint16_t(members),std::move(selected)};
        voice_members=std::uint16_t(members);
        for(unsigned i=0;i<roster.size();++i)if(!(members&(1u<<i)))voice_enabled[i]=false;
      }
      if(previous_epoch!=continuity_state.epoch || previous_host!=host) {
        incoming.clear();outgoing.clear();voices.clear();
      }
    }
    phase=name=="running"?Phase::Running:name=="loading"?Phase::Loading:
        name=="awaiting_world"?Phase::AwaitingWorld:name=="migrating"?Phase::Migrating:
        name=="rebinding"?Phase::Rebinding:Phase::Connecting;
    if(transport_options.direct_udp&&!authority&&(phase==Phase::Loading||phase==Phase::Running)&&host<16){
      std::uint16_t physical=0;for(const auto &player:roster)if(online_admission_peer(player)==player.slot)physical|=std::uint16_t(1u<<player.slot);
      const AuthorityTransferContext context{authority_session,continuity_state.epoch,0,physical,host};
      authority=std::make_shared<PeerAuthorityTransfer>(self,context,source_transfer_limits(transport_options.snapshot_candidates,transport_options.survivor_cut_selection));
      source_admissions=std::make_shared<PeerSourceAdmissions>(self,context,roster);
      if(std::popcount(physical)>1)common_cut=std::make_shared<PeerCommonCutAgreement>(self,context);
      if(transport_options.snapshot_candidates&&std::popcount(physical)>1)
        snapshot_candidate=std::make_shared<PeerSnapshotCandidate>(self,context);
      if(transport_options.peer_host_migration&&std::popcount(physical)>1){agreement_context=context;
        agreement=std::make_shared<PeerHostAgreement>(self,context,transfer_time());
        if(transport_options.survivor_cut_selection&&self!=host){
          survivor_selection=std::make_shared<PeerSurvivorSelection>(self,context);
          preparation_driver=std::make_unique<PeerMigrationPreparation>(self,context);
        }}
    }
    if(transport_options.direct_udp&&host<16&&phase!=Phase::Connecting){
      std::uint16_t physical=0;for(const auto &player:roster)
        if(online_admission_peer(player)==player.slot&&(continuity_state.members&(1u<<player.slot)))physical|=std::uint16_t(1u<<player.slot);
      auto context=AuthorityTransferContext{authority_session,continuity_state.epoch,continuity_state.members,physical,host};
      // The internally gated peer-owned generation uses the same admission
      // revision for QoS, native cuts and votes. Legacy setup keeps its seat-mask
      // revision; it cannot be mistaken for this peer migration certificate.
      if(transport_options.peer_host_migration&&agreement_context){
        require(agreement_context->session==context.session&&agreement_context->epoch==context.epoch&&
            agreement_context->peers==physical&&agreement_context->host==host);
        context=*agreement_context;
      }
      if(std::popcount(physical)>1&&(!mesh_quality||mesh_quality->context()!=context))mesh_quality=std::make_shared<PeerMeshQuality>(self,context);
    }
    if(transport_options.peer_host_migration&&phase==Phase::Running){
      peer_started=true;continuity_state.host=host;
      if(agreement)agreement->observe_host(transfer_time());
    }
  }
  void queue_world(const char *action,const OnlineWorldSeal &seal) {
    require(supports_continuity && valid_world_digest(seal.digest) && continuation_outgoing.size()<64);
    continuation_outgoing.push_back(Json{{action,{{"epoch",seal.epoch},{"tick",seal.tick},{"digest",seal.digest}}}}.dump());
  }
  void datagram(unsigned source,std::span<const std::byte> packet,std::uint32_t epoch) {
    if(PeerSurvivorSelection::is_packet(packet)){
      std::lock_guard lock(mutex);
      if(common_phase()&&selection_supported()&&epoch==survivor_selection->state().context.epoch){
        survivor_selection->receive(source,packet,transfer_time());selection_abort();selection_progress();
      }
      return;
    }
    if(PeerSnapshotCandidate::is_packet(packet)){
      std::shared_ptr<PeerSnapshotCandidate> candidate;
      {std::lock_guard lock(mutex);if(common_phase()&&snapshot_supported())candidate=snapshot_candidate;}
      if(candidate&&epoch==candidate->state().context.epoch)candidate->receive(source,packet,transfer_time());
      return;
    }
    if(PeerCommonCutAgreement::is_packet(packet)){
      // Serialize Abort with the final native publication check. The Rust
      // metadata owner invokes no native/session callbacks; lock ordering is
      // session then common-cut, as for the explicit common actions.
      std::lock_guard lock(mutex);
      if(common_phase()&&common_supported()&&(!epoch_packets()||epoch==common_cut->context().epoch))
        common_cut->receive(source,packet,transfer_time());
      selection_abort();selection_progress();
      return;
    }
    if(PeerHostAgreement::is_packet(packet)){
      std::lock_guard lock(mutex);
      if(peer_handoff_phase()&&agreement_context&&epoch==agreement_context->epoch&&agreement){
        agreement->receive(source,packet,transfer_time());selection_progress();fence_peer_generation();
      }else for(const auto &past:agreement_history)if(epoch==past.context.epoch){past.owner->receive(source,packet,transfer_time());break;}
      return;
    }
    if(PeerMeshQuality::is_packet(packet)){
      std::shared_ptr<PeerMeshQuality> quality;
      {std::lock_guard lock(mutex);if(transport_options.direct_udp)quality=mesh_quality;}
      if(quality&&(!epoch_packets()||epoch==quality->context().epoch))quality->receive(source,packet,transfer_time());
      return;
    }
    if(PeerSourceAdmissions::is_packet(packet)){
      std::shared_ptr<PeerSourceAdmissions> admissions;
      {std::lock_guard lock(mutex);if(transport_options.direct_udp&&native_control_phase())admissions=source_admissions;}
      if(admissions&&(!epoch_packets()||epoch==admissions->context().epoch))admissions->receive(source,packet);
      return;
    }
    if(PeerAuthorityTransfer::is_packet(packet)){
      std::shared_ptr<PeerAuthorityTransfer> transfer;
      {std::lock_guard lock(mutex);if(transport_options.direct_udp&&native_control_phase())transfer=authority;}
      if(transfer&&(!epoch_packets()||epoch==transfer->context().epoch))transfer->receive(source,packet,transfer_time());
      return;
    }
    std::lock_guard lock(mutex);
    fence_peer_generation();
    const bool frozen_selection=selection_started&&phase==Phase::AwaitingWorld;
    if(phase!=Phase::Running && phase!=Phase::Reviewing && phase!=Phase::Postgame&&!frozen_selection)return;
    require(source<roster.size() && source!=self && online_admission_peer(roster[source])==source);
    if(epoch_packets() && (epoch!=continuity_state.epoch || !(continuity_state.members&(1u<<source))))return;
    require(packet.size()>=7 && packet.size()<=1400 && get(packet,0,4)==0x324e3248 && get(packet,4,2)==native_multiplayer_protocol);
    const auto kind=get(packet,6,1);
    require((self==host && (kind==1 || kind==2 || kind==4 || kind==6)) ||
            (source==host && (kind==3 || kind==5 || kind==6)));
    if(kind!=6) {
      // A native freeze cannot manufacture predecessor silence. Authenticated
      // current-host traffic still services suspicion; it never reaches the
      // frozen native owner or undoes an already accepted election vote.
      if(agreement&&source==host&&(phase==Phase::Running||frozen_selection))agreement->observe_host(transfer_time());
      if(phase!=Phase::Running)return;
      if(incoming.size()<512)incoming.push_back({source,{packet.begin(),packet.end()},epoch});
      return;
    }
    if(frozen_selection)return;
    // The mandatory Rust reader returns owned syntax-checked fields. Sender,
    // seat, role and generation authority still comes from this real session.
    std::optional<VoiceOrigin::Signature> origin_signature;
    if(packet.size()>7&&get(packet,7,1)==0){
      if(packet.size()<=64)return;
      VoiceOrigin::Signature signature;std::copy(packet.end()-64,packet.end(),signature.begin());
      packet=packet.first(packet.size()-64);
      const auto speaker=get(packet,8,1);
      if(speaker>=roster.size()||!voice_origin.verify(packet,signature,online_admission_peer(roster[speaker]),epoch))return;
      origin_signature=signature;
    }
    auto parsed=decode_online_voice_frame(packet,native_multiplayer_protocol);
    require(parsed.has_value());
    MatchVoicePacket voice;voice.peer=source;
    if(const auto *preferences=std::get_if<ParsedOnlineVoicePreferences>(&*parsed)) {
      require(!preferences->forwarded_source || source==host);
      voice.preferences=true;voice.frame.source=preferences->forwarded_source.value_or(source);
      require(voice.frame.source<roster.size());
      voice.enabled=preferences->enabled;voice.muted=preferences->muted;
    } else {
      auto &frame=std::get<ParsedOnlineVoiceAudio>(*parsed);
      voice.frame.source=frame.source;voice.frame.sequence=frame.sequence;
      require(voice.frame.source<roster.size() && (voice_members&(1u<<voice.frame.source)) &&
          (source==host || online_admission_peer(roster[voice.frame.source])==source));
      voice.frame.radio=frame.radio;voice.frame.gains={frame.left,frame.right};
      // The coordinator supplies the account block mask. The receiving PC
      // enforces it even when a player host forwards a blocked speaker.
      if(self!=host && (voice_blocked&(1u<<voice.frame.source)))return;
      voice.frame.opus=std::move(frame.opus);
      voice.frame.origin_signature=origin_signature;
    }
    voice.frame.delivery_generation=audio_generation;
    if(voices.size()==80)voices.pop_front();voices.push_back(std::move(voice));
  }
  void fail(const std::string &message) {
    std::shared_ptr<GameVoice> stopped_audio;
    {std::lock_guard lock(mutex);if(phase!=Phase::Postgame)phase=Phase::Failed;
      error=message;voice_enabled.fill(false);fence_audio();stopped_audio=audio;}
    if(stopped_audio)stopped_audio->enable(false);
  }
  void run_eos_authentication(const std::shared_ptr<OnlinePeerTransport> &direct,const std::string &room_api,
                              const std::string &bearer,std::stop_token stop) {
    try{
      while(!stop.stop_requested()){
        {std::lock_guard lock(mutex);if(phase==Phase::Failed)return;}
        // Rust returns an owned message. No transport/SDK or Session mutex is
        // held during HTTP, including the slower official proof verification.
        const auto request=direct->authentication_request();
        if(!request.kind){std::this_thread::sleep_for(std::chrono::milliseconds(20));continue;}
        const auto response=request_eos_auth(room_api,bearer,request,stop);
        if(stop.stop_requested())return;
        direct->complete_authentication(response);
      }
    }catch(const std::exception &e){fail(e.what());}
    catch(...){fail("EOS account authentication could not complete.");}
  }
  void run_peers(const std::shared_ptr<OnlinePeerTransport> &direct,std::stop_token stop) {
    try {
      std::vector<unsigned> physical;
      for(const auto &player:roster)if(online_admission_peer(player)==player.slot)physical.push_back(player.slot);
      bool announced=false;
      std::optional<Clock::time_point> measurement_started;
      while(!stop.stop_requested()) {
        std::uint16_t ready_mask=0;for(auto peer:physical)if(peer!=self&&direct->ready(peer))ready_mask|=std::uint16_t(1u<<peer);
        direct_ready.store(ready_mask,std::memory_order_relaxed);
        for(auto &signal:direct->signals()) {
          const auto value=Json{{"peer_signal",{{"peer",signal.peer},{"type",signal.type},{"payload",signal.payload},{"mid",signal.mid}}}}.dump();
          std::lock_guard lock(mutex);require(continuation_outgoing.size()<1024);continuation_outgoing.push_back(value);
        }
        if(!measurement_started&&direct->authentication_ready())measurement_started=Clock::now();
        std::shared_ptr<PeerMeshQuality> quality;Phase current_phase;
        {std::lock_guard lock(mutex);quality=mesh_quality;current_phase=phase;}
        const auto measured=quality?quality->local(transfer_time()):std::nullopt;
        const auto complete_measurement=std::all_of(physical.begin(),physical.end(),[&](auto peer){
          return peer==self||(measured&&measured->links[peer].measured);});
        if(!announced&&measurement_started&&
            (Clock::now()-*measurement_started>=std::chrono::seconds(transport_options.eos?45:10)||complete_measurement)) {
          Json ready=Json::array();
          for(auto peer:physical)if(peer!=self){
            const auto q=measured?measured->links[peer]:PeerLinkQuality{};
            ready.push_back({{"peer",peer},{"rtt_ms",q.measured?Json(q.rtt_ms):Json(nullptr)},
              {"upload_bytes_per_second",q.measured?Json(q.upload_bytes_per_second):Json(nullptr)},
              {"loss_per_mille",q.measured?Json(q.loss_per_mille):Json(nullptr)}});
          }
          std::lock_guard lock(mutex);continuation_outgoing.push_back(Json{{"peer_ready",ready}}.dump());announced=true;
        }
        std::vector<std::byte> packet;unsigned peer{};
        for(unsigned received=0;received<64 && direct->receive(packet,peer);++received) {
          try {
            const auto offset=epoch_packets()?4u:0u;require(packet.size()>=offset+7);
            datagram(peer,std::span(packet).subspan(offset),epoch_packets()?get(packet,0,4):0);
          } catch(const std::exception &) {
            // Count actual receive/dispatch rejection without logging payloads
            // or account IDs. Untrusted bytes cannot evict healthy peers.
            auto value=rejected_direct.load(std::memory_order_relaxed);
            while(value!=UINT64_MAX && !rejected_direct.compare_exchange_weak(
                value,value+1,std::memory_order_relaxed)){}
          }
        }
        std::deque<Frame> frames;std::function<void()> pump;
        {std::lock_guard lock(mutex);sync_audio();frames.swap(outgoing);pump=postgame_voice_pump;}
        if(pump)pump();
        for(auto &frame:frames) {
          std::lock_guard lock(mutex);fence_peer_generation();
          if(peer_fenced || (epoch_packets()&&frame.epoch!=continuity_state.epoch))continue;
          if(frame.data.size()>6&&frame.data[6]==std::byte{6}&&frame.audio_generation!=audio_generation)continue;
          unsigned target=frame.peer;
          if(phase==Phase::Postgame && self!=host && frame.data.size()>=7 && get(frame.data,6,1)==6)target=host;
          const auto members=phase==Phase::Reviewing||phase==Phase::Postgame?voice_members:continuity_state.members;
          if(epoch_packets()&&!(members&(1u<<target)))continue;
          if(epoch_packets()) {
            std::vector<std::byte> prefix;put(prefix,frame.epoch,4);frame.data.insert(frame.data.begin(),prefix.begin(),prefix.end());
          }
          direct->send(frame.data,target);
        }
        std::shared_ptr<PeerAuthorityTransfer> transfer;
        {std::lock_guard lock(mutex);if(native_control_phase())transfer=authority;}
        if(transfer){const auto epoch=transfer->context().epoch;
          // Gameplay and microphone frames are drained first. Authority data
          // has its own rate/window budget and cannot grow the media queue.
          for(auto packet:transfer->poll(transfer_time(),2)){
            if(epoch_packets()){std::vector<std::byte> prefix;put(prefix,epoch,4);packet.bytes.insert(packet.bytes.begin(),prefix.begin(),prefix.end());}
            direct->send(packet.bytes,packet.peer);
          }
        }
        std::shared_ptr<PeerSourceAdmissions> admissions;
        {std::lock_guard lock(mutex);if(native_control_phase())admissions=source_admissions;}
        if(admissions){const auto epoch=admissions->context().epoch;
          for(auto packet:admissions->poll(transfer_time(),2)){
            if(epoch_packets()){std::vector<std::byte> prefix;put(prefix,epoch,4);packet.bytes.insert(packet.bytes.begin(),prefix.begin(),prefix.end());}
            direct->send(packet.bytes,packet.peer);
          }
        }
        std::shared_ptr<PeerCommonCutAgreement> common;
        {std::lock_guard lock(mutex);if(common_phase()&&common_supported())common=common_cut;}
        if(common){const auto epoch=common->context().epoch;
          for(auto packet:common->poll(transfer_time(),4)){
            if(epoch_packets()){std::vector<std::byte> prefix;put(prefix,epoch,4);packet.bytes.insert(packet.bytes.begin(),prefix.begin(),prefix.end());}
            direct->send(packet.bytes,packet.peer);
          }
        }
        {
          std::lock_guard lock(mutex);
          if(common_phase()&&selection_supported()){
            selection_abort();selection_progress();const auto epoch=survivor_selection->state().context.epoch;
            for(auto packet:survivor_selection->poll(transfer_time(),4)){
              std::vector<std::byte> prefix;put(prefix,epoch,4);packet.bytes.insert(packet.bytes.begin(),prefix.begin(),prefix.end());
              direct->send(packet.bytes,packet.peer);
            }
          }
        }
        std::shared_ptr<PeerSnapshotCandidate> candidate;
        {std::lock_guard lock(mutex);if(common_phase()&&snapshot_supported())candidate=snapshot_candidate;}
        if(candidate){const auto epoch=candidate->state().context.epoch;
          for(auto packet:candidate->poll(transfer_time(),4)){
            std::vector<std::byte> prefix;put(prefix,epoch,4);packet.bytes.insert(packet.bytes.begin(),prefix.begin(),prefix.end());
            direct->send(packet.bytes,packet.peer);
          }
        }
        if(quality){const auto epoch=quality->context().epoch;
          // Only setup/measurement summaries use the coordinator. All probes,
          // replies and full mesh rows use the authenticated direct links.
          for(auto packet:quality->poll(ready_mask,transfer_time(),
              current_phase==Phase::Connecting||current_phase==Phase::Loading?16:4)){
            if(epoch_packets()){std::vector<std::byte> prefix;put(prefix,epoch,4);packet.bytes.insert(packet.bytes.begin(),prefix.begin(),prefix.end());}
            direct->send(packet.bytes,packet.peer);
          }
        }
        {
          std::lock_guard lock(mutex);
          const auto now=transfer_time();
          if(const auto candidate=migration_candidate(now))agreement->drive(*candidate,now);
          // Retain the full bounded generation chain for a delayed learner.
          // All generations share one four-packet budget after gameplay/media.
          for(unsigned i=0;i<4&&peer_handoff_phase();++i){
            const auto index=agreement_cursor++%(agreement_history.size()+1);
            const auto owner=index?agreement_history[index-1].owner:agreement;
            if(!owner)continue;
            const auto epoch=index?agreement_history[index-1].context.epoch:agreement_context->epoch;
            for(auto packet:owner->poll(transfer_time(),1)){
              std::vector<std::byte> prefix;put(prefix,epoch,4);packet.bytes.insert(packet.bytes.begin(),prefix.begin(),prefix.end());
              direct->send(packet.bytes,packet.peer);
            }
          }
          fence_peer_generation();
          if(phase==Phase::Failed)return;
          if(!(transport_options.peer_host_migration&&peer_started)&&host<roster.size()&&self!=host&&phase!=Phase::Postgame&&phase!=Phase::Reviewing&&direct->failed(host))
            throw std::runtime_error("Direct player connection was lost. Search again.");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
    } catch(const std::exception &e){fail(e.what());}
  }
  void run(const std::string &url,const std::string &room_api,const std::string &bearer,std::stop_token stop) {
    try {
      require(curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK);
      std::shared_ptr<OnlinePeerTransport> direct;
      if(transport_options.direct_udp) {
        std::vector<unsigned> physical;
        for(const auto &player:roster)if(online_admission_peer(player)==player.slot)physical.push_back(player.slot);
        if(transport_options.eos){
          const auto &eos=*transport_options.eos;
          require(eos.peers.size()+1==physical.size());
          for(auto peer:physical)if(peer!=self)require(std::count_if(eos.peers.begin(),eos.peers.end(),[&](const auto &p){return p.slot==peer;})==1);
          direct=std::make_shared<OnlinePeerTransport>(self,eos);
        }else direct=std::make_shared<OnlinePeerTransport>(self,physical,transport_options.stun_servers);
      }
      std::jthread peer_worker;
      if(direct)peer_worker=std::jthread([this,direct](std::stop_token token){run_peers(direct,token);});
      // Stop/join this worker before the peer worker and their shared transport.
      std::jthread eos_auth_worker;
      if(direct&&transport_options.eos&&transport_options.eos->account_login)
        eos_auth_worker=std::jthread([this,direct,room_api,bearer](std::stop_token token){
          run_eos_authentication(direct,room_api,bearer,token);
        });
      while(!stop.stop_requested()) {
        try {run_control(url,bearer,stop,direct.get());}
        catch(const TerminalMatchError &){
          std::lock_guard lock(mutex);
          if(!transport_options.peer_host_migration||!peer_started||phase==Phase::Failed)throw;
          error="Master admission ended; direct peers retain authority until the native handoff completes.";
        }
        catch(const std::exception &) {
          // Once admitted, gameplay/audio do not depend on the master socket.
          // Its reconnect worker may stall on TLS without stalling peer UDP.
          std::lock_guard lock(mutex);
          if(!direct || (phase!=Phase::Running && phase!=Phase::Reviewing && phase!=Phase::Postgame&&
              !(transport_options.peer_host_migration&&peer_started&&phase!=Phase::Failed)))throw;
          error="Master connection unavailable; the player-hosted match continues. Results will retry.";
        }
        for(unsigned i=0;i<100 && !stop.stop_requested();++i)std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    } catch(const std::exception &e){fail(e.what());}
  }
  void run_control(const std::string &url,const std::string &bearer,std::stop_token stop,OnlinePeerTransport *direct) {
      ControlPresence presence{control_online};
      require(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
      std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> client(curl_easy_init(), curl_easy_cleanup);
      require(bool(client));
      const auto header = "Authorization: Bearer " + bearer;
      std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(curl_slist_append(nullptr, header.c_str()), curl_slist_free_all);
      curl_easy_setopt(client.get(), CURLOPT_URL, url.c_str());
      curl_easy_setopt(client.get(), CURLOPT_HTTPHEADER, headers.get());
      curl_easy_setopt(client.get(), CURLOPT_CONNECT_ONLY, 2L);
      curl_easy_setopt(client.get(), CURLOPT_PROTOCOLS_STR, "wss,ws");
      curl_easy_setopt(client.get(), CURLOPT_FOLLOWLOCATION, 0L);
      curl_easy_setopt(client.get(), CURLOPT_PROXY, "");
      curl_easy_setopt(client.get(), CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(client.get(), CURLOPT_TIMEOUT_MS, 3000L);
      curl_easy_setopt(client.get(), CURLOPT_CONNECTTIMEOUT_MS, 2000L);
      curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYPEER, 1L);
      curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYHOST, 2L);
#ifdef CURLWS_NOAUTOPONG
      curl_easy_setopt(client.get(), CURLOPT_WS_OPTIONS, CURLWS_NOAUTOPONG);
#endif
      require(curl_easy_perform(client.get()) == CURLE_OK);
      control_online=true;
      auto send = [&](const void *bytes, std::size_t size, unsigned flags) {
        std::size_t offset{};
        const auto deadline = Clock::now() + std::chrono::milliseconds(500);
        for (;;) {
          std::size_t sent{};
          const auto code = curl_ws_send(client.get(), static_cast<const std::byte*>(bytes)+offset, size-offset, &sent, 0, flags);
          require(code == CURLE_OK || code == CURLE_AGAIN);
          offset+=sent;
          if(offset==size)break;
          require(!stop.stop_requested() && Clock::now() < deadline);
          std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
      };
      std::vector<std::byte> assembled;
      bool loaded_sent = false, completion_sent = false, confirmation_sent = false;
      auto last = Clock::now();
      while (!stop.stop_requested()) {
        for (unsigned i = 0; i < 256; ++i) {
          std::byte chunk[1401]; std::size_t count{}; const curl_ws_frame *meta{};
          const auto result = curl_ws_recv(client.get(), chunk, sizeof(chunk), &count, &meta);
          if (result == CURLE_AGAIN) break;
          require(result == CURLE_OK && meta);
          if(meta->flags&CURLWS_CLOSE) {
            std::string reason="ONLINE match ended.";
            if(count>2 && count<=122)reason.assign(reinterpret_cast<const char*>(chunk+2),count-2);
            const auto code=count>=2?(std::to_integer<unsigned>(chunk[0])<<8)|std::to_integer<unsigned>(chunk[1]):1000;
            if(direct && code!=1001 && reason!="Match service is restarting.")throw TerminalMatchError(reason);
            throw std::runtime_error(reason);
          }
          last = Clock::now();
          if (meta->flags & CURLWS_PING) { require(count <= 125 && !meta->bytesleft); send(chunk, count, CURLWS_PONG); continue; }
          if (meta->flags & CURLWS_PONG) continue;
          require((meta->flags & (CURLWS_BINARY | CURLWS_TEXT)) && assembled.size() + count <= 119000);
          assembled.insert(assembled.end(), chunk, chunk + count);
          if (meta->bytesleft || (meta->flags & CURLWS_CONT)) continue;
          if (meta->flags & CURLWS_TEXT) {
            const auto data = Json::parse(reinterpret_cast<const char *>(assembled.data()), reinterpret_cast<const char *>(assembled.data()+assembled.size()));
            if(data.contains("voice_auth")) {
              require(data.size()==1);const auto &value=data["voice_auth"];
              require(value.is_object()&&value.size()==3&&value["version"]==1&&value["keys"].is_array()&&value["keys"].size()==roster.size());
              const auto key=[](const Json &text){
                require(text.is_string());const auto hex=text.get<std::string>();
                require(hex.size()==64&&hex.find_first_not_of("0123456789abcdef")==std::string::npos);
                VoiceOrigin::Key result;for(unsigned i=0;i<32;++i)result[i]=std::stoul(hex.substr(i*2,2),nullptr,16);return result;
              };
              std::vector<VoiceOrigin::Key> keys;for(const auto &item:value["keys"])keys.push_back(key(item));
              for(const auto &player:roster)require(keys[player.slot]==keys[online_admission_peer(player)]);
              std::lock_guard lock(mutex);require(voice_origin.configure(key(value["seed"]),std::move(keys),self));
            } else if(data.contains("peer_signal")) {
              require(direct && data.size()==1);
              const auto &signal=data["peer_signal"];
              require(signal.is_object() && signal.size()==4 && signal["peer"].is_number_unsigned() &&
                  signal["type"].is_string() && signal["payload"].is_string() && signal["mid"].is_string());
              direct->signal({signal["peer"].get<unsigned>(),signal["type"].get<std::string>(),
                  signal["payload"].get<std::string>(),signal["mid"].get<std::string>()});
            } else if(data.contains("world_seal")) {
              require(supports_continuity && data.size()==1);auto seal=parse_seal(data["world_seal"]);
              std::lock_guard lock(mutex);require(seal.epoch<=continuity_state.epoch);
              if(seals.size()==64)seals.pop_front();seals.push_back(std::move(seal));
            } else if(data.contains("postgame")) {
              require(data.size()==1 && data["postgame"].is_string());
              const auto hex=data["postgame"].get<std::string>();
              require(hex.size()==2*0xdc78 && hex.find_first_not_of("0123456789abcdef")==std::string::npos);
              std::vector<std::byte> bytes;bytes.reserve(0xdc78);
              for(std::size_t i=0;i<hex.size();i+=2)bytes.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
              std::lock_guard lock(mutex);require(phase==Phase::Running || (direct && (phase==Phase::Reviewing || phase==Phase::Postgame)));
              require(report.empty() || report==bytes);report=std::move(bytes);phase=Phase::Reviewing;
            } else if(data.contains("voice_blocked")) {
              require(data.size()==1 && data["voice_blocked"].is_number_unsigned());
              const auto mask=data["voice_blocked"].get<unsigned>();
              require(!(mask&~((1u<<roster.size())-1)) && !(mask&(1u<<self)));
              std::lock_guard lock(mutex);voice_blocked=std::uint16_t(mask);
            } else if(data.contains("voice_host")) {
              require(data.size()==2 && data["voice_host"].is_number_unsigned() && data["voice_members"].is_number_unsigned());
              const auto leader=data["voice_host"].get<unsigned>(),members=data["voice_members"].get<unsigned>();
              std::lock_guard lock(mutex);
              require((phase==Phase::Postgame || ((supports_continuity || transport_options.direct_udp) && phase==Phase::Reviewing)) && leader<roster.size() && (members&(1u<<leader)) &&
                  (members&(1u<<self)) && !(members&~((1u<<roster.size())-1)));
              require(online_admission_peer(roster[leader])==leader && !roster[leader].guest);
              for(const auto &p:roster)require(bool(members&(1u<<p.slot))==bool(members&(1u<<online_admission_peer(p))));
              if(host!=leader||voice_members!=members)fence_audio();
              host=leader;voice_members=std::uint16_t(members);
              for(unsigned slot=0;slot<16;++slot)if(!(members&(1u<<slot)))voice_enabled[slot]=false;
            } else if(data.contains("completed")) {
              require(data.size()==1 && data["completed"].is_string());
              std::lock_guard lock(mutex);require(phase==Phase::Reviewing && !confirmation.empty() && data["completed"].get<std::string>()==digest(report));
              phase=Phase::Postgame;
            } else if (data.contains("ping")) {
              require(data.size() == 1 && data["ping"].is_string() && data["ping"].get<std::string>().size() == 16);
              const auto pong = Json{{"pong", data["ping"]}}.dump(); send(pong.data(), pong.size(), CURLWS_TEXT);
            } else {
              control(data);
            }
          } else {
            require(!direct);
            const auto offset=supports_continuity?5u:1u;
            require(assembled.size()>=offset+7 && assembled.size()<=offset+1400);
            datagram(std::to_integer<unsigned>(assembled.front()),std::span(assembled).subspan(offset),supports_continuity?get(assembled,1,4):0);
          }
          assembled.clear();
        }
        require(Clock::now() - last < std::chrono::seconds(20));
        std::function<void()> pump;
        {std::lock_guard lock(mutex);sync_audio();pump=postgame_voice_pump;}
        if(pump && !direct)pump();
        std::deque<Frame> frames;std::deque<std::string> commands; std::string admission, publication, approval;
        { std::lock_guard lock(mutex); if(!direct)frames.swap(outgoing); if (!loaded_sent && phase == Phase::Loading) admission = loaded;
          commands.swap(continuation_outgoing);
          if(!completion_sent && phase==Phase::Running)publication=completion;
          if(!confirmation_sent && phase==Phase::Reviewing)approval=confirmation; }
        if (!admission.empty()) { const auto data = Json{{"loaded", admission}}.dump(); send(data.data(), data.size(), CURLWS_TEXT); loaded_sent = true; }
        for(const auto &command:commands)send(command.data(),command.size(),CURLWS_TEXT);
        for (auto &frame : frames) {
          {std::lock_guard lock(mutex);if(frame.data.size()>6&&frame.data[6]==std::byte{6}&&frame.audio_generation!=audio_generation)continue;}
          if(supports_continuity) {
            std::vector<std::byte> prefix;put(prefix,frame.epoch,4);
            frame.data.insert(frame.data.begin(),prefix.begin(),prefix.end());
          }
          require(!direct);
          frame.data.insert(frame.data.begin(), std::byte(frame.peer));
          send(frame.data.data(), frame.data.size(), CURLWS_BINARY);
        }
        if(!publication.empty()) {send(publication.data(),publication.size(),CURLWS_TEXT);completion_sent=true;}
        if(!approval.empty()) {send(approval.data(),approval.size(),CURLWS_TEXT);confirmation_sent=true;}
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
      if(direct) {
        const auto value=Json{{"leave",true}}.dump();
        try {send(value.data(),value.size(),CURLWS_TEXT);} catch(const std::exception &) {}
      }
      std::size_t ignored{};curl_ws_send(client.get(),"",0,&ignored,0,CURLWS_CLOSE);
  }

};
OnlineMatchSession::OnlineMatchSession(std::string o, std::string r, std::string b, unsigned s, std::vector<OnlineAdmission> roster, bool local,std::optional<bool> teams)
    : OnlineMatchSession(std::move(o),std::move(r),std::move(b),s,std::move(roster),local,teams,false) {}
OnlineMatchSession::OnlineMatchSession(std::string o,std::string r,std::string b,unsigned s,
    std::vector<OnlineAdmission> roster,bool local,std::optional<bool> teams,bool continuation)
    : OnlineMatchSession(std::move(o),std::move(r),std::move(b),s,std::move(roster),local,teams,continuation,{}) {}
OnlineMatchSession::OnlineMatchSession(std::string o,std::string r,std::string b,unsigned s,
    std::vector<OnlineAdmission> roster,bool local,std::optional<bool> teams,bool continuation,OnlineMatchTransportOptions transport)
    : impl_(std::make_unique<Impl>(std::move(o), std::move(r), std::move(b), s, std::move(roster), local,teams,continuation,std::move(transport))) {
  std::lock_guard lock(impl_->mutex);impl_->postgame_voice_pump=[this]{pump_postgame_voice();};
}
OnlineMatchSession::~OnlineMatchSession() {
  impl_->worker.request_stop();if(impl_->worker.joinable())impl_->worker.join();
  if(impl_->audio)impl_->audio->enable(false);
}
OnlineMatchSession::Phase OnlineMatchSession::phase() const { std::lock_guard lock(impl_->mutex); return impl_->phase; }
unsigned OnlineMatchSession::self() const { return impl_->self; }
bool OnlineMatchSession::direct_udp() const {return impl_->transport_options.direct_udp;}
std::uint64_t OnlineMatchSession::rejected_direct_dispatches()const noexcept {
  return impl_->rejected_direct.load(std::memory_order_relaxed);
}
bool OnlineMatchSession::snapshot_candidates_enabled()const{return impl_->transport_options.snapshot_candidates;}
std::shared_ptr<PeerSnapshotCandidate> OnlineMatchSession::snapshot_candidates()const{
  std::lock_guard lock(impl_->mutex);
  return impl_->common_phase()&&impl_->snapshot_supported()?impl_->snapshot_candidate:nullptr;
}
bool OnlineMatchSession::master_connected() const {return impl_->control_online;}
std::vector<unsigned> OnlineMatchSession::local_slots() const {
  std::vector<unsigned> slots;
  for(const auto &player:impl_->roster)
    if(online_admission_peer(player)==impl_->self)slots.push_back(player.slot);
  return slots;
}
unsigned OnlineMatchSession::host() const { std::lock_guard lock(impl_->mutex); return impl_->host; }
std::optional<bool> OnlineMatchSession::team_mode() const { return impl_->team_mode; }
const std::vector<OnlineAdmission> &OnlineMatchSession::roster() const { return impl_->roster; }
std::string OnlineMatchSession::error() const { std::lock_guard lock(impl_->mutex); return impl_->error; }
std::optional<OnlineContinuityState> OnlineMatchSession::continuity() const {
  std::lock_guard lock(impl_->mutex);if(!impl_->epoch_packets())return std::nullopt;return impl_->continuity_state;
}
std::vector<OnlineWorldSeal> OnlineMatchSession::world_seals() {
  std::lock_guard lock(impl_->mutex);std::vector<OnlineWorldSeal> result(impl_->seals.begin(),impl_->seals.end());impl_->seals.clear();return result;
}
std::optional<AuthorityTransferContext> OnlineMatchSession::authority_context() const {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  if(!transfer)return std::nullopt;
  return transfer->context();
}
bool OnlineMatchSession::authority_supported(unsigned peer) const {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return peer<16&&(impl_->direct_ready.load(std::memory_order_relaxed)&(1u<<peer))&&transfer&&transfer->supported(peer);
}
bool OnlineMatchSession::authority_replay_supported(unsigned peer) const {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return peer<16&&(impl_->direct_ready.load(std::memory_order_relaxed)&(1u<<peer))&&transfer&&
      transfer->feature_supported(peer,authority_feature_source_replay|authority_feature_source_state|authority_feature_source_camera|authority_feature_source_lens_query|authority_feature_source_lens_draw|authority_feature_source_particle_collision|authority_feature_source_projectile_contacts);
}
bool OnlineMatchSession::authority_pipeline_supported(unsigned peer)const{
  std::shared_ptr<PeerAuthorityTransfer> transfer;
  {std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return peer<16&&(impl_->direct_ready.load(std::memory_order_relaxed)&(1u<<peer))&&transfer&&
      transfer->feature_supported(peer,authority_feature_source_pipeline);
}
bool OnlineMatchSession::authority_delivered(unsigned peer,const AuthorityTransferContext &context,const AuthorityCutSeal &seal)const{
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return transfer&&transfer->delivered_to(context,seal,peer);
}
bool OnlineMatchSession::authority_host_continuation_supported(unsigned peer)const{
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return peer<16&&(impl_->direct_ready.load(std::memory_order_relaxed)&(1u<<peer))&&transfer&&
      transfer->feature_supported(peer,authority_feature_source_host_continuation);
}
std::vector<PeerHostQuality> OnlineMatchSession::host_candidates()const {
  std::shared_ptr<PeerMeshQuality> quality;unsigned host;Phase phase;
  {std::lock_guard lock(impl_->mutex);quality=impl_->mesh_quality;host=impl_->host;phase=impl_->phase;}
  if(!quality||phase==Phase::Connecting||phase==Phase::Failed||quality->context().host!=host)return {};
  return quality->candidates(transfer_time());
}
std::optional<PeerHostAgreementState> OnlineMatchSession::peer_migration()const{
  std::lock_guard lock(impl_->mutex);
  if(!impl_->agreement)return std::nullopt;
  return impl_->agreement->state(transfer_time());
}
OnlineMatchSession::MigrationPreparationStep OnlineMatchSession::migration_preparation_request(){
  auto &s=*impl_;std::lock_guard lock(s.mutex);
  using Outcome=MigrationPreparationOutcome;
  if(!s.preparation_driver)return {Outcome::Inactive,{}};
  s.selection_abort();
  const auto observation=s.preparation_observation(transfer_time());
  const auto action=s.preparation_driver->step(observation);
  using Kind=PeerMigrationPreparationActionKind;
  if(action.kind==Kind::Terminal){s.preparation_terminal();return {Outcome::Terminal,{}};}
  if(action.kind==Kind::Published)return {Outcome::Published,{}};
  if(action.kind==Kind::Wait)return {Outcome::Inactive,{}};
  if(!action.ticket)return {Outcome::Refused,{}};
  if(action.kind==Kind::Retire&&!s.preparation_driver->reserve_retirement(observation,*action.ticket))return {Outcome::Refused,{}};
  return {action.kind==Kind::Prepare?Outcome::Preparing:Outcome::Retiring,
      MigrationPreparationRequest(action.kind==Kind::Prepare?MigrationPreparationKind::Prepare:MigrationPreparationKind::Retire,
          action.ticket->context,action.ticket->self,action.ticket->id,s.preparation_identity)};
}
OnlineMatchSession::MigrationPreparationOutcome OnlineMatchSession::complete_migration_preparation(
    const MigrationPreparationRequest &request,const std::optional<PeerSurvivorInventory> &inventory){
  auto &s=*impl_;std::lock_guard lock(s.mutex);
  using Outcome=MigrationPreparationOutcome;
  if(!s.preparation_driver||request.owner_.lock()!=s.preparation_identity)return Outcome::Refused;
  const PeerMigrationPreparationTicket ticket{request.context(),request.self(),request.id()};
  s.selection_abort();
  if(request.kind()==MigrationPreparationKind::Retire){
    if(inventory)return Outcome::Refused;
    const auto result=s.preparation_driver->retired(s.preparation_observation(transfer_time()),ticket);
    switch(result){
    case PeerMigrationPreparationRetirement::Retryable:return Outcome::Retryable;
    case PeerMigrationPreparationRetirement::RetiredButFenced:s.preparation_terminal();return Outcome::Terminal;
    case PeerMigrationPreparationRetirement::Stale:return Outcome::Refused;
    }
    return Outcome::Refused;
  }
  // This records only the exact caller-retained lease identity. It grants no
  // native readiness; fresh admission and actual selector publication follow
  // under this same lock, after all fallible native capture has returned.
  if(!s.preparation_driver->retained(ticket,inventory?inventory->lease:0))return Outcome::Refused;
  const auto observation=s.preparation_observation(transfer_time());
  if(inventory){
    const auto common=s.common_cut?std::optional{s.common_cut->state()}:std::nullopt;
    const auto admission=s.preparation_driver->admit_freeze(observation,ticket,*inventory,common);
    switch(admission.kind){
    case PeerMigrationPreparationFreezeKind::Publish:{
      const bool published=s.survivor_selection->publish_inventory(*inventory);
      if(published||s.survivor_selection->state().failed)s.selection_started=true;
      s.selection_abort();s.selection_progress();
      const auto after=s.preparation_driver->step(s.preparation_observation(transfer_time()));
      if(after.kind==PeerMigrationPreparationActionKind::Terminal){s.preparation_terminal();return Outcome::Terminal;}
      return after.kind==PeerMigrationPreparationActionKind::Published?Outcome::Published:Outcome::Refused;
    }
    case PeerMigrationPreparationFreezeKind::Invalidate:
      s.survivor_selection->invalidate();s.selection_started=true;s.selection_progress();break;
    case PeerMigrationPreparationFreezeKind::Retire:break;
    case PeerMigrationPreparationFreezeKind::Refuse:break;
    }
  }
  const auto action=s.preparation_driver->step(s.preparation_observation(transfer_time()));
  if(action.kind==PeerMigrationPreparationActionKind::Terminal){s.preparation_terminal();return Outcome::Terminal;}
  if(action.kind==PeerMigrationPreparationActionKind::Retire)return Outcome::Retiring;
  if(action.kind==PeerMigrationPreparationActionKind::Published)return Outcome::Published;
  return Outcome::Refused;
}
bool OnlineMatchSession::migration_preparation_retiring()const{
  auto &s=*impl_;std::lock_guard lock(s.mutex);
  return s.preparation_driver&&s.preparation_driver->retirement_pending();
}
bool OnlineMatchSession::prepare_peer_migration(const ValidatedAuthorityCut &cut){
  std::lock_guard lock(impl_->mutex);
  if(impl_->preparation_driver&&impl_->preparation_driver->blocks_publication())return false;
  if(!impl_->peer_handoff_phase()||!impl_->agreement||!impl_->source_admissions)return false;
  const auto admitted=impl_->source_admissions->accepted();
  if(!admitted||admitted->context!=cut.context||(impl_->continuity_admissions&&admitted->players!=*impl_->continuity_admissions))return false;
  if(impl_->transport_options.survivor_cut_selection){
    impl_->selection_abort();impl_->selection_progress();
    if(!impl_->selection_started||!impl_->selection_supported()||
        !impl_->survivor_selection->allows_ready(impl_->self,cut.seal))return false;
  }
  const auto result=impl_->agreement->publish_ready(cut);
  if(result&&impl_->transport_options.survivor_cut_selection)
    return impl_->survivor_selection->observe_ready(impl_->self,cut.seal);
  return result;
}
bool OnlineMatchSession::freeze_survivor_selection(const AuthorityTransferContext &context,const PeerSurvivorInventory &inventory){
  auto &s=*impl_;std::lock_guard lock(s.mutex);
  if(!s.preparation_driver||!s.survivor_selection||s.survivor_selection->state().context!=context)return false;
  const auto common=s.common_cut?std::optional(s.common_cut->state()):std::nullopt;
  const auto observation=s.preparation_observation(transfer_time());
  switch(s.preparation_driver->admit_external_freeze(observation,inventory,common).kind){
  case PeerMigrationPreparationFreezeKind::Invalidate:
    s.survivor_selection->invalidate();s.selection_started=true;s.selection_progress();
    (void)s.preparation_driver->step(s.preparation_observation(transfer_time()));return false;
  case PeerMigrationPreparationFreezeKind::Refuse:
  case PeerMigrationPreparationFreezeKind::Retire:return false;
  case PeerMigrationPreparationFreezeKind::Publish:break;
  }
  const auto result=s.survivor_selection->publish_inventory(inventory);
  if(result||s.survivor_selection->state().failed)s.selection_started=true;
  s.selection_abort();s.selection_progress();
  const auto after=s.preparation_driver->step(s.preparation_observation(transfer_time()));
  if(after.kind==PeerMigrationPreparationActionKind::Terminal)s.preparation_terminal();
  return result&&!s.survivor_selection->state().failed;
}
std::optional<PeerSurvivorSelectionState> OnlineMatchSession::survivor_selection()const{
  std::lock_guard lock(impl_->mutex);
  if(!impl_->survivor_selection)return {};
  return impl_->survivor_selection->state();
}
bool OnlineMatchSession::invalidate_survivor_selection(const AuthorityTransferContext &context,std::optional<AuthorityCutSeal> cut){
  auto &s=*impl_;std::lock_guard lock(s.mutex);
  if(!s.selection_started||!s.survivor_selection||s.survivor_selection->state().context!=context)return false;
  const auto result=s.survivor_selection->invalidate(cut);s.selection_progress();return result;
}
bool OnlineMatchSession::propose_peer_migration(unsigned peer){
  std::lock_guard lock(impl_->mutex);
  if(!impl_->peer_handoff_phase()||!impl_->agreement)return false;
  if(impl_->transport_options.survivor_cut_selection&&(!impl_->selection_started||!impl_->selection_supported()||
      !impl_->survivor_selection->state().selection_complete||impl_->survivor_selection->state().failed))return false;
  const auto result=impl_->agreement->propose(peer,transfer_time());impl_->fence_peer_generation();return result;
}
std::optional<unsigned> OnlineMatchSession::migration_candidate()const{
  std::lock_guard lock(impl_->mutex);return impl_->migration_candidate(transfer_time());
}
bool OnlineMatchSession::propose_peer_migration(){
  std::lock_guard lock(impl_->mutex);const auto now=transfer_time();const auto peer=impl_->migration_candidate(now);
  if(!peer)return false;
  const bool result=impl_->agreement->propose(*peer,now);impl_->fence_peer_generation();return result;
}
bool OnlineMatchSession::finish_peer_migration(const PeerMigrationDecision &decision){
  return finish_peer_migration(decision,nullptr,nullptr);
}
bool OnlineMatchSession::finish_peer_migration(const PeerMigrationDecision &decision,NativeCommit commit,void *native){
  auto &s=*impl_;std::lock_guard lock(s.mutex);
  if(!s.peer_handoff_phase()||!s.agreement||!s.agreement_context||!s.authority||!s.source_admissions)return false;
  if(s.transport_options.survivor_cut_selection){
    s.selection_abort();s.selection_progress();
    if(!s.selection_started||!s.selection_supported()||!s.survivor_selection->allows_ready(s.self,decision.value.cut))return false;
  }
  const auto agreement_state=s.agreement->state(transfer_time());
  const auto &learned=agreement_state.decision;
  if(!learned||*learned!=decision||decision.previous!=*s.agreement_context||s.agreement_history.size()>=15||
      !(decision.next.peers&(1u<<s.self))||s.authority->context()!=decision.previous)return false;
  if(s.common_cut){const auto common=s.common_cut->state();
    if(common.abort&&common.pending&&common.pending->context==decision.previous&&common.pending->seal==decision.value.cut)return false;
  }
  // Majority election selects a host; it does not prove every physical
  // survivor retained that exact preparation. Rust enforces the fixed set
  // before any successor owner is allocated or native publication is called.
  std::array<online_protocol_bridge::MigrationPreparationCut,16> prepared{};
  for(unsigned peer=0;peer<prepared.size();++peer)
    if(agreement_state.ready[peer])prepared[peer]=preparation_cut(*agreement_state.ready[peer]);
  const auto selected=preparation_cut(decision.value.cut);
  if(!online_protocol_bridge::all_survivors_prepared(decision.previous.peers,decision.next.peers,
      std::uint8_t(decision.previous.host),std::uint8_t(decision.value.host),selected,
      {prepared.data(),prepared.size()}))return false;
  const auto prior=s.source_admissions->accepted();
  if(!prior||prior->context!=decision.previous||(s.continuity_admissions&&*s.continuity_admissions!=prior->players))return false;
  if(s.audio&&!s.audio->ready(s.audio_generation))return false;
  if(s.snapshot_candidate){const auto candidate=s.snapshot_candidate->state();
    if(candidate.pending&&!(candidate.retired&(std::uint16_t(1)<<s.self)))return false;
  }
  // Construct every successor owner before publication. Outstanding old-context
  // workers retain only their old objects and cannot reset a new epoch.
  auto transfer=std::make_shared<PeerAuthorityTransfer>(s.self,decision.next,source_transfer_limits(s.transport_options.snapshot_candidates,s.transport_options.survivor_cut_selection));
  std::shared_ptr<PeerSourceAdmissions> admissions=PeerSourceAdmissions::resume_after_host_loss(s.self,decision.next,s.roster,*prior);
  std::shared_ptr<PeerMeshQuality> quality;
  std::shared_ptr<PeerHostAgreement> agreement;
  std::shared_ptr<PeerCommonCutAgreement> common;
  std::shared_ptr<PeerSurvivorSelection> selection;
  std::unique_ptr<PeerMigrationPreparation> preparation;
  auto preparation_identity=std::make_shared<const unsigned char>(0);
  std::shared_ptr<PeerSnapshotCandidate> candidate;
  if(std::popcount(decision.next.peers)>1){
    quality=std::make_shared<PeerMeshQuality>(s.self,decision.next);
    agreement=std::make_shared<PeerHostAgreement>(s.self,decision.next,transfer_time());
    common=std::make_shared<PeerCommonCutAgreement>(s.self,decision.next);
    if(s.transport_options.survivor_cut_selection&&s.self!=decision.next.host){
      selection=std::make_shared<PeerSurvivorSelection>(s.self,decision.next);
      preparation=std::make_unique<PeerMigrationPreparation>(s.self,decision.next);
    }
    if(s.transport_options.snapshot_candidates)candidate=std::make_shared<PeerSnapshotCandidate>(s.self,decision.next);
  }
  std::uint16_t members{};
  for(const auto &p:s.roster)if(decision.next.peers&(1u<<online_admission_peer(p)))members|=std::uint16_t(1u<<p.slot);
  auto continuity_players=prior->players;
  if(commit&&!commit(native))return false;
  s.continuity_admissions=std::move(continuity_players);s.authority=std::move(transfer);s.source_admissions=std::move(admissions);s.mesh_quality=std::move(quality);
  s.common_cut=std::move(common);
  s.survivor_selection=std::move(selection);s.selection_started=false;
  s.preparation_driver=std::move(preparation);s.preparation_identity=std::move(preparation_identity);
  s.snapshot_candidate=std::move(candidate);
  s.agreement_history.push_back({*s.agreement_context,s.agreement});
  s.agreement=std::move(agreement);s.agreement_context=decision.next;s.peer_fenced=false;
  s.host=decision.next.host;s.continuity_state={decision.next.epoch,s.host,members,std::nullopt};s.voice_members=members;
  for(unsigned slot=0;slot<16;++slot)if(!(members&(1u<<slot)))s.voice_enabled[slot]=false;
  s.incoming.clear();s.outgoing.clear();s.voices.clear();s.phase=Phase::Running;
  s.sync_audio();
  return true;
}
bool OnlineMatchSession::publish_source_players(const AuthorityTransferContext &context,std::span<const SourcePlayerAdmission> players) {
  std::shared_ptr<PeerSourceAdmissions> admissions;
  {std::lock_guard lock(impl_->mutex);require(impl_->transport_options.direct_udp&&impl_->phase==Phase::Running&&impl_->host==impl_->self);
    if(impl_->continuity_admissions&&!std::ranges::equal(players,*impl_->continuity_admissions))return false;
    admissions=impl_->source_admissions;}
  return admissions&&admissions->publish(context,players);
}
std::optional<AcceptedSourcePlayers> OnlineMatchSession::source_players()const {
  std::lock_guard lock(impl_->mutex);const auto admissions=impl_->source_admissions;
  const auto value=admissions?admissions->accepted():std::nullopt;
  if(value&&impl_->continuity_admissions&&value->players!=*impl_->continuity_admissions)return std::nullopt;
  return value;
}
bool OnlineMatchSession::source_players_confirmed(unsigned peer)const {
  std::shared_ptr<PeerSourceAdmissions> admissions;{std::lock_guard lock(impl_->mutex);admissions=impl_->source_admissions;}
  return peer<16&&(impl_->direct_ready.load(std::memory_order_relaxed)&(1u<<peer))&&admissions&&admissions->confirmed(peer);
}
AuthorityCutSeal OnlineMatchSession::publish_authority(const AuthorityTransferContext &context,std::uint64_t tick,
    std::vector<std::byte> archive,std::span<const unsigned> targets) {
  std::shared_ptr<PeerAuthorityTransfer> transfer;
  {std::lock_guard lock(impl_->mutex);require(impl_->transport_options.direct_udp&&impl_->phase==Phase::Running&&impl_->host==impl_->self);
    transfer=impl_->authority;require(bool(transfer));}
  return transfer->publish(context,tick,std::move(archive),targets,transfer_time());
}
std::optional<ReceivedAuthorityCut> OnlineMatchSession::received_authority() {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  if(!transfer)return std::nullopt;
  return transfer->take_completed();
}
AuthorityCutSeal OnlineMatchSession::publish_authority_ahead(const AuthorityTransferContext &context,const AuthorityCutSeal &previous,
    std::uint64_t tick,std::vector<std::byte> bytes,unsigned target){
  std::shared_ptr<PeerAuthorityTransfer> transfer;
  {std::lock_guard lock(impl_->mutex);require(impl_->transport_options.direct_udp&&impl_->phase==Phase::Running&&impl_->host==impl_->self);
    transfer=impl_->authority;require(bool(transfer));}
  return transfer->publish_ahead(context,previous,tick,std::move(bytes),target,transfer_time());
}
bool OnlineMatchSession::acknowledge_authority(const AuthorityTransferContext &context,const AuthorityCutSeal &seal) {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return transfer&&transfer->acknowledge_validated(context,seal);
}
bool OnlineMatchSession::reject_authority(const AuthorityTransferContext &context,const AuthorityCutSeal &seal){
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  return transfer&&transfer->reject_delivered(context,seal);
}
std::vector<ValidatedAuthorityCut> OnlineMatchSession::validated_authority() {
  std::shared_ptr<PeerAuthorityTransfer> transfer;{std::lock_guard lock(impl_->mutex);transfer=impl_->authority;}
  if(!transfer)return {};
  return transfer->validated_cuts();
}
std::optional<PeerCommonCutState> OnlineMatchSession::common_cut()const{
  std::lock_guard lock(impl_->mutex);if(!impl_->common_supported())return {};
  return impl_->common_cut->state();
}
bool OnlineMatchSession::offer_common_cut(const PeerCommonCut &cut){return impl_->common_action(cut,&PeerCommonCutAgreement::offer);}
bool OnlineMatchSession::confirm_common_cut_native(const PeerCommonCut &cut){return impl_->common_action(cut,&PeerCommonCutAgreement::confirm_native);}
bool OnlineMatchSession::associate_common_cut(const PeerCommonCut &cut){return impl_->common_action(cut,&PeerCommonCutAgreement::associate);}
bool OnlineMatchSession::apply_common_cut(const PeerCommonCut &cut){return impl_->common_action(cut,&PeerCommonCutAgreement::apply);}
bool OnlineMatchSession::release_common_cut(const PeerCommonCut &cut){return impl_->common_action(cut,&PeerCommonCutAgreement::release);}
bool OnlineMatchSession::reject_common_cut(const PeerCommonCut &cut){return impl_->common_action(cut,&PeerCommonCutAgreement::reject);}
bool OnlineMatchSession::abort_common_cut(const PeerCommonCut &cut){return impl_->common_action(cut,&PeerCommonCutAgreement::abort);}
bool OnlineMatchSession::acknowledge_common_cut_abort(const PeerCommonCut &cut){return impl_->common_action(cut,&PeerCommonCutAgreement::acknowledge_abort,true);}
void OnlineMatchSession::seal_world(std::uint64_t tick,const std::string &hash) {
  std::lock_guard lock(impl_->mutex);require(impl_->phase==Phase::Running && impl_->host==impl_->self);
  impl_->queue_world("world_seal",{impl_->continuity_state.epoch,tick,hash});
}
void OnlineMatchSession::acknowledge_world(const OnlineWorldSeal &seal) {
  std::lock_guard lock(impl_->mutex);
  require((impl_->phase==Phase::Running && seal.epoch==impl_->continuity_state.epoch) ||
      (impl_->phase==Phase::AwaitingWorld && seal.epoch+std::uint64_t{1}==impl_->continuity_state.epoch));
  impl_->queue_world("world_ack",seal);
}
void OnlineMatchSession::accept_migration(std::uint64_t tick,const std::string &hash) {
  std::lock_guard lock(impl_->mutex);const auto &selected=impl_->continuity_state.selected;
  require(impl_->phase==Phase::Migrating && impl_->self==impl_->host && selected && selected->tick==tick && selected->digest==hash);
  impl_->queue_world("migration_accept",*selected);
}
void OnlineMatchSession::rebind_migration(std::uint64_t tick,const std::string &hash) {
  std::lock_guard lock(impl_->mutex);const auto &selected=impl_->continuity_state.selected;
  require(impl_->phase==Phase::Rebinding && impl_->self!=impl_->host && selected && selected->tick==tick && selected->digest==hash);
  impl_->queue_world("migration_rebind",*selected);
}
void OnlineMatchSession::loaded(std::uint64_t fingerprint,const UiVariant& variant) {
  // The cache/weapon fingerprint alone does not distinguish score limits,
  // clocks, life limits or the other authored variant settings.
  const auto source=source_game_variant(variant);std::vector<std::byte> admitted;
  for(unsigned i=0;i<8;++i)admitted.push_back(std::byte(fingerprint>>(8*i)));
  for(auto byte:source.bytes)admitted.push_back(std::byte(byte));
  for(const auto &[id,value]:variant.values)if(id>=native_slayer_ruleset_parameter)
    for(auto word:{id,unsigned(value)})for(unsigned i=0;i<4;++i)admitted.push_back(std::byte(word>>(8*i)));
  std::ostringstream value; value << digest(admitted).substr(0,16) << ':' << std::hex << std::setfill('0') << std::setw(7) << native_multiplayer_protocol;
  std::lock_guard lock(impl_->mutex); impl_->loaded = value.str();impl_->native_fingerprint=fingerprint;
}
std::optional<std::uint64_t> OnlineMatchSession::loaded_fingerprint()const{
  std::lock_guard lock(impl_->mutex);return impl_->native_fingerprint;
}
void OnlineMatchSession::send(std::span<const std::byte> packet, unsigned peer) {
  std::lock_guard lock(impl_->mutex);
  impl_->fence_peer_generation();
  if (impl_->phase != Phase::Running || peer >= impl_->roster.size() || packet.size() > 1400 || impl_->outgoing.size() >= 512) return;
  if(impl_->epoch_packets() && !(impl_->continuity_state.members&(1u<<peer)))return;
  impl_->outgoing.push_back({peer, {packet.begin(), packet.end()},impl_->continuity_state.epoch});
}
bool OnlineMatchSession::receive(std::vector<std::byte> &packet, unsigned &peer) {
  std::lock_guard lock(impl_->mutex);
  impl_->fence_peer_generation();
  if (impl_->incoming.empty()) return false;
  auto frame = std::move(impl_->incoming.front()); impl_->incoming.pop_front();
  peer = frame.peer; packet = std::move(frame.data); return true;
}
void OnlineMatchSession::voice(unsigned target,const GameVoiceFrame &frame) {
  if(frame.source>=16 || frame.opus.empty() || frame.opus.size()>256)return;
  auto packet=voice_header(0);put(packet,frame.source,1);put(packet,frame.sequence,4);put(packet,frame.radio,1);
  put(packet,std::bit_cast<unsigned>(frame.gains.left),4);put(packet,std::bit_cast<unsigned>(frame.gains.right),4);
  for(auto byte:frame.opus)packet.push_back(std::byte(byte));
  std::lock_guard lock(impl_->mutex);
  impl_->fence_peer_generation();
  impl_->sync_audio();
  std::optional<VoiceOrigin::Signature> signature;
  if(frame.source>=impl_->roster.size())return;
  const auto owner=online_admission_peer(impl_->roster[frame.source]);
  if(owner==impl_->self)signature=impl_->voice_origin.sign(packet,impl_->continuity_state.epoch);
  else if(frame.origin_signature&&impl_->voice_origin.verify(packet,*frame.origin_signature,owner,impl_->continuity_state.epoch))signature=frame.origin_signature;
  if(!signature)return;
  packet.insert(packet.end(),signature->begin(),signature->end());
  if(impl_->audio&&(frame.delivery_generation!=impl_->audio_generation||!impl_->audio->ready(impl_->audio_generation)))return;
  if((impl_->phase==Phase::Running || impl_->phase==Phase::Reviewing || impl_->phase==Phase::Postgame) &&
      target<impl_->roster.size() && (!impl_->epoch_packets()||(impl_->voice_members&(1u<<target)))&&impl_->outgoing.size()<512)impl_->outgoing.push_back({target,std::move(packet),impl_->continuity_state.epoch,impl_->audio_generation});
}
void OnlineMatchSession::voice_preferences(unsigned target,bool enabled,std::uint16_t muted) {
  auto packet=voice_header(1);put(packet,enabled,1);put(packet,muted,2);
  std::lock_guard lock(impl_->mutex);
  impl_->fence_peer_generation();
  if((impl_->phase==Phase::Running || impl_->phase==Phase::Reviewing || impl_->phase==Phase::Postgame) &&
      target<impl_->roster.size() && (!impl_->epoch_packets()||(impl_->voice_members&(1u<<target)))&&impl_->outgoing.size()<512)impl_->outgoing.push_back({target,std::move(packet),impl_->continuity_state.epoch,impl_->audio_generation});
}
std::uint16_t OnlineMatchSession::voice_blocks() const {std::lock_guard lock(impl_->mutex);return impl_->voice_blocked;}
std::vector<MatchVoicePacket> OnlineMatchSession::voice_packets() {
  std::lock_guard lock(impl_->mutex);impl_->fence_peer_generation();std::vector<MatchVoicePacket> packets(impl_->voices.begin(),impl_->voices.end());impl_->voices.clear();return packets;
}
void OnlineMatchSession::retain_voice(std::shared_ptr<GameVoice> audio) {
  std::lock_guard lock(impl_->mutex);require(!impl_->audio || impl_->audio==audio);
  if(impl_->audio==audio)return;
  impl_->audio=std::move(audio);impl_->fence_audio();impl_->sync_audio();
}
std::shared_ptr<GameVoice> OnlineMatchSession::retained_voice() const {
  std::lock_guard lock(impl_->mutex);return impl_->audio;
}
bool OnlineMatchSession::postgame_voice_enabled(unsigned slot) const {
  std::lock_guard lock(impl_->mutex);return slot<impl_->roster.size() && (impl_->voice_members&(1u<<slot)) && impl_->voice_enabled[online_admission_peer(impl_->roster[slot])];
}
void OnlineMatchSession::update_postgame_voice(bool focused) {
  impl_->voice_focused=focused;
}
void OnlineMatchSession::pump_postgame_voice() {
  const auto audio=retained_voice();if(!audio)return;
  const auto phase_now=phase();
  if(phase_now==Phase::Failed){audio->enable(false);return;}
  if(phase_now!=Phase::Reviewing && phase_now!=Phase::Postgame)return;
  audio->focus(impl_->voice_focused);audio->radio(false,true);
  audio->blocks(voice_blocks());
  const auto selected=audio->snapshot();const auto local=self(),leader=host();
  const bool hosting=local==leader;
  {std::lock_guard lock(impl_->mutex);impl_->voice_enabled[local]=selected.enabled;impl_->voice_mutes[local]=selected.muted;}
  const auto now=Clock::now();
  const double elapsed=std::chrono::duration<double>(now-impl_->voice_credit_at).count();impl_->voice_credit_at=now;
  for(auto &credit:impl_->voice_credits)credit=std::min(10.,credit+50*elapsed);
  const bool announce=now-impl_->voice_preferences_at>std::chrono::milliseconds(200);
  if(announce) {
    impl_->voice_preferences_at=now;
    if(!hosting)voice_preferences(leader,selected.enabled,selected.muted);
  }
  auto route=[&](GameVoiceFrame frame) {
    const auto source=frame.source;if(source>=roster().size() || !(impl_->voice_members&(1u<<source)))return;
    if(frame.sequence<=impl_->voice_sequences[source] || impl_->voice_credits[source]<1)return;
    impl_->voice_credits[source]-=1;
    impl_->voice_sequences[source]=frame.sequence;
    // Original passive mode2 uses session membership, without world distance
    // or team filtering. Both ends' actual mute/enable preferences still apply.
    for(const auto &p:roster())if(online_admission_peer(p)==p.slot && p.slot!=online_admission_peer(roster()[source]) && (impl_->voice_members&(1u<<p.slot)) && impl_->voice_enabled[source] && impl_->voice_enabled[p.slot] &&
        !(impl_->voice_mutes[source]&(1u<<p.slot)) && !(impl_->voice_mutes[p.slot]&(1u<<source))) {
      frame.gains={1,1};frame.radio=false;
      if(p.slot==local)audio->receive(frame);else voice(p.slot,frame);
    }
  };
  for(auto &packet:voice_packets()) {
    if(packet.preferences) {
      const auto source=hosting?packet.peer:packet.frame.source;
      if(source>=roster().size() || source==local || (!hosting && packet.peer!=leader))continue;
      std::lock_guard lock(impl_->mutex);impl_->voice_enabled[source]=packet.enabled;impl_->voice_mutes[source]=packet.muted;
    } else if(hosting) {route(std::move(packet.frame));}
    else if(packet.peer==leader && packet.frame.source<roster().size())audio->receive(std::move(packet.frame));
  }
  if(hosting && announce)for(const auto &p:roster())if(online_admission_peer(p)==p.slot && p.slot!=local && (impl_->voice_members&(1u<<p.slot)))for(const auto &member:roster()) {
    const auto peer=online_admission_peer(member);
    auto packet=voice_header(2);put(packet,impl_->voice_enabled[peer],1);
    put(packet,impl_->voice_mutes[peer],2);put(packet,member.slot,1);
    std::lock_guard lock(impl_->mutex);if(impl_->outgoing.size()<512)impl_->outgoing.push_back({p.slot,std::move(packet),impl_->continuity_state.epoch,impl_->audio_generation});
  }
  for(auto &frame:audio->outgoing()){frame.source=local;if(hosting)route(std::move(frame));else voice(leader,frame);}
}
void OnlineMatchSession::close_postgame() {
  if(const auto audio=retained_voice())audio->close();
  impl_->worker.request_stop();if(impl_->worker.joinable())impl_->worker.join();
}
void OnlineMatchSession::complete(std::span<const std::byte> report) {
  require(report.size()==0xdc78);
  std::string hex;hex.reserve(report.size()*2);constexpr char digits[]="0123456789abcdef";
  for(auto b:report){auto n=std::to_integer<unsigned>(b);hex+=digits[n>>4];hex+=digits[n&15];}
  std::lock_guard lock(impl_->mutex);require(impl_->self==impl_->host && impl_->phase==Phase::Running);
  if(impl_->completion.empty())impl_->completion=Json{{"postgame",hex}}.dump();
}
void OnlineMatchSession::confirm_result() {
  std::lock_guard lock(impl_->mutex);require(impl_->phase==Phase::Reviewing && impl_->report.size()==0xdc78);
  if(impl_->confirmation.empty())impl_->confirmation=Json{{"confirm_result",digest(impl_->report)}}.dump();
}
std::vector<std::byte> OnlineMatchSession::postgame() const {
  std::lock_guard lock(impl_->mutex);return impl_->report;
}
} // namespace halo2
