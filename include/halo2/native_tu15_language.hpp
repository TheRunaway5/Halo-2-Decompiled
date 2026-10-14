#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <optional>
#include <thread>
#include <string_view>
namespace halo2 {
struct NativeTu15LanguagePreference {std::string language,country;std::uint32_t platform_value;};
NativeTu15LanguagePreference native_tu15_language_preference();
std::uint32_t native_tu15_platform_language(std::string_view);
class NativeTu15Language final {
public:
    explicit NativeTu15Language(SourceRuntimeMemory&,NativeTu15LanguagePreference = native_tu15_language_preference());
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    const NativeTu15LanguagePreference& preference()const noexcept{return preference_;}
    bool queried()const noexcept{return queried_;}
    std::size_t queries()const noexcept{return queries_;}
private:
    SourceRuntimeMemory& memory_;
    NativeTu15LanguagePreference preference_;
    std::thread::id thread_;
    bool queried_=false;
    std::size_t queries_=0;
};
}
