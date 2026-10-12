#include "physics_step_internal.hpp"
#include <iostream>

namespace {
struct Memory : halo2::SourcePhysicsContactMemory {
    mutable unsigned reads = 0;
    void read(std::uint32_t p, std::span<std::byte> out) const override {
        if (p != 0x1234 || out.size() > 4)
            throw std::runtime_error("Unexpected scalar memory access");
        ++reads;
        for (unsigned n = 0; n < out.size(); ++n) out[n] = std::byte(0x11 * (n + 1));
    }
    void write(std::uint32_t, std::span<const std::byte>) override {
        throw std::runtime_error("Unexpected scalar memory write");
    }
};
}
int main() {
    try {
        Memory memory;
        for (const auto width : {0u, 5u, 8u, 16u}) {
            bool rejected = false;
            try { halo2::physics_step_detail::r(memory, 0x1234, width); }
            catch (const std::runtime_error &e) {
                rejected = std::string(e.what()) == "Invalid original physics scalar read width";
            }
            if (!rejected || memory.reads)
                throw std::runtime_error("Invalid width accessed memory before rejection");
        }
        for (const auto &[width, value] : {std::pair{1u, 0x11u}, {2u, 0x2211u}, {4u, 0x44332211u}})
            if (halo2::physics_step_detail::r(memory, 0x1234, width) != value)
                throw std::runtime_error("Scalar extent or byte order differs");
        if (memory.reads != 3) throw std::runtime_error("Scalar read count differs");
        std::cout << "PASS scalar widths reject before memory access and retain valid extents\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
