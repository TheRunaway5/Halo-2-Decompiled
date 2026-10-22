#!/usr/bin/env python3
"""Retain complete TU1.1/u5 instruction correspondence for recent damage.

Reuses the existing byte/branch/absolute operand comparison owner. Matching
instruction shapes locate code; each external/writable operand remains named
evidence, not a claim about its live runtime producer.
"""
from research_mp_live_combat_updates import main

if __name__ == '__main__':
    main(groups={'recent_damage': (0xda150, 0xd73e0, 0xdac40, 0xdaa90)},
         source_files=('src/gameplay/units/damage_feedback.cpp', 'src/gameplay/units/unit_damage.cpp',
                       'src/gameplay/simulation.cpp', 'include/halo2/damage_feedback.hpp'),
         configuration_tool='tools/research_damage_feedback_updates.py')
