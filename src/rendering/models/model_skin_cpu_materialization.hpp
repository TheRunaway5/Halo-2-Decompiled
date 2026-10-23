#pragma once
#include "halo2/math.hpp"
#include "halo2/model_skin_gpu_admission.hpp"
#include "halo2/transform.hpp"
#include "rendering/models/model_skin_vertices.hpp"
#include <cerrno>
#include <cfenv>
#include <cstring>
#include <exception>
#include <limits>
#include <type_traits>

namespace halo2 {
namespace model_skin_cpu_materialization_detail {
struct AddressRange {std::uintptr_t first{},end{};};
template<class T> AddressRange address(std::uintptr_t first,std::size_t count) {
    if(!count)return {};
    if(!first || first%alignof(T) ||
       count>(std::numeric_limits<std::uintptr_t>::max()-first)/sizeof(T))
        throw FormatError("CPU model skin has an invalid input address range");
    return {first,first+count*sizeof(T)};
}
template<class T,std::size_t N> AddressRange address(std::span<T,N> values) {
    return address<T>(reinterpret_cast<std::uintptr_t>(values.data()),values.size());
}
inline bool overlaps(AddressRange a,AddressRange b)noexcept {
    return a.first<b.end && b.first<a.end;
}
class Environment {
    std::fenv_t saved_{};
    int error_=errno;
    bool available_=std::fegetenv(&saved_)==0;
public:
    Environment()=default;
    Environment(const Environment&)=delete;
    Environment& operator=(const Environment&)=delete;
    ~Environment()noexcept {
        // A caller must never continue with a partially restored FP owner.
        if(available_ && std::fesetenv(&saved_)!=0)std::terminate();
        errno=error_;
    }
    void prepare() {
        if(!available_)throw FormatError("CPU model skin cannot save the caller floating-point environment");
        std::fenv_t held{};
        if(std::feholdexcept(&held)!=0 || std::fesetround(FE_TONEAREST)!=0)
            throw FormatError("CPU model skin cannot establish masked round-nearest arithmetic");
#if defined(__x86_64__) || defined(_M_X64)
        // Retain DAZ/FTZ. The shared numeric admission excludes every
        // denormal input/intermediate; these controls cannot change output.
        _mm_setcsr((_mm_getcsr()&~0x6000u)|0x1f80u);
#endif
    }
};
template<class Record> SkinBinding binding(const Record &record) {
    SkinBinding result;
    std::memcpy(result.weights.data(),record.weights.data(),sizeof(result.weights));
    int last=-1;
    for(unsigned j=0;j<4;++j) {
        const auto bits=model_skin_admission_detail::absolute_bits(result.weights[j]);
        if(!model_skin_admission_detail::normal_bounded(bits,0x3f800000u))
            throw FormatError("CPU model skin weight leaves its admitted domain");
        if(bits)last=int(j);
    }
    for(int j=last;j>=0;--j) {
        const auto bone=record.bones[unsigned(j)];
        if(bone>std::numeric_limits<std::uint16_t>::max() || bone>=record.palette_count)
            throw FormatError("CPU model skin bone escapes its original owner bank or index width");
        result.bones[unsigned(j)]=std::uint16_t(bone);
    }
    return result;
}
inline void place_identity(Vertex &vertex) {
    const Transform identity{};
    const auto rotate=[&](Vec3 v) {
        return identity.basis[0]*v.x+identity.basis[1]*v.y+identity.basis[2]*v.z;
    };
    vertex.position=identity.position+rotate(vertex.position)*identity.scale;
    vertex.normal=rotate(vertex.normal);
    vertex.tangent=rotate(vertex.tangent);
    vertex.binormal=rotate(vertex.binormal);
}
}

// Materialize only declared destination records, using the existing skin
// kernel and first-person append graph. Record is ModelSkinGpuRecord-shaped;
// this private header deliberately has no Vulkan/service dependency.
//
// The caller owns valid, accessible typed spans and keeps records/palette
// immutable throughout. Structural range checks cannot prove an arbitrary
// nonnull address belongs to a live object or readable mapped allocation.
// Reject overlapping mapped/input storage and validate the COMPLETE job before
// the first write. Declared owner extents are authoritative: no other owner
// identity can be recovered from a flattened palette alone. Unmapped vertices
// are uninterpreted bytes and remain untouched. No explicit heap allocation is
// performed. Increasing destinations check aliases in O(n); unordered jobs
// use O(n^2) comparisons. Shared numeric qualification scans the full palette
// once plus each record's owner extent, intentionally retaining the existing
// arithmetic predicate instead of duplicating its proof.
// The caller's complete FP environment and errno are restored, including on
// validation/establishment exceptions; failed restoration terminates.
template<class Record,std::size_t N>
void model_skin_cpu_materialize(std::span<Vertex> mapped,std::span<Record,N> records,
    std::span<const std::array<float,12>> palette) {
    using RawRecord=std::remove_cv_t<Record>;
    static_assert(std::is_trivially_copyable_v<RawRecord> && std::is_standard_layout_v<RawRecord>);
    static_assert(std::is_same_v<decltype(RawRecord::destination),std::uint32_t> &&
                  std::is_same_v<decltype(RawRecord::palette),std::uint32_t> &&
                  std::is_same_v<decltype(RawRecord::palette_count),std::uint32_t> &&
                  std::is_same_v<decltype(RawRecord::post_identity_placement),std::uint32_t> &&
                  std::is_same_v<decltype(RawRecord::bones),std::array<std::uint32_t,4>> &&
                  std::is_same_v<decltype(RawRecord::weights),std::array<float,4>>);
    using namespace model_skin_cpu_materialization_detail;
    Environment environment;
    const auto output=address(mapped),inputs=address(records),rows=address(palette);
    if(overlaps(output,inputs) || overlaps(output,rows))
        throw FormatError("CPU model skin inputs alias mapped destination storage");
    for(const auto &row:palette)for(const auto value:row)
        if(!model_skin_admission_detail::finite_bounded(value,0x49800000u))
            throw FormatError("CPU model skin unused palette leaves its finite domain");
    bool increasing=true;
    for(std::size_t i=0;i<records.size();++i) {
        const auto &record=records[i];
        if(record.destination>=mapped.size())throw FormatError("CPU model skin destination escapes its mapped bank");
        if(record.post_identity_placement>1)throw FormatError("CPU model skin has an unknown post-placement mode");
        if(record.palette>palette.size() || record.palette_count>palette.size()-record.palette)
            throw FormatError("CPU model skin palette extent escapes its declared owner bank");
        const auto skin=binding(record);
        if(!model_skin_gpu_arithmetic_admitted(mapped.subspan(record.destination,1),{&skin,1},
            palette.subspan(record.palette,record.palette_count)))
            throw FormatError("CPU model skin vertex or referenced palette leaves its admitted domain");
        if(i && records[i-1].destination>=record.destination)increasing=false;
    }
    if(!increasing)for(std::size_t i=0;i<records.size();++i)for(std::size_t j=0;j<i;++j)
        if(records[i].destination==records[j].destination)
            throw FormatError("CPU model skin destinations alias different records");
    environment.prepare();
    for(const auto &record:records) {
        const auto skin=binding(record);
        auto vertex=mapped.subspan(record.destination,1);
        skin_model_vertices(vertex,{&skin,1},palette.subspan(record.palette,record.palette_count),true);
        if(record.post_identity_placement)place_identity(vertex.front());
    }
}
}
