#include "assets/tag_owner_index.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include <algorithm>

namespace halo2 {
namespace {
struct Block {std::uint32_t address{};unsigned count{};};
class Reader {
public:
  explicit Reader(const Cache &cache):cache_(cache){}
  std::uint32_t tag(std::uint32_t datum,const char *group) {
    const auto found=cache_.query_tag({.handle=TagHandle{datum}, .group=TagGroup::from_string(group), .require_address=true, .minimum_size=1});
    if(!found)throw FormatError("explicit source pass tag missing or wrong group");
    (void)cache_.tag_payload(found->handle());return found->address;
  }
  std::span<const std::byte> bytes(std::uint32_t address,std::size_t extent) {
    if(!extent || std::uint64_t(address)+extent>(std::uint64_t{1}<<32))
      throw FormatError("explicit source pass pointer has invalid extent");
    const Tag *found=nullptr;
    for(const auto *candidate:indexed_tag_owners(cache_,{.contains=TagAddressRange{address,extent}, .require_address=true})) {
    const auto &t=*candidate;
      if(found)throw FormatError("explicit source pass pointer has ambiguous backing owner");
      found=&t;
    }
    if(!found)throw FormatError("explicit source pass pointer escapes all owned tag payloads");
    (void)cache_.tag_payload(found->handle());
    const MenuWorldSourceMetadataRead read{found->datum,address,extent};
    if(std::find(reads.begin(),reads.end(),read)==reads.end())reads.push_back(read);
    return cache_.read_tag_bytes(address,extent);
  }
  Block block(std::uint32_t header,unsigned stride) {
    const auto descriptor=read_tag_block_descriptor(Bytes(bytes(header,8)),0);
    const auto extent=descriptor.extent(stride,65536);
    if(!descriptor.count)return {};
    (void)bytes(descriptor.address,extent);
    return {descriptor.address,unsigned(descriptor.count)};
  }
  std::vector<MenuWorldSourceMetadataRead> reads;
private:
  const Cache &cache_;
};
std::uint32_t entry(Block block,unsigned ordinal,unsigned stride) {
  if(ordinal>=block.count)throw FormatError("explicit source pass ordinal outside block");
  return block.address+ordinal*stride;
}
void range(ShaderRange range,Block block) {
  if(range.first()>block.count || range.count()>block.count-range.first())
    throw FormatError("explicit source pass packed range escapes block");
}
template<std::size_t N>void copy(std::array<std::byte,N> &out,std::span<const std::byte> in) {
  if(in.size()!=N)throw FormatError("explicit source pass record has invalid size");
  std::copy(in.begin(),in.end(),out.begin());
}
}
MenuWorldSourcePass load_menu_world_source_pass(const Cache &cache,std::uint32_t shader,
    const MenuWorldSourcePassChoice &choice) {
  Reader reader(cache);MenuWorldSourcePass result;result.shader_=shader;result.choice_=choice;
  const auto root=reader.tag(shader,"shad");
  const auto definitions=reader.block(root+32,124);
  if(definitions.count!=1)throw FormatError("explicit source pass requires one ordinary shader postprocess definition");
  const Bytes definition(reader.bytes(definitions.address,124));result.template_=definition.u32(0);
  const auto template_root=reader.tag(result.template_,"stem");
  const auto templates=reader.block(template_root+88,40);
  if(templates.count!=1)throw FormatError("explicit source pass requires one template postprocess definition");
  const auto pp=templates.address;
  const auto lods=reader.block(pp,10),layers=reader.block(pp+8,2),passes=reader.block(pp+16,10),remaps=reader.block(pp+24,6);
  result.lod_address_=entry(lods,choice.shader_lod,10);
  copy(result.lod_record_,reader.bytes(result.lod_address_,10));const Bytes lod(result.lod_record_);
  result.lod_layers_={lod.u16(0)};range(result.lod_layers_,layers);
  if(choice.layer>=32 || choice.layer>=result.lod_layers_.count() ||
      !(lod.u32(2)&(std::uint32_t{1}<<choice.layer)))
    throw FormatError("explicit source pass layer unavailable in supplied shader LOD");
  result.layer_address_=entry(layers,result.lod_layers_.first()+choice.layer,2);
  result.layer_passes_={Bytes(reader.bytes(result.layer_address_,2)).u16(0)};range(result.layer_passes_,passes);
  if(choice.pass_in_layer>=result.layer_passes_.count())
    throw FormatError("explicit source pass entry outside supplied layer");
  result.template_pass_=result.layer_passes_.first()+choice.pass_in_layer;
  result.template_pass_address_=entry(passes,result.template_pass_,10);
  copy(result.pass_record_,reader.bytes(result.template_pass_address_,10));
  const Bytes pass(result.pass_record_);result.pass_=pass.u32(4);result.implementations_={pass.u16(8)};
  range(result.implementations_,remaps);
  if(choice.implementation>=result.implementations_.count())
    throw FormatError("explicit source pass implementation outside supplied pass range");
  result.template_remap_=result.implementations_.first()+choice.implementation;
  result.template_remap_address_=entry(remaps,result.template_remap_,6);
  copy(result.remap_record_,reader.bytes(result.template_remap_address_,6));
  const auto pass_root=reader.tag(result.pass_,"spas");
  const auto postprocess=reader.block(pass_root+28,88);
  if(postprocess.count!=1)throw FormatError("explicit source pass requires one pass postprocess definition");
  result.pass_postprocess_address_=postprocess.address;
  const auto implementations=reader.block(postprocess.address,306);
  if(result.implementations_.count()>implementations.count)
    throw FormatError("explicit template implementation range exceeds shader pass");
  result.implementation_address_=entry(implementations,choice.implementation,306);
  copy(result.implementation_record_,reader.bytes(result.implementation_address_,306));
  result.vertex_=Bytes(result.implementation_record_).u32(256);
  (void)reader.tag(result.vertex_,"vrtx"); // Validate identity; binding owns the import.
  result.reads_=std::move(reader.reads);return result;
}
} // namespace halo2
