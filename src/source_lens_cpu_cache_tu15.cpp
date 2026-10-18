#include "source_lens_cpu_cache_tu15.hpp"
#include "halo2/render_program.hpp"
#include <array>
namespace halo2::private_lens_cpu {
namespace {
template<std::size_t N> std::vector<std::byte> encode(const std::array<unsigned,N>& words) {
    std::vector<std::byte> bytes(N*4);
    for(unsigned n=0;n<N;++n)for(unsigned b=0;b<4;++b)bytes[n*4+b]=std::byte(words[n]>>(b*8));
    return bytes;
}
}
std::vector<Region> mcc_cache_plan() {
    std::vector<Region> result;
    const ShaderTextureState sampler;
    // Original texture-state records have32 DWORD slots, addressed by the
    // original stage*128 producer/consumer arithmetic. The real lens GPU
    // consumer selects precisely ShaderTextureState{}; these fourteen input
    // fields describe that same native sampler. Remaining original CPU-cache
    // slots retain their authenticated loader-zero preimage, not GPU state.
    const std::array<unsigned,32> row{sampler.address_u,sampler.address_v,sampler.address_w,
        sampler.mag_filter,sampler.min_filter,sampler.mip_filter,sampler.lod_bias,
        sampler.max_mip,sampler.anisotropy,sampler.color_key_op,sampler.color_sign,
        sampler.alpha_kill,sampler.border_color,sampler.key_color};
    for(unsigned stage=0;stage<4;++stage)
        result.push_back({0x40eeb0u+stage*128u,SourceRuntimeMemory::Access::ReadWrite,
            "native lens original CPU texture-state cache32",encode(row)});
    const auto writable=[&](unsigned address,unsigned bytes,const char* role) {
        result.push_back({address,SourceRuntimeMemory::Access::ReadWrite,role,std::vector<std::byte>(bytes)});
    };
    writable(0x411568,4,"native lens live renderer logical driver alias");
    writable(0x411570,4,"native lens original CPU dirty-mask cache");
    // Whole4044B0 publishes57 original program DWORDs here, then publishes
    // its separate final mode word at411798. No precomputed program output.
    writable(0x411578,57u*4,"native lens original pixel-program CPU shadow57");
    writable(0x411798,4,"native lens original pixel-program mode cache");
    writable(0x4116c0,4,"native lens original CPU depth-control cache");
    writable(0x4117b4,4,"native lens original CPU depth-mode cache");
    writable(0x4c4c58,4,"native lens original CPU raster publication cache");
    // Actual lens uses the owned untransformed UV0 program/default sampler.
    // Whole409EE0 reads slot0 when its private TSS index at+70 is zero.
    // Preserve only that authenticated loader-zero entry; other indices and
    // any attempt to produce a different coordinate table remain unsupported.
    result.push_back({0x411700,SourceRuntimeMemory::Access::ReadOnly,
        "native lens original coordinate lookup slot0 only",std::vector<std::byte>(4)});
    // Authenticated TU1.5 read-only arrays consumed by the unchanged genuine
    // 1301E0 header producer and139080 child. MCC projection retains its
    // existing finite supported-format guard before these lookups; entries
    // for unsupported formats are data only, never new format admission.
    result.push_back({0x45c488,SourceRuntimeMemory::Access::ReadOnly,
        "native lens authored nonlinear-format header table24",encode(std::array<unsigned,24>{0x00000019u,0x00000000u,0x00000001u,0x0000001au,0xffffffffu,0xffffffffu,0x00000005u,0xffffffffu,0x00000002u,0x00000004u,0x00000007u,0x00000006u,0xffffffffu,0xffffffffu,0x0000000cu,0x0000000eu,0x0000000fu,0x0000000bu,0x0000000bu,0xffffffffu,0xffffffffu,0xffffffffu,0x00000028u,0x00000028u})});
    result.push_back({0x45c4e8,SourceRuntimeMemory::Access::ReadOnly,
        "native lens authored linear-format header table24",encode(std::array<unsigned,24>{0x0000001fu,0x00000013u,0x0000001bu,0x00000020u,0xffffffffu,0xffffffffu,0x00000011u,0xffffffffu,0x00000010u,0x0000001du,0x0000001eu,0x00000012u,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu,0x00000017u,0x00000017u})});
    result.push_back({0x45f290,SourceRuntimeMemory::Access::ReadOnly,
        "native lens authored bit-depth table24",encode(std::array<unsigned,12>{0x00080008u,0x00100008u,0x00000000u,0x00000010u,0x00100010u,0x00200020u,0x00000000u,0x00080004u,0x00080008u,0x00800008u,0x00300060u,0x00100010u})});
    return result;
}
}
