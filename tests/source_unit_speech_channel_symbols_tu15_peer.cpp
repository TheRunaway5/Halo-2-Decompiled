#include "halo2/source_unit_speech_channel_symbols_tu15.hpp"

std::unique_ptr<halo2::SourceUnitSpeechChannelSymbolsTu15> speech_lease_from_peer(
    const halo2::SourcePhysicsContactMemory& memory,
    const halo2::SourceUnitSpeechChannelSymbolsTu15::Name& names) {
    return std::make_unique<halo2::SourceUnitSpeechChannelSymbolsTu15>(memory,true,names);
}
std::uint32_t speech_query_from_peer(const halo2::SourcePhysicsContactMemory& memory,
    std::uint32_t pc,std::uint32_t raw) {
    return halo2::source_unit_speech_channel_symbol_tu15(memory,pc,raw);
}
