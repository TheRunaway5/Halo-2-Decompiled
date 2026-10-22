#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace halo2 {
// Owning native transport of original25AAF1's four literal-message rows.
// No local-view index or borrowed source address crosses a native snapshot.
struct SourceHudMessages {
    std::array<std::array<std::byte,0x88>,4> rows{};
    bool operator==(const SourceHudMessages &)const=default;
};
struct SourceHudMessageEvent {
    std::uint32_t player=0xffffffffu,tick{};
    std::u16string text;
};
struct SourceHudMessageLine {
    std::u16string text;
    float alpha{};
};
// Whole literal-message admission/oldest replacement25AAF1/25ABDE. The
// source-wide sequence byte is independent of each recipient's four rows.
void source_hud_message_submit(SourceHudMessages &,std::uint8_t &sequence,
    std::uint32_t tick,std::u16string_view);
//25ACDB expiry and25B2C5..25B326 fade. Paused native worlds retain their
// completed game tick; rendering never advances this queue's clock.
void source_hud_messages_expire(SourceHudMessages &,std::uint32_t tick,float tick_seconds,
    float hold_seconds,float fade_seconds);
float source_hud_message_alpha(float age,float hold,float fade,float opacity=1);
std::vector<SourceHudMessageLine> source_hud_message_lines(const SourceHudMessages &,
    std::uint32_t tick,float tick_seconds,float hold,float fade,unsigned maximum,float opacity=1);
void validate_source_hud_messages(const SourceHudMessages &);
// Original13BD5E's ordinary Spartan/Elite literal text RGB.
std::array<float,3> source_hud_message_color(bool elite);
}
