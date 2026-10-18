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
    // Keep asset references held by the renderer alive through a role change.
    std::shared_ptr<MultiplayerPreparedWorld> resident;
    std::unique_ptr<LanHost> initial_host;
    std::unique_ptr<LanClient> initial_client;
    std::unique_ptr<SourcePeerHostTakeover> active,pending;
    std::unique_ptr<SourceMatchAuthority> authority,pending_authority;
    std::unique_ptr<SourceAuthorityReplayWorker> receiver;
    std::unique_ptr<SourceAuthorityPublisher> publisher;
    std::optional<std::future<SourceAuthorityRetainedWorld>> recovery;
    std::shared_ptr<const SourceAuthorityRetainedWorld> retained;
    std::exception_ptr recovery_failure;
    std::optional<AuthorityTransferContext> published_context,client_context,shadow_context;
    std::optional<unsigned> shadow_target;
    Clock::time_point capture_retry{};
    std::string admission_error,receiver_error,publication_error;
    bool frame{},continuing_input{};
    std::optional<std::uint64_t> continued_input_tick;
    std::array<Controls,4> continued_input;
    void own()const{need(executor==std::this_thread::get_id());}
    void quiescent()const{own();need(!frame);}
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
    s.online=options.online_match;s.resident=std::move(world);
    s.initial_host=std::move(host);s.initial_client=std::move(client);s.receiver=std::move(receiver);
    need(!s.receiver||(s.online&&s.initial_client));
    if(s.online&&s.initial_host&&s.world().source_objects&&s.world().source_objects->owns_physics())
        s.authority=std::make_unique<SourceMatchAuthority>(s.world().match,*s.world().source_objects);
}
MultiplayerRuntimeAuthority::~MultiplayerRuntimeAuthority(){
    // Native objects and publisher frames must retire on their executor.
    if(impl_->executor!=std::this_thread::get_id()||impl_->frame)std::terminate();
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
    auto &s=*impl_;s.quiescent();need(candidate&&s.online&&!s.host()&&!s.pending&&!s.receiver);
    const auto state=s.online->peer_migration();
    need(state&&state->decision&&*state->decision==candidate->decision()&&s.online->self()==candidate->decision().value.host);
    auto &world=candidate->world();need(world.source_objects&&world.source_objects->owns_physics()&&
        world.cache->path()==s.resident->cache->path()&&world.cache->shared_directory()==s.resident->cache->shared_directory());
    auto authority=std::make_unique<SourceMatchAuthority>(world.match,*world.source_objects);
    s.pending_authority=std::move(authority);s.pending=std::move(candidate);
}
SourcePeerHostTakeover *MultiplayerRuntimeAuthority::pending_takeover(){impl_->own();return impl_->pending.get();}
bool MultiplayerRuntimeAuthority::activate_takeover(){
    auto &s=*impl_;s.quiescent();if(!s.pending||!s.pending->activate())return false;
    // All potentially throwing construction precedes the session's atomic
    // generation publication. Retire old role owners before exposing the new one.
    s.publisher.reset();s.initial_client.reset();s.initial_host.reset();s.authority=std::move(s.pending_authority);
    s.active=std::move(s.pending);s.retained.reset();s.recovery.reset();
    s.continuing_input=true;
    s.published_context.reset();s.client_context.reset();s.shadow_context.reset();s.shadow_target.reset();s.capture_retry={};
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
