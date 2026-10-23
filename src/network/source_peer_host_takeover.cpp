#include "halo2/source_peer_host_takeover.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/original_data.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>

namespace halo2 {
namespace {
void need(bool value){if(!value)throw FormatError("Invalid native peer host takeover ownership, decision or admission");}
bool aborted(const OnlineMatchSession &session,const AuthorityTransferContext &context,const AuthorityCutSeal &seal){
    const auto common=session.common_cut();
    return common&&common->abort&&common->pending&&common->pending->context==context&&common->pending->seal==seal;
}
bool current(const OnlineMatchSession &session,const PeerMigrationDecision &decision,const AcceptedSourcePlayers &admissions){
    const auto agreement=session.peer_migration();
    return session.phase()==OnlineMatchSession::Phase::Migrating&&session.self()==decision.value.host&&
        session.authority_context()==decision.previous&&session.source_players()==admissions&&
        agreement&&agreement->decision&&*agreement->decision==decision&&!aborted(session,decision.previous,decision.value.cut);
}
bool current_import(const OnlineMatchSession &session,const ValidatedAuthorityCut &cut,const AcceptedSourcePlayers &admissions){
    const auto phase=session.phase();
    return (phase==OnlineMatchSession::Phase::Running||phase==OnlineMatchSession::Phase::AwaitingWorld||phase==OnlineMatchSession::Phase::Migrating)&&
        session.self()==cut.peer&&session.authority_context()==cut.context&&session.source_players()==admissions&&!aborted(session,cut.context,cut.seal);
}
}
struct SourcePeerHostTakeover::ProspectiveNative {
    PeerProspectiveHostRole role;
    std::uint64_t fingerprint{};
    std::shared_ptr<MultiplayerPreparedWorld> world;
    std::shared_ptr<const Cache> cache;
    const PhysicsLiveWorldTu15 *native{};
    std::unique_ptr<LanHost> host;
    LanHost::LocalContinuation local;
    LanHost::Continuation input_cut;
    std::unique_ptr<NativeVehicleCamera> camera;
    std::unique_ptr<SourceMatchAuthority> authority;
    SourceMatchAuthority::ExecutionBoundary boundary;
    std::optional<MccSoundVariationProvider> audio_provider;
    std::shared_ptr<SourceNativePublication::Owner> publication_owner;
    // No timing/SDL scope is retained here. The real native scope remains
    // eligible across the private wait and moves unchanged at certification.
    std::unique_ptr<SourceNativePublication> publication;
};
struct SourcePeerHostTakeover::NativeImport {
    const std::thread::id executor=std::this_thread::get_id();
    std::shared_ptr<OnlineMatchSession> session;
    ValidatedAuthorityCut cut;
    AcceptedSourcePlayers admissions;
    std::vector<unsigned> local_views;
    LanHost::Continuation saved_host;
    std::unique_ptr<MultiplayerPreparedWorld> world;
    SourceMatchAuthority::ExecutionBoundary boundary;
    std::unique_ptr<ProspectiveNative> prospective;
    ~NativeImport(){
        if(executor!=std::this_thread::get_id())std::terminate();
        SourcePeerHostTakeover::close_prospective(*this);
    }
};
std::unique_ptr<SourcePeerHostTakeover> SourcePeerHostTakeover::prepare(const MultiplayerOptions &input,
    const SourceAuthorityRetainedWorld &retained,const PeerMigrationDecision &decision,
    std::shared_ptr<PhysicsLiveClockSource> candidate_clock) {
    return prepare_impl(input,retained,decision,std::move(candidate_clock),nullptr);
}
std::unique_ptr<SourcePeerHostTakeover> SourcePeerHostTakeover::prepare_with_resident_cache(
    const MultiplayerOptions &input,const SourceAuthorityRetainedWorld &retained,
    const PeerMigrationDecision &decision,const MultiplayerPreparedWorld &resident,
    std::shared_ptr<PhysicsLiveClockSource> candidate_clock) {
    return prepare_impl(input,retained,decision,std::move(candidate_clock),&resident);
}
std::unique_ptr<SourcePeerHostTakeover> SourcePeerHostTakeover::prepare_impl(const MultiplayerOptions &input,
    const SourceAuthorityRetainedWorld &retained,const PeerMigrationDecision &decision,
    std::shared_ptr<PhysicsLiveClockSource> candidate_clock,const MultiplayerPreparedWorld *resident) {
    need(input.online_match&&current(*input.online_match,decision,retained.admissions)&&
        retained.cut.context==decision.previous&&retained.cut.seal==decision.value.cut);
    auto imported=import_native(input,retained,std::move(candidate_clock),resident);
    return prepare_imported(imported,decision);
}
std::shared_ptr<SourcePeerHostTakeover::NativeImport> SourcePeerHostTakeover::import_native(const MultiplayerOptions &input,
    const SourceAuthorityRetainedWorld &retained,std::shared_ptr<PhysicsLiveClockSource> candidate_clock,
    const MultiplayerPreparedWorld *resident) {
    if(resident)(void)resident->original_cache();
    const auto session=input.online_match;
    need(session&&retained.host&&current_import(*session,retained.cut,retained.admissions)&&
        retained.admissions.context==retained.cut.context&&retained.admissions.players.size()==session->roster().size()&&
        retained.local_views==session->local_slots()&&retained.local_views.size()==input.local_players&&
        retained.checkpoint.match.simulation.ticks==retained.cut.seal.tick);
    std::vector<unsigned> producer_views;
    for(const auto &p:session->roster())if(online_admission_peer(p)==retained.cut.context.host)producer_views.push_back(p.slot);
    need(retained.producer_views==producer_views);
    const auto options=source_authority_options(input);
    auto target=resident?MultiplayerPreparedWorld::incoming_source_shadow(options,*resident,std::move(candidate_clock)):
        MultiplayerPreparedWorld::source_shadow(options,MultiplayerPreparedWorld::SourceShadowViews::IncomingHost,std::move(candidate_clock));
    need(target->local_slots==producer_views);
    SourceMatchAuthority authority(target->match,*target->source_objects);
    for(auto slot:retained.local_views){
        const auto found=std::ranges::find(retained.admissions.players,slot,&SourcePlayerAdmission::slot);
        const auto &player=retained.checkpoint.match.players.at(slot);
        need(found!=retained.admissions.players.end()&&player.connected&&!(player.source_flags&2u)&&
            player.source_datum!=null_datum&&player.source_datum==found->player);
        if(!options.selected_profiles.empty())need(retained.checkpoint.match.players.at(slot).source_configuration==target->match.player(slot).source_configuration);
    }
    // This whole owner is disposable until activation. Its initial producer
    // view bank validates the exact cut; subsequent local binding runs against
    // that imported native state, not a newly born recipient prototype. A
    // second transactional workspace is unnecessary for this private import.
    auto prepared=authority.prepare(retained.checkpoint,source_authority_binding(retained.cut.context),retained.admissions.players);
    need(authority.commit(std::move(prepared)));
    target->bind_source_local_views(retained.local_views);
    need(current_import(*session,retained.cut,retained.admissions)&&target->local_slots==session->local_slots()&&
        authority.execution_tick()==retained.cut.seal.tick);
    auto imported=std::make_shared<NativeImport>();
    imported->session=session;imported->cut=retained.cut;imported->admissions=retained.admissions;
    imported->local_views=retained.local_views;imported->saved_host=*retained.host;
    imported->boundary=authority.execution_boundary();imported->world=std::move(target);
    return imported;
}
const ValidatedAuthorityCut &SourcePeerHostTakeover::imported_cut(const NativeImport &imported){
    need(imported.executor==std::this_thread::get_id()&&
        (imported.world||(imported.prospective&&imported.prospective->world)));return imported.cut;
}
void SourcePeerHostTakeover::close_prospective(NativeImport &imported)noexcept{
    if(imported.prospective)SourceNativePublication::close_owner(imported.prospective->publication_owner);
}
void SourcePeerHostTakeover::prepare_role_mutations(ProspectiveNative &p,
    const std::shared_ptr<OnlineMatchSession> &session,const AcceptedSourcePlayers &admissions,SourceMatchAuthority &authority){
    auto &world=*p.world;need(authority.execution_tick()==p.role.cut.tick);
    auto resumed=LanHost::resume_after_host_loss(world.match,session,p.fingerprint,session->roster(),p.input_cut,
        p.role.previous,p.role.next,admissions.players);
    p.host=std::move(resumed.host);p.local=std::move(resumed.local);
    need(p.local.slots==world.local_slots&&p.local.camera.has_value());
    p.camera=std::make_unique<NativeVehicleCamera>(*world.source_objects,*world.cache->original_data());
    auto camera=p.camera->prepare(*p.local.camera,source_authority_binding(p.role.next));
    need(p.camera->commit(std::move(camera)));
    // These are actual original native operations on the disposable target,
    // never a claim that the derived target is byte-identical to its source cut.
    for(const auto &player:session->roster())if(online_admission_peer(player)==p.role.previous.host)
        world.match.depart(player.slot);
    world.source_objects->retire_lens_queries();
    need(authority.execution_tick()==p.role.cut.tick);p.boundary=authority.execution_boundary();
}
bool SourcePeerHostTakeover::prepare_prospective(std::shared_ptr<NativeImport> &handle,const PeerProspectiveHostRole &role){
    need(bool(handle));auto &imported=*handle;
    need(imported.executor==std::this_thread::get_id()&&imported.session);
    if(role!=derive_peer_prospective_host_role(imported.cut.context,imported.cut.seal,imported.cut.peer)||
        !current_import(*imported.session,imported.cut,imported.admissions)||
        imported.local_views!=imported.session->local_slots())return false;
    if(imported.prospective){
        if(imported.prospective->role!=role)return false;
        const auto &p=*imported.prospective;
        if(imported.session->loaded_fingerprint()!=p.fingerprint)return false;
        if(p.publication)return prospective_ready(imported);
        (void)p.world->original_cache();
        return p.authority->execution_boundary()==p.boundary&&
            imported.session->loaded_fingerprint()==p.fingerprint;
    }
    need(bool(imported.world));(void)imported.world->original_cache();
    {SourceMatchAuthority authority(imported.world->match,*imported.world->source_objects);
        need(authority.execution_boundary()==imported.boundary);}
    const auto fingerprint=imported.session->loaded_fingerprint();if(!fingerprint)return false;
    // Every fallible value copy precedes consumption. A partly finalized
    // private native target is disposable, never a valid retry image.
    auto prospective=std::make_unique<ProspectiveNative>();prospective->role=role;
    prospective->fingerprint=*fingerprint;prospective->input_cut=imported.saved_host;
    try{
        prospective->world=std::move(imported.world);
        auto &p=*prospective;auto &world=*p.world;p.cache=world.original_cache();
        p.authority=std::make_unique<SourceMatchAuthority>(world.match,*world.source_objects);
        prepare_role_mutations(p,imported.session,imported.admissions,*p.authority);
        need(current_import(*imported.session,imported.cut,imported.admissions)&&
            imported.session->loaded_fingerprint()==fingerprint);
        p.native=&world.source_objects->tu15_world();
        // Converted native sound ownership is optional for the original-cache
        // reference route; it is mandatory when target audio requests a binding.
        if(p.cache->is_mcc())p.audio_provider=p.native->sound_variation_provider(*p.cache);
        p.publication_owner=SourceNativePublication::make_owner(p.world,imported.cut);
        imported.prospective=std::move(prospective);return true;
    }catch(...){if(!imported.world)handle.reset();throw;}
}
bool SourcePeerHostTakeover::has_prospective(const NativeImport &imported)noexcept{
    return imported.executor==std::this_thread::get_id()&&bool(imported.prospective);
}
const PeerProspectiveHostRole &SourcePeerHostTakeover::prospective_role(const NativeImport &imported){
    need(imported.executor==std::this_thread::get_id()&&imported.prospective);return imported.prospective->role;
}
MultiplayerPreparedWorld &SourcePeerHostTakeover::prospective_target(NativeImport &imported){
    need(imported.executor==std::this_thread::get_id()&&imported.prospective);
    auto &p=*imported.prospective;need(!p.publication&&SourceNativePublication::owner_available(p.publication_owner));
    need(current_import(*imported.session,imported.cut,imported.admissions)&&
        imported.session->loaded_fingerprint()==p.fingerprint);
    (void)p.world->original_cache();need(p.authority->execution_boundary()==p.boundary);return *p.world;
}
void SourcePeerHostTakeover::seal_prospective(NativeImport &imported){
    need(imported.executor==std::this_thread::get_id()&&imported.prospective&&imported.session);
    auto &p=*imported.prospective;
    need(current_import(*imported.session,imported.cut,imported.admissions)&&
        imported.session->loaded_fingerprint()==p.fingerprint);
    if(p.publication){need(prospective_ready(imported));return;}
    (void)p.world->original_cache();need(p.authority->execution_boundary()==p.boundary);
    p.publication=std::unique_ptr<SourceNativePublication>(new SourceNativePublication(
        SourceNativePublication::acquire(p.publication_owner)));
}
bool SourcePeerHostTakeover::prospective_ready(const NativeImport &imported)noexcept{
    if(imported.executor!=std::this_thread::get_id()||!imported.prospective)return false;
    const auto &p=*imported.prospective;
    return p.world&&p.authority&&p.host&&p.camera&&p.native&&p.cache&&p.publication&&
        p.publication->matches(*p.native,p.cache)&&p.host->continuation_liveness_ready();
}
MccSoundVariationProvider::NativeBinding SourcePeerHostTakeover::prospective_audio_binding(const NativeImport &imported){
    need(prospective_ready(imported));const auto &p=*imported.prospective;need(p.audio_provider.has_value());
    return p.audio_provider->native_binding(*p.publication,p.cache);
}
SourceNativePublication::Boundary SourcePeerHostTakeover::prospective_boundary_binding(NativeImport &imported){
    need(prospective_ready(imported));return imported.prospective->publication->boundary();
}
std::unique_ptr<SourcePeerHostTakeover> SourcePeerHostTakeover::prepare_imported(
    std::shared_ptr<NativeImport> &handle,const PeerMigrationDecision &decision){
    // Refused certificate/context/admission/elected-self checks precede the
    // first move, preserving the valid private reconstruction for retry.
    need(bool(handle));auto &imported=*handle;
    need(imported.executor==std::this_thread::get_id()&&imported.session&&
        imported.cut.peer==decision.value.host&&imported.cut.context==decision.previous&&imported.cut.seal==decision.value.cut&&
        current(*imported.session,decision,imported.admissions)&&
        imported.local_views==imported.session->local_slots());
    if(imported.prospective){
        const auto &p=*imported.prospective;
        need(prospective_ready(imported)&&p.role.previous==decision.previous&&p.role.next==decision.next&&p.role.cut==decision.value.cut&&
            p.world->local_slots==imported.local_views&&imported.session->loaded_fingerprint()==p.fingerprint);
        // All potentially throwing copies/allocation precede the no-throw
        // owner transfer. The same actual native scope/target remains held.
        auto session=imported.session;auto admissions=imported.admissions;
        auto result=std::unique_ptr<SourcePeerHostTakeover>(new SourcePeerHostTakeover(
            std::move(session),decision,std::move(admissions),std::move(imported.prospective)));
        handle.reset();return result;
    }
    need(imported.world&&imported.world->local_slots==imported.local_views);
    (void)imported.world->original_cache();
    {SourceMatchAuthority authority(imported.world->match,*imported.world->source_objects);
        need(authority.execution_boundary()==imported.boundary);}
    // Finish fallible value copies before consuming the target. Constructor
    // work below can still reject changed session/fingerprint state or throw
    // during native role/camera/publication allocation. That target is then
    // disposable: clear the consumed wrapper and retain only the caller's
    // authentic strong pair, from which a fresh native import can retry.
    auto session=imported.session;auto admissions=imported.admissions;auto saved=imported.saved_host;
    try{
        auto result=std::unique_ptr<SourcePeerHostTakeover>(new SourcePeerHostTakeover(std::move(session),decision,
            std::move(admissions),std::move(imported.world),saved,imported.cut));
        handle.reset();return result;
    }catch(...){if(!handle->world)handle.reset();throw;}
}
SourcePeerHostTakeover::SourcePeerHostTakeover(std::shared_ptr<OnlineMatchSession> session,PeerMigrationDecision decision,
    AcceptedSourcePlayers admissions,std::unique_ptr<MultiplayerPreparedWorld> world,const LanHost::Continuation &saved,
    const ValidatedAuthorityCut &cut)
    :session_(std::move(session)),decision_(std::move(decision)),admissions_(std::move(admissions)),world_(std::move(world)) {
    need(session_&&current(*session_,decision_,admissions_)&&world_&&world_->source_objects&&
        world_->local_slots==session_->local_slots()&&cut.peer==session_->self()&&
        cut.context==decision_.previous&&cut.seal==decision_.value.cut);
    (void)world_->original_cache();
    const auto fingerprint=session_->loaded_fingerprint();need(fingerprint.has_value());
    SourceMatchAuthority authority(world_->match,*world_->source_objects);
    need(authority.execution_tick()==decision_.value.cut.tick);
    ProspectiveNative role;role.role={decision_.previous,decision_.next,decision_.value.cut};
    role.fingerprint=*fingerprint;role.world=world_;role.input_cut=saved;
    prepare_role_mutations(role,session_,admissions_,authority);
    host_=std::move(role.host);local_=std::move(role.local);input_cut_=std::move(role.input_cut);camera_=std::move(role.camera);
    need(authority.execution_tick()==decision_.value.cut.tick&&current(*session_,decision_,admissions_));
    prepared_=authority.execution_boundary();
    // world_ was formed by moving this constructor's actual unique target
    // into a real shared owner. Capture its original Cache control block and
    // nested native identities while the complete target is still private.
    publication_owner_=SourceNativePublication::make_owner(world_,cut);
}
SourcePeerHostTakeover::SourcePeerHostTakeover(std::shared_ptr<OnlineMatchSession> session,PeerMigrationDecision decision,
    AcceptedSourcePlayers admissions,std::unique_ptr<ProspectiveNative> prospective)noexcept
    :session_(std::move(session)),decision_(std::move(decision)),admissions_(std::move(admissions)),
     world_(std::move(prospective->world)),publication_owner_(std::move(prospective->publication_owner)),
     host_(std::move(prospective->host)),local_(std::move(prospective->local)),input_cut_(std::move(prospective->input_cut)),
     camera_(std::move(prospective->camera)),prepared_(prospective->boundary),prospective_cache_(std::move(prospective->cache)),
     prospective_native_(prospective->native),prospective_fingerprint_(prospective->fingerprint),
     prospective_authority_(std::move(prospective->authority)),prospective_publication_(std::move(prospective->publication)){}
SourcePeerHostTakeover::~SourcePeerHostTakeover(){
    // Camera/host value destruction does not reenter native World/timing;
    // final session/voice teardown still belongs outside timing/device holds.
    SourceNativePublication::close_owner(publication_owner_);
}
void SourcePeerHostTakeover::own()const{
    need(std::this_thread::get_id()==executor_ && SourceNativePublication::owner_available(publication_owner_));
    (void)world_->original_cache();
}
std::shared_ptr<const Cache> SourcePeerHostTakeover::original_presentation_cache(const MultiplayerPreparedWorld &world){
    return world.original_cache();
}
bool SourcePeerHostTakeover::has_prospective()const noexcept{
    return executor_==std::this_thread::get_id()&&prospective_fingerprint_.has_value();
}
bool SourcePeerHostTakeover::publication_held()const noexcept{
    return executor_==std::this_thread::get_id()&&bool(prospective_publication_);
}
bool SourcePeerHostTakeover::held_prospective()const noexcept{
    return has_prospective()&&prospective_cache_&&prospective_native_&&prospective_publication_&&
        prospective_publication_->matches(*prospective_native_,prospective_cache_);
}
const PeerMigrationDecision &SourcePeerHostTakeover::prospective_decision()const{
    need(held_prospective());return decision_;
}
std::unique_ptr<SourceMatchAuthority> SourcePeerHostTakeover::take_prospective_authority(){
    need(held_prospective()&&!active_&&prospective_authority_);return std::move(prospective_authority_);
}
void SourcePeerHostTakeover::refresh_prospective_local_input(const LanClient &client){
    need(held_prospective()&&!active_);client.merge_local_input(input_cut_,decision_.previous,local_);
}
bool SourcePeerHostTakeover::activate_prospective(OnlineMatchSession::NativeCommit native_adopt,void *owner){
    if(!held_prospective()||active_||!current(*session_,decision_,admissions_)||
        session_->loaded_fingerprint()!=prospective_fingerprint_||!host_->continuation_liveness_ready())return false;
    // The actual retained native gate and the never-exposed target exclude
    // mutation since the complete derived boundary was sealed. Reentering
    // SourceMatchAuthority/World here would violate that very exclusion.
    struct Adoption {SourcePeerHostTakeover *self;OnlineMatchSession::NativeCommit adopt;void *owner;};
    Adoption pending{this,native_adopt,owner};
    const auto commit=[](void *opaque)noexcept{
        auto &p=*static_cast<Adoption *>(opaque);p.self->active_=true;
        if(p.adopt&&!p.adopt(p.owner)){p.self->active_=false;return false;}
        // No ownership-slot write, native call, transport service or scope
        // release may intervene in this preflighted callback transaction.
        if(!p.self->held_prospective()||!p.self->host_->activate_continuation_liveness())std::terminate();
        return true;
    };
    return session_->finish_peer_migration(decision_,commit,&pending);
}
void SourcePeerHostTakeover::release_prospective_publication(){
    need(executor_==std::this_thread::get_id()&&active_&&prospective_publication_);
    prospective_publication_.reset();input_cut_={};
}
const PeerMigrationDecision &SourcePeerHostTakeover::decision()const{own();return decision_;}
MultiplayerPreparedWorld &SourcePeerHostTakeover::world(){own();return *world_;}
LanHost &SourcePeerHostTakeover::host(){own();need(active_);return *host_;}
LanHost::LocalContinuation &SourcePeerHostTakeover::local(){own();return local_;}
void SourcePeerHostTakeover::refresh_local_input(const LanClient &client){
    own();need(!active_);client.merge_local_input(input_cut_,decision_.previous,local_);
}
NativeVehicleCamera &SourcePeerHostTakeover::camera(){own();return *camera_;}
SourceNativePublication SourcePeerHostTakeover::hold_native_publication() {
    own();
    // Full native reads/admission checks finish before any retained gate and
    // before a later timing/SDL scope. The held token itself never re-reads
    // native bytes or claims renderer/Match/Simulation exclusion for the root.
    SourceMatchAuthority authority(world_->match,*world_->source_objects);
    need(authority.execution_boundary()==prepared_);
    return SourceNativePublication::acquire(publication_owner_);
}
bool SourcePeerHostTakeover::activate(OnlineMatchSession::NativeCommit native_adopt,void *owner){
    own();if(active_||!current(*session_,decision_,admissions_)||!host_->continuation_liveness_ready())return false;
    SourceMatchAuthority authority(world_->match,*world_->source_objects);
    if(authority.execution_boundary()!=prepared_)return false;
    struct Adoption {SourcePeerHostTakeover *self;OnlineMatchSession::NativeCommit adopt;void *owner;};
    Adoption pending{this,native_adopt,owner};
    const auto commit=[](void *opaque)noexcept{
        auto &p=*static_cast<Adoption *>(opaque);p.self->active_=true;
        if(p.adopt&&!p.adopt(p.owner)){p.self->active_=false;return false;}
        // Readiness was checked on this executor before publication. The
        // native adoption contract forbids transport service/reentry, so the
        // post-adoption clock swap cannot fail or return a partial refusal.
        if(!p.self->host_->activate_continuation_liveness())std::terminate();
        return true;
    };
    const bool activated=session_->finish_peer_migration(decision_,commit,&pending);
    if(activated)input_cut_={};
    return activated;
}
}
