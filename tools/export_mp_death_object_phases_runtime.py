#!/usr/bin/env python3
"""Reexecute complete original object phases and export native comparisons.

Every original store is checked by independent original reexecution. Native
comparisons retain gameplay stores; private host/Unicorn FP ABI is a separate
boundary. No lower gameplay owner, pose, contact or solver result is replayed.
"""
import export_mp_unit_animation_pose_runtime as exporter
exporter.ROOTS={0x13a380,0xb7e00,0x1c7660,0xb7f50,0xb8040}
for entry in exporter.ROOTS:exporter.ARGUMENT_COUNTS[entry]=0
exporter.CAPTURE_ENTRY='original_death_object_phases'
exporter.CAPTURE_ORACLE='oracle_mp_death_object_phases_1_1.py'
exporter.CONFIGURATION_TOOL=__file__
exporter.__doc__=__doc__
if __name__=='__main__':exporter.main()
