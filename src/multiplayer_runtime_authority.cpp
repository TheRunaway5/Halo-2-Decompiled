#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_authority_publisher.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/source_peer_host_takeover.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

namespace halo2 {
namespace {
using Clock=std::chrono::steady_clock;
void need(bool value){if(!value)throw FormatError("Invalid runtime authority lifetime or role transition");}
void diagnostic(std::string &previous,std::string_view prefix,const std::string &error){
    if(!error.empty()&&previous!=error){previous=error;std::cerr<<prefix<<error<<'\n';}
}
}
struct MultiplayerRuntimeAuthority::Impl {
    const std::thread::id executor=std::this_thread::get_id();
    std::shared_ptr<OnlineMatchSession> online;
    MultiplayerOptions recovery_options;
    // Keep asset references held by the renderer alive through a role change.
    std::shared_ptr<MultiplayerPreparedWorld> resident;
    std::unique_ptr<LanHost> initial_host;
    std::unique_ptr<LanClient> initial_client;
    std::unique_ptr<SourcePeerHostTakeover> active,pending;
    struct ClientRebind {PeerMigrationDecision decision;std::unique_ptr<LanClient::PreparedRebind> native;};
    std::optional<ClientRebind> pending_client;
    std::unique_ptr<SourceMatchAuthority> authority,pending_authority;
    std::unique_ptr<SourceAuthorityReplayWorker> receiver;
    std::unique_ptr<SourceAuthorityPublisher> publisher;
    std::optional<std::future<SourceAuthorityRetainedWorld>> recovery;
    std::shared_ptr<const SourceAuthorityRetainedWorld> retained;
    std::exception_ptr recovery_failure;
    std::optional<AuthorityTransferContext> published_context,client_context,shadow_context;
    std::optional<unsigned> shadow_target;
    Clock::time_point capture_retry{};
    Clock::time_point receiver_retry{};
    std::string admission_error,receiver_error,publication_error;
    bool frame{},committing{},continuing_input{},recovery_enabled{},restart_receiver{};
    std::optional<std::uint64_t> continued_input_tick;
    std::array<Controls,4> continued_input;
    void own()const{need(executor==std::this_thread::get_id()&&!committing);}
    void quiescent()const{own();need(!frame);}
    bool presentation_commit(PresentationCommit commit,void *owner) noexcept {
        // Reject runtime reentrancy before a callback can enter the session
        // mutex or observe the middle of a coupled owner publication.
        committing=true;const bool accepted=!commit||commit(owner);committing=false;
        return accepted;
    }
    MultiplayerPreparedWorld &world(){return active?active->world():*resident;}
    LanHost *host(){return active?&active->host():initial_host.get();}
    LanClient *client(){return active?nullptr:initial_client.get();}
    void publish(){
        if(!authority)return;
        try{
            const auto context=online->authority_context();
            if(!context||online->phase()!=OnlineMatchSession::Phase::Running||context->host!=online->self()){
                publisher.reset();shadow_target.reset();shadow_context.reset();return;
            }
            const auto candidates=online->host_candidates();
            const auto eligible=[&](unsigned peer){return peer!=online->self()&&online->authority_replay_supported(peer)&&
                online->authority_host_continuation_supported(peer)&&online->source_players_confirmed(peer)&&
                std::ranges::any_of(candidates,[&](const auto &q){return q.peer==peer;});};
            auto selected=shadow_target;
            if(shadow_context!=context||!selected||!eligible(*selected)){
                selected.reset();for(const auto &q:candidates)if(eligible(q.peer)){selected=q.peer;break;}
            }
            if(selected!=shadow_target||shadow_context!=context){
                publisher.reset();shadow_target=selected;shadow_context=context;capture_retry={};
                if(selected)publisher=std::make_unique<SourceAuthorityPublisher>(*authority,*host(),online,*selected);
            }
            if(!publisher)return;
            publisher->update();auto state=publisher->state();
            if(!state.tracking_replay&&!state.encoding&&!state.awaiting_validation&&Clock::now()>=capture_retry){
                capture_retry=Clock::now()+std::chrono::seconds(3);publisher->capture();state=publisher->state();
            }
            diagnostic(publication_error,"Source shadow publication unavailable: ",state.error);
        }catch(const std::exception &e){
            if(publisher)publisher->cancel();
            capture_retry=Clock::now()+std::chrono::seconds(3);
            diagnostic(publication_error,"Source shadow publication unavailable: ",e.what());
        }
    }
};
MultiplayerRuntimeAuthority::MultiplayerRuntimeAuthority(const MultiplayerOptions &options,
    std::shared_ptr<MultiplayerPreparedWorld> world,std::unique_ptr<LanHost> host,std::unique_ptr<LanClient> client,
    std::unique_ptr<SourceAuthorityReplayWorker> receiver):impl_(std::make_unique<Impl>()){
    auto &s=*impl_;need(world&&world->matches(options)&&!(host&&client));
    s.online=options.online_match;s.recovery_options=source_authority_options(options);s.resident=std::move(world);
    s.initial_host=std::move(host);s.initial_client=std::move(client);s.receiver=std::move(receiver);
    s.recovery_enabled=bool(s.receiver);
    need(!s.receiver||(s.online&&s.initial_client));
    if(s.online&&s.initial_host&&s.world().source_objects&&s.world().source_objects->owns_physics())
        s.authority=std::make_unique<SourceMatchAuthority>(s.world().match,*s.world().source_objects);
}
MultiplayerRuntimeAuthority::~MultiplayerRuntimeAuthority(){
    // Native objects and publisher frames must retire on their executor.
    if(impl_->executor!=std::this_thread::get_id()||impl_->frame||impl_->committing)std::terminate();
}
MultiplayerPreparedWorld &MultiplayerRuntimeAuthority::world(){impl_->own();return impl_->world();}
MultiplayerMatch &MultiplayerRuntimeAuthority::match(){return world().match;}
LanHost *MultiplayerRuntimeAuthority::host(){impl_->own();return impl_->host();}
LanClient *MultiplayerRuntimeAuthority::client(){impl_->own();return impl_->client();}
void MultiplayerRuntimeAuthority::admit_players(){
    auto &s=*impl_;s.own();if(!s.authority||s.online->phase()!=OnlineMatchSession::Phase::Running)return;
    const auto context=s.online->authority_context();
    if(!context||context->host!=s.online->self()||s.published_context==context)return;
    std::vector<unsigned> slots;
    for(const auto &p:s.online->roster()){
        if(!s.world().match.player(p.slot).connected)return;
        slots.push_back(p.slot);
    }
    try{const auto players=s.authority->player_admissions(slots);
        if(s.online->publish_source_players(*context,players))s.published_context=context;
    }catch(const std::exception &e){diagnostic(s.admission_error,"Source player admission unavailable: ",e.what());}
}
void MultiplayerRuntimeAuthority::service(){
    auto &s=*impl_;s.quiescent();admit_players();s.publish();receive();
}
void MultiplayerRuntimeAuthority::receive(){
    auto &s=*impl_;s.own();
    if(s.restart_receiver&&s.online&&s.client()&&s.online->phase()==OnlineMatchSession::Phase::Running){
        if(s.receiver&&s.receiver->stopped())s.receiver.reset();
        if(!s.receiver&&Clock::now()>=s.receiver_retry)try{
            s.receiver=std::make_unique<SourceAuthorityReplayWorker>(s.recovery_options);s.restart_receiver=false;
        }catch(const std::exception &error){
            s.receiver_retry=Clock::now()+std::chrono::seconds(1);
            diagnostic(s.receiver_error,"Source shadow restart unavailable: ",error.what());
        }
    }
    if(s.online&&s.client()&&s.client()->ready()&&!s.client_context&&s.online->direct_udp()){
        if(const auto context=s.online->authority_context())try{
            s.client()->bind_continuity(*context,s.online->self(),s.online->roster());s.client_context=context;
        }catch(const std::exception &e){diagnostic(s.receiver_error,"Source client continuity unavailable: ",e.what());}
    }
    if(s.receiver)diagnostic(s.receiver_error,"Source shadow unavailable: ",s.receiver->state().error);
}
void MultiplayerRuntimeAuthority::suspend_publication(){auto &s=*impl_;s.quiescent();if(s.publisher)s.publisher->cancel();}
struct MultiplayerRuntimeAuthority::Frame::Impl {
    MultiplayerRuntimeAuthority::Impl *owner;
    std::unique_ptr<SourceAuthorityPublisher::Frame> recording;
};
MultiplayerRuntimeAuthority::Frame::Frame(MultiplayerRuntimeAuthority &owner):impl_(std::make_unique<Impl>()){
    auto &s=*owner.impl_;s.quiescent();
    impl_->owner=&s;impl_->recording=s.publisher?s.publisher->record_frame():nullptr;s.frame=true;
}
bool MultiplayerRuntimeAuthority::Frame::finish()noexcept{
    if(!impl_->owner)return true;
    auto &s=*impl_->owner;if(s.executor!=std::this_thread::get_id()||!s.frame)std::terminate();
    const bool recorded=!impl_->recording||impl_->recording->finish();impl_->recording.reset();s.frame=false;impl_->owner=nullptr;return recorded;
}
MultiplayerRuntimeAuthority::Frame::~Frame(){finish();}
std::unique_ptr<MultiplayerRuntimeAuthority::Frame> MultiplayerRuntimeAuthority::begin_frame(){
    auto &s=*impl_;s.quiescent();
    if(s.online){const auto phase=s.online->phase();
        if(phase==OnlineMatchSession::Phase::AwaitingWorld||phase==OnlineMatchSession::Phase::Migrating||
            phase==OnlineMatchSession::Phase::Rebinding||phase==OnlineMatchSession::Phase::Failed)return {};}
    return std::unique_ptr<Frame>(new Frame(*this));
}
bool MultiplayerRuntimeAuthority::has_receiver()const{impl_->own();return bool(impl_->receiver)||bool(impl_->retained);}
std::optional<SourceAuthorityReceiverState> MultiplayerRuntimeAuthority::shadow_state()const{
    impl_->own();return impl_->receiver?std::optional(impl_->receiver->state()):std::nullopt;
}
std::shared_ptr<const SourceAuthorityRetainedWorld> MultiplayerRuntimeAuthority::recover(){
    auto &s=*impl_;s.quiescent();if(s.recovery_failure)std::rethrow_exception(s.recovery_failure);
    if(!s.retained){
        if(!s.receiver)return {};
        if(!s.recovery)s.recovery=s.receiver->retain_validated_world();
        if(s.recovery->wait_for(std::chrono::milliseconds(0))!=std::future_status::ready)return {};
        try{s.retained=std::make_shared<const SourceAuthorityRetainedWorld>(s.recovery->get());}
        catch(...){s.recovery_failure=std::current_exception();s.recovery.reset();s.receiver->cancel();throw;}
        s.recovery.reset();s.receiver->cancel();
    }
    // A completed export does not imply completed native-world destruction.
    // Keep OS/input events moving until the executor acknowledges retirement.
    if(s.receiver){if(!s.receiver->stopped())return {};s.receiver.reset();}
    return s.retained;
}
void MultiplayerRuntimeAuthority::cancel_recovery(){auto &s=*impl_;s.quiescent();if(s.receiver)s.receiver->cancel();}
void MultiplayerRuntimeAuthority::stage_takeover(std::unique_ptr<SourcePeerHostTakeover> &&candidate){
    auto &s=*impl_;s.quiescent();need(candidate&&s.online&&!s.host()&&!s.pending&&!s.pending_client&&!s.receiver);
    const auto state=s.online->peer_migration();
    need(state&&state->decision&&*state->decision==candidate->decision()&&s.online->self()==candidate->decision().value.host);
    auto &world=candidate->world();need(world.source_objects&&world.source_objects->owns_physics()&&
        world.cache->path()==s.resident->cache->path()&&world.cache->shared_directory()==s.resident->cache->shared_directory());
    auto authority=std::make_unique<SourceMatchAuthority>(world.match,*world.source_objects);
    s.pending_authority=std::move(authority);s.pending=std::move(candidate);
}
SourcePeerHostTakeover *MultiplayerRuntimeAuthority::pending_takeover(){impl_->own();return impl_->pending.get();}
bool MultiplayerRuntimeAuthority::prepare_takeover(std::shared_ptr<PhysicsLiveClockSource> clock){
    auto &s=*impl_;s.quiescent();if(s.pending)return true;
    if(!s.online||s.host()||!s.client()||s.online->phase()!=OnlineMatchSession::Phase::Migrating)return false;
    const auto agreement=s.online->peer_migration();
    if(!agreement||!agreement->decision||agreement->decision->value.host!=s.online->self())return false;
    const auto retained=recover();if(!retained)return false;
    auto candidate=SourcePeerHostTakeover::prepare(s.recovery_options,*retained,*agreement->decision,std::move(clock));
    stage_takeover(std::move(candidate));return true;
}
bool MultiplayerRuntimeAuthority::activate_takeover(PresentationCommit presentation,void *owner){
    auto &s=*impl_;s.quiescent();if(!s.pending)return false;
    need(!s.active&&!s.authority&&!s.initial_host&&!s.publisher&&!s.pending_client&&s.pending_authority);
    receive();need(bool(s.initial_client));s.pending->refresh_local_input(*s.initial_client);
    struct Adoption {Impl &state;PresentationCommit presentation;void *owner;std::unique_ptr<LanClient> retired_client;};
    Adoption adoption{s,presentation,owner,{}};
    const auto commit=[](void *opaque)noexcept{
        auto &a=*static_cast<Adoption *>(opaque);auto &state=a.state;
        if(!state.presentation_commit(a.presentation,a.owner))return false;
        // Only move into empty owners under the session lock. Destruction of
        // the old client can send a leave and must happen outside that lock.
        a.retired_client=std::move(state.initial_client);
        state.authority=std::move(state.pending_authority);state.active=std::move(state.pending);
        state.continuing_input=true;state.continued_input_tick.reset();
        state.published_context.reset();state.client_context.reset();state.shadow_context.reset();state.shadow_target.reset();state.capture_retry={};
        return true;
    };
    auto *candidate=s.pending.get();if(!candidate->activate(commit,&adoption))return false;
    s.retained.reset();s.recovery.reset();
    return true;
}
bool MultiplayerRuntimeAuthority::prepare_client_rebind(){
    auto &s=*impl_;s.quiescent();if(s.pending_client)return true;
    if(!s.online||s.host()||!s.client()||s.pending||s.online->phase()!=OnlineMatchSession::Phase::Rebinding)return false;
    receive();const auto agreement=s.online->peer_migration();
    if(!agreement||!agreement->decision||agreement->decision->value.host==s.online->self())return false;
    const auto &decision=*agreement->decision;need(s.client_context==decision.previous);
    auto prepared=s.client()->prepare_rebind_host(decision.previous,decision.next,decision.value.cut);
    s.pending_client=Impl::ClientRebind{decision,std::move(prepared)};return true;
}
std::optional<PeerMigrationDecision> MultiplayerRuntimeAuthority::pending_client_rebind()const{
    auto &s=*impl_;s.own();return s.pending_client?std::optional(s.pending_client->decision):std::nullopt;
}
bool MultiplayerRuntimeAuthority::activate_client_rebind(PresentationCommit presentation,void *owner){
    auto &s=*impl_;s.quiescent();if(!s.pending_client)return false;
    need(s.initial_client&&s.initial_client->can_commit_rebind_host(*s.pending_client->native));
    const auto decision=s.pending_client->decision;
    struct Adoption {Impl &state;PresentationCommit presentation;void *owner;};
    Adoption adoption{s,presentation,owner};
    const auto commit=[](void *opaque)noexcept{
        auto &a=*static_cast<Adoption *>(opaque);auto &state=a.state;
        // No executor/transport mutation can intervene after this preflight:
        // the presentation callback publishes only its prepared local owners.
        if(!state.initial_client->can_commit_rebind_host(*state.pending_client->native)||
            !state.presentation_commit(a.presentation,a.owner))return false;
        return state.initial_client->commit_rebind_host(std::move(state.pending_client->native));
    };
    if(!s.online->finish_peer_migration(decision,commit,&adoption))return false;
    s.client_context=decision.next;
    const bool retired=bool(s.recovery)||bool(s.retained)||bool(s.recovery_failure);
    if(retired&&s.receiver)s.receiver->cancel();
    s.restart_receiver=s.recovery_enabled&&retired;s.receiver_retry={};
    s.recovery.reset();s.retained.reset();s.recovery_failure={};s.pending_client.reset();
    return true;
}
NativeVehicleCamera *MultiplayerRuntimeAuthority::promoted_camera(){
    auto &s=*impl_;s.own();return s.active?&s.active->camera():nullptr;
}
bool MultiplayerRuntimeAuthority::local_input_pending()const{
    auto &s=*impl_;s.own();return s.continuing_input||(s.active&&s.continued_input_tick==s.active->world().match.simulation().ticks());
}
bool MultiplayerRuntimeAuthority::continue_local_input(std::array<Controls,multiplayer_capacity> &input){
    auto &s=*impl_;s.own();need(s.frame);
    if(s.active&&s.continued_input_tick==s.active->world().match.simulation().ticks()){
        const auto &slots=s.active->local().slots;
        for(unsigned view=0;view<slots.size();++view)input[slots[view]]=s.continued_input[view];
        return true;
    }
    if(!s.continuing_input)return false;
    need(bool(s.active));auto &saved=s.active->local();LanHost::receive_local_continuation(saved,input);
    s.continued_input_tick=s.active->world().match.simulation().ticks();
    for(unsigned view=0;view<saved.slots.size();++view)s.continued_input[view]=input[saved.slots[view]];
    s.continuing_input=false;
    for(unsigned view=0;view<saved.slots.size();++view)
        for(unsigned action=0;action<saved.received_presses[view].size();++action)
            s.continuing_input|=saved.received_presses[view][action]!=saved.delivered_presses[view][action];
    return true;
}
}
