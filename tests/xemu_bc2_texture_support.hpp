#pragma once
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>

// Test-only readers for the frozen unchanged-Xemu DXT3 oracle. Compressed
// inputs and independently retained RGBA expectations remain separate; no
// production decoder is used to create an expectation here.
namespace xemu_bc2_test {
using namespace halo2;
using Data = std::vector<std::byte>;
namespace fs = std::filesystem;
inline void require(bool value,const std::string &message) {
  if(!value)throw FormatError(message);
}
inline Data read(const fs::path &path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  require(bool(file),"missing original BC2 fixture: "+path.string());
  const auto size=file.tellg();
  require(size>=0 && size<=std::streamoff(64*1024*1024),"invalid original BC2 fixture size");
  Data result(static_cast<std::size_t>(size));file.seekg(0);
  if(!result.empty())require(bool(file.read(reinterpret_cast<char*>(result.data()),size)),
                            "truncated original BC2 fixture");
  return result;
}
inline std::vector<std::string> fields(const std::string &line) {
  std::vector<std::string> out;std::istringstream input(line);std::string field;
  while(std::getline(input,field,'\t'))out.push_back(field);
  return out;
}
inline unsigned number(const std::string &value) {
  std::size_t consumed{};const auto out=std::stoull(value,&consumed,10);
  require(consumed==value.size() && out<=std::numeric_limits<unsigned>::max(),
          "invalid BC2 fixture integer");
  return unsigned(out);
}
inline Data unhex(const std::string &value) {
  require(value.size()%2==0,"invalid BC2 fixture hex length");Data out;
  const auto nibble=[](char c)->unsigned {
    if(c>='0' && c<='9')return unsigned(c-'0');
    if(c>='a' && c<='f')return unsigned(c-'a'+10);
    if(c>='A' && c<='F')return unsigned(c-'A'+10);
    throw FormatError("invalid BC2 fixture hex digit");
  };
  out.reserve(value.size()/2);
  for(std::size_t i=0;i<value.size();i+=2)out.push_back(std::byte(nibble(value[i])*16+nibble(value[i+1])));
  return out;
}
inline Texture source(unsigned width,unsigned height,unsigned layers,unsigned levels,Data raw) {
  require(width && height && width<=4096 && height<=4096 &&
      (layers==1 || layers==6) && (layers!=6 || width==height) && levels &&
      levels<=std::bit_width(std::max(width,height)),"invalid frozen BC2 source shape");
  Texture out{width,height,15,std::move(raw),{},layers};std::size_t offset{};
  for(unsigned face=0;face<layers;++face)for(unsigned level=0;level<levels;++level) {
    const auto w=std::max(1u,width>>level),h=std::max(1u,height>>level);
    const auto size=std::size_t((w+3)/4)*((h+3)/4)*16;
    Bytes(out.pixels).require(offset,size);out.mips.push_back({w,h,offset,size,1});offset+=size;
  }
  require(offset==out.pixels.size(),"frozen BC2 source has trailing/missing face bytes");
  return out;
}
struct Image {
  std::string label;
  Texture source;
  Data expected;
  unsigned levels{};
  bool actual{};
};
inline std::vector<Image> load_images(const fs::path &directory) {
  std::ifstream file(directory/"manifest.tsv");std::string line;
  require(bool(std::getline(file,line)) && line==
      "# BC2_ORIGINAL_IMAGES_V1 label width height layers levels source_format raw rgba layout usage",
      "original BC2 image manifest version changed");
  std::vector<Image> out;std::size_t mip_faces{},rgba_bytes{};unsigned actual{};
  while(std::getline(file,line)) {
    if(line.empty())continue;const auto f=fields(line);
    require(f.size()==10 && number(f[5])==15,"invalid original BC2 image manifest row");
    for(unsigned field:{6u,7u,8u})require(!fs::path(f[field]).has_parent_path(),
                                       "original BC2 image path escapes fixture directory");
    Image image;image.label=f[0];image.levels=number(f[4]);
    image.source=source(number(f[1]),number(f[2]),number(f[3]),image.levels,read(directory/f[6]));
    image.expected=read(directory/f[7]);image.actual=f[9].starts_with("actual direct-wgit-bound image;");
    std::size_t expected_size{};for(const auto &m:image.source.mips)expected_size+=std::size_t(m.width)*m.height*4;
    require(image.expected.size()==expected_size,"original BC2 RGBA extent differs");
    require(std::none_of(out.begin(),out.end(),[&](const auto &p){return p.label==image.label;}),
            "duplicate original BC2 image");
    mip_faces+=image.source.mips.size();rgba_bytes+=image.expected.size();actual+=image.actual;
    out.push_back(std::move(image));
  }
  require(out.size()==11 && mip_faces==59 && rgba_bytes==9611464 && actual==6,
          "original BC2 image coverage changed");
  return out;
}
struct Synthetic {std::string label;Texture source;Data expected;};
inline std::vector<Synthetic> load_synthetic(const fs::path &directory) {
  std::ifstream file(directory/"synthetic.tsv");std::string line;
  require(bool(std::getline(file,line)) && line==
      "# BC2_ORIGINAL_SYNTHETIC_V1 label width height raw_bc2_hex original_rgba_hex",
      "original BC2 synthetic manifest version changed");
  std::vector<Synthetic> out;
  while(std::getline(file,line)) {
    if(line.empty())continue;const auto f=fields(line);require(f.size()==5,"invalid original BC2 synthetic row");
    Synthetic test{f[0],source(number(f[1]),number(f[2]),1,1,unhex(f[3])),unhex(f[4])};
    require(test.expected.size()==std::size_t(test.source.width)*test.source.height*4,
            "original synthetic BC2 RGBA extent differs");
    out.push_back(std::move(test));
  }
  require(out.size()==768,"original BC2 synthetic coverage changed");return out;
}
inline void exact(const Texture &candidate,const Texture &input,const Data &expected,const std::string &label) {
  require(candidate.format==11 && candidate.width==input.width && candidate.height==input.height &&
      candidate.layers==input.layers && !candidate.depth && candidate.mips.size()==input.mips.size(),
      label+": expanded BC2 format/topology differs");
  require(candidate.pixels==expected,label+": BC2 texels differ from unchanged original decoder");
  std::size_t offset{};
  for(unsigned i=0;i<candidate.mips.size();++i) {
    const auto &a=candidate.mips[i],&b=input.mips[i];const auto size=std::size_t(b.width)*b.height*4;
    require(a.width==b.width && a.height==b.height && a.depth==1 && a.offset==offset && a.size==size,
            label+": expanded BC2 face/mip layout differs");offset+=size;
  }
  require(offset==candidate.pixels.size(),label+": expanded BC2 mip bytes differ");
}
} // namespace xemu_bc2_test
