#include "halo2/native_tu15_language.hpp"
#include <SDL_locale.h>
#include <SDL_stdinc.h>
#include <memory>
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p) {
    auto b=m.view(p,4);std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
void put(SourceRuntimeMemory& m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);
}
}
std::uint32_t native_tu15_platform_language(std::string_view language) {
    // Primary enum definitions: Cxbx-Reloaded-legacy96aabe72, EmuEEPROM.h.
    // These encode an owned application preference, not console EEPROM data.
    constexpr std::array<std::string_view,9> supported{"en","ja","de","fr","es","it","ko","zh","pt"};
    for(unsigned i=0;i<supported.size();++i)if(language==supported[i])return i+1;
    return 0; // Native preference has no platform-language encoding.
}
NativeTu15LanguagePreference native_tu15_language_preference() {
    std::unique_ptr<SDL_Locale,decltype(&SDL_free)> locales(SDL_GetPreferredLocales(),SDL_free);
    if(!locales || !locales->language)throw std::runtime_error("Native preferred-language provider is unavailable");
    const std::string language=locales->language,country=locales->country?locales->country:"";
    return {language,country,native_tu15_platform_language(language)};
}
NativeTu15Language::NativeTu15Language(SourceRuntimeMemory& memory,NativeTu15LanguagePreference preference)
    :memory_(memory),preference_(std::move(preference)),thread_(std::this_thread::get_id()) {
    if(preference_.language.empty() || preference_.platform_value!=native_tu15_platform_language(preference_.language))
        throw std::invalid_argument("Native language preference encoding differs from its owned locale");
}
bool NativeTu15Language::try_call(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x80000018u)return false;
    const auto sp=abi.gp[4],bp=abi.gp[5];
    if(&bank!=&memory_ || std::this_thread::get_id()!=thread_ || call.instruction!=0x2e101cu || !call.tail ||
        word(memory_,sp)!=0x33a4beu || word(memory_,sp+4)!=7 || word(memory_,sp+8)!=bp-8 ||
        word(memory_,sp+12)!=bp-4 || word(memory_,sp+16)!=4 || word(memory_,sp+20)!=0)
        throw std::invalid_argument("Native language query bank, executor or complete settings frame differs");
    memory_.mutable_view(bp-8,8);put(memory_,bp-8,4);put(memory_,bp-4,preference_.platform_value);
    queried_=true;++queries_;abi.gp[0]=0;abi.gp[4]+=24;return true;
}
}
