#include "halo2/source_loop_sound_request_tu15.hpp"

#include <bit>
#include <iostream>

// This differential fixture constructs a request for the authenticated
// radio_default_in loop component, with an already admitted impulse parent.
// The live audio scheduler does not use this fixture.
int main() {
    std::uint32_t child, gain_bits, phase, priority, parent, effect;
    while (std::cin >> std::hex >> child >> gain_bits >> phase >> priority >> parent >> effect) {
        std::array<std::uint32_t, 17> location;
        for (auto& word : location) {
            if (!(std::cin >> word)) return 1;
        }
        if (phase > 2) return 1;

        std::array<std::byte,68> location_bytes{};
        for (std::size_t word = 0; word < location.size(); ++word) {
            for (unsigned byte = 0; byte < 4; ++byte) {
                location_bytes[word * 4 + byte] = std::byte(location[word] >> (byte * 8));
            }
        }
        std::optional<std::array<float,2>> bounds;
        if(location[0]&0x100u)bounds=std::array{std::bit_cast<float>(location[13]),std::bit_cast<float>(location[14])};
        const auto request=halo2::source_sound_loop_companion_request_tu15(child,priority,parent,effect|0x4000u,
            location_bytes,0,std::bit_cast<float>(gain_bits),0,phase,false,bounds);
        for (const auto argument : request.arguments) std::cout << std::hex << argument << ' ';
        for (const auto byte : request.location) std::cout << std::to_integer<unsigned>(byte) << ' ';
        std::cout << '\n';
    }
    return std::cin.eof() ? 0 : 1;
}
