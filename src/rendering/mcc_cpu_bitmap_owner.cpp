#include "rendering/mcc_cpu_bitmap_owner.hpp"
#include "halo2/mcc_lens_bitmap_layout.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
namespace halo2::private_cpu_bitmap {
bool raw_resource_format(unsigned format) noexcept {
 switch(format){
 case 0:case 1:case 2:case 3:case 6:case 8:case 9:case 10:case 11:case 22:case 23:return true;
 default:return false;
 }
}
bool cpu_sampling_format(unsigned format) noexcept {
 return format==10||format==11||(format>=14&&format<=16);
}
Owner project(const Tag&tag,unsigned frame,std::span<const std::byte> root,unsigned first,
 std::span<const std::byte> rows,const BitmapResource&resource,const Texture&texture,bool resource_only){
 if(tag.group!="bitm"||tag.name.empty()||tag.handle().is_null()||!tag.address||tag.size<0x4c||root.size()!=tag.size||std::uint64_t(tag.address)+tag.size>0x100000000ull)
  throw FormatError("CPU bitmap requires the complete typed named source root");
 const Bytes r(root);const auto count=r.u32(0x44);
 if(!count||count>65536||frame>=count||frame>0x7fff||!first||r.u32(0x48)!=first||std::uint64_t(first)+std::uint64_t(count)*168>0x100000000ull||rows.size()!=std::size_t(count)*168)
  throw FormatError("CPU bitmap frame/whole168-stride source bank is unowned");
 const auto root_end=std::uint64_t(tag.address)+tag.size,rows_end=std::uint64_t(first)+rows.size();
 if(std::uint64_t(first)<root_end&&std::uint64_t(tag.address)<rows_end){
  if(first<tag.address||rows_end>root_end||!std::ranges::equal(root.subspan(first-tag.address,rows.size()),rows))
   throw FormatError("CPU bitmap shared root/bank bytes differ or partially alias");
 }
 const auto row=rows.subspan(std::size_t(frame)*168,168);const Bytes d(row);
 std::array<std::byte,24> common{};std::vector<std::byte> encoded;
 const bool cube=resource_only&&resource.type==2;
 if(cube){
  if(d.u32(0)!=tag_groups::bitmap.value||resource.width!=d.u16(4)||resource.height!=d.u16(6)||
     resource.depth!=std::to_integer<unsigned>(row[8])||resource.type!=d.u16(10)||resource.format!=d.u16(12)||
     resource.width!=resource.height||resource.depth!=1||resource.format<14||resource.format>16||
     texture.layers!=6||texture.depth||texture.width!=resource.width||texture.height!=resource.height||
     texture.format!=resource.format||resource.levels[0].empty())
   throw FormatError("Inline cube requires its complete actual compressed six-face resource");
  for(unsigned n=0;n<3;++n){const auto stored=d.u32(0x50+n*4);
   if((stored&&(stored<8||stored>256u*1024u*1024u||resource.levels[n].empty()))||
      (!stored&&!resource.levels[n].empty())||resource.levels[n].size()>256u*1024u*1024u)
    throw FormatError("Inline cube lost its declared complete texture partitions");
  }
  const auto prepared=prepare_mcc_cubemap(Bytes(resource.levels[0]),resource.width,resource.format,d.u16(14)&8u,d.u16(20));
  if(prepared.pixels!=texture.pixels||prepared.mips.size()!=texture.mips.size()||
     !std::equal(prepared.mips.begin(),prepared.mips.end(),texture.mips.begin(),[](const auto&a,const auto&b){return a.width==b.width&&a.height==b.height&&a.offset==b.offset&&a.size==b.size&&a.depth==b.depth;}))
   throw FormatError("Inline cube differs from actual MCC face/mip conversion");
  std::copy_n(row.begin(),common.size(),common.begin());
  encoded.assign(texture.pixels.begin(),texture.pixels.end());
 }else{
  const auto layout=project_mcc_lens_bitmap_layout(tag.handle(),frame,row,resource,texture);common=layout.common;
  encoded=resource.levels[0];
 }
 if((d.u16(14)&0x18u)||!std::has_single_bit(resource.width)||!std::has_single_bit(resource.height))
  throw FormatError("CPU bitmap requires proved non-linear non-swizzled power-of-two storage");
 if(resource.format>=14 && resource.format<=16){
  if(texture.format!=resource.format||!(d.u16(14)&2u)||(!cube&&
      (resource.levels[0].size()<texture.pixels.size()||!std::equal(texture.pixels.begin(),texture.pixels.end(),resource.levels[0].begin()))))
   throw FormatError("CPU bitmap requires the same retained encoded DXT1/DXT3/DXT5 resource/image");
 }else if(resource.format==10 || resource.format==11 || (resource_only&&raw_resource_format(resource.format))){
  // Retain the complete original raw storage. Its stride comes from the
  // unchanged TU1.5 format table; Cache RGBA is an independently validated
  // decoded view, never replacement bytes in the native resource header.
  const auto bits=source_bitmap_format_bits(resource.format);
  if(!bits||bits%8)throw FormatError("Raw bitmap resource lacks a complete original byte stride");
  std::size_t expected{};auto width=resource.width,height=resource.height;
  for(unsigned level=0;level<=d.u16(20);++level){expected+=std::size_t(width)*height*(bits/8u);width=std::max(1u,width/2u);height=std::max(1u,height/2u);}
  if(texture.format!=11||d.u16(14)!=0x201u||resource.levels[0].size()!=expected)
   throw FormatError("Raw bitmap resource requires complete non-linear non-swizzled mip storage");
  const auto prepared=prepare_texture(Bytes(resource.levels[0]),resource.width,resource.height,resource.format,false,d.u16(20));
  if(prepared.pixels!=texture.pixels||prepared.mips.size()!=texture.mips.size()||!std::equal(prepared.mips.begin(),prepared.mips.end(),texture.mips.begin(),[](const auto&a,const auto&b){return a.width==b.width&&a.height==b.height&&a.offset==b.offset&&a.size==b.size&&a.depth==b.depth;}))
   throw FormatError("Raw bitmap resource differs from existing Cache RGBA preparation");
 }else throw FormatError("CPU bitmap format is outside proved DXT1/DXT3/DXT5/format10/11 domain");
 Owner result{tag,frame,count,first,{root.begin(),root.end()},{rows.begin(),rows.end()},std::move(encoded),common,
  {std::bit_cast<std::int16_t>(d.u16(4)),std::bit_cast<std::int16_t>(d.u16(6)),std::bit_cast<std::int16_t>(d.u16(12)),std::bit_cast<std::int16_t>(d.u16(20)),std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(row[8])),d.u16(10),d.u16(14)}};
 (void)source_bitmap_pending_header(result.metadata,0,0);return result;
}
Owner load(const Cache&cache,TagHandle bitmap,unsigned frame,bool resource_only){
 if(!cache.is_mcc())throw FormatError("CPU bitmap compatibility requires MCC");
 const auto*t=cache.query_tag({.handle=bitmap,.group=tag_groups::bitmap,.require_address=true});
 if(!t||t->handle()!=bitmap||t->size<0x4c)throw FormatError("CPU bitmap full-salt source owner is absent");
 const Tag tag=*t;const auto root=cache.read_tag_bytes(tag.address,tag.size);const Bytes r(root);const auto count=r.u32(0x44),first=r.u32(0x48);
 if(!count||count>65536||frame>=count||frame>0x7fff||count!=cache.bitmap_count(bitmap.value))throw FormatError("CPU bitmap count/frame differs from its Cache owner");
 const auto rows=cache.read_tag_bytes(first,std::size_t(count)*168);const auto resource=cache.load_bitmap_resource(bitmap.value,frame);const auto texture=cache.load_bitmap(bitmap.value,frame);
 return project(tag,frame,root,first,rows,resource,texture,resource_only);
}

