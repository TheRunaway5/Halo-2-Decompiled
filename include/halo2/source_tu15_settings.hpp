#pragma once
#include "halo2/source_tu15_startup_records.hpp"

namespace halo2 {
// An immutable snapshot of the actual 96-byte EEPROM UserSettings section.
// The loader reads the owned 256-byte file and keeps only this non-key section.
// An invalid checksum remains an actual query failure; no settings default is
// selected. Layout and checksum expressions come from pinned primary source.
class SourceTu15UserSettings final {
public:
    SourceTu15UserSettings(std::array<std::byte,96>,std::string owner);
    static SourceTu15UserSettings load(const std::filesystem::path &);
    std::span<const std::byte,96> bytes() const { return bytes_; }
    bool checksum_valid() const;
    std::uint32_t miscellaneous_flags() const;
private:
    std::array<std::byte,96> bytes_;
    std::string owner_;
};
// Actual TU1.5 XC_MISC query frame at the import thunk. Native success/failure
// ABI preserves other live CPU state and consumes the full stdcall frame.
// Observations are logical primary output writes, not Xbox kernel store PCs.
bool source_try_tu15_settings_query(SourceRuntimeMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsContactRuntimeAbi &,
    const SourceTu15UserSettings &,const SourcePhysicsContactRecordObserver & = {});
void source_run_tu15_settings_runtime(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Wire the complete source settings wrapper and actual data query into the
// existing fresh startup caller. Status conversion301 and timer149 remain
// required owners. This never supplies their returns or installs a heap.
void source_run_tu15_process_startup_with_settings(SourceRuntimeMemory &,
    std::uint32_t canonical_entry,SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRuntimeServices &,
    SourceRuntimeHostVirtualMemory &,PhysicsPlatformLocks &,
    const PhysicsPlatformLockServices &,SourceProcessShutdownRegistry &,
    const SourceTu15UserSettings &,
    const SourcePhysicsContactRecordObserver & = {},
    const SourcePhysicsContactRecordObserver & = {},
    const SourceProcessShutdownObserver & = {},
    const SourcePhysicsContactRecordObserver & = {},
    const SourceProcessHeapDataImportOwner & = {});
} // namespace halo2
