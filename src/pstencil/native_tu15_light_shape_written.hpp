#pragma once

#include "halo2/physics_contact_record.hpp"
#include <array>
#include <cstdint>

namespace halo2::pstencil {

// The live light-shape caller records which source output bytes were written.
// Its callable has no source-memory or service access. The synchronous scope
// lets the native owner distinguish its exact copies from arbitrary wrappers.
class LightShapeWrittenObserver final {
    using Written = std::array<std::uint8_t,124>;
    struct Capture {
        const LightShapeWrittenObserver *owner;
        std::uint64_t generation;
        void operator()(const SourcePhysicsContactRecordStore &store) const noexcept {
            for (unsigned offset = 0; offset < store.written.size(); ++offset) {
                const auto address = store.address + offset;
                if (address >= owner->output_ && address < owner->output_ + owner->written_.size())
                    owner->written_[address - owner->output_] = 1;
            }
        }
    };

    const std::uint32_t output_;
    Written &written_;
    const LightShapeWrittenObserver *previous_;
    const Capture capture_;
    inline static thread_local const LightShapeWrittenObserver *active_{};
    inline static thread_local std::uint64_t generations_{};

public:
    LightShapeWrittenObserver(std::uint32_t output, Written &written)
        : output_(output), written_(written), previous_(active_), capture_{this,++generations_} {
        active_ = this;
    }
    ~LightShapeWrittenObserver() { active_ = previous_; }
    LightShapeWrittenObserver(const LightShapeWrittenObserver &) = delete;
    LightShapeWrittenObserver &operator=(const LightShapeWrittenObserver &) = delete;

    SourcePhysicsContactRecordObserver observer() const { return capture_; }
    static bool qualified(const SourcePhysicsContactRecordObserver &observer) noexcept {
        const auto *capture = observer.target<Capture>();
        return capture && active_ && capture->owner == active_ &&
               capture->generation == active_->capture_.generation;
    }
};

} // namespace halo2::pstencil