PredictionPartitions PredictionPartitions::load(const Cache& cache, TagHandle bitmap, unsigned frame) {
    const auto* tag = cache.query_tag({.handle=bitmap, .group=tag_groups::bitmap, .require_address=true});
    if (!cache.is_mcc() || !tag || tag->handle() != bitmap || tag->size < 0x4c)
        throw FormatError("Bitmap prediction requires an owned MCC bitmap tag");

    const Bytes root(cache.read_tag_bytes(tag->address, tag->size));
    const auto count = root.u32(0x44);
    if (frame >= count || count != cache.bitmap_count(bitmap.value))
        throw FormatError("Bitmap prediction frame escapes its MCC bitmap bank");
    constexpr std::size_t row_size = 168;
    const auto rows = cache.read_tag_bytes(root.u32(0x48), std::size_t(count) * row_size);
    const auto row_bytes = rows.subspan(std::size_t(frame) * row_size, row_size);
    const Bytes row(row_bytes);
    if (row.u32(0) != tag_groups::bitmap.value)
        throw FormatError("Bitmap prediction frame lacks its authored bitmap signature");
    auto resource = cache.load_bitmap_resource(bitmap.value, frame);
    if (resource.width != row.u16(4) || resource.height != row.u16(6) ||
        resource.depth != std::to_integer<unsigned>(row_bytes[8]) ||
        resource.type != row.u16(10) || resource.format != row.u16(12))
        throw FormatError("Bitmap prediction payload metadata differs from its authored row");

    PredictionPartitions result;
    result.bitmap_ = bitmap;
    std::ranges::copy(row_bytes.first(result.common_.size()), result.common_.begin());
    for (unsigned level = 0; level < result.descriptors_.size(); ++level) {
        const auto stored = row.u32(80 + level * 4);
        const auto decoded = resource.levels[level].size();
        if ((!stored && decoded) || (stored && (stored < 8 || !decoded || decoded > 128u * 1024u * 1024u)))
            throw FormatError("Bitmap prediction partition differs from its declared texture extent");
        result.descriptors_[level] = {row.u32(56 + level * 4), stored, static_cast<std::uint32_t>(decoded)};
    }
    result.payloads_ = std::move(resource.levels);
    return result;
}

