#pragma once
#include "halo2/source_object_light_uv.hpp"
#include "halo2/source_object_light_mix.hpp"
#include "halo2/object_lighting.hpp"
#include <functional>
namespace halo2 {
using SourceObjectLightContactPacket=std::array<std::uint32_t,14>;
struct SourceObjectLightPrtResource {
    std::uint8_t declaration{};
    //Resolved24490 descriptor+4 size offset plus+C resident base. Literal CPU point
    //payload, separately supplied from GPU PRT declarations/expanded vertices.
    std::vector<std::byte> points;
};
struct SourceObjectLightSampleContext {
    SourceObjectLightContactPacket contact; //+14 geometry section differs from+1C PRT cache index.
    std::int32_t model_prt_count{};
    const SourceObjectLightPrtResource* prt_resource{}; //actual24490 result.
    std::span<const SourcePrtContactCacheRecord> current_prt_cache;
    std::int32_t current_prt_count{};
    std::span<const std::byte> current_prt_arena;
    Vec3 default_incident;
};
enum class SourceObjectLightSampleStatus:std::int32_t {resolved=0,unavailable=1,miss=2};
//D30D0's selected bitmap/residency service, at source flags0 and detail1.
//NONE means it returnedfalse; a successful RGB may still be outside[0,1].
using SourceObjectLightDiffuseSample=std::function<std::optional<Vec3>(Vec2)>;
//Complete D1E10 object branch, with explicit original contact/residency/cache
//and bitmap service inputs. A nonzero source status can still write the
//sample. Callers must preserve that distinction from accepted query success.
SourceObjectLightSampleStatus source_object_light_sample(
    const SourceObjectLightDataGeometry&,const SourceObjectLightSampleContext&,
    const SourceObjectLightDiffuseSample&,ObjectLightmapSample& caller_output);
std::optional<Vec3> source_object_light_texture_diffuse(const Texture&,Vec2);
}
