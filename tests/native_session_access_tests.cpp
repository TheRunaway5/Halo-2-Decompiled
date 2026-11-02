// Modern Session caller comparison. Devices, phase races, preferences and frames
// are declared fixtures. The preserved C++ bodies are exact source extractions;
// production Rust owns the new policy. No native-world/provider claim is made.
#include "halo2/online_match_session.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <mutex>
#include <stdexcept>
using Json=nlohmann::json;
namespace actual=halo2::online_protocol_bridge;
using namespace halo2;
namespace {
Json limit_json(const AuthorityTransferLimits &l){return {{"maximum",l.maximum_bytes},{"rate",l.bytes_per_second},{"burst",l.burst_bytes},{"retry",l.retry_ms},{"offer",l.offer_ms},{"idle",l.idle_ms},{"lifetime",l.lifetime_ms},{"window",l.window_chunks},{"features",l.features}};}
AuthorityTransferLimits native_limits(const actual::AuthorityTransferLimits &l){AuthorityTransferLimits r;r.maximum_bytes=l.maximum_bytes;r.bytes_per_second=l.bytes_per_second;r.burst_bytes=l.burst_bytes;r.retry_ms=l.retry_ms;r.offer_ms=l.offer_ms;r.idle_ms=l.idle_ms;r.lifetime_ms=l.lifetime_ms;r.window_chunks=l.window_chunks;r.features=l.features;return r;}
#include "reference/native_session_limits_before_rust_20261101.inc"
actual::NativeSessionConfig config(const Json &v){actual::NativeSessionConfig c;auto &o=c.origin;o.input_valid=true;o.origin="http://127.0.0.1:4555";o.bearer=std::string(48,'A');o.loopback=true;o.url_parsed=true;o.url_normalized="http://127.0.0.1:4555/";o.url_scheme="http";o.url_host="127.0.0.1";o.url_path="/";c.room="01234567-89ab-cdef-0123-456789abcdef";c.self_peer=v.value("self",0u);c.protocol=native_multiplayer_protocol;c.extension_floor=native_slayer_ruleset_parameter;c.team_present=true;c.team_mode=false;c.direct=true;c.continuity=v.value("continuity",false);c.peer_migration=!v.value("terminal",false)&&!c.continuity;c.snapshot_candidates=v.value("snapshot",false);c.survivor_selection=v.value("selection",false);c.terminal_recovery=v.value("terminal",false);const unsigned n=v.value("seats",8u),group=v.value("group",4u);for(unsigned i=0;i<n;++i){actual::NativeSessionSeat p;p.slot=i;p.peer=i-i%group;p.guest=i%group!=0;p.team=-1;p.name="seat"+std::to_string(i);for(auto &b:p.identity)b=i;if(v.value("legacy",false)&&p.peer==p.slot)p.peer=UINT32_MAX;c.seats.push_back(std::move(p));}return c;}
GameVoiceFrame frame(unsigned i){GameVoiceFrame f;f.source=i%2?4:0;f.sequence=i+1;f.radio=i%2;f.gains={0.5f,0.25f};f.delivery_generation=100+i;for(unsigned j=0;j<i+1;++j)f.opus.push_back((i*43+j*17)&255);if(i%2)f.origin_signature=std::array<std::byte,64>{std::byte{0x71}};return f;}
actual::NativeSessionAudio wire_audio(const GameVoiceFrame &f){actual::NativeSessionAudio r;r.source=f.source;r.sequence=f.sequence;r.radio=f.radio;r.left=f.gains.left;r.right=f.gains.right;r.generation=f.delivery_generation;for(auto b:f.opus)r.opus.push_back(b);if(f.origin_signature)for(auto b:*f.origin_signature)r.signature.push_back(std::to_integer<std::uint8_t>(b));return r;}
GameVoiceFrame native_audio(const actual::NativeSessionAudio &f){GameVoiceFrame r;r.source=f.source;r.sequence=f.sequence;r.radio=f.radio;r.gains={f.left,f.right};r.delivery_generation=f.generation;r.opus.assign(f.opus.begin(),f.opus.end());if(f.signature.size()==64){std::array<std::byte,64> a;for(unsigned i=0;i<64;++i)a[i]=std::byte(f.signature[i]);r.origin_signature=a;}return r;}
Json frame_json(const actual::NativeSessionAudio &f){Json opus=Json::array(),signature=Json::array();for(auto b:f.opus)opus.push_back(b);for(auto b:f.signature)signature.push_back(b);return {{"source",f.source},{"sequence",f.sequence},{"radio",f.radio},{"left",f.left},{"right",f.right},{"generation",f.generation},{"opus",opus},{"signature",signature}};}
}
namespace fixture {
class OnlineMatchSession;
void record(OnlineMatchSession &,const std::string &,Json=Json::object());
struct Mutex{OnlineMatchSession &s;bool held{};void lock();void unlock();};
struct Voice {
    OnlineMatchSession &s;void enable(bool);void focus(bool);void radio(bool,bool);void blocks(std::uint16_t);
    GameVoiceSnapshot snapshot()const;std::vector<GameVoiceFrame> outgoing();bool ready(std::uint64_t)const;void receive(GameVoiceFrame);
};
struct PeerSnapshotCandidate{};
struct Controller{OnlineMatchSession *s;};
namespace bridge {using NativeSessionAudio=actual::NativeSessionAudio;rust::Vec<NativeSessionAudio> native_session_postgame_voice(Controller &,std::uint64_t,bool,std::uint16_t,std::uint16_t,rust::Vec<NativeSessionAudio>,bool);}
std::uint64_t transfer_time(){return 77;}
class OnlineMatchSession {
public:
    using Phase=halo2::OnlineMatchSession::Phase;
    struct Impl {
        OnlineMatchSession &s;Mutex mutex{s};bool voice_focused{},postgame_presence_observed{},postgame_presence_closed{};
        Phase phase=Phase::Reviewing;std::uint64_t audio_generation=8;std::uint16_t voice_members=0xffff;
        unsigned self{};std::vector<OnlineAdmission> roster;OnlineContinuityState continuity_state;
        std::shared_ptr<PeerSnapshotCandidate> snapshot_candidate=std::make_shared<PeerSnapshotCandidate>();
        std::unique_ptr<Controller> controller=std::make_unique<Controller>(Controller{&s});
        bool common_phase()const;bool snapshot_supported()const;bool epoch_packets()const;void fence_peer_generation();
    };
    Json args,trace=Json::array();std::shared_ptr<Voice> audio;Impl impl;Impl *impl_=&impl;
    explicit OnlineMatchSession(const Json &v):args(v),impl{*this}{impl.phase=Phase(v.value("phase",3u));impl.voice_focused=v.value("focused",true);impl.postgame_presence_observed=v.value("observed",true);impl.postgame_presence_closed=v.value("closed",false);impl.voice_members=v.value("members",65535u);if(v.value("audio",true))audio=std::make_shared<Voice>(Voice{*this});if(!v.value("candidate",true))impl.snapshot_candidate.reset();}
    std::shared_ptr<Voice> retained_voice()const{std::lock_guard lock(impl_->mutex);record(impl_->s,"pin");return audio;}
    Phase phase()const{std::lock_guard lock(impl_->mutex);record(impl_->s,"phase");return impl_->phase;}
    std::uint16_t voice_blocks()const{std::lock_guard lock(impl_->mutex);record(impl_->s,"blocks_read");return args.value("blocked",0x8001u);}
    void pump_postgame_voice();std::optional<std::uint16_t> postgame_presence()const;
    std::shared_ptr<PeerSnapshotCandidate> snapshot_candidates()const;std::vector<unsigned> local_slots()const;
    std::optional<OnlineContinuityState> continuity()const;
};
void record(OnlineMatchSession &s,const std::string &event,Json data){s.trace.push_back({{"event",event},{"locked",s.impl.mutex.held},{"data",data}});if(s.args.value("fail",std::string{})==event)throw std::runtime_error("declared device/IO refusal");}
void Mutex::lock(){if(held)throw std::runtime_error("double lock");held=true;record(s,"lock");}void Mutex::unlock(){if(!held)throw std::runtime_error("unlock");record(s,"unlock");held=false;}
bool OnlineMatchSession::Impl::common_phase()const{return s.args.value("common",true);}bool OnlineMatchSession::Impl::snapshot_supported()const{record(s,"supported");return s.args.value("supported",true);}bool OnlineMatchSession::Impl::epoch_packets()const{return s.args.value("epochs",true);}
void OnlineMatchSession::Impl::fence_peer_generation(){record(s,"fence");audio_generation=s.args.value("generation",999ull);if(s.args.contains("after_phase"))phase=Phase(s.args.at("after_phase").get<unsigned>());}
void Voice::enable(bool b){record(s,"enable",b);}void Voice::focus(bool b){record(s,"focus",b);}void Voice::radio(bool a,bool b){record(s,"radio",Json::array({a,b}));}void Voice::blocks(std::uint16_t b){record(s,"blocks",b);}
GameVoiceSnapshot Voice::snapshot()const{record(s,"preferences");GameVoiceSnapshot r;r.enabled=s.args.value("enabled",true);r.muted=s.args.value("muted",2u);r.blocked=s.args.value("blocked",0x8001u);return r;}
std::vector<GameVoiceFrame> Voice::outgoing(){record(s,"outgoing");std::vector<GameVoiceFrame> out;for(unsigned i=0;i<s.args.value("outgoing",3u);++i)out.push_back(frame(i));return out;}
bool Voice::ready(std::uint64_t g)const{record(s,"ready",g);return s.args.value("ready",true);}void Voice::receive(GameVoiceFrame f){record(s,"receive",frame_json(wire_audio(f)));}
namespace bridge {rust::Vec<NativeSessionAudio> native_session_postgame_voice(Controller &c,std::uint64_t now,bool enabled,std::uint16_t muted,std::uint16_t blocked,rust::Vec<NativeSessionAudio> outgoing,bool ready){auto &s=*c.s;Json frames=Json::array();for(const auto &f:outgoing)frames.push_back(frame_json(f));record(s,"exchange",{{"now",now},{"enabled",enabled},{"muted",muted},{"blocked",blocked},{"ready",ready},{"frames",frames}});rust::Vec<NativeSessionAudio> out;if(s.impl.phase==OnlineMatchSession::Phase::Reviewing||s.impl.phase==OnlineMatchSession::Phase::Postgame)for(unsigned i=0;i<s.args.value("incoming",2u);++i)out.push_back(wire_audio(frame(i+4)));return out;}}
#include "reference/native_session_postgame_before_rust_20261101.inc"
#include "reference/native_session_views_before_rust_20261101.inc"
}
class PostgameIo final:public actual::NativePostgameIo {
    fixture::OnlineMatchSession &s;mutable std::shared_ptr<fixture::Voice> audio;
public:
    explicit PostgameIo(fixture::OnlineMatchSession &v):s(v){}~PostgameIo(){if(s.impl.mutex.held)s.impl.mutex.unlock();}
    bool postgame_pin_audio()const override{audio=s.retained_voice();return bool(audio);}std::uint32_t postgame_phase()const override{return std::uint32_t(s.phase());}
    void postgame_enable(bool b)const override{audio->enable(b);}bool postgame_focused()const override{return s.impl.voice_focused;}
    void postgame_focus(bool b)const override{audio->focus(b);}void postgame_radio(bool a,bool b)const override{audio->radio(a,b);}
    std::uint16_t postgame_blocks()const override{return s.voice_blocks();}void postgame_set_blocks(std::uint16_t b)const override{audio->blocks(b);}
    actual::NativePostgamePreferences postgame_preferences()const override{const auto p=audio->snapshot();actual::NativePostgamePreferences r;r.enabled=p.enabled;r.muted=p.muted;r.blocked=p.blocked;return r;}
    rust::Vec<actual::NativeSessionAudio> postgame_outgoing()const override{rust::Vec<actual::NativeSessionAudio> out;for(const auto &f:audio->outgoing())out.push_back(wire_audio(f));return out;}
    void postgame_lock()const override{s.impl.mutex.lock();}void postgame_unlock()const override{s.impl.mutex.unlock();}void postgame_fence()const override{s.impl.fence_peer_generation();}
    rust::Vec<actual::NativeSessionAudio> postgame_exchange(const actual::NativePostgamePreferences &p,rust::Vec<actual::NativeSessionAudio> frames)const override{return fixture::bridge::native_session_postgame_voice(*s.impl.controller,fixture::transfer_time(),p.enabled,p.muted,p.blocked,std::move(frames),audio->ready(s.impl.audio_generation));}
    void postgame_deliver(rust::Vec<actual::NativeSessionAudio> frames)const override{for(const auto &f:frames)audio->receive(native_audio(f));}
};
class SnapshotIo final:public actual::NativeSnapshotViewIo{fixture::OnlineMatchSession &s;public:mutable std::shared_ptr<fixture::PeerSnapshotCandidate> selected;explicit SnapshotIo(fixture::OnlineMatchSession &v):s(v){}bool snapshot_view_common()const override{return s.impl.common_phase();}bool snapshot_view_supported()const override{return s.impl.snapshot_supported();}void snapshot_view_select()const override{selected=s.impl.snapshot_candidate;}};
Json pump(fixture::OnlineMatchSession &s,bool rust){bool error{};try{if(rust){PostgameIo io(s);actual::native_session_pump_postgame(io);}else s.pump_postgame_voice();}catch(const std::exception &){error=true;}return {{"error",error},{"held",s.impl.mutex.held},{"trace",s.trace}};}
int main(){std::string line;while(std::getline(std::cin,line)){try{const auto v=Json::parse(line);const auto op=v.value("op",std::string("pump"));Json a,b;if(op=="pump"){fixture::OnlineMatchSession x(v),y(v);a=pump(x,false);b=pump(y,true);}else if(op=="snapshot"){fixture::OnlineMatchSession x(v),y(v);const auto old=x.snapshot_candidates();std::shared_ptr<fixture::PeerSnapshotCandidate> next;{std::lock_guard lock(y.impl.mutex);SnapshotIo io(y);actual::native_session_snapshot_view(io);next=std::move(io.selected);}a={{"selected",bool(old)},{"trace",x.trace}};b={{"selected",bool(next)},{"trace",y.trace}};}else if(op=="limits"){auto c=config(v);auto s=actual::new_native_session(c);a=limit_json(source_transfer_limits(c.snapshot_candidates,c.survivor_selection,c.terminal_recovery));b=limit_json(native_limits(actual::native_session_authority_limits(*s)));}else if(op=="seats"){auto c=config(v);auto s=actual::new_native_session(c);fixture::OnlineMatchSession old(v);old.impl.self=c.self_peer;for(const auto &p:c.seats){OnlineAdmission row;row.slot=p.slot;row.peer=p.peer;row.guest=p.guest;old.impl.roster.push_back(row);}auto local=old.local_slots();std::vector<unsigned> physical;for(const auto &p:old.impl.roster)if(online_admission_peer(p)==p.slot)physical.push_back(p.slot);std::vector<unsigned> next_local,next_physical;for(auto p:actual::native_session_local_slots(*s))next_local.push_back(p);for(auto p:actual::native_session_physical_peers(*s))next_physical.push_back(p);a={{"local",local},{"physical",physical}};b={{"local",next_local},{"physical",next_physical}};}else if(op=="metadata"){
 auto c=config(v);auto state=actual::new_native_session(c);auto send=[&](const Json &x){const auto text=x.dump();actual::NativeSessionResources resources;actual::native_session_text(*state,{reinterpret_cast<const std::uint8_t*>(text.data()),text.size()},resources);};
 const auto stage=v.value("stage",0u);if(stage){const unsigned host=(unsigned(c.seats.size())-1)/v.value("group",4u)*v.value("group",4u);Json start={{"phase","running"},{"host",host}};if(c.continuity)start["continuity"]={{"phase","running"},{"host",host},{"epoch",0},{"members",(1u<<c.seats.size())-1}};send(start);send({{"postgame",std::string(0xdc78*2,'0')}});send({{"voice_host",host},{"voice_members",(1u<<c.seats.size())-1}});if(stage==2)actual::native_session_close_postgame(*state);}
 const auto view=actual::native_session_view(*state);fixture::OnlineMatchSession old(v);old.impl.phase=fixture::OnlineMatchSession::Phase(view.phase);old.impl.postgame_presence_observed=view.presence_present;old.impl.postgame_presence_closed=!view.presence_present;old.impl.voice_members=view.voice_members;old.impl.continuity_state={view.epoch,view.continuity_host,view.members,std::nullopt};if(view.selected.present)old.impl.continuity_state.selected=OnlineWorldSeal{view.selected.epoch,view.selected.tick,std::string(view.selected.digest)};old.args["epochs"]=c.continuity||c.peer_migration;
 const auto presence=old.postgame_presence();const auto next_presence=actual::native_session_presence(*state);const auto continuity=old.continuity();const auto next=actual::native_session_continuity_view(*state);
 a={{"presence",presence?Json(*presence):Json(nullptr)},{"continuity",continuity?Json{{"epoch",continuity->epoch},{"host",continuity->host},{"members",continuity->members}}:Json(nullptr)}};
 b={{"presence",next_presence.present?Json(next_presence.members):Json(nullptr)},{"continuity",next.present?Json{{"epoch",next.epoch},{"host",next.host},{"members",next.members}}:Json(nullptr)}};
 }else if(op=="control"){const bool direct=v.value("direct",false);a={{"pump",!direct}};b={{"pump",actual::native_session_control_postgame(direct)}};}
 else throw std::invalid_argument("op");Json out={{"matched",a==b},{"result",a}};if(a!=b)out["rust"]=b;std::cout<<out.dump()<<std::endl;}catch(const std::exception &e){std::cout<<Json{{"fixture_error",e.what()}}.dump()<<std::endl;}}}
