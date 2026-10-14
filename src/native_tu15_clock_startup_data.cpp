#include "halo2/source_startup_state_tu15.hpp"
namespace halo2 {
void mount_native_tu15_clock_startup_data(SourceRuntimeMemory& m) {
    m.mount(0x50ebfc,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadWrite,"source22C100 four-field constructor destination");
    m.mount(0x51df54,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source game-clock constructor binding");
}
}
