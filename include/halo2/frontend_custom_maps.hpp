#pragma once
#include <filesystem>
namespace halo2 {
struct FrontendContent;
// Bind authored catalog IDs to exact admitted custom caches. Stock catalog
// rows and their default filename routing remain owned by the original data.
void bind_frontend_custom_maps(FrontendContent &, const std::filesystem::path &maps);
}
