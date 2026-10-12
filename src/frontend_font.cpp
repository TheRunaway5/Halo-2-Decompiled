#include "halo2/frontend.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <set>
namespace halo2 {
static std::vector<std::uint16_t> decode_font_pixels_bounded(std::span<const std::byte> source,
                                              std::size_t limit,std::size_t maximum) {
  if (limit > maximum)
    throw FormatError("excessive font glyph size");
  const Bytes bytes(source);
  std::vector<std::uint16_t> result;
  unsigned color = 0xfff;
  auto append = [&](unsigned alpha, unsigned count) {
    if (count > limit - result.size())
      throw FormatError("font pixel run outside glyph");
    const auto pixel =
        std::uint16_t(((((alpha << 1) | (alpha & 1)) & 15) << 12) | color);
    result.insert(result.end(), count, pixel);
  };
  for (std::size_t i = 0; i < source.size();) {
    const auto code = std::to_integer<unsigned>(source[i++]), type = code >> 6;
    if (type == 3) {
      append((code >> 3) & 7, 1);
      append(code & 7, 1);
    } else if (type == 2) {
      append((code >> 3) & 7, 1);
      const auto low = code & 7;
      if (low >= 1 && low <= 3)
        append(7, 5 - low);
      if (low >= 4)
        append(0, 9 - low);
    } else if (type == 0 && !(code & 63)) {
      bytes.require(i, 2);
      const auto pixel = (std::to_integer<unsigned>(source[i]) << 8) |
                         std::to_integer<unsigned>(source[i + 1]);
      i += 2;
      color = pixel & 0xfff;
      if (result.size() == limit)
        throw FormatError("font literal outside glyph");
      result.push_back(std::uint16_t(pixel));
    } else
      append(type == 1 ? 7 : 0, code & 63);
  }
  // Retail streams omit trailing transparent pixels, especially for spaces.
  result.resize(limit, 0);
  return result;
}
std::vector<std::uint16_t> decode_font_pixels(std::span<const std::byte> source,
                                              std::size_t limit) {
  return decode_font_pixels_bounded(source,limit,256*128);
}
RetailFont load_retail_font(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file || file.tellg() < 0x40400 || file.tellg() > 128 * 1024 * 1024)
    throw FormatError("invalid retail font " + path.string());
  std::vector<std::byte> data(static_cast<std::size_t>(file.tellg()));
  file.seekg(0);
  if (!file.read(reinterpret_cast<char *>(data.data()), data.size()))
    throw FormatError("truncated font file");
  const Bytes b(data);
  const bool vector_font=b.u32(0x200)==0xf0000003;
  const bool high_density=b.u32(0x200)==0xf0000002 || vector_font;
  if (b.u32(0x200) != 0xf0000001 && !high_density)
    throw FormatError("unsupported retail font version");
  unsigned density=1;
  if(high_density) {
    const auto trailer=data.size()-(vector_font?16:8);
    if(b.u32(trailer)!=(vector_font?0x54563248:0x44483248))throw FormatError("invalid density font trailer"); // H2VT / H2HD
    density=b.u32(trailer+4);
    if(density<2 || density>8)throw FormatError("invalid exact-density font scale");
  }
  RetailFont out;
  out.file_fingerprint = 0xcbf29ce484222325ull;
  for (const auto byte : data) {
    out.file_fingerprint ^= std::to_integer<unsigned>(byte);
    out.file_fingerprint *= 0x100000001b3ull;
  }
  if(vector_font)out.file_fingerprint=std::uint64_t(b.u32(data.size()-8)) | (std::uint64_t(b.u32(data.size()-4))<<32);
  else if(!high_density && data.size()>16*1024*1024)throw FormatError("excessive retail font file");
  out.glyph_record_indices.resize(65536);
  for (unsigned c = 0; c < out.glyph_record_indices.size(); ++c)
    out.glyph_record_indices[c] = b.u32(0x400 + c * 4);
  out.ascent = b.u16(0x204);
  out.descent = b.u16(0x206);
  out.leading = b.u16(0x208);
  if (out.ascent > 128 || out.descent > 128 || out.leading > 128)
    throw FormatError("invalid font metrics");
  const auto pairs = b.u32(0x220);
  if (pairs > 96)
    throw FormatError("excessive font kerning pairs");
  for (unsigned i = 0; i < pairs; ++i) {
    const auto p = 0x224 + i * 4;
    out.kerning[{std::to_integer<unsigned>(data[p]),
                 std::to_integer<unsigned>(data[p + 1])}] =
        std::bit_cast<std::int16_t>(b.u16(p + 2));
  }
  struct Pending {
    unsigned character{}, offset{}, x{}, y{};
    FontGlyph glyph;
  };
  std::vector<Pending> pending;
  std::map<unsigned, FontGlyph> locations;
  unsigned atlas_width=1024;
  if(high_density)for(const auto index:out.glyph_record_indices) {
    if(index==0xffffffffu)continue;
    if(index>1000000)throw FormatError("invalid font character offset");
    const auto p=std::size_t(index+0x4040)*16;b.require(p,16);
    if(high_density && (b.u16(p+4)>256 || b.u16(p+6)>128))atlas_width=2048;
  }
  if(high_density) {
    const auto packed_height=[&](unsigned width) {
      std::set<unsigned> records;unsigned px=1,py=1,highest=0;
      for(unsigned c=32;c<65536;++c) {
        const auto index=out.glyph_record_indices[c];
        if(index==0xffffffffu || !records.insert(index).second)continue;
        const auto p=std::size_t(index+0x4040)*16;
        const auto w=b.u16(p+4),h=b.u16(p+6);
        if(px+w+1>width){px=1;py+=highest+1;highest=0;}
        px+=w+1;highest=std::max(highest,unsigned(h));
      }
      return py+highest+1;
    };
    //1024 original +1 gutter +3070 sampled =4095, within the existing
    // combined-atlas limit. Large authored control glyphs remain complete.
    if(packed_height(atlas_width)>4096)atlas_width=3070;
  }
  unsigned x = 1, y = 1, row = 0;
  for (unsigned c = 32; c < 65536; ++c) {
    const auto index = out.glyph_record_indices[c];
    if (index == 0xffffffffu)
      continue;
    if (index > 1000000)
      throw FormatError("invalid font character offset");
    const auto p = std::size_t(index + 0x4040) * 16;
    b.require(p, 16);
    const auto width = b.u16(p + 4), height = b.u16(p + 6);
    // MCC's authored E121 control is280x58 in17 supplied font files. Admit
    // this exact retained stream; keep the existing bounds for other v1 data.
    bool mcc_control=false;
    if(!high_density && c==0xe121 && width==280 && height==58 && b.u16(p+2)==3733) {
      std::uint64_t fingerprint=0xcbf29ce484222325ull;
      for(const auto byte:b.slice(b.u32(p+12),3733)) {
        fingerprint^=std::to_integer<unsigned>(byte);fingerprint*=0x100000001b3ull;
      }
      mcc_control=fingerprint==0xf04424c4259e42b7ull;
    }
    if (width > (high_density?2048:mcc_control?280:256) || width+2>atlas_width || height > (high_density?1024:128) ||
        b.u16(p+2)>8192 || width%density || height%density)
      throw FormatError("invalid font character header");
    if (const auto it = locations.find(index); it != locations.end()) {
      out.glyphs[c] = it->second;
      continue;
    }
    if (x + width + 1 > atlas_width) {
      x = 1;
      y += row + 1;
      row = 0;
    }
    if (y + height + 1 > 4096)
      throw FormatError("font atlas capacity exceeded: "+path.string());
    FontGlyph glyph{x,
                    y,
                    width,
                    height,
                    int(b.u16(p)),
                    std::bit_cast<std::int16_t>(b.u16(p + 8)),
                    std::bit_cast<std::int16_t>(b.u16(p + 10))};
    out.glyphs[c] = glyph;
    locations[index] = glyph;
    pending.push_back({c, unsigned(p), x, y, glyph});
    x += width + 1;
    row = std::max(row, unsigned(height));
  }
  out.atlas = {atlas_width, y + row + 1, 11, {}, {}};
  out.atlas.pixels.resize(std::size_t(out.atlas.width) * out.atlas.height * 4);
  // 1F5C0 clears each glyph's one-texel surround to A4R4G4B4=0x0FFF
  // before copying its decoded pixels. Transparent RGB participates in
  // linear filtering at half-pixel glyph positions, including button icons.
  for (const auto &p : pending)
    for (unsigned yy = p.y - 1; yy <= p.y + p.glyph.height; ++yy)
      for (unsigned xx = p.x - 1; xx <= p.x + p.glyph.width; ++xx) {
        const auto dst = (std::size_t(yy) * out.atlas.width + xx) * 4;
        out.atlas.pixels[dst] = out.atlas.pixels[dst + 1] =
            out.atlas.pixels[dst + 2] = std::byte{255};
      }
  for (const auto &p : pending) {
    if(vector_font) {
      const auto count=std::size_t(p.glyph.width)*p.glyph.height*4;
      const auto offset=b.u32(p.offset+12);
      if(count && (offset<0x40400 || offset>data.size()-16 || count>data.size()-16-offset))
        throw FormatError("vector glyph pixels outside font payload");
      const auto pixels=b.slice(offset,count);
      for(unsigned yy=0;yy<p.glyph.height;++yy)
        std::copy_n(pixels.begin()+std::size_t(yy)*p.glyph.width*4,std::size_t(p.glyph.width)*4,
          out.atlas.pixels.begin()+(std::size_t(p.y+yy)*out.atlas.width+p.x)*4);
      continue;
    }
    const auto encoded = b.slice(b.u32(p.offset + 12), b.u16(p.offset + 2));
    // Version2 retains the donor RLE stream byte-for-byte. Its explicit
    // integer subdivision is applied only when filling the physical atlas.
    const auto source_width=p.glyph.width/density,source_height=p.glyph.height/density;
    const auto pixels=decode_font_pixels(encoded,source_width*source_height);
    for (unsigned yy = 0; yy < p.glyph.height; ++yy)
      for (unsigned xx = 0; xx < p.glyph.width; ++xx) {
        const auto v = pixels[(yy/density)*source_width+xx/density];
        const auto dst =
            (std::size_t(p.y + yy) * out.atlas.width + p.x + xx) * 4;
        out.atlas.pixels[dst] = std::byte(((v >> 8) & 15) * 17);
        out.atlas.pixels[dst + 1] = std::byte(((v >> 4) & 15) * 17);
        out.atlas.pixels[dst + 2] = std::byte((v & 15) * 17);
        out.atlas.pixels[dst + 3] = std::byte((v >> 12) * 17);
      }
  }
  return out;
}
} // namespace halo2
