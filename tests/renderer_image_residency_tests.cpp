#include "../src/rendering/renderer/renderer_image_residency.hpp"
#include <iostream>

using namespace halo2;
int main() try {
    const auto check = [](bool ok, const char *message) {
        if (!ok) throw std::runtime_error(message);
    };
    Texture original{32,32,11,std::vector<std::byte>(4096),{{32,32,0,4096}}};
    auto changed = original;
    changed.pixels[17] = std::byte{1};
    check(detail::image_residency_bucket({original.pixels.data(),original.pixels.size()}) ==
          detail::image_residency_bucket({changed.pixels.data(),changed.pixels.size()}),
          "Fixture did not force a bucket collision");
    check(!detail::same_resident_image(original,changed),"Bucket collision shared different image pixels");
    check(detail::same_resident_image(original,original),"Identical image no longer shares residency");
    for (unsigned field = 0; field < 11; ++field) {
        auto other = original;
        switch (field) {
        case 0: ++other.width; break;
        case 1: ++other.height; break;
        case 2: ++other.format; break;
        case 3: ++other.layers; break;
        case 4: ++other.depth; break;
        case 5: ++other.mips[0].width; break;
        case 6: ++other.mips[0].height; break;
        case 7: ++other.mips[0].offset; break;
        case 8: ++other.mips[0].size; break;
        case 9: ++other.mips[0].depth; break;
        case 10: other.mips.clear(); break;
        }
        check(!detail::same_resident_image(original,other),"Image shape/mip distinction lost");
    }
    std::cout << "PASS full pixel/shape/mip equality survives forced sampled-hash collision\n";
} catch(const std::exception &error) {
    std::cerr << error.what() << '\n'; return 1;
}
