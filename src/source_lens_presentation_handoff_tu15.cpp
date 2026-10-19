#include "halo2/source_lens_presentation_handoff_tu15.hpp"

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
bool SourceLensPresentationHandoffTu15::ready(const SourceLensQueryFrameTu15 &queries,
    const SourceLensDrawFrameTu15 &draws,const Prepared &prepared,
    const SourceRuntimeMemory &incoming) noexcept {
    return queries.rebind_ready(prepared.queries_,incoming)&&
        draws.rebind_ready(prepared.draws_,incoming)&&
        queries.renderer_owner()==draws.renderer_owner();
}
bool SourceLensPresentationHandoffTu15::commit(SourceLensQueryFrameTu15 &queries,
    SourceLensDrawFrameTu15 &draws,Prepared &&prepared,const SourceRuntimeMemory &incoming) noexcept {
    if(!ready(queries,draws,prepared,incoming))return false;
    queries.publish_rebind(prepared.queries_);draws.publish_rebind(prepared.draws_);
    return true;
}
}
