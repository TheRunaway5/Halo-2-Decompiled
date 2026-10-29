#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
class NativeTu15CrcInput {
public:
    virtual ~NativeTu15CrcInput()=default;
    virtual std::uint8_t byte(std::uint32_t index)const=0;
};
// Complete166AE0 reflected polynomial table initialization and166A80 update.
// The actual writable table remains lazy; neither a host CRC library nor a
// cached replacement table can substitute for its observable source storage.
void native_tu15_crc32_build_table(SourcePhysicsContactMemory &,std::uint32_t table,
    const SourcePhysicsContactRecordObserver & = {});
void native_tu15_crc32_update(SourcePhysicsContactMemory &,const NativeTu15CrcInput &,
    std::int32_t count,std::uint32_t checksum,const SourcePhysicsContactRecordObserver & = {});
}
