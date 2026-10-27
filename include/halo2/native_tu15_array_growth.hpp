#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15ArrayHeader {std::uint32_t identity{};};
struct NativeTu15ArrayAllocator {std::uint32_t receiver{},method_table{};};
struct NativeTu15ArrayAllocationRequest {
  std::uint32_t receiver{},owner{},bytes{},memory_class{0x12};
};
struct NativeTu15ArrayReleaseRequest {
  std::uint32_t receiver{},owner{},data{},bytes{},memory_class{0x12};
};
struct NativeTu15ArrayGrowthServices {
  // These are actual current allocator virtual owners, not supplied pointer
  // outcomes. The implementation must execute genuine allocation/release
  // semantics and own their backing lifetime. Missing owners remain explicit.
  std::function<std::uint32_t(const NativeTu15ArrayAllocationRequest&,NativeTu15PhysicsFrameFp&)> allocate;
  std::function<void(const NativeTu15ArrayReleaseRequest&,NativeTu15PhysicsFrameFp&)> release;
};
struct NativeTu15ArrayGrowthActivity {
  std::uint64_t completed_growths{},allocations{},releases{},copied_words{},copied_bytes{};
};
enum class NativeTu15ArrayCountRead {initial,after_allocation};
enum class NativeTu15ArrayDataRead {copy_source,release_source};
class NativeTu15ArrayCopyStorage {
public:
  virtual ~NativeTu15ArrayCopyStorage()=default;
  virtual std::uint32_t copy_word(std::uint32_t source)const=0;
  virtual std::uint8_t copy_byte(std::uint32_t source)const=0;
  virtual void copy_word(std::uint32_t destination,std::uint32_t value)=0;
  virtual void copy_byte(std::uint32_t destination,std::uint8_t value)=0;
};
class NativeTu15ArrayGrowthStorage:public NativeTu15ArrayCopyStorage {
public:
  virtual std::uint32_t count(NativeTu15ArrayCountRead)const=0;
  virtual std::uint32_t data(NativeTu15ArrayDataRead)const=0;
  virtual std::uint32_t capacity_flags()const=0;
  virtual NativeTu15ArrayAllocator allocator(bool releasing)const=0;
  virtual std::uint32_t allocate_owner(NativeTu15ArrayAllocator)const=0;
  virtual std::uint32_t release_owner(NativeTu15ArrayAllocator)const=0;
  virtual void publish_data(std::uint32_t)=0;
  virtual void publish_capacity(std::uint32_t)=0;
};
// Actual2E2E30 under its admitted C/C++ forward-copy ABI (DF clear). Preserve
// four-byte transfers followed by byte transfers, 32-bit address wrapping,
// overlap behavior and each read-before-write/fault boundary. No libc memcpy,
// temporary complete-buffer snapshot or guest execution is used.
void native_tu15_copy_array_bytes(NativeTu15ArrayCopyStorage&,
  std::uint32_t destination,std::uint32_t source,std::uint32_t bytes,
  NativeTu15ArrayGrowthActivity* = nullptr);
// Actual2E3250 through its2E3290 continuation. Growth doubles the initial
// COUNT, using1 only for zero count; all products/additions wrap at32bits.
// It rereads allocator/data/count/capacity at their source phases, releases
// owned old data before publishing the new pointer, and never writes count.
// Allocator virtual owners remain explicitly required services.
void native_tu15_grow_array(NativeTu15ArrayGrowthStorage&,std::uint32_t stride,
  NativeTu15PhysicsFrameFp&,const NativeTu15ArrayGrowthServices&,
  NativeTu15ArrayGrowthActivity* = nullptr);
class NativeTu15ArrayGrowthMemory final:public NativeTu15ArrayGrowthStorage {
  SourcePhysicsContactMemory&memory_;NativeTu15ArrayHeader header_;
  SourcePhysicsContactRecordObserver observer_;
  std::uint32_t read(std::uint32_t,unsigned bytes=4)const;
  void write(std::uint32_t site,std::uint32_t address,std::uint32_t,unsigned bytes=4);
public:
  NativeTu15ArrayGrowthMemory(SourcePhysicsContactMemory&,NativeTu15ArrayHeader,
    const SourcePhysicsContactRecordObserver& = {});
  std::uint32_t count(NativeTu15ArrayCountRead)const override;
  std::uint32_t data(NativeTu15ArrayDataRead)const override;
  std::uint32_t capacity_flags()const override;
  NativeTu15ArrayAllocator allocator(bool releasing)const override;
  std::uint32_t allocate_owner(NativeTu15ArrayAllocator)const override;
  std::uint32_t release_owner(NativeTu15ArrayAllocator)const override;
  std::uint32_t copy_word(std::uint32_t)const override;
  std::uint8_t copy_byte(std::uint32_t)const override;
  void copy_word(std::uint32_t,std::uint32_t)override;
  void copy_byte(std::uint32_t,std::uint8_t)override;
  void publish_data(std::uint32_t)override;
  void publish_capacity(std::uint32_t)override;
};
} // namespace halo2
