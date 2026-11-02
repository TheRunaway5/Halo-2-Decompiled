#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>

namespace halo2 {
class SourcePhysicsContactMemory;
namespace unit_speech_symbols_detail_tu15 {
struct Symbol {std::uint32_t instruction,original,mcc;std::string_view name;};
// Compiled literals in actual1102A0, feeding its real1E5360 queries. Exact
// unique-name joins of Original TU1.5 and authenticated MCC namespaces.
inline constexpr std::array symbols{
    Symbol{0x1102e9u,0x08000145u,0x08000149u,"phonemes"},
    Symbol{0x110314u,0x0c00024du,0x0c00025bu,"mouth_closed"}
};
struct Binding {const SourcePhysicsContactMemory*memory;};
struct Registry {
    std::mutex mutex;
    std::map<const SourcePhysicsContactMemory*,std::weak_ptr<const Binding>>banks;
};
// One inline-function local registry across the executable's translation
// units. No separate target dependency is needed for either generated clone.
inline Registry&registry(){static Registry state;return state;}
}

// Actual World/cache lifetime lease, matching the existing compiled-effect
// and weapon-query namespace owners. It publishes no graph/channel result and
// never reads/writes authored or retained source bytes.
class SourceUnitSpeechChannelSymbolsTu15 {
    std::shared_ptr<const unit_speech_symbols_detail_tu15::Binding>binding_;
public:
    using Name=std::function<std::string(std::uint32_t)>;
    SourceUnitSpeechChannelSymbolsTu15(const SourcePhysicsContactMemory&memory,bool mcc,const Name&name){
        if(!mcc)return;
        if(!name)throw std::invalid_argument("MCC unit speech symbols require the actual Cache name resolver");
        for(const auto&symbol:unit_speech_symbols_detail_tu15::symbols)
            if(name(symbol.mcc)!=symbol.name)throw std::invalid_argument("MCC unit speech symbols differ from their authenticated named join");
        auto binding=std::make_shared<const unit_speech_symbols_detail_tu15::Binding>(unit_speech_symbols_detail_tu15::Binding{&memory});
        auto&state=unit_speech_symbols_detail_tu15::registry();std::lock_guard lock(state.mutex);
        const auto previous=state.banks.find(&memory);
        if(previous!=state.banks.end()&&!previous->second.expired())throw std::invalid_argument("Source bank already owns its unit speech namespace");
        state.banks[&memory]=binding;binding_=std::move(binding);
    }
    ~SourceUnitSpeechChannelSymbolsTu15(){
        if(!binding_)return;
        auto&state=unit_speech_symbols_detail_tu15::registry();std::lock_guard lock(state.mutex);
        const auto at=state.banks.find(binding_->memory);
        if(at!=state.banks.end()&&at->second.lock()==binding_)state.banks.erase(at);
    }
    SourceUnitSpeechChannelSymbolsTu15(const SourceUnitSpeechChannelSymbolsTu15&)=delete;
    SourceUnitSpeechChannelSymbolsTu15&operator=(const SourceUnitSpeechChannelSymbolsTu15&)=delete;
};

// Only the exact compiled push-site/literal pairs above may enter MCC's
// namespace. Original/unbound banks, other instructions and every other raw
// authored value retain their bits. Real1E5360 computes both graph lookups.
inline std::uint32_t source_unit_speech_channel_symbol_tu15(const SourcePhysicsContactMemory&memory,
    std::uint32_t instruction,std::uint32_t original){
    const unit_speech_symbols_detail_tu15::Symbol*match=nullptr;
    for(const auto&symbol:unit_speech_symbols_detail_tu15::symbols)
        if(symbol.instruction==instruction&&symbol.original==original){match=&symbol;break;}
    if(!match)return original;
    auto&state=unit_speech_symbols_detail_tu15::registry();std::lock_guard lock(state.mutex);const auto at=state.banks.find(&memory);
    return at==state.banks.end()||at->second.expired()?original:match->mcc;
}
}
