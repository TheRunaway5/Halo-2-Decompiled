#include "halo2/vista_loading_assets.hpp"
#include "halo2/ui_dds.hpp"
#include <charconv>
#include <fstream>
#include <sstream>
#include <string_view>
#include <utility>

namespace halo2 {
namespace {
std::vector<std::byte> read(const std::filesystem::path& path,std::size_t limit) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);
  const auto length=input.tellg();
  if (!input || length<0 || std::uint64_t(length)>limit)
    throw FormatError("cannot read bounded Vista loading asset: "+path.string());
  std::vector<std::byte> result(static_cast<std::size_t>(length));
  input.seekg(0);
  if (!input.read(reinterpret_cast<char*>(result.data()),std::streamsize(result.size())))
    throw FormatError("incomplete Vista loading asset: "+path.string());
  return result;
}
unsigned number(std::string_view text) {
  unsigned result{};
  const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
  if (text.empty() || parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size())
    throw FormatError("invalid Vista loading asset manifest integer");
  return result;
}
std::filesystem::path contained(const std::filesystem::path& root,std::string_view name) {
  if (name.empty() || name.find_first_of("\\/:\t\r\n")!=name.npos ||
      name.find('\0')!=name.npos || name=="." || name=="..")
    throw FormatError("Vista loading assets require local filenames");
  std::error_code error;
  const auto result=std::filesystem::canonical(root/std::string(name),error);
  if (error || result.parent_path()!=root || !std::filesystem::is_regular_file(result))
    throw FormatError("Vista loading asset is missing or outside its directory");
  return result;
}
}
VistaLoadingAssets load_vista_loading_assets(const std::filesystem::path& directory) {
  const auto root=std::filesystem::canonical(directory);
  const auto manifest=read(contained(root,"loading-textures.tsv"),16*1024);
  const std::string text(reinterpret_cast<const char*>(manifest.data()),manifest.size());
  if (text.find('\0')!=text.npos) throw FormatError("NUL in Vista loading manifest");
  std::istringstream lines(text);
  std::string line;
  if (!std::getline(lines,line) || line!="H2V_LOADING_TEXTURES_V1")
    throw FormatError("unsupported Vista loading asset manifest");
  VistaLoadingAssets result;
  std::array<bool,12> seen{};
  while (std::getline(lines,line)) {
    if (!line.empty() && line.back()=='\r') line.pop_back();
    std::array<std::string_view,7> fields{};
    std::string_view remaining=line;
    for (unsigned i=0;i<fields.size();++i) {
      const auto separator=remaining.find('\t');
      if ((i+1==fields.size())!=(separator==remaining.npos))
        throw FormatError("invalid Vista loading asset manifest row");
      fields[i]=remaining.substr(0,separator);
      if (separator!=remaining.npos) remaining.remove_prefix(separator+1);
    }
    const auto index=number(fields[0]);
    if (index>=result.size() || seen[index]) throw FormatError("invalid Vista loading texture index");
    const auto source_width=number(fields[1]),source_height=number(fields[2]);
    const auto width=number(fields[3]),height=number(fields[4]),format=number(fields[5]);
    if (!source_width || !source_height || source_width>4096 || source_height>4096 ||
        !width || !height || width>4096 || height>4096 || (format!=10 && format!=11))
      throw FormatError("unsupported Vista loading texture metadata");
    const auto raw=read(contained(root,fields[6]),128+4096u*4096u*4u);
    const Bytes dds(raw);
    // This source loader requests exactly one level, with original A8/X8
    // masks. Reject an export whose allocation metadata no longer agrees.
    dds.require(0,128);
    if (dds.u32(16)!=width || dds.u32(12)!=height || dds.u32(28)>1 ||
        dds.u32(80)!=(format==11?0x41u:0x40u) || dds.u32(88)!=32)
      throw FormatError("Vista loading DDS differs from its manifest");
    auto texture=read_ui_dds_texture(dds);
    result[index]={source_width,source_height,std::move(texture)};
    seen[index]=true;
  }
  for (bool present:seen) if (!present) throw FormatError("incomplete Vista loading texture bank");
  return result;
}
}
