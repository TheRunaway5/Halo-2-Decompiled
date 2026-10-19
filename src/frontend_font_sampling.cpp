#include "halo2/frontend_font_sampling.hpp"
#include "halo2/frontend.hpp"
#include "halo2/ui_dds.hpp"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace halo2 {
namespace {
constexpr std::uint32_t no_record = 0xffffffffu;
struct FingerprintPair { std::uint64_t original, sampled; };
// Actual files and decoded control artwork are pinned in the independent
// hd-font-sampling-20261009/private-symbols.json audit. FNV is a conservative
// known-asset selector, not a security or Xbox pixel-equivalence assertion.
constexpr std::array private_pairs{
    FingerprintPair{0x54fdb36be14f28e1ull, 0xa777e2206cc7b9c5ull}, // Conduit12
    FingerprintPair{0xf786d72c76a5daf8ull, 0x723ed89e049433c4ull}, // Conduit13
    FingerprintPair{0xc61f89b160203bd0ull, 0x7827a218a933f3cfull}, // Handel11
    FingerprintPair{0x5e6ba74015b7d73aull, 0x20f28e7709320621ull}, // Handel13
    FingerprintPair{0x894ba7d1a49dfd19ull, 0x22c8a50b84f1e701ull}, // Handel24
};
bool normal_character(unsigned c) {
  return c >= 32 && c < 0xfffe && !(c >= 0x7f && c <= 0x9f) &&
         !(c >= 0xd800 && c <= 0xf8ff) && !(c >= 0xfdd0 && c <= 0xfdef);
}
bool reviewed_private_character(unsigned c) {
  // E104/E105 deliberately select PC LB/RB instead of Xbox black/white at the
  // user's request. Other admitted controls retain their reviewed symbols;
  // unreviewed private symbols keep the original raster.
  return c >= 0xe100 && c <= 0xe111;
}
bool reviewed_pair(const RetailFont &a, const RetailFont &b) {
  return std::any_of(private_pairs.begin(), private_pairs.end(), [&](const auto &p) {
    return a.file_fingerprint == p.original && b.file_fingerprint == p.sampled;
  });
}
void validate_atlas(const Texture &atlas) {
  if (!atlas.width || !atlas.height || atlas.width > 4096 || atlas.height > 4096 ||
      atlas.format != 11 || atlas.layers != 1 || atlas.depth || !atlas.mips.empty() ||
      atlas.pixels.size() != std::size_t(atlas.width) * atlas.height * 4)
    throw FormatError("invalid decoded font sampling atlas");
}
void validate_glyph(const Texture &atlas, const FontGlyph &g) {
  if (g.x > atlas.width || g.y > atlas.height ||
      g.width > atlas.width - g.x || g.height > atlas.height - g.y)
    throw FormatError("font sampling glyph outside atlas");
}
std::map<std::uint32_t, unsigned> record_counts(const RetailFont &font) {
  std::map<std::uint32_t, unsigned> counts;
  for (const auto record : font.glyph_record_indices)
    if (record != no_record) ++counts[record];
  return counts;
}
} // namespace

