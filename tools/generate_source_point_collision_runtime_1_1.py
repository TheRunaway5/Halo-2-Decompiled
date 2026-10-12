#!/usr/bin/env python3
"""Compile complete original point containment and object hierarchy owners.

Actual source model permutations, current node transforms, exclusions and
world spatial banks remain required. No posed-object miss is supplied.
"""
import generate_unit_animation_selection_runtime_1_1 as generator
generator.ENTRIES=(0x16a9c0,0x16a8a0,0x16a7e0,0x1f3780,0xbabd0,0x143e70)
generator.RUNTIME_HEADER='halo2/source_point_collision_runtime.hpp'
generator.RUNTIME_FUNCTION='source_run_point_collision_bodies_1_1'
generator.CONFIGURATION_TOOL=__file__
if __name__=='__main__':generator.main()
