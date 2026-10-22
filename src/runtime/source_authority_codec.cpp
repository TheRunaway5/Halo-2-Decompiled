#include "runtime/source_authority_archive.hpp"

namespace halo2 {
std::vector<std::byte> encode_source_authority(const SourceMatchAuthority::Checkpoint &saved,SourceAuthorityCodecLimits limits){
    using namespace authority_archive;source_only(saved);Writer body(limits);body.add(saved);
    if(body.bytes.size()>std::numeric_limits<uLong>::max())throw FormatError("Authority archive exceeds compression extent");
    const auto bound=compressBound(uLong(body.bytes.size()));
    if(limits.encoded_bytes<64)throw FormatError("Authority archive exceeds encoded byte budget");
    const auto capacity=std::min<std::uint64_t>(bound,limits.encoded_bytes-64);
    std::vector<std::byte> compressed(static_cast<std::size_t>(capacity));uLongf n=uLongf(capacity);
    if(compress2(reinterpret_cast<Bytef *>(compressed.data()),&n,reinterpret_cast<const Bytef *>(body.bytes.data()),uLong(body.bytes.size()),Z_BEST_SPEED)!=Z_OK)
        throw FormatError("Cannot compress authority archive");compressed.resize(n);
    if(n>limits.encoded_bytes-64)throw FormatError("Authority archive exceeds encoded byte budget");
    Writer envelope({limits.encoded_bytes,limits.encoded_bytes,limits.allocated_bytes,limits.container_elements});
    envelope.raw(magic);envelope.put(1,4);envelope.put(0,4);envelope.put(body.bytes.size(),8);envelope.put(n,8);envelope.raw(digest(body.bytes));envelope.raw(compressed);
    return std::move(envelope.bytes);
}
}
