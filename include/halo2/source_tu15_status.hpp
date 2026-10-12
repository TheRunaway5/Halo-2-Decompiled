#pragma once
#include "halo2/source_tu15_settings.hpp"

namespace halo2 {
// Exact immutable data for the owned modified Complex4627 export301. The
// selected BIOS/table fingerprints are required; no Windows or compatibility
// defaults are used. These resources do not establish physical Xbox parity.
class SourceTu15StatusTables final {
public:
    SourceTu15StatusTables(std::vector<std::byte> ranges,std::vector<std::byte> values,std::string owner);
    static SourceTu15StatusTables load(const std::filesystem::path &kernel_capture);
    void mount(SourceRuntimeMemory &) const;
    void validate(const SourceRuntimeMemory &) const;
private:
    std::vector<std::byte> ranges_,values_;
    std::string owner_;
};
void source_run_tu15_status_runtime(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
bool source_try_tu15_status_conversion(SourceRuntimeMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,const SourceTu15StatusTables &,
    const std::function<void(std::uint32_t)> & = {},const SourcePhysicsContactRecordObserver & = {});
// Tables must already be mounted in this same memory. The complete settings
// caller returns with the original kernel's GP/flags/stack effects. Timer149
// and subsequent startup/world owners remain required and are not skipped.
void source_run_tu15_process_startup_with_status(SourceRuntimeMemory &,
    std::uint32_t canonical_entry,SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRuntimeServices &,
    SourceRuntimeHostVirtualMemory &,PhysicsPlatformLocks &,const PhysicsPlatformLockServices &,
    SourceProcessShutdownRegistry &,const SourceTu15UserSettings &,const SourceTu15StatusTables &,
    const SourcePhysicsContactRecordObserver & = {},const SourcePhysicsContactRecordObserver & = {},
    const SourceProcessShutdownObserver & = {},const SourcePhysicsContactRecordObserver & = {},
    const SourceProcessHeapDataImportOwner & = {},const std::function<void(std::uint32_t)> & = {},
    const SourcePhysicsContactRecordObserver & = {});
}
