#pragma once
#include "rendering/renderer/frame_vertex_upload_plan.hpp"
#include <cstdint>
#include <memory>
#include <thread>

namespace halo2 {
// One renderer owns this persistent, single-job byte-copy service. It never
// calls source owners or Vulkan. Only one ticket may be outstanding. It joins before borrowed frame data or
// mapped destination storage can be edited, unmapped or destroyed.
class PresentationCopyWorker {
    friend struct PresentationCopyWorkerTestAccess;
    struct Impl;
    std::shared_ptr<Impl> impl_;
    using Copy=void(*)(const void*,void*,std::size_t);
public:
    struct Result {
        std::thread::id executor;
        std::size_t vertices{};
        std::uint64_t begin_ns{},end_ns{};
    };
    class Ticket {
        friend class PresentationCopyWorker;
        std::shared_ptr<Impl> impl_;
        std::uint64_t serial_{};
        Ticket(std::shared_ptr<Impl>,std::uint64_t);
    public:
        Ticket()=default;
        Ticket(const Ticket&)=delete;
        Ticket& operator=(const Ticket&)=delete;
        Ticket(Ticket&&)noexcept;
        Ticket& operator=(Ticket&&)noexcept;
        ~Ticket();
        explicit operator bool()const noexcept{return bool(impl_);}
        Result finish();
        void join()noexcept;
    };
    PresentationCopyWorker();
    ~PresentationCopyWorker();
    PresentationCopyWorker(const PresentationCopyWorker&)=delete;
    PresentationCopyWorker& operator=(const PresentationCopyWorker&)=delete;
    template<class Key> Ticket start(const FrameVertexUploadPlan<Key>& plan,
                                    void *destination,std::size_t prefix) {
        if(prefix>plan.size())throw FormatError("worker vertex prefix escapes its plan");
        if(prefix && !destination)throw FormatError("worker vertex prefix lacks its destination");
        return enqueue(&plan,destination,prefix,[](const void *source,void *out,std::size_t count) {
            static_cast<const FrameVertexUploadPlan<Key>*>(source)->copy_to(out,0,count);
        });
    }
private:
    Ticket enqueue(const void*,void*,std::size_t,Copy);
};
}