std::optional<PredictionPartitions::Descriptor> PredictionPartitions::descriptor(unsigned partition) const {
    if (partition >= descriptors_.size())
        throw FormatError("Bitmap prediction partition is outside the original cache descriptor");
    const auto descriptor = descriptors_[partition];
    if (!descriptor.stored_bytes) return std::nullopt;
    return descriptor;
}

std::array<std::byte, 116> PredictionPartitions::cold_source_row() const {
    if (!descriptor(0) || !(std::to_integer<unsigned>(common_[15]) & 2u) ||
        !source_bitmap_format_bits(Bytes(common_).u16(12)))
        throw FormatError("Bitmap prediction frame lacks an admitted hardware cache descriptor");
    std::array<std::byte, 116> row{};
    std::ranges::copy(common_, row.begin());
    constexpr std::uint32_t absent = 0xffffffffu;
    const auto put = [&](unsigned offset, std::uint32_t word) {
        for (unsigned byte = 0; byte < 4; ++byte) row[offset + byte] = std::byte(word >> (byte * 8));
    };
    for (unsigned level = 0; level < descriptors_.size(); ++level) {
        const auto entry = descriptor(level);
        put(0x1c + level * 4, entry ? entry->encoded_offset : absent);
        put(0x28 + level * 4, absent);
        put(0x34 + level * 4, entry ? entry->payload_bytes : 0);
    }
    put(0x4c, bitmap_.value);
    return row;
}

std::vector<std::byte> PredictionPartitions::read_sectors(unsigned partition, std::uint32_t encoded_offset,
                                                         std::uint32_t transfer_bytes) const {
    const auto input = descriptor(partition);
    // Original 130183..130194 rounds the declared image extent to a sector.
    if (!input || input->encoded_offset != encoded_offset ||
        transfer_bytes != ((input->payload_bytes + 511u) & ~511u))
        throw FormatError("Bitmap prediction read differs from its owned partition and sector extent");
    std::vector<std::byte> bytes(transfer_bytes);
    std::ranges::copy(payloads_[partition], bytes.begin());
    return bytes;
}

namespace {
constexpr std::uint32_t prediction_first = 0xb1000000u, prediction_end = 0xb2000000u;
constexpr unsigned prediction_row_bytes = 116, prediction_binding_bytes = prediction_row_bytes + 4;
constexpr std::string_view prediction_owner = "MCC authored bitmap prediction row";

bool prediction_address(std::uint32_t row) {
    return row >= prediction_first && row < prediction_end;
}

void validate_prediction_row(std::span<const std::byte> bytes, const PredictionPartitions& partitions) {
    const auto initial = partitions.cold_source_row();
    // The explicit partition-0 worker writes the handle and backing pointer.
    // The native release path can change those words. All other descriptor
    // bytes, including the full-salt tag at +0x4c, belong to the map.
    for (const auto [first, last] : {std::pair{0u, 0x28u}, {0x2cu, 0x40u}, {0x44u, 0x54u}, {0x58u, prediction_row_bytes}})
        if (!std::equal(initial.begin() + first, initial.begin() + last, bytes.begin() + first))
            throw FormatError("Bitmap prediction row changed its authored descriptor");
}
}

