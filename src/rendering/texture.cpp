#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <utility>

namespace halo2 {
namespace {
std::size_t mip_size(std::uint32_t width, std::uint32_t height, std::uint16_t format) {
    if (format >= 14 && format <= 16)
        return std::size_t((width + 3) / 4) * ((height + 3) / 4) * (format == 14 ? 8 : 16);
    const bool byte = format <= 2;
    const bool word = format == 3 || format == 6 || format == 8 || format == 9 || format == 22 || format == 23;
    if (!byte && !word && format != 10 && format != 11)
        throw FormatError("unsupported diffuse bitmap format " + std::to_string(format));
    return std::size_t(width) * height * (byte ? 1 : word ? 2 : 4);
}
Texture prepare_level(const Bytes& raw, std::uint32_t width, std::uint32_t height,
                      std::uint16_t format, bool swizzled) {
    if (!width || !height || width > 4096 || height > 4096) throw FormatError("unsupported bitmap dimensions");
    Texture texture{width, height, format, {}, {}};
    if (format >= 14 && format <= 16) {
        if (swizzled) throw FormatError("swizzled compressed bitmap is unsupported");
        const auto size = std::size_t((width + 3) / 4) * ((height + 3) / 4) * (format == 14 ? 8 : 16);
        const auto data = raw.slice(0, size);
        texture.pixels.assign(data.begin(), data.end());
        return texture;
    }
    const bool byte_format = format <= 2;
    const bool word_format = format == 3 || format == 6 || format == 8 || format == 9 || format == 22 || format == 23;
    if (!byte_format && !word_format && format != 10 && format != 11) throw FormatError("unsupported diffuse bitmap format " + std::to_string(format));
    const std::size_t bytes_per_pixel = byte_format ? 1 : word_format ? 2 : 4;
    const auto source = raw.slice(0, std::size_t(width) * height * bytes_per_pixel);
    if (swizzled && (!std::has_single_bit(width) || !std::has_single_bit(height)))
        throw FormatError("swizzled bitmap dimensions must be powers of two");
    texture.format = 11;
    texture.pixels.resize(std::size_t(width) * height * 4);
    // Validate once, then retain this decoder's private writable view. Calling
    // the copy-on-write accessor for every channel adds millions of ownership
    // checks to large uncompressed MCC light/material images.
    auto *pixels = texture.pixels.data();
    // A separate linear RGBA path lets the compiler vectorize the channel
    // shuffle. The general Morton/format branches otherwise keep this common
    // uncompressed MCC case in the scalar inner loop.
    if (!swizzled && (format == 10 || format == 11)) {
        for (std::size_t at = 0; at < source.size(); at += 4) {
            if constexpr (std::endian::native == std::endian::little) {
                // memcpy admits unaligned source data without aliasing UB.
                // A word shuffle also stays cheap without SIMD extensions.
                std::uint32_t value;
                std::memcpy(&value, source.data() + at, sizeof value);
                value = (value & 0xff00ff00u) | ((value & 0xffu) << 16) |
                        ((value >> 16) & 0xffu);
                if (format == 10) value |= 0xff000000u;
                std::memcpy(pixels + at, &value, sizeof value);
                continue;
            }
            pixels[at] = source[at + 2];
            pixels[at + 1] = source[at + 1];
            pixels[at + 2] = source[at];
            pixels[at + 3] = format == 11 ? source[at + 3] : std::byte{255};
        }
        return texture;
    }
    auto morton = [&](std::uint32_t x, std::uint32_t y) {
        std::size_t address = 0, destination_bit = 1;
        for (std::uint32_t bit = 1; bit < width || bit < height; bit <<= 1) {
            if (bit < width) { if (x & bit) address |= destination_bit; destination_bit <<= 1; }
            if (bit < height) { if (y & bit) address |= destination_bit; destination_bit <<= 1; }
        }
        return address;
    };
    for (std::uint32_t y = 0; y < height; ++y) for (std::uint32_t x = 0; x < width; ++x) {
        const auto src = (swizzled ? morton(x, y) : std::size_t(y) * width + x) * bytes_per_pixel;
        const auto dst = (std::size_t(y) * width + x) * 4;
        auto byte = [&](std::size_t offset) { return std::to_integer<unsigned>(source[offset]); };
        auto word = [&] { return byte(src) | (byte(src + 1) << 8); };
        unsigned r = 255, g = 255, b = 255, a = 255;
        if (format == 0) a = byte(src);
        else if (format == 1) r = g = b = byte(src);
        else if (format == 2) r = g = b = a = byte(src);
        else if (format == 3) { r = g = b = byte(src); a = byte(src + 1); }
        else if (format == 22 || format == 23) {
            // Retail 450768 maps both cache V8U8/G8B8 to NV2A G8B8
            // (0x28; linear 0x17), with RGBA swizzle U,V,U,V.
            // The image fetch is unsigned; BUMPENVMAP applies the signed
            // interpretation after filtering, independently of this decode.
            r = b = byte(src); g = a = byte(src + 1);
        }
        else if (format == 6) {
            const auto p = word();
            const unsigned rv=(p>>11)&31,gv=(p>>5)&63,bv=p&31;
            r=(rv<<3)|(rv>>2);g=(gv<<2)|(gv>>4);b=(bv<<3)|(bv>>2);
        } else if (format == 8) {
            const auto p = word();
            const unsigned rv=(p>>10)&31,gv=(p>>5)&31,bv=p&31;
            r=(rv<<3)|(rv>>2);g=(gv<<3)|(gv>>2);b=(bv<<3)|(bv>>2);a=(p>>15)*255;
        } else if (format == 9) {
            const auto p = word();
            r = ((p >> 8) & 15) * 17; g = ((p >> 4) & 15) * 17; b = (p & 15) * 17; a = (p >> 12) * 17;
        } else { b = byte(src); g = byte(src + 1); r = byte(src + 2); if (format == 11) a = byte(src + 3); }
        pixels[dst] = std::byte(r); pixels[dst + 1] = std::byte(g);
        pixels[dst + 2] = std::byte(b); pixels[dst + 3] = std::byte(a);
    }
    return texture;
}
}

Texture prepare_texture(const Bytes& raw, std::uint32_t width, std::uint32_t height,
                        std::uint16_t format, bool swizzled, unsigned additional_mips) {
    if (!width || !height || width > 4096 || height > 4096)
        throw FormatError("unsupported bitmap dimensions");
    if (additional_mips >= static_cast<unsigned>(std::bit_width(std::max(width, height))))
        throw FormatError("bitmap mip count exceeds dimensions");
    Texture result{width, height, format, {}, {}};
    std::size_t source_offset = 0;
    for (unsigned level = 0; level <= additional_mips; ++level) {
        const auto size = mip_size(width, height, format);
        const auto source = raw.slice(source_offset, size);
        auto mip = prepare_level(Bytes(source), width, height, format, swizzled);
        result.format = mip.format;
        result.mips.push_back({width, height, result.pixels.size(), mip.pixels.size()});
        // The base level already owns exactly the completed bytes. Transfer
        // its allocation instead of allocating and copying that entire image
        // again (including large single-level UI and prompt atlases).
        if (!level) result.pixels = std::move(mip.pixels);
        else result.pixels.insert(result.pixels.end(), mip.pixels.begin(), mip.pixels.end());
        source_offset += size;
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
    }
    return result;
}
Texture prepare_palettized_texture(const Bytes& raw, std::uint32_t width, std::uint32_t height,
    bool swizzled, std::span<const std::byte> palette, unsigned additional_mips) {
    if (palette.size() != 1024) throw FormatError("P8 texture requires a 256-color palette");
    auto result = prepare_texture(raw,width,height,1,swizzled,additional_mips);
    for(std::size_t i=0;i<result.pixels.size();i+=4) {
        const auto index=std::to_integer<unsigned>(result.pixels[i])*4;
        result.pixels[i]=palette[index+2]; result.pixels[i+1]=palette[index+1];
        result.pixels[i+2]=palette[index]; result.pixels[i+3]=palette[index+3];
    }
    return result;
}
Texture prepare_volume_texture(const Bytes& raw,std::uint32_t width,
    std::uint32_t height,std::uint32_t depth,std::uint16_t format,
    bool swizzled,unsigned additional_mips) {
    if(!width || !height || !depth || width>4096 || height>4096 || depth>4096 ||
       std::uint64_t(width)*height*depth>16777216)
        throw FormatError("unsupported volume dimensions");
    // Original rasterizer\distance_attenuation is I8 (format2); authored
    // object-lighting volumes use A8L8 (format3). Both use XYZ swizzling.
    if(format!=2 && format!=3)throw FormatError("unsupported volume format");
    const unsigned bytes_per_texel=format==2?1:2;
    if(additional_mips>=std::bit_width(std::max({width,height,depth})))
        throw FormatError("volume mip count exceeds dimensions");
    Texture result{width,height,11,{},{},1,depth};std::size_t offset=0;
    for(unsigned level=0;level<=additional_mips;++level) {
        const auto count=std::size_t(width)*height*depth;
        const auto source=raw.slice(offset,count*bytes_per_texel);
        if(swizzled && (!std::has_single_bit(width) || !std::has_single_bit(height) || !std::has_single_bit(depth)))
            throw FormatError("swizzled volume dimensions must be powers of two");
        const auto output_offset=result.pixels.size();result.pixels.resize(output_offset+count*4);
        for(std::uint32_t z=0;z<depth;++z)for(std::uint32_t y=0;y<height;++y)for(std::uint32_t x=0;x<width;++x) {
            const auto linear=(std::size_t(z)*height+y)*width+x;
            std::size_t address=linear;
            if(swizzled) {
                address=0;std::size_t destination_bit=1;
                for(std::uint32_t bit=1;bit<width || bit<height || bit<depth;bit<<=1) {
                    for(const auto &[coordinate,dimension]:std::array{
                        std::pair{x,width},std::pair{y,height},std::pair{z,depth}}) {
                        if(bit>=dimension)continue;
                        if(coordinate&bit)address|=destination_bit;
                        destination_bit<<=1;
                    }
                }
            }
            const auto out=output_offset+linear*4;
            result.pixels[out]=result.pixels[out+1]=result.pixels[out+2]=source[address*bytes_per_texel];
            result.pixels[out+3]=source[address*bytes_per_texel+(format==2?0:1)];
        }
        result.mips.push_back({width,height,output_offset,count*4,depth});offset+=count*bytes_per_texel;
        width=std::max(1u,width/2);height=std::max(1u,height/2);depth=std::max(1u,depth/2);
    }
    return result;
}
Texture prepare_signed_texture(Texture texture,std::uint32_t color_sign) {
    if(!color_sign)return texture;
    if(color_sign&0x0fffffffu)throw FormatError("unsupported retail signed texture mask");
    if(texture.format!=11 || texture.pixels.size()%4)
        throw FormatError("signed texture requires decoded RGBA8 texels");
    std::vector<std::byte> decoded(texture.pixels.size()*4);
    const auto& source=std::as_const(texture).pixels;
    for(std::size_t at=0;at<texture.pixels.size();++at) {
        const unsigned byte=std::to_integer<unsigned>(source[at]);
        const bool signed_channel=color_sign&(0x80000000u>>(at%4));
        // NVIDIA NV_texture_shader's 8-bit signed internal format uses
        // fixed-point byte/128: -128 is -1 and +127 is 0.9921875.
        // This is distinct from PS signed DOT mapping (byte/127).
        const float value=signed_channel?float(byte>=128?int(byte)-256:int(byte))/128.f:
            float(byte)/255.f;
        std::memcpy(decoded.data()+at*4,&value,sizeof(value));
    }
    for(auto &mip:texture.mips) {mip.offset*=4;mip.size*=4;}
    texture.pixels=std::move(decoded);texture.format=native_rgba32f;
    // The new output buffer has never escaped this producer. Decoder-local
    // writes are complete, so subsequent material/pass copies can share it.
    // A later mutable access still detaches through TexturePixels::write().
    texture.pixels.publish();
    return texture;
}
}
