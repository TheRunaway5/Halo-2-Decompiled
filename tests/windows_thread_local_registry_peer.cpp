#include "windows_thread_local_registry_fixture.hpp"

namespace halo2::tests::tls {
namespace {thread_local LifetimeMarker marker(31);}
std::array<const void*,11> producer_addresses(){return registry_addresses();}
int producer_marker(){return marker.value;}
std::unique_ptr<SourceObjectAttachmentBindingTu15> producer_bind(
    const SourcePhysicsContactMemory& memory,SourceObjectAttachmentHandlerTu15 handler) {
    return std::make_unique<SourceObjectAttachmentBindingTu15>(memory,std::move(handler));
}
void producer_emit(const SourcePhysicsContactMemory& memory) {
    SourceObjectAttachmentScopeTu15 scope(memory);scope.inputs(9,10,11,12);scope.complete(13);
}
const source_script_sound_detail::Admission* producer_admission() {
    const auto& entries=registry(source_script_sound_detail::admissions);
    const auto at=entries.find(nullptr);return at==entries.end()?nullptr:at->second.get();
}
}
