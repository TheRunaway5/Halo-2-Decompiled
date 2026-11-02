// Exact modern C++ Session callers versus Rust orchestration, both invoking
// the production Rust Session owner. Resource observations and IO refusals are
// declared fixtures. This does not certify original Live or a native world.
#include "halo2/online_match_session.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>
using Json=nlohmann::json;
using namespace halo2;
namespace actual=halo2::online_protocol_bridge;
namespace fixture {
struct Caller;
struct Controller {rust::Box<actual::NativeSession> state;Caller &owner;};
namespace bridge {
using NativeSessionFence=actual::NativeSessionFence;
using NativeSessionCandidate=actual::NativeSessionCandidate;
actual::NativeSessionEffects native_session_fence(Controller &,const NativeSessionFence &);
void native_session_selection(Controller &,const actual::HostAgreementContext &);
std::uint32_t native_session_candidate(const Controller &,const NativeSessionCandidate &);
}
actual::HostAgreementContext wire_context(const AuthorityTransferContext &c){actual::HostAgreementContext r;for(unsigned i=0;i<32;++i)r.session[i]=std::to_integer<std::uint8_t>(c.session[i]);r.epoch=c.epoch;r.membership_revision=c.membership_revision;r.peers=c.peers;r.host=c.host;return r;}
AuthorityTransferContext native_context(const actual::HostAgreementContext &c){AuthorityTransferContext r;for(unsigned i=0;i<32;++i)r.session[i]=std::byte(c.session[i]);r.epoch=c.epoch;r.membership_revision=c.membership_revision;r.peers=c.peers;r.host=c.host;return r;}
actual::NativePublicationContext raw_context(const AuthorityTransferContext &c){actual::NativePublicationContext r;for(unsigned i=0;i<32;++i)r.session[i]=std::to_integer<std::uint8_t>(c.session[i]);r.epoch=c.epoch;r.membership_revision=c.membership_revision;r.peers=c.peers;r.host=c.host;return r;}
actual::MigrationPreparationCut wire_cut(const AuthorityCutSeal &c){actual::MigrationPreparationCut r;r.present=true;r.id=c.id;r.tick=c.tick;r.bytes=c.bytes;for(unsigned i=0;i<32;++i)r.digest[i]=std::to_integer<std::uint8_t>(c.digest[i]);return r;}
actual::HostAgreementDecision wire_decision(const std::optional<PeerMigrationDecision> &d){actual::HostAgreementDecision r;r.present=d.has_value();if(!d)return r;r.previous=wire_context(d->previous);r.next=wire_context(d->next);r.ballot.round=d->ballot.round;r.ballot.proposer=d->ballot.proposer;r.value.host=d->value.host;r.value.cut=wire_cut(d->value.cut);r.voters=d->voters;return r;}
actual::HostAgreementState wire_agreement(const PeerHostAgreementState &s){actual::HostAgreementState r;r.suspected=s.suspected;r.fenced=s.fenced;r.electorate=s.electorate;r.silent=s.silent;r.majority=s.majority;r.rejected=s.rejected;for(unsigned i=0;i<16;++i)if(s.ready[i])r.ready[i]=wire_cut(*s.ready[i]);r.decision=wire_decision(s.decision);return r;}
Json context_json(const actual::HostAgreementContext &c){return {{"session",c.session},{"epoch",c.epoch},{"revision",c.membership_revision},{"peers",c.peers},{"host",c.host}};}
Json cut_json(const actual::MigrationPreparationCut &c){return c.present?Json{{"id",c.id},{"tick",c.tick},{"bytes",c.bytes},{"digest",c.digest}}:Json(nullptr);}
Json decision_json(const actual::HostAgreementDecision &d){return d.present?Json{{"previous",context_json(d.previous)},{"next",context_json(d.next)},{"round",d.ballot.round},{"proposer",d.ballot.proposer},{"host",d.value.host},{"cut",cut_json(d.value.cut)},{"voters",d.voters}}:Json(nullptr);}
Json agreement_json(const actual::HostAgreementState &s){Json ready=Json::array();for(const auto &c:s.ready)ready.push_back(cut_json(c));return {{"suspected",s.suspected},{"fenced",s.fenced},{"electorate",s.electorate},{"silent",s.silent},{"majority",s.majority},{"rejected",s.rejected},{"ready",ready},{"decision",decision_json(s.decision)}};}
actual::NativeSessionConfig config(const Json &v){actual::NativeSessionConfig c;auto &o=c.origin;o.input_valid=true;o.origin="http://127.0.0.1:4555";o.bearer=std::string(48,'A');o.loopback=true;o.url_parsed=true;o.url_normalized="http://127.0.0.1:4555/";o.url_scheme="http";o.url_host="127.0.0.1";o.url_path="/";c.room="01234567-89ab-cdef-0123-456789abcdef";c.self_peer=v.value("self",1u);c.protocol=native_multiplayer_protocol;c.extension_floor=native_slayer_ruleset_parameter;c.team_present=true;c.team_mode=false;c.direct=true;c.peer_migration=true;c.survivor_selection=v.value("feature",false);for(unsigned i=0;i<v.value("nodes",4u);++i){actual::NativeSessionSeat p;p.slot=p.peer=i;p.team=-1;p.name="seat"+std::to_string(i);for(auto &b:p.identity)b=i;c.seats.push_back(std::move(p));}return c;}
struct Agreement {Caller &s;PeerHostAgreementState state(std::uint64_t)const;};
struct Quality {Caller &s;AuthorityTransferContext context()const;struct Row{unsigned peer;};std::vector<Row> migration_candidates(std::uint64_t)const;};
struct Selection {Caller &s;struct State{bool selection_complete,failed;};State state()const;};
std::uint64_t transfer_time(){return 0xfedcba9876543210ull;}
struct Caller {
    Json args,trace=Json::array();std::unique_ptr<Controller> controller;
    actual::NativeSessionView projection;
    std::optional<AuthorityTransferContext> agreement_context;AuthorityTransferContext base,quality_context;
    std::unique_ptr<Agreement> agreement;std::unique_ptr<Quality> mesh_quality;std::unique_ptr<Selection> survivor_selection;
    bool selection_started{};PeerHostAgreementState learned;
    explicit Caller(const Json &v):args(v){
        auto c=config(v);controller=std::make_unique<Controller>(Controller{actual::new_native_session(c),*this});projection=actual::native_session_view(*controller->state);
        base.session={};for(unsigned i=0;i<32;++i)base.session[i]=std::byte(projection.session[i]);base.peers=std::uint16_t((1u<<c.seats.size())-1);base.host=v.value("host",0u);
        const auto phase=v.value("phase",std::string("running"));
        if(phase!="connecting"){
            const auto text=Json{{"phase",phase=="loading"?"loading":"running"},{"host",base.host}}.dump();actual::NativeSessionResources r;
            const auto e=actual::native_session_text(*controller->state,{reinterpret_cast<const std::uint8_t*>(text.data()),text.size()},r);if(!e.initialize)throw std::runtime_error("fixture initialize");base=native_context(e.context);
        }
        if(phase=="failed")(void)actual::native_session_fail(*controller->state,"declared failed phase");
        if(v.value("started",false)&&c.survivor_selection&&phase!="connecting")actual::native_session_selection(*controller->state,wire_context(base));
        projection=actual::native_session_view(*controller->state);selection_started=projection.selection_started;
        agreement_context=base;quality_context=base;
        auto change=[&](AuthorityTransferContext &ctx,const Json &mods){if(mods.contains("host"))ctx.host=mods["host"].get<unsigned>();if(mods.contains("epoch"))ctx.epoch=mods["epoch"].get<std::uint32_t>();if(mods.contains("revision"))ctx.membership_revision=mods["revision"].get<std::uint64_t>();if(mods.contains("peers"))ctx.peers=mods["peers"].get<std::uint16_t>();if(mods.value("session",false))ctx.session[31]^=std::byte{1};};
        if(v.contains("binding_change"))change(*agreement_context,v["binding_change"]);if(v.contains("quality_change"))change(quality_context,v["quality_change"]);
        if(!v.value("binding",true))agreement_context.reset();if(v.value("agreement",true))agreement=std::make_unique<Agreement>(Agreement{*this});if(v.value("quality",true))mesh_quality=std::make_unique<Quality>(Quality{*this});if(v.value("selection",true))survivor_selection=std::make_unique<Selection>(Selection{*this});
        learned.suspected=v.value("suspected",true);learned.fenced=v.value("fenced",false);learned.electorate=v.value("electorate",unsigned(base.peers&~(1u<<base.host)));learned.silent=v.value("silent",0u);learned.majority=v.value("majority",99u);learned.rejected=v.value("rejected",UINT64_MAX);
        const unsigned ready=v.value("ready",unsigned(base.peers));for(unsigned p=0;p<16;++p)if(ready&(1u<<p)){AuthorityCutSeal cut;cut.id=100+p;cut.tick=UINT64_MAX-p;cut.bytes=1000+p;cut.digest[0]=std::byte(p+1);learned.ready[p]=cut;}
        const auto decision=v.value("decision",std::string("none"));if(decision!="none"){
            PeerMigrationDecision d;d.previous=base;d.next=base;++d.next.epoch;++d.next.membership_revision;d.next.peers=base.peers&~(1u<<base.host);d.next.host=v.value("chosen",base.host==0?1u:0u);d.ballot={1u,d.next.host};d.value.host=d.next.host;d.value.cut.id=901;d.value.cut.tick=UINT64_MAX;d.value.cut.bytes=1024;d.value.cut.digest[0]=std::byte{0xa5};d.voters=d.next.peers;
            if(decision=="bad_epoch")++d.next.epoch;if(decision=="bad_revision")++d.next.membership_revision;if(decision=="bad_bytes")d.value.cut.bytes=0;if(decision=="bad_digest")d.value.cut.digest={};if(decision=="bad_voters")d.voters=0;if(decision=="bad_round")d.ballot.round=0;if(decision=="bad_previous")d.previous.epoch=1;
            learned.decision=d;
        }
    }
    void record(const std::string &event,Json data=Json::object())const{auto &s=const_cast<Caller&>(*this);s.trace.push_back({{"event",event},{"data",data}});if(args.value("fail",std::string{})==event)throw std::runtime_error("declared IO refusal");}
    void refresh(){record("refresh");projection=actual::native_session_view(*controller->state);selection_started=projection.selection_started;}
    void apply(const actual::NativeSessionEffects &e){record("apply",{{"audio_fence",e.audio_fence},{"audio_stop",e.audio_stop},{"initialize",e.initialize}});refresh();if(e.audio_fence){record("audio_fence");actual::native_session_fence_audio(*controller->state,projection.audio_generation+1);refresh();}}
    bool selection_supported()const{record("supported");return args.value("supported",true);}
#include "reference/native_migration_callers_before_rust_20261101.inc"
};
PeerHostAgreementState Agreement::state(std::uint64_t now)const{s.record("agreement",now);return s.learned;}
AuthorityTransferContext Quality::context()const{s.record("quality_context");return s.quality_context;}
std::vector<Quality::Row> Quality::migration_candidates(std::uint64_t now)const{s.record("ranked",now);std::vector<Row> out;for(auto p:s.args.value("ranked",std::vector<unsigned>{3,2,1,0}))out.push_back({p});return out;}
Selection::State Selection::state()const{s.record("selection");return {s.args.value("complete",true),s.args.value("selection_failed",false)};}
namespace bridge {
actual::NativeSessionEffects native_session_fence(Controller &c,const NativeSessionFence &v){c.owner.record("fence",{{"context",context_json(v.context)},{"fenced",v.fenced},{"decision",decision_json(v.decision)},{"started",v.selection_started},{"failed",v.selection_failed}});return actual::native_session_fence(*c.state,v);}
void native_session_selection(Controller &c,const actual::HostAgreementContext &v){c.owner.record("begin",context_json(v));actual::native_session_selection(*c.state,v);}
std::uint32_t native_session_candidate(const Controller &c,const NativeSessionCandidate &v){Json ranked=Json::array();for(auto p:v.ranked)ranked.push_back(p);c.owner.record("pick",{{"agreement",v.agreement_present?agreement_json(v.agreement):Json(nullptr)},{"context",v.agreement_present?context_json(v.context):Json(nullptr)},{"quality",v.quality_present?context_json(v.quality):Json(nullptr)},{"ranked",ranked},{"supported",v.selection_supported},{"complete",v.selection_complete},{"failed",v.selection_failed}});return actual::native_session_candidate(*c.state,v);}
}
}
class MigrationIo final:public actual::NativeMigrationIo {
    fixture::Caller &s;
public:
    explicit MigrationIo(fixture::Caller &v):s(v){}
    actual::NativeMigrationBinding migration_binding()const override{actual::NativeMigrationBinding r;r.present=s.agreement_context.has_value();if(s.agreement_context)r.context=fixture::raw_context(*s.agreement_context);return r;}
    bool migration_agreement_present()const override{return bool(s.agreement);}
    std::uint64_t migration_now_ms()const override{return fixture::transfer_time();}
    actual::HostAgreementState migration_agreement(std::uint64_t now)const override{return fixture::wire_agreement(s.agreement->state(now));}
    bool migration_handoff()const override{return s.projection.peer_handoff_phase;}
    bool migration_selection_started()const override{return s.selection_started;}
    actual::NativeMigrationBinding migration_quality()const override{actual::NativeMigrationBinding r;r.present=bool(s.mesh_quality);if(s.mesh_quality)r.context=fixture::raw_context(s.mesh_quality->context());return r;}
    rust::Vec<std::uint32_t> migration_ranked(std::uint64_t now)const override{rust::Vec<std::uint32_t> r;for(const auto &q:s.mesh_quality->migration_candidates(now))r.push_back(q.peer);return r;}
    bool migration_selection_supported()const override{return s.selection_supported();}
    actual::NativeMigrationSelection migration_selection()const override{actual::NativeMigrationSelection r;r.present=bool(s.survivor_selection);if(s.survivor_selection){const auto v=s.survivor_selection->state();r.complete=v.selection_complete;r.failed=v.failed;}return r;}
    void migration_apply_fence(const actual::NativeSessionFence &v)const override{s.apply(fixture::bridge::native_session_fence(*s.controller,v));}
    void migration_begin(const actual::HostAgreementContext &v)const override{fixture::bridge::native_session_selection(*s.controller,v);s.refresh();}
    std::uint32_t migration_pick(const actual::NativeSessionCandidate &v)const override{return fixture::bridge::native_session_candidate(*s.controller,v);}
};
Json run(fixture::Caller &s,bool rust){Json results=Json::array();for(const auto &command:s.args.value("commands",Json::array({Json{{"op",s.args.value("op",std::string("candidate"))}}}))){const auto op=command.value("op",std::string("candidate"));bool error{};std::string exception;Json selected=nullptr;try{MigrationIo io(s);if(op=="fence"){const bool force=command.value("force",s.args.value("force",false));const bool failed=command.value("failed",s.args.value("failed",false));if(rust)actual::native_migration_fence(io,force,failed);else s.native_fence(force,failed);}else if(op=="terminal"){if(rust)actual::native_migration_fence(io,true,false);else s.preparation_terminal();}else if(op=="begin"){if(rust)actual::native_migration_begin(io);else s.begin_selection();}else if(op=="candidate"){const auto now=command.value("now",s.args.value("now",UINT64_MAX));if(rust){const auto p=actual::native_migration_candidate(io,now);if(p.present)selected=p.peer;}else {const auto p=s.migration_candidate(now);if(p)selected=*p;}}else throw std::runtime_error("fixture op");}catch(const std::exception &e){error=true;exception=e.what();}const auto v=actual::native_session_view(*s.controller->state);results.push_back({{"error",error},{"exception",exception},{"selected",selected},{"phase",v.phase},{"host",v.host},{"epoch",v.epoch},{"members",v.members},{"fenced",v.peer_fenced},{"started",v.selection_started},{"generation",v.audio_generation},{"message",std::string(v.error)}});}return {{"steps",results},{"trace",s.trace}};}
int main(){std::string line;while(std::getline(std::cin,line)){try{const auto v=Json::parse(line);fixture::Caller before(v),after(v);const auto a=run(before,false),b=run(after,true);Json result={{"matched",a==b},{"result",a}};if(a!=b)result["rust"]=b;std::cout<<result.dump()<<std::endl;}catch(const std::exception &e){std::cout<<Json{{"fixture_error",e.what()}}.dump()<<std::endl;}}}
