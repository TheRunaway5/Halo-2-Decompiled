#pragma once
#include "halo2/menu_world_source_binding.hpp"
#include <memory>
#include <utility>
#include <vulkan/vulkan.h>

namespace halo2 {
struct MenuWorldSourceStorageContext {
  VkDevice device{};
  VkPhysicalDevice physical{};
  bool storage_fetch{}; // Additional padded SSBO bank for triangle-vertex mode.
};
struct MenuWorldSourceBufferSlice {
  VkBuffer buffer{};
  VkDeviceSize offset{}, size{};
};
// Vertex bindings use dense ordinals and attributes keep their source register
// locations. Every buffer range belongs to the immutable geometry section.
struct MenuWorldSourceDrawGeometry {
  std::vector<MenuWorldSourceBufferSlice> resources; // Original resource ordinal.
  std::vector<VkVertexInputBindingDescription> vertex_bindings;
  std::vector<VkVertexInputAttributeDescription> vertex_attributes;
  std::vector<VkBuffer> vertex_buffers;
  std::vector<VkDeviceSize> vertex_offsets;
  // Original ordered strip triangles are expanded to a ushort triangle list,
  // including every degenerate and odd order(i,i+2,i+1). No vertex is decoded.
  MenuWorldSourceBufferSlice indices;
  std::uint32_t index_count{};
  VkIndexType index_type=VK_INDEX_TYPE_UINT16;
  VkPrimitiveTopology topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  MenuWorldSourceBufferSlice raw_storage; // Nonempty only in storage_fetch mode.
  MenuWorldSourceIndexRange source_index_range; // Resolved original first/count.
};
// The supplied source binding retains this draw's owner and constants by value.
struct MenuWorldSourceStoredDraw : MenuWorldSourceDrawGeometry {
  explicit MenuWorldSourceStoredDraw(MenuWorldSourceBinding source,MenuWorldSourceDrawGeometry geometry={})
      : MenuWorldSourceDrawGeometry(std::move(geometry)),source(std::move(source)) {}
  MenuWorldSourceBinding source;
};
// Owns immutable host-uploaded Vulkan vertex/index buffers. Distinct owners of
// one section share its raw buffer. Index buffers share only when the part,
// topology and resolved absolute(first,count) agree; explicit whole and default
// whole ranges have the same identity. Empty ranges allocate no index buffer.
// Repeated section identities with different immutable bytes are rejected. Buffer
// usage includes TRANSFER_SRC for independent readback verification.
//
// store() performs no queue operation, shader/pipeline/descriptor allocation,
// selector or visibility decision. Device format/vertex-input limits are
// queried and unsupported fetch formats rejected. Returned metadata remains
// valid until the next store(); all uploaded buffers remain valid until this
// storage is destroyed. The caller must complete GPU uses before destruction
// and keep the VkDevice/VkPhysicalDevice alive throughout the storage lifetime.
class MenuWorldSourceStorage {
public:
  explicit MenuWorldSourceStorage(MenuWorldSourceStorageContext);
  ~MenuWorldSourceStorage();
  MenuWorldSourceStorage(const MenuWorldSourceStorage &)=delete;
  MenuWorldSourceStorage &operator=(const MenuWorldSourceStorage &)=delete;
  std::span<const MenuWorldSourceStoredDraw> store(std::span<const MenuWorldSourceBinding>);
  // The same store over a list of borrowed input bindings.
  std::span<const MenuWorldSourceStoredDraw> store_borrowed(std::span<const MenuWorldSourceBinding *const>);
  // Reuse a part's immutable buffers and fetch layout across submissions. The
  // reference remains valid until this storage is destroyed. Owner and frame
  // constants remain in the caller's current binding.
  const MenuWorldSourceDrawGeometry &retain_geometry(const MenuWorldSourceBinding &);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