std::uint32_t PredictionRows::bind(SourceRuntimeMemory& memory, const Cache& cache, TagHandle bitmap, unsigned frame) {
    const Key key{bitmap.value, frame};
    if (const auto found = rows_.find(key); found != rows_.end()) return found->second;
    const auto initial = PredictionPartitions::load(cache, bitmap, frame).cold_source_row();
    const auto address = memory.find_unowned_range(prediction_first, prediction_end, prediction_binding_bytes, 4);
    if (!address) throw FormatError("World exhausted its bitmap prediction row space");
    std::array<std::byte, prediction_binding_bytes> bytes{};
    std::ranges::copy(initial, bytes.begin());
    for (unsigned i = 0; i < 4; ++i) bytes[prediction_row_bytes + i] = std::byte(frame >> (8 * i));
    const auto index = rows_.emplace(key, *address).first;
    try {
        memory.mount(*address, bytes, SourceRuntimeMemory::Access::ReadWrite, std::string(prediction_owner));
    } catch (...) {
        rows_.erase(index);
        throw;
    }
    return *address;
}

std::optional<std::vector<std::byte>> PredictionRows::read(const SourceRuntimeMemory& memory, const Cache& cache,
    std::uint32_t row, unsigned partition, std::uint32_t offset, std::uint32_t size) const {
    if (!prediction_address(row)) return std::nullopt;
    const auto bytes = memory.view(row, prediction_binding_bytes);
    const Bytes binding(bytes);
    const Key key{binding.u32(0x4c), binding.u32(prediction_row_bytes)};
    const auto found = rows_.find(key);
    if (found == rows_.end() || found->second != row || partition != 0)
        throw FormatError("Bitmap prediction read has no retained map row and partition");
    const auto partitions = PredictionPartitions::load(cache, {key.first}, key.second);
    validate_prediction_row(bytes, partitions);
    return partitions.read_sectors(partition, offset, size);
}

PredictionRows PredictionRows::restore(const SourceRuntimeMemory& memory, const Cache& cache) {
    PredictionRows result;
    memory.visit_region_metadata([&](const auto& region) {
        const bool overlaps = region.address < prediction_end &&
            std::uint64_t(region.address) + region.bytes > prediction_first;
        if (!overlaps && region.owner != prediction_owner) return;
        if (!prediction_address(region.address) || region.address % 4 ||
            region.address > prediction_end - prediction_binding_bytes || region.owner != prediction_owner ||
            region.bytes != prediction_binding_bytes || region.page_bytes ||
            region.access != SourceRuntimeMemory::Access::ReadWrite)
            throw FormatError("World authority has an invalid bitmap prediction row owner");
        const auto bytes = memory.view(region.address, prediction_binding_bytes);
        const Bytes binding(bytes);
        const Key key{binding.u32(0x4c), binding.u32(prediction_row_bytes)};
        validate_prediction_row(bytes, PredictionPartitions::load(cache, {key.first}, key.second));
        if (!result.rows_.emplace(key, region.address).second)
            throw FormatError("World authority repeats a bitmap prediction frame");
    });
    return result;
}

std::array<unsigned,5> Owner::header(unsigned address)const{
 if(address<0x80000000u||std::uint64_t(address)+encoded.size()>0x90000000ull||encoded.empty())throw FormatError("CPU bitmap needs an actual represented original physical alias");
 return source_bitmap_pending_header(metadata,0,address);
}
Delivery delivery(const Owner&owner,const SourceRuntimeMemory&bank,
 NativeRuntimeAddress actual_encoded,NativeRuntimeAddress actual_header){
 const auto encoded=bank.source_address(actual_encoded,owner.encoded.size());
 const auto header=bank.source_address(actual_header,20);
 const auto words=owner.header(encoded);
 if(std::uint64_t(encoded)<std::uint64_t(header)+20&&std::uint64_t(header)<std::uint64_t(encoded)+owner.encoded.size())
  throw FormatError("CPU bitmap header aliases its encoded resource");
 if(!std::ranges::equal(bank.view(actual_encoded,owner.encoded.size()),owner.encoded))
  throw FormatError("CPU bitmap encoded binding differs from actual retained bytes");
 const Bytes existing(bank.view(actual_header,20));for(unsigned n=0;n<5;++n)
  if(existing.u32(n*4)!=words[n])throw FormatError("CPU bitmap header binding differs from actual encoded owner");
 const auto clock=Bytes(bank.view(0x4f2d58u,4)).u32(0);
 if(clock>0x7ffffffdu)throw FormatError("CPU bitmap source cache clock exceeds proved signed frame range");
 // Actual TU1.5 12F8DF/12F8E5/12F8E8: selected partition0 header caches
 // current source clock+2. This planner requires the actual CPU bytes/header
 // above; it creates neither an image-pool handle nor a ready flag.
 return {owner.common,header,encoded,static_cast<unsigned>(owner.encoded.size()),clock,clock+2u};
}

}