static std::size_t apply_sampling(RetailFont &original,
                                 const RetailFont &sampled,bool private_admitted,bool complete=false,
                                 bool mcc=false) {
  if (!original.sampled_glyphs.empty() || !sampled.sampled_glyphs.empty())
    throw FormatError("font sampling already applied");
  // Hand-built fonts without original lookup provenance are deliberately not
  // admitted. Never infer authored coverage from glyphs' fallback aliases.
  if (original.glyph_record_indices.empty() || sampled.glyph_record_indices.empty())
    return 0;
  if (original.glyph_record_indices.size() != 65536 ||
      sampled.glyph_record_indices.size() != 65536)
    throw FormatError("incomplete source font lookup records");
  validate_atlas(original.atlas);
  validate_atlas(sampled.atlas);
  const auto original_counts = record_counts(original),
             sampled_counts = record_counts(sampled);
  std::map<unsigned, FontGlyphSample> selected;
  for (const auto &[c, logical] : original.glyphs) {
    if (c >= 65536 || (!complete && !mcc && !normal_character(c) &&
        !(private_admitted && reviewed_private_character(c)))) continue;
    const auto replacement = sampled.glyphs.find(c);
    if (replacement == sampled.glyphs.end()) continue;
    const auto a = original.glyph_record_indices[c],
               b = sampled.glyph_record_indices[c];
    if (a == no_record || b == no_record || (!complete && (original_counts.at(a) != 1 ||
        sampled_counts.at(b) != 1))) continue;
    const auto &physical = replacement->second;
    validate_glyph(original.atlas, logical);
    validate_glyph(sampled.atlas, physical);
    if (!logical.width || !logical.height || !physical.width || !physical.height) continue;
    if (!mcc && (physical.width < logical.width || physical.height < logical.height ||
        (physical.width == logical.width && physical.height == logical.height))) continue;
    selected.emplace(c, FontGlyphSample{physical.x, physical.y,
                                       physical.width, physical.height});
  }
  if (selected.empty()) return 0;
  const auto x_offset = original.atlas.width + 1;
  const auto combined_width = std::uint64_t(x_offset) + sampled.atlas.width;
  if (combined_width > 4096)
    throw FormatError("combined font sampling atlas capacity exceeded");
  Texture atlas{std::uint32_t(combined_width),
                std::max(original.atlas.height, sampled.atlas.height), 11, {}, {}};
  atlas.pixels.resize(std::size_t(atlas.width) * atlas.height * 4);
  // Match the original transparent-white glyph surrounds. Each source atlas
  // is copied byte-for-byte; no upscale, resampling or font regeneration occurs.
  for (std::size_t i = 0; i < atlas.pixels.size(); i += 4)
    atlas.pixels[i] = atlas.pixels[i + 1] = atlas.pixels[i + 2] = std::byte{255};
  auto copy = [&](const Texture &source, unsigned left) {
    for (unsigned y = 0; y < source.height; ++y) {
      const auto begin = source.pixels.begin() + std::size_t(y) * source.width * 4;
      std::copy_n(begin, std::size_t(source.width) * 4,
                  atlas.pixels.begin() + (std::size_t(y) * atlas.width + left) * 4);
    }
  };
  copy(original.atlas, 0);
  copy(sampled.atlas, x_offset);
  for (auto &[c, rect] : selected) rect.x += x_offset;
  const auto count = selected.size();
  original.atlas = std::move(atlas);
  original.sampled_glyphs = std::move(selected);
  return count;
}

std::size_t apply_retail_font_sampling(RetailFont &original,const RetailFont &sampled) {
  return apply_sampling(original,sampled,reviewed_pair(original,sampled));
}

std::size_t apply_mcc_classic_font_sampling(RetailFont &original,const RetailFont &sampled) {
  return apply_sampling(original,sampled,true,false,true);
}

bool is_mcc_classic_hud_pack(const std::filesystem::path &pack) {
  if(pack.empty() || !std::filesystem::exists(pack/"hud-source.tsv"))return false;
  std::ifstream file(pack/"hud-source.tsv",std::ios::binary);
  std::string header,row,extra;
  if(!std::getline(file,header) || header!="H2_MCC_CLASSIC_HUD_V1" ||
      !std::getline(file,row) || row!="radar-size\t128" || std::getline(file,extra))
    throw FormatError("Invalid MCC Classic HUD source marker");
  return true;
}

bool is_mcc_classic_menu_pack(const std::filesystem::path &pack) {
  if(pack.empty() || !std::filesystem::exists(pack/"menu-source.tsv"))return false;
  std::ifstream file(pack/"menu-source.tsv",std::ios::binary);
  std::string header,extra;
  if(!std::getline(file,header) || header!="H2_MCC_CLASSIC_MENUS_V1" || std::getline(file,extra))
    throw FormatError("Invalid MCC Classic menu source marker");
  return true;
}

std::filesystem::path ui_sampled_font_path(const std::filesystem::path &pack,
    unsigned selector,std::string_view original_name) {
  if(!is_mcc_classic_hud_pack(pack) && !is_mcc_classic_menu_pack(pack))return pack/"fonts"/original_name;
  std::ifstream table(pack/"fonts/font_table.txt");
  std::vector<std::string> names;
  for(std::string name;std::getline(table,name);) {
    if(!name.empty() && name.back()=='\r')name.pop_back();
    if(name.empty() || name.find_first_of("/\\:\t\0",0,5)!=name.npos || name=="." || name=="..")
      throw FormatError("Invalid MCC Classic font owner");
    names.push_back(name);
  }
  if(names.size()!=12 || selector>=names.size())throw FormatError("Incomplete MCC Classic font table");
  return pack/"fonts"/names[selector];
}

std::filesystem::path hud_sampled_font_path(const std::filesystem::path &pack,
    unsigned selector,std::string_view original_name) {
  return ui_sampled_font_path(pack,selector,original_name);
}

