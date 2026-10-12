#!/usr/bin/env python3
"""Compile the complete original DEFC0 postphysics phase and its packet owners.

Actual geometry, damage, animation and physical services keep their original
ABI. Source banks and reached callees are required; no successful missing
provider or native position/contact substitute is installed.
"""
import generate_unit_animation_selection_runtime_1_1 as generator

generator.ENTRIES=(0xdefc0,0xde6d0,0xdef10,0xdee00,0xe46f0,0xe5cb0,
                   0xe5da0,0xe45d0,0xe5960,0xe23f0,0xbbe70,0x1efef0,
                   0x1d37e0,0x1d4260,0x1ea230,0x143860,0x11db80,
                   0x11da40,0x11d950,0xbb050)
generator.EXTRA_TABLES={0xde8e4:6}
generator.RUNTIME_HEADER='halo2/biped_postphysics_phase.hpp'
generator.RUNTIME_FUNCTION='source_run_biped_postphysics_phase_bodies_1_1'
generator.CONFIGURATION_TOOL=__file__
if __name__=='__main__':generator.main()
