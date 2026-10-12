#pragma once
#include "halo2/object_light_spatial.hpp"

namespace halo2 {
// Literal B88E0/16B430 inputs. The primitive sphere is the retained +40/+4C
// cache, separate from the rendering/light-query sphere at +30/+3C.
struct SourceObjectPrimitiveInput {
    std::uint8_t header_flags{}; // Object header+2.
    std::uint32_t object_flags{}; // Object+4, including collidable bit9.
    std::int8_t type{}; // Object+AA; B88E0 sign-extends it to a short.
    std::uint8_t unit_flags_10a{}, unit_flags_348{}; // Type0-only consumers.
    std::uint32_t prop_flags_12c{}; // Type11-only bit1 consumer.
    ObjectLightSpatialSphere primitive_sphere;
};
// Exact 20-byte source primitive. Noncollidable objects receive flags0.
// This helper neither chooses a spatial list nor performs registration.
ObjectLightSpatialPrimitive source_object_registration_primitive(
    const SourceObjectPrimitiveInput&);
}
