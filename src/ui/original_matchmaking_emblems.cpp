#include "halo2/original_matchmaking_emblems.hpp"
#include "halo2/cache.hpp"
#include "halo2/texture.hpp"
#include <array>
#include <openssl/evp.h>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <zlib.h>

namespace halo2 {
namespace {
struct OriginalEmblemPage {
    unsigned stage, index, width, height, format, flags, additional_mips;
    std::size_t offset, bytes;
};
#include "ui/original_matchmaking_emblems.inc"

const std::array<std::vector<Texture>,2>& source_banks() {
    static const auto banks=[] {
        static_assert(original_emblem_bytes==196608);
        static_assert(original_emblem_compressed_bytes<=16000);
        std::vector<unsigned char> compressed;
        compressed.reserve(original_emblem_compressed_bytes);
        const auto digit=[](char c)->unsigned {
            if(c>='0' && c<='9')return unsigned(c-'0');
            if(c>='a' && c<='f')return unsigned(c-'a'+10);
            throw FormatError("Invalid original emblem payload encoding");
        };
        for(const auto chunk:original_emblem_chunks) {
            if(chunk.size()%2 || chunk.size()>4096 ||
               chunk.size()/2>original_emblem_compressed_bytes-compressed.size())
                throw FormatError("Invalid original emblem payload chunk");
            for(std::size_t i=0;i<chunk.size();i+=2)
                compressed.push_back(static_cast<unsigned char>((digit(chunk[i])<<4)|digit(chunk[i+1])));
        }
        if(compressed.size()!=original_emblem_compressed_bytes)
            throw FormatError("Truncated original emblem payload");
        std::vector<std::byte> bytes(original_emblem_bytes);
        uLongf size=bytes.size();
        if(uncompress(reinterpret_cast<Bytef*>(bytes.data()),&size,
                      compressed.data(),compressed.size())!=Z_OK || size!=bytes.size())
            throw FormatError("Invalid original emblem compression");
        std::array<unsigned char,32> hash{};unsigned hash_size{};
        if(EVP_Digest(bytes.data(),bytes.size(),hash.data(),&hash_size,EVP_sha256(),nullptr)!=1 ||
           hash_size!=hash.size() || original_emblem_sha256.size()!=hash.size()*2)
            throw FormatError("Cannot authenticate original emblem payload");
        constexpr char hex[]="0123456789abcdef";
        for(std::size_t i=0;i<hash.size();++i)
            if(original_emblem_sha256[i*2]!=hex[hash[i]>>4] || original_emblem_sha256[i*2+1]!=hex[hash[i]&15])
                throw FormatError("Original emblem payload changed");
        std::array<std::vector<Texture>,2> result;
        for(const auto& page:original_emblem_pages) {
            if(page.stage>=2 || page.index!=result[page.stage].size() ||
               page.width!=32 || page.height!=32 || (page.format!=8 && page.format!=9) ||
               page.flags!=4617 || page.additional_mips!=0 || page.bytes!=2048 ||
               page.offset>bytes.size() || page.bytes>bytes.size()-page.offset)
                throw FormatError("Original emblem page descriptor changed");
            auto decoded=prepare_texture(Bytes(std::span<const std::byte>(bytes).subspan(page.offset,page.bytes)),
                page.width,page.height,page.format,(page.flags&8)!=0,
                retail_mip_count(page.width,page.height,1,page.format,(page.flags&16)!=0,page.additional_mips));
            // Decoder-local writers have expired. The public const-span
            // constructor creates an immutable independent resident owner;
            // private Cache publication is not part of this provider API.
            decoded.pixels=TexturePixels(std::span<const std::byte>(
                static_cast<const Texture&>(decoded).pixels.data(),decoded.pixels.size()));
            result[page.stage].push_back(std::move(decoded));
        }
        if(result[0].size()!=32 || result[1].size()!=64)
            throw FormatError("Original emblem page bank is incomplete");
        return result;
    }();
    return banks;
}
} // namespace

bool original_matchmaking_emblem_private(std::string_view name) noexcept {
    return name==original_matchmaking_emblem_private_name;
}

Material make_original_matchmaking_emblem(Material source) {
    if(source.name!=original_matchmaking_emblem_source_name || !source.ui_emblem_pass ||
       !source.forward_pass || !source.hud_pass || source.forward_pass_index!=0 ||
       source.passes.size()!=1 || source.passes.front().implementation!=0 ||
       source.passes.front().name!="shaders\\shader_passes\\hud\\waypoint_emblem" ||
       source.passes.front().textures.size()!=2 ||
       source.animated_textures[0].size()!=32 || source.animated_textures[1].size()!=64)
        throw FormatError("Original emblem pixels need their admitted two-stage UI owner");
    std::array<bool,2> stages{};
    for(const auto& binding:source.passes.front().textures) {
        if(binding.external || binding.stage>=2 || stages[binding.stage])
            throw FormatError("Original emblem texture stages changed");
        stages[binding.stage]=true;
    }
    // These exact-name joins are authenticated by the selected mainmenu
    // identifier bank (mcc_frontend_string_ids.inc). This private material
    // calls original14CCF0/22ACE7 inputs, whose identifiers are Xbox ones.
    // Leaving imported606..60B/5D7 references here feeds their default1 branch
    // and loses the real appearance's selected pages, flips and opacity.
    constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,7> inputs{{
        {0x17000606,0x170005fa},{0x17000607,0x170005fb},
        {0x18000608,0x180005fc},{0x1d000609,0x1d0005fd},
        {0x1d00060a,0x1d0005fe},{0x0d00060b,0x0d0005ff},
        {0x0c0005d7,0x0c0005cb},
    }};
    const auto original_input=[&](std::uint32_t value) {
        for(const auto& [mcc,original]:inputs)if(value==mcc)return original;
        return value;
    };
    for(auto& overlay:source.animation.overlays) {
        overlay.input=original_input(overlay.input);
        overlay.range=original_input(overlay.range);
    }
    const auto& banks=source_banks();
    for(unsigned stage=0;stage<2;++stage) {
        if(!stages[stage] || source.texture_frames[stage]>=banks[stage].size())
            throw FormatError("Original emblem page selection changed");
        source.animated_textures[stage]=banks[stage];
    }
    source.diffuse=banks[0][source.texture_frames[0]];
    source.detail=banks[1][source.texture_frames[1]];
    source.name=original_matchmaking_emblem_private_name;
    return source;
}
} // namespace halo2
