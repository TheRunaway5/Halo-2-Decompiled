#pragma once
#include <cstdint>
namespace halo2 {
struct SourceDamageRecordNativeActivity {
    std::uint64_t calls{},admitted{},rejected{};
};
// Host diagnostics only; no counter participates in source state or decisions.
SourceDamageRecordNativeActivity source_damage_record_native_activity();
}
