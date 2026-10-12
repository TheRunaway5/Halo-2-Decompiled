#pragma once
#include "halo2/source_draw_unit.hpp"

namespace halo2 {
// Actual current owner banks, expressed in the caller's native address/token
// namespace. A tag table entry is16 bytes; an object header retains the real
// pool stride/salt/body pointer. The reader must resolve current mutable data,
// including writes made by an actual resource request.
struct SourceDrawResourceWorld {
    std::uint32_t tag_table{},bsp{},lightmaps{},objects{};
    std::function<std::span<const std::byte>(std::uint32_t,std::size_t)> read;
    // Consumed12DE70(mode,address) result. Metadata/resource counts alone do
    // not supply this publication decision or any resource pointer writes.
    std::function<bool(std::uint32_t,std::uint32_t)> request;
};
// Whole44940. Selector consumes only its low byte for BSP geometry.
std::uint32_t source_draw_geometry_address(const SourceCullDrawUnit&,
    std::uint32_t selector,const SourceDrawResourceWorld&);
// Whole449E0, including request-before-pointer-read and actual null payloads.
std::uint32_t source_draw_resident_geometry(const SourceCullDrawUnit&,
    std::uint32_t selector,bool request,const SourceDrawResourceWorld&);
// Whole2D000 and its completeBADC0 fallback lookup. Returns original0..3;
// model is the actual unit+4 datum, instance the consumed11-bit source value.
std::uint32_t source_draw_lighting_classification(std::uint32_t owner,
    std::uint32_t model,std::uint32_t instance,const SourceDrawResourceWorld&);
// Connect these three owners to2CBF0. Context, center and rectangle ownership
// remain caller inputs. The supplied world must outlive consumption.
SourceDrawUnitServices source_draw_resource_services(const SourceDrawResourceWorld&);
}
