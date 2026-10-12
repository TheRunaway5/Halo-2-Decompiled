#pragma once
#include "halo2/cache.hpp"
#include <algorithm>
#include <fstream>

namespace halo2 {
// Deliberately bounded mod seam: one complete 2D BC2/DXT3 page, retaining
// stock dimensions and filtering. It does not decode/recompress Xbox art.
inline std::size_t frontend_frame_bc2_size(const Texture &stock) {
  if(!stock.width || !stock.height || stock.width>16384 || stock.height>16384 ||
     stock.layers!=1 || stock.depth || stock.format!=15)
    throw FormatError("frame edge requires a single 2D BC2 page");
  const auto size=std::size_t((stock.width+3)/4)*((stock.height+3)/4)*16;
  if(stock.pixels.size()!=size || (!stock.mips.empty() &&
      (stock.mips.size()!=1 || stock.mips[0].width!=stock.width ||
       stock.mips[0].height!=stock.height || stock.mips[0].offset ||
       stock.mips[0].size!=size || stock.mips[0].depth!=1)))
    throw FormatError("frame edge BC2 page has an incomplete or unsupported mip layout");
  return size;
}
inline std::vector<std::byte> frontend_frame_dds(const Texture &stock) {
  const auto size=frontend_frame_bc2_size(stock);
  std::vector<std::byte> dds(128+size);
  const auto word=[&](std::size_t offset,std::uint32_t value) {
    for(unsigned i=0;i<4;++i)dds[offset+i]=std::byte((value>>(i*8))&255);
  };
  word(0,0x20534444); // "DDS ", followed by the standard124-byte header.
  word(4,124);word(8,0x81007); // CAPS/HEIGHT/WIDTH/PIXELFORMAT/LINEARSIZE.
  word(12,stock.height);word(16,stock.width);word(20,std::uint32_t(size));
  word(28,1);word(76,32);word(80,4);word(84,0x33545844); // FOURCC DXT3.
  word(108,0x1000); // DDSCAPS_TEXTURE; neither cube nor volume.
  std::copy(stock.pixels.begin(),stock.pixels.end(),dds.begin()+128);
  return dds;
}
inline Texture read_frontend_frame_dds(const Bytes &dds,const Texture &stock) {
  const auto size=frontend_frame_bc2_size(stock);
  if(dds.size()!=128+size)throw FormatError("frame edge DDS payload size differs from stock");
  const auto flags=dds.u32(8);
  if(dds.u32(0)!=0x20534444 || dds.u32(4)!=124 ||
     (flags&0x1007)!=0x1007 || (flags&~0xa1007u) ||
     dds.u32(12)!=stock.height || dds.u32(16)!=stock.width ||
     ((flags&0x80000) && dds.u32(20)!=size) ||
     dds.u32(24)>1 || dds.u32(28)>1 ||
     dds.u32(76)!=32 || dds.u32(80)!=4 || dds.u32(84)!=0x33545844 ||
     dds.u32(88) || dds.u32(92) || dds.u32(96) || dds.u32(100) || dds.u32(104) ||
     !(dds.u32(108)&0x1000) || (dds.u32(108)&~0x401008u) ||
     dds.u32(112) || dds.u32(116) || dds.u32(120))
    throw FormatError("frame edge override must retain stock dimensions and a single 2D DXT3 page");
  Texture result=stock;
  const auto payload=dds.slice(128,size);
  result.pixels.assign(payload.begin(),payload.end());
  return result;
}
inline Texture frontend_frame_edge_texture(const Texture &stock,
                                           const std::filesystem::path &path) {
  // The painter never writes assets or requires an override directory.
  if(path.empty())return stock;
  std::error_code error;
  const bool present=std::filesystem::exists(path,error);
  if(error)throw FormatError("cannot inspect frame edge override: "+path.string());
  if(!present)return stock;
  const auto size=frontend_frame_bc2_size(stock);
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  if(!file || file.tellg()!=std::streamoff(128+size))
    throw FormatError("cannot read complete frame edge override: "+path.string());
  std::vector<std::byte> data(128+size);
  file.seekg(0);
  if(!file.read(reinterpret_cast<char *>(data.data()),std::streamsize(data.size())))
    throw FormatError("cannot read frame edge override: "+path.string());
  return read_frontend_frame_dds(Bytes(data),stock);
}
} // namespace halo2
