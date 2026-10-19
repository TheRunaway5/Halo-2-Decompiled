#include "halo2/frontend_ui_assets.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/texture.hpp"
#include "halo2/ui_dds.hpp"
#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <fstream>
#include <optional>
#include <sstream>
#include <string_view>

namespace halo2 {
namespace {
constexpr std::size_t max_dds_bytes=128u*1024u*1024u;
constexpr std::size_t max_pack_bytes=512u*1024u*1024u;
std::vector<std::byte> read_file(const std::filesystem::path &path,std::size_t limit) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  const auto size=file.tellg();
  if (!file || size<0 || std::uint64_t(size)>limit)
    throw FormatError("cannot read bounded UI asset file: "+path.string());
  std::vector<std::byte> data(static_cast<std::size_t>(size));
  file.seekg(0);
  if (!file.read(reinterpret_cast<char *>(data.data()),std::streamsize(data.size())))
    throw FormatError("incomplete UI asset file: "+path.string());
  return data;
}
std::string ascii_lower(std::string value) {
  for (auto &c:value) if (c>='A' && c<='Z') c=char(c-'A'+'a');
  return value;
}
std::filesystem::path contained_file(const std::filesystem::path &root,
                                     std::string_view relative) {
  if (relative.empty() || relative.find_first_of("\\:\t\r\n")!=relative.npos ||
      relative.find('\0')!=relative.npos)
    throw FormatError("invalid relative UI asset path");
  const std::filesystem::path path{relative};
  if (path.is_absolute() || path.has_root_name() || path.has_root_directory())
    throw FormatError("absolute UI asset paths are forbidden");
  for (const auto &part:path)
    if (part.empty() || part=="." || part=="..")
      throw FormatError("UI asset path traversal is forbidden");
  std::error_code error;
  const auto resolved=std::filesystem::canonical(root/path,error);
  if (error || !std::filesystem::is_regular_file(resolved,error) || error)
    throw FormatError("missing regular UI asset file: "+path.string());
  auto parent=root.begin(), child=resolved.begin();
  for (;parent!=root.end();++parent,++child)
    if (child==resolved.end() || *parent!=*child)
      throw FormatError("UI asset symlink escapes the pack directory");
  return resolved;
}
unsigned number(std::string_view field) {
  unsigned value{};
  const auto result=std::from_chars(field.data(),field.data()+field.size(),value);
  if (field.empty() || result.ec!=std::errc{} || result.ptr!=field.data()+field.size())
    throw FormatError("invalid unsigned UI asset manifest field");
  return value;
}
std::size_t level_size(unsigned width,unsigned height,unsigned format) {
  if (format==14 || format==15)
    return std::size_t((width+3)/4)*((height+3)/4)*(format==14?8:16);
  const unsigned bytes=format==0 || format==1?1:format==3 || format==9?2:4;
  return std::size_t(width)*height*bytes;
}
Texture read_dds(const Bytes &dds) {
  dds.require(0,128);
  const auto flags=dds.u32(8),caps=dds.u32(108);
  if (dds.u32(0)!=0x20534444 || dds.u32(4)!=124 || dds.u32(76)!=32 ||
      (flags&0x1007)!=0x1007 || (flags&~0xa100fu) ||
      !(caps&0x1000) || (caps&~0x401008u) ||
      dds.u32(24) || dds.u32(112) || dds.u32(116) ||
      dds.u32(120) || dds.u32(124))
    throw FormatError("UI asset DDS must be a supported ordinary 2D texture");
  for (unsigned offset=32;offset<76;offset+=4)
    if (dds.u32(offset)) throw FormatError("unsupported reserved UI asset DDS metadata");
  const auto width=dds.u32(16),height=dds.u32(12);
  if (!width || !height || width>4096 || height>4096)
    throw FormatError("UI asset DDS dimensions exceed the supported range");
  const auto stored_mips=dds.u32(28);
  const auto mips=std::max(1u,stored_mips);
  if ((stored_mips>1 && !(flags&0x20000)) ||
      ((flags&0x20000) && !stored_mips) ||
      mips>static_cast<unsigned>(std::bit_width(std::max(width,height))) ||
      (mips>1 && (caps&0x400008)!=0x400008) ||
      (mips==1 && (caps&0x400000)))
    throw FormatError("invalid UI asset DDS mip metadata");
  const auto pf=dds.u32(80),fourcc=dds.u32(84),bits=dds.u32(88);
  const auto red=dds.u32(92),green=dds.u32(96),blue=dds.u32(100),alpha=dds.u32(104);
  unsigned format{};
  if (pf==4 && !bits && !red && !green && !blue && !alpha) {
    if (fourcc==0x31545844) format=14;
    else if (fourcc==0x33545844) format=15;
    else throw FormatError("unsupported UI asset DDS FourCC (including DX10)");
  } else if (!fourcc && (pf==0x40 || pf==0x41) && bits==32 &&
             red==0xff0000 && green==0xff00 && blue==0xff &&
             alpha==(pf==0x41?0xff000000u:0u)) format=pf==0x41?11:10;
  else if (!fourcc && pf==0x41 && bits==16 && red==0xf00 &&
           green==0xf0 && blue==0xf && alpha==0xf000) format=9;
  else if (!fourcc && pf==0x20001 && bits==16 && red==0xff &&
           !green && !blue && alpha==0xff00) format=3;
  else if (!fourcc && pf==0x20000 && bits==8 && red==0xff &&
           !green && !blue && !alpha) format=1;
  else if (!fourcc && pf==2 && bits==8 && !red && !green && !blue && alpha==0xff)
    format=0;
  else throw FormatError("unsupported UI asset DDS pixel masks");
  const bool compressed=format==14 || format==15;
  const auto base_size=level_size(width,height,format);
  const auto pitch=std::size_t(width)*(bits/8);
  if (compressed ? (!(flags&0x80000) || (flags&8) || dds.u32(20)!=base_size)
                 : (!(flags&8) || (flags&0x80000) || dds.u32(20)!=pitch))
    throw FormatError("unsupported UI asset DDS pitch or linear size");
  std::size_t size{};
  auto w=width,h=height;
  for (unsigned level=0;level<mips;++level) {
    size+=level_size(w,h,format);
    w=std::max(1u,w/2);h=std::max(1u,h/2);
  }
  if (dds.size()!=128+size)
    throw FormatError("UI asset DDS payload does not exactly match its mip chain");
  // DDS rows are tightly packed and linear. prepare_texture preserves every
  // mip and expands BGRA/XRGB, alpha/luma and A4R4G4B4 with its existing rules.
  return prepare_texture(Bytes(dds.slice(128,size)),width,height,
                         std::uint16_t(format),false,mips-1);
}
struct Bank {
  const UiImage *stock{};
  std::vector<std::optional<std::string>> paths;
};
unsigned authored_crosshair_extent(const std::string &name,unsigned frame,
    unsigned width,unsigned height,unsigned sampled_width,unsigned sampled_height) {
  constexpr std::string_view prefix="ui\\hud\\bitmaps\\new_hud\\crosshairs\\";
  if(!name.starts_with(prefix))return false;
  struct Extent {std::string_view bank;unsigned frame,width,height,sampled_width,sampled_height;};
  // Exact Xbox/Vista shared.map records. All other pages retain the ordinary
  // aspect/density checks; these seven authored reticles retain source quads.
  constexpr std::array<Extent,7> extents{{
    {"hud_reticles",3,53,45,106,91}, {"hud_reticles",5,28,50,59,100},
    {"hud_reticles",16,55,55,110,111}, {"hud_reticles",22,9,9,20,20},
    {"hud_reticles",41,52,55,106,111},
    {"hud_weak_spot_reticles",14,35,35,70,71},
    {"hud_weak_spot_reticles",22,49,54,98,109}}};
  for(const auto &e:extents) {
    if(std::string_view(name).substr(prefix.size())!=e.bank || frame!=e.frame ||
       width!=e.width || height!=e.height || sampled_width%e.sampled_width)continue;
    const auto multiplier=sampled_width/e.sampled_width;
    if(multiplier>=1 && multiplier<=8 && sampled_height==e.sampled_height*multiplier)
      return 2*multiplier;
  }
  return 0;
}
} // namespace

