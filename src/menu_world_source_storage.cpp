#include "halo2/menu_world_source_storage.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <tuple>

namespace halo2 {
namespace {
void check(VkResult result,const char *operation) {
  if(result!=VK_SUCCESS)
    throw FormatError(std::string(operation)+": Vulkan result "+std::to_string(result));
}
VkDeviceSize checked_add(VkDeviceSize a,VkDeviceSize b) {
  if(b>std::numeric_limits<VkDeviceSize>::max()-a)
    throw FormatError("source world storage buffer extent overflow");
  return a+b;
}
VkDeviceSize align_four(VkDeviceSize value) {
  return checked_add(value,3)&~VkDeviceSize{3};
}
VkFormat vertex_format(MenuWorldSourceFormat format) {
  switch(format) {
  case MenuWorldSourceFormat::float32x3:return VK_FORMAT_R32G32B32_SFLOAT;
  case MenuWorldSourceFormat::float32x2:return VK_FORMAT_R32G32_SFLOAT;
  case MenuWorldSourceFormat::snorm16x3:return VK_FORMAT_R16G16B16_SNORM;
  case MenuWorldSourceFormat::snorm16x2:return VK_FORMAT_R16G16_SNORM;
  // The source VS decodes signed11/11/10 from this unchanged signed word.
  // Vulkan10/10/10/2 normalized formats are not this source declaration.
  case MenuWorldSourceFormat::packed_signed_11_11_10:return VK_FORMAT_R32_SINT;
  case MenuWorldSourceFormat::unorm8x1:return VK_FORMAT_R8_UNORM;
  case MenuWorldSourceFormat::unorm8x2:return VK_FORMAT_R8G8_UNORM;
  case MenuWorldSourceFormat::unorm8x3:return VK_FORMAT_R8G8B8_UNORM;
  case MenuWorldSourceFormat::unorm8x4:return VK_FORMAT_R8G8B8A8_UNORM;
  }
  throw FormatError("unsupported source world storage vertex format");
}
bool same_section(const MenuWorldSourceSection &a,const MenuWorldSourceSection &b) {
  if(a.kind()!=b.kind() || a.owner_datum()!=b.owner_datum() || a.section()!=b.section() ||
     a.vertex_count()!=b.vertex_count() || a.model_geometry_class()!=b.model_geometry_class() ||
     a.compression_flags()!=b.compression_flags() ||
     std::memcmp(a.compression_rows().data(),b.compression_rows().data(),sizeof(a.compression_rows())) ||
     a.descriptors()!=b.descriptors() || !std::ranges::equal(a.node_map(),b.node_map()) ||
     !std::ranges::equal(a.raw_header(),b.raw_header()) ||
     !std::ranges::equal(a.raw_model_header(),b.raw_model_header()) ||
     !std::ranges::equal(a.indices(),b.indices()) ||
     a.resources().size()!=b.resources().size() || a.streams().size()!=b.streams().size() ||
     a.parts().size()!=b.parts().size())return false;
  for(unsigned i=0;i<a.resources().size();++i) {
    const auto &x=a.resources()[i],&y=b.resources()[i];
    if(x.type!=y.type || x.channel!=y.channel || x.raw_offset!=y.raw_offset || x.bytes!=y.bytes)return false;
  }
  for(unsigned i=0;i<a.streams().size();++i) {
    const auto &x=a.streams()[i],&y=b.streams()[i];
    if(std::tuple{x.semantic,x.format,x.resource,x.byte_offset,x.byte_stride,x.descriptor}!=
       std::tuple{y.semantic,y.format,y.resource,y.byte_offset,y.byte_stride,y.descriptor})return false;
  }
  for(unsigned i=0;i<a.parts().size();++i) {
    const auto &x=a.parts()[i],&y=b.parts()[i];
    if(x.bytes!=y.bytes || x.type!=y.type || x.flags!=y.flags || x.index_first!=y.index_first ||
       x.index_count!=y.index_count || x.material!=y.material || x.shader!=y.shader)return false;
  }
  return true;
}
struct Buffer {
  explicit Buffer(VkDevice owner):device(owner){}
  ~Buffer() {
    if(buffer)vkDestroyBuffer(device,buffer,nullptr);
    if(memory)vkFreeMemory(device,memory,nullptr);
  }
  Buffer(const Buffer &)=delete;
  Buffer &operator=(const Buffer &)=delete;
  VkDevice device{};
  VkBuffer buffer{};
  VkDeviceMemory memory{};
  VkDeviceSize size{};
};
struct IndexBuffer {
  std::unique_ptr<Buffer> storage;
  std::uint32_t count{};
};
struct SectionBuffers {
  std::shared_ptr<const MenuWorldSourceSection> source;
  std::unique_ptr<Buffer> storage;
  std::vector<MenuWorldSourceBufferSlice> resources;
  std::map<std::tuple<unsigned,MenuWorldSourceTopology,unsigned,unsigned>,IndexBuffer> indices;
};
}

struct MenuWorldSourceStorage::Impl {
  MenuWorldSourceStorageContext context;
  VkPhysicalDeviceProperties properties{};
  VkPhysicalDeviceMemoryProperties memory_properties{};
  std::map<VkFormat,bool> formats;
  using SectionKey=std::tuple<MenuWorldSourceSectionKind,std::uint32_t,unsigned>;
  std::map<SectionKey,std::unique_ptr<SectionBuffers>> sections;
  std::vector<MenuWorldSourceStoredDraw> draws;
  explicit Impl(MenuWorldSourceStorageContext input):context(input) {
    if(!context.device || !context.physical)
      throw FormatError("incomplete source world storage context");
    vkGetPhysicalDeviceProperties(context.physical,&properties);
    vkGetPhysicalDeviceMemoryProperties(context.physical,&memory_properties);
  }
  std::unique_ptr<Buffer> upload(std::span<const std::byte> bytes,VkBufferUsageFlags usage) {
    if(bytes.empty())throw FormatError("source world storage cannot upload an empty buffer");
    auto result=std::make_unique<Buffer>(context.device);
    result->size=bytes.size();
    VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    create.size=result->size;create.usage=usage|VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    create.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    check(vkCreateBuffer(context.device,&create,nullptr,&result->buffer),"create original world geometry buffer");
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(context.device,result->buffer,&requirements);
    unsigned selected=memory_properties.memoryTypeCount;
    bool coherent=false;
    for(unsigned i=0;i<memory_properties.memoryTypeCount;++i) {
      const auto flags=memory_properties.memoryTypes[i].propertyFlags;
      if(!(requirements.memoryTypeBits&(1u<<i)) || !(flags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))continue;
      if(selected==memory_properties.memoryTypeCount)selected=i;
      if(flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {selected=i;coherent=true;break;}
    }
    if(selected==memory_properties.memoryTypeCount)
      throw FormatError("source world geometry lacks host-visible upload memory");
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=selected;
    check(vkAllocateMemory(context.device,&allocation,nullptr,&result->memory),"allocate original world geometry buffer");
    check(vkBindBufferMemory(context.device,result->buffer,result->memory,0),"bind original world geometry buffer");
    void *mapped{};
    check(vkMapMemory(context.device,result->memory,0,VK_WHOLE_SIZE,0,&mapped),"map original world geometry buffer");
    struct Mapping {
      VkDevice device;VkDeviceMemory memory;
      ~Mapping(){vkUnmapMemory(device,memory);}
    } mapping{context.device,result->memory};
    std::memcpy(mapped,bytes.data(),bytes.size());
    if(!coherent) {
      VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
      range.memory=result->memory;range.offset=0;range.size=VK_WHOLE_SIZE;
      check(vkFlushMappedMemoryRanges(context.device,1,&range),"flush original world geometry buffer");
    }
    return result;
  }
  void require_format(VkFormat format) {
    auto [found,inserted]=formats.try_emplace(format);
    if(inserted) {
      VkFormatProperties support{};
      vkGetPhysicalDeviceFormatProperties(context.physical,format,&support);
      found->second=bool(support.bufferFeatures&VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT);
    }
    if(!found->second)
      throw FormatError("source world geometry fetch format unsupported by device: "+std::to_string(format));
  }
  SectionBuffers &section(const MenuWorldSourceDraw &draw) {
    if(!draw.geometry)throw FormatError("source world storage draw lacks immutable geometry");
    const auto &source=*draw.geometry;
    const SectionKey key{source.kind(),source.owner_datum(),source.section()};
    if(const auto found=sections.find(key);found!=sections.end()) {
      if(found->second->source!=draw.geometry && !same_section(*found->second->source,source))
        throw FormatError("source world storage section identity has different immutable content");
      return *found->second;
    }
    auto result=std::make_unique<SectionBuffers>();result->source=draw.geometry;
    VkDeviceSize total{};
    for(const auto &resource:source.resources()) {
      total=align_four(total);
      result->resources.push_back({VK_NULL_HANDLE,total,VkDeviceSize(resource.bytes.size())});
      total=checked_add(total,resource.bytes.size());
    }
    if(!total || total>std::numeric_limits<std::size_t>::max())
      throw FormatError("source world resource bank exceeds host upload extent");
    std::vector<std::byte> bytes(static_cast<std::size_t>(total));
    for(unsigned i=0;i<source.resources().size();++i) {
      const auto &resource=source.resources()[i];
      if(!resource.bytes.empty())
        std::memcpy(bytes.data()+result->resources[i].offset,resource.bytes.data(),resource.bytes.size());
    }
    // Keep every original resource, including its unchanged ushort index bank,
    // descriptor/part bytes and node map. Drawing uses a separate ordered bank.
    result->storage=upload(bytes,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    for(auto &slice:result->resources)slice.buffer=result->storage->buffer;
    return *sections.emplace(key,std::move(result)).first->second;
  }
  IndexBuffer &indices(SectionBuffers &section,const MenuWorldSourceDraw &draw) {
    const auto range=draw.index_range();
    const auto triangles=draw.triangle_count();
    const auto key=std::tuple{draw.part,draw.topology,range.first,range.count};
    if(const auto found=section.indices.find(key);found!=section.indices.end())return found->second;
    if(triangles>std::numeric_limits<std::uint32_t>::max()/3u ||
       triangles>std::numeric_limits<std::size_t>::max()/6u)
      throw FormatError("source world triangle list exceeds index count extent");
    IndexBuffer result;result.count=std::uint32_t(triangles*3);
    if(result.count) {
      std::vector<std::byte> bytes(triangles*6);
      for(std::size_t i=0;i<triangles;++i) {
        const auto triangle=draw.triangle(i);
        for(unsigned j=0;j<3;++j) {
          bytes[i*6+j*2]=std::byte(triangle[j]&255);
          bytes[i*6+j*2+1]=std::byte(triangle[j]>>8);
        }
      }
      result.storage=upload(bytes,VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }
    return section.indices.emplace(key,std::move(result)).first->second;
  }
  MenuWorldSourceStoredDraw stored(const MenuWorldSourceBinding &binding) {
    if(!binding.draw().geometry)
      throw FormatError("source world storage binding lacks immutable geometry");
    const auto &limits=properties.limits;
    if(binding.inputs().size()>limits.maxVertexInputAttributes)
      throw FormatError("source world attributes exceed device input limit");
    // Validate the complete fetch layout before allocating any new resources.
    std::map<unsigned,unsigned> resource_bindings;
    MenuWorldSourceStoredDraw result(binding);
    result.source_index_range=binding.draw().index_range();
    for(const auto &input:binding.inputs()) {
      const auto &stream=input.stream;
      if(input.vertex_register>=limits.maxVertexInputAttributes ||
         stream.byte_stride>limits.maxVertexInputBindingStride ||
         stream.byte_offset>limits.maxVertexInputAttributeOffset ||
         stream.resource>=binding.draw().geometry->resources().size())
        throw FormatError("source world vertex input exceeds device/section limits");
      const auto format=vertex_format(stream.format);require_format(format);
      auto [found,inserted]=resource_bindings.try_emplace(stream.resource,unsigned(resource_bindings.size()));
      if(inserted) {
        if(resource_bindings.size()>limits.maxVertexInputBindings)
          throw FormatError("source world streams exceed device input binding limit");
        result.vertex_bindings.push_back({found->second,stream.byte_stride,VK_VERTEX_INPUT_RATE_VERTEX});
      } else if(result.vertex_bindings.at(found->second).stride!=stream.byte_stride)
        throw FormatError("source world shared resource has inconsistent vertex strides");
      result.vertex_attributes.push_back({input.vertex_register,found->second,format,stream.byte_offset});
    }
    auto &buffers=section(binding.draw());result.resources=buffers.resources;
    result.vertex_buffers.resize(resource_bindings.size());result.vertex_offsets.resize(resource_bindings.size());
    for(const auto &[resource,ordinal]:resource_bindings) {
      const auto &slice=buffers.resources.at(resource);
      result.vertex_buffers[ordinal]=slice.buffer;result.vertex_offsets[ordinal]=slice.offset;
    }
    auto &ordered=indices(buffers,binding.draw());result.index_count=ordered.count;
    if(ordered.storage)result.indices={ordered.storage->buffer,0,ordered.storage->size};
    return result;
  }
  std::span<const MenuWorldSourceStoredDraw> store(std::span<const MenuWorldSourceBinding> bindings) {
    std::vector<MenuWorldSourceStoredDraw> next;next.reserve(bindings.size());
    for(const auto &binding:bindings)next.push_back(stored(binding));
    draws=std::move(next);return draws;
  }
};

MenuWorldSourceStorage::MenuWorldSourceStorage(MenuWorldSourceStorageContext context)
    :impl_(std::make_unique<Impl>(context)){}
MenuWorldSourceStorage::~MenuWorldSourceStorage()=default;
std::span<const MenuWorldSourceStoredDraw> MenuWorldSourceStorage::store(
    std::span<const MenuWorldSourceBinding> bindings){return impl_->store(bindings);}
} // namespace halo2
