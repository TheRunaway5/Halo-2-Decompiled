#include "mcc_cpu_bitmap_owner.hpp"
#include "halo2/mcc_lens_bitmap_layout.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
namespace halo2::private_cpu_bitmap {
Owner project(const Tag&tag,unsigned frame,std::span<const std::byte> root,unsigned first,
 std::span<const std::byte> rows,const BitmapResource&resource,const Texture&texture){
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
 const auto row=rows.subspan(std::size_t(frame)*168,168);const auto layout=project_mcc_lens_bitmap_layout(tag.handle(),frame,row,resource,texture);const Bytes d(row);
 if((d.u16(14)&0x18u)||!std::has_single_bit(resource.width)||!std::has_single_bit(resource.height))
  throw FormatError("CPU bitmap requires proved non-linear non-swizzled power-of-two storage");
 if(resource.format==16){
  if(texture.format!=16||!(d.u16(14)&2u)||resource.levels[0].size()<texture.pixels.size()||!std::equal(texture.pixels.begin(),texture.pixels.end(),resource.levels[0].begin()))
   throw FormatError("CPU bitmap requires the same retained encoded DXT5 resource/image");
 }else if(resource.format==11){
  if(texture.format!=11||d.u16(14)!=0x201u||d.u16(20)!=0||resource.levels[0].size()!=std::size_t(resource.width)*resource.height*4)
   throw FormatError("CPU bitmap requires proved complete raw BGRA format11 top-level storage");
  const auto prepared=prepare_texture(Bytes(resource.levels[0]),resource.width,resource.height,11,false,0);
  if(prepared.pixels!=texture.pixels||prepared.mips.size()!=texture.mips.size()||!std::equal(prepared.mips.begin(),prepared.mips.end(),texture.mips.begin(),[](const auto&a,const auto&b){return a.width==b.width&&a.height==b.height&&a.offset==b.offset&&a.size==b.size&&a.depth==b.depth;}))
   throw FormatError("CPU bitmap raw BGRA owner differs from existing Cache RGBA preparation");
 }else throw FormatError("CPU bitmap format is outside proved DXT5/format11 domain");
 Owner result{tag,frame,count,first,{root.begin(),root.end()},{rows.begin(),rows.end()},resource.levels[0],layout.common,
  {std::bit_cast<std::int16_t>(d.u16(4)),std::bit_cast<std::int16_t>(d.u16(6)),std::bit_cast<std::int16_t>(d.u16(12)),std::bit_cast<std::int16_t>(d.u16(20)),std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(row[8])),d.u16(10),d.u16(14)}};
 (void)source_bitmap_pending_header(result.metadata,0,0);return result;
}
Owner load(const Cache&cache,TagHandle bitmap,unsigned frame){
 if(!cache.is_mcc())throw FormatError("CPU bitmap compatibility requires MCC");
 const auto*t=cache.query_tag({.handle=bitmap,.group=tag_groups::bitmap,.require_address=true});
 if(!t||t->handle()!=bitmap||t->size<0x4c)throw FormatError("CPU bitmap full-salt source owner is absent");
 const Tag tag=*t;const auto root=cache.read_tag_bytes(tag.address,tag.size);const Bytes r(root);const auto count=r.u32(0x44),first=r.u32(0x48);
 if(!count||count>65536||frame>=count||frame>0x7fff||count!=cache.bitmap_count(bitmap.value))throw FormatError("CPU bitmap count/frame differs from its Cache owner");
 const auto rows=cache.read_tag_bytes(first,std::size_t(count)*168);const auto resource=cache.load_bitmap_resource(bitmap.value,frame);const auto texture=cache.load_bitmap(bitmap.value,frame);
 return project(tag,frame,root,first,rows,resource,texture);
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
