#include "runtime/source_authority_archive.hpp"
#include "halo2/source_authority_delta.hpp"

namespace halo2 {
namespace {
SourceMatchAuthority::Checkpoint materialize_checked(std::unique_ptr<ParsedAuthorityArchive> values,SourceAuthorityCodecLimits limits,SourceAuthorityCodecStats *stats){
    using namespace authority_archive;
    Reader reader(std::move(values),limits.staged_bytes);SourceMatchAuthority::Checkpoint result;reader.add(result);
    if(!reader.complete())throw FormatError("Authority archive has trailing state");source_only(result);
    if(stats){const auto measured=reader.stats();*stats={measured.raw_bytes,measured.metadata_bytes,measured.native_claimed,measured.working_peak,reader.staged_bytes()};}
    return result;
}
SourceMatchAuthority::Checkpoint materialize(std::unique_ptr<ParsedAuthorityArchive> values,SourceAuthorityCodecLimits limits,SourceAuthorityCodecStats *stats=nullptr){
    using namespace authority_archive;
    if(!values||digest(values->raw())!=values->expected_hash())throw FormatError("Invalid, corrupt or over-budget authority archive");
    return materialize_checked(std::move(values),limits,stats);
}
}
SourceMatchAuthority::Checkpoint decode_source_authority(std::span<const std::byte> bytes,SourceAuthorityCodecLimits limits){
    using namespace authority_archive;
    auto values=parse_authority_archive(bytes,{limits.encoded_bytes,limits.decoded_bytes,limits.allocated_bytes,
        limits.container_elements,sizeof(SourceMatchAuthority::Checkpoint),limits.working_bytes});
    return materialize(std::move(values),limits);
}
SourceMatchAuthority::Checkpoint decode_source_authority_raw(std::span<const std::byte> raw,
    std::array<std::byte,32> hash,SourceAuthorityCodecLimits limits,SourceAuthorityCodecStats *stats){
    return materialize(parse_authority_archive_raw(raw,hash,{limits.encoded_bytes,limits.decoded_bytes,limits.allocated_bytes,
        limits.container_elements,sizeof(SourceMatchAuthority::Checkpoint),limits.working_bytes}),limits,stats);
}
SourceMatchAuthority::Checkpoint decode_source_authority_image(const AuthorityArchiveImage& image,
    SourceAuthorityCodecLimits limits,SourceAuthorityCodecStats *stats){
    auto values=parse_authority_archive_image(image,{limits.encoded_bytes,limits.decoded_bytes,limits.allocated_bytes,
        limits.container_elements,sizeof(SourceMatchAuthority::Checkpoint),limits.working_bytes});
    // from_raw computes the digest; read/apply_delta verify it before minting
    // the immutable image. The parser retains that exact shared byte owner.
    if(!values||values->expected_hash()!=image.digest())throw FormatError("Invalid, corrupt or over-budget authority archive");
    return materialize_checked(std::move(values),limits,stats);
}
}
