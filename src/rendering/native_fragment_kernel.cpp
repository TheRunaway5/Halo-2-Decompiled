#include "halo2/native_fragment_kernel.hpp"
#include "halo2/cache.hpp"
#include "native_fragment_kernels.generated.hpp"
#include <algorithm>
#include <string>

namespace halo2 {
std::vector<std::string_view> native_fragment_kernel_paths(std::string_view family) {
    std::vector<std::string_view> result;
    for(const auto &entry:generated_native_fragment_kernels::entries)
        if(entry.family==family)result.push_back(entry.path);
    if(result.empty())throw FormatError("Native fragment family has no compiled programs: "+std::string(family));
    return result;
}
std::string_view native_fragment_kernel_path(std::string_view family,
    std::span<const std::uint32_t> complete_program) {
    if(complete_program.size()!=60)throw FormatError("Native fragment requires complete source identity");
    std::array<std::uint32_t,60> controls;
    std::copy(complete_program.begin(),complete_program.end(),controls.begin());
    for(unsigned i=10;i<26;++i)controls[i]=0;
    controls[43]=controls[44]=0;
    for(const auto &entry:generated_native_fragment_kernels::entries)
        if(entry.family==family && entry.controls==controls)return entry.path;
    throw FormatError("No exact compiled native fragment kernel for family "+std::string(family));
}
}