std::filesystem::path font_prompt_asset_directory(
    const std::filesystem::path &font_directory,
    const std::filesystem::path &explicit_pack) {
  if(font_directory.filename()=="h2_fonts" &&
      font_directory.parent_path().filename()=="halo2") {
    const auto bank=font_directory.parent_path().parent_path()/"presentation/prompts";
    if(std::filesystem::exists(bank/"prompt-source.tsv"))return bank;
  }
  return explicit_pack;
}

std::size_t apply_vista_prompt_sampling(RetailFont &font,
    const std::filesystem::path &pack,unsigned selector) {
  if(pack.empty() || !std::filesystem::exists(pack/"prompt-source.tsv"))return 0;
  std::ifstream binding(pack/"prompt-source.tsv");
  std::string header,directory,extra;
  if(!std::getline(binding,header) || header!="H2_VISTA_PROMPTS_V1" ||
      !std::getline(binding,directory) || directory.empty() || std::getline(binding,extra))
    throw FormatError("Invalid Vista prompt source binding");
  const std::filesystem::path binding_directory(directory);
  const auto root=std::filesystem::canonical(binding_directory.is_absolute()?binding_directory:pack/binding_directory);
  std::ifstream manifest(root/"prompts.tsv");
  if(!std::getline(manifest,header) || header!="H2_VISTA_PROMPT_RASTERS_V1" || selector>=12)
    throw FormatError("Invalid Vista prompt raster manifest");
  validate_atlas(font.atlas);
  std::map<unsigned,FontGlyphSample> controls;
  std::string atlas_name,line;
  while(std::getline(manifest,line)) {
    std::istringstream row(line);
    unsigned owner{},code{},x{},y{},width{},height{};std::string relative;
    if(!(row>>owner>>code>>x>>y>>width>>height>>relative) || row>>extra || owner>=12 ||
        !((code>=0xe100 && code<=0xe111) || (code>=0xe12c && code<=0xe13c)) ||
        !width || !height || width>2048 || height>1024 || width%4 || height%4)
      throw FormatError("Invalid Vista prompt raster record");
    if(owner!=selector)continue;
    const std::filesystem::path asset(relative);
    if(asset.is_absolute() || asset.extension()!=".dds" || relative.find_first_of("\\:")!=relative.npos)
      throw FormatError("Invalid Vista prompt atlas owner");
    for(const auto &part:asset)if(part=="." || part=="..")
      throw FormatError("Vista prompt atlas path traversal");
    if(!atlas_name.empty() && atlas_name!=relative)
      throw FormatError("Vista selector uses multiple prompt atlases");
    atlas_name=relative;
    if(!controls.emplace(code,FontGlyphSample{x,y,width,height}).second)
      throw FormatError("Duplicate Vista prompt codepoint");
  }
  if(!manifest.eof() || controls.empty() || controls.size()>35)
    throw FormatError("Incomplete Vista prompt raster selection");
  const auto path=std::filesystem::canonical(root/atlas_name);
  const auto relative=path.lexically_relative(root);
  if(relative.empty() || *relative.begin()=="..")throw FormatError("Vista prompt atlas escapes pack");
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  const auto size=file.tellg();
  if(!file || size<128 || size>32*1024*1024)throw FormatError("Invalid Vista prompt atlas size");
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));file.seekg(0);
  if(!file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size())))
    throw FormatError("Truncated Vista prompt atlas");
  auto prompts=read_ui_dds_texture(Bytes(bytes));
  // The DDS reader explicitly describes even its single base level. Font
  // atlases use an empty mip vector for that same single-level storage.
  if(prompts.mips.size()!=1 || prompts.mips[0].offset ||
      prompts.mips[0].width!=prompts.width || prompts.mips[0].height!=prompts.height ||
      prompts.mips[0].size!=prompts.pixels.size())
    throw FormatError("Vista prompt atlas must have exactly one base level");
  prompts.mips.clear();validate_atlas(prompts);
  std::map<unsigned,FontGlyphSample> selected;
  for(const auto &[code,sample]:controls) {
    if(sample.x>prompts.width || sample.y>prompts.height ||
        sample.width>prompts.width-sample.x || sample.height>prompts.height-sample.y)
      throw FormatError("Vista prompt rectangle outside atlas");
    // Xbox does not own some later Vista controls. Never apply those rasters
    // to a fallback alias or introduce a new logical glyph/advance.
    if(font.glyph_record_indices.size()!=65536)throw FormatError("Missing prompt glyph provenance");
    const auto index=font.glyph_record_indices.at(code);
    const auto logical=font.glyphs.find(code);
    if(index==no_record || logical==font.glyphs.end() ||
        std::count(font.glyph_record_indices.begin(),font.glyph_record_indices.end(),index)!=1 ||
        !logical->second.width || !logical->second.height)continue;
    selected.emplace(code,sample);
  }
  if(selected.empty())return 0;
  const auto left=font.atlas.width+1;
  const auto width=std::uint64_t(left)+prompts.width;
  if(width>4096)throw FormatError("Vista prompt combined atlas capacity exceeded");
  Texture combined{std::uint32_t(width),std::max(font.atlas.height,prompts.height),11,{},{}};
  combined.pixels.resize(std::size_t(combined.width)*combined.height*4);
  for(std::size_t i=0;i<combined.pixels.size();i+=4)
    combined.pixels[i]=combined.pixels[i+1]=combined.pixels[i+2]=std::byte{255};
  const auto copy=[&](const Texture &source,unsigned offset) {
    for(unsigned y=0;y<source.height;++y)
      std::copy_n(source.pixels.begin()+std::size_t(y)*source.width*4,std::size_t(source.width)*4,
          combined.pixels.begin()+(std::size_t(y)*combined.width+offset)*4);
  };
  copy(font.atlas,0);copy(prompts,left);
  for(auto &[code,sample]:selected) {sample.x+=left;font.sampled_glyphs[code]=sample;}
  font.atlas=std::move(combined);
  return selected.size();
}

