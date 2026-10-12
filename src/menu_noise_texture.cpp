/*
 * Title BC1/BC2/BC3 decoding follows Xemu 0.8.136's pgraph/s3tc.c, commit
 * fc24584ce88f0915ad7f04775bb7712c2e3f49ee, by Wilhelm Kovatch.
 * Copyright (c) 2020 Wilhelm Kovatch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "halo2/atmosphere.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
Texture prepare_xemu_bc1_texture(const Texture &source) {
    if(source.format!=14 || source.layers!=1 || source.depth || source.mips.empty())
        throw FormatError("original BC1 expansion requires a 2D mip chain");
    Texture result{source.width,source.height,11,{},{}};
    const Bytes bytes(source.pixels);
    for(const auto &mip:source.mips) {
        const auto blocks_x=(mip.width+3)/4,blocks_y=(mip.height+3)/4;
        if(mip.size!=std::size_t(blocks_x)*blocks_y*8)
            throw FormatError("original BC1 mip size differs");
        bytes.require(mip.offset,mip.size);
        const auto output=result.pixels.size(),size=std::size_t(mip.width)*mip.height*4;
        result.mips.push_back({mip.width,mip.height,output,size});result.pixels.resize(output+size);
        for(unsigned by=0;by<blocks_y;++by)for(unsigned bx=0;bx<blocks_x;++bx) {
            const auto at=mip.offset+(std::size_t(by)*blocks_x+bx)*8;
            const auto first=bytes.u16(at),second=bytes.u16(at+2);
            const auto indices=bytes.u32(at+4);
            std::array<std::array<unsigned,4>,4> colors{};
            for(unsigned i=0;i<2;++i) {
                const auto word=i?second:first;
                colors[i]={unsigned(word>>11)*255/31,unsigned((word>>5)&63)*255/63,unsigned(word&31)*255/31,255};
            }
            if(first<=second) {
                for(unsigned c=0;c<3;++c)colors[2][c]=(colors[0][c]+colors[1][c])/2;
                colors[2][3]=255;
            }else {
                for(unsigned c=0;c<3;++c) {
                    colors[2][c]=(2*colors[0][c]+colors[1][c])/3;
                    colors[3][c]=(colors[0][c]+2*colors[1][c])/3;
                }
                colors[2][3]=colors[3][3]=255;
            }
            for(unsigned y=0;y<4 && by*4+y<mip.height;++y)
                for(unsigned x=0;x<4 && bx*4+x<mip.width;++x) {
                    const auto &color=colors[(indices>>((y*4+x)*2))&3];
                    const auto pixel=output+((std::size_t(by*4+y)*mip.width)+bx*4+x)*4;
                    for(unsigned c=0;c<4;++c)result.pixels[pixel+c]=std::byte(color[c]);
                }
        }
    }
    return result;
}
// Pinned s3tc.c decompress_dxt3_block always disables BC1's transparent RGB
// branch. Its explicit alpha nibbles are independent of the color selectors.
Texture prepare_xemu_bc2_texture(const Texture &source) {
  if(source.format!=15 || source.layers!=1 || source.depth || source.mips.empty())
    throw FormatError("original BC2 expansion requires a 2D mip chain");
  Texture result{source.width,source.height,11,{},{}};
  const Bytes bytes(source.pixels);
  for(const auto &mip:source.mips) {
    if(!mip.width || !mip.height || mip.width>4096 || mip.height>4096 || mip.depth!=1)
      throw FormatError("invalid original BC2 mip dimensions");
    const auto blocks_x=(mip.width+3)/4,blocks_y=(mip.height+3)/4;
    if(mip.size!=std::size_t(blocks_x)*blocks_y*16)
      throw FormatError("original BC2 mip size differs");
    bytes.require(mip.offset,mip.size);
    const auto output=result.pixels.size(),size=std::size_t(mip.width)*mip.height*4;
    result.mips.push_back({mip.width,mip.height,output,size});result.pixels.resize(output+size);
    for(unsigned by=0;by<blocks_y;++by)for(unsigned bx=0;bx<blocks_x;++bx) {
      const auto at=mip.offset+(std::size_t(by)*blocks_x+bx)*16;
      const auto first=bytes.u16(at+8),second=bytes.u16(at+10);
      const auto indices=bytes.u32(at+12);
      const std::uint64_t alpha=bytes.u32(at)|(std::uint64_t(bytes.u32(at+4))<<32);
      std::array<std::array<unsigned,3>,4> colors{};
      for(unsigned i=0;i<2;++i) {
        const auto word=i?second:first;
        colors[i]={unsigned(word>>11)*255/31,unsigned((word>>5)&63)*255/63,unsigned(word&31)*255/31};
      }
      for(unsigned channel=0;channel<3;++channel) {
        colors[2][channel]=(2*colors[0][channel]+colors[1][channel])/3;
        colors[3][channel]=(colors[0][channel]+2*colors[1][channel])/3;
      }
      for(unsigned y=0;y<4 && by*4+y<mip.height;++y)
        for(unsigned x=0;x<4 && bx*4+x<mip.width;++x) {
          const auto index=y*4+x;
          const auto &color=colors[(indices>>(index*2))&3];
          const auto pixel=output+(std::size_t(by*4+y)*mip.width+bx*4+x)*4;
          for(unsigned channel=0;channel<3;++channel)result.pixels[pixel+channel]=std::byte(color[channel]);
          result.pixels[pixel+3]=std::byte(((alpha>>(index*4))&15)*17);
        }
    }
  }
  return result;
}
// Same pinned MIT s3tc.c DXT5 kernel formerly private to frontend_content.cpp.
Texture prepare_xemu_bc3_texture(const Texture &source) {
  if (source.format != 16 || source.layers != 1 || source.depth || source.mips.empty())
    throw FormatError("UI BC3 expansion requires a 2D mip chain");
  Texture result{source.width, source.height, 11, {}, {}};
  const Bytes bytes(source.pixels);
  for (const auto &mip : source.mips) {
    if (!mip.width || !mip.height || mip.width > 4096 || mip.height > 4096)
      throw FormatError("invalid UI BC3 mip dimensions");
    const auto blocks_x = (mip.width + 3) / 4, blocks_y = (mip.height + 3) / 4;
    if (mip.size != std::size_t(blocks_x) * blocks_y * 16)
      throw FormatError("UI BC3 mip size differs");
    bytes.require(mip.offset, mip.size);
    const auto output = result.pixels.size(), size = std::size_t(mip.width) * mip.height * 4;
    result.mips.push_back({mip.width, mip.height, output, size});
    result.pixels.resize(output + size);
    for (unsigned by = 0; by < blocks_y; ++by)
      for (unsigned bx = 0; bx < blocks_x; ++bx) {
        const auto at = mip.offset + (std::size_t(by) * blocks_x + bx) * 16;
        const auto color0 = bytes.u16(at + 8), color1 = bytes.u16(at + 10);
        const auto color_indices = bytes.u32(at + 12);
        const std::uint64_t alpha_indices = bytes.u32(at) |
            (std::uint64_t(bytes.u32(at + 4)) << 32);
        std::array<std::array<unsigned, 3>, 4> colors{};
        for (unsigned i = 0; i < 2; ++i) {
          const auto word = i ? color1 : color0;
          colors[i] = {unsigned(word >> 11) * 255 / 31,
                       unsigned((word >> 5) & 63) * 255 / 63,
                       unsigned(word & 31) * 255 / 31};
        }
        // DXT5 always has four opaque RGB entries, including color0<=color1.
        for (unsigned channel = 0; channel < 3; ++channel) {
          colors[2][channel] = (2 * colors[0][channel] + colors[1][channel]) / 3;
          colors[3][channel] = (colors[0][channel] + 2 * colors[1][channel]) / 3;
        }
        const unsigned a0 = std::to_integer<unsigned>(source.pixels[at]);
        const unsigned a1 = std::to_integer<unsigned>(source.pixels[at + 1]);
        std::array<unsigned, 8> alpha{a0, a1};
        if (a0 > a1)
          for (unsigned i = 2; i < 8; ++i)
            alpha[i] = ((8 - i) * a0 + (i - 1) * a1) / 7;
        else {
          for (unsigned i = 2; i < 6; ++i)
            alpha[i] = ((6 - i) * a0 + (i - 1) * a1) / 5;
          alpha[6] = 0;
          alpha[7] = 255;
        }
        for (unsigned y = 0; y < 4 && by * 4 + y < mip.height; ++y)
          for (unsigned x = 0; x < 4 && bx * 4 + x < mip.width; ++x) {
            const auto index = y * 4 + x;
            const auto &color = colors[(color_indices >> (index * 2)) & 3];
            const auto pixel = output +
                (std::size_t(by * 4 + y) * mip.width + bx * 4 + x) * 4;
            for (unsigned channel = 0; channel < 3; ++channel)
              result.pixels[pixel + channel] = std::byte(color[channel]);
            result.pixels[pixel + 3] =
                std::byte(alpha[(alpha_indices >> (16 + 3 * index)) & 7]);
          }
      }
  }
  return result;
}
Texture prepare_xemu_bc_texture(const Texture &source) {
    if((source.format<14 || source.format>16) || source.depth ||
        (source.layers!=1 && source.layers!=6) || source.mips.empty() ||
        source.mips.size()%source.layers || !source.width || !source.height ||
        source.width>4096 || source.height>4096 ||
        (source.layers==6 && source.width!=source.height))
        throw FormatError("original BC expansion requires a valid 2D/cube mip chain");
    const unsigned levels=unsigned(source.mips.size()/source.layers);
    if(levels>std::bit_width(std::max(source.width,source.height)))
        throw FormatError("original BC mip chain is too long");
    Texture result{source.width,source.height,11,{},{},source.layers};
    Texture face=source;face.layers=1;
    for(unsigned layer=0;layer<source.layers;++layer) {
        face.mips.assign(source.mips.begin()+layer*levels,source.mips.begin()+(layer+1)*levels);
        for(unsigned level=0;level<levels;++level) {
            const auto &m=face.mips[level];
            if(m.width!=std::max(1u,source.width>>level) ||
                m.height!=std::max(1u,source.height>>level) || m.depth!=1)
                throw FormatError("original BC face/mip dimensions differ");
        }
        const auto decoded=source.format==14?prepare_xemu_bc1_texture(face):
            source.format==15?prepare_xemu_bc2_texture(face):prepare_xemu_bc3_texture(face);
        const auto offset=result.pixels.size();
        result.pixels.insert(result.pixels.end(),decoded.pixels.begin(),decoded.pixels.end());
        for(auto mip:decoded.mips){mip.offset+=offset;result.mips.push_back(mip);}
    }
    return result;
}
Texture prepare_menu_noise_texture(const Texture &source) {
    return prepare_xemu_bc1_texture(source);
}
}
