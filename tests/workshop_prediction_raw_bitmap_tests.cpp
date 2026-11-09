#include "rendering/mcc_cpu_bitmap_owner.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <iostream>

using namespace halo2;
using namespace halo2::private_cpu_bitmap;
namespace {
void need(bool value, const char* message) { if (!value) throw FormatError(message); }
void put(std::vector<std::byte>& bytes, unsigned at, unsigned value, unsigned size = 2) {
    for (unsigned n = 0; n < size; ++n) bytes[at + n] = std::byte(value >> (n * 8));
}
template<class F> void refused(F&& action) {
    bool failed = false;
    try { action(); } catch (const FormatError&) { failed = true; }
    need(failed, "Raw prediction resource negative control was admitted");
}
}

int main() try {
    // Independent literal byte strides of the uncompressed formats in the
    // unchanged TU1.5 format table. Palette/float/block formats stay separate.
    const std::pair<unsigned, unsigned> formats[] = {
        {0, 1}, {1, 1}, {2, 1}, {3, 2}, {6, 2}, {8, 2}, {9, 2}, {10, 4}, {11, 4}, {22, 2}, {23, 2}};
    unsigned cases = 0, controls = 0;
    for (const auto [format, stride] : formats) {
        Tag tag;
        tag.group = "bitm";
        tag.datum = 0xe1230042;
        tag.name = "declared complete raw prediction fixture";
        tag.address = 0x20000000;
        tag.size = 0x4c;
        std::vector<std::byte> root(tag.size), rows(168);
        put(root, 0x44, 1, 4);
        put(root, 0x48, 0x20001000, 4);
        put(rows, 0, tag_groups::bitmap.value, 4);
        put(rows, 4, 8);
        put(rows, 6, 4);
        put(rows, 8, 1);
        put(rows, 12, format);
        put(rows, 14, 0x201);
        put(rows, 20, 2);
        // The complete 8x4,4x2,2x1 mip bank has42 literal source texels.
        BitmapResource resource{8, 4, 1, 0, std::uint16_t(format), {}};
        resource.levels[0].resize(42 * stride);
        for (unsigned n = 0; n < resource.levels[0].size(); ++n) resource.levels[0][n] = std::byte(n * 53 + 17);
        put(rows, 0x50, resource.levels[0].size(), 4);
        const auto texture = prepare_texture(Bytes(resource.levels[0]), 8, 4, format, false, 2);
        const auto before = resource.levels;
        const auto owner = project(tag, 0, root, 0x20001000, rows, resource, texture, true);
        need(raw_resource_format(format) && source_bitmap_format_bits(format) == stride * 8 &&
             owner.encoded == resource.levels[0] && owner.rows == rows && owner.root == root &&
             std::ranges::equal(owner.common, std::span(rows).first(24)) && before == resource.levels,
             "Raw prediction changed complete source metadata or encoded mip bytes");
        const bool sampled = format == 10 || format == 11;
        need(cpu_sampling_format(format) == sampled, "Raw resource admission changed the CPU sampling domain");
        if (sampled) (void)project(tag, 0, root, 0x20001000, rows, resource, texture, false);
        else { refused([&] { (void)project(tag, 0, root, 0x20001000, rows, resource, texture, false); }); ++controls; }
        auto short_resource = resource;
        short_resource.levels[0].pop_back();
        refused([&] { (void)project(tag, 0, root, 0x20001000, rows, short_resource, texture, true); });
        auto padded = resource;
        padded.levels[0].push_back(std::byte{1});
        refused([&] { (void)project(tag, 0, root, 0x20001000, rows, padded, texture, true); });
        auto changed = texture;
        changed.pixels[0] ^= std::byte{1};
        refused([&] { (void)project(tag, 0, root, 0x20001000, rows, resource, changed, true); });
        for (unsigned bit : {8u, 16u, 32u}) {
            auto changed_rows = rows;
            changed_rows[14] |= std::byte(bit);
            refused([&] { (void)project(tag, 0, root, 0x20001000, changed_rows, resource, texture, true); });
        }
        controls += 6;
        ++cases;
    }
    for (unsigned format : {4u, 5u, 7u, 12u, 13u, 17u, 18u, 19u, 20u, 21u, 24u, 0xffffu}) {
        need(!raw_resource_format(format) && !cpu_sampling_format(format), "Unqualified bitmap format was admitted");
        ++controls;
    }
    need(cpu_sampling_format(14) && cpu_sampling_format(15) && cpu_sampling_format(16), "Existing DXT sampling domain changed");
    need(cases == 11 && controls == 87, "Raw prediction coverage is incomplete");
    std::cout << "PASS raw prediction storage:11 formats, complete mip banks,87 rejection/domain controls; CPU sampling admission unchanged\n";
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
