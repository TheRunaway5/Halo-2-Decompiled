#include "runtime/source_authority_archive.hpp"

namespace halo2 {
SourceMatchAuthority::Checkpoint decode_source_authority(std::span<const std::byte> bytes,SourceAuthorityCodecLimits limits){
    using namespace authority_archive;
    auto values=parse_authority_archive(bytes,{limits.encoded_bytes,limits.decoded_bytes,limits.allocated_bytes,
        limits.container_elements,sizeof(SourceMatchAuthority::Checkpoint)});
    if(!values||digest(values->raw())!=values->expected_hash())throw FormatError("Invalid, corrupt or over-budget authority archive");
    Reader reader(std::move(values));SourceMatchAuthority::Checkpoint result;reader.add(result);
    if(!reader.complete())throw FormatError("Authority archive has trailing state");source_only(result);return result;
}
}
