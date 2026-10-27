#include "halo2/original_matchmaking_fonts.hpp"
#include "halo2/frontend.hpp"
#include <array>
#include <openssl/evp.h>
#include <span>
#include <string_view>
#include <vector>
#include <zlib.h>

namespace halo2 {
namespace {
struct OriginalFontPayload {
    unsigned selector;
    std::string_view name, sha256;
    std::size_t bytes, compressed_bytes;
    std::span<const std::string_view> chunks;
};
#include "ui/original_matchmaking_fonts.inc"

std::vector<std::byte> decode_payload(const OriginalFontPayload& source) {
    // Four authenticated v1 files, each below441KiB. Neither a pathname nor a
    // selected runtime cache can replace these finite original raster owners.
    if(source.bytes < 0x40400 || source.bytes > 441000 ||
       source.compressed_bytes > 81000)
        throw FormatError("Invalid original matchmaking font extent");
    std::vector<unsigned char> compressed;
    compressed.reserve(source.compressed_bytes);
    const auto digit=[](char c)->unsigned {
        if(c>='0' && c<='9')return unsigned(c-'0');
        if(c>='a' && c<='f')return unsigned(c-'a'+10);
        throw FormatError("Invalid original matchmaking font encoding");
    };
    for(const auto chunk:source.chunks) {
        if(chunk.size()%2 || chunk.size()>4096 ||
           chunk.size()/2>source.compressed_bytes-compressed.size())
            throw FormatError("Invalid original matchmaking font chunk");
        for(std::size_t i=0;i<chunk.size();i+=2)
            compressed.push_back(static_cast<unsigned char>((digit(chunk[i])<<4)|digit(chunk[i+1])));
    }
    if(compressed.size()!=source.compressed_bytes)
        throw FormatError("Truncated original matchmaking font");
    std::vector<std::byte> bytes(source.bytes);
    uLongf size=bytes.size();
    if(uncompress(reinterpret_cast<Bytef*>(bytes.data()),&size,
                  compressed.data(),compressed.size())!=Z_OK || size!=bytes.size())
        throw FormatError("Invalid original matchmaking font compression");
    std::array<unsigned char,32> hash{};
    unsigned hash_size{};
    if(EVP_Digest(bytes.data(),bytes.size(),hash.data(),&hash_size,EVP_sha256(),nullptr)!=1 ||
       hash_size!=hash.size() || source.sha256.size()!=hash.size()*2)
        throw FormatError("Cannot authenticate original matchmaking font");
    constexpr char hex[]="0123456789abcdef";
    for(std::size_t i=0;i<hash.size();++i)
        if(source.sha256[i*2]!=hex[hash[i]>>4] || source.sha256[i*2+1]!=hex[hash[i]&15])
            throw FormatError("Original matchmaking font payload changed");
    return bytes;
}
} // namespace

bool original_matchmaking_font_selector(unsigned selector) noexcept {
    return selector==1 || selector==2 || selector==4 || selector==6 || selector==9;
}

RetailFont load_original_matchmaking_font(unsigned selector) {
    if(selector==6)selector=4; // Both original font-table slots own conduit-13.
    for(const auto& source:original_font_payloads)
        if(source.selector==selector)
            // The existing decoder retains the complete original glyph
            // lookup, logical metrics, kerning and A4R4G4B4 pixel values. No
            // MCC glyph is resized, filtered or substituted in this bank.
            return decode_retail_font(decode_payload(source));
    throw FormatError("No original matchmaking font for selector");
}
} // namespace halo2