std::size_t apply_retail_font_sampling(RetailFont &original,const RetailFont &sampled,
                                     const RetailFont &donor) {
  validate_atlas(donor.atlas);validate_atlas(sampled.atlas);
  if(donor.file_fingerprint!=sampled.file_fingerprint)throw FormatError("Vector font donor fingerprint mismatch");
  if(donor.glyph_record_indices!=sampled.glyph_record_indices ||
     donor.glyphs.size()!=sampled.glyphs.size() || donor.ascent!=sampled.ascent ||
     donor.descent!=sampled.descent || donor.leading!=sampled.leading || donor.kerning!=sampled.kerning)
    throw FormatError("Reconstructed font changes its donor lookup or metrics");
  for(const auto &[c,a]:donor.glyphs) {
    const auto &b=sampled.glyphs.at(c);
    validate_glyph(donor.atlas,a);validate_glyph(sampled.atlas,b);
    if(a.advance!=b.advance || a.bearing_x!=b.bearing_x || a.bearing_y!=b.bearing_y ||
       (!a.width || !a.height ? bool(b.width)!=bool(a.width) || bool(b.height)!=bool(a.height) :
        b.width<a.width || b.height<a.height || b.width>16*a.width+16 || b.height>16*a.height+16))
      throw FormatError("Reconstructed font changes donor glyph metrics");
    if(!a.width || !a.height)continue;
    // Vector rasters intentionally replace source antialiasing. Provenance,
    // lookup and logical metrics are validated here; reproducible SVG-to-RGBA
    // identity and source ink geometry are independently checked by the pack tool.
  }
  return apply_sampling(original,sampled,reviewed_pair(original,donor),original.file_fingerprint==donor.file_fingerprint);
}

std::array<float, 4> retail_font_sample_uvs(const RetailFont &font, unsigned c,
                                         int x, int y, int width, int height) {
  const auto &logical = font.glyphs.at(c);
  if (!font.atlas.width || !font.atlas.height || x < 0 || y < 0 ||
      width < 0 || height < 0 || std::uint64_t(x) + width > logical.width ||
      std::uint64_t(y) + height > logical.height)
    throw FormatError("font sampling clip outside original logical glyph");
  const auto selected = font.sampled_glyphs.find(c);
  if (selected == font.sampled_glyphs.end()) {
    // Keep the original-only expression order and conversions exactly.
    return {float(int(logical.x) + x) / font.atlas.width,
            float(int(logical.y) + y) / font.atlas.height,
            float(int(logical.x) + x + width) / font.atlas.width,
            float(int(logical.y) + y + height) / font.atlas.height};
  }
  if (!logical.width || !logical.height)
    throw FormatError("empty logical glyph has sampled pixels");
  const auto &sample = selected->second;
  return {(sample.x + float(x) * sample.width / logical.width) / font.atlas.width,
          (sample.y + float(y) * sample.height / logical.height) / font.atlas.height,
          (sample.x + float(x + width) * sample.width / logical.width) / font.atlas.width,
          (sample.y + float(y + height) * sample.height / logical.height) / font.atlas.height};
}
} // namespace halo2
