#include "halo2/native_tu15_rng_startup.hpp"
#include <chrono>

namespace halo2 {
namespace {
constexpr std::uint32_t fs=0x60200000u,tick_cell=0x60212000u,thread_context=0x60213000u;
std::array<std::byte,4> encode(std::uint32_t value) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));return bytes;
}
void put(SourceRuntimeMemory& m,std::uint32_t p,std::uint32_t value){m.write(p,encode(value));}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p) {
    auto bytes=m.view(p,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
}
void mount_native_tu15_rng_startup_data(SourceRuntimeMemory& memory) {
    for(auto cell:{0x4f3cd8u,0x573840u,0x573720u,0x48d6f8u})
        memory.mount(cell,encode(0),SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 cold RNG/CRT binding or allocation policy");
    for(auto [cell,target]:{std::pair{0x41b668u,0x80000080u},std::pair{0x41b680u,0x80000115u},std::pair{0x41b67cu,0x80000126u}})
        memory.mount(cell,encode(target),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 time/heap platform function identity");
}
NativeTu15RngClock native_tu15_rng_clock() {
    return {[] {
        using Units=std::chrono::duration<std::int64_t,std::ratio<1,10000000>>;
        const auto unix_ticks=std::chrono::duration_cast<Units>(std::chrono::system_clock::now().time_since_epoch()).count();
        constexpr std::int64_t epoch=116444736000000000ll;
        if(unix_ticks< -epoch || unix_ticks>INT64_MAX-epoch)throw std::runtime_error("Native UTC outside FILETIME domain");
        return std::uint64_t(unix_ticks+epoch);
    },[] {
        return std::uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }};
}
NativeTu15RngStartup::NativeTu15RngStartup(NativeTu15HeapBootstrap& heap,NativeTu15RngClock clock)
    :memory_(heap.memory()),thread_(std::this_thread::get_id()),thread_token_(heap.current_thread_token()),clock_(std::move(clock)) {
    if(!clock_.utc_filetime || !clock_.monotonic_milliseconds)throw std::invalid_argument("Native RNG requires retained clock inputs");
    mount_native_tu15_rng_startup_data(memory_);
    // Explicit native executor context. This is not an Xbox KTHREAD snapshot.
    // Ordinary executor level and no Xbox TLS expansion select the source CRT
    // main-thread bank. Only the fields consumed by the actual source exist.
    put(memory_,fs+0x24u,0);put(memory_,fs+0x28u,thread_context);
    memory_.mount(thread_context+0x28u,encode(0),SourceRuntimeMemory::Access::ReadOnly,"native ordinary executor has no Xbox TLS expansion");
    memory_.mount(thread_context+0x12cu,encode(thread_token_),SourceRuntimeMemory::Access::ReadOnly,"native main executor thread equality token");
    memory_.mount(tick_cell,encode(0),SourceRuntimeMemory::Access::ReadWrite,"native monotonic millisecond data owner");
    memory_.mount(0x41b774u,encode(tick_cell),SourceRuntimeMemory::Access::ReadOnly,"native imported tick-count data binding");
}
bool NativeTu15RngStartup::try_call(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,
    SourcePhysicsContactRuntimeAbi& abi) {
    if(call.target!=0x80000080u)return false;
    const auto sp=abi.gp[4];auto& memory=memory_;
    if(&bank!=&memory_ || std::this_thread::get_id()!=thread_ || abi.fs_base!=fs ||
        call.instruction!=0x33a4d3u || !call.tail || word(memory,sp)!=0x32bbbcu ||
        word(memory,sp+4u)!=abi.gp[5]-8u)
        throw std::invalid_argument("Native RNG UTC bank, thread or actual time output frame differs");
    const auto output=word(memory,sp+4u);memory.mutable_view(output,8);
    const auto value=clock_.utc_filetime();
    put(memory,output,std::uint32_t(value));put(memory,output+4u,std::uint32_t(value>>32));
    utc_=value;abi.gp[4]+=8u;return true;
}
void NativeTu15RngStartup::instruction(std::uint32_t instruction) {
    if(instruction==0x148d98u) {
        if(std::this_thread::get_id()!=thread_ || !tick_)throw std::invalid_argument("Source RNG seed has no completed native clock read");
        seed_tick_=tick_;return;
    }
    if(instruction!=0x33b595u)return;
    if(std::this_thread::get_id()!=thread_)throw std::invalid_argument("Native RNG tick read crossed its executor thread");
    const auto value=clock_.monotonic_milliseconds();
    put(memory_,tick_cell,value);tick_=value;ticks_.push_back(value);
}
}
