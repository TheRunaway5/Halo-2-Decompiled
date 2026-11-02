#pragma once
#include "halo2/native_tu15_contact_producer.hpp"
namespace halo2 {
// Actual3038C0 and2E4F30, cdecl(result byte, dispatcher, first class, second
// class). Class-index arithmetic wraps at32 bits. Each owner returns the same
// result DATA identity; no virtual call or source CPU state is involved.
std::uint32_t native_tu15_test_gsk_contact_pair(SourcePhysicsContactMemory&,
    std::uint32_t result,std::uint32_t dispatcher,std::uint32_t first_class,
    std::uint32_t second_class,const SourcePhysicsContactRecordObserver& = {});
std::uint32_t native_tu15_test_contact_pair(SourcePhysicsContactMemory&,
    std::uint32_t result,std::uint32_t dispatcher,std::uint32_t first_class,
    std::uint32_t second_class,const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
