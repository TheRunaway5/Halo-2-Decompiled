#include "halo2/source_process_startup_records.hpp"

namespace halo2 {
namespace {
void store(SourceRuntimeMemory &memory, std::uint32_t p, std::uint32_t value,
    std::size_t width, const SourceProcessStartupRecordObserver &observe) {
    const auto old=memory.view(p,width);
    SourceProcessStartupRecordStore change{p,{old.begin(),old.end()},std::vector<std::byte>(width)};
    for(std::size_t i=0;i<width;++i)change.written[i]=std::byte(value>>(i*8));
    memory.write(p,change.written);
    if(observe)observe(change);
}
}
void source_initialize_process_dpc(SourceRuntimeMemory &memory, std::uint32_t p,
    std::uint32_t routine, std::uint32_t context, const SourceProcessStartupRecordObserver &observe) {
    memory.mutable_view(p,28);
    store(memory,p,0x13,2,observe);
    store(memory,p+2,0,1,observe);
    store(memory,p+12,routine,4,observe);
    store(memory,p+16,context,4,observe);
}
void source_initialize_process_timer(SourceRuntimeMemory &memory, std::uint32_t p,
    std::uint32_t type, const SourceProcessStartupRecordObserver &observe) {
    memory.mutable_view(p,40);
    store(memory,p,(8u+type)&255u,1,observe);
    store(memory,p+3,0,1,observe);
    store(memory,p+2,10,1,observe);
    store(memory,p+4,0,4,observe);
    store(memory,p+12,p+8,4,observe);
    store(memory,p+8,p+8,4,observe);
    store(memory,p+28,0,4,observe);
    store(memory,p+24,0,4,observe);
    store(memory,p+16,0,4,observe);
    store(memory,p+20,0,4,observe);
    store(memory,p+36,0,4,observe);
}
} // namespace halo2
