#pragma once
#include "halo2/vista_announcer_events.hpp"

namespace halo2 {
// Shared value storage only: the Xbox selectors below follow1A79A0,
// 1A7A30,1A8290 and248270/248470, independently of PC presentation policy.
struct SourceAnnouncerAlternativeTu15 {
    std::uint8_t flags{};
    std::array<std::uint32_t,9> sounds{};
    float weight{};
};
struct SourceAnnouncerResponseSelectionTu15 {
    std::uint32_t sound=0xffffffffu;
    std::int32_t delay_ticks{};
    bool announcer{};
    bool operator==(const SourceAnnouncerResponseSelectionTu15&)const=default;
};
struct SourceAnnouncerRecipientSelectionTu15 {
    unsigned ordinal{},audience{};
    bool operator==(const SourceAnnouncerRecipientSelectionTu15&)const=default;
};
struct SourceAnnouncerResponseBankTu15 {
    std::vector<VistaAnnouncerResponse> definitions;
    std::vector<std::vector<SourceAnnouncerAlternativeTu15>> alternatives;
};
struct SourceAnnouncerResponsesTu15 {
    std::array<SourceAnnouncerResponseBankTu15,11> banks;
};
SourceAnnouncerResponsesTu15 load_source_announcer_responses_tu15(const Cache&);
std::optional<SourceAnnouncerRecipientSelectionTu15> source_announcer_recipient_tu15(
    std::span<const VistaAnnouncerResponse>,const VistaAnnouncerEvent&,
    const VistaAnnouncerLocalPlayer&);
SourceAnnouncerResponseSelectionTu15 source_announcer_response_tu15(
    const VistaAnnouncerResponse&,std::span<const SourceAnnouncerAlternativeTu15>,
    unsigned language,std::int16_t tick_rate,std::uint32_t& actual_random,
    const VistaLoadingFloatingPoint&);
// The recent-event bank is shared by local recipients. Admit an actual sound
// once, preserving the original general20/21 exception and signed tick delta.
bool source_announcer_remember_tu15(VistaAnnouncerEventState&,
    const VistaAnnouncerEvent&,std::uint32_t sound,std::uint32_t tick,
    std::int16_t tick_rate,const VistaLoadingFloatingPoint&);
} // namespace halo2
