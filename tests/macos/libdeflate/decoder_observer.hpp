#pragma once
#include <cstdint>
struct DecoderObservation {
    std::uint64_t calls{},successes{},input_bytes{},output_bytes{};
};
DecoderObservation decoder_observation();