Texture read_ui_dds_texture(const Bytes& dds) { return read_dds(dds); }

std::filesystem::path hud_menu_asset_directory(const std::filesystem::path &pack) {
  if(!is_mcc_classic_hud_pack(pack) && !is_mcc_classic_menu_pack(pack))return pack;
  std::ifstream file(pack/"menu-ui-source.tsv");std::string header,line,extra;
  if(!std::getline(file,header) || header!="H2_HUD_MENU_SOURCE_V1" ||
      !std::getline(file,line) || !line.starts_with("vista\t") || line.size()==6 ||
      line.find_first_of("\t\r\n\0",6,4)!=line.npos || std::getline(file,extra))
    throw FormatError("MCC UI import lacks its separate original pause-menu source; reimport the pack");
  std::filesystem::path path=line.substr(6);
  if(path.is_relative())path=pack/path;
  if(!std::filesystem::is_regular_file(path/"maps/01a_tutorial.map"))
    throw FormatError("The selected pause-menu source is unavailable: "+path.string());
  return std::filesystem::canonical(path);
}

UiAssetSampleOffsets load_hud_asset_sample_offsets(const FrontendUiAssetPack &pack,
    const std::filesystem::path &directory) {
  UiAssetSampleOffsets out;
  if(directory.empty() || !std::filesystem::exists(directory/"hud-alignment.tsv"))return out;
  const auto root=std::filesystem::canonical(directory);
  const auto raw=read_file(contained_file(root,"hud-alignment.tsv"),16*1024);
  std::istringstream lines(std::string(reinterpret_cast<const char*>(raw.data()),raw.size()));
  std::string line;
  if(!std::getline(lines,line) || line!="H2UI_HUD_ALIGNMENT_V1")throw FormatError("Invalid HUD artwork alignment version");
  while(std::getline(lines,line)) {
    if(line.empty())continue;
    std::istringstream row(line);std::string name,frame,u,v,extra;
    if(!std::getline(row,name,'\t') || !std::getline(row,frame,'\t') ||
       !std::getline(row,u,'\t') || !std::getline(row,v,'\t') || std::getline(row,extra,'\t'))
      throw FormatError("Invalid HUD artwork alignment row");
    const auto index=number(frame);
    const auto real=[](const std::string &field) {
      std::istringstream value(field);float result;char extra;
      if(!(value>>result) || value>>extra || !std::isfinite(result) || std::abs(result)>.125f)
        throw FormatError("Invalid HUD artwork sampling offset");
      return result;
    };
    const std::array offset{real(u),real(v)};
    const auto bank=pack.find(name);
    if(bank==pack.end())continue;
    if(index>=bank->second.size() || !out.emplace(std::pair{name,index},offset).second)
      throw FormatError("Duplicate or invalid HUD artwork frame alignment");
  }
  return out;
}

