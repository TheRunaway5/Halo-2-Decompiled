#include "halo2/macos/application.hpp"
#include "halo2/macos/executable.hpp"
#import <Foundation/Foundation.h>
#include <cstdlib>
#include <stdexcept>

namespace halo2 {
std::optional<MacOSApplicationLayout> macos_application_layout() {
    const auto contents = macos_bundle_contents();
    if (contents.empty()) return {};
    const auto resources = std::filesystem::canonical(contents / "Resources");
    if (!std::filesystem::is_regular_file(resources / "tools/setup_assets.py"))
        throw std::runtime_error("The launcher bundle is missing its setup tools");
    std::filesystem::path data;
    if (const auto* override = std::getenv("H2_USER_DATA_DIR")) {
        data = override;
        if (!data.is_absolute())
            throw std::runtime_error("H2_USER_DATA_DIR must name an absolute user-data directory");
    } else {
        @autoreleasepool {
            NSArray<NSURL*>* urls = [[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory
                inDomains:NSUserDomainMask];
            NSURL* directory = [urls firstObject];
            const char* path = [[directory path] UTF8String];
            if (!path) throw std::runtime_error("The Mac Application Support directory is unavailable");
            data = std::filesystem::path(path) / "net.halo2forever";
        }
    }
    data = std::filesystem::weakly_canonical(data);
    const auto bundle = contents.parent_path();
    const auto within = [](const auto& child, const auto& parent) {
        const auto relative = child.lexically_relative(parent);
        return !relative.empty() && *relative.begin() != "..";
    };
    if (within(data, bundle) || within(bundle, data))
        throw std::runtime_error("Mac user data must be separate from the application bundle");
    return MacOSApplicationLayout{data, resources};
}
}
