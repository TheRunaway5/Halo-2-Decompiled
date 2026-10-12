#pragma once
#include "halo2/source_runtime_memory.hpp"
#include <mutex>

namespace halo2 {
struct SourceProcessShutdownStore {
    std::uint32_t address;
    std::vector<std::byte> before, written;
};
using SourceProcessShutdownObserver = std::function<void(const SourceProcessShutdownStore &)>;
struct SourceProcessShutdownRecord {
    std::uint32_t address, routine;
    std::int32_t priority;
};
// Persistent native intrusive registry derived from pinned compatibility source.
// The explicit head token owns real retained backing. Its memory bank must stay
// in place and outlive this owner; release its head only through this owner.
// A host mutex serializes operations; this does not reproduce Xbox/Cxbx IRQL,
// interrupt delivery or callback execution. Existing linked records are not
// silently deduplicated and removed records retain their original link bytes.
class SourceProcessShutdownRegistry {
public:
    SourceProcessShutdownRegistry(SourceRuntimeMemory &, std::uint32_t head_token);
    ~SourceProcessShutdownRegistry();
    SourceProcessShutdownRegistry(const SourceProcessShutdownRegistry &) = delete;
    SourceProcessShutdownRegistry &operator=(const SourceProcessShutdownRegistry &) = delete;
    bool backs(const SourceRuntimeMemory &m) const noexcept { return &memory_ == &m; }
    void registration(std::uint32_t record, bool add, const SourceProcessShutdownObserver & = {});
    std::vector<SourceProcessShutdownRecord> records() const;
    std::uint32_t head() const noexcept { return head_; }
private:
    SourceRuntimeMemory &memory_;
    std::uint32_t head_;
    mutable std::mutex mutex_;
};
} // namespace halo2
