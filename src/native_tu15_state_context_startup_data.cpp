#include "halo2/source_state_context_startup_tu15.hpp"
namespace halo2 {
void mount_native_tu15_state_context_startup_data(SourceRuntimeMemory& m) {
    m.mount(0x5020c4,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source1E7710 state-context constructor binding");
}
}
