#include "halo2/vista_widget_cache.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <string_view>

namespace halo2 {
namespace {
std::uint32_t scalar(std::span<const std::byte> bytes) {
  std::uint32_t value{};
  for(std::size_t i=0;i<bytes.size();++i)
    value|=std::uint32_t(std::to_integer<std::uint8_t>(bytes[i]))<<(i*8);
  return value;
}
std::span<const std::byte> checked(std::span<const std::byte> bank,std::uint64_t offset,std::size_t size) {
  if(offset>bank.size() || size>bank.size()-std::size_t(offset))
    throw FormatError("Vista widget cache read escapes its retained source bank");
  return bank.subspan(std::size_t(offset),size);
}
std::uint32_t add(std::uint32_t address,std::uint32_t offset) {
  const auto result=std::uint64_t(address)+offset;
  if(result>0xffffffffull)throw FormatError("Vista widget cache source address wraps source32");
  return std::uint32_t(result);
}
} // namespace

std::array<std::pair<std::uint32_t,std::uint32_t>,5> VistaWidgetCachePointers::bindings() const {
  return {{{0x87cd50u,tag_table},{0x87cd54u,tag_base},{0x882290u,block_base},
           {0x879e74u,scenario},{0x879e70u,globals}}};
}
std::uint32_t VistaWidgetCachePointers::value(std::uint32_t address) const {
  for(const auto& [at,value]:bindings())if(at==address)return value;
  throw FormatError("Unowned Vista widget loaded-cache global address");
}

VistaWidgetCacheBank::VistaWidgetCacheBank(std::span<const std::byte> input) {
  checked(input,0,2048);
  const auto u32=[&](std::size_t at){return scalar(checked(input,at,4));};
  if(input.size()>512ull*1024*1024 || u32(0)!=0x68656164u || u32(4)!=8 ||
      u32(8)!=input.size() || u32(2044)!=0x666f6f74u)
    throw FormatError("Unsupported or truncated Vista widget cache");
  constexpr std::string_view build="11081.07.04.30.0934.main";
  const auto raw_build=checked(input,0x12c,32);
  for(std::size_t i=0;i<build.size();++i)
    if(raw_build[i]!=std::byte{std::uint8_t(build[i])})
      throw FormatError("Unsupported Vista widget cache build");
  if(raw_build[build.size()]!=std::byte{})throw FormatError("Unsupported Vista widget cache build suffix");
  file_size_=u32(8);index_=u32(16);
  if(index_<2048)throw FormatError("Vista widget tag region overlaps the file header");
  checked(input,index_,32);
  count_=u32(std::size_t(index_)+24);
  if(!count_ || count_>65536 || count_!=u32(0x2cc))
    throw FormatError("Invalid Vista widget tag directory count");
  table_=u32(std::size_t(index_)+8);
  checked(input,std::uint64_t(index_)+table_,std::size_t(count_)*16);
  scenario_=u32(std::size_t(index_)+12);globals_=u32(std::size_t(index_)+16);
  std::copy_n(input.begin(),header_.size(),header_.begin());
  region_.assign(input.begin()+index_,input.end());
  // Root projections are checked before publication. Low-WORD lookup remains
  // literal; the roots are not silently changed to another tag or to NULL.
  (void)tag_address(0,scenario_);(void)tag_address(0,globals_);
}
void VistaWidgetCacheBank::require_base(std::uint32_t base) const {
  if(std::uint64_t(base)+region_.size()>0x100000000ull)
    throw FormatError("Vista widget cache mapping wraps source32");
}
std::span<const std::byte> VistaWidgetCacheBank::relative(std::uint64_t offset,std::size_t size) const {
  return checked(region_,offset,size);
}
std::span<const std::byte> VistaWidgetCacheBank::index_header() const {return relative(0,32);}
std::span<const std::byte> VistaWidgetCacheBank::tag_table() const {return relative(table_,std::size_t(count_)*16);}
std::span<const std::byte> VistaWidgetCacheBank::directory_row(std::uint32_t ordinal) const {
  if(ordinal>=count_)throw FormatError("Vista widget directory ordinal is outside its source count");
  return relative(std::uint64_t(table_)+std::uint64_t(ordinal)*16,16);
}
std::span<const std::byte> VistaWidgetCacheBank::bytes(std::uint32_t base,std::uint32_t address,std::size_t size) const {
  require_base(base);
  if(address<base || size>0x100000000ull-address)
    throw FormatError("Vista widget cache read is unmapped or wraps source32");
  return relative(address-base,size);
}
std::uint32_t VistaWidgetCacheBank::read(std::uint32_t base,std::uint32_t address,std::size_t size) const {
  if(size!=1 && size!=2 && size!=4)throw FormatError("Invalid Vista widget source scalar width");
  return scalar(bytes(base,address,size));
}
std::uint32_t VistaWidgetCacheBank::source_address(std::uint32_t base,std::uint32_t offset,std::size_t size) const {
  require_base(base);relative(offset,size);
  return add(base,offset);
}
std::uint32_t VistaWidgetCacheBank::tag_address(std::uint32_t base,std::uint32_t datum) const {
  require_base(base);
  const auto offset=scalar(relative(std::uint64_t(table_)+std::uint64_t(datum&0xffffu)*16+8,4));
  return source_address(base,offset);
}
std::span<const std::byte> VistaWidgetCacheBank::tag_metadata(std::uint32_t base,std::uint32_t datum) const {
  const auto row=std::uint64_t(table_)+std::uint64_t(datum&0xffffu)*16;
  const auto offset=scalar(relative(row+8,4));const auto size=scalar(relative(row+12,4));
  return bytes(base,source_address(base,offset),size);
}
std::uint32_t VistaWidgetCacheBank::block_address(std::uint32_t base,std::uint32_t block) const {
  const auto offset=read(base,add(block,4),4);
  return offset==0xffffffffu ? 0u : source_address(base,offset);
}
VistaWidgetCachePointers VistaWidgetCacheBank::loaded_pointers(std::uint32_t base) const {
  require_base(base);
  return {source_address(base,table_,std::size_t(count_)*16),base,base,
          tag_address(base,scenario_),tag_address(base,globals_)};
}
} // namespace halo2
