#!/usr/bin/env python3
"""Statically lower the authenticated original melee-physics body and initializer."""
from pathlib import Path
import generate_unit_animation_selection_runtime_1_1 as compiler
compiler.ENTRIES=(0x30bd0,0x1ef030,0x1ef290)
compiler.ROOT=Path('/home/eric/.local/share/halo2-native/cache-reset-backups/20261012T201413Z')
compiler.RUNTIME_FUNCTION='source_run_melee_physics_1_1'
compiler.CONFIGURATION_TOOL=__file__
if __name__=='__main__': compiler.main()
