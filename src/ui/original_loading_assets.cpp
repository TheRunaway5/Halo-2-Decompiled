#include "halo2/original_loading_assets.hpp"
#include "halo2/cache.hpp"
#include <openssl/evp.h>
#include <zlib.h>

namespace halo2 {
namespace {
struct OriginalLoadingOwner {std::string_view name;std::uint32_t datum;unsigned pages;};
struct OriginalLoadingPage {
  unsigned bank,index,width,height,format,flags,additional_mips;
  std::size_t offset,bytes;
};
#include "ui/original_loading_assets.inc"
}
const std::array<OriginalLoadingBitmap,5>& original_loading_bitmaps() {
  static const auto result=[] {
    static_assert(original_loading_bytes<=2*1024*1024 && original_loading_compressed_bytes<=2*1024*1024);
    std::vector<unsigned char> encoded;encoded.reserve(original_loading_compressed_bytes);
    const auto digit=[](char c)->unsigned {
      if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a'+10);
      throw FormatError("Invalid original loading source encoding");
    };
    for(const auto chunk:original_loading_chunks) {
      if(chunk.size()>16384 || chunk.size()%2 || chunk.size()/2>original_loading_compressed_bytes-encoded.size())
        throw FormatError("Original loading source chunk exceeds its extent");
      for(std::size_t i=0;i<chunk.size();i+=2)encoded.push_back(static_cast<unsigned char>((digit(chunk[i])<<4)|digit(chunk[i+1])));
    }
    if(encoded.size()!=original_loading_compressed_bytes)throw FormatError("Incomplete original loading source bank");
    std::vector<std::byte> raw(original_loading_bytes);uLongf size=raw.size();
    if(uncompress(reinterpret_cast<Bytef*>(raw.data()),&size,encoded.data(),encoded.size())!=Z_OK || size!=raw.size())
      throw FormatError("Original loading source bank failed to decode");
    std::array<unsigned char,32> hash{};unsigned hash_size{};
    if(EVP_Digest(raw.data(),raw.size(),hash.data(),&hash_size,EVP_sha256(),nullptr)!=1 || hash_size!=hash.size())
      throw FormatError("Original loading source digest unavailable");
    constexpr char hex[]="0123456789abcdef";
    for(std::size_t i=0;i<hash.size();++i)
      if(original_loading_sha256[i*2]!=hex[hash[i]>>4] || original_loading_sha256[i*2+1]!=hex[hash[i]&15])
        throw FormatError("Original loading source bank changed");
    std::array<OriginalLoadingBitmap,5> banks;
    for(unsigned bank=0;bank<banks.size();++bank) {
      banks[bank].name=original_loading_owners[bank].name;
      banks[bank].source_datum=original_loading_owners[bank].datum;
    }
    for(const auto &page:original_loading_pages) {
      if(page.bank>=banks.size() || page.index!=banks[page.bank].frames.size() || !page.width || !page.height ||
          page.width>2048 || page.height>1024 || page.offset>raw.size() || page.bytes>raw.size()-page.offset ||
          (page.format!=1 && page.format!=14 && page.format!=15))throw FormatError("Original loading page descriptor changed");
      const auto bytes=std::span<const std::byte>(raw).subspan(page.offset,page.bytes);
      const auto mips=retail_mip_count(page.width,page.height,1,page.format,(page.flags&16)!=0,page.additional_mips);
      auto texture=prepare_texture(Bytes(bytes),page.width,page.height,page.format,(page.flags&8)!=0,mips);
      texture.pixels=TexturePixels(std::span<const std::byte>(static_cast<const Texture&>(texture).pixels.data(),texture.pixels.size()));
      banks[page.bank].frames.push_back(std::move(texture));
    }
    for(unsigned bank=0;bank<banks.size();++bank)
      if(banks[bank].frames.size()!=original_loading_owners[bank].pages)throw FormatError("Incomplete original loading bitmap pages");
    return banks;
  }();
  return result;
}
} // namespace halo2
