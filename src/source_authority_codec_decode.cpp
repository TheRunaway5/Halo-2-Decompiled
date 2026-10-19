#include "source_authority_archive.hpp"

namespace halo2 {
SourceMatchAuthority::Checkpoint decode_source_authority(std::span<const std::byte> bytes,SourceAuthorityCodecLimits limits){
    using namespace authority_archive;
    if(bytes.size()<64||bytes.size()>limits.encoded_bytes)throw FormatError("Authority archive envelope exceeds admitted extent");
    Reader envelope(bytes,limits);if(!std::ranges::equal(envelope.raw(8),magic)||envelope.get(4)!=1||envelope.get(4)!=0)
        throw FormatError("Unknown authority archive envelope");
    const auto raw_size=envelope.get(8),compressed_size=envelope.get(8);const auto expected=envelope.raw(32);
    if(raw_size>limits.decoded_bytes||raw_size>std::numeric_limits<uLongf>::max()||compressed_size!=bytes.size()-64||
        compressed_size>std::numeric_limits<uLong>::max())throw FormatError("Authority archive compression extent exceeds admitted budget");
    std::vector<std::byte> raw(static_cast<std::size_t>(raw_size));uLongf n=uLongf(raw_size);uLong source=uLong(compressed_size);
    if(uncompress2(reinterpret_cast<Bytef *>(raw.data()),&n,reinterpret_cast<const Bytef *>(bytes.data()+64),&source)!=Z_OK||n!=raw_size||source!=compressed_size||
        !std::ranges::equal(digest(raw),expected))throw FormatError("Authority archive has corrupt or trailing compressed data");
    Reader reader(raw,limits);SourceMatchAuthority::Checkpoint result;reader.add(result);
    if(!reader.complete())throw FormatError("Authority archive has trailing state");source_only(result);return result;
}
}
