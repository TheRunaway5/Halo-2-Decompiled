#pragma once
#include "halo2/source_tu15_startup_records.hpp"

namespace halo2 {
// An application preference, not an Xbox EEPROM snapshot. The original
// startup consumes XC_MISC bit0 to enable its six-hour power-down callback.
struct NativeTu15StartupSettings {
    bool automatic_power_down;
};

// Native desktop implementation of the platform interfaces consumed by the
// original startup prefix. No BIOS, EEPROM, captured kernel table or machine
// identity is input. The desktop is not an Xbox arcade machine. Its retained
// eight-byte ABI record has no claimed Xbox GPU/MCP revision; these bytes are
// not consumed by the supported original callers.
class NativeTu15StartupPlatform final {
public:
    explicit NativeTu15StartupPlatform(NativeTu15StartupSettings);
    SourceRuntimeMemory load_image(const std::filesystem::path &) const;
    SourceProcessHeapDataImportOwner data_owner() const;
    bool try_call(SourceRuntimeMemory &, const SourcePhysicsContactRuntimeCall &,
                  SourcePhysicsContactRuntimeAbi &,
                  const SourcePhysicsContactRecordObserver & = {}) const;
    void run_startup(SourceRuntimeMemory &, std::uint32_t canonical_entry,
        SourcePhysicsContactRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsContactRuntimeServices &, SourceRuntimeHostVirtualMemory &,
        PhysicsPlatformLocks &, const PhysicsPlatformLockServices &,
        SourceProcessShutdownRegistry &,
        const SourcePhysicsContactRecordObserver & = {},
        const SourcePhysicsContactRecordObserver & = {},
        const SourceProcessShutdownObserver & = {},
        const SourcePhysicsContactRecordObserver & = {}) const;
private:
    NativeTu15StartupSettings settings_;
    SourceRuntimeKernelHardwareInfo hardware_;
};
} // namespace halo2
