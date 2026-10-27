#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <functional>
#include <map>
namespace halo2 {
// Read-only provenance from the original BAF10 arguments and completed RET.
// No marker, datum, transform or physical result is written into source RAM.
struct SourceObjectAttachmentTu15 {
    std::uint32_t parent{},parent_marker{},child{},child_marker{},caller_return{};
    bool operator==(const SourceObjectAttachmentTu15 &) const = default;
};
using SourceObjectAttachmentHandlerTu15=std::function<void(const SourceObjectAttachmentTu15 &)>;
// Function-local TLS preserves one registry per thread across translation
// units without MinGW namespace-scope TLS initializer aliases.
inline std::map<const SourcePhysicsContactMemory *,SourceObjectAttachmentHandlerTu15>& source_object_attachment_handlers_tu15() {
    static thread_local std::map<const SourcePhysicsContactMemory *,SourceObjectAttachmentHandlerTu15> value;
    return value;
}
class SourceObjectAttachmentBindingTu15 {
    const SourcePhysicsContactMemory *memory_;
    SourceObjectAttachmentHandlerTu15 previous_;
public:
    SourceObjectAttachmentBindingTu15(const SourcePhysicsContactMemory &memory,SourceObjectAttachmentHandlerTu15 handler):memory_(&memory) {
        auto &slot=source_object_attachment_handlers_tu15()[memory_];
        previous_=std::move(slot);slot=std::move(handler);
    }
    ~SourceObjectAttachmentBindingTu15() {
        if(previous_)source_object_attachment_handlers_tu15()[memory_]=std::move(previous_);
        else source_object_attachment_handlers_tu15().erase(memory_);
    }
    SourceObjectAttachmentBindingTu15(const SourceObjectAttachmentBindingTu15 &)=delete;
};
class SourceObjectAttachmentScopeTu15 {
    const SourcePhysicsContactMemory &memory_;
    SourceObjectAttachmentTu15 event_{};
public:
    explicit SourceObjectAttachmentScopeTu15(const SourcePhysicsContactMemory &memory):memory_(memory) {}
    // BAF5F already has all four actual arguments in registers. Observing
    // them here adds no early stack/object reads or source failure path.
    void inputs(std::uint32_t parent,std::uint32_t parent_marker,std::uint32_t child,std::uint32_t child_marker) {
        event_={parent,parent_marker,child,child_marker,0};
    }
    void complete(std::uint32_t actual_return) {
        const auto found=source_object_attachment_handlers_tu15().find(&memory_);
        if(found!=source_object_attachment_handlers_tu15().end()) {
            event_.caller_return=actual_return;found->second(event_);
        }
    }
};
}
