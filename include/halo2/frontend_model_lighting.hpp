#pragma once
#include "halo2/object_lighting.hpp"
namespace halo2 {
// BBFC0/BC070: literal HS angles in degrees, RGB values and original .8
// accuracy/shadow opacity. 2A2349 copies ambient RGB independently.
void frontend_cinematic_primary(ObjectLightingState &,float elevation,float azimuth,Vec3 color);
void frontend_cinematic_secondary(ObjectLightingState &,float elevation,float azimuth,Vec3 color);
void frontend_cinematic_ambient(ObjectLightingState &,Vec3 color);
}
