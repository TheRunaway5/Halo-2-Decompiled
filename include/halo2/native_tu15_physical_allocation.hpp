#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <map>

namespace halo2 {
class NativeTu15PhysicalAllocationFrame {
public:
    virtual ~NativeTu15PhysicalAllocationFrame()=default;
    virtual std::uint32_t requested_bytes()const=0;
    virtual std::uint32_t attribute(unsigned index)const=0;
    virtual void record_slot(std::uint32_t record,std::uint32_t publication)=0;
    virtual std::uint32_t record_slot()const=0;
};
struct NativeTu15PhysicalAllocationServices {
    std::function<std::uint32_t(std::uint32_t pool,std::uint32_t bytes,std::uint32_t policy,NativeTu15PhysicsFrameFp&)> pages; //13FE50
    std::function<std::uint32_t(std::uint32_t array,std::uint32_t handle,NativeTu15PhysicsFrameFp&)> allocate_specific; //16E770
    std::function<void(std::uint32_t address,std::uint32_t bytes,std::uint32_t protection,NativeTu15PhysicsFrameFp&)> protect; //2DB4CA
};
enum class NativeTu15PhysicalAllocationPhase {partition_binding,requested,handle,partition,
    page_table,page_records,page_shift,page_offset,descriptor_binding,shifted_offset,base,
    address,record_array,record,attribute,encoded_address,previous,scratch_record,
    link_inputs,publish_list,clear_record,active_byte,returning};
struct NativeTu15PhysicalAllocationPhaseData {
    std::uint32_t pool{},bytes{},handle{},table{},records{},shift{},offset{},array{},base{},address{},
        header{},record{},value{},field{},previous{},protection_bytes{},protection_address{};
};
struct NativeTu15PhysicalAllocationPhases {
    std::function<void(NativeTu15PhysicalAllocationPhase,const NativeTu15PhysicalAllocationPhaseData&)> observe;
};
struct NativeTu15PhysicalAllocationStats {
    std::uint64_t calls{},refusals{},allocated{},unbound_services{};
    std::map<std::uint32_t,std::uint64_t> unbound_owners;
};
// Actual12FE00 retained physical-page ownership wrapper. The page allocator,
// requested datum and physical protection remain distinct genuine services;
// no page handle, protection outcome or allocation address is supplied here.
std::uint32_t native_tu15_allocate_physical_block(SourcePhysicsContactMemory&,
    NativeTu15PhysicalAllocationFrame&,NativeTu15PhysicsFrameFp&,
    const NativeTu15PhysicalAllocationServices&,const NativeTu15PhysicalAllocationPhases& = {},
    const SourcePhysicsContactRecordObserver& = {},NativeTu15PhysicalAllocationStats* = nullptr);
} // namespace halo2