FrontendUiAssetPack load_ui_asset_library(const std::map<std::uint32_t,UiImage> &images,
    const std::filesystem::path &directory, const char *manifest) {
  if (directory.empty()) return {};
  std::error_code error;
  const auto root=std::filesystem::canonical(directory,error);
  if (error || !std::filesystem::is_directory(root,error) || error)
    throw FormatError("UI asset pack directory is unavailable: "+directory.string());
  const auto raw=read_file(contained_file(root,manifest),1024u*1024u);
  if (raw.empty()) throw FormatError("empty UI asset pack manifest");
  const std::string text(reinterpret_cast<const char *>(raw.data()),raw.size());
  if (text.find('\0')!=text.npos) throw FormatError("NUL in UI asset pack manifest");
  std::istringstream lines(text);
  std::string line;
  if (!std::getline(lines,line)) throw FormatError("empty UI asset pack manifest");
  if (!line.empty() && line.back()=='\r') line.pop_back();
  const bool menus=line=="H2UI_MENU_SAMPLING_PACK_V1";
  if(menus && !is_mcc_classic_menu_pack(root))throw FormatError("Menu sampling library lacks its MCC source marker");
  const bool authored=line=="H2UI_AUTHORED_SAMPLING_PACK_V3" || menus;
  const bool library=line=="H2UI_SAMPLING_PACK_V2" || authored;
  if (!library && line!="H2UI_TEXTURE_PACK_V1") throw FormatError("unsupported UI asset pack manifest version");
  std::map<std::string,Bank> banks;
  while (std::getline(lines,line)) {
    if (!line.empty() && line.back()=='\r') line.pop_back();
    if (line.empty()) continue;
    std::array<std::string_view,6> fields;
    std::size_t start{};
    for (unsigned i=0;i<fields.size();++i) {
      const auto end=line.find('\t',start);
      if ((i+1<fields.size() && end==line.npos) ||
          (i+1==fields.size() && end!=line.npos))
        throw FormatError("UI asset pack rows require six tab-separated fields");
      fields[i]=std::string_view(line).substr(start,end==line.npos?line.size()-start:end-start);
      start=end==line.npos?line.size():end+1;
    }
    const std::string name(fields[0]);
    if (name.empty() || name.size()>1024 || name.find('\r')!=name.npos ||
        (!menus && ascii_lower(name).find("framing")!=name.npos))
      throw FormatError("UI frame assets cannot be replaced by a texture pack");
    if (library && !name.starts_with("ui\\"))
      throw FormatError("UI libraries cannot replace world or model textures");
    if(menus && name.starts_with("ui\\hud\\"))
      throw FormatError("Menu libraries cannot replace gameplay HUD artwork");
    if(authored && !menus && !name.starts_with("ui\\hud\\") &&
        !name.starts_with("ui\\global_bitmaps\\emblems\\"))
      throw FormatError("Authored HUD libraries cannot replace unrelated menu artwork");
    const auto frame=number(fields[1]),count=number(fields[2]);
    const auto width=number(fields[3]),height=number(fields[4]);
    auto found=banks.find(name);
    if (found==banks.end()) {
      const UiImage *stock{};
      for (const auto &[datum,image]:images)
        if (image.name==name) {
          if (stock) throw FormatError("ambiguous original UI bitmap tag: "+name);
          stock=&image;
        }
      if (!stock && library) continue;
      if (!stock || stock->images.empty())
        throw FormatError("UI asset pack tag is absent from original content: "+name);
      found=banks.emplace(name,Bank{stock,std::vector<std::optional<std::string>>(stock->images.size())}).first;
    }
    auto &bank=found->second;
    if (!width || !height || count!=bank.stock->images.size() || frame>=count ||
        width!=bank.stock->images[frame].width || height!=bank.stock->images[frame].height ||
        bank.stock->images[frame].layers!=1 || bank.stock->images[frame].depth)
      throw FormatError("UI asset pack changes an original bank or logical extent: "+name);
    if (bank.paths[frame]) throw FormatError("duplicate UI asset pack bank index: "+name);
    if (fields[5].empty()) throw FormatError("empty UI asset pack DDS path");
    bank.paths[frame]=fields[5];
  }
  for (const auto &[name,bank]:banks)
    if (std::any_of(bank.paths.begin(),bank.paths.end(),[](const auto &path){return !path;}))
      throw FormatError("incomplete UI asset pack bank: "+name);
  FrontendUiAssetPack result;
  std::size_t encoded_size{},decoded_size{};
  for (const auto &[name,bank]:banks) {
    auto &textures=result[name];
    std::optional<std::pair<std::uint32_t,std::uint32_t>> density;
    for (unsigned frame=0;frame<bank.paths.size();++frame) {
      const auto &path=bank.paths[frame];
      const auto file=contained_file(root,*path);
      if (ascii_lower(file.extension().string())!=".dds")
        throw FormatError("UI asset pack pages must use DDS files");
      const auto dds=read_file(file,max_dds_bytes);
      if (dds.size()>max_pack_bytes-encoded_size)
        throw FormatError("UI asset pack exceeds its bounded encoded size");
      encoded_size+=dds.size();
      auto texture=read_dds(Bytes(dds));
      const auto &stock=bank.stock->images[frame];
      const auto crosshair=authored_crosshair_extent(name,frame,stock.width,stock.height,texture.width,texture.height);
      // V3 explicitly maps independently authored source pages to unchanged
      // logical owners. MCC meters/reticles include different padding/crops;
      // the importer verifies exact sequence/UV/registration correspondence.
      if (!authored && !crosshair && std::uint64_t(texture.width)*stock.height!=
          std::uint64_t(texture.height)*stock.width)
        throw FormatError("UI asset DDS changes an original page aspect ratio: "+name);
      const auto sampled_width=crosshair?stock.width*crosshair:texture.width;
      if (!authored && density && std::uint64_t(sampled_width)*density->second!=
                     std::uint64_t(density->first)*stock.width)
        throw FormatError("UI asset DDS pages use mixed density within a bank: "+name);
      density=std::pair{sampled_width,stock.width};
      if (texture.pixels.size()>max_pack_bytes-decoded_size)
        throw FormatError("UI asset pack exceeds its bounded prepared size");
      decoded_size+=texture.pixels.size();
      textures.push_back(std::move(texture));
    }
  }
  return result;
}
FrontendUiAssetPack load_frontend_ui_assets(const FrontendContent &content,
                                           const std::filesystem::path &directory) {
  if(content.source_fonts.filename()=="h2_fonts" || !is_mcc_classic_menu_pack(directory))
    return load_ui_asset_library(content.images,directory,"pack.tsv");
  const auto *manifest=content.source_fonts.empty()?"pack.tsv":
      content.gameplay_overlay?"pause.tsv":"vista.tsv";
  FrontendUiAssetPack result;
  if(content.source_fonts.empty())
    result=load_ui_asset_library(content.images,hud_menu_asset_directory(directory),"pack.tsv");
  auto sampled=load_ui_asset_library(content.images,directory,manifest);
  for(auto &[name,pages]:sampled)result.insert_or_assign(name,std::move(pages));
  return result;
}
} // namespace halo2
