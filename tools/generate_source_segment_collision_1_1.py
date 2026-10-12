#!/usr/bin/env python3
"""Compile the complete original world segment dispatcher and geometry owners.

Retains original terrain, cluster, instance, posed-object, attachment, breakable
surface and Havok tree traversal. Reached virtual shape/platform owners require
their actual ABI; a terrain-only answer is never a missing-provider fallback.
"""
import generate_unit_animation_selection_runtime_1_1 as generator

generator.ENTRIES=(0x16b240,0x16a560,0x16a760,0x16cf80,0xb9830,0xb98a0,
    0x11fd00,0x16af90,0x1f3900,0x11d4a0,0x1e2020,0x1e20c0,0x1e25c0,
    0x14b9d0,0x16a490,0x16a5c0,0x1f3500,0x1f3740,0x1f3590,0x1f3660,
    0x1e23f0,0xda0e0,0x246a50,0x29c7d0,0x2e5ce0,0x2e5ca0,0x2e5c00,
    0x306f40,0x2e5bf0,0x307d90,0x306f50,0x1eed10,0x306f70,
    0x246ce0,0x246cb0,0x246c80,0x246a40,0x16ac20,0x16a430,0x185a60)
generator.EXTRA_TABLES={0x307bb8:32}
generator.RUNTIME_HEADER='halo2/source_segment_collision.hpp'
generator.RUNTIME_FUNCTION='source_run_segment_collision_bodies_1_1'
generator.CONFIGURATION_TOOL=__file__
if __name__=='__main__':generator.main()
