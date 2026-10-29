#include "halo2/source_shadow_presentation_owner.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_authority_receiver.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

namespace halo2 {
struct SourceShadowPresentationOwner::Impl {
    const std::thread::id executor=std::this_thread::get_id();
    MultiplayerOptions options;const MultiplayerPreparedWorld &resident;
    std::unique_ptr<MultiplayerPreparedWorld> recipient,workspace;
    std::unique_ptr<SourceMatchAuthority> authority,work_authority;
    MultiplayerSnapshotOrigin origin;
    std::optional<AcceptedShadowCut> cut;
    std::vector<unsigned> producer_views,local_views;
    unsigned traced_adoptions{};
    explicit Impl(const MultiplayerOptions &request,const MultiplayerPreparedWorld &r)
        :options(source_authority_options(request)),resident(r){}
    void own()const {if(executor!=std::this_thread::get_id())throw FormatError("Native shadow presentation crossed its foreground executor");}
    void clear(){authority.reset();work_authority.reset();recipient.reset();workspace.reset();origin={};cut.reset();producer_views.clear();local_views.clear();}
};
SourceShadowPresentationOwner::SourceShadowPresentationOwner(const MultiplayerOptions &options,
    const MultiplayerPreparedWorld &resident):impl_(std::make_unique<Impl>(options,resident)){}
SourceShadowPresentationOwner::~SourceShadowPresentationOwner(){if(impl_->executor!=std::this_thread::get_id())std::terminate();}
bool SourceShadowPresentationOwner::adopt(const LanClient &client,const AcceptedShadowCut &cut,
    const MultiplayerSnapshotOrigin &current) {
    auto &p=*impl_;p.own();
    if(!client.owns_shadow_cut(cut,current) || !cut.origin().same(current) || !current.tick() || !current.sequence())
        throw FormatError("Native shadow cut does not own the currently accepted recipient body");
    if(p.origin.same(current) && p.cut && p.cut->same_generation(cut) &&
       client.owns_shadow_cut(*p.cut,current))return true;
    if(p.cut && p.cut->same_world(cut) && p.origin.sequence() &&
       std::int32_t(*current.sequence()-*p.origin.sequence())<=0)
        throw FormatError("Native shadow presentation rejected a stale recipient snapshot");
    using Clock=std::chrono::steady_clock;
    const auto* trace_option=std::getenv("HALO2_SHADOW_TRANSPORT_TRACE");
    const bool trace=trace_option&&*trace_option&&p.traced_adoptions<16;
    const auto started=trace?Clock::now():Clock::time_point{};
    auto checkpoint=decode_source_authority_image(cut.archive_image(),
        source_shadow_archive_codec_limits());
    const auto decoded=trace?Clock::now():Clock::time_point{};
    if(checkpoint.match.simulation.ticks!=*current.tick() || checkpoint.match.admitted_binding!=cut.binding() ||
       checkpoint.match.simulation.admitted_binding!=cut.binding() || cut.admissions().empty() ||
       cut.local_views().size()!=p.options.local_players)
        throw FormatError("Native shadow checkpoint has a foreign tick, binding or admitted player owner");
    const std::vector<unsigned> producer(cut.producer_views().begin(),cut.producer_views().end());
    const std::vector<unsigned> locals(cut.local_views().begin(),cut.local_views().end());
    if(p.cut && (!p.cut->same_world(cut) || p.producer_views!=producer || p.local_views!=locals))p.clear();
    if(!p.recipient) {
        auto recipient=MultiplayerPreparedWorld::shadow_presentation_world(p.options,p.resident,cut.roster(),locals,locals);
        auto workspace=MultiplayerPreparedWorld::shadow_presentation_world(p.options,p.resident,cut.roster(),producer,locals);
        auto authority=std::make_unique<SourceMatchAuthority>(recipient->match,*recipient->source_objects);
        auto work=std::make_unique<SourceMatchAuthority>(workspace->match,*workspace->source_objects);
        p.recipient=std::move(recipient);p.workspace=std::move(workspace);
        p.authority=std::move(authority);p.work_authority=std::move(work);
        p.producer_views=producer;p.local_views=locals;
    }
    std::vector<SourcePlayerAdmission> admitted;
    for(auto slot:locals) {
        const auto found=std::ranges::find(cut.admissions(),slot,&SourcePlayerAdmission::slot);
        if(found==cut.admissions().end())throw FormatError("Native shadow cut omitted an admitted local player");
        admitted.push_back(*found);
    }
    const auto import_started=trace?Clock::now():Clock::time_point{};
    p.work_authority->import_presentation(checkpoint,cut.binding(),admitted,
        cut.admissions(),producer,*p.authority);
    const auto imported=trace?Clock::now():Clock::time_point{};
    // Authentication/generation can retire while the large native cut stages.
    // Recheck before publishing either coupled target bank.
    if(!client.owns_shadow_cut(cut,current))return false;
    if(p.work_authority->execution_tick()!=*current.tick())throw FormatError("Native shadow presentation staged a different source tick");
    p.recipient->source_objects->tu15_world().require_presentation_boundary();
    p.workspace->source_objects->tu15_world().require_presentation_boundary();
    p.recipient.swap(p.workspace);p.authority.swap(p.work_authority);
    p.origin=current;p.cut=cut;
    if(trace) {
        const auto finished=Clock::now();
        const auto us=[](auto a,auto b){return static_cast<unsigned long long>(
            std::chrono::duration_cast<std::chrono::microseconds>(b-a).count());};
        std::fprintf(stderr,"[shadow-presentation] {\"tick\":%u,\"sequence\":%u,\"closeddecode_us\":%llu,\"setup_us\":%llu,\"import_us\":%llu,\"total_us\":%llu}\n",
            *current.tick(),*current.sequence(),us(started,decoded),us(decoded,import_started),
            us(import_started,imported),us(started,finished));
        ++p.traced_adoptions;
    }
    return true;
}
MultiplayerObjectRuntime *SourceShadowPresentationOwner::world(const LanClient &client,const MultiplayerSnapshotOrigin &current)const {
    const auto &p=*impl_;p.own();return p.recipient && p.cut && p.origin.same(current) &&
        client.owns_shadow_cut(*p.cut,current)?p.recipient->source_objects.get():nullptr;
}
const MultiplayerMatch *SourceShadowPresentationOwner::match(const LanClient &client,const MultiplayerSnapshotOrigin &current)const {
    return world(client,current)?&impl_->recipient->match:nullptr;
}
const MultiplayerSnapshotOrigin &SourceShadowPresentationOwner::origin()const{impl_->own();return impl_->origin;}
void SourceShadowPresentationOwner::reset(){impl_->own();impl_->clear();}
}
