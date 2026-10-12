#!/usr/bin/env python3
"""Compile the complete original physical-contact refresh and state dispatcher.

The same authenticated instruction lowering is shared with the animation
owners. Every geometry, object and physics dependency keeps its actual ABI;
no contact, support object, physical mode or success result is synthesized.
"""
import generate_unit_animation_selection_runtime_1_1 as generator

generator.ENTRIES=(0x1218c0,0x1e93f0,0x1f0e90,0x1f0d30,0x1d5600,0x3121a0)
generator.EXTRA_TABLES={0x1e9464:6}
generator.RUNTIME_HEADER='halo2/biped_postphysics_runtime.hpp'
generator.RUNTIME_FUNCTION='source_run_biped_postphysics_runtime_1_1'
generator.CONFIGURATION_TOOL=__file__
if __name__=='__main__':generator.main()
