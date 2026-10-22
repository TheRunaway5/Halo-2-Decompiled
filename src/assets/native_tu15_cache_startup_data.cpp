#include "halo2/source_cache_startup_tu15.hpp"
#include <array>
#include <algorithm>
namespace halo2 {
namespace {
std::array<std::byte,4> encode(std::uint32_t v) {
    std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));return b;
}
}
void mount_native_tu15_game_language_storage(SourceRuntimeMemory& m) {
    // Reusing the full cache startup must preserve the getter's live result.
    try {m.validate_mutable_view(0x48c750u,4);}
    catch(const std::out_of_range&) {
        m.mount(0x48c750,encode(0xffffffffu),SourceRuntimeMemory::Access::ReadWrite,"source unset game language binding");
    }
    // Actual11F440 converts the platform language into the game's language.
    // English1/Portuguese9 select the original English0 fallback; this table
    // preserves all nine authored dispatch identities, not a supplied result.
    constexpr std::array<std::uint32_t,9> branches{0x11f477,0x11f44d,0x11f453,0x11f459,0x11f45f,0x11f465,0x11f46b,0x11f471,0x11f477};
    std::array<std::byte,36> bytes{};
    for(unsigned i=0;i<branches.size();++i)for(unsigned j=0;j<4;++j)bytes[i*4+j]=std::byte(branches[i]>>(8*j));
    bool present=false;std::array<std::byte,36> actual{};
    try {m.read(0x11f47cu,actual);present=true;}catch(const std::out_of_range&) {}
    if(present) {
        bool readonly=true;for(unsigned i=0;i<actual.size();++i)readonly&=m.is_read_only(0x11f47cu+i,1);
        if(!readonly || actual!=bytes)throw std::invalid_argument("Language table conflicts with its original immutable owner");
    } else m.mount(0x11f47c,bytes,SourceRuntimeMemory::Access::ReadOnly,"source platform-to-game language branch identities");
}
void mount_native_tu15_cache_startup_data(SourceRuntimeMemory& m) {
    mount_native_tu15_game_language_storage(m);
    m.mount(0x56aed0,std::vector<std::byte>(0x254),SourceRuntimeMemory::Access::ReadWrite,"SGF cache startup constructor destination");
    m.mount(0x52be9c,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"SGF cache startup state bindings");
    constexpr std::array<char,8> path{"n:\\sgf"};
    m.mount(0x463318,std::as_bytes(std::span(path)),SourceRuntimeMemory::Access::ReadOnly,"source SGF cache logical directory name");
    m.mount(0x41b6f0,encode(0x80000018u),SourceRuntimeMemory::Access::ReadOnly,"source settings query platform identity");
}
}
