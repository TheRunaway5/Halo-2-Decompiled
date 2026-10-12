#include "halo2/cache.hpp"
#include "halo2/texture.hpp"
#include <bit>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>

namespace {
void expect(bool value) { if (!value) throw std::runtime_error("test assertion failed"); }
template<class F> void rejects(F action) {
    bool rejected = false;
    try { action(); } catch (const halo2::FormatError&) { rejected = true; }
    expect(rejected);
}
void put(std::vector<std::byte>& data, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) data.at(offset + i) = std::byte((value >> (i * 8)) & 255);
}
}
int main() {
    try {
        std::vector<std::byte> data{std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12}, std::byte{0}};
        halo2::Bytes bytes(data);
        expect(bytes.u16(0) == 0x5678);
        expect(bytes.u32(0) == 0x12345678);
        expect(bytes.string(0, 5).size() == 4);
        expect(bytes.slice(5, 0).empty());
        rejects([&] { bytes.u16(4); });
        rejects([&] { bytes.u32(2); });
        rejects([&] { bytes.slice(std::numeric_limits<std::size_t>::max(), 2); });
        rejects([&] { bytes.slice(1, std::numeric_limits<std::size_t>::max()); });
        rejects([&] { bytes.string(0, 4); });
        rejects([&] { halo2::Bytes({}).u32(0); });
        // Xbox rectangular Morton layout: the first four source bytes describe a 2x2 tile.
        std::vector<std::byte> swizzled;
        for (unsigned value : {0, 1, 4, 5, 2, 3, 6, 7}) swizzled.push_back(std::byte(value));
        auto texture = halo2::prepare_texture(halo2::Bytes(swizzled), 4, 2, 1, true);
        expect(texture.format == 11 && texture.pixels.size() == 32);
        for (unsigned i = 0; i < 8; ++i) {
            expect(std::to_integer<unsigned>(texture.pixels[i * 4]) == i);
            expect(texture.pixels[i * 4 + 1] == texture.pixels[i * 4]);
            expect(texture.pixels[i * 4 + 3] == std::byte{255});
        }
        data = {std::byte{3}, std::byte{5}, std::byte{7}, std::byte{11}};
        texture = halo2::prepare_texture(halo2::Bytes(data), 1, 1, 11, false);
        expect(texture.pixels == std::vector<std::byte>({std::byte{7}, std::byte{5}, std::byte{3}, std::byte{11}}));
        data.assign(16, std::byte{42});
        expect(halo2::prepare_texture(halo2::Bytes(data), 4, 4, 14, false).pixels.size() == 8);
        expect(halo2::prepare_texture(halo2::Bytes(data), 4, 4, 15, false).pixels == data);
        expect(halo2::prepare_texture(halo2::Bytes(data), 4, 4, 16, false).pixels == data);
        rejects([&] { halo2::prepare_texture(halo2::Bytes(data), 8, 8, 16, false); });
        rejects([&] { halo2::prepare_texture(halo2::Bytes(data), 3, 2, 1, true); });
        rejects([&] { halo2::prepare_texture(halo2::Bytes(data), 4, 4, 16, true); });
        rejects([&] { halo2::prepare_texture(halo2::Bytes(data), 0, 4, 1, false); });
        rejects([&] { halo2::prepare_texture(halo2::Bytes(data), 8192, 4, 1, false); });
        expect(halo2::retail_mip_count(512, 128, 1, 16, false, 9) == 7);
        expect(halo2::retail_mip_count(128, 512, 1, 14, false, 9) == 7);
        expect(halo2::retail_mip_count(512, 256, 1, 16, false, 9) == 7);
        expect(halo2::retail_mip_count(512, 256, 1, 1, false, 9) == 9);
        expect(halo2::retail_mip_count(512, 256, 1, 16, false, 3) == 3);
        expect(halo2::retail_mip_count(512, 256, 1, 1, true, 9) == 0);
        expect(halo2::retail_mip_count(511, 256, 1, 1, false, 9) == 0);
        // BC mips include complete 4x4 blocks even below 4 texels. Distinct bytes
        // catch accidental regeneration, skipped levels and incorrect tail offsets.
        data.clear();
        for (const auto [size, value] : {std::pair{16, 10}, {8, 20}, {8, 30}, {8, 40}})
            data.insert(data.end(), size, std::byte(value));
        texture = halo2::prepare_texture(halo2::Bytes(data), 8, 4, 14, false, 3);
        expect(texture.pixels == data && texture.mips.size() == 4);
        expect(texture.mips[1].width == 4 && texture.mips[1].height == 2 && texture.mips[1].offset == 16);
        expect(texture.mips[3].width == 1 && texture.mips[3].height == 1 && texture.mips[3].offset == 32);
        rejects([&] { halo2::prepare_texture(halo2::Bytes(data), 8, 4, 14, false, 4); });
        data.pop_back();
        rejects([&] { halo2::prepare_texture(halo2::Bytes(data), 8, 4, 14, false, 3); });
        // Deswizzle each authored level at its own rectangular dimensions.
        data = swizzled;
        data.insert(data.end(), {std::byte{80}, std::byte{90}, std::byte{100}});
        texture = halo2::prepare_texture(halo2::Bytes(data), 4, 2, 1, true, 2);
        expect(texture.mips.size() == 3 && texture.pixels.size() == 44);
        expect(texture.mips[1].offset == 32 && texture.mips[2].offset == 40);
        expect(texture.pixels[32] == std::byte{80} && texture.pixels[36] == std::byte{90} && texture.pixels[40] == std::byte{100});
        // A synthetic header exercises size checks before any tag data allocation.
        auto temp = std::filesystem::temp_directory_path() / ("halo2-cache-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(temp);
        struct Clean { std::filesystem::path p; ~Clean() { std::filesystem::remove_all(p); } } cleanup{temp};
        const auto file = temp / "invalid.map";
        data.assign(2048, std::byte{0});
        auto write = [&] { std::ofstream out(file, std::ios::binary); out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size())); };
        write();
        rejects([&] { halo2::Cache cache(file); });
        put(data, 0, 0x68656164); put(data, 4, 8); put(data, 2044, 0x666f6f74);
        put(data, 8, 4096); write();
        rejects([&] { halo2::Cache cache(file); });
        put(data, 8, 2048); write();
        rejects([&] { halo2::Cache cache(file); }); // Unsupported build, no accidental Vista handling.
        std::cout << "cache bounds, bitmap swizzle, compressed payload and invalid-input tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
