#include "halo2/source_lens_presentation_handoff_tu15.hpp"
#include "halo2/source_peer_host_takeover.hpp"
#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/multiplayer_world.hpp"

namespace halo2 {
SourceLensPresentationHandoffTu15::Prepared::Prepared(
    SourceLensQueryFrameTu15::PreparedRebind &&queries,
    SourceLensDrawFrameTu15::PreparedRebind &&draws):
    queries_(std::move(queries)),draws_(std::move(draws)) {}
SourceLensPresentationHandoffTu15::Prepared SourceLensPresentationHandoffTu15::prepare(
    SourceLensQueryFrameTu15 &queries,SourceLensDrawFrameTu15 &draws,
    const SourceRuntimeMemory &incoming) {
    auto q=queries.prepare_rebind(incoming);
    auto d=draws.prepare_rebind(incoming);
    if(queries.renderer_owner()!=draws.renderer_owner())
        throw FormatError("Lens presentation handoff requires one actual renderer");
    return Prepared(std::move(q),std::move(d));
}
SourceLensPresentationHandoffTu15::Prepared SourceLensPresentationHandoffTu15::prepare_host(
    SourceLensQueryFrameTu15 &queries,SourceLensDrawFrameTu15 &draws,SourcePeerHostTakeover &candidate) {
    const auto &decision=candidate.decision();
    const ValidatedAuthorityCut cut{decision.value.host,decision.previous,decision.value.cut};
    return prepare_native(queries,draws,candidate.world(),cut);
}
SourceLensPresentationHandoffTu15::Prepared SourceLensPresentationHandoffTu15::prepare_common(
    MultiplayerRuntimeAuthority &runtime,const ValidatedAuthorityCut &cut,
    SourceLensQueryFrameTu15 &queries,SourceLensDrawFrameTu15 &draws) {
    return prepare_native(queries,draws,runtime.common_prospective_target_for_lens(cut),cut);
}
SourceLensPresentationHandoffTu15::Prepared SourceLensPresentationHandoffTu15::prepare_native(
    SourceLensQueryFrameTu15 &queries,SourceLensDrawFrameTu15 &draws,
    MultiplayerPreparedWorld &target,const ValidatedAuthorityCut &cut) {
    const auto cache=SourcePeerHostTakeover::original_presentation_cache(target);
    auto *const objects=target.source_objects.get();
    if(!objects||!objects->owns_physics())throw FormatError("Lens native preparation requires its genuine target owner");
    SourceMatchAuthority authority(target.match,*objects);
    const auto before=authority.execution_boundary();
    if(before.tick!=cut.seal.tick)throw FormatError("Lens native preparation differs from its exact target cut");
    const auto &memory=objects->memory();
    // Both preparations perform the original retired-ring reads before any
    // native publication hold. Capture/recheck the complete real boundary,
    // never substitute caller scalars or a merely matching native tick.
    auto prepared=prepare(queries,draws,memory);
    const auto after_cache=SourcePeerHostTakeover::original_presentation_cache(target);
    if(target.source_objects.get()!=objects||cache.get()!=after_cache.get()||
        cache.owner_before(after_cache)||after_cache.owner_before(cache)||authority.execution_boundary()!=before)
        throw FormatError("Lens native preparation changed its actual target boundary or Cache owner");
    prepared.native_target_=&target;prepared.native_cache_=cache;prepared.native_cut_=cut;
    prepared.native_boundary_=before;prepared.native_memory_=&memory;
    return prepared;
}
bool SourceLensPresentationHandoffTu15::bind_publication(Prepared &prepared,SourceNativePublication::Boundary boundary) noexcept {
    if(prepared.consumed_||!prepared.native_target_||!prepared.native_cache_||!prepared.native_memory_||
        !prepared.queries_.data_||!prepared.draws_.data_||!boundary.current())return false;
    if(!boundary.matches_prepared(*prepared.native_target_,prepared.native_cache_,prepared.native_cut_,prepared.native_boundary_))return false;
    if(prepared.publication_)return prepared.publication_->current()&&prepared.publication_->same_scope(boundary);
    prepared.publication_=std::move(boundary);return true;
}
bool SourceLensPresentationHandoffTu15::ready(const SourceLensQueryFrameTu15 &queries,
    const SourceLensDrawFrameTu15 &draws,const Prepared &prepared,
    const SourceRuntimeMemory &incoming) noexcept {
    if(prepared.consumed_)return false;
    if(prepared.publication_)return prepared.native_memory_==&incoming&&ready(queries,draws,prepared);
    return queries.rebind_ready(prepared.queries_,incoming)&&
        draws.rebind_ready(prepared.draws_,incoming)&&
        queries.renderer_owner()==draws.renderer_owner();
}
bool SourceLensPresentationHandoffTu15::ready(const SourceLensQueryFrameTu15 &queries,
    const SourceLensDrawFrameTu15 &draws,const Prepared &prepared) noexcept {
    // Check the genuine weak held scope first. No raw target member/ring is
    // touched before its full world/Cache/cut/boundary identity is admitted.
    if(prepared.consumed_||!prepared.publication_||!prepared.publication_->current()||
        !prepared.native_target_||!prepared.native_memory_||!prepared.native_cache_)return false;
    if(!prepared.publication_->matches_prepared(*prepared.native_target_,prepared.native_cache_,prepared.native_cut_,prepared.native_boundary_))return false;
    return queries.rebind_ready_held(prepared.queries_,*prepared.native_memory_)&&
        draws.rebind_ready_held(prepared.draws_,*prepared.native_memory_)&&
        queries.renderer_owner()==draws.renderer_owner();
}
bool SourceLensPresentationHandoffTu15::commit(SourceLensQueryFrameTu15 &queries,
    SourceLensDrawFrameTu15 &draws,Prepared &&prepared,const SourceRuntimeMemory &incoming) noexcept {
    if(!ready(queries,draws,prepared,incoming))return false;
    queries.publish_rebind(prepared.queries_);draws.publish_rebind(prepared.draws_);
    prepared.consumed_=true;
    return true;
}
bool SourceLensPresentationHandoffTu15::commit(SourceLensQueryFrameTu15 &queries,
    SourceLensDrawFrameTu15 &draws,Prepared &&prepared) noexcept {
    if(!ready(queries,draws,prepared))return false;
    queries.publish_rebind(prepared.queries_);draws.publish_rebind(prepared.draws_);
    prepared.consumed_=true;
    return true;
}
}
