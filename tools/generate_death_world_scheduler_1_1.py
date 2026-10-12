#!/usr/bin/env python3
"""Compile the complete original world tick and deferred-body scheduling owners.

Actual world, garbage collector, retained thread and component banks remain
caller-owned. This connects native owners; it does not invent world readiness,
solver/contact answers or missing virtual methods.
"""
import generate_unit_animation_selection_runtime_1_1 as generator

generator.ENTRIES=(0x1c7660,0x1c7b90,0x1c8350,0x2ef630,0x27fa50,
    0x27f190,0x148360,0x148740,0x148510,0x1c7970,0x1c7ec0,
    0x1c7e90,0x1c7860,0x22ecb0,0x1d4c50,0x1d33e0,0x1d3bb0,
    0x1c70d0,0x1c7150,0x312270,0x3122b0,0x314b30)
generator.RUNTIME_HEADER='halo2/death_world_scheduler.hpp'
generator.RUNTIME_FUNCTION='source_run_death_world_scheduler_bodies_1_1'
generator.CONFIGURATION_TOOL=__file__
if __name__=='__main__':generator.main()
